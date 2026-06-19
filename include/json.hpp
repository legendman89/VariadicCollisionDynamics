#pragma once

#include "settings.hpp"

#include "nlohmann/json.hpp"

#include <array>
#include <cstddef>
#include <string>

namespace JSON {

	using json = nlohmann::json;

#define BOOL2JSON(S, D) { #S, a_settings.S },
#define FLOAT2JSON(S, D) { #S, a_settings.S },
#define INT2JSON(S, D) { #S, a_settings.S },
#define COLOR2JSON(S, D0, D1, D2, D3) { #S, a_settings.S },
#define PRESET2JSON(S, D) { #S, Settings::PresetToString(a_settings.S) },


#define BOOL2GETTER(S, D) Settings::getBool(a_json, #S, a_settings.S);
#define FLOAT2GETTER(S, D) Settings::getFloat(a_json, #S, a_settings.S);
#define INT2GETTER(S, D) Settings::getInt(a_json, #S, a_settings.S);
#define COLOR2GETTER(S, D0, D1, D2, D3) Settings::getColor(a_json, #S, a_settings.S);
#define PRESET2GETTER(S, D) Settings::getPreset(a_json, #S, a_settings.S);

	inline json ToolsToJson(const Settings::VCDSettings& a_settings)
	{
		auto data = json{
			FOREACH_BOOL_SETTING(BOOL2JSON)
			FOREACH_FLOAT_SETTING(FLOAT2JSON)
			FOREACH_INT_SETTING(INT2JSON)
			FOREACH_COLOR_SETTING(COLOR2JSON)
		};

		return data;
	}

	inline void ToolsFromJson(const json& a_json, Settings::VCDSettings& a_settings)
	{
		FOREACH_BOOL_SETTING(BOOL2GETTER)
		FOREACH_FLOAT_SETTING(FLOAT2GETTER)
		FOREACH_INT_SETTING(INT2GETTER)
		FOREACH_COLOR_SETTING(COLOR2GETTER)
	}

	inline bool PresetOverridesEqual(const Settings::VCDSettings::PresetOverride& a_left, const Settings::VCDSettings::PresetOverride& a_right)
	{
		return a_left.edited == a_right.edited && a_left.data.IsSame(a_right.data);
	}

	inline json PointToJson(const VCD::Vec3& a_point)
	{
		return json::array({ a_point.x, a_point.y, a_point.z });
	}

	inline json PointToJson(const RE::NiPoint3& a_point)
	{
		return json::array({ a_point.x, a_point.y, a_point.z });
	}

	inline bool ReadVec3(VCD::Vec3& a_out, const json& a_json)
	{
		if (!a_json.is_array()) {
			return false;
		}

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

	inline bool ReadNiPoint3(RE::NiPoint3& a_out, const json& a_json)
	{
		if (!a_json.is_array()) {
			return false;
		}

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

	inline json CollisionDataToJson(const VCD::CollisionData& a_data)
	{
		return {
			{ "position", PointToJson(a_data.bump.translation) },
			{ "radius", a_data.capsule.radius },
			{ "height", a_data.capsule.height },
			{ "firstPoint", PointToJson(a_data.capsule.point1) },
			{ "secondPoint", PointToJson(a_data.capsule.point2) }
		};
	}

	inline void CollisionDataFromJson(const json& a_json, VCD::CollisionData& a_data)
	{
		if (a_json.contains("position")) {
			ReadNiPoint3(a_data.bump.translation, a_json.at("position"));
		}

		if (a_json.contains("radius")) {
			a_data.capsule.radius = a_json.at("radius").get<float>();
		}

		if (a_json.contains("firstPoint")) {
			ReadVec3(a_data.capsule.point1, a_json.at("firstPoint"));
		}

		if (a_json.contains("secondPoint")) {
			ReadVec3(a_data.capsule.point2, a_json.at("secondPoint"));
		}

		a_data.RecalculateHeight();
	}

	inline json PresetOverridesToJson(const std::array<Settings::VCDSettings::PresetOverride, VCD::kPresetCount>& a_presets)
	{
		json presets = json::object();

		for (size_t i = 0; i < a_presets.size(); ++i) {
			const auto preset = static_cast<VCD::Preset>(i);
			const auto& presetOverride = a_presets[i];
			auto entry = json{
				{ "edited", presetOverride.edited }
			};

			if (presetOverride.edited) {
				entry.update(CollisionDataToJson(presetOverride.data));
			}

			presets[Settings::PresetToString(preset)] = entry;
		}

		return presets;
	}

	inline void PresetOverridesFromJson(const json& a_json, const char* a_key, std::array<Settings::VCDSettings::PresetOverride, VCD::kPresetCount>& a_presets)
	{
		if (!a_json.contains(a_key) || !a_json.at(a_key).is_object()) {
			return;
		}

		const auto& presets = a_json.at(a_key);
		for (size_t i = 0; i < a_presets.size(); ++i) {
			const auto preset = static_cast<VCD::Preset>(i);
			const auto name = Settings::PresetToString(preset);
			if (!presets.contains(name)) {
				continue;
			}

			const auto& entry = presets.at(name);
			auto& presetOverride = a_presets[i];

			if (entry.contains("edited")) {
				presetOverride.edited = entry.at("edited").get<bool>();
			}

			if (presetOverride.edited) {
				CollisionDataFromJson(entry, presetOverride.data);
			}
		}
	}

	inline json NPCActorPresetsToJson(const Settings::VCDSettings& a_settings)
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

	inline void NPCActorPresetsFromJson(const json& a_json, Settings::VCDSettings& a_settings)
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
			actorPreset.preset = Settings::PresetFromString(entry.at("preset").get<std::string>());
			if (entry.contains("name")) {
				actorPreset.name = entry.at("name").get<std::string>();
			}
			CollisionDataFromJson(entry.at("data"), actorPreset.data);
			a_settings.npcActorPresets.push_back(actorPreset);
		}
	}

