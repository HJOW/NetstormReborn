# C++ 빌드 (`cpppj/`)

> 2026-10-05 추가·갱신. **메뉴→캠페인→브리핑→지형/객체·선택/이동→복귀를 연결했다.** 건설·경제·전투·AI·승패·소리·MCP는 후속이다. **cpppj는 Windows 전용이다.** [공용 계층](exe/cpp-reconstruction.md), [자산](exe/cpp-assets-reconstruction.md), [설정](exe/cpp-config-reconstruction.md), [요새](exe/cpp-fort-reconstruction.md), [창](exe/cpp-screen-reconstruction.md), [표시 기반](exe/cpp-renderer-reconstruction.md), [메뉴](exe/cpp-menu-reconstruction.md), [월드/조작의 범위와 제한](exe/cpp-world-reconstruction.md).

**기존 게임 전체가 실제 플레이 가능한 수준으로 복원되는 것이 우선이다.** 화면비 확장·한국어·Linux·추가 기능은 이후다. 현재 작업 순서·완료 기준은 [실제 플레이 복원 계획](cpp-playable-plan.md), 기반 복원 세부 항목은 [기존 로드맵](cpp-roadmap.md)에 있다. 이 문서는 빌드와 소스 복원 규칙을 다룬다.

## 1. 두 빌드의 관계

저장소에는 게임 빌드가 둘 있다. 서로의 코드를 참조하지 않는다.

| | C# + MonoGame 빌드 | C++ 빌드 |
|---|---|---|
| 폴더 | [`dotnetpj/`](../dotnetpj/) | [`cpppj/`](../cpppj/) |
| 만드는 방법 | 원본을 분석해 규칙·화면을 새로 구현한다 (클론 코딩) | **기존 게임을 디컴파일한 소스를 토대로 C++ 소스를 다시 만든다** |
| 빌드 도구 | .NET 10 SDK (`dotnetpj/Netstorm.sln`) | CMake 3.21 이상 + C++20 컴파일러(MSVC), **Windows 전용** |
| 플랫폼 | MonoGame (Windows·Linux) | **Win32 API 를 원본처럼 직접 호출** (외부 라이브러리 없음) |
| 실행 파일 | `NetstormClone` | `NetstormCpp` |
| 상태 | 캠페인 1-1·1-2 플레이 가능 | 메뉴→브리핑→실제 지형/객체·선택/이동·정지/복귀. 건설·전투·미션 완주는 후속 |

두 빌드가 함께 쓰는 것은 저장소 루트에 그대로 있다.

* `assets/game-data/` — 클론 게임 데이터 ([클론 게임 데이터](../assets/README.md))
* `fonts/` — 한국어 글꼴 D2Coding
* `docs/` — 분석 문서. 원본 동작에 대한 사실은 두 빌드가 같은 문서를 근거로 삼는다.
* `tools/` — Python 추출·검증 도구, Ghidra 스크립트
* `analyzeManager/` — 원본 게임 자동 탐험 프로그램. C# 으로 쓴 **분석 도구**라서 `dotnetpj/` 로 옮기지 않았다. `dotnetpj/src/Netstorm.Assets` 를 참조하고, 공통 빌드 설정은 `analyzeManager/Directory.Build.props` 가 `dotnetpj/` 의 것을 가져온다.

공통 목표(AGENTS.md)는 영어·한국어, 풀스크린과 16:9·16:10·4:3, 원본 수준의 프레임을 먼저 맞추고 이후 60·120프레임이다.

