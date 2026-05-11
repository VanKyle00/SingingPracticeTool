#include "JsonRpcClient.h"
#include "SidecarProcess.h"

namespace sp
{
    JsonRpcClient::JsonRpcClient (SidecarProcess& p) : proc (p) {}

    int JsonRpcClient::call (const juce::String& method, const juce::var& params, ResponseHandler onDone)
    {
        const int id = nextId++;

        auto* obj = new juce::DynamicObject();
        obj->setProperty ("jsonrpc", "2.0");
        obj->setProperty ("id",      id);
        obj->setProperty ("method",  method);
        obj->setProperty ("params",  params);

        juce::var msg (obj);
        const auto line = juce::JSON::toString (msg, true) + "\n";

        pending[id] = std::move (onDone);

        if (! proc.sendLine (line))
        {
            // Sidecar send not implemented yet — fail the call synchronously.
            auto h = std::move (pending[id]);
            pending.erase (id);
            if (h) h ({}, juce::var ("sidecar transport not available"));
        }
        return id;
    }

    void JsonRpcClient::onNotification (const juce::String& method, NotificationHandler h)
    {
        notifications[method] = std::move (h);
    }

    void JsonRpcClient::handleLine (const juce::String& line)
    {
        const auto parsed = juce::JSON::parse (line);
        if (! parsed.isObject()) return;

        if (parsed.hasProperty ("id"))
        {
            const int id = (int) parsed["id"];
            auto it = pending.find (id);
            if (it != pending.end())
            {
                auto h = std::move (it->second);
                pending.erase (it);
                h (parsed["result"], parsed["error"]);
            }
        }
        else if (parsed.hasProperty ("method"))
        {
            const auto method = parsed["method"].toString();
            auto it = notifications.find (method);
            if (it != notifications.end())
                it->second (parsed["params"]);
        }
    }
}
