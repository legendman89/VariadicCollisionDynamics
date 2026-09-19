#include "race.hpp"
#include "helper.hpp"
#include "json_file.hpp"

namespace VCD::Race {

    const RegisteredRace* FindRegisteredRace(const RE::Actor* a_actor)
    {
        const auto* race = a_actor ? a_actor->GetRace() : nullptr;
        if (race) {
            for (const auto& registeredRace : GetRaceRegistry().races) {
                if (registeredRace.race == race) {
                    return &registeredRace;
                }
            }
        }
        return nullptr;
    }

    void LoadRegisteredRaces()
    {
        auto& registry = GetRaceRegistry();
        registry.races.clear();
        registry.canSave = false;

        const auto path = GetRegisteredRacesPath();
        try {
            if (!fs::exists(path)) {
                registry.canSave = true;
                return;
            }

            nlohmann::json data;
            if (!JSON::ReadFile(path, data, "Registered races", false)) {
                return;
            }

            if (data.at("version").get<int>() != 1 || !data.at("races").is_array()) {
                logger::error("Unsupported RegisteredRaces.json format. Race registration is disabled.");
                return;
            }

            auto* handler = RE::TESDataHandler::GetSingleton();
            if (!handler) {
                return;
            }

            std::vector<RegisteredRace> registeredRaces;
            for (const auto& value : data.at("races")) {

                RegisteredRace registeredRace;
                registeredRace.plugin = value.at("plugin").get<std::string>();

                if (!value.at("localFormID").is_number_unsigned() || value.at("localFormID").get<uint64_t>() > 0xFFFFFF) {
                    logger::error("Invalid local race Form ID in RegisteredRaces.json");
                    return;
                }

                registeredRace.localFormID = value.at("localFormID").get<RE::FormID>();
                registeredRace.editorID = value.value("editorID", std::string{});
                registeredRace.presetKey = value.at("preset").get<std::string>();

                const auto limits = value.at("limits").get<std::string>();
                const auto* limit = FindRegisteredRaceLimit(limits, &RegisteredRaceLimit::key);
                if (!limit || registeredRace.plugin.empty() || registeredRace.presetKey.empty() || !registeredRace.localFormID) {
                    logger::error("Invalid race registration in RegisteredRaces.json");
                    return;
                }

                registeredRace.limitClass = limit->limitClass;
                registeredRace.race = handler->LookupForm<RE::TESRace>(registeredRace.localFormID, registeredRace.plugin);
                if (!registeredRace.race) {
                    logger::warn("Race not found: {} [{:06X}]. Saved registered race kept.", registeredRace.plugin, registeredRace.localFormID);
                }

                registeredRaces.push_back(std::move(registeredRace));
            }

            registry.races = std::move(registeredRaces);
            registry.canSave = true;

            logger::info("Loaded {} custom race registrations", registry.races.size());
        }
        catch (const std::exception& error) {
            logger::error("Could not load RegisteredRaces.json: {}", error.what());
        }
    }

    bool RegisterRace(RE::TESRace* a_race, Preset a_preset, CollisionLimitClass a_limitClass)
    {
        auto& registry = GetRaceRegistry();
        const auto* plugin = a_race ? a_race->GetFile(0) : nullptr;
        const auto* preset = Manager::GetSingleton().GetPresetConfig(a_preset);
        if (!registry.canSave || !plugin || !preset || Manager::GetSingleton().IsCameraPreset(preset->preset)) {
            return false;
        }

        if (!FindRegisteredRaceLimit(a_limitClass, &RegisteredRaceLimit::limitClass)) {
            return false;
        }

        const auto* editorID = a_race->GetFormEditorID();
        RegisteredRace registeredRace;
        registeredRace.plugin = plugin->fileName;
        registeredRace.localFormID = a_race->GetLocalFormID();
        registeredRace.editorID = editorID ? editorID : "";
        registeredRace.presetKey = preset->key;
        registeredRace.race = a_race;
        registeredRace.limitClass = a_limitClass;
        auto registeredRaces = registry.races;
        bool replaced = false;

        for (auto& existing : registeredRaces) {
            if (existing.race == a_race || (existing.plugin == registeredRace.plugin && existing.localFormID == registeredRace.localFormID)) {
                existing = registeredRace;
                replaced = true;
                break;
            }
        }

        if (!replaced) {
            registeredRaces.push_back(registeredRace);
        }

        try {
            auto data = nlohmann::json{ { "version", 1 }, { "races", nlohmann::json::array() } };
            for (const auto& saved : registeredRaces) {

                const auto* limit = FindRegisteredRaceLimit(saved.limitClass, &RegisteredRaceLimit::limitClass);
                const auto* limits = limit ? limit->key : kRegisteredRaceLimits.front().key;

                data["races"].push_back({ { "plugin", saved.plugin }, { "localFormID", saved.localFormID }, { "editorID", saved.editorID }, { "preset", saved.presetKey }, { "limits", limits } });
            }

            if (!JSON::WriteFile(GetRegisteredRacesPath(), data, "Registered races", nullptr, { 4, true })) {
                return false;
            }

            registry.races = std::move(registeredRaces);

            logger::info("Registered race {} from {} with preset {}", registeredRace.editorID, registeredRace.plugin, registeredRace.presetKey);
            
            return true;
        }
        catch (const std::exception& error) {
            logger::error("Could not register race: {}", error.what());
            return false;
        }
    }
}