**cpppj의 방향(2026-10-05 사용자 결정, AGENTS.md에 반영됨).** AGENTS.md: "cpppj - C++ , 기존 게임을 디컴파일하여 C++ 코드로 최대한 복원하는 것이 1차 목표이다. 이후 윈도우10, 11에서 실행 가능한 수준으로 만들고, 요구사항 반영 및 MCP 추가한다. 윈도우용만 개발한다." / "dotnetpj - C# + MonoGame 기반으로 개발. cpppj 완성 후 이를 분석하여 개발." 목표의 순서는 **① 디컴파일한 코드를 C++로 최대한 복원(1차 목표) → ② Windows 10/11에서 실행 가능한 수준 → ③ 요구사항 반영과 MCP**다. cpppj는 기존 게임 원본을 되살리는 것이 목적이다. 기능 변경을 최소로 하고 **원본과 같은 방식**으로 만들며, 이후 MCP를 붙여 원본 게임 분석에 쓴다. 그래서 창·화면·입력은 원본처럼 Win32 API를 직접 부른다. **cpppj는 Windows 전용**이고(다른 OS에서는 CMake 구성이 멈춘다) Linux 지원은 C# 빌드의 후순위 목표다. 한국어 지원은 cpppj에서는 후순위다.

## 2. 폴더 구조

원본 exe 의 assert 문자열에는 원본 소스 경로가 남아 있다. 폴더는 그 원본 소스 트리를 따른다.

```text
cpppj/
  CMakeLists.txt          최상위 빌드 정의 (C++20, 테스트 옵션)
  CMakePresets.json       구성 프리셋: vs2022, ninja(컴파일러가 PATH 에 있는 Windows 환경)
  cmake/
    CompilerOptions.cmake 공통 컴파일 옵션 (UTF-8, 경고 수준)
  SOURCE_MAP.md           원본 소스 파일 → cpppj 경로 표 (자동 생성)
  src/
    o/                    원본 \Ns\O\      공용 모듈 (게임 규칙·데이터·프로세스 커널)
    client/               원본 .\          클라이언트 모듈 (화면·입력·소리·메인 루프)
    zacket/               원본 \Ns\Zacket\ 네트워크 패킷
    platform/             새로 쓰는 코드   원본에 없는 보조 기능 (콘솔 출력·BMP 저장·파일 쓰기)
    app/                  새로 쓰는 코드   실행 파일 진입점 (main.cpp), 임시 검사용 화면 (InspectView)
  tests/                  단위 테스트 (ctest)
```

| 원본 폴더 (assert 문자열) | 파일 수 | cpppj | 내용 |
|---|---|---|---|
| `\Ns\O\`, `c:\ns\o\`, `..\o\` | 75 | `src/o/` | Squid·Template·Player·Kernel·Bridge·RiftType·Config·Xlat·Ai 등. 클라이언트가 `..\o\squid.h` 로 포함하므로 클라이언트 폴더의 형제 폴더다 |
| `.\` (패치판), 폴더 없음 (CD판) | 60 | `src/client/` | ClientMain·UserInput·Gump 계열·Screen·Renderer·Sound·State·Mission 등 |
| `\Ns\Zacket\` | 1 | `src/zacket/` | Zacket.cpp |

파일 수는 두 판본의 assert 문자열에 이름이 남은 것만 센 값이다. assert 가 없는 소스 파일은 알 수 없다. 전체 목록은 [SOURCE_MAP.md](../cpppj/SOURCE_MAP.md)에 있으며 `python tools/cpp_source_map.py` 로 다시 만든다(원본 exe 를 읽기만 한다).

* **파일 이름**: 패치판(10.78)의 문자열은 `Clientmain.cpp` 처럼 첫 글자만 대문자로 바뀌어 있고, CD판(10.72)에는 `ClientMain.cpp`·`RiftType.cpp` 같은 원래 표기가 남아 있다. cpppj 는 **CD판 표기의 첫 글자를 대문자로 한 이름**을 쓴다(CD판에 없으면 패치판 표기). 헤더는 짝이 되는 `.cpp` 의 대소문자를 따른다(`RiftType.h`).
* **한 판본에만 있는 파일**: CD판에만 `Cloud.cpp`·`CloudSystem.cpp`·`Fountain.cpp`·`Look.cpp`·`Markup.cpp`·`Piecegump.cpp`·`Splash.cpp`·`TextGump.cpp`·`Connection.cpp`·`Terr.cpp`, 패치판에만 `Dissolve.cpp`·`Factory.cpp`·`Fierytail.cpp`·`Priest.cpp`·`Fence.cpp`·`Periodicharmer.cpp`·`Totalmade.cpp`·`rifttype.h` 가 나온다. assert 문자열이 있고 없고의 차이일 수 있어서, 그 판본에 그 소스가 없었다는 뜻은 아니다.
* **`_p_*.cpp`**: `_p_gunprocessdata.cpp`·`_p_packets.cpp` 등 10개. 이름으로 보아 자동 생성된 직렬화 코드로 추정한다(확인하지 않았다).
* **포함 경로**의 기준은 `cpppj/src/` 다. 예: `#include "client/ClientMain.h"`, `#include "o/Squid.h"`.
* 지금은 정적 라이브러리 하나(`netstorm`)로 묶는다. `o/` 는 `client/`·`platform/` 을 포함하지 않는 것이 규칙이지만 빌드가 강제하지는 않는다. 라이브러리는 `user32`·`gdi32`·`winmm` 에 연결된다(`client/` 가 원본처럼 Win32 를 직접 부른다).

