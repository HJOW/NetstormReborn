Netstorm - Reborn
-------------------------------------------------------------------------

**한국어** | [English](README.en.md)

# 개요 
Activision 사에서 1997년도에 개발 후 한참 전에 손 놓은
Netstorm - Islands at war 게임을
AI 를 이용해 되살리는 프로젝트입니다.

# 현재 개발 진행상황

**2026-10-29 기준:** cpppj는 제품 단위563·문서 단위564, dotnetpj는 `20261029-02` / R2d-2, webpj는 2026-10-28 WB03-D 인수인계 기준입니다. 실행·플레이 가능 범위와 원본 게임과의 동일성을 구분해 기록합니다.

| 프로젝트 | 실행·플레이 가능 범위 | 원본과의 동일성 및 남은 범위 |
| --- | --- | --- |
| cpppj — C++ 복원판 | 원본 메뉴·브리핑, 건설·생산·전투, 좌측 덱·미니맵·QWASZX·ESC와 공통 AI가 연결돼 있습니다. 560제품/561단위에서 일반31·데모3의 진입·짧은 진행·종료를 확인했습니다. Edit의 기존/새 맵·저장·섬 추가·Test Battle 왕복 일부도 확인했습니다. 563제품은 새 배포 첫 실행·저장 전체화면 재실행·창 런처에서 실제 메인 메뉴 진입을 통과했습니다. | 10.78 디컴파일·기계어 대조 기반입니다. 563의 집중15개/실패0·등록3,427, Release 게임/검사 경고·오류0입니다. 현재563제품의 전체34경로·전체 C++ 검사는 미실행입니다. 자연 완주·진행 저장/승패/데모 반복, 남은 Edit 도구·시나리오/수명·외관/음향의 전체 동일성과 Windows11 실기는 남았습니다. Windows10 DIB 호환 전체화면·3개 화면비·가장자리 스크롤은 구현/일부 확인했으며 전체 호환 완료와 구별합니다. |
| dotnetpj — C# + MonoGame 클론 | 기존 경로에서 캠페인 **1-1 전쟁의 시작!(The War Begins!)**, **1-2 훨리기그의 지배자(Master of Whirligigs)**를 플레이할 수 있습니다. 영어·한국어 UI, 도움말·우클릭 메뉴·옵션을 제공합니다. | 원본 구조에 맞춘 새 `Netstorm.Client` 경로를 구축 중이며 8비트 화면·MonoGame 출력, 그리기 원시·팔레트·판본/표시자료 처리로 R2를 진행하고 있습니다. 2026-10-29 최신 Release 빌드·전체 검사는 경고·오류0, 1,500개 통과입니다. 이 새 화면 경로와 월드 코어는 아직 기본 게임 세션에 연결되지 않았고, 플레이 가능한 범위와 원본 동일성은 기존 1-1·1-2 근사 구현 수준입니다. |
| webpj — HTML + JavaScript 웹 클론 | 원본 자산을 적재하고 "클릭하여 시작"을 거쳐 메인 메뉴를 표시합니다. Campaign·Demo·Credits 대화상자와 목록을 구현했으며 메인 메뉴 Edit 버튼은 제거했습니다. | 2026-10-28 WB03-D 기준 자동 검사 34개가 통과했습니다. 원본 메인 메뉴와 대화상자의 배치·입력·효과음 등을 복원 중이며, Help·Options 등 나머지 화면과 실제 게임 플레이는 아직 없습니다. 웹에서는 커스텀 맵 Edit와 멀티플레이를 구현하지 않습니다. |

## C++ 복원판의 최신 상태

현재 목표는 Early Missions 6개·Priest Training 9개·캠페인 16개·데모 3개를 10.78과 같게 플레이할 수 있도록 하고, Multiplayer를 제외한 메뉴·게임 UI와 커스텀 맵/시나리오 편집을 복원하는 것입니다. 건설·생산·다리·채취/운반·교전과 공통 AI는 세션에 연결돼 있습니다. 560제품/561단위에서는 일반31개를 CLI 미션 지정과 실제 안내 입력으로, 데모3개를 실제 메뉴와 안내 타이머로 진입해 짧은 진행·정상 종료를 확인했습니다. Early1·2의 과거 자연 진행과 캠페인1-1의 과거 자동검사/사용자 완주 확인은 별도 근거이며, 현재 제품의 전체 자연 완주나 원본 동일성 완료를 뜻하지 않습니다. UI/Edit 잔여와 우선순위는 [최신 인수인계](LEFT_JOBS.md)를 따릅니다.

