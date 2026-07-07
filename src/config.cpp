#include "config.hpp"
#include "json.hpp"

#include <algorithm>
#include <cctype>

using json = nlohmann::json;

namespace VCD {


    std::vector<fs::path> GetPresetPaths()
    {
        std::vector<fs::path> paths;
        const auto dir = GetPresetDir();

        std::error_code ec;
        if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec)) {
            return paths;
        }

        auto it = fs::recursive_directory_iterator(dir, fs::directory_options::skip_permission_denied, ec);
        fs::recursive_directory_iterator end;

        if (ec) {
            logger::critical("Cannot iterate over {}: {}", ToUTF8(dir), ec.message());
        }

        while (it != end) {
            const auto& path = it->path();

            if (fs::is_regular_file(path, ec) && path.extension() == ".json") {
                logger::info("Found preset file: {}", ToUTF8(path));
                paths.push_back(path);
            }

            ec.clear();
            it.increment(ec);

            if (ec) {
                logger::critical("Skipping path under {}: {}", ToUTF8(dir), ec.message());
                ec.clear();
            }
        }

        return paths;
    }

    bool LoadPresetConfiguration(PresetConfig& a_preset, const json& a_data)
    {
        try {
            if (a_data.contains("name")) {
                a_preset.name = a_data["name"].get<std::string>();
            }

            if (a_data.contains("position")) {
                JSON::ReadNiPoint3(a_preset.data.bump.translation, a_data["position"]);
            }

            if (a_data.contains("radius")) {
                a_preset.data.capsule.radius = a_data["radius"].get<float>();
            }

            if (a_data.contains("firstPoint")) {
                JSON::ReadVec3(a_preset.data.capsule.point1, a_data["firstPoint"]);
            }

            if (a_data.contains("secondPoint")) {
                JSON::ReadVec3(a_preset.data.capsule.point2, a_data["secondPoint"]);
            }

            a_preset.data.RecalculateHeight();
            a_preset.loaded = true;
            a_preset.fileBacked = true;
            return true;
        }
        catch (const json::exception& e) {
            logger::error("cannot read preset JSON object due to {}", e.what());
            return false;
        }
    }

    // a_presets is already initialized with built-in presets.
    // Found custom presets will be appended to it.
    bool LoadPresetConfigurations(std::vector<PresetConfig>& a_presets)
    {
        logger::info("Parsing collision presets..");

        auto paths = GetPresetPaths();
        size_t loadedCount = 0;
        std::array<bool, kBuiltInPresetCount> loadedPresets{};
        std::vector<PresetConfig> customPresets{};

        for (const auto& path : paths) {

            const auto pathStr = ToUTF8(path);
            logger::info("Reading {}", pathStr);

            std::ifstream presetFile(path);
            if (!presetFile.is_open()) {
                logger::error("Failed to open preset file: {}", pathStr);
                continue;
            }

            std::string raw((std::istreambuf_iterator<char>(presetFile)), std::istreambuf_iterator<char>());
            json data;
            try {
                data = json::parse(raw, nullptr, true, true);
            }
            catch (const json::exception& e) {
                logger::error("Failed to parse preset file {}: {}", pathStr, e.what());
                continue;
            }

            if (!data.is_object()) {
                logger::error("Preset file must contain one JSON object (single preset per file): {}", pathStr);
                continue;
            }

            const auto key = ToUTF8(path.stem()); // Filename is our key.
            if (key == kPresetKeys[ToUnderlying(Preset::kVanilla)]) {
                logger::info("Ignoring {} preset file; using built-in Vanilla defaults.", key);
                continue;
            }

            // Check for duplicate builtin preset.
            auto builtIn = std::find_if( a_presets.begin(), a_presets.end(),
                [&](const PresetConfig& a_config) {
                    return a_config.key == key;
                }
            );

            // Check for duplicate custom preset.
            auto custom = std::find_if(customPresets.begin(), customPresets.end(),
                [&](const PresetConfig& a_config) {
                    return a_config.key == key;
                }
            );

            if (custom != customPresets.end()) {
                logger::error("Duplicate custom preset key {} in {}", key, pathStr);
                continue;
            }

            if (builtIn != a_presets.end()) {
                const auto index = static_cast<size_t>(ToUnderlying(builtIn->preset));
                if (loadedPresets[index]) {
                    logger::error("Duplicate built-in preset key {} in {}", key, pathStr);
                    continue;
                }
            }

            // For now, preset display name is iinitialized to key.
            PresetConfig loadedConfig = builtIn != a_presets.end() ? *builtIn : PresetConfig{ Preset::kTotal, key, key, {}, false, false };

            if (!LoadPresetConfiguration(loadedConfig, data)) {
                continue;
            }

            if (builtIn != a_presets.end()) {
                if (loadedConfig.preset == Preset::kVanilla) {
                    loadedConfig.name = kPresetNames[0];
                }
                *builtIn = loadedConfig;
                const auto index = static_cast<size_t>(ToUnderlying(loadedConfig.preset));
                loadedPresets[index] = true;
            }
            else {
                customPresets.push_back(loadedConfig);
            }

            ++loadedCount;
            loadedConfig.data.Log();
        }

        const auto& fallbackData = GetVanillaFallbackData();
        for (size_t i = 0; i < a_presets.size(); ++i) {
            auto& presetConfig = a_presets[i];
            if (loadedPresets[i] || presetConfig.loaded) {
                continue;
            }

            const auto preset = static_cast<Preset>(i);
            presetConfig.preset = preset;
            presetConfig.data = fallbackData;
            presetConfig.loaded = true;
            presetConfig.name = preset == Preset::kVanilla ? kPresetNames[i] : std::string("Vanilla (") + kPresetNames[i] + " fallback)";
            logger::warn("{} preset unavailable; using built-in vanilla fallback.", kPresetNames[i]);
        }

        // Order custom presets in Alp. order.
        std::sort(customPresets.begin(), customPresets.end(),
            [](const PresetConfig& a_left, const PresetConfig& a_right) {
                return a_left.name < a_right.name;
            }
        );

        // Append custom presets.
        for (auto& presetConfig : customPresets) {
            presetConfig.preset = static_cast<Preset>(a_presets.size()); // New virtual enum id.
            a_presets.push_back(std::move(presetConfig));
        }

        logger::info("Preset parsing finished: {}/{} loaded successfully", loadedCount, a_presets.size());
        return true;
    }

    std::string MakePresetKey(std::string_view a_name)
    {
        std::string key;
        bool hasAlphaNumeric = false;
        key.reserve(a_name.size());
        for (const auto character : a_name) {
            const auto value = static_cast<unsigned char>(character);
            if (std::isalnum(value) || character == '_') {
                key.push_back(character);
                hasAlphaNumeric = hasAlphaNumeric || std::isalnum(value);
            }
        }
        return hasAlphaNumeric ? key : std::string{};
    }

    bool SavePresetConfiguration(const PresetConfig& a_preset, std::string& a_error)
    {
        const auto dir = GetPresetDir();
        std::error_code ec;
        fs::create_directories(dir, ec);
        if (ec) {
            a_error = "Could not create the preset directory.";
            logger::error("Failed to create preset directory {}: {}", ToUTF8(dir), ec.message());
            return false;
        }

        // Atomic, safer as we use written presets immedietly in dynamics.
        const auto path = dir / (a_preset.key + ".json");
        const auto temporaryPath = path.string() + ".tmp";
        auto data = JSON::CollisionDataToJson(a_preset.data);
        data["name"] = a_preset.name;

        {
            std::ofstream file(temporaryPath, std::ios::trunc);
            if (!file.is_open()) {
                a_error = "Could not open the preset file for writing.";
                return false;
            }

            file << data.dump(2);
            if (!file.good()) {
                a_error = "Could not write the preset file.";
                return false;
            }
        }

        fs::rename(temporaryPath, path, ec);
        if (ec) {
            fs::remove(temporaryPath);
            a_error = "Could not finish saving the preset file.";
            logger::error("Failed to save preset {}: {}", ToUTF8(path), ec.message());
            return false;
        }

        logger::info("Preset saved: {}", ToUTF8(path));
        return true;
    }
}
