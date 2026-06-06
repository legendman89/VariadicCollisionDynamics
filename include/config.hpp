#pragma once

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

    inline std::string ToUTF8(const fs::path& a_path)
    {
        auto u8 = a_path.u8string();
        return std::string(reinterpret_cast<const char*>(u8.c_str()));
    }

    inline std::string GetPresetDir()
    {
        const auto root = fs::path(REL::Module::get().filename()).parent_path();
        const auto dataPath = root / "Data" / "SKSE" / "Plugins" / PRODUCT_NAME / "Presets";

        std::error_code ec;
        if (fs::exists(dataPath, ec) && fs::is_directory(dataPath, ec)) {
            return dataPath.string();
        }

        return (root / PRODUCT_NAME / "Presets").string();
    }

    inline std::vector<std::string> GetPresetPaths()
    {
        const fs::path dir = GetPresetDir();
        std::vector<std::string> paths;

        std::error_code ec;
        if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec)) {
            logger::critical("Preset directory {} does not exist.", ToUTF8(dir));
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
                paths.push_back(ToUTF8(path));
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

    bool LoadPresetConfiguration(PresetConfig& a_preset, const nlohmann::json& a_data);

    bool LoadPresetConfigurations(std::array<PresetConfig, 4>& a_presets);
}
