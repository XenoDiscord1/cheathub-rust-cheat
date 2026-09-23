#include "etc.h"

#pragma once

// ============================================
// OFFSET CONFIGURATION (Updated from IL2CPP)
// ============================================
namespace GameOffsets {
	constexpr uintptr_t SendClientTick = 0x30EAC0;
	constexpr uintptr_t CreateProjectile = 0x2E9450;
	constexpr uintptr_t GetSkinColor = 0xAC50C0;
	constexpr uintptr_t SetSkinColor = 0x73DF60;
	constexpr uintptr_t ClientInput = 0x3A52A0;
	constexpr uintptr_t GetModifiedAimConeDirection = 0x984350;
	constexpr uintptr_t HandleRunning = 0x896340;
	
	// Memory structure offsets
	namespace BasePlayer {
		constexpr uintptr_t _modelState = 0x4C8;
		constexpr uintptr_t _input = 0x500;
		constexpr uintptr_t _clientTickRate = 0x5C8;
		constexpr uintptr_t _flags = 0x220;
	}
	
	namespace Projectile {
		constexpr uintptr_t _ricochet = 0x114;
		constexpr uintptr_t _damageScale = 0x2C;
		constexpr uintptr_t _radius = 0x30;
	}
	
	namespace ModelState {
		constexpr uintptr_t _eyesRotation = 0x18;
		constexpr uintptr_t _bodyRotation = 0x20;
	}
}

// ============================================
// FUNCTION POINTER TYPEDEFS
// ============================================

typedef void(__fastcall* clientinput_fn)(DWORD64, DWORD64);
inline clientinput_fn original_clientinput;

typedef bool(__fastcall* sendclienttick)(void*);
inline sendclienttick original_sendclienttick;

typedef uintptr_t(__fastcall* create_projectile_fn)(void*, void*, Vector3, Vector3, Vector3);
inline create_projectile_fn original_create_projectile{ };

typedef Vector3(__fastcall* modifiedaimconedirection)(float, Vector3, bool);
inline modifiedaimconedirection original_aimconedirection;

typedef void(__fastcall* HandleRunning_fn)(void*, void*, bool);
inline HandleRunning_fn original_handleRunning{};

inline bool waslagging = false;
__declspec(selectany) uintptr_t TargetSilentPlayer = NULL;

// ============================================
// HOOK IMPLEMENTATIONS
// ============================================

inline void __fastcall ClientInput(DWORD64 baseplayah, DWORD64 ModelState) {
	if (!waslagging && features::aimbot::AimbotActive) {
		safe_write(LxcalPlayer.lclPlayer + GameOffsets::BasePlayer::_clientTickRate, 0.4f, float);
		waslagging = true;
	}
	else if (waslagging && !features::aimbot::AimbotActive) {
		safe_write(LxcalPlayer.lclPlayer + GameOffsets::BasePlayer::_clientTickRate, 0.05f, float);
		waslagging = false;
	}

	typedef void(__stdcall* ClientInput)(DWORD64, DWORD64);
	((ClientInput)original_clientinput)(baseplayah, ModelState);
	if (features::exploits::FakeLag)
		LxcalPlayer.lclPlayer->AddFlag(32);
	if (features::exploits::FakeLag)
		LxcalPlayer.lclPlayer->RemoveFlag(4);
}

inline uintptr_t __fastcall CreateProjectile(void* BaseProjectile, void* prefab_pathptr, Vector3 pos, Vector3 forward, Vector3 velocity) {
	uintptr_t projectile = original_create_projectile(BaseProjectile, prefab_pathptr, pos, forward, velocity);
	auto* TargetPlayer = reinterpret_cast<LocalPlayer*>(features::closestPlayer);
	safe_write(projectile + GameOffsets::Projectile::_ricochet, true, bool);
	if (features::exploits::FatBoolet) {
		safe_write(projectile + GameOffsets::Projectile::_damageScale, 1.f, float);
	}
	else {
		safe_write(projectile + GameOffsets::Projectile::_damageScale, 0.1f, float);
	}
	return projectile;
}

typedef Vector4(__fastcall* GetSkinColor)(void*);
inline GetSkinColor Orig_GetSkinColor{ };
inline Vector4 __fastcall Hook_GetSkinColor(void* ecx) {
	if (features::exploits::SkinChanger)
		return Vector4(5.988f, 0.f, 5.909f, 1.f);
	return Orig_GetSkinColor(ecx);
}

typedef Vector4(__fastcall* SetSkinColor)(void*, float);
inline SetSkinColor Orig_SetSkinColor{ };
inline Vector4 __fastcall Hook_SetSkinColor(void* ecx, float skinNumber) {
	if (features::exploits::SkinChanger)
		return Vector4(5.988f, 0.f, 5.909f, 1.f);
	return Orig_SetSkinColor(ecx, skinNumber);
}

