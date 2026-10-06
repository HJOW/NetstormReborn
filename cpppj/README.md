# cpppj — NetStorm C++ 빌드

기존 게임(NetStorm: Islands at War)을 디컴파일한 소스를 토대로 C++ 소스를 다시 만드는 프로젝트다.
C# + MonoGame 빌드(`../dotnetpj/`)는 cpppj 완성 후 이를 분석하여 개발한다.

**방향(AGENTS.md):** 기존 게임을 디컴파일하여 C++ 코드로 **최대한 복원하는 것이 1차 목표**다. 이후 Windows 10/11에서 실행 가능한 수준으로 만들고, 요구사항 반영과 MCP 추가로 이어 간다. 기능 변경을 최소로 하고 원본과 같은 방식으로 만든다. `options.cfg`는 원본과 같은 시작·변경 저장 시점에 `<게임 폴더>/d/options.cfg`로 쓴다(Windows-1252·XOR). 그래서 창·화면·입력은 원본처럼 **Win32 API를 직접 부른다**(외부 라이브러리 없음). **Windows 전용**이다.

**현재 우선순위(2026-10-05 사용자 요청): 기존 게임이 온전히 동작하고 실제 게임 플레이가 가능하도록 복원한다.** 먼저 메인 메뉴→캠페인 선택→브리핑→선택·이동·건설·경제·전투→승패·결과·재시작을 완성하고, 나머지 원본 기능·캠페인까지 복원한다. 화면비 확장·한국어·Linux·60/120프레임·MCP·추가 기능은 후순위다. 작업 순서와 완료 기준은 [실제 플레이 복원 계획](../docs/cpp-playable-plan.md)을 따른다.

**원본 메뉴→캠페인→브리핑→실제 지형/객체·선택·사제 이동→메뉴 복귀를 연결했다.** 1-1과 TEST01의 시작 SP/동맹·지형·점유를 적용하고, 몸통 좌클릭 선택→땅 좌클릭 이동, 우클릭 메뉴, 화면 이동·일시정지·재진입을 검사했다. **건설·채집·전투·AI·승패·사운드 출력·DirectDraw 전체화면은 후속이며 미션 완주는 아직 불가능하다.** [월드/조작의 범위와 제한](../docs/exe/cpp-world-reconstruction.md), [메뉴 연결](../docs/exe/cpp-menu-reconstruction.md).

[복원 근거·함수 대응·검증 범위](../docs/exe/cpp-reconstruction.md), [검토 목록](recovery-manifest.json), [기계어 검증 기록](recovery-evidence.json).

[타입·그래픽 복원과 판본 차이](../docs/exe/cpp-assets-reconstruction.md), [VFX 기계어 검증 기록](recovery-graphics-evidence.json).

[설정 계층 복원](../docs/exe/cpp-config-reconstruction.md), [설정 기계어 검증 기록](recovery-config-evidence.json), [타입 표·요새 파일 복원](../docs/exe/cpp-fort-reconstruction.md).

[창·화면 장치·입력 큐·영역 배치 복원](../docs/exe/cpp-screen-reconstruction.md) — 플랫폼 결정(Win32), 원본 시작 순서와 옮긴 범위, 원본과 다르게 둔 것.

[옵션 저장·전체화면 시작 표시 복원](../docs/exe/cpp-options-reconstruction.md) — 저장 시점·바이트 대조, 시작 표시 조회/생성, 전체화면 실패 시 원본도 창 모드로 이어진다는 분석 정정.

[Renderer·글꼴·커서 복원](../docs/exe/cpp-renderer-reconstruction.md), [표시 기계어 검증 기록](recovery-renderer-evidence.json) — 현재 계획의 1단계 표시 기반과 후속 연결점.

[다리 계산 복원·CD판 차이·신뢰도 보강](../docs/exe/cpp-bridge-reconstruction.md), [다리 기계어 검증 기록](recovery-bridge-evidence.json) — 모양·추첨·회전/프레임·열린 끝·수명 접두 구간을 복원했다. 배치 UI·표면 그래프·전체 붕괴의 월드 연결은 후속이다.

[표면 이웃·연결·붕괴 방문 목록](../docs/exe/cpp-surface-reconstruction.md), [표면 기계어 검증 기록](recovery-surface-evidence.json) — 정수 발자국/flag 8 이웃과 재귀 계산을 추가했다. 실제 표면/SID·spot의 GameWorld 연결과 수명/삭제·배치 UI는 후속이다.

[공간 해시·점유 비트 복원](../docs/exe/cpp-hash-reconstruction.md), [해시 기계어 검증 기록](recovery-hash-evidence.json) — 4단계 배열/버킷 주소·객체 단계·발자국/지붕 genus를 복원했다. x87 제어 워드를 명시해 두 판본과 53/64비트 결과를 확인했다.

