# dotnetpj 최근 작업 검토와 보류 검사 실행 (2026-10-11)

작업 PC는 `HJOW-Athlon`(Windows 10), SDK는 .NET `10.0.401`이다. 사용자가 이 PC에서 창 실행을 허용하여 클론의 GUI·전체화면 검사를 수행했다. 검토 기준은 `d565346`의 캠페인 연결·진행값 저장·이동 객체 정렬과 2026-10-10에 인계된 화면 변경이다. 원본 게임 실행과 새 디컴파일은 없었다.

## 확인한 진행 범위

- Assets/Core의 설정·타입·포맷·다리·그래프·SID·점유·표시 계산은 cpppj의 기존 x86 기대값을 공유해 검증한다. 객체 단위 그래프·표면 identity의 실제 세션 연결 등은 아직 남아 있다.
- 캠페인 1-1·1-2의 플레이, 영어/한국어 UI, 도움말·옵션·우클릭·건설·생산·이동·전투 화면이 구현되어 있다. 캠페인 메뉴는 원본 스크립트의 장 목록과 선행 완료 조건을 읽고, `CampaignAccess`로 실제 지원 미션을 제한한다.
- 미션 본문의 완료 명령은 `TutorialDialogScript.ConfigHandler → NetstormGame.SaveScriptValue → GameResources/DisplaySettings`로 연결된다. 진행값은 사용자 `settings.json`에 저장된다. 브리핑 조건 평가의 임시 설정 층은 처리기 호출 전에 해제되는 것을 확인했다.
- 5-1 정렬은 **저장된 객체의 표시와 진단용 hover**까지다. 실제 선택의 `PickMobileAt`는 모든 세션 유닛의 그림 중심 거리를 비교하고, `PickEntityAt`는 이동형 유닛 우선/점유 칸 판정을 사용한다. 인계 문서의 “오브젝트 선택이 같은 정렬 목록을 사용”은 이 범위를 넘어서므로 정정했다.

## 검토에서 발견하고 수정한 문제

| 문제 | 확인 근거 | 수정 |
|---|---|---|
| 대소문자만 다른 진행 키가 함께 있는 유효 JSON을 읽으면 시작 예외 발생 | `Load_ScriptKeysDifferingOnlyInCase_LastValueWins`가 `Dictionary` 생성 중 `ArgumentException`으로 실패 | `Normalize`에서 대소문자를 무시하고 입력의 마지막 값을 채택한다. 다른 진행값·해상도와 저장 후 재읽기를 검사한다. |
| 공통 메뉴 스모크가 활성화된 Early Missions를 비활성으로 가정 | 첫 GUI 실행에서 `예상=campaigns, 실제=missions` | 무동작 검사는 구분선을 클릭하고, 미지원 미션 비활성 검사는 장 목록에서 수행한다. |
| 한국어 목록에서 1-1 좌표로 1-2를 선택함 | `04-briefing.png`가 “휘리기그의 지배자”이고 뒤의 1-1 시작 좌표는 무효 | 한국어 행 중심을 1-1 `+38`, 1-2 `+56`, 비활성 1-3 `+74`로 갱신한다. `mission=…`도 단언한다. `recordplay_details`의 같은 옛 좌표도 수정했다. |
| Workshop 검사 준비의 (98,98)이 섬 경계를 벗어남 | `ConstructionOrdered` 대신 `CommandRejected`가 기록되고 메뉴 검사 실패 | 7×8 발자국이 섬 안에 있는 (109,106)을 사용하고 완공 상태를 먼저 검사한다. 오른쪽 가장자리의 하위 창이 왼쪽에 열리는 실제 좌표를 적용했다. 제네레이터(`rainBattery`)와 캐논을 각각 등록하여 생산 커서도 검사한다. |
| 전체화면 검사가 메뉴 상태·최종 설정만으로는 전환 성공을 보장하지 않음 | 기존 스크립트에는 실제 전체화면 상태 단언이 없음 | 자동 검사 세부값에 실제 그래픽 장치의 `fullscreen`·백버퍼 `display`·미션 이름을 추가했다. 일반 UI에는 표시하지 않는다. |