## 3. 빌드와 테스트

**2026-10-06 raw Graph 후속:** [실제 SID·프레임·해시·spot과 공통 postPop 연결](exe/cpp-rawgraph-reconstruction.md)을 추가했다. Pop의 최종 상태를 쓰기 전에 시험하고 raw graph byte를 갱신한다. x64 Release 경고/오류 0·CTest 내부 132개·새 x86 1,536회가 통과했다. 누적 제한 입력은 57,564개다. 정상 다리/섬의 영역·부착 효과·그래프 삭제 분할/소진 복구·raw GameWorld 연결은 남았다. 이번 단계는 새 GUI 실행 없이 raw 계산/후처리를 검사했다.

**2026-10-06 Graph 계산:** [정수 표면 연결·flood·병합/표 관리](exe/cpp-graph-reconstruction.md)를 추가했다. 당시 Release·CTest 내부 128개·새 x86 2,560회가 통과했다. raw Pop/postPop 연결은 위 후속에서 추가했다.

**2026-10-06 공통 postPop 후속:** [비용 집계·목록·통계와 noGraph 리셋](exe/cpp-postpop-reconstruction.md)을 raw Pop에 선택 연결했다. x64 Release 경고/오류 0·CTest 내부 124개 검사·새 x86 984회와 기존 world 창 회귀가 통과했다. 최신 누적 제한 입력은 53,468개다. 표면 그래프/영역 통지·AI/배치 선택·생산 계산/SP 차감·raw GameWorld 연결은 남았다.

**2026-10-06 표시 활성 후속:** [공통 표시 갱신](exe/cpp-display-reconstruction.md)을 raw Pop/Unpop과 실제 Renderer 변경 표에 연결했다. Release·CTest 내부 120개 검사, 새 x86 1,728회와 두 판본 SHP 헤더 6,950개 읽기를 통과했다. renderer/world 창 회귀도 다시 실행했다. 공통 postPop 영역/소유자/생산 효과와 raw GameWorld 연결은 남았다.

**2026-10-06 최신 사용자 지시:** 이번 PC에서는 창이 뜨는 검사를 허용했다. Release 빌드·CTest와 클론 window/renderer/menu/world GUI 스모크를 실행했다. 원본/복사본 게임 프로세스는 실행하지 않았다. 이전 PC의 창 금지는 과거 작업 조건이다. [raw Pop 후속·실행 검증의 범위](exe/cpp-pop-reconstruction.md).

후속 [기본 생성/Take·가상 초기화](exe/cpp-creation-reconstruction.md)는 SID raw 풀과 타입 표에 연결했다. 자산 파생 생성자는 후속 연결했으며 form/process·실제 월드 연결은 남았다. 생성자 표 359행은 964회 새 기계어 호출과 별도로 센다. [자산 파생 생성자 후속](exe/cpp-derived-reconstruction.md)에서 153개 생성자·179개 타입 연결을 추가했다(6,028회). 이어 [raw 일반 공간 해제·non-void Take·firstPop 플래그](exe/cpp-unpop-reconstruction.md)를 표시 비활성 경로에서 연결했다(3,618회, vtable 메타데이터 151행 별도). 섬·다리·건물 부착/파생 효과·표시 활성·실제 Pop/삭제·월드 연결은 남았다. raw +8 word는 섬 번호로 정정했다.

