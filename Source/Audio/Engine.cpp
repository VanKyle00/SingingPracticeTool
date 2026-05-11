#include "Engine.h"
#include <cmath>

namespace sp
{
    Engine::Engine()
    {
        formatManager.registerBasicFormats();

        loadPersistedDeviceState();

        deviceManager.addAudioCallback (this);
        deviceManager.addChangeListener (this);   // persist on device/driver/buffer changes

        transportThread.startThread();
    }

    Engine::~Engine()
    {
        deviceManager.removeChangeListener (this);
        savePersistedDeviceState();               // belt: save on shutdown regardless

        deviceManager.removeAudioCallback (this);
        transport.setSource (nullptr);
        readerSource.reset();
        transportThread.stopThread (500);
    }

    juce::File Engine::getDeviceSettingsFile()
    {
        return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("SingingPracticeTool")
                   .getChildFile ("device_settings.xml");
    }

    void Engine::loadPersistedDeviceState()
    {
        std::unique_ptr<juce::XmlElement> state;
        if (auto f = getDeviceSettingsFile(); f.existsAsFile())
            state = juce::XmlDocument::parse (f);

        // initialise() accepts a saved state and falls back to defaults if it's null or invalid.
        auto err = deviceManager.initialise (1, 2, state.get(), true);
        if (err.isNotEmpty())
            juce::Logger::writeToLog ("AudioDeviceManager init: " + err);
    }

    void Engine::savePersistedDeviceState()
    {
        auto state = deviceManager.createStateXml();
        if (state == nullptr) return;

        // AudioDeviceManager broadcasts in bursts (driver/buffer/sample-rate changes
        // each arrive separately); skip rewriting the file when the serialised state
        // hasn't actually changed.
        const auto xml = state->toString();
        if (xml == lastWrittenDeviceXml) return;

        const auto f = getDeviceSettingsFile();
        f.getParentDirectory().createDirectory();
        if (f.replaceWithText (xml))
            lastWrittenDeviceXml = xml;
    }

    void Engine::changeListenerCallback (juce::ChangeBroadcaster*)
    {
        savePersistedDeviceState();
    }

    bool Engine::loadInstrumental (const juce::File& file)
    {
        if (! file.existsAsFile()) return false;

        std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
        if (reader == nullptr) return false;

        const double srcSr = reader->sampleRate;
        auto newSource = std::make_unique<juce::AudioFormatReaderSource> (reader.release(), true);
        newSource->setLooping (false);

        const double sr = currentSampleRate.load() > 0.0 ? currentSampleRate.load() : 48000.0;
        const int    bs = currentBufferSize.load() > 0 ? currentBufferSize.load() : 512;

        {
            const juce::ScopedLock sl (transportLock);
            transport.stop();
            transport.setSource (nullptr);
            readerSource = std::move (newSource);
            transport.setSource (readerSource.get(), 32768, &transportThread, srcSr, 2);
            transport.prepareToPlay (bs, sr);
            transport.setPosition (0.0);
        }
        return true;
    }

    void Engine::unloadInstrumental()
    {
        const juce::ScopedLock sl (transportLock);
        transport.stop();
        transport.setSource (nullptr);
        readerSource.reset();
    }

    void Engine::play()
    {
        const juce::ScopedLock sl (transportLock);
        if (readerSource != nullptr)
            transport.start();
    }

    void Engine::stop()
    {
        const juce::ScopedLock sl (transportLock);
        transport.stop();
    }

    bool Engine::isPlaying() const noexcept
    {
        return transport.isPlaying();
    }

    bool Engine::hasInstrumental() const noexcept
    {
        return readerSource != nullptr;
    }

    double Engine::getEngineTimeSeconds() const noexcept
    {
        const double sr = currentSampleRate.load();
        if (sr <= 0.0) return 0.0;
        return (double) samplesProcessed.load (std::memory_order_relaxed) / sr;
    }

    double Engine::getPlaybackPositionSeconds() const noexcept
    {
        return transport.getCurrentPosition();
    }

    double Engine::getInstrumentalLengthSeconds() const noexcept
    {
        return transport.getLengthInSeconds();
    }

    void Engine::setPlaybackPositionSeconds (double seconds)
    {
        const juce::ScopedLock sl (transportLock);
        if (readerSource == nullptr) return;
        const double len = transport.getLengthInSeconds();
        transport.setPosition (juce::jlimit (0.0, juce::jmax (0.0, len), seconds));
    }

    double Engine::getRoundTripLatencyMs() const noexcept
    {
        const auto sr = currentSampleRate.load();
        if (sr <= 0.0) return 0.0;
        const auto total = inputLatencySamples.load() + outputLatencySamples.load() + currentBufferSize.load();
        return 1000.0 * (double) total / sr;
    }

