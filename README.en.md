Netstorm - Reborn
-------------------------------------------------------------------------

[한국어](README.md) | **English**

# Overview
This project uses AI to revive    
Netstorm - Islands at War,    
a game developed by Activision in 1997 and abandoned long ago.
    
    
The first priority is for the game to run without bugs on Windows 10 and 11 while offering an experience nearly identical to the original.
    
     
The revival work is split into three subprojects.
- cpppj - Developed in C++. Since the original game is believed to have been written in C++ as well, it follows the original game as closely as possible. Windows only.
- dotnetpj - Developed in C# with MonoGame. Ports the game to the .NET platform using the source code obtained through cpppj, supporting a variety of platforms.
- webpj - Developed in HTML/JavaScript so the campaigns can be enjoyed conveniently on the web. It supports every platform with a web browser, but some features will not be implemented.

# Current development status

**As of November 27, 2026:** the main line, `cpppj`, is product 1377, displayed as version 11.01; its latest distribution ZIP is unit 1378. It includes all 43 Japanese text files and 128 help topics, tutorial step instructions, a real TCP/IP opponent bot and existing-player recovery, and richer error context. Unit 1380 rechecked production, construction and recovery on the current executable and verified the distribution hashes. At the user's request, the current goal pauses after the documentation update and local commit.

| Project | Running and playing | Original parity and remaining scope |
| --- | --- | --- |
| **cpppj — C++ reconstruction** | **Playable.** Includes 31 regular missions and 3 demos, Edit/scenario editing, Skirmish, TCP/IP local multiplayer/Outposts, English/Korean/Japanese, widescreen, AA/FSR, difficulty, Reset All and local MCP. The six early missions display the actual next-step condition as text without a background at the lower right. | The latest full 34-route gate remains 1327 on product 1326; the Japanese gate is 1362. Later changes have focused tests and real GUI/TCP evidence. Production/construction, host takeover and existing-ID recovery passed real TCP tests. Physical LAN, natural playthroughs, full visual/audio parity and other hardware still need user checks. CD 10.72 and optional extensions remain deferred. |
| **dotnetpj — C# + MonoGame** | `Netstorm.Game`/`Netstorm.exe` is the only executable path. Menus, session loading, world collection/tiles and parts of the HUD are connected. The last completed unit is 1.079; the unfinished R5-4c3 code is preserved in checkpoint commit 1015ca4a6. Object rendering, input and rules remain; **it is not playable yet.** | Twelve menu scenes matched cpppj; mission-start parity is 0/34. The last completed regression gate, 1.076, passed 7,379 tests. The interrupted 1.080 work passed 7,385 tests but all three representative L2 runs exited 1. Rebuilding and recollecting after the Idle fix remain pending. Tutorial instructions are also required follow-up work. |
| **webpj — HTML + JavaScript** | **Playable in a browser.** Provides 31 missions, demos, Skirmish, English/Korean with Galmuri11, game menus/chat, production/construction, combat/AI, saved progress and WebMCP. The last product unit is WB04-E1380, paused at the user's request. | All 1,007 baseline port units are complete. Comparisons cover Tutorial1 starts at three aspect ratios, earlier Tutorial1–3 completion runs, allowed later-tutorial segments, and entry/waiting in all 16 campaign missions. Real allied-AI tests won 1-1/1-2/1-5 and checked progress saving/unlocking in 1-2/1-5. The latest Node 26 and 22 full runs each passed 6,867 tests. App HUD/State7 lifetime, remaining objectives/audio/distribution gates and the new instructions remain open. |

**Tutorial step instructions are required in all three projects.** cpppj has implemented and verified the allowed scope. dotnetpj and webpj each have four unfinished tasks: conditions/text, display, progression/lifetime, and verification/distribution. [Shared contract](docs/tutorial-objectives-porting-20261127.md), [cpppj handover](LEFT_JOBS.md), [dotnetpj handover](LEFT_JOBS.dotnetpj.md), [webpj handover](LEFT_JOBS.webpj.md).

## Latest C++ reconstruction status (cpppj)

