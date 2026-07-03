 
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

//thirdperson state inlines a function to set camera phantom position inside itself
// so hooking this func works to reach that inlined func allows us to decouple camera pos from collision
void ThirdPersonState_SetRotation::thunk(
	RE::ThirdPersonState* a_state,
	float* rotation,
	bool a_flag,
	bool a_someFlag)
{
	// Call original first - this calculates everything
	func(a_state, rotation, a_flag, a_someFlag);

	if (!a_state) return;

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

	float worldScale = RE::bhkWorld::GetWorldScale();

	translation.quad.m128_f32[0] += cameraGlobals::CollisionPosX * worldScale;
	translation.quad.m128_f32[1] += cameraGlobals::CollisionPosY * worldScale;

	// logger::info("Phantom AFTER Z: {:.2f}", translation.quad.m128_f32[2]);
}

void ThirdPersonState_SetRotation::Install()
{
	SKSE::AllocTrampoline(1 << 7);
	auto& trampoline = SKSE::GetTrampoline();

	// IDA / Ghidra Address [AE 1408e8640 REL: 50911]

	std::array targets{
		std::make_pair(
			RELOCATION_ID(49960, 50896),  // AE 1408E7810 SE 14084F490
			REL::VariantOffset{ 0x144, 0x1E8, 0}  // Offset to the CALL instruction
		),
		std::make_pair(
			RELOCATION_ID(49966, 50902),  // AE sub_1408E7C50 SE 14084f830  ThirdPersonState::ResetFreeRotation() 
			REL::VariantOffset{ 0x5B, 0x5B, 0 }  // Offset to the CALL instruction
		)
	};

	for (auto& [id, offset] : targets) {
		REL::Relocation<std::uintptr_t> target(id, offset);
		func = trampoline.write_call<5>(target.address(), thunk);
	}

	logger::info("ThirdPersonState_SetRotation hook installed");
}

//called for arrows, player character
void BhkSimpleShapePhantom_SetPosition::thunk(RE::bhkSimpleShapePhantom* phantom, RE::hkVector4* position)
{
	auto playerCamera = RE::PlayerCamera::GetSingleton();
	if (!playerCamera) return func(phantom, position);

		auto& rtd = playerCamera->GetRuntimeData();
		if (!rtd.unk120) return func(phantom, position);

			auto* cameraPhantom = rtd.unk120->unk00.get();

			if (phantom == cameraPhantom) {
				float worldScale = RE::bhkWorld::GetWorldScale();
				position->quad.m128_f32[0] += cameraGlobals::CollisionPosX * worldScale;
				position->quad.m128_f32[1] += cameraGlobals::CollisionPosY * worldScale;
			}
		
	
	func(phantom, position);
}

void BhkSimpleShapePhantom_SetPosition::Install()
{
	SKSE::AllocTrampoline(1 << 7);
	auto& trampoline = SKSE::GetTrampoline();

	std::array targets{
			std::make_pair(
				RELOCATION_ID(32271, 33008),  //  SE 1404F4C00 AE 14054fd00  - CheckCharacterCollision
				REL::VariantOffset{ 0x29F, 0x27B, 0 }  
			)
	};

	for (auto& [id, offset] : targets) {
		REL::Relocation<std::uintptr_t> target(id, offset);
		func = trampoline.write_call<5>(target.address(), thunk);
	}

	logger::info("BhkSimpleShapePhantom_SetPosition hook installed");
}


void Hook::Install() {


	Hook::PlayerUpdate::Install();

	Hook::SneakHandlerProcessButton::Install();

	MenuTopicManagerHook::Install();

	ThirdPersonState_SetRotation::Install();

	BhkSimpleShapePhantom_SetPosition::Install();


}