    void Engine::startRecording (const juce::File& folder)
    {
        const auto stamp = juce::Time::getCurrentTime().formatted ("%Y%m%d_%H%M%S");
        const auto file  = folder.getChildFile ("vocal_dry_" + stamp + ".wav");
        recorder.start (file, currentSampleRate.load(), 1);
    }

    void Engine::stopRecording()
    {
        recorder.stop();
    }

    void Engine::audioDeviceAboutToStart (juce::AudioIODevice* device)
    {
        currentSampleRate.store (device->getCurrentSampleRate());
        currentBufferSize.store (device->getCurrentBufferSizeSamples());
        inputLatencySamples.store  (device->getInputLatencyInSamples());
        outputLatencySamples.store (device->getOutputLatencyInSamples());

        // Reset engine clock so it stays aligned with the pitch detector's clock
        // (PitchDetector::prepare resets its own sample counter). Without this, a
        // device change leaves the two clocks at different origins and the pitch
        // trace renders way outside the visible window.
        samplesProcessed.store (0, std::memory_order_relaxed);

        pitchDetector.prepare (currentSampleRate.load(), 1024);

        vocalScratch.setSize (2, currentBufferSize.load(), false, true, false);
        vocalChain.prepareToPlay (currentSampleRate.load(), currentBufferSize.load());

        const juce::ScopedLock sl (transportLock);
        transport.prepareToPlay (currentBufferSize.load(), currentSampleRate.load());
    }

    void Engine::audioDeviceStopped()
    {
        vocalChain.releaseResources();

        const juce::ScopedLock sl (transportLock);
        transport.releaseResources();
        recorder.stop();
    }

    void Engine::audioDeviceIOCallbackWithContext (const float* const* inputChannelData,
                                                   int numInputChannels,
                                                   float* const* outputChannelData,
                                                   int numOutputChannels,
                                                   int numSamples,
                                                   const juce::AudioIODeviceCallbackContext&)
    {
        samplesProcessed.fetch_add ((std::uint64_t) numSamples, std::memory_order_relaxed);

        const int sel = juce::jlimit (0, juce::jmax (0, numInputChannels - 1),
                                      selectedInputChannel.load());
        const float* micIn = (numInputChannels > 0) ? inputChannelData[sel] : nullptr;

        // Pitch + level
        if (micIn != nullptr)
        {
            pitchDetector.process (micIn, numSamples);

            float peak = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                peak = juce::jmax (peak, std::abs (micIn[i]));
            // Simple peak-follower (no smoothing — UI smooths if it wants to).
            inputLevel.store (peak);

            recorder.processInput (micIn, numSamples);
        }

        // Clear outputs.
        for (int ch = 0; ch < numOutputChannels; ++ch)
            if (outputChannelData[ch] != nullptr)
                juce::FloatVectorOperations::clear (outputChannelData[ch], numSamples);

        // Pull instrumental into outputs.
        {
            const juce::ScopedTryLock sl (transportLock);
            if (sl.isLocked() && readerSource != nullptr && transport.isPlaying() && numOutputChannels > 0)
            {
                juce::AudioBuffer<float> outBuf (outputChannelData, numOutputChannels, numSamples);
                juce::AudioSourceChannelInfo info (&outBuf, 0, numSamples);
                transport.getNextAudioBlock (info);

                const float ig = instrumentalGain.load();
                if (ig != 1.0f)
                    outBuf.applyGain (ig);
            }
        }

        // Vocal bus: mic -> stereo expand -> plugin chain -> monitor mix.
        const float vg = vocalGain.load();
        if (micIn != nullptr && vg > 0.0f && vocalScratch.getNumChannels() >= 2
            && vocalScratch.getNumSamples() >= numSamples)
        {
            float* l = vocalScratch.getWritePointer (0);
            float* r = vocalScratch.getWritePointer (1);
            juce::FloatVectorOperations::copy (l, micIn, numSamples);
            juce::FloatVectorOperations::copy (r, micIn, numSamples);

            juce::AudioBuffer<float> sub (vocalScratch.getArrayOfWritePointers(), 2, 0, numSamples);
            vocalChain.processBlock (sub);

            for (int ch = 0; ch < juce::jmin (numOutputChannels, 2); ++ch)
                if (outputChannelData[ch] != nullptr)
                    juce::FloatVectorOperations::addWithMultiply (
                        outputChannelData[ch], sub.getReadPointer (ch), vg, numSamples);
        }
    }
}
