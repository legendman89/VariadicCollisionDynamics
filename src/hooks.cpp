 
#include "draw.hpp"
#include "hooks.hpp"
#include "helper.hpp"
#include "logger.hpp"
#include "manager.hpp"
#include "dynamics.hpp"
#include "settings.hpp"
#include "raycast.hpp"
#include "posefixes.hpp"
#include <CLibUtilsQTR/DrawDebug.hpp>

using namespace Hook; 
using namespace DebugAPI_IMPL;

void PlayerUpdate::thunk(RE::PlayerCharacter* player, float delta) {

	func(player, delta);

	if (!player) {
		logger::warn("Player is null, cannot update.");
		return;
	}

	static bool initialized = false;

	if (!initialized) {
		initialized = true;

		const auto playerIsSneaking = player->IsSneaking();

		logger::info("player is sneaking on startup = {}", playerIsSneaking);

		//set playerIsSneaking when first loaded
		if (playerIsSneaking && Settings::GetSettings().fixPlayerSneaking) {
			auto& manager = VCD::Manager::GetSingleton();
			manager.FixSneakingPose(player, PoseFixes::PlayerPose(player), false);
		}
	}

	Dynamics::Update(player);

	Dynamics::UpdateNPCs(player);

	const auto& settings = Settings::GetSettings();
	if (settings.drawCollision || settings.drawNearbyActors) {
		DebugAPI::GetSingleton()->LinesToDraw.clear();
	}

	if (settings.drawCollision) {
		Draw::DrawPlayerBumper();
	}

	if (settings.drawNearbyActors) {
		Draw::DrawNearbyActorBumpers();
	}

	if (settings.drawCameraCollision) {
		Draw::DrawCameraBumper(); 
	}

	if (settings.drawCollision || settings.drawNearbyActors || settings.drawCameraCollision) {
		DebugAPI::GetSingleton()->Update();
	}
}

void PlayerUpdate::Install()
{
	func = REL::Relocation<std::uintptr_t>(RE::PlayerCharacter::VTABLE[0]).write_vfunc(0xAD, thunk);
	logger::info("Player update hook installed.");
}

// isUp() called only once per sneak key press
void SneakHandlerProcessButton::thunk(
	RE::SneakHandler* a_this,
	RE::ButtonEvent* a_event,
	RE::PlayerControlsData* a_data)
{

	if (!a_this || !a_event || !a_data)
		return func(a_this, a_event, a_data);

	if (!Settings::GetSettings().fixPlayerSneaking) {
		return func(a_this, a_event, a_data);
	}

	auto* player = RE::PlayerCharacter::GetSingleton();
	if (!player) return func(a_this, a_event, a_data);

	auto& manager = VCD::Manager::GetSingleton();
	if (player->IsSneaking()) {
		auto standingHeight = manager.GetStandingCapsuleHeight(player);
		if (standingHeight <= 0.0F) {
			standingHeight = player->GetHeight();
		}

		if (!raycast::canStandUp(standingHeight)) {
			return;
		}
	}

	if (a_event->IsUp()) {
	
		// let game process button first so isSneaking() returns valid state
		func(a_this, a_event, a_data);

		const bool gameIsSneaking = player->IsSneaking();

		logger::info("ProcessButton fired, gameIsSneaking={}", gameIsSneaking);

		const auto poseFlags = PoseFixes::PlayerPose(player);
		if (gameIsSneaking) {
			logger::info("player is sneaking, shrinking collision");
		}
		else {
			logger::info("player stood up, restoring collision");
		}
		manager.FixSneakingPose(player, poseFlags, false);

		return; // already called func above
	}

	return func(a_this, a_event, a_data);
}

void SneakHandlerProcessButton::Install()
{
	if (VCD::IsDynamicCollisionAdjustmentInstalled()) {
        logger::info("DynamicCollisionAdjustment detected; sneak hook disabled.");
        return;
    }
	
	func = REL::Relocation<std::uintptr_t>(RE::SneakHandler::VTABLE[0]).write_vfunc(0x04, thunk);
	logger::info("process sneak button hook installed");
}

