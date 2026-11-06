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

**As of November 6, 2026:** `cpppj`, the main line, was paused at unit 986 at the user's request (the runtime product is unit 985, displayed as version 11.01). In the meantime, phase 4 (local MCP and 16:9 up to 1920×1080), phase 5 (user-home save locations) and phase 7 (anti-aliasing and FSR 1.0) were implemented. Phase 6 (Korean) has its first pass done, and phase 3 (LAN multiplayer) is being implemented ahead of testing, starting with transport parts and the login parent. `webpj` is porting the core world components needed to enter missions, up to WB04-D5a (November 6, 2026), and `dotnetpj` has ported Fort-loading components up to unit 0.154 (November 4, 2026). Per-unit evidence is in the [cpppj/shared handover](LEFT_JOBS.md), the [dotnetpj handover](LEFT_JOBS.dotnetpj.md) and the [webpj handover](LEFT_JOBS.webpj.md).

| Project | Execution and playability | Fidelity and remaining work |
| --- | --- | --- |
| **cpppj — C++ reconstruction (main line)** | **Playable.** All 34 paths (6 Early Missions, 9 Priest Training missions, 16 campaign missions and 3 demos) can be entered and played, with menus, the deck, the minimap, Edit and combat/AI connected. The user has played Campaigns 1-1 and 1-2 to the end, and all three demos were verified to loop through two 360-second cycles into a third world (946–948). A Korean menu (Options > Language), 16:9 resolutions up to 1920×1080, optional anti-aliasing and FSR 1.0, and a `--mcp` local MCP server are available. | Each component is reconstructed against the original 10.78 machine code, and the implementation/comparison of non-multiplayer UI (929), Edit (943), mission lifetimes (956) and internal lifetimes/files (980) has been closed out. Full parity and natural playthroughs of every mission remain user checks; the last complete 34-path check was unit 645 (product 644). Remaining: phase 3 multiplayer (LAN UI, world sync, regular battles, Outpust), phase 6 Korean body text (missions, tips, help) and Korean button art, and the phase 8 Edit scenario editor UI. |
| dotnetpj — C# + MonoGame clone | The existing path (`Netstorm.exe`) can play Campaign **1-1 The War Begins!** and **1-2 Master of Whirligigs** as an approximate implementation, with English/Korean UI, help, context menus and options. The new-path host (`Netstorm.Client.Host`) covers part of the menu screens, such as the title and main menu, and has no missions yet. | Following the plan set on October 28, 2026 (R0–R8), the game is being replaced by a new `Netstorm.Client` path that carries over cpppj's structure as is. R0, R1 and R2 are done; R3 (Gump UI and menus) is in progress; and R4 (world session) has ported the Template store (TS0–TS6) and Fort-loading stages F1–F2c-11. Version 11.01 with three Credits entries, the external-connection inventory with client validation disabled, and the `Netstorm.exe` executable name are applied. The Release run at 0.154 passed 1,243 Core, 858 Client and 366 Assets tests. Menu capture comparison (D1) and connecting the new path to real missions are still pending, so fidelity remains at the approximate 1-1/1-2 level. |
| webpj — HTML + JavaScript web clone | After a "click to start" screen, the main menu is shown with the original assets. The Campaign, Demo and Credits dialogs (three entries, including 11.01 Patch Credits) and lists, the Help and Options menus (saved settings), the F1 help window with picture icons, the startup welcome/tip windows and saving of campaign progress values are implemented. In a Korean-language browser, menus, dialogs and F1 help (126 of 128 topics) are shown in Korean with the D2Coding font. **Missions cannot be played yet**; only a development terrain preview (`?map=`) without game rules exists. | Menus, dialogs and the help window were compared with original screenshots (the help window matches almost pixel for pixel). To enter missions, Fort reading, terrain computation and core world components (SID pool, spatial hash, PRNG, game clock, Kernel, input queue, main loop, Unpop/destruction, the Form layer, SquidFactory and Form hosts, WB04-A–D5a) were ported and checked against original 10.78 data, but they are not yet connected to the app lifecycle. 429 automated tests pass (WB04-D5a). Remaining: WB04-D5b–E, real Fort Load→Start, mission screens and gameplay rules, opening external links in a new tab, and Korean mission text. Edit, User Made Campaigns and multiplayer are not provided on the web. |

