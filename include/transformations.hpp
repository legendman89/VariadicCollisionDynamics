#pragma once

#include "helper.hpp"

#include <string>
#include <vector>

namespace VCD {

	struct TransformationRule
	{
		std::vector<std::string> raceEditorIDs{};
		std::vector<std::string> actorKeywordEditorIDs{};
		std::vector<std::string> effectKeywordEditorIDs{};
		std::vector<RE::BGSKeyword*> actorKeywords{};
		std::vector<RE::BGSKeyword*> effectKeywords{};
	};

	struct TransformationRules
	{
		TransformationRule vampireLord{};
		TransformationRule werewolf{};
	};

	inline TransformationRule MakeDefaultVampireLordRule()
	{
		TransformationRule rule{};
		rule.raceEditorIDs.emplace_back("DLC1VampireBeastRace");
		return rule;
	}

	inline TransformationRule MakeDefaultWerewolfRule()
	{
		TransformationRule rule{};
		rule.raceEditorIDs.emplace_back("WerewolfBeastRace");
		return rule;
	}

	inline TransformationRules MakeDefaultTransformationRules()
	{
		TransformationRules rules{};
		rules.vampireLord = MakeDefaultVampireLordRule();
		rules.werewolf = MakeDefaultWerewolfRule();
		return rules;
	}

	inline bool ContainsEditorID(const std::vector<std::string>& a_editorIDs, const char* a_editorID)
	{
		if (!a_editorID) {
			return false;
		}

		for (const auto& editorID : a_editorIDs) {
			if (editorID == a_editorID) {
				return true;
			}
		}

		return false;
	}

	inline bool ActorHasAnyKeyword(const RE::Actor* a_actor, const std::vector<RE::BGSKeyword*>& a_keywords)
	{
		if (!a_actor) {
			return false;
		}

		for (const auto* keyword : a_keywords) {
			if (keyword && a_actor->HasKeyword(keyword)) {
				return true;
			}
		}

		return false;
	}

	inline bool ActorHasAnyEffectKeyword(const RE::Actor* a_actor, const std::vector<RE::BGSKeyword*>& a_keywords)
	{
		auto* actor = const_cast<RE::Actor*>(a_actor);
		if (!actor) {
			return false;
		}

		for (auto* keyword : a_keywords) {
			if (keyword && actor->HasMagicEffectWithKeyword(keyword)) {
				return true;
			}
		}

		return false;
	}

	inline bool MatchesRace(const RE::Actor* a_actor, const TransformationRule& a_rule)
	{
		const auto* race = a_actor ? a_actor->GetRace() : nullptr;
		const auto* editorID = race ? race->GetFormEditorID() : nullptr;
		return ContainsEditorID(a_rule.raceEditorIDs, editorID);
	}

	inline bool MatchesTransformation(const RE::Actor* a_actor, const TransformationRule& a_rule)
	{
		return MatchesRace(a_actor, a_rule) ||
		       ActorHasAnyKeyword(a_actor, a_rule.actorKeywords) ||
		       ActorHasAnyEffectKeyword(a_actor, a_rule.effectKeywords);
	}

	inline TransformationRules& GetTransformationRules()
	{
		static auto rules = MakeDefaultTransformationRules();
		return rules;
	}

	inline fs::path GetTransformationsPath()
	{
		return GetPluginDataPath() / "Transformations.ini";
	}

	inline bool IsVampireLord(const RE::Actor* a_actor)
	{
		return MatchesTransformation(a_actor, GetTransformationRules().vampireLord);
	}

	inline bool IsWerewolf(const RE::Actor* a_actor)
	{
		return MatchesTransformation(a_actor, GetTransformationRules().werewolf);
	}

	void LoadTransformations();

}
