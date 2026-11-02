# cpppj 게임·테스트·전체 빌드 선택 (단위739, 2026-11-02)

사용자가738 다음 우선 작업으로 요청한 컴파일 분리를 구현했다. 게임 소스는738 그대로이며 실행 파일의 SHA-256은 `56d190464315a86f43bfc2418c769386f19321550b330c097971dd7a13ce0340`, 표시 버전은11.01이다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이며 이번 작업에는 새 디컴파일이나 원본 실행이 필요하지 않았다.

## 선택과 범위

`cpppj/build-cpppj.ps1`을 추가했다. 모드 생략 시 번호/이름 선택 메뉴, 빈 입력 시 취소, 잘못된 입력 시 재선택을 제공한다. Windows PowerShell5.1에서 한국어 메뉴를 읽도록 UTF-8 BOM으로 저장했다.

| 선택 | CMake 구성 | 빌드 대상 |
| --- | --- | --- |
| `-Mode Game` | `NETSTORM_BUILD_TESTS=OFF` | `NetstormCpp`와 필요한 엔진/디코더 |
| `-Mode Tests` | `NETSTORM_BUILD_TESTS=ON` | 통합 `netstorm_tests`와 필요한 엔진/도우미 |
| `-Mode All` | `NETSTORM_BUILD_TESTS=ON` | 대상 생략. 기존 게임·통합/집중 테스트·도우미 전체 |

Tests는 별도 집중 실행 파일마다 같은 테스트 소스를 중복 컴파일하는 것을 피한다. `-TestTarget netstorm_editor_file_ring_tests`처럼 필요한 집중 대상만 지정할 수도 있다. 공용 엔진의 컴파일은 테스트에도 필요하다. 기본 Release/병렬4, 다른 구성/빌드 폴더/생성기/CMake 경로 지정과 `-DryRun`, `-ConfigureOnly`를 지원한다. 상대 빌드 폴더는 저장소 루트 기준이며 네이티브 인자는 배열로 전달한다. 실패 코드를 전달하고 구성 실패 뒤 빌드를 시작하지 않는다. 스크립트는 게임이나 테스트를 실행하지 않는다.

새 CMake 빌드 폴더의 테스트 기본값은 OFF로 바꿨다. CMake는 기존 캐시를 보존하므로 기존 ON 캐시에서 게임 전용으로 전환하려면 OFF가 필요하며 Game 모드가 이를 명시한다. 기존 `vs2022`/`ninja` 프리셋과 CI는 ON을 명시해 전체 빌드와 CTest 동작을 유지한다. 루트 `README.md`, `README.en.md`, `docs/cpp-build.md`, `cpppj/README.md`에 선택/직접 명령/별도 테스트 실행을 반영했다. 배포 스크립트의 기존 게임 대상 빌드 동작은 유지한다.

## 확인 결과와 한계

- PowerShell 구문 검사와 Windows PowerShell5.1의 선택/명령행/취소/재선택/오류/집중 대상/드라이런15개를 통과했다. PowerShell7에서도 통합 테스트 명령 계획을 확인했다.
- 실제 Game 모드로 기존 빌드 폴더를 OFF로 전환해 증분 빌드했다(종료0,6.112초). File API의 Release 그래프는 전체5대상이고 테스트 대상/소스0이다. EXE 해시는738과 같다.
- 새 폴더에 옵션을 주지 않은 CMake 구성에서도 OFF를 확인했다. 스크립트로 새 `한글 빌드` 경로를 구성해 공백/Unicode/기본 x64 분기도 확인했다. Visual Studio 구성에서는 전달한 `CMAKE_BUILD_TYPE`이 사용되지 않는다는 안내가 있고 실제 구성은 `--config`로 선택한다.
- 실제 Tests 모드의 집중 대상 `netstorm_editor_file_ring_tests` 증분 빌드를 확인했다(종료0,16.160초). 통합 대상의 File API 의존성은 `netstorm_tests`, `netstorm_help_probe`, `netstorm`, `NetstormSmacker`, `ZERO_CHECK`의5개이며 게임 실행 파일과 다른 집중 대상은 제외된다.
- All은 실제 구성까지 수행했다. Release 전체71대상 중 테스트 실행 파일66개와 게임이 `ALL_BUILD`에 모두 포함된다. 드라이런의 전체 명령은 `--target`을 생략한다. 기존 전체 빌드 기능을 보존한 구성 확인이며 **전체 재컴파일이나 통합 CTest 실행은 하지 않았다.** 작업 후 주 빌드 캐시는 ON이다.
- 잘못된 생성기로 CMake 자체 실패를 유도해 종료1 전달과 빌드 미호출을 확인했다. 보호2652개는 모두 불변이며 새 fixture/게임 실행/통신/GUI/원본 관측0이다.

시간은 변경된 C++ 소스가 없는 증분 실행의 관측값이다. 기존 전체 컴파일과의 성능 비교나 새 폴더에서의 전체 컴파일 시간으로 해석하지 않는다. 게임 제품의 최신 집중 검증은738의51개/실패0, 전체34는645의644 제품이며 이번에 재실행하지 않았다. 전체1~8차 목표는 진행 중이다.

요약 보고서는 `cpppj/build-modes-20261102.json`, 원시 로그/생성 그래프는 Git 제외 `extracted/cpp-build-modes-739/`에 있다.
