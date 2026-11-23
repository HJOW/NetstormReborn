# webpj State 설치의 Game 난수 소비를 cpppj 와 맞춤 (WB04-E1280, 2026-11-23)

고정 시각 모드(`?fixed=25`)의 Tutorial1 90프레임에서 웹의 Game 난수열을 cpppj 오라클과 **한 번도 어긋남 없이** 맞췄다. 그 결과 웹 논리 화면은 오라클 `load.png` 와 **다른 픽셀이 0개**(E1278: 7,195개)가 되고, Session 덤프의 `random` 행을 포함한 월드·Graph·장부 행이 모두 기준선과 같아졌다.

## 방법: cpppj 난수 호출 추적

기준선과 같은 소스의 HEAD(덤프가 `sp` 소유자 0 난수 한 행만 다르게 나옴을 확인)에 **임시로** `o::GameRandom::Next`/`SetState` 추적(호출 번호·직전 상태·상한·호출 스택 RVA)과 링크 맵(`/MAP`)을 넣어 Tutorial1 90프레임을 돌렸다(HJOW-Athlon, 25ms 고정, 소리 끔). RVA 를 맵으로 함수 이름에 풀어 구간별로 묶었다. 임시 패치(`cpppj/src/o/SpStore.cpp`)와 링커 플래그는 작업 뒤 원복했고 저장소에는 넣지 않았다. 웹 쪽은 탐침 시험에서 `GameRandom.next` 를 감싸 같은 모양(상태·상한·JS 호출 스택)으로 기록해 첫 불일치 지점을 찾았다.

## cpppj 의 Game 난수 소비 순서 (주 연쇄)

| 호출 번호 | 횟수 | cpppj 호출자 | 웹 |
| --- | --- | --- | --- |
| 0~32 | 33 | `ScrambledSpStore::Initialize`(재시드 + offset + 32회 교환) ← `Client::Run` 시작 | 이미 같음(시계 원점의 Unix 초로 재시드; 시작 직후 상태가 cpppj 33번째 호출 직전 상태 `0x8cc543fc` 와 같음) |
| 33~1312 | 1,280 | `CleanupStateWorld` ← `CleanupNativeStateWorld` ← **`InstallIntroNativeState`(State 39)** | **새:** `_installIntroStateEffects`(시작 입력 직후) |
| 1313~2592 | 1,280 | `CleanupNativeStateWorld` ← `PrepareNativeFortState` ← **`InstallMainMenuNativeState`(State 3)** | **새:** `_installMainMenuStateEffects` 의 정리 |
| 2593~3393 | 801 | `BuildKencloud` ← `TransitionFortPalette` ← `ChangeFortPalette` ← State 3 의 `ApplyExistingEditorAfterCheckpoint` | **새:** 같은 함수의 팔레트 전환(battlePal) |
| 3394~3396 | 3 | `UpdateAmbientWind` 3프레임(소리 기한이 벽시각+10초라 주변음 난수 없음) | **새:** 같은 함수의 주변음 기한 재설정 |
| 3397~4676 | 1,280 | `CleanupNativeStateWorld` ← **`LoadMissionWorldBody`(State7)** | **새:** `_runCampaignState7` 본문의 정리 |
| 4677 | 1 | `SceneMusic::Start` | 이미 같음 |
| 4678 | 1 | 바람 | 이미 같음 |
| 4679~4887 | 209 | `RawTerrainStamp`(지형 도장) | 이미 같음 |
| 4888~5191 | 304 | 농장 군집(**별도 난수 인스턴스**, 시드 12705 — 주 연쇄에는 들어가지 않음) | 이미 같음 |
| 5192~5641 | 450 | `RawRegionPaint` 채우기·경계 | 이미 같음 |
| 5642 | 1 | Fort 객체 무작위 프레임 | 이미 같음 |
| 5643~6922 | 1,280 | SP 수신(`ReceiveFixedLoginPacket`) | 이미 같음 |
| 6923 | 160 | 시작 SP(`RawMissionStart`) | 이미 같음 |
| 이후 | 프레임당 1 + 드문 추가 | 주변 바람·Residence·Priest 재생 등 | 이미 같음 |

State 정리(`CleanupStateWorld`)는 플레이어 1~8 반복에서 SP 를 0 으로 쓴다(`resetOtherPlayer`/`setSp`). SP 저장소 Set 은 소유자마다 난수 160회(32비트 × 5)를 쓰므로 한 번에 1,280회다. 이 소비가 웹에 없던 것이 E1272 가 찾던 **약 3.5천 회 차이**의 전부다(33+1,280×3 = 3,873회, 구름 801회는 웹도 쓰되 위치가 달랐다).

