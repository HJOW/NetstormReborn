Netstorm - Reborn
-------------------------------------------------------------------------

**한국어** | [English](README.en.md)

# 개요 
Activision 사에서 1997년도에 개발 후 한참 전에 손 놓은
Netstorm - Islands at war 게임을
AI 를 이용해 되살리는 프로젝트입니다.

# 현재 개발 진행상황

**cpppj는 2026-10-22 단위300, dotnetpj는 2026-10-21 W03 열째 조각의 인수인계 기준**입니다. 실행·플레이 가능 범위와 원본 게임과의 동일성을 구분해 기록합니다.

| 프로젝트 | 실행·플레이 가능 범위 | 원본과의 동일성 및 남은 범위 |
| --- | --- | --- |
| cpppj — C++ 복원판 | Windows 창 모드에서 메뉴·브리핑·안내, 건설·생산·다리 배치/회전·채취/운반·실제 교전을 연결했습니다. Early1·2의 자연 완주와 다음 튜토리얼 진입, 캠페인1-1의 적 신전 파괴를 실제로 관측했습니다. | 10.78 디컴파일·기계어 대조 기반입니다. **적·아군·데모 공통 AI의 다리 전략·전체 행동 순서/자동 프레임**, 미션 전환·정리, 자연 포획/희생·승패 및 목표15개 전체 플레이 검증이 남았습니다. Edit·커스텀 맵/시나리오와 전체 그래픽·음향·입력 동일성은 미완료입니다. |
| dotnetpj — C# + MonoGame 클론 | 캠페인 **1-1 전쟁의 시작!(The War Begins!)**, **1-2 훨리기그의 지배자(Master of Whirligigs)**의 플레이와 건설·생산·이동·전투 조작을 구현했습니다. 영어·한국어 UI, 도움말·우클릭 메뉴·옵션을 제공합니다. Early Missions와 데모는 아직 메뉴에서 열리지 않습니다. | 원본 검증 규칙을 차례로 이식 중입니다. 공통 미션 판정 등의 세션 연결과 별도로, 원본 월드 코어의 생성·그래프·삭제 수명·일반/사제 피격 모듈을 검증했습니다. **새 월드 코어는 아직 실제 세션에 연결되지 않았으며**, 실제 플레이의 건설·전투·경로·SP·AI·시간 규칙은 관찰 기반 근사입니다. 자연 완주→진행 저장→재실행→다음 미션 해제와 원본 동일성은 추가 검증이 필요합니다. |

## C++ 복원판의 최신 상태

현재 구현 목표는 **Early Missions1~6·캠페인1-1~1-6·데모3개, 총15개 미션의 원본10.78과 동일한 플레이와 Edit 메뉴의 커스텀 맵/시나리오 기능**입니다. 기존 플레이 기반에는 SP·기술·사제 참여·Start/StartDemo, 건설·생산·다리·경제·전투, Early 전용 판정과 공통 미션 프레임을 연결했습니다. F8 안내 재표시, Guide 전환·실패/중단/다음 미션 명령 큐, 지식 카드와 Help도 연결했습니다. 확대된 목표와 최신 완료/미완료 범위는 [인수인계](LEFT_JOBS.md)를 참고하세요.

[Early1](docs/exe/cpp-tutorial-harvest-20261019.md)과 [Early2](docs/exe/cpp-tutorial-two-runtime-20261019.md)는 정상 입력으로 건설·채취/생산·완료 기록과 다음 튜토리얼 진입까지 관측했습니다. 캠페인1-1은 [실제 전진 캐논으로 적 시설/신전 파괴](docs/exe/cpp-forward-window-20261019.md)까지 확인했으나 적 사제 포획·희생·승리는 확인하지 못했습니다. Early5의 부분 진행과 이전 범위11개 미션의 로드/Start·짧은 프레임 검사 이력은 현재15개 전체 완주 증거와 구분합니다.

