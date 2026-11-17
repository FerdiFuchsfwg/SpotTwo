#!/usr/bin/env bash
set -euo pipefail

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target SpotTwo_VST3

echo "Done. Look in: build/SpotTwo_artefacts/Release/VST3/SpotTwo.vst3"