2026-10-29 단위563은 새 설치에서 인트로 상태39가 미연결로 종료하던 문제를 원문39→40→41→3 순서로 복원했습니다. 원본 CPU17입력의 상태 블록·호출 순서가 일치했고 집중 검사15개/실패0·등록3,427, 최종 Release 게임/검사 경고·오류0입니다. 같은 배포본의 새 설치·저장된 전체화면 재실행·기본 창 런처가 모두 실제 메인 메뉴/정상 종료를 통과했습니다. 기본 증분 빌드→ZIP·게임 자료1,356개·VC DLL3개도 확인했습니다. 인트로 영상은 CD 삽입 때만 재생됐다는 사용자 확인에 따라 10.78 영상 부재를 기준으로 검증했으며, CD 탐색/존재하는 SMK 재생은 미복원입니다. 560에서 실제 Ctrl Shift F7 진단은 연결됐지만 비영 traceToChat·메타맵 특수 경계와 UI/Edit·수명·자연 완주·Windows11 실기는 남았습니다. [563 기록](docs/exe/cpp-native-intro-state-20261029.md)·[561의34경로 기록](docs/exe/cpp-solo-entry-current-20261029.md)을 구별해 참고하세요.

## C# + MonoGame 클론의 최신 상태

기존 실행 경로에서는 캠페인 **1-1 The War Begins!**와 **1-2 Master of Whirligigs**를 플레이할 수 있습니다. 별도로 cpppj 구조에 맞춘 새 C# 클라이언트(`Netstorm.Client`)를 구축하고 있습니다. 화면은 8비트 논리 버퍼·팔레트 기반으로 재구성 중이며 MonoGame은 호스트와 최종 출력 역할을 맡습니다. [캠페인 1-1](docs/gameplay/campaign-one.md)·[1-2](docs/gameplay/campaign-two.md)의 기존 구현 기록과 [최신 인수인계](LEFT_JOBS.dotnetpj.md)를 참고하세요.

2026-10-29의 R2d-2에서는 색상·표시 판본 자료와 `RawShapeToBuffer`를 추가하고, 그 전 단계의 그리기 원시·화면 캡처 기반도 확장했습니다. Release 전체 검사는 경고·오류0, **1,500개 통과**(Assets 363·Core 1,030·Client 107)입니다. 이는 기존 코드 및 새 모듈 단위 검증을 포함한 결과이며, 새 화면/월드 경로가 실제 게임 세션에 연결됐다는 뜻은 아닙니다. 다음은 `SpriteDraw` 이식 후 Gump UI·메뉴이며, 기존 플레이 범위와 원본 동일성은 아직 1-1·1-2의 근사 경로에 한정됩니다. 순서와 남은 범위는 [cpppj → dotnetpj 이식 계획](docs/dotnet-cpp-port-plan-20261016.md)과 [인수인계](LEFT_JOBS.dotnetpj.md)를 참고하세요.

## HTML + JavaScript 웹 클론의 최신 상태

브라우저의 오디오 재생 제한에 따라 원본 자산을 적재한 뒤 "클릭하여 시작" 화면을 거쳐 메인 메뉴를 표시합니다. 메인 메뉴의 Edit 버튼은 제거했고, Campaign·Demo·Credits 대화상자와 목록 화면을 연결했습니다. 2026-10-28 WB03-D 기준 자동 검사 34개가 통과했습니다. Help·Options 화면 등 나머지 버튼이 여는 화면과 실제 게임 플레이는 아직 구현되지 않았습니다. 웹에서는 커스텀 맵 Edit와 멀티플레이를 제외합니다. 자세한 상태와 다음 작업은 [최신 인수인계](LEFT_JOBS.webpj.md)를 참고하세요.

# 프로젝트 구성과 개발 목표

`cpppj`는 **① 10.78 싱글플레이 복원 → ② Windows 10/11 안정 구동(DirectDraw 오류 수정 포함) → ③ TCP/IP 로컬 네트워크 멀티플레이·아웃포스트 → ④ 한국어 지원·MCP → ⑤ 추가 요구사항** 순서로 진행합니다. Windows 전용이며, 멀티플레이 전용 아웃포스트는 3차 목표에 포함합니다. 공식 서버 접속은 복원하지 않습니다. CD판 10.72의 비교·분석 기록도 보존합니다.

