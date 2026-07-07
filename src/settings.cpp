#include "settings.hpp"


#include "json.hpp"
#include "helper.hpp"
#include "logger.hpp"
#include "manager.hpp"
#include "dynamics.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <cstddef>
#include <algorithm>

namespace Settings {

#define SETTING2EQ(S, D) a_left.S == a_right.S &&
#define SETTING2EQ_COLOR(S, D0, D1, D2, D3) a_left.S == a_right.S &&

#define SETTING2COPY(S, D) a_target.S = a_source.S;
#define SETTING2COPY_COLOR(S, D0, D1, D2, D3) a_target.S = a_source.S;

	bool PresetOverrideMapsEqual(
		const std::unordered_map<std::string, VCDSettings::PresetOverride>& a_left,
		const std::unordered_map<std::string, VCDSettings::PresetOverride>& a_right)
	{
		if (a_left.size() != a_right.size()) {
			return false;
		}

		for (const auto& [key, left] : a_left) {
			const auto it = a_right.find(key);
			if (it == a_right.end() || !JSON::PresetOverridesEqual(left, it->second)) {
				return false;
			}
		}

		return true;
	}

	bool ToolsSettingsEqual(const VCDSettings& a_left, const VCDSettings& a_right)
	{
		if (!(FOREACH_TOOL_BOOL_SETTING(SETTING2EQ) FOREACH_TOOL_FLOAT_SETTING(SETTING2EQ) FOREACH_TOOL_INT_SETTING(SETTING2EQ) true)) {
			return false;
		}

		return FOREACH_COLOR_SETTING(SETTING2EQ_COLOR) true;
	}

	void CopyToolsSettings(VCDSettings& a_target, const VCDSettings& a_source)
	{
		FOREACH_TOOL_BOOL_SETTING(SETTING2COPY)
		FOREACH_TOOL_FLOAT_SETTING(SETTING2COPY)
		FOREACH_TOOL_INT_SETTING(SETTING2COPY)
		FOREACH_COLOR_SETTING(SETTING2COPY_COLOR)
	}

	bool DynamicsSettingsEqual(const VCDSettings& a_left, const VCDSettings& a_right)
	{
		if (!(FOREACH_DYNAMICS_BOOL_SETTING(SETTING2EQ) FOREACH_DYNAMICS_FLOAT_SETTING(SETTING2EQ) FOREACH_DYNAMICS_INT_SETTING(SETTING2EQ) FOREACH_PRESET_STATE(SETTING2EQ) true)) {
			return false;
		}

		if (!PresetOverrideMapsEqual(a_left.presets, a_right.presets) ||
			!PresetOverrideMapsEqual(a_left.npcPresets, a_right.npcPresets) ||
			!PresetOverrideMapsEqual(a_left.cameraPresets, a_right.cameraPresets)) {
			return false;
		}

		if (a_left.npcActorPresets.size() != a_right.npcActorPresets.size()) {
			return false;
		}

		for (size_t i = 0; i < a_left.npcActorPresets.size(); ++i) {
			const auto& left = a_left.npcActorPresets[i];
			const auto& right = a_right.npcActorPresets[i];
			if (left.formID != right.formID || left.name != right.name || left.preset != right.preset || !left.data.IsSame(right.data)) {
				return false;
			}
		}

		return true;
	}

	void CopyDynamicsSettings(VCDSettings& a_target, const VCDSettings& a_source)
	{
		FOREACH_DYNAMICS_BOOL_SETTING(SETTING2COPY)
		FOREACH_DYNAMICS_FLOAT_SETTING(SETTING2COPY)
		FOREACH_DYNAMICS_INT_SETTING(SETTING2COPY)
		FOREACH_PRESET_STATE(SETTING2COPY)
		a_target.presets = a_source.presets;
		a_target.npcPresets = a_source.npcPresets;
		a_target.cameraPresets = a_source.cameraPresets;
		a_target.npcActorPresets = a_source.npcActorPresets;
	}

	bool PoseFixesSettingsEqual(const VCDSettings& a_left, const VCDSettings& a_right)
	{
		return FOREACH_POSE_FIX_BOOL_SETTING(SETTING2EQ) FOREACH_POSE_FIX_FLOAT_SETTING(SETTING2EQ) true;
	}

