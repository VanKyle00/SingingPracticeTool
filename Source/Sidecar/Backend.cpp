#include "Backend.h"

namespace sp
{
    Backend::Backend() : client (process)
    {
        process.onLineReceived = [this] (const juce::String& line) { client.handleLine (line); };
        process.onStderr = [this] (const juce::String& chunk)
        {
            if (onStderrLine) onStderrLine (chunk.trim());
        };
        process.onExit = [this] (int code)
        {
            if (onStatus) onStatus ("sidecar exited (code " + juce::String (code) + ")");
            if (onExit)   onExit (code);
            // Fail any pending jobs (rare; usually a crash).
            for (auto& [id, h] : jobs)
                if (h.onComplete) h.onComplete (false, juce::var ("sidecar exited"));
            jobs.clear();
        };

        // Route progress notifications to per-job handlers.
        client.onNotification ("progress", [this] (const juce::var& params)
        {
            const int id = (int) params["id"];
            auto it = jobs.find (id);
            if (it == jobs.end()) return;
            const float frac = (float) (double) params["frac"];
            const juce::String text = params["text"].toString();
            if (it->second.onProgress) it->second.onProgress (frac, text);
        });
    }

    Backend::~Backend()
    {
        shutdown();
    }

    bool Backend::ensureStarted()
    {
        if (process.isRunning()) return true;

        const auto cmd = SidecarProcess::getDefaultCommand();
        if (cmd.isEmpty())
        {
            if (onStatus) onStatus ("sidecar binary not found and no python source fallback located");
            return false;
        }

        const bool ok = process.start (cmd);
        startedOnce = true;
        if (ok)
        {
            if (onStatus) onStatus ("sidecar started: " + cmd.joinIntoString (" "));
        }
        else
        {
            if (onStatus) onStatus ("sidecar start failed: " + process.getLastError());
        }
        return ok;
    }

    void Backend::shutdown()
    {
        process.stop();
    }

    bool Backend::isRunning() const
    {
        return process.isRunning();
    }

    void Backend::ping (std::function<void (bool, const juce::String&)> onDone)
    {
        if (! ensureStarted())
        {
            if (onDone) onDone (false, process.getLastError());
            return;
        }

        client.call ("ping", juce::var (new juce::DynamicObject()),
            [cb = std::move (onDone)] (const juce::var& result, const juce::var& error)
            {
                if (! cb) return;
                if (! error.isVoid())
                    cb (false, juce::JSON::toString (error));
                else
                    cb (true, juce::JSON::toString (result));
            });
    }

    // Shared completion-handler factory: looks the JobHandle up by id once the call
    // has returned its id, then invokes onComplete with success/error and cleans up.
    static void dispatchCompletion (std::map<int, Backend::JobHandle>& jobs,
                                    int id,
                                    const juce::var& result,
                                    const juce::var& error)
    {
        auto it = jobs.find (id);
        if (it == jobs.end()) return;
        auto onComplete = it->second.onComplete;
        jobs.erase (it);
        if (onComplete)
        {
            if (! error.isVoid()) onComplete (false, error);
            else                  onComplete (true, result);
        }
    }

    int Backend::separateStems (const juce::File& input, const juce::File& outputDir,
                                const juce::String& model, bool vocalsOnly,
                                const juce::String& device,
                                JobHandle handlers)
    {
        if (! ensureStarted())
        {
            if (handlers.onComplete)
                handlers.onComplete (false, juce::var (process.getLastError()));
            return -1;
        }

        auto* obj = new juce::DynamicObject();
        obj->setProperty ("input",       input.getFullPathName());
        obj->setProperty ("output",      outputDir.getFullPathName());
        obj->setProperty ("model",       model);
        obj->setProperty ("vocals_only", vocalsOnly);
        obj->setProperty ("device",      device);

        auto idPtr = std::make_shared<int> (-1);
        const int id = client.call ("separate_stems", juce::var (obj),
            [this, idPtr] (const juce::var& result, const juce::var& error)
            {
                dispatchCompletion (jobs, *idPtr, result, error);
            });
        *idPtr = id;
        handlers.rpcId = id;
        jobs[id] = std::move (handlers);
        return id;
    }

    int Backend::youtubeDownload (const juce::String& url, const juce::File& outputDir,
                                  const juce::String& audioFormat, JobHandle handlers)
    {
        if (! ensureStarted())
        {
            if (handlers.onComplete)
                handlers.onComplete (false, juce::var (process.getLastError()));
            return -1;
        }

        auto* obj = new juce::DynamicObject();
        obj->setProperty ("url",          url);
        obj->setProperty ("output",       outputDir.getFullPathName());
        obj->setProperty ("audio_format", audioFormat);

        auto idPtr = std::make_shared<int> (-1);
        const int id = client.call ("youtube_download", juce::var (obj),
            [this, idPtr] (const juce::var& result, const juce::var& error)
            {
                dispatchCompletion (jobs, *idPtr, result, error);
            });
        *idPtr = id;
        handlers.rpcId = id;
        jobs[id] = std::move (handlers);
        return id;
    }

    int Backend::vocalToMidi (const juce::File& input, const juce::File& outputDir,
                              float onsetThreshold, float frameThreshold,
                              float minimumNoteLengthMs,
                              JobHandle handlers)
    {
        if (! ensureStarted())
        {
            if (handlers.onComplete)
                handlers.onComplete (false, juce::var (process.getLastError()));
            return -1;
        }

        auto* obj = new juce::DynamicObject();
        obj->setProperty ("input",                 input.getFullPathName());
        obj->setProperty ("output",                outputDir.getFullPathName());
        obj->setProperty ("onset_threshold",       onsetThreshold);
        obj->setProperty ("frame_threshold",       frameThreshold);
        obj->setProperty ("minimum_note_length_ms", minimumNoteLengthMs);

        auto idPtr = std::make_shared<int> (-1);
        const int id = client.call ("vocal_to_midi", juce::var (obj),
            [this, idPtr] (const juce::var& result, const juce::var& error)
            {
                dispatchCompletion (jobs, *idPtr, result, error);
            });
        *idPtr = id;
        handlers.rpcId = id;
        jobs[id] = std::move (handlers);
        return id;
    }

    void Backend::cancel (int rpcId)
    {
        if (rpcId < 0 || ! process.isRunning()) return;
        auto* obj = new juce::DynamicObject();
        obj->setProperty ("id", rpcId);
        client.call ("cancel", juce::var (obj), [] (const juce::var&, const juce::var&) {});
    }
}
