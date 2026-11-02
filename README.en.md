Netstorm - Reborn
-------------------------------------------------------------------------

[한국어](README.md) | **English**

# Overview
This project uses AI to revive
Netstorm - Islands at War,
a game developed by Activision in 1997 and abandoned long ago.

Three versions are developed in parallel, and **the one you can actually play today is `cpppj`, the C++ reconstruction**. It is rebuilt by comparing against original 10.78 at the machine-code level; every mission (34 paths) can be entered, and Campaign 1-1 can be played through to the end at nearly the same level as the original. `dotnetpj` (C# + MonoGame) and `webpj` (HTML + JavaScript) follow, building on what `cpppj` has verified.

# Current development status

**November 3, 2026, cpppj unit758 / runtime product758·11.01:** World knowledge cards and the Storm/World selector now use the original HTML command queue and current Session. All 4 focused tests pass. No new game or multiplayer tests were run. The overall goal remains incomplete. See the [handover](LEFT_JOBS.md) and [758 record](docs/exe/cpp-knowledge-world-menu-20261103.md).

| Project | Execution and playability | Fidelity and remaining work |
| --- | --- | --- |
| **cpppj — C++ reconstruction (main line)** | **Playable now.** Supports entry and progress on 34 paths: Early6, Training9, Campaign16 and Demo3. The user has completed Campaign1-1 and1-2 and confirmed the next-mission prompt/entry. The product/hash for the user's1-2 run is unknown, so it is not a regression result for the current build. Non-multiplayer menus, the deck/minimap/QWASZX/ESC UI, construction/production/bridges/economy/combat/rituals/AI and Edit loading/saving/test battles are connected. | Product758/11.01 includes World knowledge cards and the Storm/World selector, the complete BaseFile source-selection parent and live TAFF file/key path, the D/d repeat-item shortcut, the shared Client filename ring for object dumps, the T/t game-time display and live mission-message suppression, the Pause key command queue, original FileSpec/virtual-CWD enumeration, the U/u latest-loss camera key, actual object-menu return/success sound and direct Ctrl-special tracing, editor pickup/right-click cancellation and polling-coordinate separation, editor-primary owner tracking/Ctrl ownership and bridge-quality cycling, editor-owner/SetOwner world-redraw wiring, screen-mode failure recovery, initialization/refresh and adjustment confirmation, plus actual Form/shared world projection for object-attached SP text, fullscreen F12 mouse switching before Guide/modal processing, the current mission demo gate for actual selection, ESC selection cleanup followed by cursor cancellation/menu toggling and exact event-kind recognition, F6 handling after the original shared pause check, shared Client path-ring use for Edit reads/saves/deletes, corrected native startup registration order and the existing Startup filename construction/local copies after file-location selection and shared Config Load/search-handle/CLI lifetimes, the original filename interior view/four-slot options save, local path copying, Capture/CD/Help callers and the shared Client buffer ring, Edit-only CHEAT visibility, multi-island/bridge Graph comparison, Client-owned terrain99cache lifetime, individual Graph membership reporting/comparison, saved/Tab camera and rotation keys, language input/translation notifications, the complete technology list and HTML input dialog, as well as RecheckCD current-serial cleanup, actual Capture frame-sequence storage and completion UI, URL button routing through the existing Help dispatcher, original Pop overlap returns and the previous reconstruction and retired-validation exclusion,11.01 display and the third Patch Credits entry with HJOW/AI acknowledgement in the same window with Back/Cancel. The last complete34-path integration is645 on product644. Later changes use original-code comparisons, focused tests and short related GUI checks. Natural completion and full parity remain user checks. Remaining UI/Edit/scenario/file/lifetime/compatibility work, physical input/PCM/DPI/focus, untested multiplayer preparation and phases4–8 remain. Windows11 hardware verification is not a stage-transition gate. |
| dotnetpj — C# + MonoGame clone | The existing path can play Campaign **1-1 The War Begins!** and **1-2 Master of Whirligigs**, with English/Korean UI, help, context menus and options. | The plan was redone on October 28, 2026: the approximate existing implementation is frozen and is being replaced by a new `Netstorm.Client` path built around the original architecture (currently paused). The 8-bit screen and MonoGame output (R1) and the drawing primitives, palettes, color tables and generic object drawing (R2) are done. Gump UI (R3) has reached the tree, input, fonts, buttons, menus, text entry, images, background GIFs, and HTML/help window creation with link and modal wiring (R3k-8). The latest Release build on October 29, 2026 had zero warnings/errors and 2,002 passing tests. R3 as a whole and menu-screen capture comparison are unfinished, and the new screen path and world core are not connected to the default game session, so playability and fidelity remain at the approximate existing 1-1/1-2 path. Renaming the executable to `Netstorm.exe` has not been applied yet. |
| webpj — HTML + JavaScript web clone | Loads the original assets, shows a "click to start" screen and then the main menu. The Campaign, Demo and Credits dialogs, five campaign sub-lists (Early Missions, Priest Training and the three campaign chapters), the Help and Options pop-up menus (saved settings, with sound/music/volume applied) and the F1 General Help window (scroll bar, links, history) are implemented, and campaign progress values (`Done*`) are saved in the browser. The Edit button has been removed. | As of WB03-H on October 30, 74 automated tests pass, and the real-browser check (headless Edge) passes. Dialogs, lists and the help window were compared with original screenshots (the help window matches almost pixel for pixel). The stat headers and type pictures of help topics about unit types, start tips, Korean text, the static map screen and gameplay are not implemented yet, and the Wind Noise, Speaker Swap L/R, Edge Scroll, Auto-Demo and Tell Tips settings are stored without effect for now. Custom-map Edit, User Made Campaigns and multiplayer are not provided on the web. |

## Latest C++ reconstruction (cpppj)

### What you can do now

- **34 mission paths:** all 6 Early Missions, 9 Priest Training missions, 16 campaign missions (chapters 1–3) and 3 demos can be entered and played. The user played Campaign 1-1 to the end and confirmed that the next-stage prompt works.
- **Original menus and presentation:** the main menu (Campaign, Demo, Help, Options, Credits, Quit), briefings and the CD intro movies (SMK, English and German) are reproduced with the original structure.
- **Game rules:** construction, production, bridges, harvesting/transport, combat, the 25 Workshop units, ritual rules (capture, sacrifice, SP, mission completion), technology acquisition, a shared strategy AI used by enemies, allies and demos alike, and world geyser generation are restored against the original code.
- **In-game UI:** the deck and minimap, QWASZX building hotkeys, the ESC menu, chat (Enter), help links, 4:3 / 16:9 / 16:10 aspect ratios and edge scrolling are connected.
- **Edit:** loading, saving and saving as existing maps, adding islands, returning to editing after a test battle, and scenario values such as directional alliances and AI settings.
- **Windows 10/11:** the original's error when relaunching after fullscreen is designed not to occur, and relaunching from a saved fullscreen state was verified to reach the main menu normally (units 563 and 588). The distributable runs directly with `Netstorm.bat`.

### Recent progress

- **745:** connected object-attached SP text to the same session Form and current object/camera projection. Two prior exceptions were reproduced, then all 52 focused tests passed, including actual collection/font output and cleanup. No new game processes were started; this unit will be followed by the requested pause ([745](docs/exe/cpp-floating-sp-reference-20261102.md)).
- **744:** connected fullscreen F12 mouse switching to the existing compatibility device. All 552 original inputs/1,104 executions and 23 focused tests pass. Actual modal retention, key release, modifiers and windowed preservation were checked; the initial test-driver error is recorded separately ([744](docs/exe/cpp-screen-mouse-key-20261102.md)).
- **743:** connected actual selection to the current mission demo gate. Ten original observations and before/after tests detected 20 failed checks in two new cases, followed by 28 menu/session and 23 selection tests passing. No new game processes were started ([743](docs/exe/cpp-mission-demo-selection-20261102.md)).
- **742:** connected ESC selection cleanup and the original cursor/menu order. Original ESC/Select0 matched for 154 inputs at two precisions (308 executions each), and all 23 focused tests pass. Actual before/after runs reproduced selection15000 remaining after ESC, then verified clearing while running/paused, menu toggling and Q-piece cancellation ([742](docs/exe/cpp-escape-selection-20261102.md)).

- **741:** moved F6 scheduling behind the shared pause check in the actual UserInput path. All 20 focused tests pass, including 16 new original-machine-code observations and 480 existing selection-key rows. One actual process verified pause/modifier/release blocking, 29 cards with plain/Alt+F6 and ESC close/reopen behavior. F9 scheduling before the pause check and direct menu commands remain unchanged ([741](docs/exe/cpp-knowledge-key-20261102.md)).

- **739–740:** added game/combined-test/existing-full-build selection and updated both README guides, then connected actual Edit saves/deletes to the shared Client path ring. Seven new tests initially failed18 checks; all58 focused tests and1,902 existing fixture rows now pass. No new game run was added, and the existing focused target was extended without adding test executables ([739](docs/exe/cpp-build-modes-20261102.md), [740](docs/exe/cpp-editor-mutation-ring-20261102.md)).

- **685:** unified product display at11.01 and added the third Patch Credits entry with HJOW/AI acknowledgement in the same Credits window. All24 existing focused tests pass; final1024/1280 runs verify exactly three rows, Back/Cancel and both original Credits routes ([685](docs/exe/cpp-product-1101-20261101.md)).
- **684:** documented external connection features/targets and disabled the retired client-validation lookup/Not Validated notice. All12 focused tests pass; actual startup reaches the main menu with validation value0. The original82 reference observations and product policy are separate ([684](docs/exe/cpp-external-services-20261101.md)).
- **683:** restored GetTechnology/ChangeMoney script names and reused the existing technology acquisition/snapshot/NewTech and SP routines. Original48inputs/144observations and10 focused tests pass; no new game run ([683](docs/exe/cpp-get-technology-input-20261101.md)).
- **681–682:** classified the83 original menu VTs and restored PlaySound/ViewPriest/SetAllBridge/DigUp script-name bindings. Original48inputs/144observations,12 focused tests and two combat-free before/after GUI processes verify command handling through the sound-name table; actual PCM/listening and the full34 paths were not rerun ([681](docs/exe/cpp-object-menu-wiring-20261101.md), [682](docs/exe/cpp-script-bindings-20261101.md)).
- **677–680:** fixed Fast deck reactivation from10s to1s, restored original destination-color shadow translation, detected five genuine mutations and connected MoveDialog to the current modal. Focused sets of8,31 and9 tests passed on their respective products; actual UI placement, initial scenes and a combat-free message window were checked ([677](docs/exe/cpp-deck-reactivation-20261101.md), [678](docs/exe/cpp-translated-shadow-20261101.md), [679](docs/exe/cpp-focused-mutation-20261101.md), [680](docs/exe/cpp-move-dialog-20261101.md)).
- **645 (last complete34-path check):** product644 passed all34 entry/progress/normal-exit checks with window DC RGB and Native Frame evidence. This is not a complete34-path or natural-completion result for later products ([645](docs/exe/cpp-solo-integrated-20261031.md)).
- **615 (Alt/middle camera pan):** restored the original viewport-distance, FTOL/8, Shift-axis and target-write behavior. All 259 original cases/repeats, 29 focused tests and three actual-window runs with explicit test polling passed, including key consumption during pan, stopping on release and the outside-window option ([615 record](docs/exe/cpp-camera-pan-20261030.md)).
- **614 (Chat configuration and restart):** on the same product 613 distribution, three actual runs checked requests 1/0/-1, configuration reinterpretation/consumption, and saved formats on restart, with four matching whole-window RGB captures. Eight original cases matched their repeat observations, preserving distinct startup/mission subscriber keys. No new game build or ZIP ([614 record](docs/exe/cpp-chat-settings-20261030.md)).
- **613 (Sun Barricade event 3):** compared 301 original cases on a real 32-bit CPU and restored neighbor/cache preservation, cleanup/new scheduling, and the caller's NaN handling. All 57 focused tests and the new distribution's natural Tutorial6 progress checks passed ([613 record](docs/exe/cpp-fence-event3-20261030.md)).
- **612 (zero-displacement ricochet):** connected the preserved eight stack bytes to the original validity and generation control. All 213 original cases matched, 48 focused tests passed, and the new distribution passed basic entry checks. Stack layout and indeterminate-value identity remain a separate limit ([612 record](docs/exe/cpp-ice-ricochet-scratch-20261030.md)).
- **611 (Cloud Floater avoidance):** restored the original avoidance and shared game PRNG where projectile impact had thrown an unrestored-path exception. All 481 original cases matched, 46 focused tests passed, and the new distribution passed basic entry/progress/window-RGB checks. Actual targeted attacks remain for user verification ([611 record](docs/exe/cpp-missile-floater-20261030.md)).
- **607–609 (geysers):** ordinary geyser placement (607), ranges, splitting and the Player budget (608), and the real world parent plus mission Start wiring (609) were restored against original machine code. In 609, the scenario option `randGeysers` was set to 0 and 1 on the new distribution and run twice; both exited normally and the geyser count grew from 4 to 43 ([609 record](docs/exe/cpp-world-geyser-20261030.md)).
- **606 (all 34 paths):** on product 605's new distribution, all 6 Early Missions, 9 Priest Training missions, 16 campaign missions and 3 demos entered, progressed briefly and exited normally, with a real Session start, an advancing game clock, and matching actual-window RGB for all 34. These are 1024×768 windowed checks, not original-game pixel comparisons or natural playthroughs ([606 record](docs/exe/cpp-solo-current-20261030.md)).
- **605:** restored the original ConfirmRetreat prompt where a confirmation request had returned straight to the main menu ([605 record](docs/exe/cpp-retreat-confirmation-20261030.md)).
- **Earlier:** the `Netstorm.exe` artifact and launchers, real Chat Enter, Native return to the main menu, the intro SMK and bundled CD movies, Edit test-battle scenarios and alliance lifetimes, and help links were connected.

### What remains

The overall goal is incomplete and pauses by user request after unit 616 documentation and commit. On resumption, unit 617 first corrects the scaled-fullscreen pan coordinate units. Natural playthroughs and win/loss for every mission, remaining UI, Edit tools, scenarios and file lifetimes, complete visual/audio comparison with the original, and Windows 11 hardware, PCM audio and user-driven active fullscreen verification are still open. Capture, sacrifice, specific-target attacks and victories are left for user verification. Multiplayer/Outpust follows as a preliminary implementation based on decompilation, without new tests, then phase 4 (MCP), phase 5 (save location, custom-map paths, list expansion) and phase 6 (Korean support). Phase 7, optional anti-aliasing and FSR 1.0, remains a later requirement. See the [handover](LEFT_JOBS.md) for per-unit evidence and boundaries.

## Latest C# + MonoGame clone (dotnetpj)

The existing runtime path can play Campaign **1-1 The War Begins!** and **1-2 Master of Whirligigs**. A new C# client (`Netstorm.Client`) is being built to follow cpppj's architecture. Its rendering is moving to an 8-bit logical screen and palette, with MonoGame responsible for the host (`Netstorm.Client.Host`) and final output. See the existing [Campaign 1-1](docs/gameplay/campaign-one.md)/[1-2](docs/gameplay/campaign-two.md) implementation notes and the [latest handover](LEFT_JOBS.dotnetpj.md).

The plan was redone on October 28, 2026 (R0–R8): the approximate existing implementation is frozen, and the work now carries over the structure that cpppj verified against original machine code. R0 (baseline and port map), R1 (8-bit screen, MonoGame output host, capture tools) and R2 (drawing primitives, palettes, color tables, generic object drawing) are done; R3 (Gump UI and the main-menu family) was in progress when the user paused the work on October 29. As of R3k-8 (handover 0.61), the Gump tree and input, fonts, buttons, menus, text-entry boxes, images, background GIFs, and HTML and dedicated help windows have been ported, including link and modal wiring. The Release test run had zero warnings/errors and **2,002 passing tests** (Assets 363, Core 1,030, Client 609). This includes existing code and new module-level checks; it does not mean the new screen or world path is connected to live gameplay, or that the main-menu screen matches the original pixel for pixel. When resumed, the next steps are script commands, the full configuration interface and list ownership, message/dialog windows, and assembling the upper-level menu; then the world session (R4), world display/HUD (R5), play input/rules (R6) and full missions/Edit/settings (R7). Playability and parity remain limited to the approximate existing 1-1/1-2 path. See the [handover](LEFT_JOBS.dotnetpj.md) for sequence and remaining scope, the [HTML window](docs/dotnet-html-window-20261029.md) and [HTML window factory](docs/dotnet-html-factory-20261029.md) records for the latest units, and the [cpppj → dotnetpj porting plan](docs/dotnet-cpp-port-plan-20261016.md) for the evidence list.

## Latest HTML + JavaScript web clone (webpj)

Because browsers block audio until a click, the web clone loads the original assets, shows a "click to start" screen and then displays the main menu. The Edit button has been removed, and the Campaign, Demo and Credits dialogs plus five campaign sub-lists (Early Missions, Priest Training and the three campaign chapters) are connected and compared with original screenshots. WB03-F (October 29, 2026) restored the Help and Options pop-up menus at the original positions (sub-menus, check marks, closing with Esc or an outside click), saves and restores nine settings in browser storage, and applies Sound/Music on-off and volume to the actual audio. Wind Noise, Speaker Swap L/R, Edge Scroll, Auto-Demo and Tell Tips are stored only; their features do not exist yet. WB03-G (October 30, 2026) restored the F1 / Help-menu General Help window (450×350 panel, scroll bar, drag/key/wheel scrolling, link navigation, Back/OK, all 128 topics) and confirmed it matches the original screenshot almost pixel for pixel. The same day, WB03-H added writing and immediate saving of campaign progress values (`Done*`) plus a settings write layer, confirmed they persist across reloads, and fixed an empty Campaign list in Korean browsers by falling back to the English data. 74 automated tests pass, and the real-browser check (headless Edge) passes.

By user decision, the web version does not provide custom-map Edit or the **User Made Campaigns** entry in the Campaign list (the code change that hides it comes in the next unit, so the current screen still shows 8 rows). The stat headers and type pictures of help topics about unit types, start tips, Korean text, the static map screen and gameplay are not implemented yet. Multiplayer and resolution/fullscreen settings are also excluded. See the [latest handover](LEFT_JOBS.webpj.md).

The linked analysis and handover documents are written in Korean.

# Project layout and development goals

`cpppj` proceeds in this order: **① version 10.78 single-player reconstruction → ② stable Windows 10/11 execution (including mandatory DirectDraw error fixes) → ③ TCP/IP LAN multiplayer and outposts → ④–⑦ additional requirements**. The additional requirements are phase 4, local MCP (all in-game controls, screen capture and recording including audio for dotnetpj analysis), plus16:9 resolutions up to1920×1080 without requiring graphics-size scaling at this stage; phase 5, moving settings and campaign progress to the user's home folder (`Saved Games\Netstorm\Saves\`), loading custom maps from both the existing folder and `Saved Games\Netstorm\CustomMaps\` (new maps from Edit are saved to the latter), and adding scrolling and removing the display-count limit in the Edit map list; phase 6, Korean support; and phase 7, anti-aliasing and FSR1.0 in Main Menu → Options (off by default), with2560×1440 and3840×2160 on supported systems automatically enabling FSR to upscale an internal1920×1080 image. Until phase 5, saves stay in the game data folder like the original. It is Windows only. Multiplayer-only outposts belong to phase 3; official-server access is not restored. Comparison records for CD version 10.72 are also retained.

`dotnetpj` uses C# and MonoGame. It began by moving the parts verified by `cpppj` into the existing clone piece by piece (the October 16 porting plan), but because much of the existing implementation differs from the original, the plan was redone on October 28. The approximate existing implementation is frozen and replaced, in the order R0–R8, by a new `Netstorm.Client` that carries over cpppj's structure (8-bit screen, Gump UI, world session), with cpppj serving as the comparison reference for identical inputs. Matching the original behavior comes first; once `cpppj` is complete, everything is compared again before new requirements are added. cpppj's phase 4–7 goals (MCP, relocating saves/custom-map paths/Edit list scrolling, Korean support, anti-aliasing and FSR 1.0) are also part of the later requirements. The Windows executable must be named `Netstorm.exe`. GUI Linux support and multiplayer have lower priority. Final requirements include fullscreen, 16:9/16:10/4:3 aspect ratios, fullscreen edge scrolling, and English/Korean support. Animation first targets the original cadence, with 60/120-frame support afterwards.

`webpj` uses HTML and JavaScript. Once `cpppj` is complete, it will be analyzed to develop the web clone toward original behavior. Custom maps (Edit), User Made Campaigns, multiplayer and features that do not fit the web (quitting the game, resolution and fullscreen settings, and so on) are excluded, and required assets are copied into `webpj/www/`.

| Path | Purpose |
| --- | --- |
| `cpppj/` | C++20/Win32 reconstruction and tests (main line) |
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

`--run <game-data folder>` runs the C++ reconstruction (it does not run the original game). Adding `--mission <mission key>` skips the menu and opens that mission's briefing (for example, `--mission Tutorial1`; Campaign 1-1 is `--mission TheWarBegins`). `--view fonts`, `--view types`, and `--view TEST01` open font, type and mission inspection scenes. Running without arguments opens the GUI main menu; `--help` prints build information and the available options. Live construction, production, bridges, harvesting/transport, combat and automatic execution of the shared strategy AI are connected, while natural playthroughs of all target missions, remaining Edit tools, scenarios and lifetime transitions, and original-parity verification remain unfinished. See [C++ build](docs/cpp-build.md) for build instructions and the [handover](LEFT_JOBS.md) for current integration status.

## Building and running the distributable archive

Build a distributable archive (executable, Visual C++ runtime DLLs, original 10.78 game data and launcher batch files) with the script below. If you omit the output path it prompts for one, and an empty answer cancels. See section 7 of [C++ build](docs/cpp-build.md) for the layout and options. On the receiving PC, extract the archive and run `Netstorm.bat` (windowed) or `Netstorm-SavedMode.bat` (saved display mode); the game writes its settings file, so the extraction folder must be writable. User-specific settings and progress files (`d/options.cfg`, `fullscreenStateFile.dat`) are not included in the archive and are created on the recipient's first run. The default incremental build-to-ZIP path in Windows PowerShell 5.1, fresh extraction with first run, saved fullscreen restart and the windowed launcher, and a real run of the latest unit-609 distribution (1,376 entries) have been checked. Complete mission/parity and Windows 11 distribution validation remain separate.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File cpppj\package-cpppj.ps1 D:\dist\NetstormReborn-cpppj.zip
```

# C# + MonoGame clone build and run (dotnetpj, development paused)

Run the commands below from the project root (`NetstormReborn`). The game is written in C# with MonoGame DesktopGL, and the target framework is `net10.0`. The executable built today is the approximate existing-implementation path (`Netstorm`).

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
| Windows 10/11 x64 | `dist/win-x64/` | `Netstorm.exe` |
| Linux x64 | `dist/linux-x64/` | `Netstorm` |

`--self-contained true` bundles the .NET runtime, so the PC that runs the game does not need the .NET SDK or runtime installed. The SDL2 and OpenAL native libraries, `fonts/`, and `game-data/` are also included in the output. The executable uses the DLLs, runtime files, font, and game data generated alongside it, so **distribute the whole output folder**. The OS environment, such as the graphics driver and a GUI on Linux, is still required separately.

If the .NET 10 runtime is already installed on the PC that will run the game, you can make a smaller package. Change `--self-contained true` to `--self-contained false` in the commands above, and use a separate output folder such as `dist/win-x64-fdd` or `dist/linux-x64-fdd`.

Create the distributable zip with the script below. If you omit the output path it prompts for one, and an empty answer cancels. By default the receiving PC must have the .NET 10 runtime installed; add `-SelfContained` to bundle the runtime. It publishes to a temporary folder, so nothing is left in `dist/`.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File dotnetpj\package-dotnetpj.ps1 D:\dist\NetstormReborn-dotnetpj.zip
```

## Bundled game data and running the binaries

On build and publish, the contents of `assets/game-data/` are **copied automatically to `game-data/`** in the output. The Windows distribution folder looks like this (Linux is the same, with the executable named `Netstorm`).

```text
dist/win-x64/
  Netstorm.exe
  Netstorm.dll
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
.\dist\win-x64\Netstorm.exe --language english
```

Run on Linux:

```sh
chmod +x dist/linux-x64/Netstorm
./dist/linux-x64/Netstorm --language english
```

A normal run finds the bundled data automatically. To use data from another location for development or verification, set `NETSTORM_DATA` to the **absolute path of the folder that contains `netstorm.tarc`**. The search order is a valid `NETSTORM_DATA`, then `game-data/` or `assets/game-data/` in the executable folder, the working folder, and their parent folders.

```powershell
$env:NETSTORM_DATA = "D:\Games\NetstormReborn\game-data"
.\dist\win-x64\Netstorm.exe --language english
```

```sh
NETSTORM_DATA="/absolute/path/game-data" ./dist/linux-x64/Netstorm --language english
```

# Running the web clone (HTML + JavaScript)

Node.js is required (verified with v24); no external npm packages are used. The required assets are committed in `webpj/www/data/`. From the repository root, start the static web server and open `http://localhost:9803/` in a browser:

```sh
npm --prefix webpj start
```

Run the automated tests with `npm --prefix webpj test`. If the original data in `assets/game-data/` changes, rebuild `webpj/www/data/` with `npm --prefix webpj run prepare-assets` and check the copies with `npm --prefix webpj run check-assets`.

# Key documents

- [C++/shared handover](LEFT_JOBS.md) / [C# handover](LEFT_JOBS.dotnetpj.md) / [web handover](LEFT_JOBS.webpj.md): current state and upcoming work. Detailed history of earlier units is in [LEFT_JOBS.old.md](LEFT_JOBS.old.md) (cpppj/shared) and [LEFT_JOBS.dotnetpj.old.md](LEFT_JOBS.dotnetpj.old.md) (dotnetpj).
- [TODO.md](TODO.md): the current cpppj goal prompt (Early Missions, Priest Training, campaigns, demos, Edit, menus and in-game UI)
- [C++ roadmap](docs/cpp-roadmap.md) / [playable reconstruction plan](docs/cpp-playable-plan.md): development stages and reconstruction plans
- [cpppj → dotnetpj porting plan](docs/dotnet-cpp-port-plan-20261016.md): porting units (W numbers) and the evidence list. The current order (R0–R8) in section 0 of the [dotnetpj handover](LEFT_JOBS.dotnetpj.md) takes precedence.
- [Clone game data](assets/README.md): asset layout, provenance and build inclusion
- [Original-game/separate-window execution rules](docs/execution-confirmation.md): execution conditions for AI work

# License

MIT License
Copyright (c) 2026 HJOW
