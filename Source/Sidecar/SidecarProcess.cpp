#include "SidecarProcess.h"

#if JUCE_WINDOWS
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
 #endif
 #include <windows.h>
#endif

namespace sp
{
   #if JUCE_WINDOWS
    struct SidecarProcess::Impl
    {
        HANDLE hStdinWrite  = nullptr;
        HANDLE hStdoutRead  = nullptr;
        HANDLE hStderrRead  = nullptr;
        PROCESS_INFORMATION pi {};

        ~Impl() { closeAll(); }

        void closeAll() noexcept
        {
            if (hStdinWrite)  { CloseHandle (hStdinWrite);  hStdinWrite  = nullptr; }
            if (hStdoutRead)  { CloseHandle (hStdoutRead);  hStdoutRead  = nullptr; }
            if (hStderrRead)  { CloseHandle (hStderrRead);  hStderrRead  = nullptr; }
            if (pi.hProcess)  { CloseHandle (pi.hProcess);  pi.hProcess  = nullptr; }
            if (pi.hThread)   { CloseHandle (pi.hThread);   pi.hThread   = nullptr; }
        }
    };
   #else
    struct SidecarProcess::Impl {};  // TODO: posix_spawn + pipe() for macOS
   #endif

    class SidecarProcess::ReaderThread : public juce::Thread
    {
    public:
        ReaderThread (const juce::String& nm,
                      void* readHandle,
                      juce::WeakReference<SidecarProcess> ownerRef,
                      bool isStdout)
            : juce::Thread (nm), handle (readHandle), owner (ownerRef), stdoutChannel (isStdout) {}

        void run() override
        {
           #if JUCE_WINDOWS
            char buf[4096];
            juce::MemoryBlock accum;

            while (! threadShouldExit())
            {
                DWORD bytesRead = 0;
                BOOL ok = ReadFile ((HANDLE) handle, buf, (DWORD) sizeof (buf), &bytesRead, nullptr);
                if (! ok || bytesRead == 0) break;  // EOF / broken pipe

                accum.append (buf, (std::size_t) bytesRead);

                if (stdoutChannel)
                {
                    // Split on \n; keep partial tail in accum.
                    const char* data = (const char*) accum.getData();
                    std::size_t len = accum.getSize();
                    std::size_t start = 0;
                    for (std::size_t i = 0; i < len; ++i)
                    {
                        if (data[i] == '\n')
                        {
                            std::size_t end = i;
                            if (end > start && data[end - 1] == '\r') --end;
                            juce::String line (juce::CharPointer_UTF8 (data + start),
                                               juce::CharPointer_UTF8 (data + end));
                            dispatchLine (std::move (line));
                            start = i + 1;
                        }
                    }
                    if (start > 0)
                    {
                        juce::MemoryBlock rem;
                        rem.append (data + start, len - start);
                        accum = std::move (rem);
                    }
                }
                else
                {
                    juce::String chunk (juce::CharPointer_UTF8 ((const char*) accum.getData()),
                                        juce::CharPointer_UTF8 ((const char*) accum.getData() + accum.getSize()));
                    accum.reset();
                    dispatchStderr (std::move (chunk));
                }
            }

            // stdout EOF == child closed stdout. Report exit (once, from stdout reader only).
            if (stdoutChannel)
            {
                int code = 0;
               #if JUCE_WINDOWS
                if (auto strong = owner.get())
                    if (strong->impl && strong->impl->pi.hProcess)
                    {
                        WaitForSingleObject (strong->impl->pi.hProcess, 500);
                        DWORD ec = 0;
                        if (GetExitCodeProcess (strong->impl->pi.hProcess, &ec)) code = (int) ec;
                    }
               #endif
                auto ref = owner;
                juce::MessageManager::callAsync ([ref, code]
                {
                    if (auto* s = ref.get()) s->dispatchExit (code);
                });
            }
           #else
            juce::ignoreUnused (stdoutChannel);
           #endif
        }

