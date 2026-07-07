#include "json.hpp"

namespace JSON {

	bool ReadVec3(VCD::Vec3& a_out, const json& a_json)
	{
		if (a_json.is_array()) {
			if (a_json.size() > 0) {
				a_out.x = a_json[0].get<float>();
			}

			if (a_json.size() > 1) {
				a_out.y = a_json[1].get<float>();
			}

			if (a_json.size() > 2) {
				a_out.z = a_json[2].get<float>();
			}

			return true;
		}

		if (!a_json.is_object()) {
			return false;
		}

		if (a_json.contains("x")) {
			a_out.x = a_json["x"].get<float>();
		}

		if (a_json.contains("y")) {
			a_out.y = a_json["y"].get<float>();
		}

		if (a_json.contains("z")) {
			a_out.z = a_json["z"].get<float>();
		}

		return true;
	}

	bool ReadNiPoint3(RE::NiPoint3& a_out, const json& a_json)
	{
		if (a_json.is_array()) {
			if (a_json.size() > 0) {
				a_out.x = a_json[0].get<float>();
			}

			if (a_json.size() > 1) {
				a_out.y = a_json[1].get<float>();
			}

			if (a_json.size() > 2) {
				a_out.z = a_json[2].get<float>();
			}

			return true;
		}

		if (!a_json.is_object()) {
			return false;
		}

		if (a_json.contains("x")) {
			a_out.x = a_json["x"].get<float>();
		}

		if (a_json.contains("y")) {
			a_out.y = a_json["y"].get<float>();
		}

		if (a_json.contains("z")) {
			a_out.z = a_json["z"].get<float>();
		}

		return true;
	}

	json PresetOverridesToJson(const std::unordered_map<std::string, Settings::VCDSettings::PresetOverride>& a_presets)
	{
		json presets = json::object();

		for (const auto& [key, presetOverride] : a_presets) {
			auto entry = json{
				{ "edited", presetOverride.edited },
				{ "relative", presetOverride.relative }
			};

			if (presetOverride.edited) {
				entry.update(CollisionDataToJson(presetOverride.data));
			}

			presets[key] = entry;
		}

		return presets;
	}

	void PresetOverridesFromJson(const json& a_json, const char* a_key, std::unordered_map<std::string, Settings::VCDSettings::PresetOverride>& a_presets)
	{
		if (!a_json.contains(a_key) || !a_json.at(a_key).is_object()) {
			return;
		}

		const auto& presets = a_json.at(a_key);
		for (const auto& [name, entry] : presets.items()) {
			if (!VCD::Manager::GetSingleton().GetPresetConfig(name) ||
				!entry.contains("edited") ||
				!entry.at("edited").get<bool>()) {
				continue;
			}

			auto& presetOverride = a_presets[name];
			presetOverride.edited = true;
			presetOverride.relative = entry.contains("relative") && entry.at("relative").get<bool>();
			CollisionDataFromJson(entry, presetOverride.data);
		}
	}

	json NPCActorPresetsToJson(const Settings::VCDSettings& a_settings)
	{
		auto actors = json::array();

		for (const auto& actorPreset : a_settings.npcActorPresets) {
			actors.push_back({
				{ "formID", actorPreset.formID },
				{ "name", actorPreset.name },
				{ "preset", Settings::PresetToString(actorPreset.preset) },
				{ "data", CollisionDataToJson(actorPreset.data) }
			});
		}

		return actors;
	}

	void NPCActorPresetsFromJson(const json& a_json, Settings::VCDSettings& a_settings)
	{
		if (!a_json.contains("actorPresets") || !a_json.at("actorPresets").is_array()) {
			return;
		}

		a_settings.npcActorPresets.clear();
		for (const auto& entry : a_json.at("actorPresets")) {
			if (!entry.is_object() || !entry.contains("formID") || !entry.contains("preset") || !entry.contains("data")) {
				continue;
			}

			Settings::VCDSettings::NPCActorPresetOverride actorPreset{};
			actorPreset.formID = entry.at("formID").get<RE::FormID>();
			const auto presetKey = entry.at("preset").get<std::string>();
			const auto* presetConfig = VCD::Manager::GetSingleton().GetPresetConfig(presetKey);
			if (!presetConfig) {
				logger::warn("Actor preset {} is unavailable; skipping its override.", presetKey);
				continue;
			}
			actorPreset.preset = presetConfig->preset;
			if (entry.contains("name")) {
				actorPreset.name = entry.at("name").get<std::string>();
			}
			CollisionDataFromJson(entry.at("data"), actorPreset.data);
			a_settings.npcActorPresets.push_back(actorPreset);
		}
	}

#define PRESET2JSON(S, D) { #S, Settings::PresetToString(a_settings.S) },

#define PRESET2GETTER(S, D) Settings::getPreset(a_json, #S, a_settings.S);

