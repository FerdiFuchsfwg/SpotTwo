$ErrorActionPreference = "Stop"

Write-Host "Configuring SpotTwo (Release / x64)..."
cmake -S . -B build -A x64 -DCMAKE_BUILD_TYPE=Release

Write-Host "Building the VST3 target..."
cmake --build build --config Release --target SpotTwo_VST3

Write-Host "Done. Look in: build/SpotTwo_artefacts/Release/VST3/SpotTwo.vst3"
