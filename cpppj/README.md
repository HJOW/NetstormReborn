# cpppj — NetStorm C++ 빌드

기존 게임(NetStorm: Islands at War)을 디컴파일한 소스를 토대로 C++ 소스를 다시 만드는 프로젝트다.
C# + MonoGame 빌드(`../dotnetpj/`)와 함께 개발한다.

**공용 계층과 타입·그래픽 자산 로더가 빌드된다.** TAFF·설정·번역·타입·프레임 검색·프로세스 커널·게임 시계, SHP 압축 해제·기본 8비트 합성·팔레트를 복원했다. 원본/CD 자산 전체를 검사하고 프레임을 투명 BMP로 내보낼 수 있다. 게임 창·전투·전체 루프·MCP는 후속 작업이다.

[복원 근거·함수 대응·검증 범위](../docs/exe/cpp-reconstruction.md), [검토 목록](recovery-manifest.json), [기계어 검증 기록](recovery-evidence.json).

[타입·그래픽 복원과 판본 차이](../docs/exe/cpp-assets-reconstruction.md), [VFX 기계어 검증 기록](recovery-graphics-evidence.json).

[후속 복원 계획](../docs/cpp-roadmap.md)에 설정·맵 로딩부터 첫 게임 창·캠페인 플레이·한국어·전체화면·MCP까지의 순서, 단계별 완료 기준과 다음 작업을 정리했다.

## 빌드와 테스트

CMake 3.21 이상과 C++20 컴파일러(Visual Studio 2022 이상, GCC 11 이상, Clang 14 이상)가 필요하다. 저장소 루트에서 실행한다.

```sh
cmake -S cpppj -B cpppj/build -DCMAKE_BUILD_TYPE=Release
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build --build-config Release --output-on-failure
```

실행 파일은 Windows 에서 `cpppj/build/bin/Release/NetstormCpp.exe`, Linux 에서 `cpppj/build/bin/NetstormCpp` 에 생긴다.

```powershell
cpppj/build/bin/Release/NetstormCpp.exe --inspect-data originals
cpppj/build/bin/Release/NetstormCpp.exe --inspect-data originalCD
python tools/cpp_recovery_smoke.py
cpppj/build/bin/Release/NetstormCpp.exe --inspect-assets originals
cpppj/build/bin/Release/NetstormCpp.exe --inspect-assets originalCD --cd
python tools/cpp_assets_smoke.py
cpppj/build/bin/Release/NetstormCpp.exe --export-frame originals sunCannon N00 0 extracted/sunCannon.bmp
```

CTest의 17개 테스트에는 두 판본 기계어의 2,515개 기대값 검사가 포함되어 있다(기존 1,806개＋VFX 709개). 빌드에는 원본 실행 파일·Ghidra·Python·외부 라이브러리가 필요 없다. 기대값 재생성 방법은 위 복원 문서에 있다. 실제 자산 검사는 원본 게임을 실행하지 않는다. C++ 제품 지원 대상은 Windows이며 Linux 빌드는 공용 코드의 이식성 검사로 사용한다.

## 폴더

| 폴더 | 내용 |
|---|---|
| `src/o/` | 원본 `\Ns\O\` 의 공용 모듈 (게임 규칙·데이터·프로세스 커널) |
| `src/client/` | 원본 클라이언트 모듈 (화면·입력·소리·메인 루프) |
| `src/zacket/` | 원본 `\Ns\Zacket\` 의 네트워크 패킷 |
| `src/platform/` | 새로 쓰는 코드 — 원본의 Win32·DirectX 호출을 대신하는 계층 |
| `src/app/` | 새로 쓰는 코드 — 실행 파일 진입점 |
| `tests/` | 단위 테스트 (`TestSupport.h` 의 `TEST_CASE`·`CHECK`) |
| `cmake/` | 공통 컴파일 옵션 |

원본 소스 파일이 어느 경로로 가는지는 [SOURCE_MAP.md](SOURCE_MAP.md)에 있다(`python tools/cpp_source_map.py` 로 생성).

## 소스를 옮기는 방법과 규칙

[docs/cpp-build.md](../docs/cpp-build.md)에 정리했다: 디컴파일 결과에서 C++ 소스를 만드는 절차, 출처 주석 규칙, 32비트 코드를 64비트로 옮길 때의 주의, 아직 정하지 않은 것.