    private:
        void dispatchLine (juce::String line)
        {
            auto ref = owner;
            juce::MessageManager::callAsync ([ref, line = std::move (line)]
            {
                if (auto* s = ref.get())
                    if (s->onLineReceived) s->onLineReceived (line);
            });
        }

        void dispatchStderr (juce::String chunk)
        {
            auto ref = owner;
            juce::MessageManager::callAsync ([ref, chunk = std::move (chunk)]
            {
                if (auto* s = ref.get())
                    if (s->onStderr) s->onStderr (chunk);
            });
        }

        void* handle;
        juce::WeakReference<SidecarProcess> owner;
        bool stdoutChannel;
    };

    SidecarProcess::SidecarProcess() : impl (std::make_unique<Impl>()) {}
    SidecarProcess::~SidecarProcess() { stop(); }

    bool SidecarProcess::isRunning() const noexcept
    {
       #if JUCE_WINDOWS
        if (! impl || ! impl->pi.hProcess) return false;
        DWORD ec = 0;
        if (! GetExitCodeProcess (impl->pi.hProcess, &ec)) return false;
        return ec == STILL_ACTIVE;
       #else
        return false;
       #endif
    }

    void SidecarProcess::dispatchExit (int code)
    {
        if (onExit) onExit (code);
    }

   #if JUCE_WINDOWS
    static juce::String quoteArg (const juce::String& a)
    {
        if (! a.containsAnyOf (" \t\"")) return a;
        juce::String out = "\"";
        for (auto c : a)
        {
            if (c == '"') out += "\\\"";
            else          out += c;
        }
        out += "\"";
        return out;
    }
   #endif

    bool SidecarProcess::start (const juce::StringArray& command)
    {
        stop();
        lastError = {};

        if (command.isEmpty()) { lastError = "no command"; return false; }

       #if JUCE_WINDOWS
        SECURITY_ATTRIBUTES sa { sizeof (SECURITY_ATTRIBUTES), nullptr, TRUE };

        HANDLE childStdinRead = nullptr, childStdoutWrite = nullptr, childStderrWrite = nullptr;

        auto fail = [&] (const char* where)
        {
            lastError = juce::String (where) + " failed (GetLastError=" + juce::String ((int) GetLastError()) + ")";
            if (childStdinRead)    CloseHandle (childStdinRead);
            if (childStdoutWrite)  CloseHandle (childStdoutWrite);
            if (childStderrWrite)  CloseHandle (childStderrWrite);
            impl->closeAll();
            return false;
        };

        if (! CreatePipe (&childStdinRead,  &impl->hStdinWrite, &sa, 0))                  return fail ("CreatePipe stdin");
        if (! SetHandleInformation (impl->hStdinWrite,  HANDLE_FLAG_INHERIT, 0))          return fail ("SetHandleInformation stdin");
        if (! CreatePipe (&impl->hStdoutRead, &childStdoutWrite, &sa, 0))                 return fail ("CreatePipe stdout");
        if (! SetHandleInformation (impl->hStdoutRead, HANDLE_FLAG_INHERIT, 0))           return fail ("SetHandleInformation stdout");
        if (! CreatePipe (&impl->hStderrRead, &childStderrWrite, &sa, 0))                 return fail ("CreatePipe stderr");
        if (! SetHandleInformation (impl->hStderrRead, HANDLE_FLAG_INHERIT, 0))           return fail ("SetHandleInformation stderr");

        STARTUPINFOW si {};
        si.cb         = sizeof (si);
        si.dwFlags    = STARTF_USESTDHANDLES;
        si.hStdInput  = childStdinRead;
        si.hStdOutput = childStdoutWrite;
        si.hStdError  = childStderrWrite;

        juce::String cmdLine;
        for (int i = 0; i < command.size(); ++i)
        {
            if (i > 0) cmdLine += " ";
            cmdLine += quoteArg (command[i]);
        }
        auto wide = cmdLine.toWideCharPointer();
        std::vector<wchar_t> mutableCmd (wide, wide + wcslen (wide) + 1);

        BOOL ok = CreateProcessW (nullptr,
                                  mutableCmd.data(),
                                  nullptr, nullptr,
                                  TRUE,                            // inherit handles
                                  CREATE_NO_WINDOW,
                                  nullptr, nullptr,
                                  &si, &impl->pi);

        // Parent doesn't need the child-side handles.
        CloseHandle (childStdinRead);
        CloseHandle (childStdoutWrite);
        CloseHandle (childStderrWrite);

        if (! ok) return fail ("CreateProcessW");

        juce::WeakReference<SidecarProcess> selfRef (this);
        stdoutReader = std::make_unique<ReaderThread> ("sidecar-stdout", impl->hStdoutRead, selfRef, true);
        stderrReader = std::make_unique<ReaderThread> ("sidecar-stderr", impl->hStderrRead, selfRef, false);
        stdoutReader->startThread();
        stderrReader->startThread();
        return true;
       #else
        lastError = "sidecar transport not implemented on this platform";
        return false;
       #endif
    }