저장소 루트에서 실행한다. Windows 10/11 과 Visual Studio 2022(또는 Build Tools)의 C++ 도구가 필요하다. 외부 라이브러리가 없어 인터넷 연결이 필요 없다.

```sh
cmake -S cpppj -B cpppj/build -DCMAKE_BUILD_TYPE=Release
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build --build-config Release --output-on-failure
```

* Windows 에서는 CMake 가 설치된 Visual Studio 를 찾아 쓴다(개발자 명령 프롬프트가 아니어도 된다). 실행 파일은 `cpppj/build/bin/Release/NetstormCpp.exe` 에 생긴다.
* **Windows 가 아니면 구성 단계에서 멈춘다**(`cpppj builds on Windows only`). 2026-10-05 이전에는 공용 코드를 Linux 에서도 컴파일해 보았지만, 클라이언트 코드가 Win32 를 직접 부르게 되면서 그만두었다.
* 프리셋을 쓰려면 `cpppj/` 에서 실행한다: `cmake --preset vs2022 && cmake --build --preset vs2022 && ctest --preset vs2022`.
* 필요한 도구는 `PREPARE.ps1` 의 "VS Build Tools 2022 (C++)"와 "CMake" 항목으로 설치한다.
* CI(`.github/workflows/ci.yml`)의 `cpp-build-test` 작업이 Windows 에서 위 세 명령을 실행한다.

인자 없이 실행하면 빌드 정보·검사 명령을 표시한다. 원본 게임을 실행하지 않고 복원한 모듈을 사용한다.

```text
NetstormCpp (menu and mission entry; playable world pending)
  reconstructed from: NetStorm 10.78 (decompiled)
  default maxFPS: 75 (frame interval 0.013333 s, 1ms clock: 14 ms)
```

단위 테스트는 외부 프레임워크 없이 `cpppj/tests/TestSupport.h` 의 `TEST_CASE`·`CHECK`·`CHECK_NEAR` 만 쓴다. 테스트 파일을 만들어 `cpppj/tests/CMakeLists.txt` 의 목록에 더하면 된다.

```powershell
cpppj/build/bin/Release/NetstormCpp.exe --inspect-data originals
cpppj/build/bin/Release/NetstormCpp.exe --inspect-data originalCD
python tools/cpp_recovery_smoke.py
cpppj/build/bin/Release/NetstormCpp.exe --inspect-assets originals
cpppj/build/bin/Release/NetstormCpp.exe --inspect-assets originalCD --cd
python tools/cpp_assets_smoke.py
cpppj/build/bin/Release/NetstormCpp.exe --config-spec originals missionSpec TEST01
python tools/cpp_config_smoke.py
cpppj/build/bin/Release/NetstormCpp.exe --inspect-mission originals TEST01
python tools/cpp_fort_smoke.py
cpppj/build/bin/Release/NetstormCpp.exe --dump-territories originals
cpppj/build/bin/Release/NetstormCpp.exe --run originals --view TEST01
python tools/cpp_window_smoke.py
python tools/cpp_renderer_smoke.py
cpppj/build/bin/Release/NetstormCpp.exe --run originals --window
python tools/cpp_menu_smoke.py
```

`--run <게임 폴더>`는 클론 창을 띄운다(원본 게임을 실행하지 않는다). `--view`가 없으면 **원본 메인 메뉴**를 표시한다. `--view types|fonts|<미션>`은 기존 독립 검사 장면이다. [메뉴·브리핑의 현재 범위/검사](exe/cpp-menu-reconstruction.md).

