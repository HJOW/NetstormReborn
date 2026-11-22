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

**As of November 23, 2026:** `cpppj`, the main line, is at product 1326 (displayed as version 11.01). Phases 1–15 of AGENTS.md are implemented (phase 15, settings reset, was added on November 22), and the final distribution gate (M8, unit 1327) was passed again on product 1326, closing the work that can proceed automatically; what remains are user checks and optional work that needs a user decision. `dotnetpj` resumed on November 22, switched its direct-translation host to `Netstorm.Game`/`Netstorm.exe` at 0.807 and, after porting the HUD, minimap, production panel, color tables and file I/O through 0.837, was paused at the user's request on November 23. The last full run, at 0.836, passed 5,942 tests; work resumes with R4 world session assembly (0.838). `webpj`, up to WB04-E1237, can run all 31 missions in the campaign lists, the demos and Skirmish in the browser, has connected the chat window and the original main loop's frame processing (bridge decay, battle resources, ambient sound), and is implementing the remaining differences found by read-only audits (such as victory/defeat music), the remaining object menus and mission objectives. Per-unit evidence is in the [cpppj/common handover](LEFT_JOBS.md), [dotnetpj handover](LEFT_JOBS.dotnetpj.md) and [webpj handover](LEFT_JOBS.webpj.md).

| Project | Execution and playability | Fidelity and remaining work |
| --- | --- | --- |
| **cpppj — C++ reconstruction (main line)** | **Playable.** The original 10.78 menus, 6 Early Missions, 9 Priest Training missions, 16 campaign missions and 3 demos (34 routes), Edit (including scenario editing) and Skirmish can be played. It includes English/Korean/Japanese UI, 16:9 up to 1920×1080 (2560×1440 and 3840×2160 through FSR 1.0), difficulty, Frame Rate 60/75/120, settings reset (Reset All), a local MCP server, and TCP/IP local multiplayer with Outposts. The distribution ZIP runs right after extraction. | Each part was reconstructed by comparison with the original 10.78 machine code, and the implementation of AGENTS.md phases 1–15 is closed (phase 14 up to embedding the QuickJS engine). The M8 distribution gate on product 1326 (unit 1327) checked all 1,437 ZIP entries and ran entry into all 34 routes, Credits in three languages and Skirmish. Natural playthroughs of every mission and full parity, actually running multiplayer (no runtime verification so far), human review of Korean and Japanese, and real 4K / other monitors and DPI / Windows 11 and ARM hardware are left to the user. All 34 known bugs have been judged (10 fixed); 12 need more reproduction details. Reconstruction of CD version 10.72 was confirmed up to the menus and then put on hold. |
| dotnetpj — C# + MonoGame clone | **Netstorm.Game (`Netstorm.exe`)** is the sole executable project. The title, menus, Help, Options, Credits, campaign selection, the Edit list and, through the development `--mission` option, session loading, AI creation and periodic processing run. The former approximate game path was removed; world display, input and rules remain, so **gameplay is not available yet**. Work was paused at the user's request after 0.837 on November 23, 2026. | Twelve menu scenes matched cpppj pixels. There are 452 registered data sets, and full mission start-state agreement remains **0/34**. The last full run, at 0.836, passed Assets 376, Core 3,557 and Client 2,009 tests (5,942 in total). See the [DN handover](LEFT_JOBS.dotnetpj.md) for current scope and verification. |
| webpj — HTML + JavaScript web clone | After "click to start", the main menu is shown with original assets. Campaign, Demo, Credits (three items including 11.01 Patch Credits), Help, Options (settings saved; language, difficulty and Frame Rate selection; Reset All) and F1 help work, and in Korean the menus and all mission scripts are shown with the Galmuri11 font. **Missions can be played in the browser:** all 31 missions in the campaign lists (6 Early Missions, 9 Priest Training, 16 campaign) start and run for long sessions without errors, and the three demos' 360-second cycle and Skirmish (random islands and geysers, number of AIs and banned units) were confirmed. Tutorial2 was played naturally from the start to its completion window, and the ESC game menu (view objectives, Pause, Restart/Leave Mission and more), shortcut keys and mission music work. The original help's control keys (screen positions, rotation, D, Pause and more) and the flow after the win/loss notice (Replay, Next Mission, progress saving) were also confirmed. The F3 chat window (input, submission, dot commands, name completion), the original main loop's bridge decay, battle resources (geyser replenishment, periodic SP) and ambient sound, and right-click quick help on the HUD also work. The remaining differences found by audits (such as victory/defeat music), the remaining object menus, and the objectives and completion of other missions are still open (the knowledge (NSInfo) panel is outside the web scope because single-player missions do not have it). | Eight menu scenes have zero pixel difference from cpppj's output (apart from the cloud background, whose cause is recorded). Of the 1,007 units to port, **973 (96.6%)** were ported while matching them against original 10.78 comparison data, and 34 remain partially ported. The Tutorial1 state dump right after start was confirmed to match the cpppj baseline structurally, excluding time-dependent values; fixed-clock L2, L3, screen comparisons and natural playthroughs are not verified yet. The latest full automated test runs pass all 6,759 tests on Node v26 (E1234) and v22 (E1236). The web version does not provide Edit, User Made Campaigns, multiplayer or the CD intro video. |