## Latest C++ reconstruction (cpppj)

### What you can do now

- **34 mission paths:** all 6 Early Missions, 9 Priest Training missions, 16 campaign missions (chapters 1–3) and 3 demos can be entered and played. The user played Campaigns 1-1 and 1-2 to the end and confirmed that the next-mission prompt works. For the three demos, the restart into a new world after 360 seconds was verified twice, through the third world and Exit Demo.
- **Original menus and presentation:** the main menu (Campaign, Demo, Help, Options, Credits, Quit), briefings and the CD intro movies (SMK, English and German) are reproduced with the original structure. Credits has a third entry, **Netstorm 11.01 Patch Credits**.
- **Game rules:** construction, production, bridges, harvesting/transport, combat, the 25 Workshop units, ritual rules (capture, sacrifice, SP, mission completion), technology acquisition, a shared strategy AI used by enemies, allies and demos alike, and world geyser generation are restored against the original code.
- **In-game UI:** the deck and minimap, QWASZX building hotkeys, the ESC menu, chat (Enter), help links, right-click menus and target-specific cursors for buildings, carriers and special targets, 4:3 / 16:9 / 16:10 aspect ratios and edge scrolling are connected.
- **Edit:** loading, saving and saving as existing maps, adding islands, returning to editing after a test battle, and scenario values such as directional alliances and AI settings. The map list has no entry limit and scrolls 50 rows at a time; new maps are saved to `CustomMaps` in the user's home folder.
- **Save locations (phase 5):** every file the game creates (settings, progress, captures, diagnostics, editor working copies and so on) goes under `%USERPROFILE%\Saved Games\Netstorm\` (`Saves`, `CustomMaps`, `Editor`, `Captures`, `Diagnostics`). Running from a folder that cannot be written without administrator rights was verified (932–934).
- **Local MCP (phase 4):** `Netstorm.exe --mcp` runs the game as a stdio MCP server for controls, state queries, PNG screenshots, frame waits and recording with sound (935–936, [MCP record](docs/exe/cpp-mcp-server-20261106.md)).
- **Resolutions and post-processing (phases 4 and 7):** the 16:9 resolutions 1600×900 and 1920×1080 are supported (937). Anti-Aliasing and FSR 1.0 Upscaling in Options are off by default, and 2560×1440 and 3840×2160 can be selected when a Direct3D 11 device exists; these two resolutions upscale a 1920×1080 image with FSR 1.0 (939).
- **Korean (phase 6, first pass):** choosing Korean in Options > Language switches menu and window text to Korean with the D2Coding font and a 502-entry translation table. When a Korean mission file is missing, the English file is used instead (938).
- **Windows 10/11:** the original's error when relaunching after fullscreen is designed not to occur, and relaunching from a saved fullscreen state was verified to reach the main menu normally (units 563 and 588). The distributable runs directly after extraction.

### Recent progress

- **986 (November 6, 2026, paused):** the in-progress Korean body-text and scenario-UI drafts and the remote-Fort analysis were preserved as [tracked handover artifacts](docs/exe/handoffs/cpp-pause-20261106/README.md), and the goal was paused at the user's request. The drafts are not applied to the product; the runtime product remains 985 ([986 record](docs/exe/cpp-pause-handoff-20261106.md)).
- **982–985 (phase 3 LAN, implemented ahead of testing):** LAN TCP transport parts based on the original framing (982), the shared transport registry joined to real world-callback lifetimes (984), and the ordinary login parent with the initial SP assignment branch (985) were implemented from original evidence. By policy no real network or multiplayer tests were run, and the LAN UI, player roster and regular battles are not connected yet. Unit 983 fixed a reported VS 2022 compile error in the MCP tests ([982](docs/exe/cpp-lan-tcp-transport-20261106.md), [983](docs/exe/cpp-mcp-test-compile-20261106.md), [984](docs/exe/cpp-shared-transport-lifetime-20261106.md), [985](docs/exe/cpp-ordinary-login-parent-20261106.md)).
- **981:** fixed a black band in the left deck at resolutions taller than 774 pixels ([981](docs/exe/cpp-wide-deck-background-20261106.md)).
- **957–980 (internal lifetimes and files):** shared Pop, Factory, Form, Process, Kernel, SID allocation recovery, color-table files, options-save failure recovery, the out-of-memory handler and shutdown cleanup were connected to the real Client/World, closing out the active non-multiplayer scope. The retired CD validation protocol was disabled by policy (965). See the [980 summary](docs/exe/cpp-internal-lifetime-closure-20261106.md).
- **944–956 (mission lifetimes):** Retry/Next/Abort and consecutive transitions, repeated continue/restart/abort/main-menu round trips in Early1, the second 360-second cycle and third world of all three demos, LeaveBattle, and State transitions and cleanup were compared ([945](docs/exe/cpp-mission-abort-replay-20261106.md), [948](docs/exe/cpp-demo-second-alliance-20261106.md), [956](docs/exe/cpp-mission-state-scope-20261106.md)).
- **940–943 (Edit and scenarios):** dynamic scenario configuration and TARC, re-saving after a sharing-violation save failure, object loading and editing-tool lifetimes were compared, closing out the Edit implementation ([943](docs/exe/cpp-editor-implementation-closure-20261106.md)).
- **932–939 (phases 4–7):** user-home save locations and the unlimited Edit list (932–934), the local MCP server and recording with sound (935–936), 16:9 resolutions (937), the first Korean pass (938), and anti-aliasing and FSR 1.0 (939) were implemented and checked with real runs ([phase 5](docs/exe/cpp-user-data-root-20261105.md), [MCP](docs/exe/cpp-mcp-server-20261106.md), [resolutions](docs/exe/cpp-wide-resolution-20261106.md), [Korean](docs/exe/cpp-korean-language-20261106.md), [AA/FSR](docs/exe/cpp-aa-fsr-20261106.md)).
- **816–931 (UI and world components):** world components such as Graph, surfaces, archipelago generation, wind/particles/weather, burning and mana supply; constructors, HP and Form lifetimes; right-click menus and cursors for buildings, carriers and special targets (913–918); and game menus, HUD, chat and modal lifetimes (920–928) were connected after comparison with the original. Non-multiplayer UI (929) and original language handling (931) were closed out as well ([929](docs/exe/cpp-ui-implementation-closure-20261105.md)).
- **Earlier (up to 815):** version 11.01 and the third Patch Credits entry (685), the external-connection inventory and disabled client validation (684), the last complete 34-path check (645), geyser generation (607–609), ESC/selection/camera/hotkey input, main-menu pop-ups and the intro, and the `Netstorm.exe` artifact and launchers were connected ([685](docs/exe/cpp-product-1101-20261101.md), [684](docs/exe/cpp-external-services-20261101.md), [645](docs/exe/cpp-solo-integrated-20261031.md)). Detailed history of earlier units is in [LEFT_JOBS.old.md](LEFT_JOBS.old.md).

### What remains

The overall goal is incomplete and was paused at unit 986 at the user's request. On resumption, phase 3 multiplayer will be implemented from original evidence, ahead of testing, starting with the remote Fort lifetime (987) and continuing with world sync, battle and central islands, regular-battle States, the player roster, the LAN UI, the metamap/saves/reconnection and Outpust (no new network tests). Phase 6 still needs `.korean` body text for mission briefings, tips and help, Korean main-menu button art, Korean text input and translation review. Phase 8 still needs the Edit scenario editor UI, automatic creation of a default `.english` file for new maps, and a fix for the exit when entering Test Battle without an `.english` file. The `CustomMaps` layout of phase 5, mission/fullscreen/4K screens under phase 7, natural playthroughs and full parity of every mission, and actual capture, sacrifice, specific-target attacks and victories are left for user verification. 60/120-frame support and a full CD 10.72 reconstruction are separate, lower priorities. See the [handover](LEFT_JOBS.md) for per-unit evidence and the scope not yet verified.

## Latest C# + MonoGame clone (dotnetpj)

The existing runtime path (`Netstorm.exe`) can play Campaign **1-1 The War Begins!** and **1-2 Master of Whirligigs**. A new C# client (`Netstorm.Client`) is being built to follow cpppj's architecture. Its rendering is moving to an 8-bit logical screen and palette, with MonoGame responsible for the host (`Netstorm.Client.Host`) and final output. See the existing [Campaign 1-1](docs/gameplay/campaign-one.md)/[1-2](docs/gameplay/campaign-two.md) implementation notes and the [latest handover](LEFT_JOBS.dotnetpj.md).

The plan was redone on October 28, 2026 (R0–R8): the approximate existing implementation is frozen, and the work now carries over the structure that cpppj verified against original machine code. R0 (baseline and port map), R1 (8-bit screen, MonoGame output host, capture tools) and R2 (drawing primitives, palettes, color tables, generic object drawing) are done. R3 (Gump UI and the main-menu family) has ported the Gump tree and input, fonts, buttons, menus, text entry, images, background GIFs and HTML/help windows, plus the new host's title, main menu, Help/Options pop-ups, options saving and the Edit map list; dialog windows, upper-level menu ownership and menu capture comparison (D1) remain, so it is still in progress. R4 (world session) has ported the mission assembly parts (Fort reader, Guide, shared Start, AI settings, Tutorial Init), the Template/DataManager store (TS0–TS6, with all 433 original `.fort` files matching on a read→save round trip) and Fort-loading stages F1–F2c-11 (type table, Money/Meta, TypeNames/Deck, Territory, object loading, save modules, island list, clusters), checked against original x86 fixtures. Version 11.01 with three Credits entries (0.150), the external-connection inventory with client validation disabled (0.151) and a local transport skeleton (0.152) are also applied.

The Release run at 0.154 (November 4, 2026) passed **1,243 Core, 858 Client and 366 Assets tests** (no failures or skips). These counts include tests of the existing path, so they do not measure progress on the new path. Next come the rest of F2c (`RawFortWorldLoad` and the terrain island Builder family), then F2d and F3 (connecting `RawCampaignSession`), followed by R5 (world display and HUD), R6 (play input and rules), R7 (full missions, Edit and settings) and R8 (Korean, phases 4–8 and more). Until the new path is connected to real missions, playability and parity remain limited to the approximate 1-1/1-2 path. See the [handover](LEFT_JOBS.dotnetpj.md) for sequence and remaining scope, and the [cpppj → dotnetpj porting plan](docs/dotnet-cpp-port-plan-20261016.md) and the [Template store porting plan](docs/dotnet-template-storage-plan-20261101.md) for the evidence list.

## Latest HTML + JavaScript web clone (webpj)

Because browsers block audio until a click, the web clone loads the original assets, shows a "click to start" screen and then displays the main menu. The Campaign, Demo and Credits dialogs, the Early Missions, Priest Training and campaign-chapter lists, the Help and Options pop-up menus (nine saved settings, with sound/music/volume applied), the F1 General Help window (scroll bar, links, history; matching the original almost pixel for pixel), the startup welcome/tip windows and saving of campaign progress values (`Done*`) are implemented and compared with original screenshots. The product version reads `v11.01`, and Credits has a third **Netstorm 11.01 Patch Credits** entry (WB03-AC). The web runtime's external connections (same-origin asset requests only; external links are reported on the console) are documented in the [external-access inventory](docs/webpj-external-access-20261105.md) (WB02-E). Picture icons in help text are resolved with the full type table (116 types) and drawn (WB03-T).

In a Korean-language browser, the main menu, pop-up menus, dialogs, campaign lists and F1 help are shown in Korean using the D2Coding font (woff2) and language files in the original format (`xlat.korean`, `tell.korean`, `offical*.korean`, `config.korean`, `help.korean`). 126 of the 128 help topics are translated; `serverHelp`, which is unfinished in the original itself, and `map`, which contains only a picture, remain in English (WB03-N–S). Korean mission text will come with the mission implementation.

As groundwork for entering missions, `.fort` reading (188 types), territory layout, terrain masks/tiles/cliffs and owner color tables were checked independently against all 465 original maps (WB03-U–Y), and shared world drawing, a development terrain preview (`?map=`, no game rules) and a Fort object loader were added (WB03-Z–AB). WB04 then ported, against original fixtures, version 10.78's SID pool, spatial hash and finder, PRNG/game clock/Kernel, game auxiliary globals, 40-slot input queue and main loop, shared spatial release/Unpop/destroyer, the shared Form layer, SquidFactory and the Form hosts (Depend/Gump/Content) (WB04-A–D5a, November 6, 2026). These components are not yet connected to the app lifecycle, so **missions cannot be played yet**. The last full test run passed **429 tests** on Node v24. Next come WB04-D5b (Form dispatcher and Content lifetimes), D5c (ProcessHost and Regular) and E (Pop/owners), then WB05's real Fort Load→Start, mission screens (M2) and gameplay rules.

By user decision, the web version does not provide custom-map Edit, the **User Made Campaigns** entry in the Campaign list, multiplayer, or resolution/fullscreen settings. Opening external links from the original data in a new tab/window has been decided but not implemented yet. The Wind Noise, Speaker Swap L/R, Edge Scroll and Auto-Demo settings are stored only. Because cpppj keeps changing, each unit is implemented against the committed cpppj at the time it starts, and later cpppj changes are brought in through a separate re-sync process. See the [latest handover](LEFT_JOBS.webpj.md) for details and next steps.

The linked analysis and handover documents are written in Korean.

# Project layout and development goals

`cpppj` proceeds in this order: **① version 10.78 single-player reconstruction → ② stable Windows 10/11 execution (including DirectDraw error fixes) → ③ TCP/IP LAN multiplayer and outposts → ④–⑧ additional requirements**. The additional requirements are:

- **Phase 4:** local MCP (all in-game controls, screen capture and recording including audio, for dotnetpj analysis) and 16:9 resolutions up to 1920×1080 (no graphics-size scaling required)
- **Phase 5:** move settings and campaign progress to the user's home folder (`Saved Games\Netstorm\Saves\`), and move every file and folder the game creates automatically under `Saved Games\Netstorm\` so that the game works even when installed where files cannot be written without administrator rights. Custom maps are loaded from both the existing folder and `Saved Games\Netstorm\CustomMaps\` (new maps from Edit are saved to the latter), and the Edit map list gains scrolling with no display-count limit.
- **Phase 6:** Korean support
- **Phase 7:** anti-aliasing and FSR 1.0 in Main Menu → Options (off by default). On systems that support FSR, 2560×1440 and 3840×2160 can be selected; choosing them turns FSR on automatically and upscales an internal 1920×1080 image to the target resolution.
- **Phase 8:** a dedicated Edit UI for editing not only maps and component placement but the scenario itself on screen (players' starting funds and technology, enemy composition, alliances and so on), and automatic creation of a default `.english` file for new maps (fixing the exit when entering Test Battle without an `.english` file)

① and ② are in progress with user checks such as natural playthroughs and full parity still open; phases 4, 5 and 7 have their first implementation done, phase 6 is in progress, and phases 3 and 8 remain. It is Windows only (targeting x86-64, with Windows 11 on ARM in mind). Multiplayer-only outposts belong to phase 3; official-server access is not restored. Comparison records for CD version 10.72 are also retained.

`dotnetpj` uses C# and MonoGame. It began by moving the parts verified by `cpppj` into the existing clone piece by piece (the October 16 porting plan), but because much of the existing implementation differs from the original, the plan was redone on October 28. The approximate existing implementation is frozen and replaced, in the order R0–R8, by a new `Netstorm.Client` that carries over cpppj's structure (8-bit screen, Gump UI, world session), with cpppj serving as the comparison reference for identical inputs. Matching the original behavior comes first; once `cpppj` is complete, everything is compared again before new requirements are added. cpppj's phase 4–8 goals (MCP and wide resolutions, relocating saves/custom-map paths/Edit list scrolling, Korean support, anti-aliasing and FSR 1.0, the Edit scenario UI) are also part of the later requirements. The Windows executable is named `Netstorm.exe`. GUI Linux support and multiplayer have lower priority. Final requirements include fullscreen, 16:9/16:10/4:3 aspect ratios, fullscreen edge scrolling, and English/Korean support. Animation first targets the original cadence, with 60/120-frame support afterwards.

`webpj` uses HTML and JavaScript. It is developed by analyzing `cpppj`, aiming at the original behavior; because cpppj is still in progress, it is implemented first against the current committed cpppj and later changes are re-synced separately (user decision, November 5, 2026). Custom maps (Edit), User Made Campaigns, multiplayer and features that do not fit the web (quitting the game, resolution and fullscreen settings, and so on) are excluded, and required assets are copied into `webpj/www/`.

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

Korean building and unit names follow the [shared naming policy](docs/korean-unit-names.md). Priest, Temple, Bridge, Thrower, Geyser, Altar, Residence and Empty may be translated; other names use Korean phonetic spellings, including Workshop → 워크샾 and Generator → 제네레이터. cpppj's translation table (`xlat.korean`, 502 entries) and dotnetpj's menus and unit titles (the shared 55-entry table) use the same names. webpj has finished translating its menus, dialogs and help; mission text is not yet translated in any of the three projects.

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

Build a distributable archive (executable, Visual C++ runtime DLLs, original 10.78 game data, the Korean font and translation table, FSR 1.0 headers and license, and launcher batch files) with the script below. If you omit the output path it prompts for one, and an empty answer cancels. See section 7 of [C++ build](docs/cpp-build.md) for the layout and options. On the receiving PC, extract the archive and run `Netstorm.exe` or `Netstorm-SavedMode.bat` (saved display mode), or `Netstorm.bat` (forces windowed mode). Settings, progress, captures and other files are stored in `%USERPROFILE%\Saved Games\Netstorm\` rather than the install folder, so the extraction folder may be read-only. To reset settings, delete `options.cfg` and `fullscreenStateFile.dat` in `%USERPROFILE%\Saved Games\Netstorm\Saves\`. The default incremental build-to-ZIP path in Windows PowerShell 5.1 and fresh extraction with first run, saved fullscreen restart and the windowed launcher have been checked, along with extracted runs of the unit 645 distribution (product 644, 1,376 entries) and the unit 938 distribution (with the Korean font and translation table, 1,379 entries). Distribution checks for later products and Windows 11 distribution validation remain separate.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File cpppj\package-cpppj.ps1 D:\dist\NetstormReborn-cpppj.zip
```

