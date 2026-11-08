#include "step.hpp"
#include "draw.hpp"
#include "hooks.hpp"
#include "helper.hpp"
#include "logger.hpp"
#include "manager.hpp"
#include "dynamics.hpp"
#include "settings.hpp"
#include "raycast.hpp"
#include "posefixes.hpp"
#include "collision_layers.hpp"

#include <CLibUtilsQTR/DrawDebug.hpp>
#include <CLibUtilsQTR/Hooks.hpp>
#include <array>
#include <cmath>

using namespace Hook; 
using namespace DebugAPI_IMPL;

void CharacterProxyProcessConstraints::thunk(
	RE::bhkCharProxyController* a_this,
	const RE::hkpCharacterProxy* a_proxy,
	const RE::hkArray<RE::hkpRootCdPoint>& a_manifold,
	RE::hkpSimplexSolverInput& a_input)
{
	if (!a_this || !a_proxy || !StepConstraints::IsManagedCharacter(a_this)) {
		func(a_this, a_proxy, a_manifold, a_input);
		return;
	}

	const auto constraintCountBefore = a_input.numConstraints;
	func(a_this, a_proxy, a_manifold, a_input);
	StepConstraints::Fix(a_this, a_proxy, a_manifold, a_input, constraintCountBefore);
}

void CharacterProxyProcessConstraints::Install()
{
	REL::Relocation<std::uintptr_t> vtable{ RE::VTABLE_bhkCharProxyController[0] };
	func = vtable.write_vfunc(0x01, thunk);
	logger::info("Character step compatibility hook installed.");
}

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

	if (!Draw::IsSupported()) {
		return;
	}

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
	func = REL::Relocation<std::uintptr_t>(RE::PlayerCharacter::VTABLE[0]).write_vfunc(REL::Relocate(0xAD,0xAD,0xAF), thunk);
	logger::info("Player update hook installed.");
}

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

	// isUp() called only once per sneak key press
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
	
	// AE 1.7.99 added MotionGesture and Sixaxis handlers before ProcessButton.
	const auto processButtonOffset = REL::Module::IsAtLeast(SKSE::RUNTIME_SSE_1_7_99) ? 0x06 : 0x04;
	func = REL::Relocation<std::uintptr_t>(RE::SneakHandler::VTABLE[0]).write_vfunc(processButtonOffset, thunk);
	logger::info("Process sneak button hook installed");
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

// allows us to decouple camera position from collision for first person camera 
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