- **Missions and controls:** 6 Early Missions, 9 Priest Training missions, 16 campaign missions and 3 demos, plus Edit/scenario editing and Skirmish. Original-style construction, production, harvesting/transport, combat/rituals, AI, HUD, menus, chat, context menus and edge scrolling are connected, with arrow-key camera movement added.
- **Tutorial instructions, 1371–1372:** 40 stages and 39 keys across six official missions, in English/Korean/Japanese, read the actual gate without changing progression. Lower-right text has no background, hides/restores around windows and menus, and is cleared on restart, transitions and leaving. All 22 focused tests and 19 real GUI paths passed with exit code 0. [Conditions](docs/exe/cpp-tutorial-objectives-1371-20261127.md), [implementation/screens](docs/exe/cpp-tutorial-objective-display-1372-20261127.md).
- **Languages:** Korean uses external Galmuri11; Japanese uses external Noto Sans JP. Japanese UI, 43 official text files and 128 help topics passed gate 1362. Each Korean/Japanese translation table now has 575 keys after the tutorial addition. Human language review and physical IME/clipboard checks remain.
- **LAN, 1364–1380:** real processes verified lobby/battle start, clocks, server takeover, temples/golems, workshops/bridges and existing-ID restart recovery. The latest run matched 7,305 shared objects, 51 locally derived surfaces and SP; all three processes exited 0. **The user's decision preserves the original scope: no new-ID joining during a battle.** Official-server access and retired client verification remain disabled. [Bot guide](docs/testing/cpp-lan-opponent-guide.md), [latest verification](docs/exe/cpp-original-lan-scope-1380-20261127.md).
- **User data and settings:** settings, progress, custom maps and logs are stored under `%USERPROFILE%\Saved Games\Netstorm\`. Includes scrolling Edit lists, default `.english` creation, scenario editing and confirmation-based Reset All for settings/progress.
- **Display and devices:** 16:9 up to 1920×1080, FSR 1.0 output at 2560×1440/3840×2160, optional AA, Frame Rate 60/75/120, and MCP input/PNG/audio-video capture. Difficulty, Skirmish and the QuickJS-ng engine are implemented. Further QuickJS APIs and flight interpolation remain deferred.
- **Presentation and logs:** includes 11.01 Patch Credits, HJOW/AI attribution and Copyright 2026 HJOW. Unit 1377 added display details and Regular SID/TYPE/frame/event context to user log files; 18/18 tests passed. [Log evidence](docs/exe/cpp-error-context-1377-20261127.md).
- **Latest distribution, 1378:** product 1377, 1,480 entries; CRC/source hashes, fonts/licenses, zero duplicates, and extracted Korean fullscreen/Japanese windowed GUI runs passed with exit code 0. Unit 1380 rechecked the same EXE/ZIP hashes and 2,810 unchanged protected files. [Distribution evidence](docs/exe/cpp-final-package-1378-20261127.md).

Explicit user checks, missing reproduction details and deferred/optional work remain. The [cpppj handover](LEFT_JOBS.md) records all 34 known-bug decisions, CD 10.72 deferred after its menus, Korean sharpness D8, further QuickJS integration and interpolation. Full gates, focused tests, runtime checks and human review are separate; full original parity is not declared complete.

## Latest C# + MonoGame status (dotnetpj)

The literal host `Netstorm.Game` uses `Netstorm.Client` for the 8-bit logical buffer, Gump UI and world session; MonoGame supplies windows, input, audio and presentation. R4 session assembly is 2/8 and R5 world display is 2/5. AI frames/temple deletion, world/input/lifetime, user data, world collection/tiles and parts of docking are connected. Units 1.077–1.079 fixed shared help, hiding menus during missions, and attaching the production-deck frame.

Next is the interrupted R5-4c3: rebuild/related tests, recover the three representative L2 runs, then check minimap pixels. Inventory, UberRoot panels, object/construction rendering, edge scrolling and 34-route L4a follow before R6 rules/input. DN-TUT-1–4 connect the new instructions to R5 display/R6 lifetime; CP LAN fixes remain DN-MP-02–08. Mission-start parity is still 0/34 and the game remains unplayable; CP verification does not count as DN completion. [DN handover](LEFT_JOBS.dotnetpj.md).

## Latest HTML + JavaScript status (webpj)

After “click to start”, the browser runs 31 missions, demos and Skirmish, with menus/chat, Korean scripts, Galmuri11, Reset All and localStorage progress. WebMCP verified real input, PNG and audio-inclusive WebM/downloads in supported Chromium; ordinary play in unsupported browsers was also checked. Edit, User Made Campaigns, multiplayer, the CD intro, resolution/fullscreen settings and Quit are outside the web scope.

Through E1380, the work split world-independent HUD/production/shared-menu/Chat creation, fixed the Korean Campaign 1-2 AI-name overflow and established the allocator input behind a one-pixel construction difference. The app HUD/world-docking and State7 draft is preserved but unapplied. On resumption, verify it, then implement WB-TUT-1–4 and continue remaining 1-3/1-4/Skirmish objectives and M4/M5. Completing the 1,007 baseline port units does not close the nine changed CP rows and ten new files awaiting review. Later Tutorial4/5/6 capture, sacrifice, specific-enemy attack and victory transitions remain user checks. [WB handover](LEFT_JOBS.webpj.md), [WebMCP](docs/webpj-webmcp-20261126.md).

# Project layout and development goals

`cpppj` proceeds in this order: **① version 10.78 single-player reconstruction → ② stable operation on Windows 10/11 (including the DirectDraw error fix) → ③ TCP/IP local-network multiplayer and Outposts → ④–⑮ additional requirements**. The additional requirements are:

- **Phase 4:** local MCP (all in-game controls, screen capture and recording with sound, for analysis while developing dotnetpj) and 16:9 resolutions up to 1920×1080 (no graphics scaling needed)
- **Phase 5:** move settings and campaign progress to the user's home folder (`Saved Games\Netstorm\Saves\`), and move every file and folder the game creates automatically under `Saved Games\Netstorm\` so that the game works even when installed where files cannot be written without administrator rights. Custom maps are loaded from both the existing folder and `Saved Games\Netstorm\CustomMaps\` (new maps from Edit are saved to the latter), and the Edit map list gains scrolling with no display-count limit.
- **Phase 6:** Korean support
- **Phase 7:** a common structure for error/exception handling and logging. A detailed log from just before an abnormal exit is written to `Saved Games\Netstorm\logs\yyyyMMddHHmmss.log`.
- **Phase 8:** anti-aliasing and FSR 1.0 in Main Menu → Options (off by default). Where FSR is supported, 2560×1440 and 3840×2160 can be selected; FSR is then turned on automatically and an effective 1920×1080 image is upscaled to the target resolution.
- **Phase 9:** a dedicated Edit UI for editing not only maps and placed elements but the scenario itself (the player's starting funds and tech, enemy composition, alliances and so on), plus automatic creation of a default `.english` file for new maps (fixing the exit that occurs when entering Test Battle without an `.english` file)
- **Phase 10:** verifying and fixing the bugs known in the original 10.78
- **Phase 11:** difficulty (Easy, Normal, Hard). On Easy the enemy AI's bridge-building and unit-production cycle is four times longer and it produces half as many resource gatherers; on Hard the cycle is twice as fast.
- **Phase 12:** Skirmish mode: play against any number of AIs on randomly placed islands as in multiplayer, choosing each AI's difficulty and alliance and the banned units.
- **Phase 13:** other languages (Japanese)
- **Phase 14:** embedding the QuickJS engine (how far to integrate it is decided later)
- **Phase 15:** settings reset. A Reset All entry at the bottom of Options asks for confirmation once more, then resets all settings including `options.cfg` and the game progress, and quits the game.

`cpppj` has closed implementation of phases 1–15 for 10.78 (phase 14 covers the engine only). Japanese UI, all 43 official text files and 128 help topics were completed through 1362, followed by tutorial instructions, real TCP verification and log improvements. Natural playthroughs, full parity, physical LAN and other environments remain user checks. It targets Windows x86-64 with Windows 11 ARM considered; official-server access stays disabled. CD 10.72 remains deferred after its menus.

`dotnetpj` uses C# and MonoGame. It began by moving the parts verified by `cpppj` into the existing clone piece by piece (the October 16 porting plan), but because much of the existing implementation differs from the original, the plan was redone on October 28. The former approximate executable was removed on November 22 and the sole `Netstorm.Game` runtime uses `Netstorm.Client` that carries over cpppj's structure (8-bit screen, Gump UI, world session), with the cpppj executable serving as the comparison reference for identical inputs. On November 15, 2026 the remaining work was reorganized, by comparison against the entire cpppj source, into checklists for stages R4–R11. Matching the original behavior comes first; cpppj's phase 4–15 goals are then added as later requirements. The Windows executable is named `Netstorm.exe`. GUI Linux support (saving under `$HOME/.netstorm/`) and multiplayer have lower priority. Final requirements include fullscreen, 16:9/16:10/4:3 aspect ratios, fullscreen edge scrolling, and English/Korean support. Animation first targets the original cadence, with 60/120-frame support afterwards.

`webpj` uses HTML and JavaScript. Aiming at the original behavior, it ports the single-player scope of `cpppj` file by file: a pinned cpppj baseline is ported first and later changes are judged and re-synced separately. Custom maps (Edit), User Made Campaigns, multiplayer and features that do not fit the web (quitting the game, resolution and fullscreen settings, and so on) are excluded, and required assets are copied into `webpj/www/`. Of cpppj's phase 4–15 goals, those that apply to the web are later requirements (phase 4 as WebMCP, phase 5 with localStorage, phase 7 as browser console output, phase 15 reloading instead of quitting, and including phase 12 Skirmish); phases 8, 9 and 14 are excluded.

| Path | Purpose |
| --- | --- |
| `cpppj/` | C++20/Win32 reconstruction and tests (main line) |
| `dotnetpj/` | C#/MonoGame DesktopGL clone and tests |
| `webpj/` | HTML + JavaScript web clone (`www/`), static web server, tests and asset preparation tools |
| `assets/game-data/` | Clone graphics, maps, missions, music and sound effects |
| `fonts/` | Galmuri11 for Korean and Noto Sans fonts for other languages. dotnetpj packages external Galmuri11 files; screen wiring is pending. |
| `docs/` | Analysis evidence, reconstruction, build, gameplay and verification notes |
| `tools/`, `analyzeManager/` | Static analysis, original-game observation and verification tools |
| `originals/`, `originalCD/` | Original version 10.78 and CD version 10.72 analysis material |
| `original1037/`, `originalPatches/` | Original version 10.37 comparison material and the 10.37 → 10.62 patch (NSP1062) |
| `extracted/` | Refined decompilation output and raw build/test logs. Not in Git, so present only on the PC that produced it. |
| `screenShots/`, `playingVideos/`, `originalFanGame/` | Screenshot, recording and fan-game references. Large recordings and fan-game code are excluded from Git and may be absent on a given PC. |

Korean building and unit names follow the [shared naming policy](docs/korean-unit-names.md). Priest, Temple, Bridge, Thrower, Geyser, Altar, Residence and Empty may be translated; other names use Korean phonetic spellings, including Workshop → 워크샾 and Generator → 제네레이터. cpppj's translation table (`xlat.korean`, 575 entries) and dotnetpj's menus and unit titles (the shared 55-entry table) use the same names. cpppj and webpj have translated their menus, help and mission scripts into Korean (a full human review of the text is left to the user); the Korean screens of dotnetpj's new path are wired up in R8 phase 6.

Read [AGENTS.md](AGENTS.md) and each project's handover ([cpppj/shared](LEFT_JOBS.md), [dotnetpj](LEFT_JOBS.dotnetpj.md), [webpj](LEFT_JOBS.webpj.md)) before working. Original-game and separate-window execution follows the [execution-confirmation rules](docs/execution-confirmation.md). Original analysis files are protected; changes to clone data belong in `assets/game-data/`.

# Preparing the development environment

The root preparation scripts let you select tools to install or inspect the current environment. To check without installing:

```powershell
powershell -ExecutionPolicy Bypass -File .\PREPARE.ps1 -CheckOnly
```

```sh
bash ./PREPARE.sh --check-only
```

Omit `-CheckOnly` or `--check-only` to open tool selection and installation. REA is optional and unselected by default, and uses an existing Node.js/npm installation. Preparing Ghidra and REA together connects `GHIDRA_INSTALL_DIR`. None of the three builds requires running the original game or installing REA.

# C++ build and run (cpppj, main line)

You need Windows 10/11, CMake 3.21 or later, and a C++20 compiler (Visual Studio 2022 or later, or the C++ tools in Build Tools). This build uses the Win32 API directly and is Windows only. Run from the repository root:

Run this script to choose **game / combined tests / everything** from a menu. An empty answer cancels.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\cpppj\build-cpppj.ps1
```

