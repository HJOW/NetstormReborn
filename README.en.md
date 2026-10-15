Netstorm - Reborn
-------------------------------------------------------------------------

[한국어](README.md) | **English**

# Overview
This project uses AI to revive
Netstorm - Islands at War,
a game developed by Activision in 1997 and abandoned long ago.

# Current development status

**Based on the handover dated October 15, 2026.** Playability and fidelity to the original are assessed separately below.

| Project | Execution and playability | Fidelity and remaining work |
| --- | --- | --- |
| `cpppj/` — C++ reconstruction | On Windows in windowed mode, menu → campaign → briefing → fort, terrain, object and dialog display, selection, priest movement, some live combat and return to the menu work. | Reconstruction uses decompilation and comparisons against version 10.78 machine code. **Construct is disabled.** Construction, production, bridges, harvesting, transport, sacrifice, AI activation and strategy, and live mission progression through outcome saving still need integration. Natural completion of Early Missions and Campaign 1-1/1-2, and full visual/audio parity, remain unfinished. |
| `dotnetpj/` — C# + MonoGame clone | Campaign **1-1 The War Begins!** and **1-2 Master of Whirligigs** have playable implementations with construction, production, movement and combat controls. English/Korean UI, help, context menus and options are available. | Original data is reused while rules are reconstructed. Provisional AI/construction rules and timing, frame and rendering differences remain. Recent session-clock and tutorial-frame changes have only been build-checked. A full playthrough → progress save → restart → next-mission unlock and complete original parity still need verification. |

## Latest C++ reconstruction

Common deletion, technology-content lifetime, and current SP/UI notices now share the campaign session. Dialog bodies use the restored ordering for HTML conditions, configuration substitution, images and inline styles. The AI constructor and initial update have been reconstructed and verified as separate modules, but live-session AI activation is pending. The four mana-metadata files are a **draft excluded from CMake, with no build or new original-code comparison performed**.

The final October 15 record reports **zero warnings/errors** in the Release game/test builds and **1/1 CTest passed (1,233 internal cases, zero failures)**. Clone GUI checks covered 97 dialog states, 23 Early1/2 dialog states, and 9 completion-value/next-tutorial states. Forced completion-dialog checks do not establish a natural gameplay playthrough. Fixing the legacy DirectDraw fullscreen error is outside the current work scope.

Development toward playthrough parity is paused at the user's request. Resume steps and remaining boundaries are in the [C++/shared handover](LEFT_JOBS.md). See [session lifetime integration](docs/exe/cpp-session-lifetime-20261015.md), [SP storage and notices](docs/exe/cpp-sp-store-notice-20261015.md), [AI initial update](docs/exe/cpp-ai-initial-update-20261015.md), and [dialog conditions and processing order](docs/exe/cpp-dialog-condition-flow-20261015.md) for evidence.

## Latest C# + MonoGame clone

The campaign menu reads chapters, prerequisite completion conditions and saved progress from the original scripts, while restricting access to supported missions. Options include resolution, windowed/fullscreen mode, sound, volume, wind noise and speaker swap. Help and F1 provide the original help content; priest, workshop and production-item context menus are available. See the [feature and progress-saving review](docs/dotnet-review-20261011.md), [recording-analysis coverage](docs/screens/recorded-ui-clone-20261002.md), and [Campaign 1-1](docs/gameplay/campaign-one.md)/[1-2 implementation](docs/gameplay/campaign-two.md). The latest handover takes precedence over early implementation notes.

On October 12–13, the original numeric clock was connected to the session and nested pauses, followed by the initial 10-frame briefing delay, 7-frame resume delay, subsequent 0.5-second/10-frame dialog scheduling, and Tutorial 2's 2/4-second timers. Behavioral, GUI and independent original-code comparisons after those integrations are pending. The full solution's October 14 Release build had **zero warnings/errors**. The earlier October 12 results—**990 unit tests passed** and **12 GUI runs/188 assertions passed**—do not verify those later changes.