## Latest C++ reconstruction (cpppj)

### What you can do now

- **34 mission routes:** the 6 Early Missions, 9 Priest Training missions, 16 campaign missions (chapters 1–3) and 3 demos can be entered and played. The user played Campaign 1-1 and 1-2 to the end, up to the next-mission prompt, and the three demos were confirmed to repeat on their 360-second cycle. The last time entry into all 34 routes was re-checked together was the distribution gate for product 1326 (unit 1327).
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
- **Settings reset (phase 15):** Reset All at the bottom of Options resets settings and campaign progress and quits normally only after confirmation in an English, Korean or Japanese dialog. The next launch starts in the first-run state, with the first welcome windows and campaign locks, while custom maps, logs and installed files are kept ([Reset All](docs/exe/cpp-reset-all-1325-20261122.md)).
- **Windows 10/11:** the original's error on restarting after leaving fullscreen does not occur. The distribution runs right after extraction. The executable icon matches the original (1323).

### Recent progress

- **1324–1327 (November 22, 2026):** remaining test expectations were corrected to match the current devices and contracts (1324), phase 15 settings reset (Reset All) was implemented (1325), and test autoplay can now observe the AI body and damage diagnostics (1326; normal-run dumps are unchanged). The M8 distribution gate was then passed again on product 1326 (1327: Release build, all 1,437 ZIP entries, entry into all 34 routes from the extracted archive, Credits in three languages and Skirmish, QuickJS self-test 6/6, icon and external-access static audits; [report](docs/exe/cpp-m8-final-gate-1327-20261122.md)).
- **1314–1323 (November 16–19, 2026, resumed at the user's instruction):** the first-run state (default volumes, first welcome windows, campaign locks, 1314), a mistranslation in Korean unit information (1315), a fix for a reported exit from the previous connection on mission transition (1320), switching the Korean font to Galmuri11 bitmaps (1322), and the original executable icon with a shared ICO (1323). 1316–1319 and 1321 corrected test baselines.
- **1312–1313 (November 14, 2026):** command-line-only test autoplay (`--test-autoplay` and related options: an AI is attached to the local player to play campaign and skirmish games automatically, with a spectator camera) and a batch runner were added, and a Campaign 1-1 victory was reproduced. None of this appears in normal runs, options or menus ([usage guide](docs/testing/cpp-autoplay-test-guide.md)).
- **1311 (M8 final distribution gate):** the Release build, a full comparison of the ZIP's 1,436 entries, running the distribution on five routes (English, Korean and Japanese menus, a mission, a skirmish) and the external-access audit all passed ([final report](docs/exe/cpp-m8-final-gate-1310-20261114.md)). The current reference is 1327 above.
- **1302–1310 (CD version 10.72):** the edition option, default resolution, messages, CD data path and menu display were reconstructed, and the work was then put on hold by user decision ([deferral handover](docs/exe/cpp-cd1072-deferred-20261114.md)).
- **1285–1301:** judgement of the 34 known bugs with 10 fixes ([status table](docs/exe/cpp-known-bugs-status-20261111.md)), the Japanese UI, the 11.01 copyright line, difficulty, Skirmish, Frame Rate 60/120, and embedding the QuickJS engine.
- **987–1284:** phase 3 LAN multiplayer and Outposts implemented and statically compared (up to 1279), the error log (1269), Korean K1–K5 (1270–1281, [closure](docs/exe/cpp-korean-k4-k5-closure-20261111.md)), and scenario editing S1–S4 (1277–1284, [read-only install check](docs/exe/cpp-scenario-s4-readonly-home-20261111.md)).
- **Earlier (up to 986):** closure of the non-multiplayer UI (929), Edit (943), mission lifetime (956) and internal lifetime/files (980); phase 4 MCP and wide screen, phase 5 save locations, anti-aliasing and FSR 1.0. Detailed history of past units is in [LEFT_JOBS.old.md](LEFT_JOBS.old.md).

### What remains

After pausing on November 14, 2026, the goal resumed on November 17 at the user's instruction, and unit 1327 on November 22 closed the work that can proceed automatically; there is no pending AI task. What remains is ① user checks (natural playthroughs of every mission and full parity; actually reproducing capture, sacrifice, specific enemy attacks and victory; running multiplayer; reviewing Korean and Japanese; the Explorer icon of the extracted distribution; real 4K, other monitors and DPI, Windows 11 and ARM) and ② optional work that needs a user decision (resuming the CD version 10.72 reconstruction, flyer display interpolation, translating Japanese mission text and adding languages, reproduction details for 12 known bugs, the scope of QuickJS integration). New reports reopen the relevant item. Details are in the [handover](LEFT_JOBS.md).

## Latest C# + MonoGame clone (dotnetpj)

On November 22, 2026, at the user’s request, the former approximate executable was removed and the direct-translation host became **Netstorm.Game**, producing **Netstorm.exe**. The internal `Netstorm.Client` library keeps the8-bit logical screen, palette and source call order. MonoGame supplies the window, input/audio devices and final output. The executable-name migration is complete; world gameplay remains in R4–R7.

So far R0 (baseline and port map), R1 (8-bit screen host and comparison tools) and R2 (drawing primitives, palettes, fonts) are done, and in R3 (Gump UI and menus) 12 menu scenes are pixel-identical to cpppj. **R4 (world session assembly) is in progress:** mission Fort loading, players, alliances, terrain, islands, fort objects, the graph and the common object-lifetime layer (factory, Pop/Unpop, deletion, damage, burning, priests, force fields) are connected to the host. Units 0.639–0.688 ported the AI runtime (creation, preparation, strategy, the full frame, notices, world registration, collectors, economy, facility production), the decoration, player, sprite and burial periodic bodies, upgrades, research and the production deck, and the flare and construction display devices; units 0.689–0.731 ported internal login, packet transport, client globals, the long-lived lifetimes of runtime objects, players and state, and the client frame execution order; units 0.733–0.797 ported state cleanup and return, root resources and selection, the cursor, camera and renderer owners, the whole chat family (manager, command table, submission, ChatWindow, receiving) with root receive and transport observers, the object packet receive bodies and the path search and processing bodies. All of these follow the original order and are wired into the session. Baselines for comparing the mission start state with cpppj exist for all 34 routes, but no route matches yet (0/34). The remaining differences are post-processing during loading (farmland frames and owners, graph numbers, ledgers), periodic processing, AI start-up, and the full login/root wiring. Work resumed on November 22: path cleanup and refunds, halt/arrival, Host/Motion and the 31h developer commands were connected, and 0.807 switched the executable project. Units 0.809–0.837 then ported island-cleanup debris and 35h, player-specific deletion with first device installation and the AI wallet, HUD docking with the full HUD parent and divider input, the minimap, NSInfo, the production panel and owner color tables, and file I/O, search and the CRT. Work was paused after 0.837 at the user's request on November 23; on resuming, the real Load/Editor installation parents and the root and production factories (0.838), the remaining 8 of 26 fixed gameplay receive bodies, and world display and input follow.

The remaining work now has a checklist per stage, made by comparison against the cpppj source: finishing **R4** session assembly (R4-1 to R4-8) → **R5** world display, camera and HUD → **R6** input, rules, AI and sound (the largest stage) → **R7** full missions, Edit, settings and distribution → **R8** AGENTS phases 4–15 → **R9** 60/120 frames → **R10** Linux → **R11** local multiplayer. The last full test run (0.836) passes **Assets 376, Core 3,557 and Client 2,009** (5,942 in total; these include part-level and original-comparison tests and do not equal gameplay progress). See the [handover](LEFT_JOBS.dotnetpj.md) for the order and checklists and the [cpppj → dotnetpj porting plan](docs/dotnet-cpp-port-plan-20261016.md) for the evidence list.

## Latest HTML + JavaScript web clone (webpj)

Because browsers restrict audio playback, the game loads the original assets, shows a "click to start" screen and then the main menu. The Campaign, Demo and Credits dialogs, the Early Missions, Priest Training and campaign chapter lists, the Help and Options drop-down menus (settings saved; sound, music and volume applied; language, difficulty and Frame Rate selection), the F1 General Help window, the startup welcome/tip window and saving of campaign progress (`Done*`) are implemented, and eight menu scenes were compared pixel by pixel with cpppj's output. The product label is `v11.01`, and Credits has a third item, **Netstorm 11.01 Patch Credits** (`Copyright 2026 HJOW`). External links in the original data open in a new tab or window. External access by the web runtime is listed in the [external access list](docs/webpj-external-access-20261105.md). Settings and progress are stored in the browser's localStorage; Reset All at the bottom of Options clears only this game's settings and progress after confirmation and reloads. The tab icon (favicon) is the original executable icon.

The language is chosen with Language in Options or from the browser language. In Korean, the main menu, drop-down menus, dialogs, campaign lists and F1 help (126 of 128 topics) are shown using the Galmuri11 font (regular and bold woff2) and original-style language files, falling back to English when the font cannot be loaded. For missions, every script (6 tutorials, 16 campaign missions, demos and training missions), the in-game ESC menu, status messages and progress windows, and the priest, player and Workshop production menus, object context menus, the knowledge window and player names are in Korean (no missing translations in the menus and object menus); some fixed English text without translation calls, such as the production window and minimap help and chat channel names, remains.

For missions, the single-player scope of cpppj is being ported file by file. Of the 1,007 units to port from the reference cpppj edition (product 1313), **973 (96.6%)** were ported while matching them against original 10.78 comparison data; most of the 34 partially ported units that remain are the hubs that tie the parts into one session (`RawCampaignSession`, `ClientMain`, `GameWorld`, `UberGump`, `RawMenuControls`) plus client state cleanup, HUD controls, help and sound. With the ported parts wired into the real mission app frame, **missions can now be played in the browser.** All 31 missions in the campaign lists start one after another and run for long sessions without errors, the three demos restart with a new world every 360 seconds, and Campaign ▸ Skirmish builds a world of random islands and geysers to play with AIs. In Tutorial1 and 2, selecting and moving the priest, edge scrolling, clicking and shortcut keys on the production deck with bridge placement, building and completing a temple from the priest's context menu, and producing a weapon from the Workshop's context menu were confirmed, and Tutorial2 was played naturally to its completion window. The left sidebar and minimap area are docked at their original positions, and the ESC game menu (view objectives, hide buildings, Pause, Restart/Leave Mission, the player list and more), mission scene music, Shift-F7 screenshots and the original help's control keys (Alt and middle-button scrolling, TAB/U/number screen positions, C rotation, D, the Pause key and more) work, and Replay, Next Mission and progress saving after the win/loss notice were confirmed. The F3 chat window (log, input line, submission, dot commands, Tab name completion, paste/copy), the Send X money dialog, right-click quick help on the HUD and the Review Knowledge/GetTechnology buttons in briefings are also connected, and the original main loop's bridge-decay check, battle resources (geyser replenishment, periodic SP), altar result notices and ambient sound/wind run in the app frame. The knowledge (NSInfo) panel was judged outside the web scope because it is a multiplayer/meta preparation window that single-player missions do not have. Next come the remaining differences found by read-only audits (no victory/defeat music, unit information in the main-menu Help and others), the remaining object menus, the objectives of other missions, and the fixed-clock L2 and first-frame screen comparison of the Tutorial1 start state (the structural match of the state dump right after start is confirmed). The latest full test runs pass **all 6,759 tests** on Node v26 (E1234) and v22 (E1236). The later stages are 5 display, HUD and mission entry (M2) → 6 input, economy and construction (M3) → 7 combat, rituals and AI → 8 mission lifetime and sound (M4) → 9 display quality and deployment (M5) → 10 later requirements (the web-relevant part of phases 4–15).

By user decision, the web version does not provide custom-map Edit, the **User Made Campaigns** entry in the Campaign list, multiplayer, the CD intro video, resolution and fullscreen settings, quitting the game and so on (phase 12 Skirmish is provided on the web as well). See the [latest handover](LEFT_JOBS.webpj.md) for the detailed status and next work.

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

`cpppj` has closed the implementation of phases 1–15 (phase 14 up to embedding the engine; phase 15, settings reset, added to AGENTS.md on November 18, 2026, was implemented on November 22). User checks such as natural playthroughs, full parity and actually running multiplayer remain open. It is Windows only (targeting x86-64, with Windows 11 on ARM in mind). Multiplayer-only Outposts belong to phase 3; official-server access is not restored. CD version 10.72 was reconstructed up to the menus and then put on hold, and its comparison and analysis records are retained.

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

Build a distributable archive (executable, Visual C++ runtime DLLs, original 10.78 game data, the Korean font and translation table, FSR 1.0 headers and license, and launcher batch files) with the script below. If you omit the output path it prompts for one, and an empty answer cancels. See section 7 of [C++ build](docs/cpp-build.md) for the layout and options. On the receiving PC, extract the archive and run `Netstorm.exe` or `Netstorm-SavedMode.bat` (saved display mode), or `Netstorm.bat` (forces windowed mode). Settings, progress, captures and other files are stored in `%USERPROFILE%\Saved Games\Netstorm\` rather than the install folder, so the extraction folder may be read-only. To reset settings and progress, use Reset All at the bottom of Options (to clear only the settings, you can also delete `options.cfg` and `fullscreenStateFile.dat` in `%USERPROFILE%\Saved Games\Netstorm\Saves\`). The default incremental build-to-ZIP path in Windows PowerShell 5.1 and fresh extraction with first run, saved fullscreen restart and the windowed launcher have been checked, along with extracted runs of the unit 645 distribution (product 644, 1,376 entries) and the unit 938 distribution (with the Korean font and translation table, 1,379 entries). For the latest product 1326, the 1327 gate checked all 1,437 ZIP entries and ran 38 routes from the extracted archive (31 regular missions, 3 demos, Credits in three languages and Skirmish). Windows 11 distribution validation remains separate.

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

Node.js v22 or later is required (the full test suite passes on v22 and v26); no external npm packages are used. The required assets are committed in `webpj/www/data/`. From the repository root, start the static web server and open `http://localhost:9803/` in a browser:

```sh
npm --prefix webpj start
```

Run the automated tests with `npm --prefix webpj test`. If the original data in `assets/game-data/` changes, rebuild `webpj/www/data/` with `npm --prefix webpj run prepare-assets` and check the copies with `npm --prefix webpj run check-assets`. For development you can combine the URL parameters `?lang=ko` (Korean), `?mute` (no sound) and `?aspect=16:9` (aspect ratio); `?map=savetheisland` opens a development view that shows only the terrain, without game rules.

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