# C# + MonoGame clone build and run (dotnetpj, migrating to the new path)

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

Run the automated tests with `npm --prefix webpj test`. If the original data in `assets/game-data/` changes, rebuild `webpj/www/data/` with `npm --prefix webpj run prepare-assets` and check the copies with `npm --prefix webpj run check-assets`. For development you can combine the URL parameters `?lang=ko` (Korean), `?mute` (no sound) and `?aspect=16:9` (aspect ratio); `?map=savetheisland` opens a development view that shows only the terrain, without game rules.

# Key documents

- [C++/shared handover](LEFT_JOBS.md) / [C# handover](LEFT_JOBS.dotnetpj.md) / [web handover](LEFT_JOBS.webpj.md): current state and upcoming work. Detailed history of earlier units is in [LEFT_JOBS.old.md](LEFT_JOBS.old.md) (cpppj/shared), [LEFT_JOBS.dotnetpj.old.md](LEFT_JOBS.dotnetpj.old.md) (dotnetpj) and [LEFT_JOBS.webpj.old.md](LEFT_JOBS.webpj.old.md) (webpj).
- [cpppj pause handover artifacts](docs/exe/handoffs/cpp-pause-20261106/README.md): the Korean body-text and scenario-UI drafts and remote-Fort analysis preserved at unit 986
- [TODO.md](TODO.md): the goal prompts for cpppj and dotnetpj (instructions to carry each handover's work through to the end, and which tests to skip)
- [C++ roadmap](docs/cpp-roadmap.md) / [playable reconstruction plan](docs/cpp-playable-plan.md): development stages and reconstruction plans
- [cpppj → dotnetpj porting plan](docs/dotnet-cpp-port-plan-20261016.md): porting units (W numbers) and the evidence list. The current order (R0–R8) in section 0 of the [dotnetpj handover](LEFT_JOBS.dotnetpj.md) takes precedence.
- [Clone game data](assets/README.md): asset layout, provenance and build inclusion
- [Original-game/separate-window execution rules](docs/execution-confirmation.md): execution conditions for AI work

# License

MIT License
Copyright (c) 2026 HJOW