재시드가 SP 초기화 안에서 일어나므로 절대 상태는 "시계 원점 + 이후 소비 횟수" 한 줄이다. 웹은 시작 때 이미 같은 재시드를 하고 있었고, 빠진 것은 **State 설치 시점의 정리·구름·주변음 기한**이었다.

## 웹 변경(`netstorm.js`)

- `_clearPlayerSpForState()` — 소유자 1~8 의 SP 를 0 으로 쓴다(난수 1,280회). 앱은 `cleanupClientStateWorld`(State 전체 정리)를 State 기계에 연결하지 않았고, 시작·메뉴·미션 시작 시점에는 나머지 정리 대상이 이미 비어 있어 난수 소비와 SP 비우기만 같은 자리에서 한다.
- `_installIntroStateEffects()` — `_activate` 의 시작 입력 처리에서 title 그림·메인 메뉴보다 앞에 한 번.
- `_installMainMenuStateEffects()` — `_enterMainMenu` 의 상태 전환 앞에서: 정리 → battlePal 로 팔레트 전환(구름 구축; 같은 파일이라 메뉴 표면·팔레트는 변하지 않음을 확인, 이후 State7 의 같은 전환은 구름을 다시 만들지 않음) → 주변음 기한을 벽시각+10초로 재설정(`ApplyExistingEditorAfterCheckpoint`).
- `_runCampaignState7` 본문 — 시작 정보 보관 뒤, 장면 음악 시작 앞에 정리(원문 `LoadMissionWorldBody` 순서).

## 결과

- 90프레임 웹 Game 난수열(6,851회)이 cpppj 주 연쇄와 상태·상한 모두 같다.
- 웹 논리 화면이 `load.png` 와 같다(다른 픽셀 0). 섬 아랫면·미니맵 조각·건물 하나가 달랐던 E1278 의 차이가 모두 난수 순서 때문이었다.
- (E1280 시점) Session 덤프는 `random` 행과 모든 월드·Graph·장부 행이 기준선과 같고, 남는 21행은 **시각·주소 의존**뿐이었다(아래 E1281 에서 시각 차이도 없앴다):
  - `frame`(게임 시각: 웹 9.225·cpp 9.2 — 웹이 첫 안내 창 정지에 한 프레임 늦게 닿음)
  - `regular:1~6`(프로세스 기한: 한 줄은 +0.025, 다섯 줄은 웹이 0.075 이름 — 월드 생성 시각 차이로 보임)
  - `object:15000`·`slot:15`·`slot:15000`(시각에 따른 애니메이션 프레임·나이 카운터)
  - `pool`(위 객체 내용에 따른 해시)
  - `player:0~8`(앞 4바이트 주소 잔재, 알려진 환경 차이)
  - `sp 1`(소유자 0 의 미초기화 난수, 알려진 차이)
- 시험: `e2e/fixed-time.spec.js` 가 두 실행의 결정론과 위 모든 것(화면 0픽셀·허용 행 밖 차이 없음)을 확인한다. 변형 확인: 정리의 소유자 반복을 7로 줄이면 다른 픽셀 7,695개로 실패한다(원복 확인).

## 하네스 조건

cpppj 기준선(`--mission Tutorial1`)은 메인 메뉴 설치 뒤 주 루프 1프레임을 돈 다음 미션을 시작한다. 웹 시험은 같은 순서로 메뉴에서 1프레임을 돈 뒤 `_beginCampaignFromDialog` 를 부르고 주 루프 프레임 번호가 90이 될 때까지 진행한다(그 1프레임의 바람 난수까지 맞아야 난수열이 같다).

## 남은 일

- (E1281 에서 해결) 시각 차이: 정지 시각 1프레임·프로세스 기한 3프레임은 시계 출발 0.1초와 월드 진입 정지 누락이 원인이었다(아래 갱신).
- 메인 메뉴로 돌아올 때(미션 이탈) State 3 재설치의 정리·팔레트 전환·주변음 기한은 아직 연결하지 않았다(`cleanupClientStateWorld` 전체 연결과 함께). 미션을 연달아 시작하는 난수열은 이 연결 뒤에 원본과 맞춘다.
- 메인 메뉴 구름은 아직 GIF 타일이다(5-A). 구름 구축 난수(801회)는 이미 State 3 설치 위치에서 쓰이므로 Kencloud 출력으로 바꿀 때 난수 순서는 그대로 쓸 수 있다.

