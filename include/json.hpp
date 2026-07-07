#pragma once

#include "settings.hpp"

#include "nlohmann/json.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <unordered_map>

namespace JSON {

	using json = nlohmann::json;


	json PresetOverridesToJson(const std::unordered_map<std::string, Settings::VCDSettings::PresetOverride>& a_presets);

	void PresetOverridesFromJson(const json& a_json, const char* a_key, std::unordered_map<std::string, Settings::VCDSettings::PresetOverride>& a_presets);

	json NPCActorPresetsToJson(const Settings::VCDSettings& a_settings);

	void NPCActorPresetsFromJson(const json& a_json, Settings::VCDSettings& a_settings);

	json PlayerStateToJson(const Settings::VCDSettings& a_settings);

	void PlayerStateFromJson(const json& a_json, Settings::VCDSettings& a_settings);

	json NPCStateToJson(const Settings::VCDSettings& a_settings);

	void NPCStateFromJson(const json& a_json, Settings::VCDSettings& a_settings);

	void CameraStateFromJson(const json& a_json, Settings::VCDSettings& a_settings);

	json CameraStateToJson(const Settings::VCDSettings& a_settings);

	json ToJson(const Settings::VCDSettings& a_settings);

	void FromJson(const json& a_json, Settings::VCDSettings& a_settings);

	bool ReadVec3(VCD::Vec3& a_out, const json& a_json);

	bool ReadNiPoint3(RE::NiPoint3& a_out, const json& a_json);

#define BOOL2JSON(S, D) { #S, a_settings.S },
#define FLOAT2JSON(S, D) { #S, a_settings.S },
#define INT2JSON(S, D) { #S, a_settings.S },
#define COLOR2JSON(S, D0, D1, D2, D3) { #S, a_settings.S },

#define BOOL2GETTER(S, D) Settings::getBool(a_json, #S, a_settings.S);
#define FLOAT2GETTER(S, D) Settings::getFloat(a_json, #S, a_settings.S);
#define INT2GETTER(S, D) Settings::getInt(a_json, #S, a_settings.S);
#define COLOR2GETTER(S, D0, D1, D2, D3) Settings::getColor(a_json, #S, a_settings.S);

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
		return a_left.edited == a_right.edited && a_left.relative == a_right.relative && a_left.data.IsSame(a_right.data);
	}

	inline json PointToJson(const VCD::Vec3& a_point)
	{
		return json::array({ a_point.x, a_point.y, a_point.z });
	}

	inline json PointToJson(const RE::NiPoint3& a_point)
	{
		return json::array({ a_point.x, a_point.y, a_point.z });
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

}
