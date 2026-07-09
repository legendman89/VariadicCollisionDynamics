#include "transformations.hpp"

#include "ini.hpp"
#include "logger.hpp"

#include <system_error>

namespace VCD {

	void ResolveKeywords(const char* a_section, const char* a_key, const std::vector<std::string>& a_editorIDs, std::vector<RE::BGSKeyword*>& a_keywords)
	{
		a_keywords.clear();
		a_keywords.reserve(a_editorIDs.size());

		for (const auto& editorID : a_editorIDs) {
			auto* keyword = RE::TESForm::LookupByEditorID<RE::BGSKeyword>(editorID);
			if (!keyword) {
				logger::warn("Transformations.ini [{}] {} keyword not found: {}", a_section, a_key, editorID);
				continue;
			}

			a_keywords.push_back(keyword);
		}
	}

	void LoadEditorIDs(const INI::Data& a_ini, const char* a_section, const char* a_key, std::vector<std::string>& a_editorIDs)
	{
		if (const auto value = INI::FindValue(a_ini, a_section, a_key)) {
			a_editorIDs = INI::SplitValue(*value);
		}
		else {
			INI::NormalizeValues(a_editorIDs);
		}
	}

	void LoadRule(const INI::Data& a_ini, const char* a_section, TransformationRule& a_rule)
	{
		LoadEditorIDs(a_ini, a_section, "RaceEditorIDs", a_rule.raceEditorIDs);
		LoadEditorIDs(a_ini, a_section, "ActorKeywordEditorIDs", a_rule.actorKeywordEditorIDs);
		LoadEditorIDs(a_ini, a_section, "EffectKeywordEditorIDs", a_rule.effectKeywordEditorIDs);

		ResolveKeywords(a_section, "ActorKeywordEditorIDs", a_rule.actorKeywordEditorIDs, a_rule.actorKeywords);
		ResolveKeywords(a_section, "EffectKeywordEditorIDs", a_rule.effectKeywordEditorIDs, a_rule.effectKeywords);
	}

	void LoadTransformations()
	{
		auto& rules = GetTransformationRules();
		rules = MakeDefaultTransformationRules();

		const auto path = GetTransformationsPath();
		const auto pathStr = ToUTF8(path);
		std::error_code ec;
		if (!fs::exists(path, ec)) {
			logger::warn("Transformations.ini unavailable at {}; using vanilla transformation race defaults.", pathStr);
			return;
		}

		const auto ini = INI::Read(path);
		LoadRule(ini, "VampireLord", rules.vampireLord);
		LoadRule(ini, "Werewolf", rules.werewolf);

		logger::info(
			"Loaded Transformations.ini: VampireLord races={}, actorKeywords={}, effectKeywords={}; Werewolf races={}, actorKeywords={}, effectKeywords={}",
			rules.vampireLord.raceEditorIDs.size(),
			rules.vampireLord.actorKeywords.size(),
			rules.vampireLord.effectKeywords.size(),
			rules.werewolf.raceEditorIDs.size(),
			rules.werewolf.actorKeywords.size(),
			rules.werewolf.effectKeywords.size());
	}

}
