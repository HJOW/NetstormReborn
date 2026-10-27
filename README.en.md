Netstorm - Reborn
-------------------------------------------------------------------------

[한국어](README.md) | **English**

# Overview
This project uses AI to revive
Netstorm - Islands at War,
a game developed by Activision in 1997 and abandoned long ago.

# Current development status

**cpppj reflects the October 27, 2026 handover (unit 483); dotnetpj reflects October 23 (W03 part 17, unchanged since); webpj reflects October 23 (WB03-M1) plus the October 27 goal change.** Playability and fidelity to the original are assessed separately.

| Project | Execution and playability | Fidelity and remaining work |
| --- | --- | --- |
| cpppj — C++ reconstruction | In Windows windowed mode, the original main menu (including Credits and Quit), briefings, guidance, construction, production, bridge placement/rotation, harvesting/transport, combat, the in-game UI (left deck, minimap, QWASZX shortcuts, ESC menu) and automatic execution of the shared enemy, allied and demo AI are connected. All 31 Early Missions, Priest Training and campaign missions, plus the three demos, can be entered and started from the menu. Early Missions 1–2 have been played through to the next tutorial, and Campaign 1-1 has reached destruction of the enemy temple. In the Edit menu, entering the editing screen for an existing map, Set All Bridge, Save as and returning to the main menu after saving have been checked in the real GUI. | Based on decompilation and machine-code comparisons against version 10.78. Natural playthroughs from Early Mission 3 onward and of the campaigns and Priest Training, priest capture/sacrifice, outcomes, progress saving, demo looping, and the Edit menu's terrain/island/object/owner tools, the Add Island window in the real GUI, test battles and custom scenarios remain. Complete visual, audio and input parity is also unfinished. |
| dotnetpj — C# + MonoGame clone | Campaign **1-1 The War Begins!** and **1-2 Master of Whirligigs** have playable implementations with construction, production, movement and combat controls. English/Korean UI, help, context menus and options are available. Early Missions and demos cannot be opened from the menu yet. | Original-verified rules are being ported. In addition to session-connected mission rules, original world-core modules for creation, graphs, destruction lifecycles, damage, Form lifetimes and the SP store/rewards have been verified. **The new world core is not yet connected to live sessions.** Live construction, combat, pathing, SP, AI and timing still use observation-based approximations. Natural completion → progress save → restart → next-mission unlock and original parity require further verification. |
| webpj — HTML + JavaScript web clone | Asset loading → "click to start" → the original main menu screen is implemented. The screens opened by the menu buttons and gameplay do not exist yet. The custom-map Edit mode has been excluded from the web clone's goals since October 27, 2026. | Main menu button layout, stone button texture, input, sound effects, music and cursor follow the original decompilation and were compared with screenshots. Removing the Edit button that remains in the code's main menu and recalculating the button layout is follow-up work. Full game implementation will follow the completion of cpppj. |

## Latest C++ reconstruction

The current target is to make **the 6 Early Missions, 9 Priest Training stages, all 16 campaign missions and 3 demos playable with version 10.78 behavior**, and to implement **the Edit menu (including custom maps and scenarios)**, every main-menu item except Multiplayer, and the in-game UI (left deck, lower-left minimap, QWASZX bridge shortcuts and ESC menu). See the [handover](LEFT_JOBS.md) for the current state and upcoming work. Detailed per-unit history up to October 25 has moved to [LEFT_JOBS.old.md](LEFT_JOBS.old.md).

[Early Mission 1](docs/exe/cpp-tutorial-harvest-20261019.md) and [Early Mission 2](docs/exe/cpp-tutorial-two-runtime-20261019.md) have been observed completing through normal input, including construction, harvesting/production, completion records and entry into the next tutorial. Entering and starting all 31 regular missions and the 3 demos from the menu also passes. In a session with the AI running, Campaign 1-1 has reached [destruction of the enemy temple with a real cannon](docs/exe/cpp-forward-ai-assault-20261022.md), but enemy priest capture, sacrifice and victory have not been verified. Mission entry and short-run checks are kept separate from playthrough evidence.