| Choice | Arguments to append | What gets compiled |
| --- | --- | --- |
| Game (recommended) | `-Mode Game` | Game executable and required shared code; no test sources |
| Combined tests | `-Mode Tests` | `netstorm_tests`, required shared code and helpers; skips duplicate compilation in separate focused test targets |
| Everything | `-Mode All` | The existing full build: game, combined tests, every focused test target and helper |

For example, build just the game or a single focused test target with the following commands. Builds are incremental. The script compiles without launching the game or running tests.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\cpppj\build-cpppj.ps1 -Mode Game
powershell -NoProfile -ExecutionPolicy Bypass -File .\cpppj\build-cpppj.ps1 -Mode Tests -TestTarget netstorm_editor_file_ring_tests
```

Defaults are Release and four parallel jobs. Use `-Configuration Debug`, `-Jobs 2` or `-BuildDirectory cpppj/build-other` to change them; `-DryRun` only prints commands. To skip the menu, use `-Mode Tests` for combined tests or `-Mode All` for the existing full build.

Equivalent direct CMake commands follow. **Tests default to OFF in a new build directory.** An existing cached ON value is preserved unless explicitly changed.

```powershell
# Game only
cmake -S cpppj -B cpppj/build -DCMAKE_BUILD_TYPE=Release -DNETSTORM_BUILD_TESTS=OFF
cmake --build cpppj/build --config Release --target NetstormCpp --parallel 4
# Combined tests only (also compiles required shared code)
cmake -S cpppj -B cpppj/build -DCMAKE_BUILD_TYPE=Release -DNETSTORM_BUILD_TESTS=ON
cmake --build cpppj/build --config Release --target netstorm_tests --parallel 4
# Existing full build (after configuring with tests ON, omit the target)
cmake -S cpppj -B cpppj/build -DCMAKE_BUILD_TYPE=Release -DNETSTORM_BUILD_TESTS=ON
cmake --build cpppj/build --config Release --parallel 4
# Optionally run combined tests separately from compilation
ctest --test-dir cpppj/build --build-config Release --output-on-failure
```

Run CTest after building the combined tests. See the [handover](LEFT_JOBS.md) for the distinction between each unit's focused checks and full-suite results. With the Visual Studio generator, the game executable is `cpppj/build/bin/Release/Netstorm.exe`. Open the reconstruction's main menu in windowed mode with the command below. Choosing Campaign in the main menu and then the first mission of chapter 1, **The War Begins**, starts Campaign 1-1.

```powershell
.\cpppj\build\bin\Release\Netstorm.exe --run originals --window
```

`--run <game-data folder>` runs the C++ reconstruction (it does not run the original game). Adding `--mission <mission key>` skips the menu and opens that mission's briefing (for example, `--mission Tutorial1`; Campaign 1-1 is `--mission TheWarBegins`). `--view fonts`, `--view types`, and `--view TEST01` open font, type and mission inspection scenes. Running without arguments opens the GUI main menu; `--help` prints build information and the available options. For Korean, choose it in Options > Language or start with `--set currentLanguage=korean`. Adding `--mcp` runs the game as a local MCP server (stdio); see the [MCP record](docs/exe/cpp-mcp-server-20261106.md) for registration and the tool list. Live construction, production, bridges, harvesting/transport, combat and automatic execution of the shared strategy AI are connected, while natural playthroughs of every mission and full original-parity verification are left for user verification. See [C++ build](docs/cpp-build.md) for build instructions and the [handover](LEFT_JOBS.md) for current integration status.

## Building and running the distributable archive

Use the script below to create a distributable archive containing the executables, Visual C++ runtime, game data, Korean/Japanese translations, external fonts/licenses, QuickJS/FSR licenses and launchers. Omitting the output path prompts for one; an empty answer cancels. See [C++ build, section 7](docs/cpp-build.md) for layout and options.

On the receiving PC, extract the archive and run `Netstorm.exe`, `Netstorm-SavedMode.bat` (saved display mode) or `Netstorm.bat` (forced windowed mode). Settings, progress and captures are written under `%USERPROFILE%\Saved Games\Netstorm\`, allowing a read-only install folder. Use Reset All in Options to reset settings/progress.

The latest ZIP, unit 1378, contains product 1377 and 1,480 entries. CRC/source hashes, languages/fonts/licenses, two real extracted GUI runs and unchanged distribution files after execution were verified. Unit 1380 rechecked the current EXE/ZIP hashes. The full 34-route gate 1327 remains evidence for its product 1326; Windows 11 and other physical environments still need user checks. [Latest distribution evidence](docs/exe/cpp-final-package-1378-20261127.md).

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File cpppj\package-cpppj.ps1 D:\dist\NetstormReborn-cpppj.zip
```

