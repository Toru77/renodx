#pragma once

#include <NRD.h>

#include <d3d12.h>
#include <wrl/client.h>

#include <array>
#include <cstdint>
#include <vector>

namespace falcom_world::rt12 {

using Microsoft::WRL::ComPtr;

struct Rt12NrdTexture {
  ComPtr<ID3D12Resource> resource;
  DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
  D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
  uint32_t width = 0;
  uint32_t height = 0;
  bool unordered_access_written = false;
};

struct Rt12NrdSlot {
  ComPtr<ID3D12CommandAllocator> allocator;
  ComPtr<ID3D12GraphicsCommandList> command_list;
  ComPtr<ID3D12DescriptorHeap> descriptors;
  ComPtr<ID3D12Resource> constants;
  ComPtr<ID3D12Resource> diffuse_readback;
  ComPtr<ID3D12Resource> specular_readback;
  std::vector<ComPtr<ID3D12Resource>> staging;
  std::array<Rt12NrdTexture, static_cast<uint32_t>(nrd::ResourceType::MAX_NUM)> resources;
  uint64_t fence_value = 0;
};

struct Rt12NrdResourcePools {
  std::vector<Rt12NrdTexture> permanent;
  std::vector<Rt12NrdTexture> transient;
  uint64_t bytes = 0;
};

struct Rt12NrdHistoryIdentity {
  uint64_t owner = 0;
  uint64_t generation = 0;
  uint64_t configuration = 0;
  uint64_t frame = 0;
  uint32_t width = 0;
  uint32_t height = 0;
  bool valid = false;
};

inline bool Rt12NrdCanContinueHistory(const Rt12NrdHistoryIdentity& previous,
                                      const Rt12NrdHistoryIdentity& current) {
  return previous.valid && current.valid && previous.owner == current.owner
         && previous.generation == current.generation && previous.configuration == current.configuration
         && previous.width == current.width && previous.height == current.height
         && previous.frame != UINT64_MAX && current.frame == previous.frame + 1u;
}

class Rt12NrdGeneration {
 public:
  Rt12NrdGeneration() = default;
  Rt12NrdGeneration(const Rt12NrdGeneration&) = delete;
  Rt12NrdGeneration& operator=(const Rt12NrdGeneration&) = delete;

  ~Rt12NrdGeneration() { Reset(); }

  bool CreateRelaxDiffuseSpecular() {
    Reset();
    const nrd::LibraryDesc* library = nrd::GetLibraryDesc();
    if (library == nullptr || library->versionMajor != NRD_VERSION_MAJOR || library->versionMinor != NRD_VERSION_MINOR
        || library->versionBuild != NRD_VERSION_BUILD || library->normalEncoding != nrd::NormalEncoding::R10_G10_B10_A2_UNORM
        || library->roughnessEncoding != nrd::RoughnessEncoding::LINEAR) {
      return false;
    }

    const nrd::DenoiserDesc denoiser = {1u, nrd::Denoiser::RELAX_DIFFUSE_SPECULAR};
    const nrd::InstanceCreationDesc creation = {{}, &denoiser, 1u};
    if (nrd::CreateInstance(creation, instance_) != nrd::Result::SUCCESS || instance_ == nullptr) {
      Reset();
      return false;
    }

    return true;
  }

  void Reset() {
    if (instance_ != nullptr) {
      nrd::DestroyInstance(*instance_);
      instance_ = nullptr;
    }
  }

  nrd::Instance* Instance() const { return instance_; }

 private:
  nrd::Instance* instance_ = nullptr;
};

}  // namespace falcom_world::rt12
