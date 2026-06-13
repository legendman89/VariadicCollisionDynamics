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

	bool SettingsEqual(const VCDSettings& a_left, const VCDSettings& a_right)
	{
#define SETTING2EQ(S, D) a_left.S == a_right.S &&

		if (!(FOREACH_BOOL_SETTING(SETTING2EQ) FOREACH_PRESET_SETTING(SETTING2EQ) true)) {
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
		a_settings.neutral = config.neutral;
	}

	void ApplySettings(const VCDSettings& a_settings)
	{
		auto& config = Dynamics::GetConfig();
		auto& manager = VCD::Manager::GetSingleton();
		config.outdoor = a_settings.outdoor;
		config.indoor = a_settings.indoor;
		config.combat = a_settings.combat;
		config.neutral = a_settings.neutral;

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
		return !SettingsEqual(GetSettings(), GetSavedSettings());
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

		file << JSON::ToJson(GetSettings()).dump(4);
		GetSavedSettings() = GetSettings();
		logger::info("Settings saved: {}", path.string());
		return true;
	}

}