설정 예외 재현 로그는 `extracted/dotnet-review-20261011-settings-before.log`, 최초 GUI 실패는 `…-ui.log`, 잘못 열린 브리핑은 `extracted/screens/dotnet-review-20261011/ui-final/04-briefing.png`, 기존 Workshop 실패는 `…-workshop-before.log`에 남겼다.

## 검증

- Release 솔루션 빌드: **경고 0, 오류 0**. 로그: `extracted/dotnet-review-20261011-build-final.log`.
- 수정 전 전체 테스트: **951개**(Assets 327 + Core 624) 통과. 수정 후: **952개**(Assets 327 + Core 625), 실패·건너뜀 0. 로그: `extracted/dotnet-review-20261011-tests-final.log`.
- 전체화면 GUI: 한국어 메뉴→1-1→메뉴→1-2→메뉴, 음량·1280×720 변경, 전체화면 왕복, 전체화면으로 저장·종료 후 재실행, 창 모드 복귀 후 재실행을 검사했다. 네 실행 모두 통과했고 시작 표식은 해제된 상태로 저장되었다. 실제 전체화면 백버퍼는 1920×1080, 창 복귀는 1280×720이다. 로그: `extracted/dotnet-review-20261011-ui-verified.log`, 캡처: `extracted/screens/dotnet-review-20261011/ui-verified/`.
- 전체 GUI 회귀는 **클론 49회 실행, 상태 단언 626개 모두 통과**했다. 아래 수는 실제 클론 로그의 `UI PASS`만 세며, 스크립트 바깥에서 수행한 설정 저장·타이머 픽셀 비교도 별도로 통과했다. 각 검사 설정은 결과 폴더에 격리했고 클론은 한 번에 하나씩 실행했다.

| 검사 | 클론 실행 수 | 상태 단언 수 |
|---|---:|---:|
| campaign (잠금/해제) | 2 | 8 |
| buttons (팁/메뉴) | 2 | 85 |
| help_options Menu | 1 | 17 |
| help_options Mission | 1 | 14 |
| help_options Workshop | 1 | 18 |
| move | 2 | 22 |
| construct | 2 | 27 |
| production | 2 | 19 |
| recordplay_details | 10 | 112 |
| TEST01 (4:3 영어/한국어·16:9 영어·16:10 한국어) | 4 | 96 |
| TEST02 한국어 | 3 | 54 |
| TEST02 영어 | 3 | 54 |
| combat (6장면 × 영어/한국어) | 12 | 56 |
| ui (미션 왕복·전체화면·재실행) | 4 | 44 |
| **합계** | **49** | **626** |

전체화면을 제외한 13개 검사 모드는 모두 종료 코드 0이다. 결과는 `extracted/dotnet-review-20261011-smokes.json`, 집계는 `…-counts.json`, 검사별 로그는 `…-matrix-<이름>.stdout.log`/`stderr.log`, 실제 클론 로그·PNG는 `extracted/screens/dotnet-review-20261011/matrix/`에 있다. 스모크는 `tools/clone_*_smoke.ps1`에 새 `-OutputDirectory`를 지정하여 재실행할 수 있다. 전체화면은 `clone_ui_smoke.ps1`에 포함된다.

## 남은 범위

- 이 검토 시점에 남았던 5-1 전체 표시 목록·실제 선택 통합은 이후 2026-10-11 작업에서 구현하고 겹침 화소·순서·선택을 직접 검증했다. [후속 구현 기록](dotnet-world-display-20261011.md).
- 캠페인 완료 콜백·저장 함수 연결은 코드/단위 검사로 확인하고, GUI 잠금·해제는 저장된 완료값을 넣어 검사했다. 실제 게임을 끝까지 플레이하여 승리→종료→재실행→다음 미션 해제를 확인하는 전체 경로는 이번 실행 범위에 포함하지 않았다.
- 9-1의 의도적 차이 판정, cpppj 복원을 기다리는 건설·전투·월드 연결 항목, 나머지 창 치수·도움말 표시 차이는 기존 계획대로 남는다. 이번 GUI 검사는 기능 회귀이며 원본과 모든 픽셀이 일치한다는 판정은 아니다.
