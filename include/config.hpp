#pragma once

#include "helper.hpp"
#include "logger.hpp"
#include "preset.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include <array>

#include "nlohmann/json.hpp"

namespace VCD {

    namespace fs = std::filesystem;

    inline fs::path GetPresetDir()
    {
        return GetPluginDataPath() / "Presets";
    }

    inline const CollisionData& GetVanillaFallbackData()
	{
		static const CollisionData fallback = MakeVanillaCollisionData();
		return fallback;
	}

    std::vector<fs::path> GetPresetPaths();

    bool LoadPresetConfiguration(PresetConfig& a_preset, const nlohmann::json& a_data);

    bool LoadPresetConfigurations(std::vector<PresetConfig>& a_presets);

    bool SavePresetConfiguration(const PresetConfig& a_preset, std::string& a_error);

    std::string MakePresetKey(std::string_view a_name);
}