Next work includes validating the recent clock/dialog integrations, migrating remaining movement, combat, construction, production, harvesting, ritual, AI and animation timers, resolving the 24 Hz/eight-tick cap, and verifying full playthroughs and saved progress. See the [C# handover](LEFT_JOBS.dotnetpj.md), [session clock](docs/dotnet-session-clock-20261012.md), and [tutorial frames and timers](docs/dotnet-stage-frames-20261013.md).

The linked analysis and handover documents are written in Korean.

# Project layout and development goals

`cpppj` proceeds in this order: **① version 10.78 single-player reconstruction → ② stable Windows 10/11 execution → ③ Korean support → ④ TCP/IP LAN multiplayer and outposts → ⑤ added requirements and MCP**. It is Windows only. Multiplayer-only outposts belong to phase 4; official-server access is not restored. Comparison records for CD version 10.72 are also retained.

`dotnetpj` uses C# and MonoGame. Its existing clone will later be refined by analyzing the completed `cpppj`, first matching the original behavior and then adding new requirements. GUI Linux support and multiplayer have lower priority. Final requirements include fullscreen, 16:9/16:10/4:3 aspect ratios, fullscreen edge scrolling, and English/Korean support. Animation first targets the original cadence, with 60/120-frame support afterwards.

| Path | Purpose |
| --- | --- |
| `cpppj/` | C++20/Win32 reconstruction and tests |
| `dotnetpj/` | C#/MonoGame DesktopGL clone and tests |
| `assets/game-data/` | Clone graphics, maps, missions, music and sound effects |
| `fonts/` | D2Coding fonts for Korean text |
| `docs/` | Analysis evidence, reconstruction, build, gameplay and verification notes |
| `tools/`, `analyzeManager/` | Static analysis, original-game observation and verification tools |
| `originals/`, `originalCD/` | Original version 10.78 and CD version 10.72 analysis material |
| `screenShots/`, `playingVideos/`, `originalFanGame/` | Screenshot, recording and fan-game references. Large recordings and fan-game code are excluded from Git and may be absent on a given PC. |

Korean building and unit names follow the [shared naming policy](docs/korean-unit-names.md). Priest, Temple, Bridge, Thrower and Geyser may be translated; other names use Korean phonetic spellings, including Workshop → 워크샾 and Generator → 제네레이터. The dotnetpj menus and unit titles use the shared 55-entry table. Full help and mission-text translation remains unfinished.

Read [AGENTS.md](AGENTS.md) and both handovers before working. Original-game and separate-window execution follows the [execution-confirmation rules](docs/execution-confirmation.md). Original analysis files are protected; changes to clone data belong in `assets/game-data/`.

# Preparing the development environment

The root preparation scripts let you select tools to install or inspect the current environment. To check without installing:

```powershell
powershell -ExecutionPolicy Bypass -File .\PREPARE.ps1 -CheckOnly
```

```sh
bash ./PREPARE.sh --check-only
```

Omit `-CheckOnly` or `--check-only` to open tool selection and installation. REA is optional and unselected by default, and uses an existing Node.js/npm installation. Preparing Ghidra and REA together connects `GHIDRA_INSTALL_DIR`. Neither game build requires running the original game or installing REA.

# Building and creating executables (C# + MonoGame build)

Run the commands below from the project root (`NetstormReborn`). The game is written in C# with MonoGame DesktopGL, and the target framework is `net10.0`.

## Prerequisites

- Install the **.NET 10 SDK** and check it with `dotnet --version`. `dotnetpj/global.json` accepts the latest feature band of the .NET 10 SDK, 10.0.100 or later.
- The first build or publish needs an internet connection to download NuGet packages and the runtime for the target OS.
- Check out `assets/game-data/` and the Korean font `fonts/D2Coding-Ver1.3.2-20180524-all.ttc` together with the repository. The data and font are copied to build and publish output automatically.
- The game runs on **Windows 10/11 x64** or **x64 Linux with a GUI (glibc-based)**. A graphics driver with OpenGL support is required.

The dotnetpj build, tests and runtime use the separate `assets/game-data/` and do not require the original executable. Keep `originals/` separately as protected analysis material. For the layout of the assets and the migration record, see [Clone game data](assets/README.md).

## Development build and run

```sh
dotnet restore dotnetpj/Netstorm.sln
dotnet build dotnetpj/Netstorm.sln -c Release --no-restore
dotnet test dotnetpj/Netstorm.sln -c Release --no-build
dotnet run --project dotnetpj/src/Netstorm.Game -c Release --no-build -- --language english
```

Use `--language korean` to run in Korean. Build output goes to `dotnetpj/src/Netstorm.Game/bin/Release/net10.0/`. A normal build produces an executable for the OS it was built on; to build binaries for another OS, use the `-r` option shown below.

## Distributable binaries with the runtime included

Run `dotnet publish` on the game project. The following commands work in both Windows PowerShell and a Linux shell, and you can build the Windows exe on Linux as well.

```sh
dotnet publish dotnetpj/src/Netstorm.Game/Netstorm.Game.csproj -c Release -r win-x64 --self-contained true -o dist/win-x64
dotnet publish dotnetpj/src/Netstorm.Game/Netstorm.Game.csproj -c Release -r linux-x64 --self-contained true -o dist/linux-x64
```

| Target | Output folder | Executable |
| --- | --- | --- |
| Windows 10/11 x64 | `dist/win-x64/` | `NetstormClone.exe` |
| Linux x64 | `dist/linux-x64/` | `NetstormClone` |

`--self-contained true` bundles the .NET runtime, so the PC that runs the game does not need the .NET SDK or runtime installed. The SDL2 and OpenAL native libraries, `fonts/`, and `game-data/` are also included in the output. The executable uses the DLLs, runtime files, font, and game data generated alongside it, so **distribute the whole output folder**. The OS environment, such as the graphics driver and a GUI on Linux, is still required separately.

If the .NET 10 runtime is already installed on the PC that will run the game, you can make a smaller package. Change `--self-contained true` to `--self-contained false` in the commands above, and use a separate output folder such as `dist/win-x64-fdd` or `dist/linux-x64-fdd`.

## Bundled game data and running the binaries

On build and publish, the contents of `assets/game-data/` are **copied automatically to `game-data/`** in the output. The Windows distribution folder looks like this (Linux is the same, with the executable named `NetstormClone`).

```text
dist/win-x64/
  NetstormClone.exe
  NetstormClone.dll
  ... (DLLs, runtime, and configuration files generated alongside)
  fonts/
    D2Coding-Ver1.3.2-20180524-all.ttc
  game-data/
    netstorm.tarc
    d/
    music/
    sound/
```

Run in Windows PowerShell:

```powershell
.\dist\win-x64\NetstormClone.exe --language english
```

Run on Linux:

```sh
chmod +x dist/linux-x64/NetstormClone
./dist/linux-x64/NetstormClone --language english
```

A normal run finds the bundled data automatically. To use data from another location for development or verification, set `NETSTORM_DATA` to the **absolute path of the folder that contains `netstorm.tarc`**. The search order is a valid `NETSTORM_DATA`, then `game-data/` or `assets/game-data/` in the executable folder, the working folder, and their parent folders.

```powershell
$env:NETSTORM_DATA = "D:\Games\NetstormReborn\game-data"
.\dist\win-x64\NetstormClone.exe --language english
```

```sh
NETSTORM_DATA="/absolute/path/game-data" ./dist/linux-x64/NetstormClone --language english
```

# C++ build (reconstruction in progress)

You need Windows 10/11, CMake 3.21 or later, and a C++20 compiler (Visual Studio 2022 or the C++ tools in Build Tools). This build uses the Win32 API directly and is Windows only. Run from the repository root:

```sh
cmake -S cpppj -B cpppj/build -DCMAKE_BUILD_TYPE=Release
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build --build-config Release --output-on-failure
```

With the Visual Studio generator, the executable is `cpppj/build/bin/Release/NetstormCpp.exe`. Open the reconstruction's main menu in windowed mode with:

```powershell
.\cpppj\build\bin\Release\NetstormCpp.exe --run originals --window
```

`--run <game-data folder>` runs the C++ reconstruction. `--view fonts`, `--view types`, and `--view TEST01` open font, type and mission inspection scenes. Menus, movement and some combat work, while Construct and mission-completion integration remain unfinished. See [C++ build](docs/cpp-build.md) for build instructions and the [handover](LEFT_JOBS.md) for current integration status.

# Key documents

- [C++/shared handover](LEFT_JOBS.md) / [C# handover](LEFT_JOBS.dotnetpj.md): latest completed work, remaining boundaries and resume order
- [C++ roadmap](docs/cpp-roadmap.md) / [playable reconstruction plan](docs/cpp-playable-plan.md): development stages and reconstruction plans
- [Clone game data](assets/README.md): asset layout, provenance and build inclusion
- [Original-game/separate-window execution rules](docs/execution-confirmation.md): execution conditions for AI work

# License

MIT License
Copyright (c) 2026 HJOW
