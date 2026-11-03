
#include "menu.hpp"
#include "hooks.hpp"
#include "plugin.hpp"
#include "logger.hpp"
#include "manager.hpp"
#include "dynamics.hpp"
#include "settings.hpp"
#include "translate.hpp"
#include "transformations.hpp"
#include "eventsinks.hpp"

static void MessageHandler(SKSE::MessagingInterface::Message* msg) {
    switch (msg->type) {
    case SKSE::MessagingInterface::kPostLoad:
    {
        break;
    }
    case SKSE::MessagingInterface::kSaveGame:
    {
        break;
    }
    case SKSE::MessagingInterface::kPostLoadGame:
    case SKSE::MessagingInterface::kNewGame:
    {
        // Skyrim resets loaded preset on loading game,
        // Thus, we reapply after a short time.
        Dynamics::ClearRuntimeState();
        Dynamics::SchedulePostLoadApply();

        auto* filter = RE::bhkCollisionFilter::GetSingleton();
        if (!filter) {
            logger::error("Cannot modify collision masks: bhkCollisionFilter is null.");
            return;
        }

        constexpr auto weaponLayer = std::to_underlying(RE::COL_LAYER::kWeapon);
        const auto weaponBit = std::uint32_t{ 1 } << weaponLayer;

        break;
    }
    case SKSE::MessagingInterface::kDataLoaded:
    {
        VCD::Manager::GetSingleton().LoadPresets();

        VCD::LoadTransformations();

        Settings::Load();

        VCD::Race::LoadRegisteredRaces();

        EventSinks::InputEventSink::RegisterEventSink();

        break;
    }
    default:
        break;
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) 
{
    SKSE::Init(skse, false);

    setupLog(spdlog::level::debug);

    logger::info("{} v{} by {} (Game v{})", BEAUTIFUL_NAME, CURR_VERSION, AUTHOR_NAME, REL::Module::get().version().string("."));
    
    auto messaging = SKSE::GetMessagingInterface();
    if (!messaging || !messaging->RegisterListener(MessageHandler)) {
        logger::critical("Failed to register SKSE message listener");
        return false;
    }

    logger::info("SKSE message listener is registered successfully");

    Trans::GetTranslator().load();

    UI::Register(); 

    Hook::Install(); 

    return true;
}
