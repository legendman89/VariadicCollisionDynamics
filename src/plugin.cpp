
#include "plugin.hpp"
#include "logger.hpp"
#include "eventSinks.hpp"
#include "manager.hpp"

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
        VCD::Manager::GetSingleton().LoadPresetMeshes();
        if (!VCD::Manager::GetSingleton().AreAllPresetMeshesLoaded()) {
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
    return true;
}
