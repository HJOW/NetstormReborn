# cpppj — NetStorm C++ 빌드

기존 게임(NetStorm: Islands at War)을 디컴파일한 소스를 토대로 C++ 소스를 다시 만드는 프로젝트다.
C# + MonoGame 빌드(`../dotnetpj/`)는 cpppj 완성 후 이를 분석하여 개발한다.

**방향(AGENTS.md):** 기존 게임을 디컴파일하여 C++ 코드로 **최대한 복원하는 것이 1차 목표**다. 이후 Windows 10/11에서 실행 가능한 수준으로 만들고, 요구사항 반영과 MCP 추가로 이어 간다. 기능 변경을 최소로 하고 원본과 같은 방식으로 만든다. `options.cfg`는 원본과 같은 시작·변경 저장 시점에 `<게임 폴더>/d/options.cfg`로 쓴다(Windows-1252·XOR). 그래서 창·화면·입력은 원본처럼 **Win32 API를 직접 부른다**(외부 라이브러리 없음). **Windows 전용**이다.

**현재 우선순위(2026-10-05 사용자 요청): 기존 게임이 온전히 동작하고 실제 게임 플레이가 가능하도록 복원한다.** 먼저 메인 메뉴→캠페인 선택→브리핑→선택·이동·건설·경제·전투→승패·결과·재시작을 완성하고, 나머지 원본 기능·캠페인까지 복원한다. 화면비 확장·한국어·Linux·60/120프레임·MCP·추가 기능은 후순위다. 작업 순서와 완료 기준은 [실제 플레이 복원 계획](../docs/cpp-playable-plan.md)을 따른다.

**공용 계층·타입·그래픽 자산 로더·설정·요새 파일·영역 배치·Win32 창·입력 큐와 Renderer·원본 글꼴·커서의 표시 기반이 빌드된다.** TAFF·설정(객체 층·치환·값 쓰기·경로 지정값)·번역·타입(번호 체계·플래그)·프레임 검색·프로세스 커널·게임 시계, SHP 압축 해제·팔레트, `.fort` 읽기와 미션 경로, 영역 패턴에 따른 청크 위치를 복원했다. 화면은 8비트 DIB에 변경 영역만 합성하며 깊이 정렬·색/그림자 변환·프레임 캐시·영어 비트맵 글꼴·원본 커서를 연결했다. **메인 메뉴·실제 월드·게임 플레이·사운드·DirectDraw 전체화면은 아직 없다.** 현재 검사용 장면은 실제 게임 화면의 복원 완료를 뜻하지 않는다.

[복원 근거·함수 대응·검증 범위](../docs/exe/cpp-reconstruction.md), [검토 목록](recovery-manifest.json), [기계어 검증 기록](recovery-evidence.json).

[타입·그래픽 복원과 판본 차이](../docs/exe/cpp-assets-reconstruction.md), [VFX 기계어 검증 기록](recovery-graphics-evidence.json).

[설정 계층 복원](../docs/exe/cpp-config-reconstruction.md), [설정 기계어 검증 기록](recovery-config-evidence.json), [타입 표·요새 파일 복원](../docs/exe/cpp-fort-reconstruction.md).

[창·화면 장치·입력 큐·영역 배치 복원](../docs/exe/cpp-screen-reconstruction.md) — 플랫폼 결정(Win32), 원본 시작 순서와 옮긴 범위, 원본과 다르게 둔 것.

[옵션 저장·전체화면 시작 표시 복원](../docs/exe/cpp-options-reconstruction.md) — 저장 시점·바이트 대조, 시작 표시 조회/생성, 전체화면 실패 시 원본도 창 모드로 이어진다는 분석 정정.

[Renderer·글꼴·커서 복원](../docs/exe/cpp-renderer-reconstruction.md), [표시 기계어 검증 기록](recovery-renderer-evidence.json) — 현재 계획의 1단계 표시 기반과 후속 연결점.

[실제 플레이 복원 계획](../docs/cpp-playable-plan.md)에 현재 실행 순서를, [기반 복원 로드맵](../docs/cpp-roadmap.md)에 완료된 데이터 기반·세부 복원 절차를 정리했다.

## 빌드와 테스트

Windows 10/11, CMake 3.21 이상, C++20 컴파일러(Visual Studio 2022 이상 또는 Build Tools)가 필요하다. 다른 OS에서는 구성 단계에서 멈춘다. 저장소 루트에서 실행한다.

```sh
cmake -S cpppj -B cpppj/build -DCMAKE_BUILD_TYPE=Release
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build --build-config Release --output-on-failure
```

실행 파일은 `cpppj/build/bin/Release/NetstormCpp.exe` 에 생긴다.