    void SidecarProcess::stop()
    {
       #if JUCE_WINDOWS
        if (! impl) return;

        // Closing stdin signals EOF; well-behaved children exit on it.
        if (impl->hStdinWrite) { CloseHandle (impl->hStdinWrite); impl->hStdinWrite = nullptr; }

        if (impl->pi.hProcess)
        {
            if (WaitForSingleObject (impl->pi.hProcess, 1500) != WAIT_OBJECT_0)
                TerminateProcess (impl->pi.hProcess, 1);
        }

        if (stdoutReader) { stdoutReader->signalThreadShouldExit(); stdoutReader->waitForThreadToExit (1000); stdoutReader.reset(); }
        if (stderrReader) { stderrReader->signalThreadShouldExit(); stderrReader->waitForThreadToExit (1000); stderrReader.reset(); }

        impl->closeAll();
       #endif
    }

    bool SidecarProcess::sendLine (const juce::String& line)
    {
       #if JUCE_WINDOWS
        if (! impl || impl->hStdinWrite == nullptr) return false;

        juce::String payload = line;
        if (! payload.endsWithChar ('\n')) payload += "\n";
        const auto utf8 = payload.toRawUTF8();
        const DWORD len = (DWORD) std::strlen (utf8);

        const juce::ScopedLock sl (writeLock);
        DWORD written = 0;
        DWORD off = 0;
        while (off < len)
        {
            if (! WriteFile (impl->hStdinWrite, utf8 + off, len - off, &written, nullptr)) return false;
            off += written;
        }
        return true;
       #else
        juce::ignoreUnused (line);
        return false;
       #endif
    }

    juce::StringArray SidecarProcess::getDefaultCommand()
    {
        auto exe = juce::File::getSpecialLocation (juce::File::currentExecutableFile);

       #if JUCE_WINDOWS
        auto binary = exe.getParentDirectory().getChildFile ("practiceml.exe");
       #else
        auto binary = exe.getParentDirectory().getChildFile ("practiceml");
       #endif

        if (binary.existsAsFile())
            return { binary.getFullPathName() };

        // Dev fallback: walk up looking for sidecar/. Prefer the venv interpreter
        // (.venv/Scripts/python.exe on Windows, .venv/bin/python on Unix) so ML deps
        // resolve against the project's isolated environment.
        auto search = exe.getParentDirectory();
        for (int i = 0; i < 10 && search != juce::File(); ++i, search = search.getParentDirectory())
        {
            auto sidecarDir = search.getChildFile ("sidecar");
            auto entry = sidecarDir.getChildFile ("practiceml/__main__.py");
            if (! entry.existsAsFile()) continue;

           #if JUCE_WINDOWS
            auto venvPy = sidecarDir.getChildFile (".venv/Scripts/python.exe");
           #else
            auto venvPy = sidecarDir.getChildFile (".venv/bin/python");
           #endif
            const juce::String pythonExe = venvPy.existsAsFile()
                ? venvPy.getFullPathName()
                : juce::String ("python");
            return { pythonExe, "-u", entry.getFullPathName() };
        }
        return {};
    }
}
