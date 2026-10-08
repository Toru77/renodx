# World module native harness

Runs the add-on's real `world/` code against a mock ReShade device on Linux. It catches logic, lifetime and threading bugs before an in-game run. It does not replace the MSVC build (the real compile check, including FXC for shaders) or in-game validation.

## Requirements

WSL (Ubuntu) or Linux with g++ 13 or newer (C++20, ASan/UBSan/TSan), bash, python3 (only to regenerate the mock).
The repo's `external/reshade` submodule must be checked out (the real ReShade API headers are used).

## Use

From this folder (in WSL the repo is usually under `/mnt/e/RenoDX/renodx`):

    bash run_all.sh                   # all tests + TSan builds, one line each
    bash build.sh test_motion         # one test (ASan/UBSan), then ./test_motion
    bash build.sh test_motion --tsan  # ThreadSanitizer build -> ./test_motion_tsan

`build.sh` copies `src/games/falcomengine-plus/world` fresh from the repo on every build (CRLF converted), so tests always run the current sources. `FALCOM_REPO=/path/to/repo` overrides the repo root.

## Layout

- `test_*.cpp`: one test program per subsystem (pool admission, indirect draws, switches/lifetime, camera visibility, live BVH + trace emulation, CPU BVH build, mesh capture verification, deform probe, deforming meshes live, motion probe + moving objects). Each prints `PASS (0 failures)` or the failing checks.
- `gen/mock_base.hpp`: ReShade device/command list/queue mock bases, generated from the ReShade API header: `python3 gen/genmock.py ../../external/reshade/include/reshade_api_device.hpp > gen/mock_base.hpp` (from this folder) if the ReShade API changes.
- `inc/include/reshade.hpp`: stand-in for the add-on's `<include/reshade.hpp>` (real declarations, event registration no-ops).
- `src/utils/*.hpp`: stubs of the RenoDX utils the world module uses (log lines captured in `renodx::utils::log::g_lines`, output dir = temp/pooltest, scene decode/layout copied from the real code).
- `mesh_fixture.hpp`: box mesh, index/vertex writers and an input layout for pool tests.
- `prelude.h`: force-included shims (`__declspec`, `__uuidof`, bytecode folder).

The mocks reproduce two ReShade D3D11 behaviors that once caused crashes: `update_buffer_region` at offset 0 writes the whole buffer, and `get_resource_desc` reports buffer stride 0.

All tests passed at the P2a deploy (2026-10-08).
