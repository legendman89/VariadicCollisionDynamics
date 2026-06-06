
#include "plugin.hpp"
#include "logger.hpp"
#include "eventSinks.hpp"
#include "manager.hpp"
#include "Menu.hpp"
#include "TrueHUDAPI.h"
#include "hooks.hpp"
#include "globals.hpp"

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
        if (!VCD::Manager::GetSingleton().AreAllPresetsLoaded()) {
            logger::error("Presets Are Not Loaded Cant Continue");
        }

        EventSinks::InstallEventSinks();

        break;
    }
    default:
        break;
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);
    setupLog(spdlog::level::debug);
    logger::info("Variadic Collision Dynamics Plugin is Loaded");
    SKSE::GetMessagingInterface()->RegisterListener(MessageHandler);
    UI::Register(); 
    hooks::PlayerCharacter_Update::Install();
    auto* api = TRUEHUD_API::RequestPluginAPI(TRUEHUD_API::InterfaceVersion::V4);

    if (!api) {
        logger::error("TrueHUD API failed to initialize (V4 not available)");
    }
    else {
        globals::g_trueHUD = static_cast<TRUEHUD_API::IVTrueHUD4*>(api);
        logger::info("TrueHUD API acquired successfully");
    }
    return true;
}