```powershell
cpppj/build/bin/Release/NetstormCpp.exe --inspect-data originals
cpppj/build/bin/Release/NetstormCpp.exe --inspect-data originalCD
python tools/cpp_recovery_smoke.py
cpppj/build/bin/Release/NetstormCpp.exe --inspect-assets originals
cpppj/build/bin/Release/NetstormCpp.exe --inspect-assets originalCD --cd
python tools/cpp_assets_smoke.py
cpppj/build/bin/Release/NetstormCpp.exe --export-frame originals sunCannon N00 0 extracted/sunCannon.bmp
cpppj/build/bin/Release/NetstormCpp.exe --config-spec originals missionSpec TEST01
python tools/cpp_config_smoke.py
cpppj/build/bin/Release/NetstormCpp.exe --inspect-mission originals TEST01
cpppj/build/bin/Release/NetstormCpp.exe --inspect-fort originals thewarbegins
python tools/cpp_fort_smoke.py
cpppj/build/bin/Release/NetstormCpp.exe --dump-territories originals
cpppj/build/bin/Release/NetstormCpp.exe --run originals --view types
cpppj/build/bin/Release/NetstormCpp.exe --run originals --view TEST01
python tools/cpp_window_smoke.py
cpppj/build/bin/Release/NetstormCpp.exe --run originals --view fonts --window --render-stats
python tools/cpp_renderer_smoke.py
```

`--run` 은 원본 방식의 창을 띄운다(원본 게임을 실행하지 않는다). `--view` 가 없으면 원본의 "Loading / Please Wait" 화면만 보인다. `--view TEST01` 은 화살표 키로 움직이고 Esc로 닫는다. `--view fonts`는 18개 글꼴/스타일을 표시한다. 검사 장면은 Renderer에 목록을 공급하는 임시 어댑터이며 실제 월드·게임 화면의 재현은 후속이다. `--render-stats`는 그리기 프레임·출력 사각형 횟수, `--dump-font <게임 폴더> <글꼴 경로>`는 독립 비교용 이진 글리프 스트림을 출력한다.

`--run`은 원본과 공유하는 `d/options.cfg`를 갱신한다. 전체화면을 요구하면 게임 폴더의 `fullscreenStateFile.dat`도 원본 시점에 만든다. DirectDraw는 아직 없으므로 창 모드로 나온다. 스모크는 두 파일을 보관하고 검사 후 바이트·존재 여부를 복구한다.

CTest의 52개 테스트에는 두 판본 기계어의 5,365개 기대값 검사가 포함되어 있다(공용 1,806개＋VFX 709개＋설정 2,440개＋옵션 저장 16개＋Renderer 394개). 빌드에는 원본 실행 파일·Ghidra·Python·외부 라이브러리가 필요 없다. 실제 자산 검사는 원본 게임을 실행하지 않는다. 영역 2,119개 전수 대조와 창 픽셀 대조(`cpp_window_smoke.py`), 글꼴 18개·4,608글리프·글자 창 전체 픽셀 대조와 두 판본의 소프트웨어 커서 생성(`cpp_renderer_smoke.py`)을 확인한다. 전체 게임 완성도를 이 검사 수치로 판단하지 않는다.

## 폴더

| 폴더 | 내용 |
|---|---|
| `src/o/` | 원본 `\Ns\O\` 의 공용 모듈 (게임 규칙·데이터·프로세스 커널) |
| `src/client/` | 원본 클라이언트 모듈 (화면·입력·소리·메인 루프). 원본처럼 Win32 를 직접 부른다 |
| `src/zacket/` | 원본 `\Ns\Zacket\` 의 네트워크 패킷 |
| `src/platform/` | 새로 쓰는 코드 — 원본에 없는 보조 기능(콘솔 출력·BMP 저장·파일 쓰기) |
| `src/app/` | 새로 쓰는 코드 — 실행 파일 진입점, 임시 검사용 화면 |
| `tests/` | 단위 테스트 (`TestSupport.h` 의 `TEST_CASE`·`CHECK`) |
| `cmake/` | 공통 컴파일 옵션 |

원본 소스 파일이 어느 경로로 가는지는 [SOURCE_MAP.md](SOURCE_MAP.md)에 있다(`python tools/cpp_source_map.py` 로 생성).

## 소스를 옮기는 방법과 규칙

[docs/cpp-build.md](../docs/cpp-build.md)에 정리했다: 디컴파일 결과에서 C++ 소스를 만드는 절차, 출처 주석 규칙, 32비트 코드를 64비트로 옮길 때의 주의, 아직 정하지 않은 것.
