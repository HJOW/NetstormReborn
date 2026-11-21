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

**As of November 21, 2026:** `cpppj`, the main line, is at product 1323 (displayed as version 11.01). After implementing phases 1–14 of AGENTS.md and passing the final distribution gate (M8, at product 1310), it resumed on November 17 at the user's instruction and added the first-run state, a fix for a reported exit on mission transition, the Korean Galmuri11 font and the original executable icon. Within the new path's world session assembly (R4), `dotnetpj` has moved on from the AI runtime and periodic processing to internal login, transport, client globals and long-lived runtime lifetimes, then chat, root receiving, object packet receiving and path processing, and was paused at the user's request at unit 0.797 (the last full run, at 0.796, passes 5,663 tests). `webpj`, up to WB04-E1166, can now enter and run all 31 missions in the campaign lists, the demos and Skirmish in the browser, with the original control keys and the flow after a win or loss also confirmed, and is implementing the rest of the in-game UI (knowledge panel and chat window contents, object menus) and mission objectives. Per-unit evidence is in the [cpppj/common handover](LEFT_JOBS.md), [dotnetpj handover](LEFT_JOBS.dotnetpj.md) and [webpj handover](LEFT_JOBS.webpj.md).

| Project | Execution and playability | Fidelity and remaining work |
| --- | --- | --- |
| **cpppj — C++ reconstruction (main line)** | **Playable.** The original 10.78 menus, 6 Early Missions, 9 Priest Training missions, 16 campaign missions and 3 demos (34 routes), Edit (including scenario editing) and Skirmish can be played. It includes English/Korean/Japanese UI, 16:9 up to 1920×1080 (2560×1440 and 3840×2160 through FSR 1.0), difficulty, Frame Rate 60/75/120, a local MCP server, and TCP/IP local multiplayer with Outposts. The distribution ZIP runs right after extraction. | Each part was reconstructed by comparison with the original 10.78 machine code, and the implementation of AGENTS.md phases 1–14 is closed (phase 14 up to embedding the QuickJS engine). Natural playthroughs of every mission and full parity, actually running multiplayer (no runtime verification so far), human review of Korean and Japanese, and real 4K / other monitors and DPI / Windows 11 and ARM hardware are left to the user. All 34 known bugs have been judged (10 fixed); 12 need more reproduction details. Reconstruction of CD version 10.72 was confirmed up to the menus and then put on hold. Rerunning the M8 distribution gate on product 1323 remains. |
| dotnetpj — C# + MonoGame clone | The existing path (`Netstorm.exe`) can play Campaign **1-1 The War Begins!** and **1-2 Master of Whirligigs** as an approximate implementation (English/Korean UI, frozen). The new-path host (`Netstorm.Client.Host`), which carries over cpppj's structure as is, runs the title, main menu, Help, Options, Credits, campaign selection and Edit list, and — through the development-only direct mission entry (`--mission`) — session, terrain and fort-object loading, AI creation and periodic processing. With no world display, input or rules yet, **the new path cannot be played.** | On the new path, 12 menu scenes are pixel-identical to cpppj, and parts were matched against 413 original x86 comparison data sets. For the mission start-state comparison, cpppj baselines for 34 routes and dotnetpj dumps for 31 missions exist, but none match in full yet (0/34). Of the 1,239 cpppj source units (169,296 lines), 580 (99,681 lines) have corresponding code and **659 units (69,615 lines) remain** (counted with `tools/port_inventory.py` on November 21, 2026; units with corresponding code may still have unported branches). The full test run at 0.796 passes Assets 366, Core 3,480 and Client 1,817. |
| webpj — HTML + JavaScript web clone | After "click to start", the main menu is shown with original assets. Campaign, Demo, Credits (three items including 11.01 Patch Credits), Help, Options (settings saved; language, difficulty and Frame Rate selection; Reset All) and F1 help work, and in Korean the menus and all mission scripts are shown with the Galmuri11 font. **Missions can be played in the browser:** all 31 missions in the campaign lists (6 Early Missions, 9 Priest Training, 16 campaign) start and run for long sessions without errors, and the three demos' 360-second cycle and Skirmish (random islands and geysers, number of AIs and banned units) were confirmed. Tutorial2 was played naturally from the start to its completion window, and the ESC game menu (view objectives, Pause, Restart/Leave Mission and more), shortcut keys and mission music work. The original help's control keys (screen positions, rotation, D, Pause and more) and the flow after the win/loss notice (Replay, Next Mission, progress saving) were also confirmed. The knowledge (NSInfo) HUD panel and chat window contents, the remaining object menus, and the objectives and completion of other missions are still open. | Eight menu scenes have zero pixel difference from cpppj's output (apart from the cloud background, whose cause is recorded). Of the 1,022 units to port, **887 (86.8%)** were ported while matching them against original 10.78 comparison data, and 135 remain. The Tutorial1 state dump right after start was confirmed to match the cpppj baseline structurally, excluding time-dependent values; fixed-clock L2, L3, screen comparisons and natural playthroughs are not verified yet. The latest full automated test runs pass all 6,702 tests on Node v26 and v22. The web version does not provide Edit, User Made Campaigns or multiplayer. |