The [shared strategy AI](docs/exe/cpp-ai-automatic-session-20261022.md) has been reconstructed so that enemy, allied and demo local owners use the same original strategy, and it is connected to the live session frame. Normal missions leave the local owner under human control; demos run that owner through the same AI. Whether expansion, harvesting, production and attacks at normal action intervals match the original still needs comparison through natural play.

For the in-game UI, the original HUD root/docking, the [minimap](docs/exe/cpp-minimap-20261023.md), the production deck with [QWASZX shortcuts](docs/exe/cpp-production-shortcuts-20261023.md) and the [horizontal ESC menu](docs/exe/cpp-game-menu-runtime-20261023.md) are connected in live missions. Wide dock dragging, chat input and pixel-level parity verification remain.

The [13 focused Devastation/capture/sacrifice checks](docs/exe/cpp-ritual-runtime-focused-20261021.md) passed, but some use explicitly supplied caster, captive and clock states. Natural UI progression from obtaining the spell through casting, capture, transport and sacrifice still needs verification. Capture/sacrifice testing will proceed after a test option is added that applies the demo's allied AI to the player's side.

**The Edit menu's entry into existing maps and its saving flow are connected on the live screen.** From the map list and selection it enters the editing screen (HUD and ESC menu), and Set All Bridge, Save as (re-entering the list under the new name and restarting) and returning to the main menu with or without saving have been checked in the real GUI ([Edit menu integration](docs/exe/cpp-editor-menu-runtime-integration-20261026.md)). The [data and empty world of a new map](docs/exe/cpp-new-editor-runtime-20261027.md), the [Add Island window](docs/exe/cpp-editor-island-window-runtime-20261027.md) (21-cell candidate list, placement preview, collision checks and built-in description), the [test-battle command parent](docs/exe/cpp-editor-test-battle-parent-20261027.md) and the [login-reload lifetime of mission working-copy files](docs/exe/cpp-mission-reload-source-runtime-20261027.md) are connected and were verified with windowless integration tests. Adding islands and re-entering after saving in a real game window, the full set of terrain/island/object/owner editing tools, real input for test battles and the return to editing, and applying briefing/player/AI/starting-SP settings from language-specific scenario files remain. This goal was paused at unit 483 on October 27, 2026; the resume points are separating the world and Guide lifetimes in editing mode with the State7 round trip, and connecting ordinary input to test battles.

In the most recent full test run (unit 483, October 27), the Release build had **zero warnings and zero errors** and all **3,271 C++ tests passed with 0 failures**. That unit did not open a game window; natural Early Mission 2 progression last passed on the unit 467 build, and the real-GUI Edit save and re-entry last passed on the unit 468 build. Unit-test passes or mission entry alone are not treated as playthrough or parity evidence. DirectDraw error fixes and stable Windows 10/11 fullscreen operation are separate mandatory phase-2 work.

## Latest C# + MonoGame clone

The campaign menu reads chapters, prerequisite completion conditions and saved progress from the original scripts while restricting access to supported missions. Options include resolution, windowed/fullscreen mode, sound, volume, wind noise and speaker swap. Help and F1 expose the original help content, and priest, Workshop and production-item context menus are available. See the [feature/progress-saving review](docs/dotnet-review-20261011.md), [recording-analysis coverage](docs/screens/recorded-ui-clone-20261002.md) and [Campaign 1-1](docs/gameplay/campaign-one.md)/[1-2 implementation](docs/gameplay/campaign-two.md). The [latest handover](LEFT_JOBS.dotnetpj.md) takes precedence over early implementation notes.

