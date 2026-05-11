#pragma once

#include <JuceHeader.h>
#include <functional>
#include <map>
#include <unordered_map>

namespace sp
{
    class SidecarProcess;

    // Line-delimited JSON-RPC 2.0 client. Stub.
    class JsonRpcClient
    {
    public:
        using ResponseHandler     = std::function<void (const juce::var& result, const juce::var& error)>;
        using NotificationHandler = std::function<void (const juce::var& params)>;

        explicit JsonRpcClient (SidecarProcess& proc);

        int  call (const juce::String& method, const juce::var& params, ResponseHandler onDone);
        void onNotification (const juce::String& method, NotificationHandler handler);

        // Feed raw stdout lines from the sidecar.
        void handleLine (const juce::String& line);

    private:
        SidecarProcess& proc;
        int nextId = 1;
        std::unordered_map<int, ResponseHandler>     pending;
        std::map<juce::String, NotificationHandler>  notifications;
    };
}
