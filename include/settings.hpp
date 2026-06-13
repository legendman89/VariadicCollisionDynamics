#pragma once

#include "helper.hpp"
#include "settings_def.hpp"
#include "plugin.hpp"

#include "nlohmann/json.hpp"

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

	bool Load();

	bool Save();

}
