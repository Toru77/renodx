#pragma once

// Data-only manual/reference metadata for the tagged VS dumps.
//
// HINTS ONLY. Nothing here may accept or reject a candidate, classify a
// family, gate coverage, exclude a shader, or validate a transform. Runtime
// evidence always wins. The table is used to prefill census labels (marked as
// hint-sourced) and to improve UI readability.
//
// A VS hash absent from this table must remain fully discoverable by the
// generic runtime path.

#include "../world_state.hpp"

namespace falcom_world {

struct ReferenceHint {
  uint32_t vs_hash = 0u;
  const char* tag = "";
  const char* category = "";
  const char* pattern = "";
  const char* file = "";
  FamilyLabel suggested_label = FamilyLabel::Unknown;
  // Structured instance hypothesis (manual evidence; runtime facts win).
  bool structured_instance = false;
  uint8_t instance_slot = 15u;
  uint32_t world_offset = 0u;
  uint32_t prev_offset = 48u;
};

inline constexpr ReferenceHint kReferenceHints[] = {
    {0xC4012709u, "ground", "Static opaque", "instances t15; stride 160; world +0; prev +48; float4x3 row", "reference/.dumps/ground_0xC4012709.vs_5_0.hlsl", FamilyLabel::Terrain, true, 15u, 0u, 48u},
    {0x0FA3C0F4u, "wall", "Static opaque", "instances t15; stride 160; world +0; prev +48; float4x3 row", "reference/.dumps/wall_0x0FA3C0F4.vs_5_0.hlsl", FamilyLabel::Buildings, true, 15u, 0u, 48u},
    {0x71D2C562u, "buildings", "Static opaque", "instances t15; stride 160; world +0; prev +48; float4x3 row", "reference/.dumps/buildings_0x71D2C562.vs_5_0.hlsl", FamilyLabel::Buildings, true, 15u, 0u, 48u},
    {0xE73AC882u, "props", "Static opaque", "instances t15; stride 160; world +0; prev +48; float4x3 row", "reference/.dumps/props_0xE73AC882.vs_5_0.hlsl", FamilyLabel::Props, true, 15u, 0u, 48u},
    {0x7157A326u, "distantmountain", "Static opaque", "instances t15; stride 160; world +0; prev +48; float4x3 row", "reference/.dumps/distantmountain_0x7157A326.vs_5_0.hlsl", FamilyLabel::Terrain, true, 15u, 0u, 48u},
    {0x5817BC23u, "glass_windows", "Glass", "instances t15; stride 160; world +0; prev +48; float4x3 row", "reference/.dumps/glass_windows_0x5817BC23.vs_5_0.hlsl", FamilyLabel::Glass, true, 15u, 0u, 48u},
    {0xC2CA78F6u, "staticfoliage", "Foliage", "instances t15; stride 160; world +0; prev +48; float4x3 row", "reference/.dumps/staticfoliage_0xC2CA78F6.vs_5_0.hlsl", FamilyLabel::Foliage, true, 15u, 0u, 48u},
    {0x9FF8E4BEu, "movingfoliage", "Foliage", "instances t15; stride 160; world +0; prev +48; float4x3 row", "reference/.dumps/movingfoliage_0x9FF8E4BE.vs_5_0.hlsl", FamilyLabel::Foliage, true, 15u, 0u, 48u},
    {0x51654E2Fu, "skin", "Character", "bones t0 + instances t15; boneAddress; blend weights", "reference/.dumps/skin_0x51654E2F.vs_5_0.hlsl", FamilyLabel::Character, false, 15u, 0u, 48u},
    {0x4A037EB5u, "face", "Character", "bones t0 + instances t15; boneAddress; blend weights", "reference/.dumps/face_0x4A037EB5.vs_5_0.hlsl", FamilyLabel::Character, false, 15u, 0u, 48u},
    {0xCE741669u, "hair", "Character", "bones t0 + instances t15; boneAddress; blend weights", "reference/.dumps/hair_0xCE741669.vs_5_0.hlsl", FamilyLabel::Character, false, 15u, 0u, 48u},
    {0xCCB5FF14u, "eye", "Character", "bones t0 + instances t15; boneAddress; blend weights", "reference/.dumps/eye_0xCCB5FF14.vs_5_0.hlsl", FamilyLabel::Character, false, 15u, 0u, 48u},
    {0x61F48AC7u, "clothes", "Character", "bones t0 + instances t15; boneAddress; blend weights", "reference/.dumps/clothes_0x61F48AC7.vs_5_0.hlsl", FamilyLabel::Character, false, 15u, 0u, 48u},
    {0x37E8915Au, "accesories", "Character", "bones t0 + instances t15; boneAddress; blend weights", "reference/.dumps/accesories_0x37E8915A.vs_5_0.hlsl", FamilyLabel::Character, false, 15u, 0u, 48u},
};

inline const ReferenceHint* FindReferenceHint(uint32_t vs_hash) {
  for (const auto& hint : kReferenceHints) {
    if (hint.vs_hash == vs_hash) return &hint;
  }
  return nullptr;
}

}  // namespace falcom_world
