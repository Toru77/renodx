#!/usr/bin/env bash
# Builds and runs every harness test (ASan/UBSan), then the ThreadSanitizer
# builds of the tests with real concurrency. Prints one line per test.
set -uo pipefail
cd "$(dirname "$0")"
tests=(test_pool test_indirect test_switches test_visibility test_live test_build test_verify test_deform test_deform_live test_motion)
tsan=(test_switches test_verify test_motion)
status=0
for t in "${tests[@]}"; do
  if ! bash ./build.sh "$t" > "$t.build.log" 2>&1; then echo "$t BUILD FAILED (see $t.build.log)"; status=1; continue; fi
  timeout 600 "./$t" > "$t.out" 2>&1; rc=$?
  echo "$t rc=$rc $(tail -1 "$t.out")"; [[ $rc -eq 0 ]] || status=1
done
for t in "${tsan[@]}"; do
  if ! bash ./build.sh "$t" --tsan > "${t}_tsan.build.log" 2>&1; then echo "$t TSAN BUILD FAILED"; status=1; continue; fi
  timeout 600 "./${t}_tsan" > "${t}_tsan.out" 2>&1; rc=$?
  echo "$t tsan rc=$rc $(tail -1 "${t}_tsan.out") warnings=$(grep -c 'WARNING: ThreadSanitizer' "${t}_tsan.out")"
  [[ $rc -eq 0 ]] || status=1
done
exit $status
