# SpotTwo

**SpotTwo** ist ein eigenständiges, mit JUCE entwickeltes VST3-Projekt zur monophonen Tonhöhenkorrektur. Der Workflow orientiert sich an der allgemeinen Idee eines kompakten Vocal-Tuners: Amount, Speed, einzeln anwählbare Zielnoten und eine Live-Pitch-Anzeige. Name, Quellcode, Oberfläche und visuelle Gestaltung sind eigenständig. Das Projekt ist nicht mit Sixth Sample verbunden und enthält weder SpotOn-Code noch dessen Grafiken oder Branding.

## Funktionen

- Automatische monophone Tonhöhenerkennung für stimmenähnliches Audiomaterial.
- **Amount** regelt die Stärke der Tonhöhenkorrektur.
- **Speed** regelt, wie schnell die Korrektur auf die Zielnote zuläuft.
- Klickbares Ein-Oktaven-Keyboard mit zwölf einzeln anwählbaren Noten.
- Jede Zielnote ist ein eigener DAW-Parameter und kann automatisiert werden.
- Schnellauswahl für Grundton und Skala: Chromatic, Major, Natural Minor, Dorian, Major Pentatonic, Minor Pentatonic und Blues.
- Live-Anzeige der erkannten Tonklasse inklusive Frequenzanzeige.
- Zehn integrierte Farbthemen plus **Custom**.
- Vier unabhängig einstellbare Akzentfarben: Primary, Secondary, Keyboard und Meter.
- Eigene Farben werden im Plug-in-Zustand gespeichert.
- DAW-Parameter und UI-Farben werden mit dem Projekt gespeichert.
- VST3-Effekt für Windows, macOS und Linux – abhängig von der lokalen JUCE- und Compiler-Umgebung.

## Hinweis zur DSP-Qualität

Die erste Implementierung verwendet einen kompakten YIN-basierten monophonen Pitch-Tracker und einen Crossfade-Pitch-Shifter mit zwei Delay-Grains. Das ist eine eigenständige, relativ leichte Echtzeit-Implementierung – keine Kopie des internen SpotOn-Algorithmus und kein Offline-Vocal-Editor oder Phase-Vocoder. Sie ist für einzelne Vocal-Linien gedacht. Dichte Akkorde, polyphone Signale, sehr hauchige Stimmen und stark verrauschtes Material können zu unruhiger Erkennung oder hörbaren Grain-/Phasenartefakten führen. Amount und Speed sollten deshalb im musikalischen Kontext eingestellt werden.

Das Plug-in meldet eine Latenz von **2048 Samples**. Bei 48 kHz entspricht das ungefähr 42,7 ms. Auch im Bypass bleibt der Delay-Pfad mit Pitch-Ratio 1 aktiv, damit die gemeldete Plug-in-Latenz konsistent bleibt.

## Windows-Build

### Voraussetzungen

- Windows 10/11, 64 Bit.
- Visual Studio 2022 oder ein kompatibler MSVC-C++-Compiler.
- CMake 3.22 oder neuer.
- Internetverbindung beim ersten Konfigurieren/Build, damit CMake den fest versionierten JUCE-Quellcode (9.0.5) laden kann.

### Kompilieren

Öffne die **x64 Native Tools Command Prompt for Visual Studio** im Projektordner und führe Folgendes aus:

```powershell
cmake -S . -B build -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target SpotTwo_VST3
```

Alternativ kann das Hilfsskript verwendet werden:

```powershell
.\scripts\build-windows.ps1
```

Das VST3-Bundle sollte anschließend hier liegen:

```text
build\SpotTwo_artefacts\Release\VST3\SpotTwo.vst3
```

Kopiere das Bundle in deinen VST3-Plugin-Ordner und starte in deiner DAW einen erneuten Plug-in-Scan.

## macOS / Linux

Installiere CMake 3.22+, einen C++17-Compiler und die von JUCE benötigten Entwicklungsbibliotheken. Führe danach Folgendes aus:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target SpotTwo_VST3
```

Ein Hilfsskript liegt unter `scripts/build-macos-linux.sh`.

## GitHub Actions

Der Workflow `.github/workflows/build-windows.yml` kompiliert das Windows-VST3 bei Pushes auf `main`/`master`, bei Pull Requests oder manuell über den Actions-Tab. Das erzeugte `.vst3`-Bundle wird als Workflow-Artefakt hochgeladen.

## Projektstruktur

```text
SpotTwo/
├── .github/workflows/build-windows.yml
├── scripts/
│   ├── build-macos-linux.sh
│   └── build-windows.ps1
├── Source/
│   ├── PluginEditor.cpp
│   ├── PluginEditor.h
│   ├── PluginProcessor.cpp
│   └── PluginProcessor.h
├── CMakeLists.txt
└── README.md
```

## Lizenz

Der SpotTwo-Quellcode ist für **AGPL-3.0-or-later** vorgesehen. JUCE wird als separate Abhängigkeit eingebunden und ist selbst unter AGPLv3 oder einer kommerziellen JUCE-Lizenz verfügbar. Prüfe die JUCE-Lizenzbedingungen vor allem bei der Weitergabe von Builds oder proprietärer/kommerzieller Nutzung: <https://github.com/juce-framework/JUCE/blob/master/LICENSE.md>.

## Hinweis

SpotTwo ist ein unabhängiges Projekt und wurde eigenständig erstellt. Produktnamen und Marken Dritter bleiben Eigentum ihrer jeweiligen Inhaber. Dieses Repository enthält keine proprietären Quelltexte, Logos, Screenshots oder Grafiken Dritter.