`dotnetpj`는 C# + MonoGame 기반입니다. 이미 구현된 클론에 `cpppj`가 원본 기계어로 검증한 부분부터 차례로 옮겨 원본과 완전히 같은 동작을 먼저 구현합니다(2026-10-16 이식 계획). `cpppj`가 완성되면 전체를 다시 대조하고, 그 뒤 추가 요구사항을 반영합니다. GUI Linux 지원과 멀티플레이는 후순위입니다. 최종 요구사항에는 전체화면·16:9/16:10/4:3 화면비·전체화면 가장자리 스크롤·영어/한국어 지원이 포함됩니다. 애니메이션은 원본 수준을 먼저 복원한 뒤 60/120프레임 지원을 진행합니다.

`webpj`는 HTML + JavaScript 기반입니다. `cpppj`가 완성되면 이를 분석해 원본과 같은 동작을 목표로 개발합니다. 커스텀 맵(Edit)과 멀티플레이, 웹 환경에 맞지 않는 기능(게임 종료, 해상도·전체화면 설정 등)은 제외하며, 필요한 에셋은 `webpj/www/` 안으로 복사해 사용합니다.

| 경로 | 용도 |
| --- | --- |
| `cpppj/` | C++20·Win32 기반 복원판과 검사 |
| `dotnetpj/` | C#·MonoGame DesktopGL 클론과 검사 |
| `webpj/` | HTML + JavaScript 웹 클론(`www/`), 정적 웹 서버, 검사·자산 준비 도구 |
| `assets/game-data/` | 클론용 그래픽·지도·미션·음악·효과음 데이터 |
| `fonts/` | 한국어 표시용 D2Coding 글꼴 |
| `docs/` | 분석·복원 근거, 빌드·플레이·검증 문서 |
| `tools/`, `analyzeManager/` | 정적 분석·원본 관찰·검증 도구 |
| `originals/`, `originalCD/` | 분석용 원본 10.78 및 CD판 10.72 자료 |
| `original1037/`, `originalPatches/` | 비교용 원본 10.37 자료와 10.37 → 10.62 패치(NSP1062) |
| `extracted/` | 정밀 디컴파일 결과와 빌드·검사 원시 로그. Git에 포함하지 않아 작업한 PC에만 있습니다. |
| `screenShots/`, `playingVideos/`, `originalFanGame/` | 스크린샷·녹화·팬게임 참고 자료. 대용량 영상과 팬게임 코드는 Git에 포함하지 않아 PC마다 없을 수 있습니다. |

한국어 건물·유닛 이름은 [공통 명칭 규칙](docs/korean-unit-names.md)을 따릅니다. 사제·신전·다리·발사기·간헐천·제단·거주 구역(Residence)·고갈된(Empty)은 번역할 수 있으며, 나머지는 워크샾·제네레이터처럼 영어 발음을 한글로 표기합니다. dotnetpj의 메뉴·유닛 제목에는 공유 이름표 55개를 연결했습니다. 도움말·미션 본문의 전체 번역은 아직 완료하지 않았습니다.

작업 전 [AGENTS.md](AGENTS.md)와 각 프로젝트의 인수인계([cpppj·공통](LEFT_JOBS.md), [dotnetpj](LEFT_JOBS.dotnetpj.md), [webpj](LEFT_JOBS.webpj.md))를 확인합니다. 원본·별도 창 실행 조건은 [실행 확인 규칙](docs/execution-confirmation.md)을 따릅니다. 분석용 원본 파일은 보호하며, 클론 데이터의 변경은 `assets/game-data/`에서 관리합니다.

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

