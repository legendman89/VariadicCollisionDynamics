#pragma once

#include "helper.hpp"
#include "settings_def.hpp"
#include "plugin.hpp"

#include "nlohmann/json.hpp"

#include <array>
#include <cstddef>
#include <filesystem>
#include <string>

namespace Settings {

	inline VCD::Preset PresetFromString(const std::string& a_preset)
	{
		if (a_preset == "PersonalSpace" || a_preset == "Personal Space") {
			return VCD::Preset::kPersonalSpace;
		}

		if (a_preset == "Compact") {
			return VCD::Preset::kCompact;
		}

		if (a_preset == "Bulky") {
			return VCD::Preset::kBulky;
		}

		if (a_preset == "Werewolf") {
			return VCD::Preset::kWerewolf;
		}

		if (a_preset == "VampireLord" || a_preset == "Vampire Lord") {
			return VCD::Preset::kVampireLord;
		}

		return VCD::Preset::kVanillaLike;
	}

	inline void getBool(const nlohmann::json& a_json, const char* a_key, bool& a_value)
	{
		if (a_json.contains(a_key)) {
			a_value = a_json.at(a_key).get<bool>();
		}
	}

	inline void getPreset(const nlohmann::json& a_json, const char* a_key, VCD::Preset& a_value)
	{
		if (a_json.contains(a_key)) {
			a_value = PresetFromString(a_json.at(a_key).get<std::string>());
		}
	}

	inline void getFloat(const nlohmann::json& a_json, const char* a_key, float& a_value)
	{
		if (a_json.contains(a_key)) {
			a_value = a_json.at(a_key).get<float>();
		}
	}

	inline void getInt(const nlohmann::json& a_json, const char* a_key, int& a_value)
	{
		if (a_json.contains(a_key)) {
			a_value = a_json.at(a_key).get<int>();
		}
	}

	inline void getColor(const nlohmann::json& a_json, const char* a_key, std::array<float, 4>& a_value)
	{
		if (!a_json.contains(a_key) || !a_json.at(a_key).is_array()) {
			return;
		}

		const auto& color = a_json.at(a_key);
		for (std::size_t i = 0; i < a_value.size() && i < color.size(); ++i) {
			a_value[i] = color[i].get<float>();
		}
	}

	inline int NormalizeLogLevel(const int& a_level)
	{
		if (a_level < 0) {
			return 0;
		}

		if (a_level > 6) {
			return 6;
		}

		return a_level;
	}

	inline const char* PresetToString(const VCD::Preset& a_preset)
	{
		switch (a_preset) {
		case VCD::Preset::kVanillaLike:
			return "VanillaLike";
		case VCD::Preset::kPersonalSpace:
			return "PersonalSpace";
		case VCD::Preset::kCompact:
			return "Compact";
		case VCD::Preset::kBulky:
			return "Bulky";
		case VCD::Preset::kWerewolf:
			return "Werewolf";
		case VCD::Preset::kVampireLord:
			return "VampireLord";
		default:
			return "VanillaLike";
		}
	}

	inline VCDSettings& GetSettings()
	{
		static VCDSettings settings{};
		return settings;
	}

	inline VCDSettings& GetSavedSettings()
	{
		static VCDSettings settings{};
		return settings;
	}

	inline std::string GetSettingsPath()
	{
		return (VCD::GetPluginDataPath() / "Settings" / "VCDSettings.json").string();
	}

	bool SettingsEqual(const VCDSettings& a_left, const VCDSettings& a_right);

	void CaptureCurrent(VCDSettings& a_settings);

	void ApplySettings(const VCDSettings& a_settings);

	void MarkPresetEdited(const VCD::Preset& a_preset, const VCD::CollisionData& a_data);

	void ClearPresetEdited(const VCD::Preset& a_preset);

	bool IsDirty();

	bool IsToolsDirty();

	bool Load();

	bool Save();

	bool SaveTools();

}
