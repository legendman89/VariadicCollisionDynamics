#pragma once

#include "logger.hpp"
#include "settings.hpp"

namespace API
{ 

namespace CustomKeyGlobals
{
	inline RE::TESGlobal* g_vLock{ nullptr };
	inline RE::TESGlobal* g_vState{ nullptr };

	inline RE::TESGlobal* LookupGlobal(const std::string& a_editorID)
	{
		if (a_editorID.empty()) {
			return nullptr;
		}

		auto* global = RE::TESForm::LookupByEditorID<RE::TESGlobal>(a_editorID);
		if (!global) {
			logger::warn("Global with editor ID '{}' was not found", a_editorID);
		}
		return global;
	}

	inline RE::TESGlobal* LookupVLock()
	{
		return LookupGlobal(Settings::GetSettings().customKeyGlobalVLock);
	}

	inline RE::TESGlobal* LookupVState()
	{
		return LookupGlobal(Settings::GetSettings().customKeyGlobalVState);
	}

	// Call at kDataLoaded, and again if the user edits either string.
	inline void Resolve()
	{
		g_vLock = LookupVLock();
		g_vState = LookupVState();
	}
}

}