# C# + MonoGame clone build and run (dotnetpj, in development)

Run the commands below from the project root (`NetstormReborn`). The game is written in C# with MonoGame DesktopGL, and the target framework is `net10.0`. The executable project is `Netstorm.Game` (output `Netstorm.exe`); it currently runs up to the menus and development mission loading, and world gameplay is not available yet.

## Prerequisites

- Install the **.NET 10 SDK** and check it with `dotnet --version`. `dotnetpj/global.json` accepts the latest feature band of the .NET 10 SDK, 10.0.100 or later.
- The first build or publish needs an internet connection to download NuGet packages and the runtime for the target OS.
- Check out `assets/game-data/`, the regular and bold Galmuri11 TTFs and `fonts/Galmuri11-LICENSE.txt` together with the repository. The data and fonts are copied to build and publish output automatically.
- The current run/verification target is **Windows 10/11 x64**; GUI Linux support has lower priority. A graphics driver with OpenGL support is required.

The dotnetpj build, tests and runtime use the separate `assets/game-data/` and do not require the original executable. Keep `originals/` separately as protected analysis material. For the layout of the assets and the migration record, see [Clone game data](assets/README.md).

## Development build and run

```sh
dotnet restore dotnetpj/Netstorm.sln
dotnet build dotnetpj/Netstorm.sln -c Release --no-restore
dotnet test dotnetpj/Netstorm.sln -c Release --no-build
dotnet run --project dotnetpj/src/Netstorm.Game -c Release --no-build -- --window --set "currentLanguage=english"
```

