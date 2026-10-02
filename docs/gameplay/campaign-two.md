# 캠페인 1-2 플레이 구현 (2026-10-01)

캠페인 1-1([campaign-one.md](campaign-one.md)) 구조를 그대로 쓰고 **1-2 Master of Whirligigs**를 메뉴와 1-1 성공 창의 `Next Mission`에서 열 수 있게 했다. 모호한 부분은 지금까지의 녹화·exe·사용자 설명으로 가장 가깝게 맞췄다. 추정은 아래 표에 따로 적었다. 원본 게임은 새로 실행하지 않았다.

## 공개 범위와 진입

- 공개 목록은 `Netstorm.Core.Rules.CampaignAccess.Missions` 한 곳에서 정한다(`thewarbegins` 1-1, `masterofwhirligigs` 1-2).
  - 메뉴 캠페인 목록은 `FirstChapter` 순서대로 그리고, 공개된 미션만 활성화한다.
  - 1-1 성공 창의 `Next Mission`(`MissionBegin,MasterOfWhirligigs`)도 같은 판정으로 열린다.
- 1-3 Save the Island! 이후와 1-2 성공 창의 `Next Mission`(`SaveTheIsland`)은 잠겨 있다. 진행도 저장이 없어 1-1을 깨지 않아도 메뉴에서 바로 1-2를 고를 수 있다.
- 1-2는 `loadFort`가 없으므로 같은 이름의 맵 `masterofwhirligigs.fort`를 연다.
- T 키로 켜는 타이머에 미션 번호 `1-2`와 경과 시간을 표시한다(2026-10-02부터 원본처럼 상단 줄이 없고 타이머는 꺼진 채 시작한다).

## 미션 데이터 (`masterofwhirligigs.english`·`.fort`)

| 항목 | 값 |
|---|---|
| 플레이어 | 2,000 SP, 지식 `sunWalker;windBattery;sunArcher;sunCannon` |
| AI(플레이어 2, "Master of Whirligigs") | 2,000 SP, 지식 `sunWalker;rainBattery;sunAviary`, 수집자 1, 가이저 연결 1, 판단 6초, `!CONCENTRATE_FIRE;!REPEAT_SUCCESSFUL_DROPS` |
| 맵 오브젝트 | 내 Wind Temple·사제, 적 Rain Temple·사제·골렘 1·Sun Workshop 2·Stone Tower 5·거주지 2, 가이저 11 |
| 성공 | `[Succeeded][BadTeamDead]` — 적 사제를 희생 |
| 실패 | `[Failed]` — 내 사제가 희생됨 |

브리핑·성공·실패의 한국어 본문은 원본 문단을 번역해 `FortMapViewer.LocalizeCampaign`에 두었다. 스크립트의 명령·조건은 바꾸지 않았다.

## 플레이 조작

조작은 1-1과 같다. 2026-10-02부터 생산 창(덱)은 원본처럼 골렘과 워크샵에 등록한 유닛만 보여 준다([생산 창 문서](../screens/clone-deck.md)).
- 이전의 생산 버튼 세 칸(3~5번)과 시작 지식 자동 선택·자동 등록은 없앴다. 1-2의 Wind Generator·Sun Cannon·Sun Disc Thrower도 워크샵 우클릭 `Put Knowledge into Production`으로 등록한다.
- 워크샵 지식 목록은 원본 지식 창의 행 안 group 순서(battery → cannon → archer → blocker → fence → aviary → balloon)다.
- 생산 창 항목을 우클릭하면 원본 정보 창(이름·원소·분류·Cost)이 열린다. 한국어 짧은 이름(발전기·대포·원반 투척기 등)은 워크샵 메뉴에 쓴다(2026-10-02부터 하단 상태줄은 없다).

가이저를 **우클릭**하면 남은 Storm Power가 지도 아래쪽 알림으로 잠깐 나온다(원본 도움말 "Right-clicking on a Storm Geyser will show the available Storm Power"). 다 쓴 가이저는 원본 `emptyGeyser` 그림으로 바뀐다.

## 이번 작업에서 함께 바꾼 공통 규칙

1-2를 1-1 수준으로 플레이할 수 있게 하는 과정에서 다음 공통 규칙을 보완했다. 1-1에도 적용된다.

1. **타입 기본 비용.** `cost`가 없는 타입의 비용은 원본 타입 로더(`Rifttype.cpp`, 상수 VA `0x510298` = 200, `0x51029c` = 400)대로 계산한다(`StormPower.TypeCost`).
   - 일반: `level × 200`
   - 수송체(walker·balloon): `level × 400`
   - 그 결과 골렘 400(1-2 녹화의 골렘 판매 환급 100과 일치), Whirlibase 400(매뉴얼 값)이다.
   - group이 없는 Whirligig와 level이 없는 사제는 0이다.
   - 이전에는 이런 타입을 0 SP로 생산·회수할 수 있었다(1-1 문서의 "타입 누락 비용 0 처리" 후속 항목 해소).