## Latest C++ reconstruction (cpppj)

### What you can do now

- **34 mission routes:** the 6 Early Missions, 9 Priest Training missions, 16 campaign missions (chapters 1–3) and 3 demos can be entered and played. The user played Campaign 1-1 and 1-2 to the end, up to the next-mission prompt, and the three demos were confirmed to repeat on their 360-second cycle. The last time all 34 routes were re-checked together was product 644.
- **Original menus and presentation:** Campaign, Demo, Help, Options, Credits and Quit in the main menu, briefings, and the CD intro video (SMK) follow the original structure. Credits has a third item, **Netstorm 11.01 Patch Credits** (`Copyright 2026 HJOW`).
- **Game rules and UI:** construction, production, bridges, harvesting/transport, combat, the 25 Workshop units, capture, sacrifice and rituals, technology acquisition, the shared AI and geyser generation were reconstructed against the original, and the deck, minimap, QWASZX, ESC menu, chat, context menus, cursors and edge scrolling are connected.
- **Multiplayer (phase 3):** TCP/IP local-network multiplayer and Outposts are implemented and statically compared. Official-server access and client validation are not restored. Verifying an actual LAN session is left to the user.
- **Local MCP and wide screen (phase 4):** `Netstorm.exe --mcp` runs the game as a stdio MCP server for control, state queries, screen PNGs and recording with sound ([MCP record](docs/exe/cpp-mcp-server-20261106.md)), and 16:9 resolutions up to 1920×1080 are supported.
- **Save locations (phase 5):** every file the game creates — settings, progress, custom maps, logs and so on — is stored under `%USERPROFILE%\Saved Games\Netstorm\`. The game works even when installed in a folder that cannot be written without administrator rights. The Edit map list scrolls with no count limit.
- **Korean (phase 6) and Japanese (phase 13):** choosing Korean in Options > Language switches menus, windows and mission text to Korean with the Galmuri11 bitmap font (1322). Japanese covers the UI (a 509-key translation table, Noto Sans JP), with mission text falling back to English. Fonts are not embedded in the executable; they are read from the `fonts\` folder.
- **Error log (phase 7):** a detailed log from just before an abnormal exit is written to `%USERPROFILE%\Saved Games\Netstorm\logs\yyyyMMddHHmmss.log` ([error log](docs/exe/cpp-error-log-20261111.md)).
- **Anti-aliasing and FSR 1.0 (phase 8):** selectable in Options and off by default. With a Direct3D 11 device, 2560×1440 and 3840×2160 can be chosen; the image rendered at 1920×1080 is then upscaled with FSR 1.0.
- **Scenario editing (phase 9):** Edit can change scenario values such as starting funds, tech, enemy composition and alliances on screen, and creates the `.english` file for new maps automatically. The exit that occurred when entering Test Battle without an `.english` file is fixed.
- **Difficulty (phase 11) and Skirmish (phase 12):** Difficulty (Easy, Normal, Hard) in Options, and Skirmish in the Campaign menu (number of enemy AIs, per-AI difficulty and alliance, up to five banned units, random islands and geysers under multiplayer rules) ([difficulty](docs/exe/cpp-difficulty-d-20261111.md), [skirmish](docs/exe/cpp-skirmish-sk-20261111.md)).
- **Frame rate and QuickJS (phase 14):** Frame Rate in Options selects 60, 75 (original) or 120. The QuickJS-ng v0.18.0 engine is statically embedded and exposes no game state, files or network ([QuickJS](docs/exe/cpp-quickjs-engine-20261111.md)).
- **Windows 10/11:** the original's error on restarting after leaving fullscreen does not occur. The distribution runs right after extraction. The executable icon matches the original (1323).

### Recent progress

- **1314–1323 (November 16–19, 2026, resumed at the user's instruction):** the first-run state (default volumes, first welcome windows, campaign locks, 1314), a mistranslation in Korean unit information (1315), a fix for a reported exit from the previous connection on mission transition (1320), switching the Korean font to Galmuri11 bitmaps (1322), and the original executable icon with a shared ICO (1323). 1316–1319 and 1321 corrected test baselines.
- **1312–1313 (November 14, 2026):** command-line-only test autoplay (`--test-autoplay` and related options: an AI is attached to the local player to play campaign and skirmish games automatically, with a spectator camera) and a batch runner were added, and a Campaign 1-1 victory was reproduced. None of this appears in normal runs, options or menus ([usage guide](docs/testing/cpp-autoplay-test-guide.md)).
- **1311 (M8 final distribution gate):** the Release build, a full comparison of the ZIP's 1,436 entries, running the distribution on five routes (English, Korean and Japanese menus, a mission, a skirmish) and the external-access audit all passed ([final report](docs/exe/cpp-m8-final-gate-1310-20261114.md)). Product code has changed since, so the gate must be rerun before the next distribution.
- **1302–1310 (CD version 10.72):** the edition option, default resolution, messages, CD data path and menu display were reconstructed, and the work was then put on hold by user decision ([deferral handover](docs/exe/cpp-cd1072-deferred-20261114.md)).
- **1285–1301:** judgement of the 34 known bugs with 10 fixes ([status table](docs/exe/cpp-known-bugs-status-20261111.md)), the Japanese UI, the 11.01 copyright line, difficulty, Skirmish, Frame Rate 60/120, and embedding the QuickJS engine.
- **987–1284:** phase 3 LAN multiplayer and Outposts implemented and statically compared (up to 1279), the error log (1269), Korean K1–K5 (1270–1281, [closure](docs/exe/cpp-korean-k4-k5-closure-20261111.md)), and scenario editing S1–S4 (1277–1284, [read-only install check](docs/exe/cpp-scenario-s4-readonly-home-20261111.md)).
- **Earlier (up to 986):** closure of the non-multiplayer UI (929), Edit (943), mission lifetime (956) and internal lifetime/files (980); phase 4 MCP and wide screen, phase 5 save locations, anti-aliasing and FSR 1.0. Detailed history of past units is in [LEFT_JOBS.old.md](LEFT_JOBS.old.md).

### What remains

After pausing on November 14, 2026, the goal resumed on November 17 at the user's instruction. What remains is ① user checks (natural playthroughs of every mission and full parity; actually reproducing capture, sacrifice, specific enemy attacks and victory; running multiplayer; reviewing Korean and Japanese; real 4K, other monitors and DPI, Windows 11 and ARM), ② optional work that needs a user decision (resuming the CD version 10.72 reconstruction, flyer display interpolation, translating Japanese mission text and adding languages, reproduction details for 12 known bugs, the scope of QuickJS integration), and ③ classifying the allowed existing test failures and building a new ZIP and rerunning the M8 gate on product 1323 (the next AI task). Details are in the [handover](LEFT_JOBS.md).

## Latest C# + MonoGame clone (dotnetpj)

There are two runtime paths. The **existing path** (`Netstorm.Game`, today's `Netstorm.exe`) is an approximate implementation based on recordings and observation; it can play Campaign **1-1 The War Begins!** and **1-2 Master of Whirligigs** and is frozen, with no new features. The **new path** (`Netstorm.Client` + `Netstorm.Client.Host`) is a direct translation that carries over cpppj's files, classes, call order and memory layout; the screen is drawn into an 8-bit logical buffer with a palette, and MonoGame handles only the window, input and audio devices and the final output. Once the new path handles both menus and missions it takes over the `Netstorm.exe` name and the existing path is deleted.

So far R0 (baseline and port map), R1 (8-bit screen host and comparison tools) and R2 (drawing primitives, palettes, fonts) are done, and in R3 (Gump UI and menus) 12 menu scenes are pixel-identical to cpppj. **R4 (world session assembly) is in progress:** mission Fort loading, players, alliances, terrain, islands, fort objects, the graph and the common object-lifetime layer (factory, Pop/Unpop, deletion, damage, burning, priests, force fields) are connected to the host. Units 0.639–0.688 ported the AI runtime (creation, preparation, strategy, the full frame, notices, world registration, collectors, economy, facility production), the decoration, player, sprite and burial periodic bodies, upgrades, research and the production deck, and the flare and construction display devices; units 0.689–0.731 ported internal login, packet transport, client globals, the long-lived lifetimes of runtime objects, players and state, and the client frame execution order; units 0.733–0.797 ported state cleanup and return, root resources and selection, the cursor, camera and renderer owners, the whole chat family (manager, command table, submission, ChatWindow, receiving) with root receive and transport observers, the object packet receive bodies and the path search and processing bodies. All of these follow the original order and are wired into the session. Baselines for comparing the mission start state with cpppj exist for all 34 routes, but no route matches yet (0/34). The remaining differences are post-processing during loading (farmland frames and owners, graph numbers, ledgers), periodic processing, AI start-up, and the full login/root wiring. On November 21, 2026 the work was paused again at the user's request at 0.797 (the 0.796 full test run and 65-run start-state re-collection passed); on resuming, 0.798 comes first: the full path cleanup adapter and construction refund, priest and carrier halt and arrival, and the real host/motion wiring.

The remaining work now has a checklist per stage, made by comparison against the cpppj source: finishing **R4** session assembly (R4-1 to R4-8) → **R5** world display, camera and HUD, and the `Netstorm.exe` switch → **R6** input, rules, AI and sound (the largest stage) → **R7** full missions, Edit, settings and distribution → **R8** AGENTS phases 4–15 → **R9** 60/120 frames → **R10** Linux → **R11** local multiplayer. The last full test run (0.796) passes **Assets 366, Core 3,480 and Client 1,817** (these include existing-path tests and do not equal new-path progress). See the [handover](LEFT_JOBS.dotnetpj.md) for the order and checklists and the [cpppj → dotnetpj porting plan](docs/dotnet-cpp-port-plan-20261016.md) for the evidence list.

## Latest HTML + JavaScript web clone (webpj)

Because browsers restrict audio playback, the game loads the original assets, shows a "click to start" screen and then the main menu. The Campaign, Demo and Credits dialogs, the Early Missions, Priest Training and campaign chapter lists, the Help and Options drop-down menus (settings saved; sound, music and volume applied; language, difficulty and Frame Rate selection), the F1 General Help window, the startup welcome/tip window and saving of campaign progress (`Done*`) are implemented, and eight menu scenes were compared pixel by pixel with cpppj's output. The product label is `v11.01`, and Credits has a third item, **Netstorm 11.01 Patch Credits** (`Copyright 2026 HJOW`). External links in the original data open in a new tab or window. External access by the web runtime is listed in the [external access list](docs/webpj-external-access-20261105.md). Settings and progress are stored in the browser's localStorage; Reset All at the bottom of Options clears only this game's settings and progress after confirmation and reloads. The tab icon (favicon) is the original executable icon.

The language is chosen with Language in Options or from the browser language. In Korean, the main menu, drop-down menus, dialogs, campaign lists and F1 help (126 of 128 topics) are shown using the Galmuri11 font (regular and bold woff2) and original-style language files, falling back to English when the font cannot be loaded. For missions, every script (6 tutorials, 16 campaign missions, demos and training missions), the in-game ESC menu, status messages and progress windows, and the priest, player and Workshop production menus, object context menus, the knowledge window and player names are in Korean (no missing translations in the menus and object menus); some fixed English text without translation calls, such as the production window and minimap help and chat channel names, remains.

For missions, the single-player scope of cpppj is being ported file by file. Of the 1,022 units to port from the reference cpppj edition (product 1313), **887 (86.8%)** were ported while matching them against original 10.78 comparison data; most of the remaining 135 units are the hubs that tie the parts into one session (`RawCampaignSession`, `ClientMain`, `GameWorld`, `UberGump`, `RawMenuControls`) plus native devices, game menus and sound. With the ported parts wired into the real mission app frame, **missions can now be played in the browser.** All 31 missions in the campaign lists start one after another and run for long sessions without errors, the three demos restart with a new world every 360 seconds, and Campaign ▸ Skirmish builds a world of random islands and geysers to play with AIs. In Tutorial1 and 2, selecting and moving the priest, edge scrolling, clicking and shortcut keys on the production deck with bridge placement, building and completing a temple from the priest's context menu, and producing a weapon from the Workshop's context menu were confirmed, and Tutorial2 was played naturally to its completion window. The left sidebar and minimap area are docked at their original positions, and the ESC game menu (view objectives, hide buildings, Pause, Restart/Leave Mission, the player list and more), mission scene music, Shift-F7 screenshots and the original help's control keys (Alt and middle-button scrolling, TAB/U/number screen positions, C rotation, D, the Pause key and more) work, and Replay, Next Mission and progress saving after the win/loss notice were confirmed. Next come the rest of the HUD such as the knowledge (NSInfo) and chat windows, the remaining object menus, the objectives of other missions, and the fixed-clock L2 and first-frame screen comparison of the Tutorial1 start state (the structural match of the state dump right after start is confirmed). The latest full test runs pass **all 6,702 tests** on Node v26 (E1166) and v22 (E1161). The later stages are 5 display, HUD and mission entry (M2) → 6 input, economy and construction (M3) → 7 combat, rituals and AI → 8 mission lifetime and sound (M4) → 9 display quality and deployment (M5) → 10 later requirements (the web-relevant part of phases 4–15).

By user decision, the web version does not provide custom-map Edit, the **User Made Campaigns** entry in the Campaign list, multiplayer, resolution and fullscreen settings, quitting the game and so on (phase 12 Skirmish is provided on the web as well). See the [latest handover](LEFT_JOBS.webpj.md) for the detailed status and next work.

The linked analysis and handover documents are written in Korean.

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

`cpppj` has closed the implementation of phases 1–14 (phase 14 up to embedding the engine). Phase 15 (settings reset), added to AGENTS.md on November 18, 2026, is not implemented yet. User checks such as natural playthroughs, full parity and actually running multiplayer remain open. It is Windows only (targeting x86-64, with Windows 11 on ARM in mind). Multiplayer-only Outposts belong to phase 3; official-server access is not restored. CD version 10.72 was reconstructed up to the menus and then put on hold, and its comparison and analysis records are retained.

`dotnetpj` uses C# and MonoGame. It began by moving the parts verified by `cpppj` into the existing clone piece by piece (the October 16 porting plan), but because much of the existing implementation differs from the original, the plan was redone on October 28. The approximate existing implementation is frozen and replaced by a new `Netstorm.Client` that carries over cpppj's structure (8-bit screen, Gump UI, world session), with the cpppj executable serving as the comparison reference for identical inputs. On November 15, 2026 the remaining work was reorganized, by comparison against the entire cpppj source, into checklists for stages R4–R11. Matching the original behavior comes first; cpppj's phase 4–15 goals are then added as later requirements. The Windows executable is named `Netstorm.exe`. GUI Linux support (saving under `$HOME/.netstorm/`) and multiplayer have lower priority. Final requirements include fullscreen, 16:9/16:10/4:3 aspect ratios, fullscreen edge scrolling, and English/Korean support. Animation first targets the original cadence, with 60/120-frame support afterwards.

`webpj` uses HTML and JavaScript. Aiming at the original behavior, it ports the single-player scope of `cpppj` file by file: a pinned cpppj baseline is ported first and later changes are judged and re-synced separately. Custom maps (Edit), User Made Campaigns, multiplayer and features that do not fit the web (quitting the game, resolution and fullscreen settings, and so on) are excluded, and required assets are copied into `webpj/www/`. Of cpppj's phase 4–15 goals, those that apply to the web are later requirements (phase 4 as WebMCP, phase 5 with localStorage, phase 7 as browser console output, phase 15 reloading instead of quitting, and including phase 12 Skirmish); phases 8, 9 and 14 are excluded.

| Path | Purpose |
| --- | --- |
| `cpppj/` | C++20/Win32 reconstruction and tests (main line) |
| `dotnetpj/` | C#/MonoGame DesktopGL clone and tests |
| `webpj/` | HTML + JavaScript web clone (`www/`), static web server, tests and asset preparation tools |
| `assets/game-data/` | Clone graphics, maps, missions, music and sound effects |
| `fonts/` | The Galmuri11 font for Korean text (dotnetpj's existing path still uses D2Coding), and Noto Sans family fonts for Japanese and other languages |
| `docs/` | Analysis evidence, reconstruction, build, gameplay and verification notes |
| `tools/`, `analyzeManager/` | Static analysis, original-game observation and verification tools |
| `originals/`, `originalCD/` | Original version 10.78 and CD version 10.72 analysis material |
| `original1037/`, `originalPatches/` | Original version 10.37 comparison material and the 10.37 → 10.62 patch (NSP1062) |
| `extracted/` | Refined decompilation output and raw build/test logs. Not in Git, so present only on the PC that produced it. |
| `screenShots/`, `playingVideos/`, `originalFanGame/` | Screenshot, recording and fan-game references. Large recordings and fan-game code are excluded from Git and may be absent on a given PC. |

Korean building and unit names follow the [shared naming policy](docs/korean-unit-names.md). Priest, Temple, Bridge, Thrower, Geyser, Altar, Residence and Empty may be translated; other names use Korean phonetic spellings, including Workshop → 워크샾 and Generator → 제네레이터. cpppj's translation table (`xlat.korean`, 502 entries) and dotnetpj's menus and unit titles (the shared 55-entry table) use the same names. cpppj and webpj have translated their menus, help and mission scripts into Korean (a full human review of the text is left to the user); the Korean screens of dotnetpj's new path are wired up in R8 phase 6.

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

Node.js v22 or later is required (the full test suite passes on v22 and v26); no external npm packages are used. The required assets are committed in `webpj/www/data/`. From the repository root, start the static web server and open `http://localhost:9803/` in a browser:

```sh
npm --prefix webpj start
```

Run the automated tests with `npm --prefix webpj test`. If the original data in `assets/game-data/` changes, rebuild `webpj/www/data/` with `npm --prefix webpj run prepare-assets` and check the copies with `npm --prefix webpj run check-assets`. For development you can combine the URL parameters `?lang=ko` (Korean), `?mute` (no sound) and `?aspect=16:9` (aspect ratio); `?map=savetheisland` opens a development view that shows only the terrain, without game rules.

# Key documents

- [C++/shared handover](LEFT_JOBS.md) / [C# handover](LEFT_JOBS.dotnetpj.md) / [web handover](LEFT_JOBS.webpj.md): current state and upcoming work. Detailed history of earlier units is in [LEFT_JOBS.old.md](LEFT_JOBS.old.md) (cpppj/shared), [LEFT_JOBS.dotnetpj.old.md](LEFT_JOBS.dotnetpj.old.md) (dotnetpj) and [LEFT_JOBS.webpj.old.md](LEFT_JOBS.webpj.old.md) (webpj).
- [cpppj M8 final distribution gate report](docs/exe/cpp-m8-final-gate-1310-20261114.md): build, ZIP, distribution run and external-access audit results for product 1310
- `tools/port_inventory.py`: a tool that tallies, per area, how much of the cpppj source remains to be ported to dotnetpj and webpj (`python -X utf8 tools/port_inventory.py`). It regenerates the per-stage remaining-work tables in the two handovers.
- [TODO.md](TODO.md): the goal prompts for cpppj and dotnetpj (instructions to carry each handover's work through to the end, and which tests to skip)
- [C++ roadmap](docs/cpp-roadmap.md) / [playable reconstruction plan](docs/cpp-playable-plan.md): development stages and reconstruction plans
- [cpppj → dotnetpj porting plan](docs/dotnet-cpp-port-plan-20261016.md): porting units (W numbers) and the evidence list. The current order (R0–R11) and stage checklists in section 1 of the [dotnetpj handover](LEFT_JOBS.dotnetpj.md) take precedence.
- [Clone game data](assets/README.md): asset layout, provenance and build inclusion
- [Original-game/separate-window execution rules](docs/execution-confirmation.md): execution conditions for AI work

# License

MIT License
Copyright (c) 2026 HJOW
