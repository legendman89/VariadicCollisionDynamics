#include "posefixes.hpp"

namespace PoseFixes {

    bool IsLedgeSitting(const RE::Actor* a_actor)
    {
        if (!a_actor) {
            return false;
        }

        const auto furniturePtr = const_cast<RE::Actor*>(a_actor)->GetOccupiedFurniture().get();
        const auto* furniture = furniturePtr.get();
        const auto* furnitureBase = furniture ? furniture->GetBaseObject() : nullptr;

        // Gort's unnamed Skyrim.esm seat marker has no runtime idle record.
        if (furnitureBase && furnitureBase->GetFormID() == 0x000C4827) {
            return true;
        }

        const auto* highProcess = a_actor->GetHighProcess();
        const auto* process = a_actor->GetActorRuntimeData().currentProcess;
        const auto* middleHigh = process ? process->middleHigh : nullptr;

        const RE::TESIdleForm* idles[]{ highProcess ? highProcess->currentProcessIdle : nullptr, middleHigh ? middleHigh->furnitureIdle : nullptr };
        for (const auto* idle : idles) {
            if (!idle) {
                continue;
            }

            const auto* editorID = idle->GetFormEditorID();
            const auto* eventName = idle->animEventName.c_str();
            if ((editorID && VCD::ContainsInsensitive(editorID, "IdleSitLedgeEnter")) || (eventName && VCD::ContainsInsensitive(eventName, "IdleSitLedgeEnter"))) {
                return true;
            }
        }

        return false;
    }

}
