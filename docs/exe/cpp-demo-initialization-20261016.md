# cpppj 데모 로드·시작의 초기화 복원 — 2026-10-16

후속 단위80에서 [맨 오 워 풀 후처리/프로세스](cpp-rain-aviary-20261016.md)를 연결해 데모3개의 Load/Start가 모두 통과했다. 아래 표/수치는 단위77 당시의 실패와 수정 경계 기록이다. 자연 재생·GUI·완주 검증은 계속 남아 있다.

현재 PC와 마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-16**이다. 기준 커밋은 `6b4ffe6`이며 원본 10.78을 대상으로 한다. 게임/GUI 실행과 보호 원본 수정은 없다.

## 실제 데모 세 개의 검사

원본 아카이브 `tell.english`의 `[DEMOVILLE]`은 다음 세 메뉴를 `StartDemo`로 실행한다.

후속 조사에서 데모2는 캠페인2-4, 데모3은 캠페인1-6과 같은 미션 자원을 쓰는 것을 확인했다. 로컬AI/데모 수명 차이와 후속 재사용 항목은 [대응 문서](../gameplay/demo-campaign-map.md)에 기록한다.

| 표시 제목 | 미션 키 | 초기 검사 | 복원 뒤 결과 |
| --- | --- | --- | --- |
| The Storm Rages | DemoStormRages | TYPE143 맨 오 워 풀의 PostPop 미지원 | 같은 후처리 미지원으로 Load 실패 |
| Demo of "Guard My Back" | GuardMyBack | Graph 재구성에서 번호 소진 | Load 통과, Start의 AI 색 변경0043ca70 미연결 |
| Demo of "Dissolved Alliance" | DissolvedAlliance | 총 프레임→방향 미연결 | Load 및 Start 통과 |

`CampaignDemoStart_` 세 검사는 각각 실제 원본 Script/Fort를 읽어 같은 `RawCampaignSession`과 기존 `RawCampaignMissionStartBindings`를 호출한다. 원본 명령004d7ffb는 미션 생성00463b70 뒤00463a00(1)로 현재 미션+68에 데모 플래그를 쓴다. 검사도 세션 생성 뒤 공유 `gate.demo=1`을 공급한다. 원본 스크립트의 `demo` 키로 이 동작을 대신하지 않는다.

시작을 통과한 미션은 로컬 사제 포함 모든 실제 참여자의 AI, 공유 데모 플래그/시작 시각/360초 종료 시각, 후처리 깊이 복귀를 확인한다. `DissolvedAlliance` 한 개의 헤드리스 시작 성공이며 데모 자연 재생·전투·승패나 GUI 메뉴 실행을 확인한 결과는 아니다.

## 총 프레임과 방향

`RawGunAngle.h`는 실제00467610의 signed 프레임/5와 상한19, DAT00540940의20개 float 각도,45/135/225/315 경계를 옮긴다. Session의 기존 파생 PostPop에서 이를 호출하고 실제004ad3e0의 정상 windArcher/0~3 경로처럼 Graph BYTE+30을 쓴다. 음수-4~-1은0으로 절삭되어 유효하고, -5 이하의 원본 무효 메모리 경로는 호스트 예외로 드러낸다.

`tools/decomp_gun_angle.py`는 원본 명령/실제 float 표를 사용해 **518개 고유 입력**(-4~512, INT_MAX)을 관찰한다. 두 x87 정밀도 재실행은 동일하며 입력 수를 중복 합산하지 않는다. 외부 함수 대체 없이 반환·스택·비휘발 레지스터와 스택 밖 쓰기를 검사한다. `GunAngle_` 두 C++ 검사가 독립 원본 fixture 및 무효 입력 처리를 확인했다.

## Graph0 예약

두 큰 데모의 실패 직전 값은 `Rebuild(false)`, 사용 중 번호0개, 표면2791/2737개였다. 전체 풀 복구 정책만 켠 시도는 실패를 해결하지 못해 제거했다. 임시 진단 출력도 제거했다.

원본 월드 초기화004b2df0은00462b20을 호출한다. 이 함수는 앞251행의 surfaces/inUse 두 WORD를0으로 지우고 **0번 inUse=1·reserved LOW BYTE=1**을 기록한다.254행의 두 WORD는0x7dfd이며 나머지 reserved 및251~253행은 보존한다. 이를 `RawGraph::ResetTable()`로 옮기고 `RawSurfaceWorld`의 첫 객체 설치 전에 호출했다. 저수준 생성자에 공급한 기존 snapshot의 의미는 유지한다.

예약이 없으면 기본 Graph0인 내부 표면과 첫 임시 번호0이 충돌한다. 초기화를 복원한 뒤 두 큰 데모 모두 같은 재구성 경계를 통과했다. 우회 번호 정책이나 더 큰 원본과 다른 표를 추가하지 않았다.

`tools/decomp_graph_reset.py`는 외부 대체 없이 원본00462b20의 전체1530BYTE 출력을 서로 다른16입력으로 관찰한다. 함수 범위·쓰기 영역·정상 반환·스택·비휘발을 확인한다. `GraphReset_Native16AndUnrelatedStorage`는255행 전체와 별도 풀/스택 비변경을 확인했다. 전체 재구성 자체의 새16입력 관찰로 세지 않는다.

## 검증과 다음 경계

Release 빌드는 경고0·오류0이다. 전체 CTest는 **1,333개 중1,331개 통과·2개 실패, exit8**, 검사115.50초·전체115.57초다. 실패는 위 두 새 데모의 미복원 경계다. 기존8개 미션 시작 검사는 통과했다. 알려진 실패를 숨기거나 검사에서 제외하지 않는다.

새534개 고유 입력의 SHA와 기존 Graph 재구성288회·훨리베이스1,734개·썬 바리케이드330/1,904/99개·매립 후처리580개·요구에너지2,352개의 보존 SHA를 확인했다. 기존 입력을 새 원본 관찰로 합산하지 않는다. 새 도구의 Python 구문, 새 파일 UTF-8, 공백 검사도 통과했다. CMake 등록 변경의 이전/현재 SHA는 요구에너지 보고서의 이력에 남긴다.

집중 Graph 검사1개와 총 각도 검사2개가 통과했다. 로그는 `extracted/goal-demo-20261016-{graph-reset-build,graph-reset-focused,final-build,reset-focused,angle-focused,ctest,tests}.log`이며 세부 제품 검증은 `cpppj/{gun-angle,graph-reset}.json`이다.

다음은 맨 오 워 풀(TYPE143·rainAviary)의 실제 후처리/프로세스, 데빌 메이커(TYPE139·windAviary), AI 색 변경0043ca70이다. GUI는 아직 `StartDemo` 명령을 지원하지 않고 일반 브리핑 종료도 복원한 Start와 연결되지 않았다. 이후 같은 장기 미션/Guide/프레임 상태 연결과 자연 완주를 확인해야 한다. Early Missions·캠페인1-1~1-2·데모3개 전체 목표는 미달성이다. DirectDraw 전체화면 문제는 수정하지 않았다.