## 갱신 (E1281): 게임 시각 맞춤 — 덤프가 `sp`·`player` 두 종류만 다름

cpppj 오라클로 프레임 1~16 의 덤프를 모아 게임 시각(`frame` 행)을 비교했다. cpppj: 프레임 1 = 0.125, 프레임 2 = 9.15, 3 = 9.175, 4 = 9.2 이고 **프레임 4 부터 9.2 로 멈춘다**(월드 생성 프레임에 게임이 정지). 웹(E1280 시점): 9.05·9.075·9.1 로 0.1초 늦게 출발하고 정지는 관문의 첫 안내 창이 열리는 프레임 9(9.225)에서 일어났다. 원인은 둘이었다.

1. **시계 출발 0.1초.** cpppj 고정 실행은 첫 주 루프 프레임 전에 시계가 4틱(0.1초) 흐른다(첫 프레임 0.125). 웹 고정 시각 모드는 시계 원점을 잡은 뒤 4틱 전진하도록 했다(`_initializeClientRuntime`). 시작 그림 뒤 게임 시각은 9.0 → 9.1 이다.
2. **월드 진입의 미션 정지 누락.** 원본 `LoadMissionWorldBody` 는 장면 음악 뒤·월드 설치 앞에서 Guide 미션이면 `PauseMissionWorldEntry` → `PauseMissionFrameForDialog` 로 게임을 멈추고 관문의 자기 정지 표식(`scriptPaused`)을 세운다. 정지 상태에서도 관문의 `lockFrames`(10→0)는 계속 줄어 5프레임 뒤 첫 안내 창이 열리고, 창이 닫힌 뒤 재개한다. 웹 앱은 이 호출이 없어 창이 뜰 때까지 4프레임(0.1초) 동안 게임이 흘렀다. `_runCampaignState7` 본문에 `pauseMissionWorldEntry`(mission-lifecycle.js)와 `pauseMissionFrameForDialog`(raw-mission-frame-gate.js)를 연결했다(모듈은 L1 시험으로 이미 원문과 같음).

맞춘 뒤 웹 타임라인이 cpppj 와 프레임 단위로 같다(n=2 9.15, 3 9.175, 4 9.2 정지, 월드 생성 n=4, 관문 `lockFrames` 10→8→…→0 이 n=5~9). 결과:

- 90프레임·130프레임 Session 덤프(`load.tsv`·`later.tsv`)가 기준선과 **`sp 1`(소유자 0 의 미초기화 난수)와 `player:0~8`(+1·+2 바이트 주소 잔재, Player 1 은 +0x68·+0x69 도 — 기준선 0x02c0·웹 0 의 동적 Deck 필드)만 다르다.** `frame`·`regular`·`object`·`slot`·`pool` 행이 모두 같아졌다. 시험(`e2e/fixed-time.spec.js`)이 허용 행을 이 둘로 좁혀 고정한다.
- 논리 화면 0픽셀(90프레임)은 그대로다.

- 사용자에게 보이는 동작 변화(원본과 같아진 쪽): **Guide 미션은 월드에 들어서는 순간 게임이 멈춘다.** Fury War(`bc8fury76`)에서 이전에는 로딩 중 흐른 실제 시간 때문에 `[@1]`(게임 시각 1초 뒤 창) 원소 선택 창이 시작 안내 창(`[A.]`, "Lets do this!")보다 먼저 열렸다. 이제 시작 안내 창이 먼저 나오고 그 창을 닫은 뒤 게임 시각 1초 뒤에 원소 선택 창이 나온다. `e2e/fury-technology.spec.js` 가 이 순서를 따르도록 고쳤다. 전체 e2e 는 162개 통과·11개 건너뜀(옵트인)·1개 실패(이 Fury War 순서, 위처럼 수정 뒤 통과)였다.

남은 일: Player 1 +0x68·+0x69(동적 Deck 필드 — 기준선 0x02c0 은 Deck 할당 토큰으로 보이며 웹은 Deck 기억장치를 Player 기록 안에 쓰지 않는 표현 차이일 수 있음)의 원인, 미션 이탈 뒤 State 3 재설치 연결, 16:9/16:10·한국어 화면 대조.
