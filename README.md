Netstorm - Reborn
-------------------------------------------------------------------------

**한국어** | [English](README.en.md)

# 개요 
Activision 사에서 1997년도에 개발 후 한참 전에 손 놓은
Netstorm - Islands at war 게임을
AI 를 이용해 되살리는 프로젝트입니다.

# 현재 개발 진행상황

**cpppj·dotnetpj 모두 2026-10-16 인수인계 기준**입니다. 실행·플레이 가능 범위와 원본 게임과의 동일성은 아래와 같습니다.

| 프로젝트 | 실행·플레이 가능 범위 | 원본과의 동일성 및 남은 범위 |
| --- | --- | --- |
| `cpppj/` — C++ 복원판 | Windows 창 모드에서 메뉴 → 캠페인 → 브리핑 → 요새·지형·객체·안내 표시, 선택·사제 이동·일부 실제 교전과 메뉴 복귀가 동작합니다. | 10.78의 디컴파일·기계어 대조를 바탕으로 복원 중입니다. **Construct는 비활성**이며 건설·생산·다리·수확·운반·희생, GUI의 공통 Start/StartDemo·AI 전략 프레임, 실제 미션 단계·승패 후 저장 연결이 남아 있습니다. Early Missions와 캠페인 1-1·1-2의 자연스러운 완주 및 전체 그래픽·음향 동일성은 미완료입니다. |
| `dotnetpj/` — C# + MonoGame 클론 | 캠페인 **1-1 전쟁의 시작!(The War Begins!)**, **1-2 훨리기그의 지배자(Master of Whirligigs)**의 플레이와 건설·생산·이동·전투 조작을 구현했습니다. 영어·한국어 UI, 도움말·우클릭 메뉴·옵션을 제공합니다. Early Missions(튜토리얼)와 데모는 아직 메뉴에서 열리지 않습니다. | 원본 자료를 재사용하면서, cpppj가 원본 기계어로 검증한 규칙을 옮기는 중입니다. 지금까지 공통 미션 프레임·플레이어 참여/방향별 동맹·누적 장부·튜토리얼 1 판정을 원본 기대값(fixture 14개)에 맞췄습니다. 건설·전투·경로·SP·AI·시간 규칙은 아직 관찰 기반 근사입니다. 나머지 cpppj 결과를 차례로 옮기는 [이식 계획](docs/dotnet-cpp-port-plan-20261016.md)을 세웠습니다. 실제 완주 → 진행 저장 → 재실행 → 다음 미션 해제의 전체 경로와 원본 동일성은 추가 검증이 필요합니다. |

## C++ 복원판의 최신 상태

같은 실제 세션에 SP·기술·사제 참여·AI 생성/초기 갱신과 공통 미션 Start를 연결했습니다. Early Missions1~6·캠페인1-1/1-2 및 데모3개는 원본 자원의 헤드리스 Load/Start 검사를 통과했습니다. GUI의 공통 Start/StartDemo·Guide·미션 프레임 연결과 AI 전략·자연 완주는 남아 있습니다.

2026-10-16 현재 Release 게임·검사 빌드는 **경고0·오류0**입니다. 데모3개 모두 Load/Start를 통과했고, **11미션 중9개가400프레임** 진행했습니다. 썬 바리케이드 연결 생성/삭제에 이어 맨 오 워의 실제 최초 후처리·그림자·공통 비행 프로세스 생성/홈 연결/상태 전송도 연결했습니다. 전체 CTest는 **1372개 중1370통과·2실패**로, 데모1의 미복원 비행 Frame 본문·데모3의 다른 총 애니메이션에서 멈춥니다([상세](docs/exe/cpp-flyer-postpop-20261016.md)). 원본1421개 비교와 실제 자원 생성 검사는 통과했으며 자연 완주·전체 그래픽/음향 동일성은 아직 미검증입니다. 기존 DirectDraw 전체화면 오류 수정은 현재 Early Missions·캠페인1-1~1-2·데모3개 복원 작업에서 제외되어 있으며, **cpppj 2차 목표에서 반드시 수정합니다**(AGENTS.md 2026-10-16 변경).

완주 동일성 개발은 **재개하여 진행 중**입니다. 다음 순서와 미완료 경계는 [cpppj·공통 인수인계](LEFT_JOBS.md)를 따릅니다. 최근 근거는 [데모 초기화](docs/exe/cpp-demo-initialization-20261016.md), [AI 색 변경](docs/exe/cpp-player-color-20261016.md), [맨 오 워 풀](docs/exe/cpp-rain-aviary-20261016.md), [데모/캠페인 대응](docs/gameplay/demo-campaign-map.md)에 있습니다.

## C# + MonoGame 클론의 최신 상태

