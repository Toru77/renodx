# AGENTS: RenoDX repository guidance

This file is the root, always-on guidance for AI coding agents. Keep it short. Put folder-specific rules in nested `AGENTS.md` files only when the subtree needs durable local rules.

## How AGENTS.md is layered

- Root `AGENTS.md` is the shared baseline for the repository.
- Nested `AGENTS.md` files apply to files under their directory; the closest/narrowest file should own conflicts.
- User chat instructions override repository instructions.
- Keep common policy here instead of repeating it in every nested file.
- Use nested files for folder-specific deltas such as shader naming, shared utility API constraints, skill-authoring rules, or per-game exceptions.
- Use workflow skills in `.agents/skills/` for task procedures; do not expand always-on `AGENTS.md` files with long workflow playbooks.

## Scoped guidance

- `.agents/skills/AGENTS.md` - authoring and maintenance rules for project workflow skills.
- `src/games/AGENTS.md` - game mod folders, shader naming, addon targets, and manual verification.
- `src/mods/AGENTS.md` - shared mod feature modules used by game-specific addons.
- `src/shaders/AGENTS.md` - shared HLSL shader library files.
- `src/utils/AGENTS.md` - shared C++ utility infrastructure and ReShade wrappers.

## Agent role guides

Role-specific workflows are in `.agents/agents/`. Read the matching guide when a task is explicitly planning, implementation from an existing plan, or review against a plan:

- `.agents/agents/low-planner.md`, `mid-planner.md`, `high-planner.md` - planning and design decisions at three model tiers. All three share the role definition in `.agents/agents/planner.md`.
- `.agents/agents/implementer-writer.md` - implementing a settled plan.
- `.agents/agents/reviewer.md` - reviewing implementation against a plan.

These guides complement this file and any nested `AGENTS.md`; they do not replace repository guidance.

Nested `AGENTS.md` files require `chat.useNestedAgentsMdFiles` to be enabled in `.vscode/settings.json` for VS Code to load them automatically.

## Agent loop policy

The lead agent owns the loop; agents never call each other or start a second cycle themselves.

- Plan once per feature set, in batches. Hand-offs name the plan file path and section, never pasted plan text.
- Review one milestone commit range at a time (`git diff <base>..<commit>`). Never re-review a range already reported on. The reviewer gets one pass per launch.
- Cap reviewer launches at two per milestone. If the second pass still finds deviations, stop and bring the decision to the user.
- Batch open questions into one amendment request and route to a higher planner tier (low → mid → high) once instead of re-planning repeatedly. Keep one tier per plan.
- A hand-off is a routing label, not an auto-retry: `NO DEVIATIONS` / `FIXED` end the milestone; `PLAN DEVIATION` / `DECISION NEEDED` go to the lead for a decision.
- Run independent milestones in parallel when their file lists do not overlap.
- Grep first and read only cited line ranges. Rerun only checks affected by a fix.

## Workflow skills

Project workflow skills live under `.agents/skills/<name>/SKILL.md`. When a request matches one of these domains, read the matching skill before planning or editing:

- `setup-game-mod-dev` - starting or resuming game mod development, DevKit setup, build targets, links, and baseline verification.
- `swapchain-resource-analysis` - swapchain/output tracing, resource formats, `SwapChainPass`, proxy resources, SDR/HDR output state, and rejecting final-SDR inverse-tonemap approaches.
- `handle-sdr-tonemap-lut` - shader-side tonemap, hard clip, LUT/color-grade, RenoDRT, PsychoV, and HDR bridge work after target signals are proven.
- `analysis-graphing` - dark-theme plots, curve comparisons, hue sweeps, LUT statistics, and other analysis figures.
- `hdr-test-pattern-generation` - deterministic SDR/HDR ramps, charts, sweeps, color bars, and synthetic validation images.
- `bt2020-png-generation` - BT.2020/BT.2100 PQ PNG output, cICP/ICC signaling, and HDR PNG validation.

For human discoverability, `.agents/skills/README.md` summarizes the skills and their handoff boundaries.

## Global rules

- Apply Don't Repeat Yourself logic (DRY) as much as possible.
- Don't create useless single-use functions.
- Keep changes small and scoped to the folder or subsystem requested.
- Before planning, reviewing, creating, or editing RenoDX-authored C++ under
    `src/`, read `docs/CXX_STYLE.md` in full and follow it. A reference to this
    guide in the prompt is not a substitute for loading its contents.

- Do not change global CMake presets, CI workflows, vendored dependencies, or external submodules without explicit approval.
- Prefer existing project abstractions over new framework-style layers.
- For C++ and HLSL, do not extract single-use helpers for local control flow, one-off switches, simple casts, basic field access, or short condition checks.
- Only add helper functions when they are reused, enforce an invariant, name a stable domain concept, hide real complexity, or form a deliberate API boundary.
- Do not introduce named single-use locals that are only passed to the next call or immediately returned; construct the value at the call/return site unless the local is mutated, reused, or materially clarifies the code.
- For C++ parameters, use pointers for modifiable/out values and `const&` for non-modified inputs that should not be copied; avoid non-const references except established callback/API signatures.
- Validate changes with the smallest relevant build target or manual verification path.

## Graphify scope

- For work on a game under `src/games/<game>`, treat that game directory as the Graphify project root. If its `graphify-out/graph.json` is missing, first run `graphify extract "<game-directory>" --code-only`; consult that graph for code investigation and run `graphify update "<game-directory>"` after each implementation. Pass `--graph "<game-directory>/graphify-out/graph.json"` to queries when outside that directory.
- Do not build a graph for the entire RenoDX repository as a default. For work spanning multiple games, use a separate graph per relevant game. For work outside a game, select the smallest relevant subsystem directory as the graph root. Follow narrower project instructions and verify graph findings against source.
