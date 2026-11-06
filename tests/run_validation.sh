#!/usr/bin/env bash
# Read/write builds only. Never cleans existing directories, flashes, or changes Git.
set -euo pipefail
root=$(cd "$(dirname "$0")/.." && pwd)
cd "$root"
mkdir -p build
out=$(mktemp -d "$root/build/validation.XXXXXX")
echo "VALIDATION_DIR=$out"
for config in Debug Release; do
    cmake --preset "$config" -B "$out/$config" 2>&1 | tee "$out/$config-configure.log"
    cmake --build "$out/$config" --verbose 2>&1 | tee "$out/$config-build.log"
    python3 tests/verify_hex.py "$out/$config/STM32-LowPower-SensorNode.elf" "$out/$config/STM32-LowPower-SensorNode.hex" | tee "$out/$config-hex.log"
done
for mode in host host-release host-sanitize; do
    options=(-DCMAKE_BUILD_TYPE=Debug)
    if [[ $mode == host-release ]]; then options=(-DCMAKE_BUILD_TYPE=Release); fi
    if [[ $mode == host-sanitize ]]; then options+=(-DSANITIZE=ON); fi
    cmake -S tests -B "$out/$mode" -G Ninja "${options[@]}" 2>&1 | tee "$out/$mode-configure.log"
    cmake --build "$out/$mode" 2>&1 | tee "$out/$mode-build.log"
    ctest --test-dir "$out/$mode" --output-on-failure 2>&1 | tee "$out/$mode-test.log"
done
git diff --check
printf 'PASS: validation logs: %s\n' "$out"
