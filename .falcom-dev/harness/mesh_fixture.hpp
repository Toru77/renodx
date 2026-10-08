#pragma once
// Test geometry for the pool harness: box meshes written into mock VB/IB
// bytes and read by the pool's two-phase mesh copies through a registered
// position-only input layout (float3 POSITION at offset 0, stride 12).
#include <array>
#include <cstring>
#include <vector>
#include "src/utils/scene.hpp"

namespace fixture {

inline constexpr uint64_t kLayout = 0x5150u;
inline constexpr uint32_t kStride = 12u;
inline constexpr uint32_t kBoxIndices = 36u;
inline constexpr uint32_t kBoxCopies = 32u;  // the IB repeats the box so first_index k*36 draws it

struct TestMesh {
  std::vector<std::array<float, 3>> positions;
  std::vector<std::array<uint32_t, 3>> triangles;
};

inline TestMesh Box(float sx) {
  TestMesh m;
  for (int i = 0; i < 8; ++i) m.positions.push_back({(i & 1) ? sx : -sx, (i & 2) ? 1.f : -1.f, (i & 4) ? 1.f : -1.f});
  const uint32_t t[12][3] = {{0,1,2},{1,3,2},{4,6,5},{5,6,7},{0,2,4},{2,6,4},{1,5,3},{3,5,7},{0,4,1},{1,4,5},{2,3,6},{3,7,6}};
  for (auto& tri : t) m.triangles.push_back({tri[0], tri[1], tri[2]});
  return m;
}

inline void RegisterLayout() {
  renodx::utils::scene::InputLayoutInfo info;
  renodx::utils::scene::InputElementCopy position;
  position.semantic = "POSITION";
  position.format = reshade::api::format::r32g32b32_float;
  position.stride = kStride;
  info.elements.push_back(position);
  renodx::utils::scene::shared.data->input_layouts[kLayout] = info;
}

inline void WriteVertices(std::vector<uint8_t>& bytes, const TestMesh& mesh) {
  for (size_t i = 0; i < mesh.positions.size(); ++i) {
    std::memcpy(bytes.data() + i * kStride, mesh.positions[i].data(), kStride);
  }
}

inline void WriteIndices(std::vector<uint8_t>& bytes) {
  const TestMesh box = Box(1.f);
  size_t offset = 0;
  for (uint32_t copy = 0; copy < kBoxCopies; ++copy) {
    for (const auto& tri : box.triangles) {
      for (const uint32_t index : tri) {
        const uint16_t value = static_cast<uint16_t>(index);
        std::memcpy(bytes.data() + offset, &value, sizeof(value));
        offset += sizeof(value);
      }
    }
  }
}

}  // namespace fixture
