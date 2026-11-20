# 미션 Client 서비스 보강과 캠페인 장시간 진행 확인 (WB04-E1122)

캠페인 16개 미션을 미션마다 길게(최대 180초) 진행하는 조사에서, AI 가 건설·공격을 시작한 뒤에야 나오는 **미연결 서비스·좌표 오류 네 가지**를 찾아 고쳤다. 앞선 시작 확인([모든 미션 시작](webpj-all-missions-start-20261120.md))은 10초 안팎이라 이 경로에 닿지 않았다. 각 오류는 프레임이 `실행 중 오류가 발생했습니다` 로 멈추는 치명 오류였다.

## 찾은 오류와 수정

| 미션(시점) | 오류 | 원인 | 수정 |
| --- | --- | --- | --- |
| ThunderingPower(약 2분) | `실제 세션 하위 소비자 미연결: RawCampaignSession.cpp:1213` | 사제 보호막 서비스에 안내음(`noticeSound`)·번역(`translate`)이 없었다(Tell 만 있음) | `services.priest.shield` 에 `makePriestShieldNoticeHooks` 의 이름 기반 전역 재생(우선 1)과 키 그대로의 번역을 연결 |
| DissolvedAlliance(약 1~2분) | `RangeError: 장애물 검색 정수 좌표 범위 오류` | AI 배치 평가가 넘기는 `{ x, y }` 점을 후보 수집이 `[x, y]` 로 읽어 좌표가 NaN 이 됨(원본은 `{point.x, point.y}` 변환) | `raw-campaign-session-motion-runtime.js` 의 `blockers` 훅에서 `[point.x, point.y]` 로 변환 |
| RunForIt(약 2분) | `Rain 피격의 실제 CurrentFlyerProcess 설치 전 호출` | 맨 오 워 비행이 맞을 때 쓰는 `o.currentFlyerProcess` 가 설치되지 않음(`broadcastFlyerCommand` 는 있었음) | `raw-campaign-session-flyers.js` 에서 부모에 붙은 `FlyerProcess` 의 필드를 돌려주는 `currentFlyerProcess` 설치(없으면 `processSid` 진단 뒤 같은 0 슬롯 조회, 원본과 같음) |
| BreakingThrough(약 3분) | `포획 안내 실제 Player 이름 미연결` | `services.priestTakenName` 이 없었다 | owner 만 채운 식별 기록을 현재 Player 표·채팅 기록으로 풀어 이름(못 찾으면 `SOMEONE`)을 돌려주도록 연결 |

## 같은 단위에서 함께 연결한 원본 서비스

원본 `CampaignClientServices` 와 웹 서비스를 대조해 같은 종류의 빈칸을 한꺼번에 메웠다(모두 해당 사건에서 `미연결` 오류를 낼 수 있었다).

- `services.unitLost`: 유닛 상실음 `unitLost.wav`(전역 재생, 우선 1).
- `services.priest.fallSound`: 사제 낙하음 `priestFall.wav`.
- `services.hit.general.warningEvent`: 섬 피격 경고가 종류 12 안내를 지움(음향·Tell 은 세션이 공급).
- `services.carrierDropPopup`: Cast/Drop 비용을 실제 생산 UI 의 떠다니는 SP 표시에 그림.
- `services.assets.factory.warnDuplicate`: 중복 워크샾 경고를 채널 4 진단 로그에 원본 문구로 기록.
- `services.newTechnology`: 기술 획득의 이름(현재 TYPE 설명 번역)·설정 저장·그림 서식(`%d`)·`NewTech` 안내.

## 시험

