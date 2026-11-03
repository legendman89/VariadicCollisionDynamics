#pragma once 

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>
#include <string>
#include <vector>

#include "logger.hpp"

namespace CollisionLayers
{
    inline constexpr std::size_t kCollisionLayerCount = 64;

    //default values captured at k new game of the collision layer filter for the entire game. 
    inline constexpr std::array<std::uint64_t, kCollisionLayerCount> kVanillaCollisionMasks = {
        0x7FD95FFFF7FFULL,       // 0  Unidentified
        0x17FCD5E1B67FFULL,      // 1  Static
        0x17FED5E1B6FFFULL,      // 2  AnimStatic
        0x5D8D5C13673FULL,       // 3  Transparent
        0x13B435E1B7EFFULL,      // 4  Clutter
        0x53B43561B7EFFULL,      // 5  Weapon
        0x41B435D927E77ULL,      // 6  Projectile
        0xC5B437D927F37ULL,      // 7  Spell
        0x407BC01C037A8FULL,     // 8  Biped
        0x7FCD5E1B67FFULL,       // 9  Trees
        0x17B435E1B6EFFULL,      // 10 Props
        0x1C51400005F4ULL,       // 11 Water
        0x1841481041F1ULL,       // 12 Trigger
        0x17FC55E1B67FFULL,      // 13 Terrain
        0x7BC95E1B77FFULL,       // 14 Trap
        0x414008000001ULL,       // 15 NonCollidable
        0x7B895C03673FULL,       // 16 CloudTrap
        0x17F815E1B67FFULL,      // 17 Ground
        0x415808200001ULL,       // 18 Portal
        0x13849023A6637ULL,      // 19 DebrisSmall
        0x17DDD5E3A76FFULL,      // 20 DebrisLarge
        0x41584ADC0001ULL,       // 21 AcousticSpace
        0x184058200001ULL,       // 22 ActorZone
        0x1840082000C1ULL,       // 23 ProjectileZone
        0x3840480000C1ULL,       // 24 GasTrap
        0x3859023A6637ULL,       // 25 ShellCasting
        0x5D8D5C1367FFULL,       // 26 TransparentSmall
        0x3D415DF7F7DFULL,       // 27 InvisibleWall
        0x5DAD5C5367FFULL,       // 28 TransparentSmallAnim
        0x4000000080ULL,         // 29 Ward
        0x5079CADD737EFFULL,     // 30 CharController
        0x1000440000000ULL,      // 31 StairHelper
        0x10BB411E1B7EFFULL,     // 32 DeadBip
        0x40400004F0ULL,         // 33 BipedNoCC
        0x9410220EULL,           // 34 AvoidBox
        0x5C40563D420FULL,       // 35 CollisionBox
        0x410002340801ULL,       // 36 CameraSphere
        0x10000004ULL,           // 37 DoorDetection
        0xC7F4B6BFCFFF7ULL,      // 38 ConeProjectile
        0x1B005413630FULL,       // 39 CameraPick
        0x5BD15C37E7FFULL,       // 40 ItemPick
        0x201BC1000367F7ULL,     // 41 LineOfSight
        0x1C481C122A0FULL,       // 42 PathPick
        0x5FC95FDB7FFFULL,       // 43 CustomPick1
        0x5FC95FDB7FFFULL,       // 44 CustomPick2
        0x400414B1B6737ULL,      // 45 SpellExplosion
        0x59585437E78FULL,       // 46 DroppingPick
        0x100000000ULL,          // 47 Unused1
        0x801A2436ULL,           // 48 Unused2
        0x0ULL,                  // 49 Unused3
        0x2040000000E0ULL,       // 50 Unused4
        0x4000000080ULL,         // 51 Unused5
        0x140000000ULL,          // 52 Unused6
        0x20000000000ULL,        // 53 Unused7
        0x40000100ULL,           // 54 Invalid
        0xFFFFFFFFFFFFFFFFULL,   // 55 Unknown
        0xFFFFFFFFFFFFFFFFULL,   // 56 Unknown
        0xFFFFFFFFFFFFFFFFULL,   // 57 Unknown
        0xFFFFFFFFFFFFFFFFULL,   // 58 Unknown
        0xFFFFFFFFFFFFFFFFULL,   // 59 Unknown
        0xFFFFFFFFFFFFFFFFULL,   // 60 Unknown
        0xFFFFFFFFFFFFFFFFULL,   // 61 Unknown
        0xFFFFFFFFFFFFFFFFULL,   // 62 Unknown
        0xFFFFFFFFFFFFFFFFULL    // 63 Unknown
    };


    inline constexpr std::array<std::string_view, 47> kCollisionLayerNames{
    "Unidentified",
    "Static",
    "AnimStatic",
    "Transparent",
    "Clutter",
    "Weapon",
    "Projectile",
    "Spell",
    "Biped",
    "Trees",
    "Props",
    "Water",
    "Trigger",
    "Terrain",
    "Trap",
    "NonCollidable",
    "CloudTrap",
    "Ground",
    "Portal",
    "DebrisSmall",
    "DebrisLarge",
    "AcousticSpace",
    "ActorZone",
    "ProjectileZone",
    "GasTrap",
    "ShellCasting",
    "TransparentSmall",
    "InvisibleWall",
    "TransparentSmallAnim",
    "Ward",
    "CharController",
    "StairHelper",
    "DeadBip",
    "BipedNoCC",
    "AvoidBox",
    "CollisionBox",
    "CameraSphere",
    "DoorDetection",
    "ConeProjectile",
    "CameraPick",
    "ItemPick",
    "LineOfSight",
    "PathPick",
    "CustomPick1",
    "CustomPick2",
    "SpellExplosion",
    "DroppingPick"
    };

    inline bool IsLayerOverridden(
        const std::vector<std::string>& a_overrides,
        const std::string_view a_layer)
    {
        return std::find(a_overrides.begin(), a_overrides.end(), a_layer) != a_overrides.end();
    }

    // we only seraialize the difference in layers from vanilla
    inline bool GetLayerEnabled(
        const std::size_t a_index,
        const std::vector<std::string>& a_overrides)
    {
        if (a_index >= kCollisionLayerNames.size()) {
            return false;
        }
        constexpr auto charControllerIndex =
            std::to_underlying(RE::COL_LAYER::kCharController);
        const auto vanillaMask = kVanillaCollisionMasks[charControllerIndex];
        const auto bit = std::uint64_t{ 1 } << a_index;
        const bool vanillaEnabled = (vanillaMask & bit) != 0;
        return IsLayerOverridden(a_overrides, kCollisionLayerNames[a_index])
            ? !vanillaEnabled
            : vanillaEnabled;
    }

};

