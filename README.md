Netstorm - Reborn
-------------------------------------------------------------------------

**한국어** | [English](README.en.md)

# 개요 
Activision 사에서 1997년도에 개발 후 한참 전에 손 놓은
Netstorm - Islands at war 게임을
AI 를 이용해 되살리는 프로젝트입니다.

# 현재 개발 진행상황 관련

현재 **C# + MonoGame 빌드**에서는 **메인 메뉴 → 캠페인 → 자유를 위한 투쟁 → 1-1 전쟁의 시작!**과 **1-2 휘리기그의 지배자(Master of Whirligigs)**를 플레이할 수 있습니다.
옵션에서 해상도·창/전체화면·소리·음량·바람 소리·스피커 좌우 교환을 바꿀 수 있습니다. 메인 메뉴의 도움말과 F1에서 원본 도움말 전체를 탐색할 수 있으며, 사제·워크샵·생산 항목의 우클릭 메뉴도 사용할 수 있습니다. [녹화 분석의 클론 적용 범위](docs/screens/recorded-ui-clone-20261002.md)를 참고하세요. 다른 미션과 미구현 메뉴는 잠금 표시합니다.
`dotnet run --project dotnetpj/src/Netstorm.Game -c Release -- --language korean`으로 시작합니다(영어: `--language english`).
게임 데이터는 `assets/game-data/`에서 관리하며 빌드·배포 출력에 자동으로 포함됩니다. 임시 AI·건설 규칙과 검증 한계는 [캠페인 1-1 구현](docs/gameplay/campaign-one.md)과 [캠페인 1-2 구현](docs/gameplay/campaign-two.md)에 정리했습니다.

# 프로젝트 구성

게임 빌드가 둘 있으며 현재는 cpppj의 원본 게임 플레이 복원을 먼저 진행합니다. dotnetpj의 새 개발은 cpppj 완성 뒤 이어 갑니다.

| 폴더 | 내용 | 상태 |
| --- | --- | --- |
| `dotnetpj/` | **C# + MonoGame 빌드.** 원본을 분석해 새로 구현한 클론입니다. | 위의 캠페인 1-1·1-2를 플레이할 수 있습니다. |
| `cpppj/` | **C++ 빌드.** 기존 게임 10.78을 디컴파일한 소스를 토대로 C++ 소스를 다시 만듭니다. | Windows 전용. 메뉴→캠페인→브리핑→지형/객체 표시·선택/사제 이동·복귀를 연결했습니다. raw 다리 계산·붕괴/삭제 훅·일반 탐색·공통 삭제/공간 해제/SID 반납과 공통 pre/post의 선택·목록·통계·비용 장부를 복원했습니다. 공통 삭제의 Graph 분할/주변 표면 Add와 삭제 보상(SP 지급·AI 지갑·샘 풀, 패치판 SP 저장소 포함)을 연결했습니다. 객체에 붙는 프로세스(Kernel 슬롯·form SID)와 지연/주기 이벤트, 다리 지연 낙하 예약도 복원했습니다. AI 부착 통지·이벤트 처리기 몸체와 다른 파생 프로세스·실제 월드 연결·건설/전투·미션 완주는 후속입니다. |

게임 데이터(`assets/game-data/`), 글꼴(`fonts/`), 분석 문서(`docs/`), 분석 도구(`tools/`, `analyzeManager/`)는 두 빌드가 함께 쓰며 저장소 루트에 있습니다. C++ 빌드의 구조와 빌드 방법, 디컴파일 결과에서 소스를 만드는 절차는 [C++ 빌드](docs/cpp-build.md)를 참고하세요.

# 빌드 및 실행 파일 만들기 (C# + MonoGame 빌드)

아래 명령은 프로젝트 루트(`NetstormReborn`)에서 실행합니다. 게임 본체는 C# / MonoGame DesktopGL이며 대상 프레임워크는 `net10.0`입니다.

## 준비

- **.NET 10 SDK**를 설치하고 `dotnet --version`으로 확인합니다. `dotnetpj/global.json`은 10.0.100 이상인 .NET 10 SDK의 최신 기능 버전을 허용합니다.
- 첫 빌드·배포 시 NuGet 패키지와 대상 OS의 런타임을 내려받을 인터넷 연결이 필요합니다.
- 저장소의 `assets/game-data/`와 한국어 글꼴 `fonts/D2Coding-Ver1.3.2-20180524-all.ttc`를 함께 체크아웃합니다. 데이터·글꼴은 빌드·배포 출력에 자동으로 복사됩니다.
- 실행 대상은 **Windows 10/11 x64** 또는 **GUI 환경의 x64 Linux(glibc 기반)**입니다. OpenGL을 지원하는 그래픽 드라이버가 필요합니다.

`originals/`는 기존 게임을 분석하기 위한 자료입니다. 클론의 빌드·테스트·실행은 독립된 `assets/game-data/`를 사용하므로 분석 완료 후 `originals/`를 제거해도 동작합니다. 자산의 구성과 이관 기록은 [클론 게임 데이터](assets/README.md)를 참고합니다.

## 개발용 빌드와 실행