기존 기반에 `Gump`·`State`·`UberGump`·`GifImage`·`DialogScript`, `TerrainBuilder`·`Player`·`Squid` 일부와 새 `GameWorld`/부분 `UserInput`을 연결했다. 이어 CanonDecoder 다리 모양/추첨과 Bridge 열린 방향·수명 접두 구간을 복원했다. `SurfaceFinder`와 Bridge 확장은 정수 발자국의 flag 8 이웃·양쪽 타입/프레임 연결·붕괴 방문 목록을 계산한다. [표면/재귀의 근거·한계](exe/cpp-surface-reconstruction.md). CTest는 x86 기대값 **46,156개 입력 사례**(기존 5,365＋다리 18,053＋표면/재귀 3,450＋해시/점유 4,632＋공간 등록 878＋SID 3,168＋base 생성/Take 964＋자산 파생 생성/Take 6,028＋raw 일반 공간 해제/수명 3,618)를 포함한 **112개 내부 검사**다. 공간 등록은 가상/영역 효과의 계약 대체 조건, 새 raw 해제는 표시 비활성·합성 기존 배치 상태이며 대체 함수가 없다. 수명 480개는 접두 구간, 해시 초기화 4개는 할당 없는 경로이며 전체 열린 끝은 패치판만 검증했다. 실제 표면/SID·spot, 배치 UI·전체 붕괴는 아직 월드에 연결하지 않았다. 메뉴/월드는 독립 자료 대조와 클론 창 조작 기록이며 원본 기계어 전체 대조는 아니다. `--mission TEST01`로 같은 브리핑/월드를 검사한다. 원본 모듈 파일이 생긴 것을 모듈 전체 복원 완료로 세지 않는다.

`SquidHash`의 네 단계 배열/버킷 주소·객체 단계와 `Squid::EffectiveGenus`의 발자국/지붕 보정을 추가했다. [x87 실행 상태·검사·표면 배열의 관계](exe/cpp-hash-reconstruction.md)를 확인한다. 이어 `SquidSpatial`의 next 체인·등록/해제·spot OR/AND·비전투 상태와 등록 지도→정수 SurfaceFinder 전달을 복원했다. [CD 충돌 차이·조건부 기계어 검증·재현](exe/cpp-spatial-reconstruction.md). SID 풀은 후속 복원했다. [판본 경계·FIFO/예측 할당·반납·검증 제한](exe/cpp-sid-reconstruction.md). form/process 생성자·다른 가상/영역 효과·GameWorld 연결은 남았다.

## 4. 디컴파일한 소스에서 C++ 소스를 만드는 절차

재료는 [정밀 디컴파일](exe/decompile-reliability.md)의 결과(`extracted/refined/`, Git 에 없으므로 `tools\ghidra\refine_all.ps1` 로 만든다)와 [모듈 맵](exe/modules.md)이다.

1. **옮길 소스 파일을 고른다.** [SOURCE_MAP.md](../cpppj/SOURCE_MAP.md)에서 cpppj 경로를, [모듈 맵](exe/modules.md)에서 그 파일에 속한 함수 주소를 찾는다. assert 가 없는 함수는 모듈 맵에 없으므로, 주소가 그 파일의 함수들 사이에 있는 이웃 함수도 같이 본다(한 소스 파일의 함수는 exe 안에서 대체로 이어져 있다).
2. **두 판본을 나란히 읽는다.** `python tools/decomp_refine.py --show <주소>` 가 패치판 함수와 CD판의 짝을 함께 보여 준다. 컴파일러가 달라 한쪽이 모호하면 다른 쪽이 답을 주는 경우가 많다. 기준은 **패치판(10.78)** 이다. 두 판본의 동작이 다르면 패치판을 따르고 차이를 주석에 적는다.
3. **신뢰도 등급을 본다.** 머리말의 `[신뢰도 A~D]` 는 "그 주소에 진짜 함수가 있고 경계가 맞는가"에 대한 것이다. D 등급과 `미추정 레지스터` 표시가 있는 함수는 역어셈블(`tools\ghidra\decompile_at.ps1`)로 확인한 뒤 옮긴다.
4. **C++ 로 다시 쓴다.** 디컴파일 출력을 그대로 붙여 넣지 않는다. 뜻이 드러나는 이름과 형식으로 고쳐 쓰고 5절의 규칙을 따른다. 이미 분석 문서(`docs/exe/`, `docs/formats/`, `docs/gameplay/`)가 있는 부분은 그 문서의 이름과 해석을 쓴다.
5. **테스트를 붙인다.** 순수 계산(좌표·추첨·타이머·파서)은 `cpppj/tests/` 에 테스트를 만든다. 원본 데이터 파일을 읽는 코드는 `assets/game-data/` 로 검증한다.
6. `cpppj/src/<폴더>/CMakeLists.txt` 의 목록에 파일을 더하고, `python tools/cpp_source_map.py` 로 표의 상태를 갱신한다.

