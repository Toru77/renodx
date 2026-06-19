# XeGTAO Dispatch: kai-vanillaplus vs sora-vanillaplus — Detailed Comparison

**Date:** 2026-06-19

---

## 1. Overview

| Dimension | kai-vanillaplus | sora-vanillaplus |
|-----------|----------------|------------------|
| Lines (XeGTAO-related) | ~3000+ | ~350 |
| Maturity | Production, battle-tested | Skeleton, initial integration |
| Dispatch model | Deferred (queue at lighting draw, execute at present) | Deferred (present-time only) |
| Compute pipeline creation | Real, from embedded bytecode | Real, same pattern |
| Scene CBV capture | Descriptor-table walk (`ResolveXeGTAOInputsFromCurrentBindings`) | **NOT IMPLEMENTED** — dispatch never runs because depth SRV is never captured |
| Descriptor tables | Pre-allocated, updated per-frame with `update_descriptor_tables` + `bind_descriptor_tables` | **NOT IMPLEMENTED** — uses raw `push_descriptors` per dispatch |
| State save/restore | Full `CommandListState` snapshot + restore around dispatch | Only unbinds compute pipeline after dispatch |
| Push constants | Separate 32-float array (`BuildXeGTAOPushConstants`), pushed via `push_constants` with layout param index | **NOT IMPLEMENTED** — GTAOConstants upload buffer created but never filled or pushed |
| t22 injection | Via `OnBeforeLightingShaderDraw` callback + push constant flags | Via `ViewBinding` on `CustomShader` (works at pipeline-layout level) |
| Fallback t22 | Returns neutral-white `composite_srv` when not ready, sets `xegtao_dedicated_bound=0` | Returns 1×1 white `fallback_srv` when not ready (recent fix) |
| Vanilla AO gating | `OnBeforeVanillaAoDraw` callback skips whole pass | Shader-level `if (xegtao_mode > 0.5f) r5.x = 0;` gate inside SSAO shader |
| Debug views | 21 debug modes via `xegtao_debug_mode` push constant | 1 debug mode (AO Only Gray) in lighting shader |
| Error handling | `fail()` lambda with detailed reason logging, per-frame dedup | Basic `if (debug_logging)` log lines |
| Startup guard | 16-frame dispatch guard + 64-frame require-current-CBV guard + 240-frame fallback quarantine | 8-frame dispatch guard |
| Resize guard | 4-frame dispatch guard + resource invalidation | 4-frame dispatch guard |
| Quality variants | Single main pipeline (quality + denoise in push constants) | 4 separate files (low/med/high/ultra) + 2 denoise files |

---

## 2. Critical Missing Pieces in sora-vanillaplus

### 2.1 Scene CBV Capture — **XeGTAO will never dispatch without this**

kai-vanillaplus `ResolveXeGTAOInputsFromCurrentBindings()` (line 4352):
- Walks the current command-list state's descriptor tables
- Finds the pixel-shader descriptor table with `register(b0, space0)` constant buffer
- Extracts the `buffer_range` for the scene CBV
- Also captures depth SRV from `kLightingDepthRegister` (t3) and SSAO SRV from `kLightingSsaoRegister` (t4)
- Validates handles with `IsViewAlive()` and `IsSceneCbvCandidateValid()`
- Has a fallback CBV system with multi-frame stability tracking

sora-vanillaplus `OnBeforeLightingShaderDraw()`:
```cpp
static bool OnBeforeLightingShaderDraw(reshade::api::command_list*) {
  if (shader_injection.xegtao_mode < 0.5f) shader_injection.xegtao_dedicated_bound = 0.f;
  return false;
}
```
This does NOTHING — it never captures `captured_depth_srv`, so `RunXeGTAO` always returns early with "No depth SRV — skip."

**Status:** ❌ XeGTAO always skips dispatch silently.

### 2.2 GTAOConstants Upload — **Dispatch would use garbage constants**

kai-vanillaplus:
- `BuildXeGTAOPushConstants()` builds a 32-float array with quality, radius, falloff, power, etc.
- These are pushed as push constants via `push_constants(stage, layout, kXeGtaoPushConstantsLayoutParam, 0, count, data)`
- The constants include runtime-computed values from `shader_injection` settings