	void CopyPoseFixesSettings(VCDSettings& a_target, const VCDSettings& a_source)
	{
		FOREACH_POSE_FIX_BOOL_SETTING(SETTING2COPY)
		FOREACH_POSE_FIX_FLOAT_SETTING(SETTING2COPY)
	}

	bool WriteJsonFile(const fs::path& a_path, const JSON::json& a_json, const char* a_label)
	{
		const auto parentPathStr = VCD::ToUTF8(a_path.parent_path());

		std::error_code ec;
		fs::create_directories(a_path.parent_path(), ec);

		if (ec) {
			logger::error("Failed to create settings directory {}: {}", parentPathStr, ec.message());
			return false;
		}

		const auto pathStr = VCD::ToUTF8(a_path);

		std::ofstream file(a_path);
		if (!file.is_open()) {
			logger::error("Failed to open {} file for writing: {}", a_label, pathStr);
			return false;
		}

		file << a_json.dump(4);
		logger::info("{} saved: {}", a_label, pathStr);
		return true;
	}

	bool ReadJsonFile(const fs::path& a_path, JSON::json& a_json, const char* a_label)
	{
		const auto pathStr = VCD::ToUTF8(a_path);

		std::ifstream file(a_path);
		if (!file.is_open()) {
			logger::error("Failed to open {} file: {}", a_label, pathStr);
			return false;
		}

		std::string raw((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

		try {
			a_json = JSON::json::parse(raw, nullptr, true, true);
		}
		catch (const JSON::json::exception& e) {
			logger::error("Failed to parse {} file {}: {}", a_label, pathStr, e.what());
			return false;
		}

		logger::info("{} loaded: {}", a_label, pathStr);
		return true;
	}

	bool WriteSettings(const VCDSettings& a_settings)
	{
		return WriteJsonFile(GetSettingsPath(), JSON::ToolsToJson(a_settings), "Settings") &&
			WriteJsonFile(GetPlayerStatePath(), JSON::PlayerStateToJson(a_settings), "Player state") &&
			WriteJsonFile(GetNPCStatePath(), JSON::NPCStateToJson(a_settings), "NPC state") &&
			WriteJsonFile(GetCameraStatePath(), JSON::CameraStateToJson(a_settings), "Camera state");
	}

	bool SettingsEqual(const VCDSettings& a_left, const VCDSettings& a_right)
	{
		if (!(FOREACH_BOOL_SETTING(SETTING2EQ) FOREACH_FLOAT_SETTING(SETTING2EQ) FOREACH_INT_SETTING(SETTING2EQ) FOREACH_PRESET_STATE(SETTING2EQ) true)) {
			return false;
		}

		if (!(FOREACH_COLOR_SETTING(SETTING2EQ_COLOR) true)) {
			return false;
		}

		if (!PresetOverrideMapsEqual(a_left.presets, a_right.presets) ||
			!PresetOverrideMapsEqual(a_left.npcPresets, a_right.npcPresets) ||
			!PresetOverrideMapsEqual(a_left.cameraPresets, a_right.cameraPresets)) {
			return false;
		}

		if (a_left.npcActorPresets.size() != a_right.npcActorPresets.size()) {
			return false;
		}

		for (size_t i = 0; i < a_left.npcActorPresets.size(); ++i) {
			const auto& left = a_left.npcActorPresets[i];
			const auto& right = a_right.npcActorPresets[i];
			if (left.formID != right.formID || left.name != right.name || left.preset != right.preset || !left.data.IsSame(right.data)) {
				return false;
			}
		}

		return true;
	}

#define PRESET_STATE2CAPTURE(S, D) a_settings.S = config.S;
#define PRESET_STATE2APPLY(S, D) config.S = a_settings.S;

	void CaptureCurrent(VCDSettings& a_settings)
	{
		const auto& config = Dynamics::GetConfig();
		FOREACH_PRESET_STATE(PRESET_STATE2CAPTURE)
	}

	void ApplySettings(const VCDSettings& a_settings)
	{
		auto& config = Dynamics::GetConfig();
		auto& manager = VCD::Manager::GetSingleton();
		FOREACH_PRESET_STATE(PRESET_STATE2APPLY)
		const auto logLevel = static_cast<spdlog::level::level_enum>(NormalizeLogLevel(a_settings.logLevel));
		spdlog::set_level(logLevel);
		spdlog::flush_on(logLevel);

		for (const auto& presetConfig : manager.GetPresetConfigs()) {
			manager.RestorePresetDefault(presetConfig.preset);
		}

		for (const auto& [key, presetOverride] : a_settings.presets) {
			if (!presetOverride.edited) {
				continue;
			}

			if (auto* presetConfig = manager.GetPresetConfig(key)) {
				presetConfig->data = presetOverride.data;
				presetConfig->data.RecalculateHeight();
			}
		}

	}

	void ApplyToolSettings(const VCDSettings& a_settings)
	{
		const auto logLevel = static_cast<spdlog::level::level_enum>(NormalizeLogLevel(a_settings.logLevel));
		spdlog::set_level(logLevel);
		spdlog::flush_on(logLevel);
	}

	void MarkPresetEdited(const VCD::Preset& a_preset, const VCD::CollisionData& a_data)
	{
		auto& settings = GetSettings();
		const auto* presetConfig = VCD::Manager::GetSingleton().GetPresetConfig(a_preset);
		if (!presetConfig) {
			return;
		}

		auto& presetOverride = settings.presets[presetConfig->key];
		presetOverride.edited = true;
		presetOverride.data = a_data;
		presetOverride.data.RecalculateHeight();
	}

	void ClearPresetEdited(const VCD::Preset& a_preset)
	{
		auto& settings = GetSettings();
		const auto* presetConfig = VCD::Manager::GetSingleton().GetPresetConfig(a_preset);
		if (!presetConfig) {
			return;
		}

		settings.presets.erase(presetConfig->key);
		VCD::Manager::GetSingleton().RestorePresetDefault(a_preset);
	}

	const VCD::CollisionData* GetNPCPresetOverride(const VCD::Preset& a_preset)
	{
		const auto& settings = GetSettings();
		const auto* presetConfig = VCD::Manager::GetSingleton().GetPresetConfig(a_preset);
		if (!presetConfig) {
			return nullptr;
		}

		const auto it = settings.npcPresets.find(presetConfig->key);
		return it != settings.npcPresets.end() && it->second.edited ? &it->second.data : nullptr;
	}

	bool IsNPCPresetOverrideRelative(const VCD::Preset& a_preset)
	{
		const auto& settings = GetSettings();
		const auto* presetConfig = VCD::Manager::GetSingleton().GetPresetConfig(a_preset);
		if (!presetConfig) {
			return false;
		}

		const auto it = settings.npcPresets.find(presetConfig->key);
		return it != settings.npcPresets.end() && it->second.edited && it->second.relative;
	}

	const VCD::CollisionData* GetCameraPresetOverride(const VCD::Preset& a_preset)
	{
		const auto& settings = GetSettings();
		const auto* presetConfig = VCD::Manager::GetSingleton().GetPresetConfig(a_preset);
		if (!presetConfig) {
			return nullptr;
		}

		const auto it = settings.cameraPresets.find(presetConfig->key);
		return it != settings.cameraPresets.end() && it->second.edited ? &it->second.data : nullptr;
	}

	void MarkNPCPresetEdited(const VCD::Preset& a_preset, const VCD::CollisionData& a_data, const bool& a_relative)
	{
		auto& settings = GetSettings();
		const auto* presetConfig = VCD::Manager::GetSingleton().GetPresetConfig(a_preset);
		if (!presetConfig) {
			return;
		}

		auto& presetOverride = settings.npcPresets[presetConfig->key];
		presetOverride.edited = true;
		presetOverride.relative = a_relative;
		presetOverride.data = a_data;
		presetOverride.data.RecalculateHeight();
	}

	void ClearNPCPresetEdited(const VCD::Preset& a_preset)
	{
		auto& settings = GetSettings();
		const auto* presetConfig = VCD::Manager::GetSingleton().GetPresetConfig(a_preset);
		if (!presetConfig) {
			return;
		}

		settings.npcPresets.erase(presetConfig->key);
	}

	void MarkCameraPresetEdited(const VCD::Preset& a_preset, const VCD::CollisionData& a_data)
	{
		auto& settings = GetSettings();
		const auto* presetConfig = VCD::Manager::GetSingleton().GetPresetConfig(a_preset);
		if (!presetConfig) {
			return;
		}

		auto& presetOverride = settings.cameraPresets[presetConfig->key];
		presetOverride.edited = true;
		presetOverride.data = a_data;
		presetOverride.data.RecalculateHeight();
	}

	void ClearCameraPresetEdited(const VCD::Preset& a_preset)
	{
		auto& settings = GetSettings();
		const auto* presetConfig = VCD::Manager::GetSingleton().GetPresetConfig(a_preset);
		if (!presetConfig) {
			return;
		}

		settings.cameraPresets.erase(presetConfig->key);
	}

	const VCD::CollisionData* GetNPCActorPresetOverride(const RE::FormID& a_formID, const VCD::Preset& a_preset)
	{
		for (const auto& actorPreset : GetSettings().npcActorPresets) {
			if (actorPreset.formID == a_formID && actorPreset.preset == a_preset) {
				return &actorPreset.data;
			}
		}

		return nullptr;
	}

	void MarkNPCActorPresetEdited(const RE::FormID& a_formID, const std::string& a_name, const VCD::Preset& a_preset, const VCD::CollisionData& a_data)
	{
		auto& settings = GetSettings();
		for (auto& actorPreset : settings.npcActorPresets) {
			if (actorPreset.formID == a_formID && actorPreset.preset == a_preset) {
				actorPreset.name = a_name;
				actorPreset.data = a_data;
				actorPreset.data.RecalculateHeight();
				return;
			}
		}

		settings.npcActorPresets.push_back({ a_formID, a_name, a_preset, a_data });
		settings.npcActorPresets.back().data.RecalculateHeight();
	}

	void ClearNPCActorPresetEdited(const RE::FormID& a_formID, const VCD::Preset& a_preset)
	{
		auto& actorPresets = GetSettings().npcActorPresets;
		std::erase_if(actorPresets,
			[&](const VCDSettings::NPCActorPresetOverride& a_actorPreset) {
				return a_actorPreset.formID == a_formID && a_actorPreset.preset == a_preset;
			}
		);
	}

	void RemapPresetSettings(VCDSettings& a_settings, const VCD::Preset& a_preset, const VCD::Preset& a_replacement, const bool& a_removeDeleted, std::string_view a_key)
	{
#define PRESET_STATE2REMAP(S, D) VCD::RemapPresetAfterDeletion(a_settings.S, a_preset, a_replacement);
		FOREACH_PRESET_STATE(PRESET_STATE2REMAP)
		if (a_removeDeleted) {
			auto actorPreset = a_settings.npcActorPresets.begin();
			while (actorPreset != a_settings.npcActorPresets.end()) {
				if (actorPreset->preset == a_preset) {
					actorPreset = a_settings.npcActorPresets.erase(actorPreset);
					continue;
				}
				VCD::RemapPresetAfterDeletion(actorPreset->preset, a_preset);
				++actorPreset;
			}
			a_settings.presets.erase(std::string(a_key));
			a_settings.npcPresets.erase(std::string(a_key));
			a_settings.cameraPresets.erase(std::string(a_key));
			return;
		}

		for (auto& actorPreset : a_settings.npcActorPresets) {
			VCD::RemapPresetAfterDeletion(actorPreset.preset, a_preset, a_replacement);
		}
	}

	void RemapDeletedPreset(const VCD::Preset& a_preset, std::string_view a_key)
	{
		RemapPresetSettings(GetSettings(), a_preset, VCD::Preset::kVanilla, true, a_key);
		RemapPresetSettings(GetSavedSettings(), a_preset, VCD::kInvalidPreset, false, a_key);
	}

	bool IsDirty()
	{
		CaptureCurrent(GetSettings());
		return !DynamicsSettingsEqual(GetSettings(), GetSavedSettings());
	}

	bool IsToolsDirty()
	{
		return !ToolsSettingsEqual(GetSettings(), GetSavedSettings());
	}

	bool IsPoseFixesDirty()
	{
		return !PoseFixesSettingsEqual(GetSettings(), GetSavedSettings());
	}

	bool IsDynamicsDefault()
	{
		CaptureCurrent(GetSettings());
		const auto defaults = VCDSettings{};
		return DynamicsSettingsEqual(GetSettings(), defaults);
	}

	bool IsToolsDefault()
	{
		const auto defaults = VCDSettings{};
		return ToolsSettingsEqual(GetSettings(), defaults);
	}

	bool IsPoseFixesDefault()
	{
		const auto defaults = VCDSettings{};
		return PoseFixesSettingsEqual(GetSettings(), defaults);
	}

	void ResetDynamics()
	{
		const auto defaults = VCDSettings{};
		CopyDynamicsSettings(GetSettings(), defaults);
		ApplySettings(GetSettings());
	}

	void ResetTools()
	{
		const auto defaults = VCDSettings{};
		CopyToolsSettings(GetSettings(), defaults);
		ApplyToolSettings(GetSettings());
	}

	void ResetPoseFixes()
	{
		const auto defaults = VCDSettings{};
		CopyPoseFixesSettings(GetSettings(), defaults);
	}

	bool Load()
	{
		auto settings = VCDSettings{};
		std::error_code ec;
		bool loaded = false;

		if (const auto path = GetSettingsPath(); fs::exists(path, ec)) {
			JSON::json data{};
			if (!ReadJsonFile(path, data, "Settings")) {
				return false;
			}

			JSON::ToolsFromJson(data, settings);
			loaded = true;
		}

		if (const auto path = GetPlayerStatePath(); fs::exists(path, ec)) {
			JSON::json data{};
			if (!ReadJsonFile(path, data, "Player state")) {
				return false;
			}

			JSON::PlayerStateFromJson(data, settings);
			loaded = true;
		}

		if (const auto path = GetNPCStatePath(); fs::exists(path, ec)) {
			JSON::json data{};
			if (!ReadJsonFile(path, data, "NPC state")) {
				return false;
			}

			JSON::NPCStateFromJson(data, settings);
			loaded = true;
		}

		if (const auto path = GetCameraStatePath(); fs::exists(path, ec)) {
			JSON::json data{};
			if (!ReadJsonFile(path, data, "Camera state")) {
				return false;
			}

			JSON::CameraStateFromJson(data, settings);
			loaded = true;
		}

		GetSettings() = settings;
		GetSavedSettings() = settings;
		ApplySettings(settings);

		if (!loaded) {
			logger::info("Settings files do not exist, using defaults: {}", VCD::ToUTF8(GetSettingsDirectory()));
		}
		return true;
	}

	bool Save()
	{
		CaptureCurrent(GetSettings());
		auto settings = GetSavedSettings();
		CopyDynamicsSettings(settings, GetSettings());

		if (!WriteSettings(settings)) {
			return false;
		}

		CopyDynamicsSettings(GetSavedSettings(), GetSettings());
		return true;
	}

	bool SaveTools()
	{
		auto settings = GetSavedSettings();
		CopyToolsSettings(settings, GetSettings());

		if (!WriteJsonFile(GetSettingsPath(), JSON::ToolsToJson(settings), "Settings")) {
			return false;
		}

		CopyToolsSettings(GetSavedSettings(), GetSettings());
		return true;
	}

	bool SavePoseFixes()
	{
		auto settings = GetSavedSettings();
		CopyPoseFixesSettings(settings, GetSettings());

		if (!WriteJsonFile(GetSettingsPath(), JSON::ToolsToJson(settings), "Settings")) {
			return false;
		}

		CopyPoseFixesSettings(GetSavedSettings(), GetSettings());
		return true;
	}

#undef SETTING2EQ
#undef SETTING2EQ_COLOR
#undef SETTING2COPY
#undef SETTING2COPY_COLOR
#undef PRESET_STATE2CAPTURE
#undef PRESET_STATE2APPLY
#undef PRESET_STATE2REMAP

}
