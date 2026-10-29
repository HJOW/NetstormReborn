Netstorm - Reborn
-------------------------------------------------------------------------

[한국어](README.md) | **English**

# Overview
This project uses AI to revive
Netstorm - Islands at War,
a game developed by Activision in 1997 and abandoned long ago.

# Current development status

**As of October 29, 2026:** cpppj reflects product unit 563 and documentation unit 564, dotnetpj reflects `20261029-02` / R2d-2, and webpj reflects the October 28 WB03-D handover. Playability and fidelity to the original are assessed separately.

| Project | Execution and playability | Fidelity and remaining work |
| --- | --- | --- |
| cpppj — C++ reconstruction | Original menus/briefings, construction, production, combat, left deck, minimap, QWASZX, ESC and shared AI are connected. Product 560/unit 561 passed entry, short progression and exit for 31 regular routes and three demos. Parts of Edit map creation/loading, saving, island placement and Test Battle return were observed. Product 563 passed fresh-install main-menu entry, saved fullscreen restart and the windowed launcher. | Based on version 10.78 decompilation and machine-code comparisons. Unit 563 passed 15 focused tests; 3,427 tests are registered, and both Release builds had zero warnings/errors. The full 34 routes and full C++ suite were not rerun on product 563. Natural playthroughs, progress/outcomes/demo looping, remaining Edit/scenario/lifetime work, complete visual/audio parity and Windows 11 hardware validation remain. Windows 10 DIB fullscreen, three aspect ratios and edge scrolling are implemented with limited runtime checks. |
| dotnetpj — C# + MonoGame clone | The existing path can play Campaign **1-1 The War Begins!** and **1-2 Master of Whirligigs**, with English/Korean UI, help, context menus and options. | A new `Netstorm.Client` path is being built around the original architecture, with an 8-bit screen and MonoGame output host, drawing primitives, palettes and edition/display data in progress under R2. The latest Release build and full test run on October 29, 2026 had zero warnings/errors and **1,500 passing tests**. The new screen path and world core are not yet connected to the default live game session; playability and fidelity remain at the approximate existing 1-1/1-2 path. |
| webpj — HTML + JavaScript web clone | Loads the original assets, shows a "click to start" screen and then the main menu. Campaign, Demo and Credits dialogs/lists are implemented, and the Edit button has been removed. | The October 28 WB03-D baseline has 34 passing automated tests. Main-menu and dialog layout/input/sound are being reconstructed. Help, Options and other destination screens, as well as gameplay, are not implemented yet. Custom-map Edit and multiplayer are excluded from the web goals. |

## Latest C++ reconstruction

The target remains **all 6 Early Missions, 9 Priest Training stages, 16 campaign missions and 3 demos with version 10.78 behavior**, plus menus except Multiplayer, in-game UI and custom map/scenario editing. Construction, production, bridges, harvesting/transport, combat and shared AI are connected. Product 560/unit 561 checked regular routes through CLI mission selection and real briefing input, and demos through their actual menu and guidance timers, followed by short progression and normal exit. Earlier Early 1–2 progression and Campaign 1-1 automation/user completion are separate evidence; they do not establish complete natural playthroughs or parity on the current product. See the [handover](LEFT_JOBS.md) for remaining work.

Unit 563 (October 29, 2026) restored the original 39 → 40 → 41 → 3 intro-state return that previously terminated fresh installations. All 17 independent CPU inputs matched the state blocks and call order. Fifteen focused tests passed, 3,427 tests are registered, and both final Release builds had zero warnings/errors. The same packaged executable passed fresh-install main-menu entry, saved fullscreen restart and the default windowed launcher. The default incremental build-to-ZIP path, 1,356 data files and three VC runtime DLLs were checked. The user confirmed that intros played only with the CD inserted; version 10.78 has no bundled movie. CD discovery and playback of existing SMK files remain unimplemented. Ctrl+Shift+F7 diagnostics were connected in unit 560; nonzero trace-to-chat, metamap boundaries, remaining UI/Edit/lifetime work, natural playthroughs and Windows 11 hardware validation remain. Distinguish the [563 report](docs/exe/cpp-native-intro-state-20261029.md) from the [561 report on all 34 routes](docs/exe/cpp-solo-entry-current-20261029.md).

## Latest C# + MonoGame clone

