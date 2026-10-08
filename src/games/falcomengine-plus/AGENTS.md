# AGENTS: falcomengine-plus (Falcom Engine+)

Scope: `src/games/falcomengine-plus/`. Long-lived graphics project: ray tracing for Trails in the Sky 2nd Chapter (D3D11) on RenoDX. Correctness, maintainability and compatibility come before quick local fixes.

**Before any work on `world/` (BVH, ray tracing, pool, deforming/moving meshes), read `world/docs/HANDOFF.md`.** It holds the current state, what is deployed and awaiting in-game validation, the next steps, the invariants and the test harness. `world/docs/ROADMAP.md` is the full round-by-round history with the evidence behind each decision.

## Standing instructions from the project owner

- "No mistakes. No regressions." Work step by step: one milestone at a time, validated in game before the next.
- Stay inside `falcomengine-plus/`. Do not modify the generic RenoDX framework (`src/utils`, `src/mods`, `external`, CMake) unless the capability is genuinely framework-wide; then first state why it cannot be local, who else benefits, what could regress and which local alternatives were considered. Dependency direction: RenoDX framework -> Falcom Engine+ integration -> Falcom-specific systems.
- Shader hashes identify stable, broadly reused renderer behavior (lighting, post, stable passes). Do not use them to special-case individual meshes/materials/variants; generalize behavior instead (the classifier in `world/contract/` recognizes shaders from bytecode).

## How to work here

1. Understand before modifying: trace the data flow, authoritative sources, existing helpers, shared resources and consumers, interaction with earlier milestones.
2. Diagnose before patching: find the failing layer (capture -> classification/admission -> CPU representation -> GPU upload -> descriptors -> shader interpretation -> BVH build -> traversal -> debug presentation -> consumer) with the cheapest decisive evidence (logs, disassembly, checksums, readbacks, counters, A/B). No speculative multi-change patches.
3. Evidence over visual guesswork. When explanations compete, design the test that separates them first. Discovery rounds (diagnostic only, no behavior change) are normal here before a feature round.
4. One authoritative implementation (GPU structs/layouts, transforms, resource/view creation, descriptor layouts, admission rules). No duplicated logic that can drift; CPU/GPU mirrors are transcribed and tested against each other.
5. Protect invariants: CPU/GPU struct sizes, register order vs descriptor tables, SRV vs UAV, matrix conventions, draw-time snapshots, validated data. When shared logic changes, update every consumer.
6. Minimal regression surface; no unrelated cleanup, optimization or speculative features. Document unrelated problems and defer them.
7. Diagnostics are part of the implementation: counts, sizes, offsets, strides, HRESULTs, reason codes, checksums; never a bare "failed". Panel lines + JSON dumps (`world_pool.json`, `world_deform.json`).
8. Treat validated subsystems as foundations; when changing them, list preserved invariants, changed behavior and required re-tests.

## Hard rules learned the hard way (details in HANDOFF.md)

- Lock rule: never make a graphics/ReShade API call while holding a module mutex (D3D11 deferred destruction fires destroy events that relock; MSVC `std::mutex` throws on same-thread relock -> CRT fast fail). Reserve under the lock, record after it.
- ReShade quirks: `update_buffer_region` at offset 0 writes the whole buffer (use `WriteBufferRange`); `get_resource_desc` reports buffer stride 0; D3D11 `create_pipeline` rejects stream-output state (create the SO geometry shader natively); ReShade 6.8 (API 20) changed `get_query_heap_results` (use native D3D11 queries, see `DeformBackend`).
- Files in this addon are CRLF. Shaders under `world/shaders/*.hlsl` are picked up by the CMake glob automatically (CONFIGURE_DEPENDS). Never put `.cso` files under this folder: the addon CMake glob embeds every `*.cso` it finds.
- Verification is done by the owner in game (DevKit build); give exact test steps and what to send back (panel lines, dumps).