```sh
dotnet restore dotnetpj/Netstorm.sln
dotnet build dotnetpj/Netstorm.sln -c Release --no-restore
dotnet run --project dotnetpj/src/Netstorm.Game -c Release --no-build -- --language korean
```

영어로 실행하려면 `--language english`를 사용합니다. 빌드 결과는 `dotnetpj/src/Netstorm.Game/bin/Release/net10.0/`에 생성됩니다. 일반 빌드는 빌드한 OS용 실행 파일을 만들며, 다른 OS용 바이너리는 아래의 `-r` 옵션으로 만듭니다.

## 런타임을 포함한 배포용 바이너리

게임 프로젝트에 `dotnet publish`를 실행합니다. 다음 명령은 Windows PowerShell과 Linux 셸에서 모두 사용할 수 있으며, Linux에서도 Windows exe를 만들 수 있습니다.

```sh
dotnet publish dotnetpj/src/Netstorm.Game/Netstorm.Game.csproj -c Release -r win-x64 --self-contained true -o dist/win-x64
dotnet publish dotnetpj/src/Netstorm.Game/Netstorm.Game.csproj -c Release -r linux-x64 --self-contained true -o dist/linux-x64
```

| 대상 | 출력 폴더 | 실행 파일 |
| --- | --- | --- |
| Windows 10/11 x64 | `dist/win-x64/` | `NetstormClone.exe` |
| Linux x64 | `dist/linux-x64/` | `NetstormClone` |

`--self-contained true`는 .NET 런타임까지 포함하므로 실행할 PC에 .NET SDK나 런타임을 따로 설치할 필요가 없습니다. SDL2·OpenAL 네이티브 라이브러리, `fonts/`, `game-data/`도 출력에 포함됩니다. 실행 파일은 함께 생성된 DLL·런타임 파일·글꼴·게임 데이터를 사용하므로 **출력 폴더 전체를 복사해 배포**합니다. 그래픽 드라이버와 Linux GUI 환경 등 OS의 실행 환경은 별도로 필요합니다.

실행할 PC에 .NET 10 런타임이 이미 설치되어 있다면 더 작은 배포물을 만들 수 있습니다. 위 명령의 `--self-contained true`를 `--self-contained false`로 바꾸고, 출력 폴더도 `dist/win-x64-fdd` 또는 `dist/linux-x64-fdd`처럼 구분합니다.

## 포함된 게임 데이터와 바이너리 실행

빌드·배포 시 `assets/game-data/`의 내용이 출력의 **`game-data/`에 자동으로 복사**됩니다. Windows 배포 폴더는 다음과 같은 구조가 됩니다(Linux도 동일하며 실행 파일 이름은 `NetstormClone`입니다).

```text
dist/win-x64/
  NetstormClone.exe
  NetstormClone.dll
  ... (함께 생성된 DLL·런타임·설정 파일)
  fonts/
    D2Coding-Ver1.3.2-20180524-all.ttc
  game-data/
    netstorm.tarc
    d/
    music/
    sound/
```

Windows PowerShell에서 실행:

```powershell
.\dist\win-x64\NetstormClone.exe --language korean
```

Linux에서 실행:

```sh
chmod +x dist/linux-x64/NetstormClone
./dist/linux-x64/NetstormClone --language korean
```

일반 실행은 함께 배포된 데이터를 자동으로 찾습니다. 개발·검증용 데이터를 다른 위치에서 사용하려면 `NETSTORM_DATA`에 **`netstorm.tarc`가 들어 있는 폴더의 절대 경로**를 지정합니다. 탐색 순서는 유효한 `NETSTORM_DATA` → 실행 파일·작업 폴더 및 그 상위의 `game-data/` 또는 `assets/game-data/`입니다.

```powershell
$env:NETSTORM_DATA = "D:\Games\NetstormReborn\game-data"
.\dist\win-x64\NetstormClone.exe --language korean
```

```sh
NETSTORM_DATA="/절대/경로/game-data" ./dist/linux-x64/NetstormClone --language korean
```

# C++ 빌드 (복원 진행 중)

Windows 10/11, CMake 3.21 이상, C++20 컴파일러(Visual Studio 2022 이상)가 필요합니다. C++ 빌드는 원본처럼 Win32 API를 직접 부르므로 Windows 전용입니다. 프로젝트 루트에서 실행합니다.

```sh
cmake -S cpppj -B cpppj/build -DCMAKE_BUILD_TYPE=Release
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build --build-config Release --output-on-failure
```

cpppj는 **기존 게임 전체를 실제 플레이 가능하게 복원하는 것이 우선**이며 화면비·한국어·Linux·추가 기능은 후순위입니다. 현재 실행 파일(`NetstormCpp`)은 자산·설정·요새·미션 검사와 Renderer에 타입 표·기본 프레임·원본 글꼴을 제출하는 검사 장면을 제공합니다. 실제 메뉴·월드·게임 플레이는 아직 없습니다. 예: `cpppj/build/bin/Release/NetstormCpp.exe --run originals --view fonts --window`. 자세한 내용은 [C++ 빌드](docs/cpp-build.md), [실제 플레이 복원 계획](docs/cpp-playable-plan.md), [표시 기반 근거](docs/exe/cpp-renderer-reconstruction.md)에 있습니다.

# License

MIT License
Copyright (c) 2026 HJOW
