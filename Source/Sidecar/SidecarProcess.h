#pragma once

#include <JuceHeader.h>
#include <functional>
#include <memory>

namespace sp
{
    // Spawns a child process and exchanges newline-terminated lines over its
    // stdin/stdout. juce::ChildProcess doesn't expose stdin writes, so this uses
    // raw OS pipes (Win32 here; macOS implementation is a TODO stub).
    class SidecarProcess
    {
    public:
        SidecarProcess();
        ~SidecarProcess();

        // Spawn the child. `command[0]` is the executable; remaining items are args.
        // Returns false on spawn failure (see lastError).
        bool start (const juce::StringArray& command);

        // Closes stdin (signals the child to exit), waits briefly, kills if needed.
        void stop();

        bool isRunning() const noexcept;
        juce::String getLastError() const   { return lastError; }

        // Thread-safe. Appends '\n' if missing. Returns false if not running or write fails.
        bool sendLine (const juce::String& line);

        // All callbacks fire on the JUCE message thread.
        std::function<void (const juce::String&)> onLineReceived;  // one complete stdout line, no \n
        std::function<void (const juce::String&)> onStderr;         // raw stderr chunk
        std::function<void (int exitCode)>        onExit;

        // Sensible default: practiceml[.exe] next to host exe, else `python -u <sidecar src>`.
        // Returns empty array if neither is locatable.
        static juce::StringArray getDefaultCommand();

    private:
        struct Impl;                          // platform-specific pipe + process handles
        std::unique_ptr<Impl> impl;

        class ReaderThread;
        std::unique_ptr<ReaderThread> stdoutReader;
        std::unique_ptr<ReaderThread> stderrReader;

        juce::CriticalSection writeLock;
        juce::String          lastError;

        void dispatchExit (int code);

        JUCE_DECLARE_WEAK_REFERENCEABLE (SidecarProcess)
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SidecarProcess)
    };
}
