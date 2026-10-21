Netstorm - Reborn
-------------------------------------------------------------------------

[한국어](README.md) | **English**

# Overview
This project uses AI to revive
Netstorm - Islands at War,
a game developed by Activision in 1997 and abandoned long ago.

# Current development status

**Both projects are described as of October 21, 2026.** Playability and fidelity to the original are assessed separately.

| Project | Execution and playability | Fidelity and remaining work |
| --- | --- | --- |
| cpppj — C++ reconstruction | Windows windowed-mode menus, briefings, guidance, construction, production, bridge placement/rotation, harvesting/transport and live combat are connected. Natural completion of Early Missions 1–2 and entry into the next tutorial have been observed, as has destruction of the enemy temple in Campaign 1-1. | Based on decompilation and machine-code comparisons against version 10.78. **Actions and frame integration for the shared enemy, allied and demo AI**, remaining mission transition/cleanup connections, natural capture/sacrifice and outcomes, and full playthrough verification of all 11 target missions remain. Complete visual, audio and input parity is unfinished. |
| dotnetpj — C# + MonoGame clone | Campaign **1-1 The War Begins!** and **1-2 Master of Whirligigs** have playable implementations with construction, production, movement and combat controls. English/Korean UI, help, context menus and options are available. Early Missions and demos cannot be opened from the menu yet. | Original-verified rules are being ported. In addition to session-connected mission rules, original world-core modules for creation, graphs, destruction lifecycles, general damage and priest damage have been verified. **The new world core is not yet connected to live sessions.** Live construction, combat, pathing, SP, AI and timing still use observation-based approximations. Natural completion → progress save → restart → next-mission unlock and original parity require further verification. |

## Latest C++ reconstruction

The current implementation target is **Early Missions 1–6, Campaign 1-1/1-2 and three demos: 11 missions playable with the behavior of version 10.78**. The same live session connects SP, technology, priest participation, Start/StartDemo, construction, production, bridges, economy, combat, Early-specific stage checks and the common mission frame. F8 guidance replay, Guide transitions, failure/abort/next-mission command queues, knowledge cards and Help are also connected. See the [current remaining-work checklist and per-mission table](LEFT_JOBS.md).

[Early Mission 1](docs/exe/cpp-tutorial-harvest-20261019.md) and [Early Mission 2](docs/exe/cpp-tutorial-two-runtime-20261019.md) have been observed completing through normal input, including construction, harvesting/production, completion records and entry into the next tutorial. Campaign 1-1 has reached [enemy facility/temple destruction using a forward Sun Cannon](docs/exe/cpp-forward-window-20261019.md), but natural priest capture, sacrifice and victory have not been verified. Partial Early Mission 5 progress and loading/Start/short-frame checks for the 11 missions are separate from full playthrough evidence.

The [13 focused Devastation/capture/sacrifice checks](docs/exe/cpp-ritual-runtime-focused-20261021.md) passed, but some use explicitly supplied caster, captive and clock states. Natural UI progression from obtaining the spell through casting, capture, transport and sacrifice still needs verification. Following [Sun Disc Thrower/Whirlibase body and flying-unit shadow fixes](docs/exe/cpp-combat-draw-and-defense-20261021.md), the user stopped live game testing; those tests remain on hold.

The [shared AI](docs/exe/cpp-ai-world-preparation-20261021.md) now has creation/initial update, the same brain's wallet and creation/deletion notices, initial world recognition and strategy preparation. **The strategy loop that automatically places bridges and buildings and manages economy/combat is still incomplete.** Normal missions leave the local owner under human control; demos must also run that owner through the same AI. Remaining Template/State transition and cleanup connections and outcome/restart lifecycles also need work.

The recent unit 275 AI Release build finished with **exit code 0, zero warnings and zero errors**. Its 13 related C++ checks were **written and compiled, but not executed**. Historical regression/playthrough results are not reported as passes for the entire current codebase. The 11-mission target and full visual, audio, input and timing parity remain unfinished. DirectDraw error fixes and stable Windows 10/11 fullscreen operation are separate mandatory phase-2 work.

## Latest C# + MonoGame clone

The campaign menu reads chapters, prerequisite completion conditions and saved progress from the original scripts while restricting access to supported missions. Options include resolution, windowed/fullscreen mode, sound, volume, wind noise and speaker swap. Help and F1 expose the original help content, and priest, Workshop and production-item context menus are available. See the [feature/progress-saving review](docs/dotnet-review-20261011.md), [recording-analysis coverage](docs/screens/recorded-ui-clone-20261002.md) and [Campaign 1-1](docs/gameplay/campaign-one.md)/[1-2 implementation](docs/gameplay/campaign-two.md). The [latest handover](LEFT_JOBS.dotnetpj.md) takes precedence over early implementation notes.

The common mission frame gate, player participation/directional alliances, cumulative build ledger and Tutorial 1 stage checks were ported from original-verified results and connected to the existing session. A separate original world core is being built: W02 covers slots/spatial hashing, Pop/Unpop, ownership/frames, bridges and graphs; W03 now includes destruction lifecycles, release, general damage and [priest damage/HP changes](docs/dotnet-original-core-priestdamage-20261021.md), verified at module level. There are **50 linked fixtures**. The latest recorded baseline, W03 part 9 on October 21, has a full Release build with **zero warnings/errors** and **1,274 passing unit tests** (Assets 363, Core 911). These are module verification results and do not expand live playability.

Next are W03 dependent/content lookup and lifetime, Form lifetimes, temple deletion, destruction rewards, derived dispatch and live-session integration. Kernel/Regular timing, SP/random-number handling and the starting states of the 11 target missions follow, then pathing, construction, combat, sacrifice, outcomes, rendering, UI and sound. Areas still unfinished in cpppj, including strategy AI, remain explicitly tracked as approximate while the [cpppj → dotnetpj porting plan](docs/dotnet-cpp-port-plan-20261016.md) guides natural playthrough and original-parity verification.

The linked analysis and handover documents are written in Korean.

# Project layout and development goals

`cpppj` proceeds in this order: **① version 10.78 single-player reconstruction → ② stable Windows 10/11 execution (including mandatory DirectDraw error fixes) → ③ TCP/IP LAN multiplayer and outposts → ④ Korean support and MCP → ⑤ additional requirements**. It is Windows only. Multiplayer-only outposts belong to phase 3; official-server access is not restored. Comparison records for CD version 10.72 are also retained.

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

`--run <game-data folder>` runs the C++ reconstruction. `--view fonts`, `--view types`, and `--view TEST01` open font, type and mission inspection scenes. Live construction, production, bridges, harvesting/transport and combat are connected, while shared strategy AI, complete playthroughs of all 11 target missions and original-parity verification remain unfinished. See [C++ build](docs/cpp-build.md) for build instructions and the [handover](LEFT_JOBS.md) for current integration status.

# Key documents

- [C++/shared handover](LEFT_JOBS.md) / [C# handover](LEFT_JOBS.dotnetpj.md): latest completed work, remaining boundaries and resume order
- [C++ roadmap](docs/cpp-roadmap.md) / [playable reconstruction plan](docs/cpp-playable-plan.md): development stages and reconstruction plans
- [cpppj → dotnetpj porting plan](docs/dotnet-cpp-port-plan-20261016.md): porting units and order for matching dotnetpj to the original
- [Clone game data](assets/README.md): asset layout, provenance and build inclusion
- [Original-game/separate-window execution rules](docs/execution-confirmation.md): execution conditions for AI work

# License

MIT License
Copyright (c) 2026 HJOW