	json PlayerStateToJson(const Settings::VCDSettings& a_settings)
	{
		auto data = json{
			FOREACH_PLAYER_PRESET_STATE(PRESET2JSON)
		};

		data["presets"] = PresetOverridesToJson(a_settings.presets);
		return data;
	}

	void PlayerStateFromJson(const json& a_json, Settings::VCDSettings& a_settings)
	{
		FOREACH_PLAYER_PRESET_STATE(PRESET2GETTER)
		PresetOverridesFromJson(a_json, "presets", a_settings.presets);
	}

	json NPCStateToJson(const Settings::VCDSettings& a_settings)
	{
		auto data = json{
			FOREACH_NPC_PRESET_STATE(PRESET2JSON)
		};

		data["presets"] = PresetOverridesToJson(a_settings.npcPresets);
		data["actorPresets"] = NPCActorPresetsToJson(a_settings);
		return data;
	}

	void NPCStateFromJson(const json& a_json, Settings::VCDSettings& a_settings)
	{
		FOREACH_NPC_PRESET_STATE(PRESET2GETTER)
		PresetOverridesFromJson(a_json, "presets", a_settings.npcPresets);
		NPCActorPresetsFromJson(a_json, a_settings);
	}

	json CameraStateToJson(const Settings::VCDSettings& a_settings)
	{
		auto data = json{
			FOREACH_CAMERA_PRESET_STATE(PRESET2JSON)
		};

		data["presets"] = PresetOverridesToJson(a_settings.cameraPresets);

		return data;
	}

	void CameraStateFromJson(const json& a_json, Settings::VCDSettings& a_settings)
	{
		FOREACH_CAMERA_PRESET_STATE(PRESET2GETTER)
			PresetOverridesFromJson(a_json, "presets", a_settings.cameraPresets);
	}

	json ToJson(const Settings::VCDSettings& a_settings)
	{
		auto data = json{
			FOREACH_BOOL_SETTING(BOOL2JSON)
			FOREACH_FLOAT_SETTING(FLOAT2JSON)
			FOREACH_INT_SETTING(INT2JSON)
			FOREACH_COLOR_SETTING(COLOR2JSON)
			FOREACH_PRESET_STATE(PRESET2JSON)
		};

		data["presets"] = PresetOverridesToJson(a_settings.presets);
		data["npcPresets"] = PresetOverridesToJson(a_settings.npcPresets);
		data["cameraPresets"] = PresetOverridesToJson(a_settings.cameraPresets);
		data["actorPresets"] = NPCActorPresetsToJson(a_settings);
		return data;
	}

	void FromJson(const json& a_json, Settings::VCDSettings& a_settings)
	{
		FOREACH_BOOL_SETTING(BOOL2GETTER)
		FOREACH_FLOAT_SETTING(FLOAT2GETTER)
		FOREACH_INT_SETTING(INT2GETTER)
		FOREACH_PRESET_STATE(PRESET2GETTER)
		FOREACH_COLOR_SETTING(COLOR2GETTER)
		PresetOverridesFromJson(a_json, "presets", a_settings.presets);
		PresetOverridesFromJson(a_json, "npcPresets", a_settings.npcPresets);
		PresetOverridesFromJson(a_json, "cameraPresets", a_settings.cameraPresets);
		NPCActorPresetsFromJson(a_json, a_settings);
	}

}

#undef BOOL2JSON
#undef FLOAT2JSON
#undef INT2JSON
#undef COLOR2JSON
#undef PRESET2JSON
#undef BOOL2GETTER
#undef FLOAT2GETTER
#undef INT2GETTER
#undef COLOR2GETTER
#undef PRESET2GETTER
