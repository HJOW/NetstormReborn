# C++ 빌드 (`cpppj/`)

> 2026-10-05 추가·갱신. **공용 계층 1차 복원 소스가 빌드되고 원본/CD 자산을 읽는다.** 화면·전투·게임 전체 루프·MCP는 아직 구현하지 않았다. [복원 근거·검증](exe/cpp-reconstruction.md).

## 1. 두 빌드를 함께 개발한다

저장소에는 게임 빌드가 둘 있다. 서로의 코드를 참조하지 않는다.

| | C# + MonoGame 빌드 | C++ 빌드 |
|---|---|---|
| 폴더 | [`dotnetpj/`](../dotnetpj/) | [`cpppj/`](../cpppj/) |
| 만드는 방법 | 원본을 분석해 규칙·화면을 새로 구현한다 (클론 코딩) | **기존 게임을 디컴파일한 소스를 토대로 C++ 소스를 다시 만든다** |
| 빌드 도구 | .NET 10 SDK (`dotnetpj/Netstorm.sln`) | CMake 3.21 이상 + C++20 컴파일러 |
| 실행 파일 | `NetstormClone` | `NetstormCpp` |
| 상태 | 캠페인 1-1·1-2 플레이 가능 | 공용 계층 일부 복원, 자산 검사 실행 가능 |

두 빌드가 함께 쓰는 것은 저장소 루트에 그대로 있다.

* `assets/game-data/` — 클론 게임 데이터 ([클론 게임 데이터](../assets/README.md))
* `fonts/` — 한국어 글꼴 D2Coding
* `docs/` — 분석 문서. 원본 동작에 대한 사실은 두 빌드가 같은 문서를 근거로 삼는다.
* `tools/` — Python 추출·검증 도구, Ghidra 스크립트
* `analyzeManager/` — 원본 게임 자동 탐험 프로그램. C# 으로 쓴 **분석 도구**라서 `dotnetpj/` 로 옮기지 않았다. `dotnetpj/src/Netstorm.Assets` 를 참조하고, 공통 빌드 설정은 `analyzeManager/Directory.Build.props` 가 `dotnetpj/` 의 것을 가져온다.

공통 목표(AGENTS.md)는 영어·한국어, 풀스크린과 16:9·16:10·4:3, 원본 수준의 프레임을 먼저 맞추고 이후 60·120프레임이다. **cpppj는 Windows용으로 개발**하며 Linux 지원은 C# 빌드의 후순위 목표다. C++ 공용 코드의 Linux 빌드는 이식성 검사에 활용할 수 있다.

## 2. 폴더 구조

원본 exe 의 assert 문자열에는 원본 소스 경로가 남아 있다. 폴더는 그 원본 소스 트리를 따른다.

```text
cpppj/
  CMakeLists.txt          최상위 빌드 정의 (C++20, 테스트 옵션)
  CMakePresets.json       구성 프리셋: vs2022(Windows), ninja(Linux 등)
  cmake/
    CompilerOptions.cmake 공통 컴파일 옵션 (UTF-8, 경고 수준)
  SOURCE_MAP.md           원본 소스 파일 → cpppj 경로 표 (자동 생성)
  src/
    o/                    원본 \Ns\O\      공용 모듈 (게임 규칙·데이터·프로세스 커널)
    client/               원본 .\          클라이언트 모듈 (화면·입력·소리·메인 루프)
    zacket/               원본 \Ns\Zacket\ 네트워크 패킷
    platform/             새로 쓰는 코드   원본의 Win32·DirectX 호출을 대신하는 계층
    app/                  새로 쓰는 코드   실행 파일 진입점 (main.cpp)
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
* 지금은 정적 라이브러리 하나(`netstorm`)로 묶는다. `o/` 는 `client/`·`platform/` 을 포함하지 않는 것이 규칙이지만 빌드가 강제하지는 않는다.

## 3. 빌드와 테스트

저장소 루트에서 실행한다. 외부 라이브러리가 없어 인터넷 연결이 필요 없다.

```sh
cmake -S cpppj -B cpppj/build -DCMAKE_BUILD_TYPE=Release
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build --build-config Release --output-on-failure
```

* Windows 에서는 CMake 가 설치된 Visual Studio 를 찾아 쓴다(개발자 명령 프롬프트가 아니어도 된다). 실행 파일은 `cpppj/build/bin/Release/NetstormCpp.exe` 에 생긴다.
* Linux 에서는 `g++`(또는 `clang++`)과 `make`(또는 `ninja`)가 필요하다. 실행 파일은 `cpppj/build/bin/NetstormCpp` 에 생긴다.
* 프리셋을 쓰려면 `cpppj/` 에서 실행한다: `cmake --preset vs2022 && cmake --build --preset vs2022 && ctest --preset vs2022` (Linux 는 `ninja`).
* 필요한 도구는 `PREPARE.ps1`·`PREPARE.sh` 의 "VS Build Tools 2022 (C++)"(Linux 는 "C++ 빌드 도구")와 "CMake" 항목으로 설치한다.
* CI(`.github/workflows/ci.yml`)의 `cpp-build-test` 작업이 Windows 와 Ubuntu 에서 위 세 명령을 실행한다.

인자 없이 실행하면 빌드 정보·검사 명령을 표시한다. 원본 게임을 실행하지 않고 복원한 모듈을 사용한다.

```text
NetstormCpp (reconstructed core; game UI pending)
  reconstructed from: NetStorm 10.78 (decompiled)
  default maxFPS: 75 (frame interval 0.013333 s, 1ms clock: 14 ms)