The existing runtime path can play Campaign **1-1 The War Begins!** and **1-2 Master of Whirligigs**. A new C# client (`Netstorm.Client`) is being built to follow cpppj's architecture. Its rendering is moving to an 8-bit logical screen and palette, with MonoGame responsible for the host and final output. See the existing [Campaign 1-1](docs/gameplay/campaign-one.md)/[1-2](docs/gameplay/campaign-two.md) implementation notes and the [latest handover](LEFT_JOBS.dotnetpj.md).

R2d-2 (October 29, 2026) added edition/display data and `RawShapeToBuffer`, following earlier drawing primitives and screen-capture support. The latest Release test run had zero warnings/errors and **1,500 passing tests** (Assets 363, Core 1,030, Client 107). This includes existing code and new module-level checks; it does not mean the new screen or world path is connected to live gameplay. `SpriteDraw` is next, followed by Gump UI and menus. Playability and parity remain limited to the approximate existing 1-1/1-2 path. See the [cpppj → dotnetpj porting plan](docs/dotnet-cpp-port-plan-20261016.md) and [handover](LEFT_JOBS.dotnetpj.md) for scope and sequence.

## Latest HTML + JavaScript web clone

Because browsers block audio until a click, the web clone loads the original assets, shows a "click to start" screen and then displays the main menu. The Edit button has been removed, and Campaign, Demo and Credits dialogs/lists are connected. The October 28 WB03-D baseline has 34 passing automated tests. Main-menu and dialog layout/input/sound are being reconstructed; Help, Options and other destination screens, as well as gameplay, are not implemented yet. Custom-map Edit and multiplayer are excluded from the web goals. See the [latest handover](LEFT_JOBS.webpj.md).

The linked analysis and handover documents are written in Korean.

# Project layout and development goals

`cpppj` proceeds in this order: **① version 10.78 single-player reconstruction → ② stable Windows 10/11 execution (including mandatory DirectDraw error fixes) → ③ TCP/IP LAN multiplayer and outposts → ④ Korean support and MCP → ⑤ additional requirements**. It is Windows only. Multiplayer-only outposts belong to phase 3; official-server access is not restored. Comparison records for CD version 10.72 are also retained.

`dotnetpj` uses C# and MonoGame. The existing clone receives the parts of `cpppj` already verified against original machine code, one area at a time, to match the original behavior first (October 16 porting plan). Once `cpppj` is complete, everything is compared again before new requirements are added. GUI Linux support and multiplayer have lower priority. Final requirements include fullscreen, 16:9/16:10/4:3 aspect ratios, fullscreen edge scrolling, and English/Korean support. Animation first targets the original cadence, with 60/120-frame support afterwards.

`webpj` uses HTML and JavaScript. Once `cpppj` is complete, it will be analyzed to develop the web clone toward original behavior. Custom maps (Edit), multiplayer and features that do not fit the web (quitting the game, resolution and fullscreen settings, and so on) are excluded, and required assets are copied into `webpj/www/`.

| Path | Purpose |
| --- | --- |
| `cpppj/` | C++20/Win32 reconstruction and tests |
| `dotnetpj/` | C#/MonoGame DesktopGL clone and tests |
| `webpj/` | HTML + JavaScript web clone (`www/`), static web server, tests and asset preparation tools |
| `assets/game-data/` | Clone graphics, maps, missions, music and sound effects |
| `fonts/` | D2Coding fonts for Korean text |
| `docs/` | Analysis evidence, reconstruction, build, gameplay and verification notes |
| `tools/`, `analyzeManager/` | Static analysis, original-game observation and verification tools |
| `originals/`, `originalCD/` | Original version 10.78 and CD version 10.72 analysis material |
| `original1037/`, `originalPatches/` | Original version 10.37 comparison material and the 10.37 → 10.62 patch (NSP1062) |
| `extracted/` | Refined decompilation output and raw build/test logs. Not in Git, so present only on the PC that produced it. |
| `screenShots/`, `playingVideos/`, `originalFanGame/` | Screenshot, recording and fan-game references. Large recordings and fan-game code are excluded from Git and may be absent on a given PC. |

Korean building and unit names follow the [shared naming policy](docs/korean-unit-names.md). Priest, Temple, Bridge, Thrower, Geyser, Altar, Residence and Empty may be translated; other names use Korean phonetic spellings, including Workshop → 워크샾 and Generator → 제네레이터. The dotnetpj menus and unit titles use the shared 55-entry table. Full help and mission-text translation remains unfinished.

