#include "config.hpp"

using json = nlohmann::json;

namespace VCD {

    namespace PresetJSON {
        bool ReadVec3(Vec3& a_out, const json& a_data)
        {
            if (a_data.is_array()) {
                if (a_data.size() > 0) {
                    a_out.x = a_data[0].get<float>();
                }

                if (a_data.size() > 1) {
                    a_out.y = a_data[1].get<float>();
                }

                if (a_data.size() > 2) {
                    a_out.z = a_data[2].get<float>();
                }

                return true;
            }

            if (!a_data.is_object()) {
                return false;
            }

            if (a_data.contains("x")) {
                a_out.x = a_data["x"].get<float>();
            }

            if (a_data.contains("y")) {
                a_out.y = a_data["y"].get<float>();
            }

            if (a_data.contains("z")) {
                a_out.z = a_data["z"].get<float>();
            }

            return true;
        }

        bool ReadNiPoint3(RE::NiPoint3& a_out, const json& a_data)
        {
            if (a_data.is_array()) {
                if (a_data.size() > 0) {
                    a_out.x = a_data[0].get<float>();
                }

                if (a_data.size() > 1) {
                    a_out.y = a_data[1].get<float>();
                }

                if (a_data.size() > 2) {
                    a_out.z = a_data[2].get<float>();
                }

                return true;
            }

            if (!a_data.is_object()) {
                return false;
            }

            if (a_data.contains("x")) {
                a_out.x = a_data["x"].get<float>();
            }

            if (a_data.contains("y")) {
                a_out.y = a_data["y"].get<float>();
            }

            if (a_data.contains("z")) {
                a_out.z = a_data["z"].get<float>();
            }

            return true;
        }

        Preset PresetFromFileName(const std::string& a_path)
        {
            const auto stem = fs::path(a_path).stem().string();

            if (stem == "PersonalSpace") {
                return Preset::kPersonalSpace;
            }

            if (stem == "Compact") {
                return Preset::kCompact;
            }

            if (stem == "Bulky") {
                return Preset::kBulky;
            }

            return Preset::kVanillaLike;
        }
    }

    bool LoadPresetConfiguration(PresetConfig& a_preset, const json& a_data)
    {
        try {
            if (a_data.contains("name")) {
                a_preset.name = a_data["name"].get<std::string>();
            }

            if (a_data.contains("position")) {
                PresetJSON::ReadNiPoint3(a_preset.data.bump.translation, a_data["position"]);
            }

            if (a_data.contains("radius")) {
                a_preset.data.capsule.radius = a_data["radius"].get<float>();
            }

            if (a_data.contains("firstPoint")) {
                PresetJSON::ReadVec3(a_preset.data.capsule.point1, a_data["firstPoint"]);
            }

            if (a_data.contains("secondPoint")) {
                PresetJSON::ReadVec3(a_preset.data.capsule.point2, a_data["secondPoint"]);
            }

            a_preset.data.RecalculateHeight();
            a_preset.loaded = true;
            return true;
        }
        catch (const json::exception& e) {
            logger::error("cannot read preset JSON object due to {}", e.what());
            return false;
        }
    }

    bool LoadPresetConfigurations(std::array<PresetConfig, 4>& a_presets)
    {
        logger::info("Parsing collision presets..");

        auto paths = GetPresetPaths();
        if (paths.empty()) {
            return false;
        }

        std::size_t loadedCount = 0;

        for (const auto& path : paths) {
            logger::info("Reading {}", path);

            std::ifstream presetFile(path);
            if (!presetFile.is_open()) {
                logger::error("Failed to open preset file: {}", path);
                continue;
            }

            std::string raw((std::istreambuf_iterator<char>(presetFile)), std::istreambuf_iterator<char>());
            json data;
            try {
                data = json::parse(raw, nullptr, true, true);
            }
            catch (const json::exception& e) {
                logger::error("Failed to parse preset file {}: {}", path, e.what());
                continue;
            }

            json entries;

            if (data.is_array()) {
                entries = data;
            }
            else if (data.is_object()) {
                entries = json::array({ data });
            }
            else {
                logger::error("Invalid JSON root in {}", path);
                continue;
            }

            for (auto entry : entries) {
                const auto preset = PresetJSON::PresetFromFileName(path);
                auto& presetConfig = a_presets[ToUnderlying(preset)];
                presetConfig.preset = preset;
                presetConfig.configPath = path;

                if (LoadPresetConfiguration(presetConfig, entry)) {
                    presetConfig.data.Log();
                    ++loadedCount;
                }
            }
        }

        logger::info("Preset parsing finished: {}/{} loaded successfully", loadedCount, a_presets.size());
        return loadedCount == a_presets.size();
    }
}