메인 루프의 뼈대는 [main-loop.md](exe/main-loop.md)에 정리되어 있다: `WinMain`(`FUN_00438dc0`) → 시각 고정(`FUN_00460e90`) → 입력·명령(`FUN_004d62b0`, UserInput.cpp) → 프로세스 커널(`FUN_00471a30`, Kernel.cpp) → 그리기(`FUN_00436450`).

## 5. 다시 만든 코드의 규칙

AGENTS.md 의 규칙(한국어 주석, UTF-8, 상수·함수·반복문마다 주석)에 더해 다음을 지킨다.

**출처 주석.** 원본에서 옮긴 함수와 상수에는 근거를 적는다. 견본은 `cpppj/src/client/ClientMain.cpp` 다.

```cpp
// 원본: FUN_00435220 @ 00435220 (패치판 10.78) [신뢰도 A] ↔ CD판 FUN_00484ac0 @ 00484ac0 (string/strong)
// 소속: 주소가 Clientdebug.cpp(00434a00)와 Clientmain.cpp(00435e40) 사이여서 ClientMain.cpp 로 추정한다.
// 범위: 설정을 읽는 함수 가운데 maxFPS 부분만 옮겼다.
```

* **원본**: 패치판 함수 주소, 신뢰도 등급, CD판의 짝(있으면 방법/수준).
* **소속**: 소스 파일이 assert 문자열로 확인된 것이 아니면 "추정"이라고 쓰고 근거를 적는다.
* **범위**: 함수의 일부만 옮겼으면 어디까지인지 적는다.
* 전역 변수·상수는 원본 주소(`DAT_005318d8`)를 적는다. 확인하지 못한 해석에는 "(추정)"을 붙인다.
* 원본에 없는 코드(`platform/`, `app/`, 와이드 화면·60/120프레임·한국어 지원)는 파일 머리에 "새로 쓰는 코드"라고 적는다.

**32비트 코드를 64비트로 옮길 때.** 원본은 32비트 x86 실행 파일이고 cpppj 는 64비트로 빌드한다.

* 디컴파일 출력의 `*(int *)(param_1 + 0x1c)` 같은 **구조체 오프셋을 코드에 그대로 쓰지 않는다.** 이름 있는 멤버를 가진 구조체·클래스로 옮기고, 원본 오프셋은 멤버 주석에 적는다. 메모리 배치를 원본과 같게 맞출 필요는 없다.
* 포인터를 `int` 에 넣거나 포인터 크기가 4바이트라고 가정한 코드는 고쳐 쓴다.
* **파일·네트워크 형식**(`.fort`, TAFF, `.shp`, 패킷)은 배치가 바뀌면 안 된다. 구조체를 통째로 읽지 말고 고정 폭 형식(`std::uint32_t` 등)으로 필드마다 리틀 엔디언으로 읽고 쓴다.
* 정수 오버플로·부호·자리 옮김은 원본과 같은 폭과 부호로 계산한다(`int` 32비트, `short` 16비트, `char` 의 부호는 MSVC 처럼 `signed`).
* 원본은 시각을 `double`(초)로 다룬다. 원본과 같은 순서로 계산한다.

