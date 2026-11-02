
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

        break;
    }
    case SKSE::MessagingInterface::kDataLoaded:
    {
        VCD::Manager::GetSingleton().LoadPresets();

        VCD::LoadTransformations();

        Settings::Load();

        VCD::Race::LoadRegisteredRaces();

        EventSinks::InputEventSink::RegisterEventSink();


        auto* filter = RE::bhkCollisionFilter::GetSingleton();
        if (!filter) {
            logger::error("Cannot capture collision masks: bhkCollisionFilter is null.");
            return;
        }

        logger::info("========== VANILLA COLLISION LAYER MASKS ==========");

        for (std::size_t i = 0; i < std::size(filter->layerBitfields); ++i) {
            const auto layer = static_cast<RE::COL_LAYER>(i);

            logger::info(
                "[{}] {} = 0x{:08X}",
                i,
                RE::CollisionLayerToString(layer),
                filter->layerBitfields[i]);
        }

        logger::info("========== END COLLISION LAYER MASKS ==========");

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