sora-vanillaplus:
- A `consts_buffer` (288-byte upload heap) is created but NEVER filled with data
- `UpdateGTAOConstantsUpload()` exists but is NEVER called
- The compute shader expects `GTAOConstantBuffer` at `b0` — but it receives zero-filled garbage

**Status:** ❌ Even if dispatch ran, the shader would compute incorrect AO.

### 2.3 Descriptor Table Management — **Uses raw push_descriptors**

kai-vanillaplus:
- Pre-allocates `XeGTAODescriptorTableSet` (4 descriptor tables per pass)
- Calls `EnsureXeGTAODescriptorTables()` to create them once
- Each frame: `update_descriptor_tables()` + `bind_descriptor_tables()`
- After dispatch: `free_descriptor_table()` for cleanup
- This is the correct D3D11 pattern for descriptor binding

sora-vanillaplus:
- Uses `push_descriptors()` inline in `RunXeGTAO` — binds raw descriptors directly
- No pre-allocated descriptor tables, no `update_descriptor_tables`
- The `push_descriptors` path doesn't match the pipeline layout descriptor_table expectations
- The pipeline layout declares `descriptor_table` ranges, but we push raw descriptors

**Status:** ⚠️ Would likely fail or produce undefined behavior at dispatch time. The `push_descriptors` API is designed for push-descriptor layouts (Vulkan-style), not descriptor-table layouts (D3D11-style) that our pipeline layouts declare.

### 2.4 Command-List State Save/Restore — **Missing**

kai-vanillaplus `DispatchXeGTAOCompute()`:
- Saves full `CommandListState` via `renodx::utils::state::GetCurrentState(cmd_list)`
- After dispatch, restores compute bindings to their prior state
- Clears sampler bindings to avoid leaking into subsequent draws
- Has extensive fix-mode isolation levels (L0-L5)

sora-vanillaplus `OnPresent()`:
- Only calls `bind_pipeline(all_compute, {0u})` after dispatch
- Never saves/restores the game's command-list state
- Could leak UAVs, SRVs, or constant buffers into subsequent game draw calls

**Status:** ⚠️ Could cause rendering corruption after XeGTAO dispatch.

---

## 3. Architecture Comparison

### 3.1 Dispatch Flow

**kai-vanillaplus (simplified):**
```
OnBeforeLightingShaderDraw():
  ├─ Reset per-draw flags
  ├─ Capture current shader hash → verify kLightingShader
  ├─ ResolveXeGTAOInputsFromCurrentBindings() → capture depth SRV, SSAO SRV, scene CBV
  ├─ If deferred (fix1+): queue deferred dispatch data, return
  └─ If direct: RunXeGTAOForFrame(cmd_list, ...)

OnPresent():
  ├─ If deferred dispatch pending: RunXeGTAOForFrame(cmd_list, ...) with frozen snapshot
  └─ (Or dispatch was already done in OnBeforeLightingShaderDraw)
```

**sora-vanillaplus:**
```
OnBeforeLightingShaderDraw():
  └─ Only resets xegtao_dedicated_bound flag. Does NOT capture anything.

OnPresent():
  ├─ Check mode, startup guard, resize guard
  ├─ Create resources if needed
  ├─ Check captured_depth_srv.handle → ALWAYS 0 (never captured!) → return
  └─ (Never reaches RunXeGTAO)
```

### 3.2 Resource Lifetime

| Resource | kai-vanillaplus | sora-vanillaplus |
|----------|----------------|------------------|
| depth_mips_texture | Created in `EnsureXeGTAOResources`, format varies by precision setting | Created in `CreateXeGTAOResources`, R16_FLOAT |
| ao_term_a/b | Ping-pong, R8_UNORM | Ping-pong, R8_UNORM |
| edges | R8_UNORM | R8_UNORM |
| composite | R16G16B16A16_FLOAT (!) for HDR | R8G8B8A8_UNORM |
| fallback | `composite_srv` returned directly (permanent, created at swapchain init) | 1×1 white texture created at device init |
| descriptor tables | 4 tables per pass, pre-allocated, updated per-frame | None — uses push_descriptors |
| pipelines | Prefilter, Main, Denoise, NormalCap (4+) | Prefilter, Main×4, Denoise×2 (7) |

