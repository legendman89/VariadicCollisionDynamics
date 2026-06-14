#include "settings.hpp"

#include "dynamics.hpp"
#include "json.hpp"
#include "logger.hpp"
#include "manager.hpp"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <cstddef>

namespace Settings {

	namespace fs = std::filesystem;

	bool ToolsSettingsEqual(const VCDSettings& a_left, const VCDSettings& a_right)
	{
#define SETTING2EQ(S, D) a_left.S == a_right.S &&

		if (!(FOREACH_BOOL_SETTING(SETTING2EQ) FOREACH_FLOAT_SETTING(SETTING2EQ) FOREACH_INT_SETTING(SETTING2EQ) true)) {
#undef SETTING2EQ
			return false;
		}

#undef SETTING2EQ
		return a_left.drawColor == a_right.drawColor;
	}

	void CopyToolsSettings(VCDSettings& a_target, const VCDSettings& a_source)
	{
#define SETTING2COPY(S, D) a_target.S = a_source.S;

		FOREACH_BOOL_SETTING(SETTING2COPY)
		FOREACH_FLOAT_SETTING(SETTING2COPY)
		FOREACH_INT_SETTING(SETTING2COPY)
		a_target.drawColor = a_source.drawColor;
#undef SETTING2COPY
	}

	bool DynamicsSettingsEqual(const VCDSettings& a_left, const VCDSettings& a_right)
	{
#define SETTING2EQ(S, D) a_left.S == a_right.S &&

		if (!(FOREACH_PRESET_SETTING(SETTING2EQ) true)) {
#undef SETTING2EQ
			return false;
		}

#undef SETTING2EQ
		for (std::size_t i = 0; i < a_left.presets.size(); ++i) {
			if (!JSON::PresetOverridesEqual(a_left.presets[i], a_right.presets[i])) {
				return false;
			}
		}

		return true;
	}

	void CopyDynamicsSettings(VCDSettings& a_target, const VCDSettings& a_source)
	{
#define SETTING2COPY(S, D) a_target.S = a_source.S;

		FOREACH_PRESET_SETTING(SETTING2COPY)
		a_target.presets = a_source.presets;
#undef SETTING2COPY
	}

	bool WriteSettings(const VCDSettings& a_settings)
	{
		const auto path = fs::path(GetSettingsPath());
		std::error_code ec;
		fs::create_directories(path.parent_path(), ec);

		if (ec) {
			logger::error("Failed to create settings directory {}: {}", path.parent_path().string(), ec.message());
			return false;
		}

		std::ofstream file(path);
		if (!file.is_open()) {
			logger::error("Failed to open settings file for writing: {}", path.string());
			return false;
		}

		file << JSON::ToJson(a_settings).dump(4);
		logger::info("Settings saved: {}", path.string());
		return true;
	}

	bool SettingsEqual(const VCDSettings& a_left, const VCDSettings& a_right)
	{
#define SETTING2EQ(S, D) a_left.S == a_right.S &&

		if (!(FOREACH_BOOL_SETTING(SETTING2EQ) FOREACH_FLOAT_SETTING(SETTING2EQ) FOREACH_INT_SETTING(SETTING2EQ) FOREACH_PRESET_SETTING(SETTING2EQ) true)) {
#undef SETTING2EQ
			return false;
		}

#undef SETTING2EQ
		if (a_left.drawColor != a_right.drawColor) {
			return false;
		}

		for (std::size_t i = 0; i < a_left.presets.size(); ++i) {
			if (!JSON::PresetOverridesEqual(a_left.presets[i], a_right.presets[i])) {
				return false;
			}
		}

		return true;
	}

	void CaptureCurrent(VCDSettings& a_settings)
	{
		auto& config = Dynamics::GetConfig();
		a_settings.outdoor = config.outdoor;
		a_settings.indoor = config.indoor;
		a_settings.combat = config.combat;
		a_settings.werewolf = config.werewolf;
		a_settings.vampireLord = config.vampireLord;
		a_settings.neutral = config.neutral;
	}

	void ApplySettings(const VCDSettings& a_settings)
	{
		auto& config = Dynamics::GetConfig();
		auto& manager = VCD::Manager::GetSingleton();
		config.outdoor = a_settings.outdoor;
		config.indoor = a_settings.indoor;
		config.combat = a_settings.combat;
		config.werewolf = a_settings.werewolf;
		config.vampireLord = a_settings.vampireLord;
		config.neutral = a_settings.neutral;
		const auto logLevel = static_cast<spdlog::level::level_enum>(NormalizeLogLevel(a_settings.logLevel));
		spdlog::set_level(logLevel);
		spdlog::flush_on(logLevel);

		for (std::size_t i = 0; i < a_settings.presets.size(); ++i) {
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

	void MarkPresetEdited(const VCD::Preset& a_preset, const VCD::CollisionData& a_data)
	{
		const auto index = static_cast<std::size_t>(VCD::ToUnderlying(a_preset));
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
		const auto index = static_cast<std::size_t>(VCD::ToUnderlying(a_preset));
		auto& settings = GetSettings();
		if (index >= settings.presets.size()) {
			return;
		}

		settings.presets[index] = {};
		VCD::Manager::GetSingleton().RestorePresetDefault(a_preset);
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

	bool Load()
	{
		auto settings = VCDSettings{};
		const auto path = GetSettingsPath();
		std::error_code ec;

		if (!fs::exists(path, ec)) {
			GetSettings() = settings;
			GetSavedSettings() = settings;
			ApplySettings(settings);
			logger::info("Settings file does not exist, using defaults: {}", path);
			return true;
		}

		std::ifstream file(path);
		if (!file.is_open()) {
			logger::error("Failed to open settings file: {}", path);
			return false;
		}

		std::string raw((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

		try {
			const auto data = JSON::json::parse(raw, nullptr, true, true);
			JSON::FromJson(data, settings);
		}
		catch (const JSON::json::exception& e) {
			logger::error("Failed to parse settings file {}: {}", path, e.what());
			return false;
		}

		GetSettings() = settings;
		GetSavedSettings() = settings;
		ApplySettings(settings);
		logger::info("Settings loaded: {}", path);
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

		if (!WriteSettings(settings)) {
			return false;
		}

		CopyToolsSettings(GetSavedSettings(), GetSettings());
		return true;
	}

}
