#pragma once

#include "eventsinks.hpp"
#include "logger.hpp"
#include "settings.hpp"
#include "dynamics.hpp"

#include "API.hpp"   // API::CustomKeyGlobals

namespace EventSinks
{
    namespace
    {
        // Mirrors the held flag into the state global, if it exists.
        void SyncStateGlobal()
        {
            auto* state = API::CustomKeyGlobals::g_vState;
            if (!state) {
                return;
            }
            state->value = Dynamics::GetCustomKeybindHeld().load() ? 1.0F : 0.0F;
        }

        // The lock only counts if the global exists and is currently 1.
        bool IsLocked()
        {
            const auto* lock = API::CustomKeyGlobals::g_vLock;
            return lock && lock->value == 1.0F;
        }
    }

    RE::BSEventNotifyControl InputEventSink::ProcessEvent(
        RE::InputEvent* const* a_event,
        RE::BSTEventSource<RE::InputEvent*>*)
    {
        if (!a_event || !*a_event)
            return RE::BSEventNotifyControl::kContinue;

        const auto keybind = static_cast<std::uint32_t>(Settings::GetSettings().customKeybind);

        for (auto event = *a_event; event; event = event->next)
        {
            if (event->eventType != RE::INPUT_EVENT_TYPE::kButton)
                continue;

            auto* buttonEvent = static_cast<RE::ButtonEvent*>(event);

            if (buttonEvent->GetDevice() != RE::INPUT_DEVICE::kKeyboard)
                continue;

            if (buttonEvent->GetIDCode() != keybind)
                continue;

            if (buttonEvent->IsDown()) {
                if (IsLocked()) {
                    logger::debug("VCD Custom Keybind with idCode {} blocked by lock global", buttonEvent->GetIDCode());
                }
                else {
                    Dynamics::GetCustomKeybindHeld().store(true);
                    logger::debug("VCD Custom Keybind with idCode {} is currently held down", buttonEvent->GetIDCode());
                }
                SyncStateGlobal();
            }
            else if (buttonEvent->IsUp()) {
                Dynamics::GetCustomKeybindHeld().store(false);
                logger::debug("VCD Custom Keybind with idCode {} is NOT currently held down", buttonEvent->GetIDCode());
                SyncStateGlobal();
            }
        }

        return RE::BSEventNotifyControl::kContinue;
    }

    void InputEventSink::RegisterEventSink()
    {
        auto inputEventSource =
            RE::BSInputDeviceManager::GetSingleton();

        if (!inputEventSource) {
            logger::error("Failed to get BSInputDeviceManager");
            return;
        }

        inputEventSource->AddEventSink(
            InputEventSink::GetSingleton());

        logger::info("InputEventSink registered");
    }
}