// AE 33007, 14054F720
void CameraLinearCastHook::Install()
{
	auto& trampoline = SKSE::GetTrampoline();

	// AE UpdatePlayerCameraTransforms = 50832 (FUN_1408e48d0) Offset = 0x38E
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

//Used to make our own collision layer filtering logic, 
//Player or player camera doesent come through here see playerFilter below
std::uint8_t* CollisionFilterHooks::actorFilter(
	const RE::hkpShapeCollectionFilter* a_this,
	std::uint8_t* a_result,
	const RE::hkpCollisionInput& a_input,
	const RE::hkpCdBody& a_bodyA,
	const RE::hkpCdBody& a_bodyB,
	const RE::hkpShapeContainer& a_shapeA,
	const RE::hkpShapeContainer& a_shapeB,
	RE::hkpShapeKey a_keyA,
	RE::hkpShapeKey a_keyB)
{
	auto* result = actorFunc(
		a_this,
		a_result,
		a_input,
		a_bodyA,
		a_bodyB,
		a_shapeA,
		a_shapeB,
		a_keyA,
		a_keyB);

	if (!result || !*result || !Settings::GetSettings().enableNPCDynamics) {
		return result;
	}

	const auto getActor = [](const RE::hkpCdBody& a_body) -> RE::Actor* {
		auto* root = &a_body;

		while (root->parent) {
			root = root->parent;
		}

		const auto& collidable = static_cast<const RE::hkpCollidable&>(*root);

		if (collidable.GetCollisionLayer() != RE::COL_LAYER::kCharController) {
			return nullptr;
		}

		auto* ref = RE::TESHavokUtilities::FindCollidableRef(collidable);
		return ref ? ref->As<RE::Actor>() : nullptr;
		};

	const auto checkNPC = [](RE::Actor* a_actor, RE::COL_LAYER a_otherLayer) -> bool {
		if (!a_actor) {
			return false;
		}

		
		auto* actorState = Dynamics::FindNPCPresetState(a_actor->GetFormID());

		if (!actorState) {
			return false;
		}

		const auto index = std::to_underlying(a_otherLayer);

		if (index >= CollisionLayers::kCollisionLayerNames.size()) {
			return false;
		}

		auto& manager = VCD::Manager::GetSingleton();
		const auto* presetConfig = manager.GetPresetConfig(actorState->preset);

		if (!presetConfig) {
			return false;
		}

		const auto& layerName = CollisionLayers::kCollisionLayerNames[index];

		const bool overridden = CollisionLayers::IsLayerOverridden(
			presetConfig->collisionLayers,
			layerName);

		logger::debug(
			"[NPC Collision] {} [{:08X}] | Preset: {} | Other layer: {} | Override: {}",
			a_actor->GetDisplayFullName(),
			a_actor->GetFormID(),
			VCD::PresetName(actorState->preset),
			layerName,
			overridden);

		return overridden;
		};

	const auto layerA =
		static_cast<const RE::hkpCollidable&>(a_bodyA).GetCollisionLayer();

	const auto layerB =
		static_cast<const RE::hkpCollidable&>(a_bodyB).GetCollisionLayer();

	auto* actorA = getActor(a_bodyA);
	auto* actorB = getActor(a_bodyB);

	//see if char controller collision layer meaning its a actor amd see if actors Preset should return false on this collision interaction
	if (checkNPC(actorA, layerB)) {
		logger::debug("[NPC Collision] Rejected collision through body A");
		*result = false;
		return result;
	}

	//see if char controller collision layer meaning its a actor amd see if actors Preset should return false on this collision interaction
	if (checkNPC(actorB, layerA)) {
		logger::debug("[NPC Collision] Rejected collision through body B");
		*result = false;
		return result;
	}

	return result;
}

// at this write_call address only the player and his camera comes through here 
bool CollisionFilterHooks::playerFilter(
	RE::bhkCollisionFilter* filter,
	RE::CFilter filterA,
	RE::CFilter filterB)
{
	//char controller is used for Npcs, Player
	constexpr auto actorLayer = RE::COL_LAYER::kCharController;

	//Camera Pick is confirmed used for the cam over kCameraSpheres
	constexpr auto cameraLayer = RE::COL_LAYER::kCameraPick;
	const auto layerA = filterA.GetCollisionLayer();
	const auto layerB = filterB.GetCollisionLayer();

	//player camera collision layer logic here
	if (layerA == RE::COL_LAYER::kCameraPick ||
		layerB == RE::COL_LAYER::kCameraPick && Settings::GetSettings().enableCameraDynamics) {
		const auto otherLayer = layerA == RE::COL_LAYER::kCameraPick ? layerB : layerA;
		const auto index = std::to_underlying(otherLayer);
		if (index >= CollisionLayers::kCollisionLayerNames.size()) {
			return playerFunc(filter, filterA, filterB);
		}
		const auto& state = Dynamics::GetPresetState();
		auto& manager = VCD::Manager::GetSingleton();
		const auto* cameraConfig = manager.GetPresetConfig(state.currentCamera);
		if (!cameraConfig) {
			return playerFunc(filter, filterA, filterB);
		}
		const auto& layerName = CollisionLayers::kCollisionLayerNames[index];
		if (CollisionLayers::IsLayerOverridden(
			cameraConfig->collisionLayers, layerName)) {
			return false;
		}
		return playerFunc(filter, filterA, filterB);
	}

	if (layerA != actorLayer && layerB != actorLayer) {
		return playerFunc(filter, filterA, filterB);
	}
	const auto otherLayer = layerA == actorLayer ? layerB : layerA;
	const auto index = std::to_underlying(otherLayer);

	// Omit the unknown layers.
	if (index >= CollisionLayers::kCollisionLayerNames.size()) {
		return playerFunc(filter, filterA, filterB);
	}
	const auto& state = Dynamics::GetPresetState();
	auto& manager = VCD::Manager::GetSingleton();
	const auto* presetConfig = manager.GetPresetConfig(state.current);
	if (!presetConfig) {
		return playerFunc(filter, filterA, filterB);
	}

	//return false if active preset says the current collision layers should not interact
	const auto& layerName = CollisionLayers::kCollisionLayerNames[index];
	if (CollisionLayers::IsLayerOverridden(
		presetConfig->collisionLayers, layerName)) {
		return false;
	}
	return playerFunc(filter, filterA, filterB);
}

void CollisionFilterHooks::Install()
{
	auto& trampoline = SKSE::GetTrampoline();

	REL::Relocation<std::uintptr_t> vtable{ RE::VTABLE_bhkCollisionFilter[2] };

	actorFunc = vtable.write_vfunc(0, actorFilter);

	REL::Relocation<std::uintptr_t> playerTarget{
		RELOCATION_ID(76677, 78549), 0xBC
	};
	playerFunc = trampoline.write_call<5>(
		playerTarget.address(), playerFilter);

	logger::info("Collision Filter Hooks installed");
}

//Collision layer "kWeapon" does not take into account hitting/damaging player or NPCS
//I wanted that functionality so with this hook, if kWeapon layer is disabled for active preset of any actor
//Said actor cannot be hit by weapons. 
void MeleeHitHookAE::func(
	RE::Actor* a_aggressor,
	RE::Actor* a_victim,
	std::int64_t a3,
	char a4)
{
	if (CollisionLayers::ShouldRejectMeleeHit(a_aggressor, a_victim)) {
		return;
	}
	originalFunc(a_aggressor, a_victim, a3, a4);
}

void MeleeHitHookAE::Install()
{
	if (!REL::Module::IsAE()) {
		return;
	}

	// i dont think this func exists in SE 
	REL::Relocation<std::uintptr_t> target{
		REL::RelocationID(0, 38627)
	};

	//detour but safe so others can still hook this without crashing
	auto trampoline = clib_utilsQTR::write_prologue_hook(
		target.address(),
		&func
	);

	if (trampoline) {
		originalFunc = reinterpret_cast<HitFunction*>(trampoline);
	}

	logger::info("Installed MeleeHitHookAE");
}

void MeleeHitHookSE::func(
	RE::Character* a_aggressor,
	RE::Actor* a_victim,
	std::int64_t a3,
	char a4)
{
	if (CollisionLayers::ShouldRejectMeleeHit(a_aggressor, a_victim)) {
		return;
	}
	originalFunc(a_aggressor, a_victim, a3, a4);
}

void MeleeHitHookSE::Install()
{
	// let VR and SE through
	if (REL::Module::IsAE()) {
		return;
	}

	auto& trampoline = SKSE::GetTrampoline();

	REL::Relocation<std::uintptr_t> meleeTarget{
		REL::ID(37650)
	};

	originalFunc = trampoline.write_call<5>(
		meleeTarget.address() + 0x38B,
		func);

	logger::info("Installed MeleeHitHookSE");
}

//to block kill moves if player / npc has weapon layer enabled
// kill moves needed to be blocked in addition to the melee hooks above
bool AdmitAnimationHook::thunk(
	void* a_manager,
	RE::BGSActionData* a_data)
{
	constexpr std::string_view killmovePrefix = "pa_Kill";

	if (!a_data) {
		return originalFunc(a_manager, a_data);
	}

	const auto& event = a_data->animEvent;

	if (event.empty()) {
		return originalFunc(a_manager, a_data);
	}

	if (_strnicmp(event.c_str(), killmovePrefix.data(),
		killmovePrefix.size()) == 0 &&
		a_data->target.get() == RE::PlayerCharacter::GetSingleton()) {
		return false;
	}

	return originalFunc(a_manager, a_data);
}

void AdmitAnimationHook::Install()
{
	auto& trampoline = SKSE::GetTrampoline();

	if (REL::Module::IsAE()) {
		REL::Relocation<std::uintptr_t> target1{
			REL::RelocationID(0, 38949), 0xF8
		};

		REL::Relocation<std::uintptr_t> target2{
			REL::RelocationID(0, 38953), 0x3D
		};

		originalFunc = trampoline.write_call<5>(
			target1.address(), thunk);

		trampoline.write_call<5>(
			target2.address(), thunk);
	}
	else {
		REL::Relocation<std::uintptr_t> target{
			REL::RelocationID(37999, 0), 0x33
		};

		originalFunc = trampoline.write_call<5>(
			target.address(), thunk);
	}

	logger::info("Installed AdmitAnimationHook");
}

void Hook::Install() {
	SKSE::AllocTrampoline(1 << 7);

	CharacterProxyProcessConstraints::Install();

	CameraLinearCastHook::Install(); 

	PlayerUpdate::Install();

	SneakHandlerProcessButton::Install();

	MenuTopicManagerHook::Install();

	ThirdPersonState_SetRotation::Install();

	CollisionFilterHooks::Install(); 

	MeleeHitHookSE::Install(); 

	MeleeHitHookAE::Install(); 

	AdmitAnimationHook::Install();
}
