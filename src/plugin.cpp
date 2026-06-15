
#include "menu.hpp"
#include "hooks.hpp"
#include "plugin.hpp"
#include "logger.hpp"
#include "manager.hpp"
#include "dynamics.hpp"
#include "settings.hpp"

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
    case SKSE::MessagingInterface::kPreLoadGame:
    {
        break;
    }
    case SKSE::MessagingInterface::kPostLoadGame:
    {
        break;
    }
    case SKSE::MessagingInterface::kNewGame:
    {

        break;
    }
    case SKSE::MessagingInterface::kDataLoaded:
    {
        VCD::Manager::GetSingleton().LoadPresets();
        if (VCD::Manager::GetSingleton().AreAllPresetsLoaded()) {
            Settings::Load();
        }
		else {
            logger::error("Presets are not loaded. Mod is disabled.");
        }

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

    UI::Register(); 

    Hook::PlayerUpdate::Install();
    Hook::SneakHandlerCanProcess::Install(); 

    return true;
}
