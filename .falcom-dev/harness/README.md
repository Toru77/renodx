# World module native harness (Windows)

Runs the add-on's real `world/` code against a mock ReShade device, built with MSVC. It catches logic and lifetime bugs before an in-game run. It does not replace the addon build (the real compile check, including FXC for shaders) or in-game validation.

## Requirements

MSVC `cl` with the x64 toolset. On this machine the working toolset is the Build Tools install under `C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools`. `build_win.bat` only looks for VS 2022 Community/Professional, so it fails there; it needs that path added (or run the `cl` command from a `vcvars64.bat` prompt).
The repo's `external/reshade` submodule must be checked out (the real ReShade API headers are used).

## Use

From this folder (`.falcom-dev\harness`), in PowerShell:

    powershell -File refresh_world.ps1                      # copy current world/ sources into the harness (CR bytes stripped)
    powershell -File run_win_all.ps1                        # all tests, table of build/run seconds and rc
    powershell -File run_win_some.ps1 -Tests test_live,test_motion
    powershell -File run_win_one.ps1 -Test test_pool        # build (only if stale) and run one test
    build_win.bat test_pool                                 # build one test with ASan (test_pool.exe)
    build_win.bat test_pool fast                            # build one test without sanitizer (test_pool_fast.exe)

Always run `refresh_world.ps1` before a build: the harness compiles the copy under `src/games/falcomengine-plus/world`, not the repo files. `run_win_some.ps1` refreshes once per run. Results from a build made before a refresh do not test the current code.

Each test prints `PASS (0 failures)` or the failing checks. `RESULT test=...` is the last line of `run_win_one.ps1` output.

## Layout

- `test_*.cpp`: one test program per subsystem (pool admission, indirect draws, switches/lifetime, camera visibility, live BVH + trace emulation, CPU BVH build, mesh capture verification, deform probe, deforming meshes live, motion probe + moving objects, alpha-tested foliage atlas).
- `gen/mock_base.hpp`: ReShade device/command list/queue mock bases, generated from the ReShade API header with `python3 gen/genmock.py ../../external/reshade/include/reshade_api_device.hpp > gen/mock_base.hpp` if the ReShade API changes (python3 is needed only for this).
- `inc/include/reshade.hpp`: stand-in for the add-on's `<include/reshade.hpp>` (real declarations, event registration no-ops).
- `src/utils/*.hpp`: stubs of the RenoDX utils the world module uses (log lines captured in `renodx::utils::log::g_lines`, output dir = temp/pooltest, scene decode/layout copied from the real code).
- `mesh_fixture.hpp`: box mesh, index/vertex writers and an input layout for pool tests.
- `prelude.h`: force-included shims (`__declspec`, `__uuidof`, bytecode folder).
- `*.log`, `*.out`, `*.err`, `*.exe`, `*.obj`, `*.pdb`: build and run output. Generated; ignored by git.

The mocks reproduce two ReShade D3D11 behaviors that once caused crashes: `update_buffer_region` at offset 0 writes the whole buffer, and `get_resource_desc` reports buffer stride 0.

## Limits

- No ThreadSanitizer build on Windows. The threaded paths were checked with TSan under the old Linux setup (P2a); those results do not cover later changes.
- The harness does not run D3D11 or FXC. GPU shader and resource changes are verified only by the addon build and in-game steps.