// called on dialogue menu open and close
RE::BSEventNotifyControl MenuTopicManagerHook::ProcessMenuOpenCloseEvent(
	RE::MenuTopicManager* a_this,
	const RE::MenuOpenCloseEvent* a_event,
	RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_eventSource)
{
	if (!a_this || !a_event ||
		a_event->menuName != RE::DialogueMenu::MENU_NAME) return func(a_this, a_event, a_eventSource);

	if (!Settings::GetSettings().enableCameraDynamics) {
		return func(a_this, a_event, a_eventSource);
	}

	if (a_event->opening) {
		Dynamics::ApplyCameraPreset(VCD::Preset::kCameraDialogue); 
	}

	// menu closed
	else {

		auto player = RE::PlayerCharacter::GetSingleton(); 
		if (!player) {
			return func(a_this, a_event, a_eventSource);
		}

		auto* cell = player->GetParentCell();
		if (!cell) {
			return func(a_this, a_event, a_eventSource);
		}
		// put correct camrea preset back after convo ended
		auto cameraPreset = Dynamics::GetCellCameraPreset(cell);

		Dynamics::ApplyCameraPreset(cameraPreset);
	}
	
	return func(a_this, a_event, a_eventSource);
}

void MenuTopicManagerHook::Install()
{
	REL::Relocation<std::uintptr_t> vtbl{ RE::VTABLE_MenuTopicManager[0] };

	func = vtbl.write_vfunc(1, ProcessMenuOpenCloseEvent);

	logger::info("Installed MenuTopicManager MenuOpenCloseEvent hook");
}

//best hook I could find that would let me set camera collision position 
// without my changes being overwritten by camera update hooks next frame
void ThirdPersonState_SetRotation::thunk(
	RE::ThirdPersonState* a_state,
	RE::NiPoint3* rotation,
	bool a_flag,
	bool a_someFlag)
{
	// Call original first - this calculates everything
	func(a_state, rotation, a_flag, a_someFlag);

	auto manager = VCD::Manager::GetSingleton(); 

	auto* playerCamera = RE::PlayerCamera::GetSingleton();
	if (!playerCamera) return;

	auto& cameraRTD = playerCamera->GetRuntimeData();
	if (!cameraRTD.unk120) return;

	auto* phantom = cameraRTD.unk120->unk00.get();
	if (!phantom) return;

	auto hkpPhantom = manager.GetCameraSimpleShapePhantom(phantom); 

	// Modify ONLY the phantom's motion state
	auto& translation = hkpPhantom->motionState.transform.translation;

	logger::info("Phantom BEFORE Z: {:.2f}", translation.quad.m128_f32[2]);

	// Apply your offset - ONLY to the phantom!
	translation.quad.m128_f32[0] += cameraGlobals::CollisionPosX;
	translation.quad.m128_f32[1] += cameraGlobals::CollisionPosY;

	logger::info("Phantom AFTER Z: {:.2f}", translation.quad.m128_f32[2]);
}

void ThirdPersonState_SetRotation::Install()
{
	SKSE::AllocTrampoline(1 << 7);
	auto& trampoline = SKSE::GetTrampoline();

	// Hook BOTH call sites to FUN_1408e8640
	std::vector<REL::Offset> sites = {
		REL::Offset(0x8E79F8),  // Call site 1
		REL::Offset(0x8E7CAB)   // Call site 2
	};

	// Hook the first one to get the function pointer
	REL::Relocation<std::uintptr_t> firstSite{ sites[0] };
	func = trampoline.write_call<5>(firstSite.address(), thunk);

	// Hook the second one
	REL::Relocation<std::uintptr_t> secondSite{ sites[1] };
	trampoline.write_call<5>(secondSite.address(), thunk);

	logger::info("ThirdPersonState_SetRotation hook installed");
}

void Hook::Install() {


	Hook::PlayerUpdate::Install();

	Hook::SneakHandlerProcessButton::Install();

	MenuTopicManagerHook::Install();

	ThirdPersonState_SetRotation::Install();
}