메인 메뉴의 캠페인 목록은 원본 스크립트의 장·선행 완료 조건과 저장된 진행값을 읽고, 실제 지원 미션을 제한합니다. 옵션에서는 해상도·창/전체화면·소리·음량·바람 소리·스피커 좌우 교환을 바꿀 수 있습니다. 도움말과 F1에서 원본 도움말 전체를 탐색하고, 사제·워크샾·생산 항목의 우클릭 메뉴를 사용할 수 있습니다. [기능·진행 저장 검토](docs/dotnet-review-20261011.md), [녹화 분석 적용 범위](docs/screens/recorded-ui-clone-20261002.md), [캠페인 1-1](docs/gameplay/campaign-one.md)·[1-2 구현](docs/gameplay/campaign-two.md)을 참고하세요. 초기 구현 문서보다 최신 인수인계가 우선합니다.

2026-10-15에는 cpppj의 원본 대조 결과를 옮겨 다음 네 가지를 세션에 연결했습니다. 공통 미션 프레임 게이트(원본 328행), 플레이어 참여와 방향별 동맹(608행), 누적 제작 장부(52행), 튜토리얼 1 단계 판정기(266행)입니다. 사람 쪽 `myAllyList`는 원본 실행 파일에 없는 키라 읽지 않도록 바꿨으며, 이 때문에 원본 미션 21개의 동맹 결과가 달라졌습니다. 승인 뒤 실행한 클론 창 스모크와 튜토리얼 2 화면 경로의 헤드리스 완주도 통과했습니다. 2026-10-16 기준선은 Release **경고 0·오류 0**, 단위 **1,057개 통과**(Assets 343·Core 714)입니다.

다음 작업은 [cpppj → dotnetpj 이식 계획](docs/dotnet-cpp-port-plan-20261016.md)을 따릅니다. 먼저 cpppj의 실제 세션 구조에 해당하는 원본 월드 코어를 만듭니다. 객체 생성·등록·삭제, Kernel/Regular 시간, SP 저장소와 난수가 그 범위입니다. 이 코어로 11개 목표 미션(Early Missions 1~6·캠페인 1-1·1-2·데모 3개)의 시작 상태를 cpppj와 맞춥니다. 이어 사제·경로·건설·전투·희생·승패와 표시·UI·소리를 차례로 옮기고 자연 완주로 검증합니다. AI 전략처럼 cpppj에서도 아직 복원되지 않은 동작은 근사로 남기고 목록으로 관리합니다. 진행 체크리스트는 [dotnetpj 인수인계](LEFT_JOBS.dotnetpj.md)에 있습니다.

# 프로젝트 구성과 개발 목표

`cpppj`는 **① 10.78 싱글플레이 복원 → ② Windows 10/11 안정 구동(DirectDraw 오류 수정 포함) → ③ 한국어 지원 → ④ TCP/IP 로컬 네트워크 멀티플레이·아웃포스트 → ⑤ 추가 요구사항·MCP** 순서로 진행합니다. Windows 전용이며, 멀티플레이 전용 아웃포스트는 4차 목표에 포함합니다. 공식 서버 접속은 복원하지 않습니다. CD판 10.72의 비교·분석 기록도 보존합니다.

`dotnetpj`는 C# + MonoGame 기반입니다. 이미 구현된 클론에 `cpppj`가 원본 기계어로 검증한 부분부터 차례로 옮겨 원본과 완전히 같은 동작을 먼저 구현합니다(2026-10-16 이식 계획). `cpppj`가 완성되면 전체를 다시 대조하고, 그 뒤 추가 요구사항을 반영합니다. GUI Linux 지원과 멀티플레이는 후순위입니다. 최종 요구사항에는 전체화면·16:9/16:10/4:3 화면비·전체화면 가장자리 스크롤·영어/한국어 지원이 포함됩니다. 애니메이션은 원본 수준을 먼저 복원한 뒤 60/120프레임 지원을 진행합니다.

| 경로 | 용도 |
| --- | --- |
| `cpppj/` | C++20·Win32 기반 복원판과 검사 |
| `dotnetpj/` | C#·MonoGame DesktopGL 클론과 검사 |
| `assets/game-data/` | 클론용 그래픽·지도·미션·음악·효과음 데이터 |
| `fonts/` | 한국어 표시용 D2Coding 글꼴 |
| `docs/` | 분석·복원 근거, 빌드·플레이·검증 문서 |
| `tools/`, `analyzeManager/` | 정적 분석·원본 관찰·검증 도구 |
| `originals/`, `originalCD/` | 분석용 원본 10.78 및 CD판 10.72 자료 |
| `screenShots/`, `playingVideos/`, `originalFanGame/` | 스크린샷·녹화·팬게임 참고 자료. 대용량 영상과 팬게임 코드는 Git에 포함하지 않아 PC마다 없을 수 있습니다. |

한국어 건물·유닛 이름은 [공통 명칭 규칙](docs/korean-unit-names.md)을 따릅니다. 사제·신전·다리·발사기·간헐천은 번역할 수 있으며, 나머지는 워크샾·제네레이터처럼 영어 발음을 한글로 표기합니다. dotnetpj의 메뉴·유닛 제목에는 공유 이름표 55개를 연결했습니다. 도움말·미션 본문의 전체 번역은 아직 완료하지 않았습니다.

