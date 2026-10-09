Netstorm - Reborn
-------------------------------------------------------------------------

[한국어](README.md) | **English**

# Overview
This project uses AI to revive
Netstorm - Islands at War,
a game developed by Activision in 1997 and abandoned long ago.

# Current development status

Right now you can play **Main Menu → Campaign → Struggle For Freedom → 1-1 The War Begins!** and **1-2 Master of Whirligigs**.
In Options you can change the resolution, windowed/fullscreen mode, sound, volume, wind noise, and left/right speaker swap. The full original help is available from Help in the main menu and with F1, and the right-click menus for the priest, workshops, and production items also work. See [how far the recording analysis has been applied to the clone](docs/screens/recorded-ui-clone-20261002.md). Other missions and menus that are not implemented yet are shown as locked.
Start the game with `dotnet run --project dotnetpj/src/Netstorm.Game -c Release -- --language english` (Korean: `--language korean`).
Game data is kept in `assets/game-data/` and is included in build and publish output automatically. The provisional AI and construction rules, and the limits of what has been verified, are described in [Campaign 1-1 implementation](docs/gameplay/campaign-one.md) and [Campaign 1-2 implementation](docs/gameplay/campaign-two.md).

The linked documents are written in Korean.

# Project layout

There are two game builds, developed side by side. The C++ build proceeds in this order: single-player reconstruction of version 10.78 → error-free execution on Windows 10/11 → TCP/IP LAN multiplayer and outposts (the official server is not restored) → added requirements, MCP and Korean language support. It is Windows only. The C# build is developed by analyzing the finished C++ build: it first aims to behave exactly like the original and adds the new requirements afterwards. Linux work on the C# build has lower priority.

| Folder | Contents | Status |
| --- | --- | --- |
| `dotnetpj/` | **C# + MonoGame build.** A clone implemented from scratch by analyzing the original. | Campaign 1-1 and 1-2 above are playable. |
| `cpppj/` | **C++ build.** C++ source reconstructed from the decompiled original version 10.78. | Windows only. Menu → campaign → briefing → terrain/object display, selection, priest movement and return are connected. Raw bridge calculations, decay/deletion hooks, general search, common destruction/spatial removal/SID release, and common pre/post selection, lists, counts and cost bookkeeping are restored. Common-deletion Graph split/neighbour Add and deletion rewards (SP payout, AI wallet, geyser pool, including the patch build's scrambled SP store) are connected. Object-attached processes (kernel slot plus form SID), delayed/periodic events and bridge delayed-fall scheduling are also restored. AI attachment notices, event-handler bodies and other derived processes, live world integration, construction, combat and mission completion are pending. |

Game data (`assets/game-data/`), the font (`fonts/`), the analysis documents (`docs/`), and the analysis tools (`tools/`, `analyzeManager/`) are shared by both builds and stay in the repository root. For the structure of the C++ build, how to build it, and how source is reconstructed from the decompiler output, see [C++ build](docs/cpp-build.md).

# Building and creating executables (C# + MonoGame build)

Run the commands below from the project root (`NetstormReborn`). The game is written in C# with MonoGame DesktopGL, and the target framework is `net10.0`.

## Prerequisites

- Install the **.NET 10 SDK** and check it with `dotnet --version`. `dotnetpj/global.json` accepts the latest feature band of the .NET 10 SDK, 10.0.100 or later.
- The first build or publish needs an internet connection to download NuGet packages and the runtime for the target OS.
- Check out `assets/game-data/` and the Korean font `fonts/D2Coding-Ver1.3.2-20180524-all.ttc` together with the repository. The data and font are copied to build and publish output automatically.
- The game runs on **Windows 10/11 x64** or **x64 Linux with a GUI (glibc-based)**. A graphics driver with OpenGL support is required.

`originals/` holds material for analyzing the original game. Building, testing, and running the clone use the separate `assets/game-data/`, so the clone still works if `originals/` is removed once the analysis is finished. For the layout of the assets and the migration record, see [Clone game data](assets/README.md).

## Development build and run

```sh
dotnet restore dotnetpj/Netstorm.sln
dotnet build dotnetpj/Netstorm.sln -c Release --no-restore
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

You need Windows 10/11, CMake 3.21 or later and a C++20 compiler (Visual Studio 2022 or later). The C++ build calls the Win32 API directly, as the original does, so it is Windows only. Run these from the project root.

```sh
cmake -S cpppj -B cpppj/build -DCMAKE_BUILD_TYPE=Release
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build --build-config Release --output-on-failure
```

For now the executable (`NetstormCpp`) offers inspection commands (assets, configuration, forts, missions, frame export) and an inspection view that shows the type table or a mission's objects in an original-style window. There is no game screen yet. Examples: `cpppj/build/bin/Release/NetstormCpp.exe --inspect-mission originals TEST01`, `cpppj/build/bin/Release/NetstormCpp.exe --run originals --view TEST01`. Details are in [C++ build](docs/cpp-build.md) and the [reconstruction roadmap](docs/cpp-roadmap.md).

# License

MIT License
Copyright (c) 2026 HJOW