[Devastation·포획/희생의 집중13검사](docs/exe/cpp-ritual-runtime-focused-20261021.md)는 통과했지만 일부 입력은 명시 시전원·포로·시계 상태를 사용했습니다. 실제 UI에서 주문 취득→시전→포획→운반→희생까지 자연스럽게 이어지는 검증은 남아 있습니다. [썬 디스크 발사기/훨리베이스 본체와 비행체 그림자 수정](docs/exe/cpp-combat-draw-and-defense-20261021.md) 이후 사용자가 실제 게임 테스트를 중단했으며, 해당 테스트는 보류 중입니다.

[공통 AI](docs/exe/cpp-ai-world-preparation-20261021.md)의 생성·가계부·통지·월드 인식·초기 전략에 이어 경제/다리 재고 보충·실제 생산·수집자·기술 생산/업그레이드·이동·사제 행동 하위를 복원했습니다. 최근에는 [후보 스캔](docs/exe/cpp-ai-drop-scan-20261022.md), [기술/회전별 TYPE 선택](docs/exe/cpp-ai-drop-selection-20261022.md), [가짜 에너지 부족좌표 갱신](docs/exe/cpp-ai-fake-dry-spots-20261022.md), [전체 건물 배치 행동](docs/exe/cpp-ai-drop-actions-20261022.md)을 같은 실제 Session의 기술·생산·비용·난수·지도·배치 경로에 연결했습니다.

**다리 전략과 전체 행동 순서·자동 AI 프레임/종료는 아직 미완료**입니다. 현재의 명시 호출 연결은 자동 전략 루프의 실행 증거가 아닙니다. 일반 미션은 사람이 로컬 소유자를 조작하고, 데모는 로컬 소유자도 같은 AI로 움직여야 합니다. Template/State의 실제 전환·정리와 승패/재시작 수명 연결도 남아 있습니다.

**Edit 메뉴의 전체 사용 흐름도 미완료**입니다. 신규/불러오기, 지형·섬·오브젝트·소유자 편집, Fort 저장/재불러오기, 언어별 시나리오 파일의 브리핑·플레이어·AI·시작 SP 적용, 테스트 전투와 편집 복귀를 원본과 같게 연결·검증해야 합니다.

최신 AI 구현 단위300의 Release 빌드는 **종료0·경고0·오류0**입니다. 이번에 추가한19개 C++검사는 **작성·컴파일만 했고 실행하지 않았습니다**. 최근 AI 복원은 빌드·정적 확인까지이며, 과거 전체 회귀/플레이 성공을 최신 코드 전체의 통과로 표시하지 않습니다. 15개 미션과 Edit 목표, 전체 그래픽·음향·입력/시간 동일성은 미완료입니다. DirectDraw 오류 수정과 Windows10/11 전체화면 안정화는 cpppj2차 목표의 별도 필수 작업입니다.

## C# + MonoGame 클론의 최신 상태

메인 메뉴의 캠페인 목록은 원본 스크립트의 장·선행 완료 조건과 저장된 진행값을 읽고, 실제 지원 미션을 제한합니다. 옵션에서는 해상도·창/전체화면·소리·음량·바람 소리·스피커 좌우 교환을 바꿀 수 있습니다. 도움말과 F1에서 원본 도움말을 탐색하고, 사제·워크샾·생산 항목의 우클릭 메뉴를 사용할 수 있습니다. [기능·진행 저장 검토](docs/dotnet-review-20261011.md), [녹화 분석 적용 범위](docs/screens/recorded-ui-clone-20261002.md), [캠페인1-1](docs/gameplay/campaign-one.md)·[1-2 구현](docs/gameplay/campaign-two.md)을 참고하세요. 초기 구현 문서보다 [최신 인수인계](LEFT_JOBS.dotnetpj.md)가 우선합니다.

