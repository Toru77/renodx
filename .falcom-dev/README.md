# .falcom-dev (local developer files for falcomengine-plus/world)

Not part of the addon build. Kept outside `src/games/falcomengine-plus/` on purpose: the addon's CMake glob embeds every `*.cso` under the addon folder.
If you don't want it in git, add `.falcom-dev/` to `.git/info/exclude` (local only) or to `.gitignore`.

- `harness/`: native test harness for the world module (mock ReShade device, stub utils, CPU transcriptions of the GPU shaders). See `harness/README.md`.
- `bytecode/`: real game shader bytecode (`0x????????.vs.cso` / `.ps.cso`) used by the classifier tests.
- `transcript/`: the raw transcript (JSON lines, gzip) of the claude.ai session that built path 1 and started path 2, for reference only. The distilled state is in `src/games/falcomengine-plus/world/docs/ROADMAP.md`.