- `tests/netstorm-gameplay-services.test.mjs`: 보호막 안내음·번역, 유닛 상실음, 낙하음, 섬 피격 경고 사건(생산 UI 없을 때 포함), Cast/Drop 비용 표시, 중복 워크샾 진단 문구, 기술 획득 이름·설정·서식·안내, 포획 이름(전투/비전투·로컬·SOMEONE)을 확인한다.
- `tests/raw-campaign-session-ritual.test.mjs`: 장애물 후보 수집에 `[x, y]` 가 넘어감(변환을 지우면 실패함을 확인).
- `tests/raw-campaign-session-flyers.test.mjs`: `currentFlyerProcess` 가 설치되고 붙은 프로세스의 필드를 돌려주며 범위 밖 부모를 거부함.
- 새 `e2e/campaign-long-run.spec.js`(Chromium): 환경 변수 `NETSTORM_LONG_SECONDS` 를 정한 실행에서만 돌며 캠페인 31개(초급 6·세 묶음 16·훈련 9)를 미션마다 그 시간(초)만큼 진행하고, 시계를 멈추는 쪽지·안내 창은 계속 닫으며, 페이지 오류 0 과 시계 진행을 확인한다.
- 조사 결과: 수정 뒤 캠페인 16개 모두 150~180초 진행에서 오류가 없었다(수정 전 4개 미션이 위 오류로 멈춤). 이어서 Early Missions 6개와 Priest Training 9개도 미션마다 100초씩 진행해 오류 0, 시계 정상 진행을 확인했다(E1123, 35.6분). 이로써 캠페인 목록의 31개 미션이 모두 100초 이상 오류 없이 도는 것이 확인됐고, 시험(`campaign-long-run`)도 31개 전체로 넓혔다.

## 남은 경계

- 이 조사도 사람이 조작하지 않는 상태(AI 만 움직이는 구간)다. 플레이어의 방어·공격과 승패 처리, 미션 목표 수행은 사용자 확인 범위다.
- `services.refreshSp`(희생 의식의 외부 SP 정규화)·`carrierContent`·`chatReceive`·`removeConnectionStatus` 등 멀티플레이·편집과 관련된 서비스는 이 조사에서 쓰이지 않아 연결하지 않았다. 해당 경로를 쓰는 미션이 드러나면 같은 방식으로 잇는다.
- 스커미시의 난이도·AI 수별 장시간 진행(E1106~E1107에서 어려움 AI 1명 5분)과 데모 목록(Demo 메뉴)의 장시간 진행은 아직 이 방식으로 확인하지 않았다.

## E1124 추가: 스커미시 장시간 진행과 섬 목록 진단

스커미시를 AI 수·난이도·동맹을 바꿔 길게 진행하는 같은 조사에서, **AI 7명(쉬움·보통)** 일 때 30초쯤 뒤 `실제 세션 하위 소비자 미연결: RawCampaignSession.cpp:1189` 로 프레임이 멈추는 오류를 찾았다.

- 원인: 세션이 섬 런타임(`RawIslandRuntime`)을 만들 때 진단 훅에 `missing(1189)`(호출하면 오류) 를 넘겼다. 원본의 실제 Client 경로(`ClientConstructionGlobals::Islands`)는 `assertionAt = (식, 줄) => storage.AssertDisplay(식, "\Ns\O\Islandlist.cpp", 줄)` 로 진단을 표시하고 계속한다.
- 수정: `raw-campaign-session-priests.js` 에서 같은 `assertionAt` 을 연결했다. 기존 시험(`o.islands.owner(99)` 가 1189 오류를 던짐)은 새 동작(진단 표시 뒤 계속)에 맞게 고쳤고, 훅을 예전 모양으로 되돌리면 실패함을 확인했다.
- 이제 7명 AI 스커미시에서 시작 섬이 지도 가장자리(256)에 닿으면 `원본 표시 진단: \Ns\O\Islandlist.cpp:676 bcr.isValid()` 가 콘솔에 한 번 나온다. 시작 섬 경계(예: 남쪽 섬 112,208~160,256)가 월드 범위(< 256) 밖이라 생기는 원본 `BoardCoordRect::isValid` 진단이며 게임은 계속된다(원본도 같은 진단을 표시한다고 보고 그대로 둠, 원본 직접 대조는 하지 못했다).
- 새 `e2e/skirmish-long-run.spec.js`(Chromium): 환경 변수 `NETSTORM_LONG_SECONDS` 를 정한 실행에서만 돌며 AI 7명 쉬움+동맹 2·7명 어려움+동맹 4·6명 보통+동맹 3·7명 어려움 동맹 없음의 네 조합을 진행하고 오류 0, 시계 진행, 객체 1000개 초과(AI 의 월드 성장)를 확인한다. 위 진단은 오류로 세지 않는다.
- 조사 결과: 수정 뒤 네 조합 모두 300초 진행에서 오류가 없었다(시계 372~395초, 객체 1만~1만 2천 개). 수정 전에는 7명 쉬움·보통 조합이 30초쯤에 멈췄다.
