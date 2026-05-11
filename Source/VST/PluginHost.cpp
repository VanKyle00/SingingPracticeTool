#include "PluginHost.h"

namespace sp
{
    PluginHost::PluginHost()
    {
        formats.addDefaultFormats();
        cache.load();
    }

    PluginHost::~PluginHost()
    {
        releaseResources();
    }

    int  PluginHost::getNumSlots() const
    {
        const juce::ScopedLock sl (chainLock);
        return (int) slots.size();
    }

    bool PluginHost::isBypassed (int index) const
    {
        const juce::ScopedLock sl (chainLock);
        if (! juce::isPositiveAndBelow (index, (int) slots.size())) return false;
        return slots[(std::size_t) index]->bypassed.load();
    }

    juce::String PluginHost::getSlotName (int index) const
    {
        const juce::ScopedLock sl (chainLock);
        if (! juce::isPositiveAndBelow (index, (int) slots.size())) return {};
        return slots[(std::size_t) index]->description.name;
    }

    juce::AudioPluginInstance* PluginHost::getInstance (int index)
    {
        const juce::ScopedLock sl (chainLock);
        if (! juce::isPositiveAndBelow (index, (int) slots.size())) return nullptr;
        return slots[(std::size_t) index]->instance.get();
    }

    void PluginHost::setBypass (int index, bool bypass)
    {
        const juce::ScopedLock sl (chainLock);
        if (juce::isPositiveAndBelow (index, (int) slots.size()))
            slots[(std::size_t) index]->bypassed.store (bypass);
    }

    void PluginHost::removePlugin (int index)
    {
        std::unique_ptr<Slot> removed;
        {
            const juce::ScopedLock sl (chainLock);
            if (! juce::isPositiveAndBelow (index, (int) slots.size())) return;
            removed = std::move (slots[(std::size_t) index]);
            slots.erase (slots.begin() + index);
        }
        // Release plugin resources outside the lock (calls into plugin code).
        if (removed && removed->instance)
            removed->instance->releaseResources();
        notifyChanged();
    }

    void PluginHost::moveSlot (int from, int to)
    {
        {
            const juce::ScopedLock sl (chainLock);
            if (! juce::isPositiveAndBelow (from, (int) slots.size())) return;
            if (! juce::isPositiveAndBelow (to,   (int) slots.size())) return;
            auto moving = std::move (slots[(std::size_t) from]);
            slots.erase (slots.begin() + from);
            slots.insert (slots.begin() + to, std::move (moving));
        }
        notifyChanged();
    }

    void PluginHost::prepareToPlay (double sampleRate, int blockSize)
    {
        const juce::ScopedLock sl (chainLock);
        currentSampleRate = sampleRate;
        currentBlockSize  = blockSize;
        for (auto& s : slots)
            if (s && s->instance)
                prepareSlot (*s);
    }

    void PluginHost::releaseResources()
    {
        const juce::ScopedLock sl (chainLock);
        for (auto& s : slots)
            if (s && s->instance)
                s->instance->releaseResources();
    }

    void PluginHost::prepareSlot (Slot& s)
    {
        // Stereo vocal bus: 2 in / 2 out.
        s.instance->setPlayConfigDetails (2, 2, currentSampleRate, currentBlockSize);
        s.instance->prepareToPlay (currentSampleRate, currentBlockSize);
    }

    void PluginHost::addPluginFromFileAsync (const juce::File& pluginFile,
                                             std::function<void (Slot*, const juce::String&)> onDone)
    {
        // 1) Scan the file for plugin types.
        juce::OwnedArray<juce::PluginDescription> found;
        for (auto* format : formats.getFormats())
            format->findAllTypesForFile (found, pluginFile.getFullPathName());

        if (found.isEmpty())
        {
            if (onDone) onDone (nullptr, "No plugins found in " + pluginFile.getFileName());
            return;
        }

        // 2) Async instance creation (multi-architecture plugins can take a moment).
        const auto desc = *found[0];
        formats.createPluginInstanceAsync (
            desc, currentSampleRate, currentBlockSize,
            [this, desc, cb = std::move (onDone)] (std::unique_ptr<juce::AudioPluginInstance> inst,
                                                   const juce::String& err)
            {
                if (! inst)
                {
                    if (cb) cb (nullptr, err.isEmpty() ? "Plugin creation failed" : err);
                    return;
                }

                auto slot = std::make_unique<Slot>();
                slot->description = desc;
                slot->instance    = std::move (inst);

                Slot* raw = slot.get();
                {
                    const juce::ScopedLock sl (chainLock);
                    if (currentSampleRate > 0.0 && currentBlockSize > 0)
                        prepareSlot (*slot);
                    slots.push_back (std::move (slot));
                    cache.getList().addType (desc);
                }
                cache.save();
                notifyChanged();
                if (cb) cb (raw, {});
            });
    }

    void PluginHost::notifyChanged()
    {
        if (onChainChanged)
            juce::MessageManager::callAsync (onChainChanged);
    }

    void PluginHost::processBlock (juce::AudioBuffer<float>& buffer) noexcept
    {
        const juce::ScopedTryLock sl (chainLock);
        if (! sl.isLocked()) return;  // Mid-edit: pass dry this block.

        for (auto& s : slots)
        {
            if (! s || ! s->instance || s->bypassed.load()) continue;
            s->instance->processBlock (buffer, emptyMidi);
        }
    }
}
