#!/usr/bin/env bash
# Builds one harness test against a fresh copy of the addon's world/ sources.
#   bash build.sh test_pool                    # ASan + UBSan
#   bash build.sh test_motion --tsan           # ThreadSanitizer build (output: test_motion_tsan)
# Needs Linux or WSL with g++ 13+ (C++20). FALCOM_REPO overrides the repo root (default: two levels up).
set -euo pipefail
cd "$(dirname "$0")"
name="${1:?usage: bash build.sh <test> [--tsan] [extra g++ flags]}"; shift
REPO="${FALCOM_REPO:-$(cd ../.. && pwd)}"
sanitize=(-fsanitize=address,undefined -fno-sanitize-recover=undefined)
out="$name"
extra=()
for arg in "$@"; do
  if [[ "$arg" == --tsan ]]; then sanitize=(-fsanitize=thread); out="${name}_tsan"; else extra+=("$arg"); fi
done
# The world headers include ../../../../utils/*.hpp: with the copy under
# src/games/falcomengine-plus/world those resolve to the stub utils in src/utils.
rm -rf src/games
mkdir -p src/games/falcomengine-plus
cp -r "$REPO/src/games/falcomengine-plus/world" src/games/falcomengine-plus/
find src/games -type f \( -name '*.hpp' -o -name '*.h' -o -name '*.hlsl' -o -name '*.hlsli' \) -exec sed -i 's/\r$//' {} +
g++ -std=c++20 -g -O1 -Wno-changes-meaning "${sanitize[@]}" -include prelude.h -I. -Iinc \
    -I"$REPO/external/reshade/include" \
    -DFALCOM_BYTECODE_DIR="\"$(cd ../bytecode && pwd)/\"" \
    "${extra[@]}" -o "$out" "$name.cpp"