작업 전 [AGENTS.md](AGENTS.md)와 양쪽 인수인계를 확인합니다. 원본·별도 창 실행 조건은 [실행 확인 규칙](docs/execution-confirmation.md)을 따릅니다. 분석용 원본 파일은 보호하며, 클론 데이터의 변경은 `assets/game-data/`에서 관리합니다.

# 개발 환경 준비

루트의 준비 스크립트에서 필요한 도구를 선택해 설치하거나 현재 환경을 점검할 수 있습니다. 점검만 하려면 다음 명령을 사용합니다.

```powershell
powershell -ExecutionPolicy Bypass -File .\PREPARE.ps1 -CheckOnly
```

```sh
bash ./PREPARE.sh --check-only
```

설치·선택 화면은 각각 `-CheckOnly` 또는 `--check-only`를 생략해 엽니다. REA는 기본 선택되지 않는 선택 항목이며 기존 Node.js·npm을 사용합니다. Ghidra와 REA를 함께 준비하면 `GHIDRA_INSTALL_DIR`를 연결합니다. 두 게임의 빌드에 원본 게임 실행이나 REA 설치는 필요하지 않습니다.

# 빌드 및 실행 파일 만들기 (C# + MonoGame 빌드)

아래 명령은 프로젝트 루트(`NetstormReborn`)에서 실행합니다. 게임 본체는 C# / MonoGame DesktopGL이며 대상 프레임워크는 `net10.0`입니다.

## 준비

- **.NET 10 SDK**를 설치하고 `dotnet --version`으로 확인합니다. `dotnetpj/global.json`은 10.0.100 이상인 .NET 10 SDK의 최신 기능 버전을 허용합니다.
- 첫 빌드·배포 시 NuGet 패키지와 대상 OS의 런타임을 내려받을 인터넷 연결이 필요합니다.
- 저장소의 `assets/game-data/`와 한국어 글꼴 `fonts/D2Coding-Ver1.3.2-20180524-all.ttc`를 함께 체크아웃합니다. 데이터·글꼴은 빌드·배포 출력에 자동으로 복사됩니다.
- 실행 대상은 **Windows 10/11 x64** 또는 **GUI 환경의 x64 Linux(glibc 기반)**입니다. OpenGL을 지원하는 그래픽 드라이버가 필요합니다.

dotnetpj의 빌드·테스트·실행은 독립된 `assets/game-data/`를 사용하며 원본 실행 파일이 필요하지 않습니다. `originals/`는 분석용 보호 자료로 별도 보존합니다. 자산의 구성과 이관 기록은 [클론 게임 데이터](assets/README.md)를 참고합니다.

## 개발용 빌드와 실행

```sh
dotnet restore dotnetpj/Netstorm.sln
dotnet build dotnetpj/Netstorm.sln -c Release --no-restore
dotnet test dotnetpj/Netstorm.sln -c Release --no-build
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

Windows 10/11, CMake 3.21 이상, C++20 컴파일러(Visual Studio 2022 또는 Build Tools의 C++ 도구)가 필요합니다. Win32 API를 직접 사용하는 Windows 전용 빌드입니다. 프로젝트 루트에서 실행합니다.

```sh
cmake -S cpppj -B cpppj/build -DCMAKE_BUILD_TYPE=Release
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build --build-config Release --output-on-failure
```

Visual Studio 생성기 기준 실행 파일은 `cpppj/build/bin/Release/NetstormCpp.exe`입니다. 다음 명령은 창 모드로 C++ 복원판의 메인 메뉴를 엽니다.

```powershell
.\cpppj\build\bin\Release\NetstormCpp.exe --run originals --window
```

`--run <게임 데이터 폴더>`는 C++ 복원판을 실행합니다. `--view fonts`, `--view types`, `--view TEST01`은 글꼴·타입·미션 검사 장면입니다. 현재 메뉴·이동·일부 교전은 동작하지만 Construct와 미션 완주 연결은 아직 미완료입니다. 빌드 절차는 [C++ 빌드](docs/cpp-build.md), 실제 최신 연결 상태는 [인수인계](LEFT_JOBS.md)를 참고하세요.

# 주요 문서

- [cpppj·공통 인수인계](LEFT_JOBS.md) / [dotnetpj 인수인계](LEFT_JOBS.dotnetpj.md): 최신 완료·미완료 범위와 재개 순서
- [C++ 복원 로드맵](docs/cpp-roadmap.md) / [실제 플레이 복원 계획](docs/cpp-playable-plan.md): 개발 단계와 복원 계획
- [cpppj → dotnetpj 이식 계획](docs/dotnet-cpp-port-plan-20261016.md): dotnetpj를 원본과 같은 동작으로 만드는 이식 단위와 순서
- [클론 게임 데이터](assets/README.md): 자산 구성·출처·빌드 포함 방식
- [원본 실행·별도 창 확인 규칙](docs/execution-confirmation.md): AI 작업의 실행 조건

# License

MIT License
Copyright (c) 2026 HJOW
