 
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

		if (!Raycast::canStandUp(standingHeight)) {
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

// Called on dialogue menu open and close.
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

// Thirdperson state inlines a function to set camera phantom position inside itself
// so hooking this func works to reach that inlined func allows us to decouple camera pos from collision
void ThirdPersonState_SetRotation::thunk(
	RE::ThirdPersonState* a_state,
	float* rotation,
	bool a_flag,
	bool a_someFlag)
{
	func(a_state, rotation, a_flag, a_someFlag);

	if (!a_state) return;

	auto* playerCamera = RE::PlayerCamera::GetSingleton();
	if (!playerCamera) return;

	auto& cameraRTD = playerCamera->GetRuntimeData();
	if (!cameraRTD.unk120) return;

	auto* phantom = cameraRTD.unk120->unk00.get();
	if (!phantom) return;

	auto manager = VCD::Manager::GetSingleton();
	auto hkpPhantom = manager.GetCameraSimpleShapePhantom(phantom);
	if (!hkpPhantom) return;

	auto& translation = hkpPhantom->motionState.transform.translation;
	float worldScale = RE::bhkWorld::GetWorldScale();
	const auto& cameraCollision = VCD::GetCameraCollisionState();

	float yaw = playerCamera->GetRuntimeData2().yaw;

	// Calculate forward and right vectors in world space
	float forwardX = std::sin(yaw);
	float forwardY = std::cos(yaw);
	float rightX = std::cos(yaw);
	float rightY = -std::sin(yaw);

	// Transform local offsets to world space
	float worldOffsetX =
		(rightX * cameraCollision.positionX) +
		(forwardX * cameraCollision.positionY);

	float worldOffsetY =
		(rightY * cameraCollision.positionX) +
		(forwardY * cameraCollision.positionY);


	//Log MUST match the phantoms collideable position below in the player camera linear cast hook below
	logger::info("[SYNC-SetRotation] yaw={:.4f} offsetX={:.4f} offsetY={:.4f} translationScale=({:.6f},{:.6f}) -> ({:.6f},{:.6f})",
		yaw, worldOffsetX, worldOffsetY,
		translation.quad.m128_f32[0], translation.quad.m128_f32[1],
		translation.quad.m128_f32[0] + worldOffsetX * worldScale,
		translation.quad.m128_f32[1] + worldOffsetY * worldScale);


	// Apply the transformed offset
	translation.quad.m128_f32[0] += worldOffsetX * worldScale;
	translation.quad.m128_f32[1] += worldOffsetY * worldScale;
}

void ThirdPersonState_SetRotation::Install()
{
	auto& trampoline = SKSE::GetTrampoline();

	// IDA / Ghidra Address [AE 1408e8640 REL: 50911]

	std::array targets{
		std::make_pair(
			RELOCATION_ID(49960, 50896),  // AE 1408E7810 SE 14084F490
			REL::VariantOffset{ 0x144, 0x1E8, 0x147 }  // Offset to the CALL instruction
		),
		std::make_pair(
			RELOCATION_ID(49966, 50902),  // AE sub_1408E7C50 SE 14084f830  ThirdPersonState::ResetFreeRotation() 
			REL::VariantOffset{ 0x5B, 0x5B, 0x5B }  // Offset to the CALL instruction
		)
	};

	for (auto& [id, offset] : targets) {
		REL::Relocation<std::uintptr_t> target(id, offset);
		func = trampoline.write_call<5>(target.address(), thunk);
	}

	logger::info("ThirdPersonState_SetRotation hook installed");
}

void CameraLinearCastHook::thunk(
	std::int64_t* param_1,   // unk120 phantom wrapper
	std::int64_t* param_2,   // bhkWorld
	float* param_3,          // TARGET pos - phantom moves here
	float* param_4,          // CURRENT pos - linear cast starts here
	float* param_5,          // output
	std::uint64_t* param_6,  // tesobject refr
	float          param_7   // radius
)
{
	auto* playerCamera = RE::PlayerCamera::GetSingleton();
	if (!playerCamera) return	func(
		param_1,
		param_2,
		param_3,
		param_4,
		param_5,
		param_6,
		param_7
	);

	auto& cameraRTD = playerCamera->GetRuntimeData();
	if (!cameraRTD.unk120) return	func(
		param_1,
		param_2,
		param_3,
		param_4,
		param_5,
		param_6,
		param_7
	);

	auto* phantom = cameraRTD.unk120->unk00.get();
	if (!phantom) return 	func(
		param_1,
		param_2,
		param_3,
		param_4,
		param_5,
		param_6,
		param_7
	);

	auto manager = VCD::Manager::GetSingleton();
	auto hkpPhantom = manager.GetCameraSimpleShapePhantom(phantom);
	if (!hkpPhantom) return 	func(
		param_1,
		param_2,
		param_3,
		param_4,
		param_5,
		param_6,
		param_7
	);

	auto* player = RE::PlayerCharacter::GetSingleton();
	if (player) {

		auto playerPos = player->GetPosition(); 

		logger::info("playerPos = {}", playerPos); 

		float yaw = playerCamera->GetRuntimeData2().yaw;

		// Forward vector (camera facing direction)
		float forwardX = std::sin(yaw);
		float forwardY = std::cos(yaw);
		// Right vector (perpendicular, 90 degrees)
		float rightX = std::cos(yaw);
		float rightY = -std::sin(yaw);

		const auto& col = VCD::GetCameraCollisionState();

		float worldOffsetX = (forwardX * col.positionY) + (rightX * col.positionX);
		float worldOffsetY = (forwardY * col.positionY) + (rightY * col.positionX);

		//Log MUST match the position for the phantom shape in set rotation hook above
		logger::debug("[SYNC-LinearCast] yaw={:.4f} offsetX={:.4f} offsetY={:.4f} param3=({:.2f},{:.2f}) -> ({:.2f},{:.2f})",
			yaw, worldOffsetX, worldOffsetY,
			param_3[0], param_3[1],
			param_3[0] + worldOffsetX, param_3[1] + worldOffsetY);

		// Apply to param_3
		param_3[0] += worldOffsetX;
		param_3[1] += worldOffsetY;
	
	}

	func(
		param_1,
		param_2,
		param_3,
		param_4,
		param_5,
		param_6,
		param_7
	);

}

// AE 33007     14054F720
void CameraLinearCastHook::Install()
{
	auto& trampoline = SKSE::GetTrampoline();

	//AE  UpdatePlayerCameraTransforms = 50832 (FUN_1408e48d0) Offset = 0x38E
	// SE UpdatePlayerCameraTransforms = 49899 (FUN_14084c870) Offset = 0x300
	std::array targets{
		std::make_pair(
			RELOCATION_ID(49899, 50832),  // UpdatePlayerCameraTransforms
			REL::VariantOffset{ 0x300, 0x38E, 0x300}  // Offset to CALL FUN_14054f720
		)
	};

	for (auto& [id, offset] : targets) {
		REL::Relocation<std::uintptr_t> target(id, offset);
		logger::info("CameraLinearCastHook target: {:X}", target.address());
		func = trampoline.write_call<5>(target.address(), thunk);
	}

	logger::info("CameraLinearCastHook installed");
}

void Hook::Install() {
	SKSE::AllocTrampoline(1 << 8);

	CameraLinearCastHook::Install(); 

	Hook::PlayerUpdate::Install();

	Hook::SneakHandlerProcessButton::Install();

	MenuTopicManagerHook::Install();

	ThirdPersonState_SetRotation::Install();
}

