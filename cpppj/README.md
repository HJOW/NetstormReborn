# cpppj — NetStorm C++ 빌드

기존 게임(NetStorm: Islands at War)을 디컴파일한 소스를 토대로 C++ 소스를 다시 만드는 프로젝트다.
C# + MonoGame 빌드(`../dotnetpj/`)와 함께 개발한다.

**방향(AGENTS.md):** 기존 게임을 디컴파일하여 C++ 코드로 **최대한 복원하는 것이 1차 목표**다. 이후 Windows 10/11에서 실행 가능한 수준으로 만들고, 요구사항 반영과 MCP 추가로 이어 간다. 기능 변경을 최소로 하고 원본과 같은 방식으로 만든다. `options.cfg`도 원본 게임과 동일하게 처리하기로 정했다(구현 전 — 지금은 읽기만 한다). 그래서 창·화면·입력은 원본처럼 **Win32 API를 직접 부른다**(외부 라이브러리 없음). **Windows 전용**이다.

**공용 계층, 타입·그래픽 자산 로더, 설정 계층, 요새 파일 로더, 영역 배치, 원본 방식의 창·화면 장치(창 모드)·입력 큐가 빌드된다.** TAFF·설정(객체 층·치환·값 쓰기·경로 지정값)·번역·타입(번호 체계·플래그)·프레임 검색·프로세스 커널·게임 시계, SHP 압축 해제·기본 8비트 합성·팔레트, `.fort` 읽기와 미션 스크립트 경로, 영역 패턴에 따른 청크 위치, WinMain 시작 순서·창 프로시저·8비트 DIB 화면·입력 사건 큐를 복원했다. 원본/CD 자산 전체를 검사하고 프레임을 투명 BMP로 내보낼 수 있으며, 원본 방식의 창에 타입 표나 미션의 오브젝트를 보여 주는 검사용 화면이 있다. Renderer·게임 화면·전투·전체화면·MCP는 후속 작업이다.

[복원 근거·함수 대응·검증 범위](../docs/exe/cpp-reconstruction.md), [검토 목록](recovery-manifest.json), [기계어 검증 기록](recovery-evidence.json).

[타입·그래픽 복원과 판본 차이](../docs/exe/cpp-assets-reconstruction.md), [VFX 기계어 검증 기록](recovery-graphics-evidence.json).

[설정 계층 복원](../docs/exe/cpp-config-reconstruction.md), [설정 기계어 검증 기록](recovery-config-evidence.json), [타입 표·요새 파일 복원](../docs/exe/cpp-fort-reconstruction.md).

[창·화면 장치·입력 큐·영역 배치 복원](../docs/exe/cpp-screen-reconstruction.md) — 플랫폼 결정(Win32), 원본 시작 순서와 옮긴 범위, 원본과 다르게 둔 것.

[후속 복원 계획](../docs/cpp-roadmap.md)에 설정·맵 로딩부터 첫 게임 창·캠페인 플레이·한국어·전체화면·MCP까지의 순서, 단계별 완료 기준과 다음 작업을 정리했다.

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
```

`--run` 은 원본 방식의 창을 띄운다(원본 게임을 실행하지 않는다). `--view` 가 없으면 원본의 "Loading / Please Wait" 화면만 보인다. `--view TEST01` 은 화살표 키로 움직이고 Esc로 닫는다. 이 화면은 Renderer를 옮기기 전까지 쓰는 임시 검사용이며 게임 화면의 재현이 아니다.

CTest의 43개 테스트에는 두 판본 기계어의 4,955개 기대값 검사가 포함되어 있다(공용 1,806개＋VFX 709개＋설정 2,440개). 빌드에는 원본 실행 파일·Ghidra·Python·외부 라이브러리가 필요 없다. 기대값 재생성 방법은 위 복원 문서에 있다. 실제 자산 검사는 원본 게임을 실행하지 않는다. 창·화면 장치·입력 큐·영역 배치는 x86 기대값이 아직 없고, 두 판본의 영역 2,119개 전수 대조와 화면 픽셀 대조(`cpp_window_smoke.py`)로 확인한다.

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