공통 미션 프레임 게이트·플레이어 참여/방향별 동맹·누적 제작 장부·튜토리얼1 단계 판정기는 원본 검증 결과를 이식해 기존 세션에 연결했습니다. 별도로 원본 월드 코어를 구축하고 있으며, W02의 슬롯/공간 해시·Pop/Unpop·소유자/프레임·다리/그래프에 이어 W03의 삭제 수명·해방·일반 피해·[사제 피격/HP 변경](docs/dotnet-original-core-priestdamage-20261021.md)과 [종속 조회·삭제](docs/dotnet-original-core-containedlookup-20261021.md)까지 모듈 단위로 검증했습니다. **연결 fixture는51개**입니다. 최신 기록인 2026-10-21 W03 열째 조각의 전체 Release는 **경고0·오류0**, 단위 **1,277개 통과**(Assets363·Core914)입니다. 이 수치는 모듈 검증 기록이며 실제 플레이 범위가 늘었다는 뜻은 아닙니다.

다음은 W03의 내용물 수명(`ContentFormLifetime`)·Form 수명·신전 삭제·삭제 보상 계산, 파생 분배 허브와 실제 세션 연결입니다. 이후 Kernel/Regular 시간·SP/난수 등 원본 코어와 미션 시작 상태를 맞추고, 경로·건설·전투·희생·승패·표시/UI/소리를 차례로 이식합니다. cpppj의 목표가15개 미션과 Edit로 확대됐어도 dotnetpj의 현재 플레이 범위는 위 캠페인2개입니다. cpppj에서도 아직 미완료인 전략 AI 등의 동작은 근사 범위를 명시하며, [cpppj → dotnetpj 이식 계획](docs/dotnet-cpp-port-plan-20261016.md)에 따라 자연 플레이와 원본 동일성을 확인합니다.

# 프로젝트 구성과 개발 목표

`cpppj`는 **① 10.78 싱글플레이 복원 → ② Windows 10/11 안정 구동(DirectDraw 오류 수정 포함) → ③ TCP/IP 로컬 네트워크 멀티플레이·아웃포스트 → ④ 한국어 지원·MCP → ⑤ 추가 요구사항** 순서로 진행합니다. Windows 전용이며, 멀티플레이 전용 아웃포스트는 3차 목표에 포함합니다. 공식 서버 접속은 복원하지 않습니다. CD판 10.72의 비교·분석 기록도 보존합니다.

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

`--run <게임 데이터 폴더>`는 C++ 복원판을 실행합니다. `--view fonts`, `--view types`, `--view TEST01`은 글꼴·타입·미션 검사 장면입니다. 실제 건설·생산·다리·채취/운반·전투가 연결돼 있으나 공통 전략 AI의 전체 자동 실행, 목표15개 전체 완주, Edit·커스텀 맵/시나리오와 원본 동일성 검증은 미완료입니다. 빌드 절차는 [C++ 빌드](docs/cpp-build.md), 실제 최신 연결 상태는 [인수인계](LEFT_JOBS.md)를 참고하세요.

배포용 압축파일(실행 파일·Visual C++ 런타임·원본 10.78 게임 데이터·실행 배치 파일)은 다음 스크립트로 만듭니다. 저장 위치를 생략하면 실행 중에 입력받고, 빈 값을 입력하면 취소합니다. 구성과 옵션은 [C++ 빌드 7절](docs/cpp-build.md)을 봅니다.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File cpppj\package-cpppj.ps1 D:\dist\NetstormReborn-cpppj.zip
```

# 주요 문서

- [cpppj·공통 인수인계](LEFT_JOBS.md) / [dotnetpj 인수인계](LEFT_JOBS.dotnetpj.md): 최신 완료·미완료 범위와 재개 순서
- [C++ 복원 로드맵](docs/cpp-roadmap.md) / [실제 플레이 복원 계획](docs/cpp-playable-plan.md): 개발 단계와 복원 계획
- [cpppj → dotnetpj 이식 계획](docs/dotnet-cpp-port-plan-20261016.md): dotnetpj를 원본과 같은 동작으로 만드는 이식 단위와 순서
- [클론 게임 데이터](assets/README.md): 자산 구성·출처·빌드 포함 방식
- [원본 실행·별도 창 확인 규칙](docs/execution-confirmation.md): AI 작업의 실행 조건

# License

MIT License
Copyright (c) 2026 HJOW