Read [AGENTS.md](AGENTS.md) and each project's handover ([cpppj/shared](LEFT_JOBS.md), [dotnetpj](LEFT_JOBS.dotnetpj.md), [webpj](LEFT_JOBS.webpj.md)) before working. Original-game and separate-window execution follows the [execution-confirmation rules](docs/execution-confirmation.md). Original analysis files are protected; changes to clone data belong in `assets/game-data/`.

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

Create the distributable zip with the script below. If you omit the output path it prompts for one, and an empty answer cancels. By default the receiving PC must have the .NET 10 runtime installed; add `-SelfContained` to bundle the runtime. It publishes to a temporary folder, so nothing is left in `dist/`.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File dotnetpj\package-dotnetpj.ps1 D:\dist\NetstormReborn-dotnetpj.zip
```

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

You need Windows 10/11, CMake 3.21 or later, and a C++20 compiler (Visual Studio 2022 or later, or the C++ tools in Build Tools). This build uses the Win32 API directly and is Windows only. Run from the repository root:

```sh
cmake -S cpppj -B cpppj/build -DCMAKE_BUILD_TYPE=Release
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build --build-config Release --output-on-failure
```

CTest registers a single wrapper test and currently has 3,427 registered C++ tests. A full run takes several minutes; see the [handover](LEFT_JOBS.md) for the distinction between each unit's focused checks and full-suite results. With the Visual Studio generator, the executable is `cpppj/build/bin/Release/NetstormCpp.exe`. Open the reconstruction's main menu in windowed mode with:

```powershell
.\cpppj\build\bin\Release\NetstormCpp.exe --run originals --window
```

`--run <game-data folder>` runs the C++ reconstruction. Adding `--mission <mission key>` skips the menu and opens that mission's briefing (for example, `--mission Tutorial1`). `--view fonts`, `--view types`, and `--view TEST01` open font, type and mission inspection scenes. Running without arguments (or with `--help`) prints build information and the available options. Live construction, production, bridges, harvesting/transport, combat and automatic execution of the shared strategy AI are connected, while natural playthroughs of all target missions, remaining Edit tools, scenarios and lifetime transitions, and original-parity verification remain unfinished. See [C++ build](docs/cpp-build.md) for build instructions and the [handover](LEFT_JOBS.md) for current integration status.

Build a distributable archive (executable, Visual C++ runtime DLLs, original 10.78 game data and launcher batch files) with the script below. If you omit the output path it prompts for one, and an empty answer cancels. See section 7 of [C++ build](docs/cpp-build.md) for the layout and options. Units 562–563 exercised the default incremental build-to-ZIP path in Windows PowerShell 5.1. Unit 563 also checked fresh extraction, saved fullscreen restart and the windowed launcher; complete mission/parity and Windows 11 distribution validation remain separate.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File cpppj\package-cpppj.ps1 D:\dist\NetstormReborn-cpppj.zip
```

# Running the web clone (HTML + JavaScript)

Node.js is required (verified with v24); no external npm packages are used. The required assets are committed in `webpj/www/data/`. From the repository root, start the static web server and open `http://localhost:9803/` in a browser:

```sh
npm --prefix webpj start
```

Run the automated tests with `npm --prefix webpj test`. If the original data in `assets/game-data/` changes, rebuild `webpj/www/data/` with `npm --prefix webpj run prepare-assets` and check the copies with `npm --prefix webpj run check-assets`.

# Key documents

- [C++/shared handover](LEFT_JOBS.md) / [C# handover](LEFT_JOBS.dotnetpj.md) / [web handover](LEFT_JOBS.webpj.md): current state and upcoming work. The cpppj per-unit history up to October 25 is in [LEFT_JOBS.old.md](LEFT_JOBS.old.md).
- [TODO.md](TODO.md): the current cpppj goal prompt (Early Missions, Priest Training, campaigns, demos, Edit, menus and in-game UI)
- [C++ roadmap](docs/cpp-roadmap.md) / [playable reconstruction plan](docs/cpp-playable-plan.md): development stages and reconstruction plans
- [cpppj → dotnetpj porting plan](docs/dotnet-cpp-port-plan-20261016.md): porting units and order for matching dotnetpj to the original
- [Clone game data](assets/README.md): asset layout, provenance and build inclusion
- [Original-game/separate-window execution rules](docs/execution-confirmation.md): execution conditions for AI work

# License

MIT License
Copyright (c) 2026 HJOW