**OS 호출(2026-10-05 변경).** 원본과 같은 방식으로 만든다는 원칙에 따라, **원본 모듈을 옮긴 `client/` 의 코드는 원본이 부른 Win32 함수를 그 자리에서 직접 부른다**(`ClientMain.cpp`·`Screen.cpp` 가 견본이다). 헤더에는 `<windows.h>` 를 넣지 않고 핸들을 `NativeHandle`(`void*`)로 넘긴다. `o/`(공용 모듈)는 OS 호출 없이 둔다. `platform/` 은 원본에 없는 보조 기능(콘솔 출력·BMP 저장·파일 쓰기)만 담는다. 원본의 호출 가운데 아직 옮기지 않은 것은 그 자리에 `[원본]` 주석으로 남긴다.

**이름.** 원본 exe 에는 기호가 없다. 형식·함수 이름은 문자열(설정 키, assert 조건식, `.type` 파일의 키)과 분석 문서에서 가져오고, 근거가 없으면 뜻을 설명하는 영어 이름을 새로 짓는다. 네임스페이스는 `netstorm::o`, `netstorm::client`, `netstorm::zacket`, `netstorm::platform` 을 쓴다.

## 6. 정하지 않은 것

* ~~플랫폼 계층의 라이브러리~~ → **정함(2026-10-05): Win32 직접 호출, 외부 라이브러리 없음.** 소리·글꼴·영상·전체화면(DirectDraw)의 방식은 해당 모듈을 옮길 때 원본 방식을 기본으로 정한다 — [창·화면 장치 복원](exe/cpp-screen-reconstruction.md) 1·8절.
* ~~설정 저장 위치~~ → **구현 완료(2026-10-05): `options.cfg`는 원본 게임과 동일하게 처리한다.** `<게임 폴더>/d/options.cfg`를 시작·변경 저장 시점에 Windows-1252·XOR로 쓴다. 시작 표시의 조회·생성·메인 버튼 사건의 삭제도 연결했다. [옵션 저장 복원](exe/cpp-options-reconstruction.md). 스모크는 허용한 두 파일을 보관하고 복구한다.
* **한국어 글꼴.** D2Coding(TTC)을 그리는 방법이 필요하다(C# 빌드는 FontStashSharp 를 쓴다). cpppj 에서는 후순위다.
* **원본에 없는 기능을 넣는 방식.** 와이드 화면, 60·120프레임, 전체화면 재실행 오류 수정은 원본 코드에 없다. 다시 만든 코드를 어디서 어떻게 바꿀지는 해당 모듈을 옮길 때 정한다. C# 빌드의 결정(와이드 화면 = 시야 확장, [LEFT_JOBS.md](../LEFT_JOBS.md) 1.7절)을 따르는 것이 기본이다.
* **옮기는 순서.** [후속 복원 계획](cpp-roadmap.md)의 단계별 완료 기준을 따른다.
  - 2026-10-05: 파일 읽기·원시 설정·번역, 타입 자산·SHP·프레임 코드/검색, 커널 실행 인터페이스·시계, 팔레트·기본 8비트 합성 일부를 복원했다. 다음은 **Config 치환→`.fort`·타입 연결→TEST01 정적 창→실제 프로세스·입력→캠페인 1-1 플레이**다. 전체 모듈 이식 완료로 보지 않는다.
  - 2026-10-05(이어서): 설정·타입·요새 파일·영역 배치·Win32 창·창 모드 화면·입력 큐·메인 루프 뼈대와 **Renderer·원본 글꼴·커서 표시 기반**을 옮겼다. 다음은 [현재 계획](cpp-playable-plan.md)의 **메인 메뉴/브리핑→실제 월드/조작→1-1 완주**다.
* **두 빌드의 결과를 비교하는 방법.** 같은 미션에서 C# 빌드와 C++ 빌드의 동작을 대조하는 도구는 없다.
