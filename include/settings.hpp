#pragma once

#include "plugin.hpp"
#include "helper.hpp"
#include "manager.hpp"
#include "settings_def.hpp"

#include "nlohmann/json.hpp"

#include <array>
#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <algorithm>

namespace Settings {

	namespace fs = std::filesystem;

	inline VCD::Preset PresetFromString(const std::string& a_preset)
	{
		const auto* presetConfig = VCD::Manager::GetSingleton().GetPresetConfig(a_preset);
		if (!presetConfig) {
			logger::warn("Preset {} is unavailable; using Vanilla.", a_preset);
			return VCD::Preset::kVanilla;
		}

		return presetConfig->preset;
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
		for (size_t i = 0; i < a_value.size() && i < color.size(); ++i) {
			a_value[i] = color[i].get<float>();
		}
	}

	inline int NormalizeLogLevel(const int& a_level)
	{
		return std::clamp(a_level, 0, 6);
	}

	inline const char* PresetToString(const VCD::Preset& a_preset)
	{
		return VCD::PresetKey(a_preset);
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

	inline fs::path GetSettingsDirectory()
	{
		return VCD::GetPluginDataPath() / "Settings";
	}

	inline fs::path GetSettingsPath()
	{
		return GetSettingsDirectory() / "Settings.json";
	}

	inline fs::path GetPlayerStatePath()
	{
		return GetSettingsDirectory() / "PlayerState.json";
	}

	inline fs::path GetNPCStatePath()
	{
		return GetSettingsDirectory() / "NPCState.json";
	}

	inline fs::path GetCameraStatePath()
	{
		return GetSettingsDirectory() / "CameraState.json";
	}

	bool SettingsEqual(const VCDSettings& a_left, const VCDSettings& a_right);

	void CaptureCurrent(VCDSettings& a_settings);

	void ApplySettings(const VCDSettings& a_settings);

	void MarkPresetEdited(const VCD::Preset& a_preset, const VCD::CollisionData& a_data);

	void ClearPresetEdited(const VCD::Preset& a_preset);

	const VCD::CollisionData* GetNPCPresetOverride(const VCD::Preset& a_preset);

	bool IsNPCPresetOverrideRelative(const VCD::Preset& a_preset);

	const VCD::CollisionData* GetCameraPresetOverride(const VCD::Preset& a_preset);

	void MarkNPCPresetEdited(const VCD::Preset& a_preset, const VCD::CollisionData& a_data, const bool& a_relative = false);

	void ClearNPCPresetEdited(const VCD::Preset& a_preset);

	void MarkCameraPresetEdited(const VCD::Preset& a_preset, const VCD::CollisionData& a_data);

	void ClearCameraPresetEdited(const VCD::Preset& a_preset);

	const VCD::CollisionData* GetNPCActorPresetOverride(const RE::FormID& a_formID, const VCD::Preset& a_preset);

	void MarkNPCActorPresetEdited(const RE::FormID& a_formID, const std::string& a_name, const VCD::Preset& a_preset, const VCD::CollisionData& a_data);

	void ClearNPCActorPresetEdited(const RE::FormID& a_formID, const VCD::Preset& a_preset);

	void RemapDeletedPreset(const VCD::Preset& a_preset, std::string_view a_key);

	bool IsDirty();

	bool IsToolsDirty();

	bool IsPoseFixesDirty();

	bool IsDynamicsDefault();

	bool IsToolsDefault();

	bool IsPoseFixesDefault();

	void ResetDynamics();

	void ResetTools();

	void ResetPoseFixes();

	bool Load();

	bool Save();

	bool SaveTools();

	bool SavePoseFixes();

}
