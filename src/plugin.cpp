
#include "menu.hpp"
#include "hooks.hpp"
#include "plugin.hpp"
#include "logger.hpp"
#include "manager.hpp"
#include "dynamics.hpp"
#include "settings.hpp"
#include "translate.hpp"

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
        break;
    }
    case SKSE::MessagingInterface::kDataLoaded:
    {
        VCD::Manager::GetSingleton().LoadPresets();

        Settings::Load();

        break;
    }
    default:
        break;
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) 
{
    SKSE::Init(skse);

    setupLog(spdlog::level::debug);

    logger::info("Variadic Collision Dynamics Plugin is Loaded");

    SKSE::GetMessagingInterface()->RegisterListener(MessageHandler);

    Trans::GetTranslator().load();

    UI::Register(); 

    Hook::PlayerUpdate::Install();

    Hook::SneakHandlerProcessButton::Install(); 

    return true;
}