2. **가이저 고갈.**
   - 가이저는 `geyser.type`의 `cost` 2,000 SP를 품는다. 싱글 플레이 전투 초기화 `FUN_004b2df0`이 가이저 타입 `+0xc4`에 넣는 `_DAT_00540bdc`의 초기값이 2000이다.
   - 결정 하나(200 SP)를 캘 때마다 줄고, 0이 되면 `GeyserDepleted` 이벤트가 나며 수집자는 반복을 멈춘다(도움말 "until the Geyser is depleted").
   - 빈 가이저에 수확 명령을 내리면 `GeyserEmpty`로 거부한다.
3. **채집 시간.**
   - 수집자는 가이저 옆에서 `HarvestMineSeconds`(1.0초, 임시) 머문 뒤 결정을 얻는다.
   - 1-2 맵은 적 신전 둘레와 가이저 둘레가 맞닿아 이동 거리가 0이다. 이 시간이 없으면 틱마다 결정이 생겨 적이 4분에 50만 SP를 모았다.
4. **임시 AI 일반화.** 1-1 전용 방어 AI를 공개 캠페인 공통으로 바꿨다(`MissionStart.Campaign`·`UsesCampaignAi`).
   - 방어 유닛은 미션 `aiTech`에서 cannon → archer → aviary 순으로 고른다(1-1 Sun Disc Thrower, 1-2 **Whirlibase**).
   - 에너지 부족으로 놓을 자리가 없을 때만 `aiTech`의 발전기(1-2 Rain Generator)를 신전 주변에 놓는다.
   - 골렘 채집·침입 사제 포획·제단·의식 규칙은 1-1과 같다.

## 근거

- [1-2 재플레이·입력 지연 점검](../videos/master-of-whirligigs-record-play-20261001-2311.md): 포획 이전 작업장 생산 슬롯(Sun Cannon·Sun Disc Thrower)·회수액 200, 신전 파괴·지면 전환·사제 기절을 재확인했다. 포획 이후 지연 보고가 있어 해당 의식 타이밍은 새 근거로 사용하지 않는다.
- [1-2 원본 녹화 판독](../videos/master-of-whirligigs-record-play-20261001.md):
  - 07:30 무렵 적 섬 신전 둘레에 Whirlibase가 빽빽하게 놓인 모습. 방어 상한 14의 근거이며, 화면으로 센 추정값이다.
  - 골렘 판매 환급 100(+100 SP)
  - 의식 일시정지·제단 판매·완료 보상 5,000
  - 템플 파괴 폭발과 25% 처치 보상
- exe: 기본 비용 상수와 타입 로더, 가이저 보유량 초기값 2000, 싱글 처치 보상 25%([전투 옵션 2-1](../exe/battle-options.md)), 파괴 폭발 `FUN_0044b9e0`([전투](combat.md))
- 원본 도움말 `help.english`의 `geyserType`·`sunAviaryType`

## 추정·차이와 후속 수정

| 현재 정책 | 추가 분석 후 수정할 내용 |
|---|---|
| 적 방어 Whirlibase 상한 14, 신전 주변 첫 합법 칸부터 배치 | 원본 AI의 생산 선택·위치·상한. 녹화의 밀집 배치는 비슷하지만 원본 전략을 복원한 것이 아니다 |
| 에너지 부족일 때만 Rain Generator 1개씩 추가 | 원본 AI 의 발전기 배치 규칙 |
| 적 초기 골렘은 1-1과 같은 방식으로 신전 주변에 1기 지급(맵에 이미 골렘 1기가 있어 총 2기) | `aiCollectors`가 맵의 기존 골렘을 포함하는지 |
| 채집 시간 1.0초 | 원본 결정 채집 시간(`geyserMined*.wav` 호출 `FUN_004266c0` 주변) |
| 가이저 2,000 SP, 미션 중 새 가이저 생성 없음 | 도움말의 "파괴될 때마다 가이저 섬이 새로 생김"(멀티 `moreGeysers`·Fury 전투) 규칙 |
| 진행도 저장 없음 — 1-2를 처음부터 선택 가능 | 원본 캠페인 체크 표시·해금(`Done{mission.fileName}` 설정) |
| 1-2 성공 뒤 Next Mission(1-3) 잠금 | 다음 범위에서 1-3 구현 |

## 검증

- `CampaignTwoTests` 5개:
  - 원본 머리 값 시작 상태(양측 2,000 SP·지식·미션 번호)
  - 적 AI의 Whirlibase 건설과 상한
  - 가이저 고갈·채집 시간·빈 가이저 거부
  - 결정론
  - 1-3 잠금 유지
- `GameRuleTests.TypeCost_UsesOriginalDefaultForMissingCost` 6건. `CampaignOneTests`의 다음 미션 판정은 1-2 공개로 갱신했다.
- 클론 UI 자동 입력(`tools/clone_ui_smoke.ps1` 갱신, 원본 실행 없음)으로 다음을 확인했다.
  - 메뉴 → 캠페인 → 1-2 활성 → 한국어 브리핑 → 전투 → 생산 커서 → 미션 떠나기 → 메인 메뉴
  - 2분 진행 화면에서 적 섬 신전 둘레의 Whirlibase 배치
  - 캡처는 Git 제외 `extracted/screens/campaign-two-20261001/`·`ui-smoke-campaign-two/`에 있다.
- 사람이 GUI로 1-2를 끝까지 완주한 검증, 원본과의 AI·밸런스 일대일 비교는 하지 않았다.