The common mission frame gate, player participation/directional alliances, cumulative build ledger and Tutorial 1 stage checks were ported from original-verified results and connected to the existing session. A separate original world core is being built: W02 covers slots/spatial hashing, Pop/Unpop, ownership/frames, bridges and graphs; W03 now includes destruction lifecycles, release, general damage, [priest damage/HP changes](docs/dotnet-original-core-priestdamage-20261021.md), [contained-object lookup/deletion](docs/dotnet-original-core-containedlookup-20261021.md), [Form lifetimes](docs/dotnet-original-core-formlifecycle-20261022.md), Form packets, content lifetimes, temple pre-destruction and asset virtual deletion, and the W05 precursor covers the [SP store and destruction rewards](docs/dotnet-original-core-spstore-reward-20261023.md), all verified at module level. There are **61 linked fixtures**. The latest recorded baseline, W03 part 17 on October 23, has a full Release build with **zero warnings/errors** and **1,349 passing unit tests** (Assets 363, Core 986). These are module verification results and do not expand live playability.

Next is assembling the destruction lifecycle (selection, cursor, Graph, ledgers and the Kernel process host) and connecting it to live sessions so that mission starting states match the original. Kernel/Regular timing and SP/random-number handling follow, then pathing, construction, combat, sacrifice, outcomes, rendering, UI and sound. Although cpppj now targets Early Missions, Priest Training, all campaigns, the demos and Edit, dotnetpj's current playable scope remains the two campaigns listed above. Areas still unfinished in cpppj, including strategy AI, remain explicitly tracked as approximate while the [cpppj → dotnetpj porting plan](docs/dotnet-cpp-port-plan-20261016.md) guides natural playthrough and original-parity verification.

## Latest HTML + JavaScript web clone

Browsers do not play sound until the page has been clicked once, so the web clone loads the original assets, shows a "click to start" screen and then displays the original main menu. The main menu follows the original decompilation for the button layout algorithm, stone button texture, press/release input, sound effects, music and cursor. Quit and the multiplayer entry, which cannot work on the web, are removed using the original empty-slot handling. Its 17 automated tests pass, and several screen sizes including 4:3, 16:10 and 16:9 were checked in a headless browser. The screens opened by the menu buttons (Campaign, Demo, Credits, Help, Options) and gameplay do not exist yet. Following the October 27, 2026 goal change, the custom-map Edit mode and user maps are excluded from the web clone; the code's main menu still has an Edit button, so removing it and recalculating the button layout is follow-up work. See the [latest handover](LEFT_JOBS.webpj.md).

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

CTest registers a single wrapper test, which runs more than 3,000 C++ tests (3,271 most recently) and takes several minutes. With the Visual Studio generator, the executable is `cpppj/build/bin/Release/NetstormCpp.exe`. Open the reconstruction's main menu in windowed mode with:

```powershell
.\cpppj\build\bin\Release\NetstormCpp.exe --run originals --window
```

`--run <game-data folder>` runs the C++ reconstruction. Adding `--mission <mission key>` skips the menu and opens that mission's briefing (for example, `--mission Tutorial1`). `--view fonts`, `--view types`, and `--view TEST01` open font, type and mission inspection scenes. Running without arguments (or with `--help`) prints build information and the available options. Live construction, production, bridges, harvesting/transport, combat and automatic execution of the shared strategy AI are connected, while natural playthroughs of all target missions, the Edit menu's remaining editing tools, test battles and custom scenarios, and original-parity verification remain unfinished. See [C++ build](docs/cpp-build.md) for build instructions and the [handover](LEFT_JOBS.md) for current integration status.

Build a distributable archive (executable, Visual C++ runtime DLLs, original 10.78 game data and launcher batch files) with the script below. If you omit the output path it prompts for one, and an empty answer cancels. See section 7 of [C++ build](docs/cpp-build.md) for the layout and options. The automatic build branch (running without `-NoBuild` so that building flows into packaging on the latest sources) has not been exercised yet.

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