### 3.3 Push Constants vs Constant Buffer

**kai-vanillaplus:**
- Uses `push_constants` API to push a 32-float array at `kXeGtaoPushConstantsLayoutParam = 4`
- The shader reads these as root/push constants, NOT as a b0 constant buffer
- The scene CBV is bound separately as a descriptor (`captured_scene_cbv`)
- This means the shader uses TWO constant sources: push constants (settings) + CBV (scene)

**sora-vanillaplus:**
- Created a 288-byte `consts_buffer` (upload heap) to hold `GTAOConstants`
- The shader's `xegtao_common.hlsl` declares `cbuffer GTAOConstantBuffer : register(b0)`
- But the buffer is never filled, and the pipeline layout's b0 is a constant buffer descriptor (not push constants)
- This doesn't match — the pipeline layout declares b0 as a CBV, but we push raw descriptors

**Status:** ❌ Constant delivery mechanism is broken.

---

## 4. What sora-vanillaplus Does Better / Differently

| Feature | sora-vanillaplus advantage |
|---------|---------------------------|
| ViewBinding for t22 | Cleaner pipeline-layout integration — t22 is automatically injected without per-draw code |
| 1×1 white fallback | Simple and robust — never returns null, always valid |
| Quality variants | Separate pipeline per quality level (compile-time constants → better optimization) |
| Code simplicity | ~350 lines vs ~3000 — much easier to understand and modify |

---

## 5. Prioritized Fix List

### P0 — Must Fix (XeGTAO will never work without these)

1. **Implement scene CBV + depth SRV capture** in `OnBeforeLightingShaderDraw`
   - Walk descriptor tables like `ResolveXeGTAOInputsFromCurrentBindings` does
   - Capture t3 (depth), t4 (SSAO), and b0 (scene CBV)
   - Set `data->captured_depth_srv` and `data->captured_scene_cbv`

2. **Implement GTAOConstants upload** before dispatch
   - Call `UpdateGTAOConstantsUpload(cmd_list, data, projMatrix, rowMajor)` 
   - Need to extract projection matrix from captured scene CBV first (map the buffer, read at known offsets)

3. **Fix descriptor binding** — switch from `push_descriptors` to `update_descriptor_tables` + `bind_descriptor_tables`
   - Pre-allocate descriptor tables per pass (like kai's `XeGTAODescriptorTableSet`)
   - Update them each frame, bind them properly
   - The pipeline layouts are already declared as descriptor_table ranges

### P1 — Should Fix (stability)

4. **Add command-list state save/restore** around dispatch
   - Snapshot current state before dispatch, restore after
   - Clear compute bindings to avoid leaking

5. **Properly fill the constant buffer** at b0 with GTAOConstants from the scene projection matrix
   - Currently the shader expects b0 to have valid constants
   - Either switch to push_constants model (like kai) or properly upload to the CBV

### P2 — Nice to Have

6. Add `ResolveXeGTAOInputsFromCurrentBindings` for robust descriptor capture
7. Add fallback scene CBV system for startup resilience  
8. Add more debug views
9. Add IS-FAST noise support (kai has this integrated)

---

## 6. Root Cause of "Everything Dark" (Previously Reported)

The darkness was caused by the `ViewBinding::get_view` returning `resource_view{0u}` (null SRV) when XeGTAO was off. This has been fixed by returning the 1×1 white `fallback_srv` instead.

However, even with that fix, XeGTAO **still doesn't produce any AO** because:
1. `captured_depth_srv` is never set → `RunXeGTAO` returns early
2. Even if it ran, constants are zero-filled → shader would produce incorrect output
3. Descriptor binding uses wrong API pattern → dispatch would likely fail

The fallback fix only ensures the lighting shader doesn't crash/black-out — it doesn't enable XeGTAO to actually run.

---

*Generated by comparing `kai-vanillaplus/addon.cpp` (XeGTAO backend) against `sora-vanillaplus/addon.cpp` (XeGTAO backend).*
