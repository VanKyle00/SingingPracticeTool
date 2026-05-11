#pragma once

#include <JuceHeader.h>
#include <functional>
#include <map>
#include <memory>
#include "SidecarProcess.h"
#include "JsonRpcClient.h"

namespace sp
{
    // Owns the sidecar process + JSON-RPC client and gives the UI a simple
    // typed surface to call ML methods. Lazy-starts the sidecar on first use.
    class Backend
    {
    public:
        Backend();
        ~Backend();

        // Starts the sidecar if not already. Returns false on failure (lastError set).
        bool ensureStarted();
        void shutdown();
        bool isRunning() const;

        juce::String  getLastError() const  { return process.getLastError(); }
        SidecarProcess& getProcess()        { return process; }
        JsonRpcClient&  getClient()         { return client; }

        // Status / diagnostics broadcast to the UI (latest stderr line, status text).
        std::function<void (const juce::String&)> onStderrLine;
        std::function<void (const juce::String&)> onStatus;
        std::function<void (int exitCode)>        onExit;

        // Typed wrappers. Result/error handlers fire on the message thread.
        void ping (std::function<void (bool ok, const juce::String& detail)> onDone);

        struct JobHandle
        {
            int rpcId = 0;
            std::function<void (float frac, const juce::String& text)> onProgress;
            std::function<void (bool ok, const juce::var& resultOrError)> onComplete;
        };

        // Long-running calls. Returns the rpc id; caller can pass it to cancel().
        // onProgress fires per progress notification; onComplete fires once with the
        // final result var (ok=true) or error message var (ok=false).
        int separateStems (const juce::File& input, const juce::File& outputDir,
                           const juce::String& model, bool vocalsOnly,
                           const juce::String& device,
                           JobHandle handlers);

        int youtubeDownload (const juce::String& url, const juce::File& outputDir,
                             const juce::String& audioFormat,
                             JobHandle handlers);

        int vocalToMidi (const juce::File& input, const juce::File& outputDir,
                         float onsetThreshold, float frameThreshold,
                         float minimumNoteLengthMs,
                         JobHandle handlers);

        void cancel (int rpcId);

    private:
        std::map<int, JobHandle> jobs;

        SidecarProcess process;
        JsonRpcClient  client;
        bool           startedOnce = false;
    };
}
