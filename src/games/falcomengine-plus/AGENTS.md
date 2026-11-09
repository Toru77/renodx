# AGENTS: falcomengine-plus (Falcom Engine+)

Scope: `src/games/falcomengine-plus/`. Long-lived graphics project: ray tracing for Trails in the Sky 2nd Chapter (D3D11) on RenoDX. Correctness, maintainability and compatibility come before quick local fixes.

**Start each task by reading `README.md`.** Use its technique index and shared-resource map to find the authoritative source files and focused documentation.

Before work on `world/` (BVH, ray tracing, pool, deforming/moving meshes), read the relevant "Open problems", "Current state", and subsystem sections in `world/docs/ROADMAP.md`. It is the world-ray-tracing handoff; `world/docs/HISTORY.md` preserves past decisions and their evidence.

README maintenance rules:

- Every technique or feature description in `README.md` must be at most three sentences. Put implementation paths, resource inputs, and research/upstream/license links in separate link lists or table fields.
- When implementing a feature or technique, update the README in the same change with its purpose, technique, implementation path, important inputs, and relevant resource provenance. Update its entry when code or resource ownership moves, and remove obsolete entries when a feature is removed.
- Keep the README as a concise index. Do not duplicate settings details, test procedures, design documents, history, or changing validation/status claims; link to the authoritative source instead.
- Keep entries current by checking their source paths, resource bindings, and attribution links while making related changes. State only resource origins and technique behavior verified in the repository or an adjacent authoritative reference.

## Standing instructions from the project owner

- "No mistakes. No regressions." Work step by step: one milestone at a time, validated in game before the next.
- Stay inside `falcomengine-plus/`. Do not modify the generic RenoDX framework (`src/utils`, `src/mods`, `external`, CMake).
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

## Hard rules learned the hard way (details in ROADMAP.md, Reference)

- Lock rule: never make a graphics/ReShade API call while holding a module mutex (D3D11 deferred destruction fires destroy events that relock; MSVC `std::mutex` throws on same-thread relock -> CRT fast fail). Reserve under the lock, record after it.
- ReShade quirks: `update_buffer_region` at offset 0 writes the whole buffer (use `WriteBufferRange`); `get_resource_desc` reports buffer stride 0; D3D11 `create_pipeline` rejects stream-output state (create the SO geometry shader natively); ReShade 6.8 (API 20) changed `get_query_heap_results` (use native D3D11 queries, see `DeformBackend`).
- Files in this addon are CRLF. Shaders under `world/shaders/*.hlsl` are picked up by the CMake glob automatically (CONFIGURE_DEPENDS). Never put `.cso` files under this folder: the addon CMake glob embeds every `*.cso` it finds.
- Verification is done by the owner in game (DevKit build); give exact test steps and what to send back (panel lines, dumps).