[객체 공간 등록·해제](../docs/exe/cpp-spatial-reconstruction.md), [등록 기계어 검증 기록](recovery-spatial-evidence.json) — Pop/Unpop의 next 체인·spot OR/AND·좌표 캐시·비전투 상태와 CD판 점유 충돌 차이를 복원했다. 실제 등록 지도를 정수 SurfaceFinder에 전달한다. 가상/영역 효과는 반환 계약·사건 목록이며 SID 풀은 아래 후속에서 복원했으며 실제 GameWorld 연결은 남았다.

[SID 풀·번호 할당/반납](../docs/exe/cpp-sid-reconstruction.md), [SID 기계어 검증 기록](recovery-sid-evidence.json) — 두 판본의 raw 슬롯·번호 경계·FIFO/예측 할당·타입 보존 반납·삭제 기록·서버 목록 재구성을 복원했다. 새 3,168회는 두 판본 합계이며 기존 메모리 초기화와 소진 전 경로만 검증했다. 이 경로에는 세대 비트가 없다. 파생 생성자·공간 수명 효과·GameWorld 연결은 남았다.

**2026-10-06 작업 제한:** 현재 PC에서는 원본 게임·복사본을 실행하거나 클론 창을 띄우는 검사를 진행하지 않는다. 아래 `--run`과 window/renderer/menu/world 스모크 명령은 다른 PC의 실행 확인용이다. 이번 작업은 정적 디컴파일·기계어 격리 검사·빌드·콘솔 CTest만 수행했다.

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
cpppj/build/bin/Release/NetstormCpp.exe --run originals --window
python tools/cpp_menu_smoke.py
cpppj/build/bin/Release/NetstormCpp.exe --run originals --window --mission TEST01
cpppj/build/bin/Release/NetstormCpp.exe --dump-world originals thewarbegins
python tools/cpp_world_smoke.py
cpppj/build/bin/Release/NetstormCpp.exe --inspect-bridges originals
cpppj/build/bin/Release/NetstormCpp.exe --inspect-bridges originalCD --cd
python tools/cpp_bridge_smoke.py
```

`--run`은 클론 창을 띄운다(원본 게임을 실행하지 않는다). 기본은 메인 메뉴다. Campaign→Struggle For Freedom→1 The War Begins!→Play Mission에서 사제를 좌클릭해 선택하고 땅을 좌클릭해 이동한다. 화살표로 화면 이동, F4는 신전/F5는 사제 보기, Game/ESC는 정지/복귀다. `--mission TEST01`은 같은 브리핑/월드의 검사 진입점이다. `--view TEST01`은 옛 정적 검사 화면으로 화살표 이동/Esc 종료, `--view fonts`는 글꼴 표시다. `--dump-world`는 원본 파일을 읽기만 한다. [자세한 조작과 검사 옵션](../docs/exe/cpp-world-reconstruction.md).

`--run`은 원본과 공유하는 `d/options.cfg`를 갱신한다. 전체화면을 요구하면 게임 폴더의 `fullscreenStateFile.dat`도 원본 시점에 만든다. DirectDraw는 아직 없으므로 창 모드로 나온다. 스모크는 두 파일을 보관하고 검사 후 바이트·존재 여부를 복구한다.

CTest의 **99개 내부 검사**에는 원본 기계어의 **35,546개 입력 사례**가 포함되어 있다(기존 5,365＋다리 계산 18,053＋표면/재귀 3,450＋해시/점유 4,632＋공간 등록 878＋SID 3,168). 공간 등록은 가상/영역 효과를 계약으로 대체한 조건부 검증이다. 수명 480개는 첫 외부 효과 전의 접두 구간, 해시 초기화 4개는 할당 없는 경로이고 전체 열린 끝은 패치판만 검증했다. 빌드에는 원본 실행 파일·Ghidra·Python·외부 라이브러리가 필요 없다. 기존 메뉴 54개 상태와 월드의 **6개 초기 자료 사례·393,216마스크 바이트·2,593객체·20개 조작 상태** 기록을 유지하며, 다리 자산 검사는 두 판본 합계 **936셀**을 독립 판독과 대조한다. `--inspect-bridges`는 파일을 읽기만 한다. 메뉴/월드 전체 함수의 x86 대조나 원본 화면 전체 픽셀 일치를 의미하지 않는다. 설정을 공유하는 창 스모크는 순차 실행하고 원본 파일을 복구한다. 이 수치는 게임 전체 완성도가 아니다.

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