```

단위 테스트는 외부 프레임워크 없이 `cpppj/tests/TestSupport.h` 의 `TEST_CASE`·`CHECK`·`CHECK_NEAR` 만 쓴다. 테스트 파일을 만들어 `cpppj/tests/CMakeLists.txt` 의 목록에 더하면 된다.

```powershell
cpppj/build/bin/Release/NetstormCpp.exe --inspect-data originals
cpppj/build/bin/Release/NetstormCpp.exe --inspect-data originalCD
python tools/cpp_recovery_smoke.py
```

현재 소스: `BaseFile`·`Config`·`Xlat`·`RiftType`·`BaseProcess`·`Kernel`·`GameClock`, 신규 인코딩·콘솔 계층. 각 모듈의 복원 범위와 미구현 부분은 [C++ 복원 문서](exe/cpp-reconstruction.md)에 있다. CTest는 두 판본의 x86 기계어 기대값 1,806개를 포함한 10개 테스트를 실행한다.

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

**OS 호출.** `o/`·`client/`·`zacket/` 의 코드는 Win32·DirectDraw·DirectSound·소켓을 직접 부르지 않고 `platform/` 의 함수를 부른다. 원본의 해당 호출 지점에는 원본이 무엇을 불렀는지 주석으로 남긴다.

**이름.** 원본 exe 에는 기호가 없다. 형식·함수 이름은 문자열(설정 키, assert 조건식, `.type` 파일의 키)과 분석 문서에서 가져오고, 근거가 없으면 뜻을 설명하는 영어 이름을 새로 짓는다. 네임스페이스는 `netstorm::o`, `netstorm::client`, `netstorm::zacket`, `netstorm::platform` 을 쓴다.

## 6. 정하지 않은 것

* **플랫폼 계층의 라이브러리.** 창·화면·입력·소리를 무엇으로 만들지(SDL 등) 정하지 않았다. 정하면 외부 라이브러리를 가져오는 방법(vcpkg, CMake `FetchContent`, 시스템 패키지)도 함께 정한다.
* **한국어 글꼴.** D2Coding(TTC)을 그리는 방법이 필요하다(C# 빌드는 FontStashSharp 를 쓴다).
* **원본에 없는 기능을 넣는 방식.** 와이드 화면, 60·120프레임, 전체화면 재실행 오류 수정은 원본 코드에 없다. 다시 만든 코드를 어디서 어떻게 바꿀지는 해당 모듈을 옮길 때 정한다. C# 빌드의 결정(와이드 화면 = 시야 확장, [LEFT_JOBS.md](../LEFT_JOBS.md) 1.7절)을 따르는 것이 기본이다.
* **옮기는 순서.** 제안: ① `o/` 의 파일·설정 계층(BaseFile·Config·Xlat·StaticString) → ② 데이터 형식(Template·RiftType·DataManager) → ③ 프로세스 커널과 시계(Kernel·BaseProcess) → ④ 플랫폼 계층과 화면(Screen·Renderer) → ⑤ 메인 루프와 입력(ClientMain·UserInput).
  - 2026-10-05: ① 읽기·원시 설정·번역, ② 프레임 검색, ③ 커널 실행 인터페이스·시계의 일부를 복원했다. 다음은 **Config 치환·`.type`·`.shp`·`.fort` 로더**, SID와 실제 프로세스 클래스, 화면·입력 순이다. 전체 모듈 이식 완료로 보지 않는다.
* **두 빌드의 결과를 비교하는 방법.** 같은 미션에서 C# 빌드와 C++ 빌드의 동작을 대조하는 도구는 없다.
