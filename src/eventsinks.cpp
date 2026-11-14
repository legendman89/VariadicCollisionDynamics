#include "eventsinks.hpp"
#include "logger.hpp"
#include "settings.hpp"
#include "dynamics.hpp"
#include "API.hpp"
#include "KeyCodes.hpp"

namespace EventSinks
{

	RE::BSEventNotifyControl InputEventSink::ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>*)
	{
		if (!a_event || !*a_event)
			return RE::BSEventNotifyControl::kContinue;

		const int keybind = Settings::GetSettings().customKeybind;
		if (keybind < 0)
			return RE::BSEventNotifyControl::kContinue;

		for (auto event = *a_event; event; event = event->next)
		{
			if (event->eventType != RE::INPUT_EVENT_TYPE::kButton)
				continue;

			auto* buttonEvent = static_cast<RE::ButtonEvent*>(event);
			const auto device = buttonEvent->GetDevice();
			const auto id = buttonEvent->GetIDCode();

			// Mouse wheel ids (8, 9) never send an IsUp, so they would leave the held flag stuck
			if (device == RE::INPUT_DEVICE::kMouse && id >= 8)
				continue;

			if (KeyNames::Encode(device, id) != keybind)
				continue;

			if (buttonEvent->IsDown()) {
				if (API::CustomKeyGlobals::IsCustomKeyBindPresetLocked()) {
					logger::debug("VCD Custom Keybind {} blocked by lock global", keybind);
				}
				else {
					Dynamics::GetCustomKeybindHeld().store(true);
					logger::debug("VCD Custom Keybind {} is currently held down", keybind);
				}
				API::CustomKeyGlobals::SyncStateGlobal();
			}
			else if (buttonEvent->IsUp()) {
				Dynamics::GetCustomKeybindHeld().store(false);
				logger::debug("VCD Custom Keybind {} is NOT currently held down", keybind);
				API::CustomKeyGlobals::SyncStateGlobal();
			}
		}

		return RE::BSEventNotifyControl::kContinue;
	}

    void InputEventSink::RegisterEventSink()
    {
        auto inputEventSource = RE::BSInputDeviceManager::GetSingleton();

        if (!inputEventSource) {
            logger::error("Failed to get BSInputDeviceManager");
            return;
        }

        inputEventSource->AddEventSink(InputEventSink::GetSingleton());

        logger::info("InputEventSink registered");
    }
}