	inline json PlayerStateToJson(const Settings::VCDSettings& a_settings)
	{
		auto data = json{
			FOREACH_PLAYER_PRESET_SETTING(PRESET2JSON)
		};

		data["presets"] = PresetOverridesToJson(a_settings.presets);
		return data;
	}

	inline void PlayerStateFromJson(const json& a_json, Settings::VCDSettings& a_settings)
	{
		FOREACH_PLAYER_PRESET_SETTING(PRESET2GETTER)
		PresetOverridesFromJson(a_json, "presets", a_settings.presets);
	}

	inline json NPCStateToJson(const Settings::VCDSettings& a_settings)
	{
		auto data = json{
			FOREACH_NPC_PRESET_SETTING(PRESET2JSON)
		};

		data["presets"] = PresetOverridesToJson(a_settings.npcPresets);
		data["actorPresets"] = NPCActorPresetsToJson(a_settings);
		return data;
	}

	inline void NPCStateFromJson(const json& a_json, Settings::VCDSettings& a_settings)
	{
		FOREACH_NPC_PRESET_SETTING(PRESET2GETTER)
		PresetOverridesFromJson(a_json, "presets", a_settings.npcPresets);
		NPCActorPresetsFromJson(a_json, a_settings);
	}

	inline json ToJson(const Settings::VCDSettings& a_settings)
	{
		auto data = json{
			FOREACH_BOOL_SETTING(BOOL2JSON)
			FOREACH_FLOAT_SETTING(FLOAT2JSON)
			FOREACH_INT_SETTING(INT2JSON)
			FOREACH_COLOR_SETTING(COLOR2JSON)
			FOREACH_PRESET_SETTING(PRESET2JSON)
		};

		data["presets"] = PresetOverridesToJson(a_settings.presets);
		data["npcPresets"] = PresetOverridesToJson(a_settings.npcPresets);
		data["actorPresets"] = NPCActorPresetsToJson(a_settings);
		return data;
	}

	inline void FromJson(const json& a_json, Settings::VCDSettings& a_settings)
	{
		FOREACH_BOOL_SETTING(BOOL2GETTER)
		FOREACH_FLOAT_SETTING(FLOAT2GETTER)
		FOREACH_INT_SETTING(INT2GETTER)
		FOREACH_PRESET_SETTING(PRESET2GETTER)
		FOREACH_COLOR_SETTING(COLOR2GETTER)
		PresetOverridesFromJson(a_json, "presets", a_settings.presets);
		PresetOverridesFromJson(a_json, "npcPresets", a_settings.npcPresets);
		NPCActorPresetsFromJson(a_json, a_settings);
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

}