Use `--set "currentLanguage=english"` for the current English path. Korean screen wiring is pending; external Galmuri11 files are packaged. Settings currently use `game-data/d/options.cfg`; moving writes to the user home is pending. Build output goes to `dotnetpj/src/Netstorm.Game/bin/Release/net10.0/`. A normal build produces an executable for the OS it was built on; to build binaries for another OS, use the `-r` option shown below.

## Distributable binaries with the runtime included

Run `dotnet publish` on the game project. The following commands work in both Windows PowerShell and a Linux shell, and you can build the Windows exe on Linux as well.

```sh
dotnet publish dotnetpj/src/Netstorm.Game/Netstorm.Game.csproj -c Release -r win-x64 --self-contained true -o dist/win-x64
```

| Target | Output folder | Executable |
| --- | --- | --- |
| Windows 10/11 x64 | `dist/win-x64/` | `Netstorm.exe` |

`--self-contained true` bundles the .NET runtime, so the PC that runs the game does not need the .NET SDK or runtime installed. The SDL2 and OpenAL native libraries, `fonts/`, and `game-data/` are also included in the output. The executable uses the DLLs, runtime files, font, and game data generated alongside it, so **distribute the whole output folder**. The OS environment, such as the graphics driver and a GUI on Linux, is still required separately.

