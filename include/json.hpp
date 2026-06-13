#pragma once

#include "settings.hpp"

#include "nlohmann/json.hpp"

#include <cstddef>

namespace JSON {

	using json = nlohmann::json;

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

	inline json PresetOverridesToJson(const Settings::VCDSettings& a_settings)
	{
		json presets = json::object();

		for (std::size_t i = 0; i < a_settings.presets.size(); ++i) {
			const auto preset = static_cast<VCD::Preset>(i);
			const auto& presetOverride = a_settings.presets[i];
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

	inline void PresetOverridesFromJson(const json& a_json, Settings::VCDSettings& a_settings)
	{
		if (!a_json.contains("presets") || !a_json.at("presets").is_object()) {
			return;
		}

		const auto& presets = a_json.at("presets");
		for (std::size_t i = 0; i < a_settings.presets.size(); ++i) {
			const auto preset = static_cast<VCD::Preset>(i);
			const auto name = Settings::PresetToString(preset);
			if (!presets.contains(name)) {
				continue;
			}

			const auto& entry = presets.at(name);
			auto& presetOverride = a_settings.presets[i];

			if (entry.contains("edited")) {
				presetOverride.edited = entry.at("edited").get<bool>();
			}

			if (presetOverride.edited) {
				CollisionDataFromJson(entry, presetOverride.data);
			}
		}
	}

	inline json ToJson(const Settings::VCDSettings& a_settings)
	{
#define BOOL2JSON(S, D) { #S, a_settings.S },
#define FLOAT2JSON(S, D) { #S, a_settings.S },
#define INT2JSON(S, D) { #S, a_settings.S },
#define PRESET2JSON(S, D) { #S, Settings::PresetToString(a_settings.S) },

		auto data = json{
			FOREACH_BOOL_SETTING(BOOL2JSON)
			FOREACH_FLOAT_SETTING(FLOAT2JSON)
			FOREACH_INT_SETTING(INT2JSON)
			FOREACH_PRESET_SETTING(PRESET2JSON)
		};

		data["drawColor"] = a_settings.drawColor;
		data["presets"] = PresetOverridesToJson(a_settings);
		return data;
	}

	inline void FromJson(const json& a_json, Settings::VCDSettings& a_settings)
	{
#define BOOL2GETTER(S, D) Settings::getBool(a_json, #S, a_settings.S);
#define FLOAT2GETTER(S, D) Settings::getFloat(a_json, #S, a_settings.S);
#define INT2GETTER(S, D) Settings::getInt(a_json, #S, a_settings.S);
#define PRESET2GETTER(S, D) Settings::getPreset(a_json, #S, a_settings.S);

		FOREACH_BOOL_SETTING(BOOL2GETTER)
		FOREACH_FLOAT_SETTING(FLOAT2GETTER)
		FOREACH_INT_SETTING(INT2GETTER)
		FOREACH_PRESET_SETTING(PRESET2GETTER)
		Settings::getColor(a_json, "drawColor", a_settings.drawColor);
		PresetOverridesFromJson(a_json, a_settings);
	}

}
