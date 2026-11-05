# 5차 사용자 홈 저장 경로 — 설정·체크포인트·표식·캡처·진단 (단위932)

2026-11-05, HJOW-Athlon(마지막 전체 정밀 디컴파일과 같은 PC). 5차(`%USERPROFILE%\Saved Games\Netstorm\` 이관)의 **첫 구현 묶음**이다. 설치 폴더가 읽기 전용이거나 관리자 권한이 필요한 위치여도 게임이 설정을 저장하고 시작할 수 있도록, 자동 생성 파일의 쓰기 위치와 설정 읽기 순서를 바꿨다. 5차 전체는 미완료이며 Editor 작업본/목록, 글꼴 캐시, HTML 도움말 목록, 읽기 전용 설치 통합 검증이 남아 있다(아래).

## 새 구성 요소

- `platform/UserDataRoot`(신규): 루트와 하위 폴더 이름(`Saves`, `CustomMaps`, `Editor`, `Captures`, `Diagnostics`)을 계산한다. 루트는 환경 변수 `NETSTORM_USER_DATA`(검사/휴대 실행용 재정의) → Windows `FOLDERID_SavedGames`(리디렉션 포함) → `%USERPROFILE%\Saved Games` → `%LOCALAPPDATA%` 순서이며, 모두 실패하면 **설치 폴더로 되돌아가지 않고 예외**를 던진다. 경로 계산은 폴더를 만들지 않고 `EnsureDirectory`가 첫 쓰기 직전에만 만든다.
- `ClientOptions::userData`(신규, 선택): 값이 있으면 사용자 홈 정책이다. 비어 있으면 원본처럼 설치 폴더를 쓰는 기존 동작이며 기존 임시 설치 폴더 단위 검사를 유지하는 용도다. 제품 진입점 `RunClient`는 항상 값을 채운다.

## 바꾼 위치

| 파일 | 변경 |
| --- | --- |
| 설정 `options.cfg` | **쓰기:** `Saves\options.cfg`. 원문 이름 고리/지역 복사는 그대로 소비한 뒤 실제 쓰기 위치만 바꾼다. 1224 교체 저장도 같은 폴더의 임시 폴더를 쓴다. **읽기:** `ConfigInterface::SetUserFileOverride`로 사용자 파일을 먼저 읽고, 없을 때만 설치의 옛 `d\options.cfg`를 읽는다(기존 설정 이관). 설치 파일은 수정하지 않는다. |
| 전투 체크포인트 `!state.dat` | `SavedStateFilePath`/`DeleteSavedStateFile`이 `Saves\!state.dat`를 사용한다. 원문 이름 고리 소비는 같다. 사용자 파일이 없으면 원문처럼 -1이며 설치의 옛 파일은 지우지 않는다. |
| 전체화면 표식 | `ClearFullScreenState`가 `Saves` 안의 표식만 정리한다. 호환판은 이 표식을 만들지 않는다. |
| GIF 스크린샷 | 상대 이름은 `Captures`에 저장하고, 기존 번호 파일 확인도 `Captures`를 본다. 사용자가 명시한 절대 경로는 그대로 쓴다. |
| 연속 BMP | `Captures`에 저장한다. 장치/절대 이름은 명시 위치를 유지한다. |
| `squids.txt`, `procSumm.txt` | `Diagnostics`에 저장한다. `procSumm.txt`가 현재 작업 폴더에 직접 생기던 경로도 포함한다. |

원본과 다른 호환 처리이며, 이유는 코드 주석에 한국어로 적었다.

## 검증

- 새 집중 검사 `netstorm_user_data_root_tests` 5개: 경로/지연 생성, 환경 재정의/기본값, 체크포인트 위치(이름 고리 소비 동일·설치 파일 보존), 설정 쓰기+읽기(설치 폴더 항목 수 불변), 이관/우선순위(사용자 파일이 설치 파일을 이기고 설치 파일은 바이트 불변).
- **최초 실행에서 2개가 실패했다.** 설정 읽기 두 단언이었다. 원인은 편집 도구가 `\\`를 `\`로 줄여 `find_last_of("\\/")`가 `"\/"`로 들어간 것이었고, 컴파일러 경고 C4129로도 확인했다. 기대값을 바꾸지 않고 소스를 고쳐 5/5가 통과했다. 최초 실패 로그는 `extracted/cpp-resume-932/test-first-failure.log`다.
- 빌드 경고/오류 0(수정 후 증분 45초). 제품 `Netstorm.exe` 10,803,200바이트, SHA-256 `793dfb55e91af8303704d5081b12f4a3ea97b3cbe8de4269f71cce6df68c457a`.
- 실제 제품을 `--run originals --frames 120 --no-audio --window`로 한 번 실행(HJOW-Athlon은 실행 확인 면제)하고 `NETSTORM_USER_DATA`를 임시 폴더로 줬다. 종료코드 0, 사용자 폴더에는 `Saves\options.cfg`(1021바이트)만 생겼고 `originals` 전체의 파일 수·크기·수정시각 지문은 실행 전후 동일했다.

## 남은 5차 범위

1. Editor 작업본/장부/잠금을 사용자 홈으로 이관하고 새 맵을 `CustomMaps`에 저장한다. 현재 작업 루트는 설치 폴더의 형제 `-editor`다. 설계 메모: 기존 장부 방식은 첫 목록 때 원본 `d`의 파일을 작업 루트로 복사하므로 작업 루트가 `CustomMaps`이면 `CustomMaps\d\...` 구조가 된다. 평평한 `CustomMaps\NAME.fort` 배치는 키 매핑 변경이 필요하다.
2. Edit 목록의 50개 상한 제거와 스크롤(원문 포인터 50칸 구조 변경).
3. 글꼴 캐시 `d\!*.chfnt` 쓰기(현재 쓰기 불가 설치에서는 저장만 건너뛴다), `NetStormPieceList.html`, MCP 생성물의 사용자 경로.
4. 읽기 전용 설치 사본에서의 실제 쓰기 거부 권한 실행 검증과 한글 사용자 이름 경로 검증.

실제 게임 실행은 위의 120프레임 한 번이며 사제 포획/희생·특정 적 공격·승리 재현, 신규 MP/통신, 전체 검사/34미션/ZIP은 실행하지 않았다.

## 단위933 추가 — Editor 작업본/장부, 글꼴 캐시, 도움말 목록

- **Editor:** `ResolveEditorRoots`가 작업 루트·장부 루트·보호 루트를 한 곳에서 계산한다. 사용자 홈 정책에서는 작업 루트가 `CustomMaps`, 장부(`catalog-v1.dat`)/잠금(`catalog.lock`)이 `Editor`이고, 정책이 꺼지면 기존의 형제 `<이름>-editor`/`-state` 배치다. 보호 루트(설치 폴더, 개발 저장소의 `originals`/`originalCD`)는 두 경우 모두 같다. `EditorCatalogSourceDevices::metadataRoot`와 `EditorFiles::ExplicitNamespace`가 명시 장부 폴더를 받으며, 작업 루트나 보호 원본과 겹치면 거부한다. 리디렉션된 Saved Games는 실제 경로로 정규화해 쓴다. 기존 장부 이관 정책(첫 목록 때 원본 `d` 파일을 작업 루트로 한 번 복사, 삭제 보존)은 그대로다.
- **설계 결정(사용자 확인 필요):** 작업 루트가 `CustomMaps`이므로 파일은 `CustomMaps\d\NAME.fort` 구조가 된다(게임의 `d` 폴더 배치를 그대로 유지). 평평한 `CustomMaps\NAME.fort` 배치는 장부 키 매핑을 바꿔야 해서 하지 않았다. 또 장부 방식은 첫 Edit 목록에서 원본 `d`의 `*.fort`/`*.english`를 사용자 작업 루트로 복사한다(원본 `d` 기준 fort 433개).
- **글꼴 캐시:** `FontStore::SetUserCacheDirectory`가 `Cache`를 연결한다. 읽기는 사용자 캐시 → 설치의 기존 `d\!*.chfnt`(읽기 전용) 순서이고, 새로 만든 캐시는 `Cache`에만 쓴다. 쓰기 실패는 게임 시작을 막지 않는다. 사용자 캐시 장치는 원문 경로 고리와 분리해 원문 `Format` 호출 횟수를 바꾸지 않는다.
- **도움말 목록 `NetStormPieceList.html`:** 사용자 홈 정책에서는 `Cache`에 만든다. 같은 이름 고리를 공유하는 BaseFileSystem으로 원문 열기 인자(1,1)와 경로 칸 소비를 유지한다.
- **검사:** `netstorm_user_data_root_tests` 9개 통과(Editor 루트 2개·글꼴 캐시 1개·도움말 목록 1개 추가, 첫 실행부터 통과). 회귀로 `netstorm_editor_file_tests` 45/0 통과. 빌드 경고/오류 0.
- **미확인 실패(기준선 비교 안 함):** `netstorm_editor_default_name_tests` 85개 중 2개(`CampaignSaveGoMain_ActualManagerGateAndSelectedBattleStop`, `FortObjectLoad_ActualFactoryOwnerHpPopAndContent`)와 `netstorm_file_tests` 628개 중 5개(`CampaignMotion_ActualWarBeginsPriestPathArrival`, `CampaignMotion_ActualMasterOfWhirligigsLocalPriestPath`, `CampaignMotion_ActualMasterOfWhirligigsInitialization`, `RawSurfaceWorld_RuntimeSharesStorageAndCurrentDispatch`, `rawProcessRemoval_native_mixed_form_filter`)가 실패한다. 메시지는 "실제 지형 생성 상위의 현재 전역/장치 미연결", "현재 Squid Pop의 실제 전송 미연결" 같은 test 장치 미연결 logic_error와 `004bf880` 선택 경계 기대 불일치다. 사용자 정책이 꺼진 경로(`userData` 없음)는 이전과 같은 값을 계산하므로 이번 변경과 무관하다고 판단하지만 변경 전 빌드와 직접 비교하지는 않았다. 다음 단위에서 기준선과 비교하거나 해당 검사를 정리한다.
- **제품:** `Netstorm.exe` 10,808,832바이트, SHA-256 `E5DEB06B637E8725BA761FDC3E9814CA02A1893A016BA6DD26D84F4FB3A79179`. `--run originals --frames 120 --no-audio --window`를 다시 실행했고 종료코드 0, 사용자 폴더에는 `Saves\options.cfg`만 생겼으며 `originals` 지문(`4a10a1e1…`)은 변하지 않았다. 실제 Edit 화면 진입/저장은 GUI로 확인하지 않았다(다음 단위의 읽기 전용 설치 통합 검증에서 다룬다).
- **남은 5차:** Edit 목록의 50개 상한 제거·스크롤, MCP 산출물(4차에서 생성되는 파일), 읽기 전용 설치 사본의 실제 쓰기 거부 권한 실행 검증, 한글 사용자 이름 경로 검증.