If the .NET 10 runtime is already installed on the PC that will run the game, you can make a smaller package. Change `--self-contained true` to `--self-contained false` in the commands above, and use a separate output folder such as `dist/win-x64-fdd` or `dist/linux-x64-fdd`.

Create the distributable zip with the script below. If you omit the output path it prompts for one, and an empty answer cancels. By default the receiving PC must have the .NET 10 runtime installed; add `-SelfContained` to bundle the runtime. It publishes to a temporary folder, so nothing is left in `dist/`.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File dotnetpj\package-dotnetpj.ps1 D:\dist\NetstormReborn-dotnetpj.zip
```

## Bundled game data and running the binaries

On build and publish, `assets/game-data/` is **copied automatically to `game-data/`**. The current Windows distribution layout is:

```text
dist/win-x64/
  Netstorm.exe
  Netstorm.dll
  ... (DLLs, runtime, and configuration files generated alongside)
  fonts/
    Galmuri11Bitmap-Regular-2.40.4.ttf
    Galmuri11Bitmap-Bold-2.40.4.ttf
    Galmuri11-LICENSE.txt
  game-data/
    netstorm.tarc
    d/
    music/
    sound/
```

Run in Windows PowerShell:

```powershell
.\dist\win-x64\Netstorm.exe --window --set "currentLanguage=english"
```

Run on Linux (not verified before the R10 Linux stage):

```sh
chmod +x dist/linux-x64/Netstorm
./dist/linux-x64/Netstorm --window --set "currentLanguage=english"
```

A normal run finds the bundled data automatically. To use data from another location for development or verification, set `NETSTORM_DATA` to the **absolute path of the folder that contains `netstorm.tarc`**. The search order is a valid `NETSTORM_DATA`, then `game-data/` or `assets/game-data/` in the executable folder, the working folder, and their parent folders.

```powershell
$env:NETSTORM_DATA = "D:\Games\NetstormReborn\game-data"
.\dist\win-x64\Netstorm.exe --window --set "currentLanguage=english"
```

```sh
NETSTORM_DATA="/absolute/path/game-data" ./dist/linux-x64/Netstorm --window --set "currentLanguage=english"
```

# Running the web clone (HTML + JavaScript)

Use Node.js 22.13+ within the 22.x line, or Node.js 24+. Install build and test dependencies with `npm --prefix webpj ci`. The required assets are committed in `webpj/www/data/`. From the repository root, start the static web server and open `http://localhost:9803/` in a browser:

```sh
npm --prefix webpj start
```

Run ESLint with `npm --prefix webpj run lint`, production builds with `npm --prefix webpj run build`, and development builds with `npm --prefix webpj run build:dev`. Before generating output, webpack waits for the full ESLint check; errors or warnings stop the build and preserve the existing bundle. Automatic regeneration and direct webpack calls use the same check. See the [configuration and existing-code exception policy](docs/webpj-eslint-20261127.md).

Choose and save English, Korean or Japanese in Options ▸ Language. Japanese fonts use Google Fonts CDN; failed loading falls back to English. See [Japanese support and validation](docs/webpj-japanese-browser-support-20261127.md) and [external access](docs/webpj-external-access-20261105.md).

Run the automated tests with `npm --prefix webpj test`. If the original data in `assets/game-data/` changes, rebuild `webpj/www/data/` with `npm --prefix webpj run prepare-assets` and check the copies with `npm --prefix webpj run check-assets`. For development you can combine the URL parameters `?lang=ko` (Korean) or `?lang=ja` (Japanese), `?mute` (no sound) and `?aspect=16:9` (aspect ratio); `?map=savetheisland` opens a development view that shows only the terrain, without game rules.

# Key documents

- [C++/shared handover](LEFT_JOBS.md) / [C# handover](LEFT_JOBS.dotnetpj.md) / [web handover](LEFT_JOBS.webpj.md): current state and upcoming work. Detailed history of earlier units is in [LEFT_JOBS.old.md](LEFT_JOBS.old.md) (cpppj/shared), [LEFT_JOBS.dotnetpj.old.md](LEFT_JOBS.dotnetpj.old.md) (dotnetpj) and [LEFT_JOBS.webpj.old.md](LEFT_JOBS.webpj.old.md) (webpj).
- [cpppj M8 final distribution gate report](docs/exe/cpp-m8-final-gate-1327-20261122.md): build, ZIP, distribution run and external-access audit results for product 1326 (the earlier product 1310 gate is in the [1311 report](docs/exe/cpp-m8-final-gate-1310-20261114.md))
- `tools/port_inventory.py`: a tool that tallies, per area, how much of the cpppj source remains to be ported to dotnetpj and webpj (`python -X utf8 tools/port_inventory.py`). It regenerates the per-stage remaining-work tables in the two handovers.
- [TODO.md](TODO.md): the goal prompts for cpppj and dotnetpj (instructions to carry each handover's work through to the end, and which tests to skip)
- [C++ roadmap](docs/cpp-roadmap.md) / [playable reconstruction plan](docs/cpp-playable-plan.md): development stages and reconstruction plans
- [cpppj → dotnetpj porting plan](docs/dotnet-cpp-port-plan-20261016.md): porting units (W numbers) and the evidence list. The current order (R0–R11) and stage checklists in section 1 of the [dotnetpj handover](LEFT_JOBS.dotnetpj.md) take precedence.
- [Clone game data](assets/README.md): asset layout, provenance and build inclusion
- [Original-game/separate-window execution rules](docs/execution-confirmation.md): execution conditions for AI work

# License

MIT License
Copyright (c) 2026 HJOW
