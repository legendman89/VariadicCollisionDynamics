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

	bool ToolsSettingsEqual(const VCDSettings& a_left, const VCDSettings& a_right)
	{
		if (!(FOREACH_TOOL_BOOL_SETTING(SETTING2EQ) FOREACH_FLOAT_SETTING(SETTING2EQ) FOREACH_INT_SETTING(SETTING2EQ) true)) {
			return false;
		}

		return FOREACH_COLOR_SETTING(SETTING2EQ_COLOR) true;
	}

	void CopyToolsSettings(VCDSettings& a_target, const VCDSettings& a_source)
	{
		FOREACH_TOOL_BOOL_SETTING(SETTING2COPY)
		FOREACH_FLOAT_SETTING(SETTING2COPY)
		FOREACH_INT_SETTING(SETTING2COPY)
		FOREACH_COLOR_SETTING(SETTING2COPY_COLOR)
	}

	bool DynamicsSettingsEqual(const VCDSettings& a_left, const VCDSettings& a_right)
	{
		if (!(FOREACH_DYNAMICS_BOOL_SETTING(SETTING2EQ) FOREACH_PRESET_SETTING(SETTING2EQ) true)) {
			return false;
		}

		for (size_t i = 0; i < a_left.presets.size(); ++i) {
			if (!JSON::PresetOverridesEqual(a_left.presets[i], a_right.presets[i])) {
				return false;
			}
		}

		for (size_t i = 0; i < a_left.npcPresets.size(); ++i) {
			if (!JSON::PresetOverridesEqual(a_left.npcPresets[i], a_right.npcPresets[i])) {
				return false;
			}
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
		FOREACH_PRESET_SETTING(SETTING2COPY)
		a_target.presets = a_source.presets;
		a_target.npcPresets = a_source.npcPresets;
		a_target.npcActorPresets = a_source.npcActorPresets;
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
			WriteJsonFile(GetNPCStatePath(), JSON::NPCStateToJson(a_settings), "NPC state");
	}

	bool SettingsEqual(const VCDSettings& a_left, const VCDSettings& a_right)
	{
		if (!(FOREACH_BOOL_SETTING(SETTING2EQ) FOREACH_FLOAT_SETTING(SETTING2EQ) FOREACH_INT_SETTING(SETTING2EQ) FOREACH_PRESET_SETTING(SETTING2EQ) true)) {
			return false;
		}

		if (!(FOREACH_COLOR_SETTING(SETTING2EQ_COLOR) true)) {
			return false;
		}

		for (size_t i = 0; i < a_left.presets.size(); ++i) {
			if (!JSON::PresetOverridesEqual(a_left.presets[i], a_right.presets[i])) {
				return false;
			}
		}

		for (size_t i = 0; i < a_left.npcPresets.size(); ++i) {
			if (!JSON::PresetOverridesEqual(a_left.npcPresets[i], a_right.npcPresets[i])) {
				return false;
			}
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

#undef PRESET2COPY
#define PRESET2COPY(S, D, C) S.C = D.C
#define FOREACH_SETTING(S, D, COPY) \
	COPY(S, D, outdoor); \
	COPY(S, D, indoor); \
	COPY(S, D, combat); \
	COPY(S, D, werewolf); \
	COPY(S, D, vampireLord); \
	COPY(S, D, neutral); \
	COPY(S, D, npcNeutral); \
	COPY(S, D, npcCombat); \
	COPY(S, D, guardNeutral); \
	COPY(S, D, guardCombat);

	void CaptureCurrent(VCDSettings& a_settings)
	{
		const auto& config = Dynamics::GetConfig();
		FOREACH_SETTING(a_settings, config, PRESET2COPY)
	}

	void ApplySettings(const VCDSettings& a_settings)
	{
		auto& config = Dynamics::GetConfig();
		auto& manager = VCD::Manager::GetSingleton();
		FOREACH_SETTING(config, a_settings, PRESET2COPY)
		const auto logLevel = static_cast<spdlog::level::level_enum>(NormalizeLogLevel(a_settings.logLevel));
		spdlog::set_level(logLevel);
		spdlog::flush_on(logLevel);

		for (size_t i = 0; i < a_settings.presets.size(); ++i) {
			const auto preset = static_cast<VCD::Preset>(i);
			if (!a_settings.presets[i].edited) {
				manager.RestorePresetDefault(preset);
				continue;
			}

			if (auto* presetConfig = manager.GetPresetConfig(preset)) {
				presetConfig->data = a_settings.presets[i].data;
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
		const auto index = static_cast<size_t>(VCD::ToUnderlying(a_preset));
		auto& settings = GetSettings();

		if (index >= settings.presets.size()) {
			return;
		}

		settings.presets[index].edited = true;
		settings.presets[index].data = a_data;
		settings.presets[index].data.RecalculateHeight();
	}

	void ClearPresetEdited(const VCD::Preset& a_preset)
	{
		const auto index = static_cast<size_t>(VCD::ToUnderlying(a_preset));
		auto& settings = GetSettings();

		if (index >= settings.presets.size()) {
			return;
		}

		settings.presets[index] = {};
		VCD::Manager::GetSingleton().RestorePresetDefault(a_preset);
	}

	const VCD::CollisionData* GetNPCPresetOverride(const VCD::Preset& a_preset)
	{
		const auto index = static_cast<size_t>(VCD::ToUnderlying(a_preset));
		const auto& settings = GetSettings();

		if (index >= settings.npcPresets.size() || !settings.npcPresets[index].edited) {
			return nullptr;
		}

		return &settings.npcPresets[index].data;
	}

	void MarkNPCPresetEdited(const VCD::Preset& a_preset, const VCD::CollisionData& a_data)
	{
		const auto index = static_cast<size_t>(VCD::ToUnderlying(a_preset));
		auto& settings = GetSettings();

		if (index >= settings.npcPresets.size()) {
			return;
		}

		settings.npcPresets[index].edited = true;
		settings.npcPresets[index].data = a_data;
		settings.npcPresets[index].data.RecalculateHeight();
	}

	void ClearNPCPresetEdited(const VCD::Preset& a_preset)
	{
		const auto index = static_cast<size_t>(VCD::ToUnderlying(a_preset));
		auto& settings = GetSettings();

		if (index >= settings.npcPresets.size()) {
			return;
		}

		settings.npcPresets[index] = {};
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

	bool IsDirty()
	{
		CaptureCurrent(GetSettings());
		return !DynamicsSettingsEqual(GetSettings(), GetSavedSettings());
	}

	bool IsToolsDirty()
	{
		return !ToolsSettingsEqual(GetSettings(), GetSavedSettings());
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

#undef SETTING2EQ
#undef SETTING2EQ_COLOR
#undef SETTING2COPY
#undef SETTING2COPY_COLOR
#undef PRESET2COPY

}
