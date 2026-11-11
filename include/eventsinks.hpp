#pragma once

namespace EventSinks
{

	struct InputEventSink : RE::BSTEventSink<RE::InputEvent*>
	{
		static void RegisterEventSink();

		static InputEventSink* GetSingleton()
		{
			static InputEventSink singleton;
			return &singleton;
		}

	private:

		InputEventSink() = default;
		InputEventSink(const InputEventSink&) = delete;
		InputEventSink(InputEventSink&&) = delete;
		InputEventSink& operator=(const InputEventSink&) = delete;
		InputEventSink& operator=(InputEventSink&&) = delete;

		RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>* a_eventSource) override;
	};

	void SyncStateGlobal();

}