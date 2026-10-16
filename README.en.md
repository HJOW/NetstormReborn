Netstorm - Reborn
-------------------------------------------------------------------------

[한국어](README.md) | **English**

# Overview
This project uses AI to revive
Netstorm - Islands at War,
a game developed by Activision in 1997 and abandoned long ago.

# Current development status

**Status as of October 16, 2026 for both C++ and C#.** Playability and fidelity to the original are assessed separately below.

| Project | Execution and playability | Fidelity and remaining work |
| --- | --- | --- |
| `cpppj/` — C++ reconstruction | On Windows in windowed mode, menu → campaign → briefing → fort, terrain, object and dialog display, selection, priest movement, some live combat and return to the menu work. | Reconstruction uses decompilation and comparisons against version 10.78 machine code. **Construct is disabled.** Construction, production, bridges, harvesting, transport, sacrifice, GUI Start/StartDemo and AI strategy frames, and live mission progression through outcome saving still need integration. Natural completion of Early Missions and Campaign 1-1/1-2, and full visual/audio parity, remain unfinished. |
| `dotnetpj/` — C# + MonoGame clone | Campaign **1-1 The War Begins!** and **1-2 Master of Whirligigs** have playable implementations with construction, production, movement and combat controls. English/Korean UI, help, context menus and options are available. Early Missions (tutorials) and demos cannot be opened from the menu yet. | Original data is reused while rules verified by cpppj against original machine code are ported. So far, the common mission frame, player participation/directional alliances, the build ledger and the Tutorial 1 stage logic match original expected values (14 fixtures). Construction, combat, pathing, SP, AI and timing rules are still observation-based approximations. A [porting plan](docs/dotnet-cpp-port-plan-20261016.md) sequences the remaining cpppj results. A full playthrough → progress save → restart → next-mission unlock and complete original parity still need verification. |

## Latest C++ reconstruction

The same session now connects SP, technology, priest participation, AI creation/initial update, and the common mission Start. Headless Load/Start checks pass for Early Missions 1–6, Campaign 1-1/1-2, and all three demos. GUI Start/StartDemo, Guide and mission-frame integration, AI strategy, natural playthroughs and full visual/audio parity remain unfinished.

The October 16 Release build has **zero warnings/errors**. All three demo Load/Start checks pass, and **9 of 11 missions run for 400 raw Kernel frames**. Sun Barricade connections and deletion are joined by Man o'War post-processing, its shadow, and base flyer construction, home binding and state transmission. The full suite reports **1,370 passed and 2 failed out of 1,372 cases**; Demo 1 now reaches the unimplemented flyer Frame body, while Demo 3 still stops at other gun animations ([details](docs/exe/cpp-flyer-postpop-20261016.md)). All 1,421 original-code comparisons and actual-resource creation checks pass. Natural playthrough and complete graphics/audio parity remain unverified. Fixing the legacy DirectDraw fullscreen error remains excluded from the current specified-mission reconstruction task, and is **mandatory in C++ phase 2** under the October 16 AGENTS.md update.

Development is **active**. See the [C++/shared handover](LEFT_JOBS.md), [demo initialization](docs/exe/cpp-demo-initialization-20261016.md), [AI colors](docs/exe/cpp-player-color-20261016.md), and [demo/campaign correspondence](docs/gameplay/demo-campaign-map.md).

## Latest C# + MonoGame clone

The campaign menu reads chapters, prerequisite completion conditions and saved progress from the original scripts, while restricting access to supported missions. Options include resolution, windowed/fullscreen mode, sound, volume, wind noise and speaker swap. Help and F1 provide the original help content; priest, workshop and production-item context menus are available. See the [feature and progress-saving review](docs/dotnet-review-20261011.md), [recording-analysis coverage](docs/screens/recorded-ui-clone-20261002.md), and [Campaign 1-1](docs/gameplay/campaign-one.md)/[1-2 implementation](docs/gameplay/campaign-two.md). The latest handover takes precedence over early implementation notes.

On October 15, four original-verified cpppj results were ported and connected to the session: the common mission frame gate (328 original rows), player participation and directional alliances (608 rows), the build ledger (52 rows), and the Tutorial 1 stage logic (266 rows). The human-side `myAllyList` key is no longer read because it does not exist in the original executable; this changes alliance results in 21 original missions. Clone-window smoke tests (run after approval) and a headless screen-path completion of Tutorial 2 passed. The October 16 baseline is a Release build with **zero warnings/errors** and **1,057 unit tests passed** (Assets 343, Core 714).

Next work follows the [cpppj → dotnetpj porting plan](docs/dotnet-cpp-port-plan-20261016.md). It first builds an original world core that matches cpppj's live session: object creation, registration and deletion; Kernel/Regular timing; and the SP store with its random-number use. With that core, the start state of the eleven target missions (Early Missions 1–6, Campaign 1-1/1-2 and three demos) is matched against cpppj. Priests, pathing, construction, combat, sacrifice, outcomes, rendering, UI and sound follow, verified by natural playthroughs. Behavior that cpppj has not restored yet, such as AI strategy, stays approximate and is tracked in a list. The checklist is in the [C# handover](LEFT_JOBS.dotnetpj.md).

The linked analysis and handover documents are written in Korean.

# Project layout and development goals

`cpppj` proceeds in this order: **① version 10.78 single-player reconstruction → ② stable Windows 10/11 execution (including mandatory DirectDraw error fixes) → ③ Korean support → ④ TCP/IP LAN multiplayer and outposts → ⑤ added requirements and MCP**. It is Windows only. Multiplayer-only outposts belong to phase 4; official-server access is not restored. Comparison records for CD version 10.72 are also retained.

`dotnetpj` uses C# and MonoGame. The existing clone receives the parts of `cpppj` already verified against original machine code, one area at a time, to match the original behavior first (October 16 porting plan). Once `cpppj` is complete, everything is compared again before new requirements are added. GUI Linux support and multiplayer have lower priority. Final requirements include fullscreen, 16:9/16:10/4:3 aspect ratios, fullscreen edge scrolling, and English/Korean support. Animation first targets the original cadence, with 60/120-frame support afterwards.

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
- [cpppj → dotnetpj porting plan](docs/dotnet-cpp-port-plan-20261016.md): porting units and order for matching dotnetpj to the original
- [Clone game data](assets/README.md): asset layout, provenance and build inclusion
- [Original-game/separate-window execution rules](docs/execution-confirmation.md): execution conditions for AI work

# License

MIT License
Copyright (c) 2026 HJOW