inline Vector3 __fastcall GetModifiedAimConeDirection(float aimCone, Vector3 inputVec, bool anywhereInside = true) {
	auto* TargetPlayer = reinterpret_cast<LocalPlayer*>(features::closestPlayer);
	Vector3 Prediction(const Vector3 & LP_Pos, LocalPlayer * Player, BoneList Bone);
	Vector3 dir = (Prediction(LxcalPlayer.lclPlayer->GetBoneByID(head), TargetPlayer, head) - LxcalPlayer.lclPlayer->GetBoneByID(head)).Normalized();
	if (features::exploits::pSilent && features::closestPlayer != NULL) {
		inputVec = dir;
	}
	if (features::exploits::AntiSpread) {
		aimCone = 0.f;
	}
	return original_aimconedirection(aimCone, inputVec, anywhereInside);
}

inline void __fastcall HandleRunning(void* a1, void* a2, bool wantsRun) {
	wantsRun = GetAsyncKeyState(0x10) && !GetAsyncKeyState(0x41) && !GetAsyncKeyState(0x53) && !GetAsyncKeyState(0x44);
	if (features::exploits::SprintAnyware)
		wantsRun = true;
	return original_handleRunning(a1, a2, wantsRun);
}

inline bool __fastcall SendClientTick(void* baseplayer) {
	if (features::exploits::AntiAim) {
		auto input = unsafe_read(baseplayer + GameOffsets::BasePlayer::_modelState, uintptr_t);
		if (!input) {
			return original_sendclienttick(baseplayer);
		}
		auto state = unsafe_read(input + 0x20, uintptr_t);
		if (!state) {
			return original_sendclienttick(baseplayer);
		}
		auto current = unsafe_read(state + 0x10, uintptr_t);
		if (!current) {
			return original_sendclienttick(baseplayer);
		}
		writenew<Vector3>(current + GameOffsets::ModelState::_eyesRotation, Vector3(100, rand() % 999 + -999, rand() % 999 + -999));
	}
	return original_sendclienttick(baseplayer);
}

inline void InjectFunction(void* Function, void** Original, void* Detour, bool autoEnable = true) {
	if (MH_Initialize() != MH_OK && MH_Initialize() != MH_ERROR_ALREADY_INITIALIZED) {
		printf("Failed to initialize MinHook\n");
		return;
	}
	MH_CreateHook(Function, Detour, Original);
	if (autoEnable)
		MH_EnableHook(Function);
}

inline void InitHookLol() {
	if (MH_Initialize() != MH_OK && MH_Initialize() != MH_ERROR_ALREADY_INITIALIZED) {
		printf("MinHook initialization failed\n");
		return;
	}

	uintptr_t gameAssemblyBase = GetModBase(L"GameAssembly.dll");
	printf("GameAssembly.dll base: 0x%llx\n", gameAssemblyBase);

	// NEW OFFSETS FROM IL2CPP DUMP
	InjectFunction((void*)(gameAssemblyBase + GameOffsets::SendClientTick), (void**)&original_sendclienttick, SendClientTick);
	printf("✓ SendClientTick hooked @ 0x%llx\n", gameAssemblyBase + GameOffsets::SendClientTick);
	
	InjectFunction((void*)(gameAssemblyBase + GameOffsets::CreateProjectile), (void**)&original_create_projectile, CreateProjectile);
	printf("✓ CreateProjectile hooked @ 0x%llx\n", gameAssemblyBase + GameOffsets::CreateProjectile);
	
	InjectFunction((void*)(gameAssemblyBase + GameOffsets::GetSkinColor), (void**)&Orig_GetSkinColor, Hook_GetSkinColor);
	printf("✓ GetSkinColor hooked @ 0x%llx\n", gameAssemblyBase + GameOffsets::GetSkinColor);
	
	InjectFunction((void*)(gameAssemblyBase + GameOffsets::SetSkinColor), (void**)&Orig_SetSkinColor, Hook_SetSkinColor);
	printf("✓ SetSkinColor hooked @ 0x%llx\n", gameAssemblyBase + GameOffsets::SetSkinColor);
	
	InjectFunction((void*)(gameAssemblyBase + GameOffsets::ClientInput), (void**)&original_clientinput, ClientInput);
	printf("✓ ClientInput hooked @ 0x%llx\n", gameAssemblyBase + GameOffsets::ClientInput);
	
	InjectFunction((void*)(gameAssemblyBase + GameOffsets::GetModifiedAimConeDirection), (void**)&original_aimconedirection, GetModifiedAimConeDirection);
	printf("✓ GetModifiedAimConeDirection hooked @ 0x%llx\n", gameAssemblyBase + GameOffsets::GetModifiedAimConeDirection);
	
	InjectFunction((void*)(gameAssemblyBase + GameOffsets::HandleRunning), (void**)&original_handleRunning, HandleRunning);
	printf("✓ HandleRunning hooked @ 0x%llx\n", gameAssemblyBase + GameOffsets::HandleRunning);
	
	printf("\n=== ALL HOOKS SUCCESSFULLY INSTALLED ===\n");
}