배포용 압축파일(zip)은 다음 스크립트로 만듭니다. 저장 위치를 생략하면 실행 중에 입력받고, 빈 값을 입력하면 취소합니다. 기본은 받는 PC에 .NET 10 런타임을 미리 설치하는 방식이며, `-SelfContained`를 붙이면 런타임까지 포함합니다. 임시 폴더에 게시하므로 저장소에 `dist/`를 남기지 않습니다.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File dotnetpj\package-dotnetpj.ps1 D:\dist\NetstormReborn-dotnetpj.zip
```

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

Windows 10/11, CMake 3.21 이상, C++20 컴파일러(Visual Studio 2022 이상 또는 Build Tools의 C++ 도구)가 필요합니다. Win32 API를 직접 사용하는 Windows 전용 빌드입니다. 프로젝트 루트에서 실행합니다.

```sh
cmake -S cpppj -B cpppj/build -DCMAKE_BUILD_TYPE=Release
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build --build-config Release --output-on-failure
```

CTest는 래퍼 검사 하나로 등록되어 있으며, 현재 등록된 C++ 검사는 3,427개입니다. 전체 실행은 수 분이 걸리며, 각 단위의 집중 검사와 전체 검사 결과는 [인수인계](LEFT_JOBS.md)에서 구분해 확인할 수 있습니다. Visual Studio 생성기 기준 실행 파일은 `cpppj/build/bin/Release/NetstormCpp.exe`입니다. 다음 명령은 창 모드로 C++ 복원판의 메인 메뉴를 엽니다.

```powershell
.\cpppj\build\bin\Release\NetstormCpp.exe --run originals --window
```

`--run <게임 데이터 폴더>`는 C++ 복원판을 실행합니다. `--mission <미션 키>`를 붙이면 메뉴를 거치지 않고 해당 미션의 브리핑으로 들어갑니다(예: `--mission Tutorial1`). `--view fonts`, `--view types`, `--view TEST01`은 글꼴·타입·미션 검사 장면입니다. 인자 없이(또는 `--help`로) 실행하면 빌드 정보와 사용할 수 있는 옵션을 표시합니다. 실제 건설·생산·다리·채취/운반·전투와 공통 전략 AI의 자동 실행이 연결돼 있으나, 목표 미션 전체의 자연 완주, Edit의 나머지 편집 도구·시나리오/수명과 원본 동일성 검증은 미완료입니다. 빌드 절차는 [C++ 빌드](docs/cpp-build.md), 실제 최신 연결 상태는 [인수인계](LEFT_JOBS.md)를 참고하세요.

배포용 압축파일(실행 파일·Visual C++ 런타임·원본 10.78 게임 데이터·실행 배치 파일)은 다음 스크립트로 만듭니다. 저장 위치를 생략하면 실행 중에 입력받고, 빈 값을 입력하면 취소합니다. 구성과 옵션은 [C++ 빌드 7절](docs/cpp-build.md)을 봅니다. 562~563에서 Windows PowerShell5.1의 기본 증분 빌드→압축을 실제 확인했습니다. 563은 새 압축 해제본 첫 실행·저장 전체화면 재실행·창 런처도 확인했으며, 전체 미션/동일성·Windows11 배포 검증은 별도입니다.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File cpppj\package-cpppj.ps1 D:\dist\NetstormReborn-cpppj.zip
```

# 웹 클론 실행 (HTML + JavaScript)

Node.js가 필요하며(v24에서 검증) 외부 npm 패키지는 쓰지 않습니다. 필요한 에셋은 `webpj/www/data/`에 커밋돼 있습니다. 프로젝트 루트에서 다음 명령으로 정적 웹 서버를 시작한 뒤 브라우저에서 `http://localhost:9803/`을 엽니다.

```sh
npm --prefix webpj start
```

자동 검사는 `npm --prefix webpj test`로 실행합니다. `assets/game-data/`의 원본 자료가 바뀌면 `npm --prefix webpj run prepare-assets`로 `webpj/www/data/`를 다시 만들고, `npm --prefix webpj run check-assets`로 복사본을 점검합니다.

# 주요 문서

- [cpppj·공통 인수인계](LEFT_JOBS.md) / [dotnetpj 인수인계](LEFT_JOBS.dotnetpj.md) / [webpj 인수인계](LEFT_JOBS.webpj.md): 최신 상태와 앞으로 할 작업. cpppj의 2026-10-25까지 단위별 이력은 [LEFT_JOBS.old.md](LEFT_JOBS.old.md)에 있습니다.
- [TODO.md](TODO.md): cpppj의 현행 목표 프롬프트(Early Missions·Priest Training·캠페인·데모·Edit·메뉴·게임 중 UI)
- [C++ 복원 로드맵](docs/cpp-roadmap.md) / [실제 플레이 복원 계획](docs/cpp-playable-plan.md): 개발 단계와 복원 계획
- [cpppj → dotnetpj 이식 계획](docs/dotnet-cpp-port-plan-20261016.md): dotnetpj를 원본과 같은 동작으로 만드는 이식 단위와 순서
- [클론 게임 데이터](assets/README.md): 자산 구성·출처·빌드 포함 방식
- [원본 실행·별도 창 확인 규칙](docs/execution-confirmation.md): AI 작업의 실행 조건

# License

MIT License
Copyright (c) 2026 HJOW
