# cpppj 캠페인 1-1 실제 실행 경로 감사

2026-10-13, `VM-W11-Codex`. AGENTS.md 및 두 LEFT_JOBS를 확인했고 현재 PC와 최신 정밀 디컴파일 PC가 일치한다. 공용 코드 및 인수인계 문서는 변경하지 않았다. 게임/GUI 실행 없이 현재 소스와 원본 자산의 읽기 전용 콘솔 출력을 확인했다. 전체화면 DirectDraw 오류는 감사/구현 권고 범위에서 제외한다.

## 판단

현재 cpppj는 정식 캠페인 메뉴에서 1-1 브리핑과 저장 지형·건물·사제를 표시하고 사제를 선택해 이동할 수 있다. 그러나 생산/건설 명령은 UI에서 비활성화되어 있고, 비표면 오브젝트는 raw 월드와 다른 객체 저장소에 있다. 실제 수확·전투·사제 운반/희생·승패 미션 프로세스가 없으므로 1-1을 진행하거나 완주할 수 없다. 많은 raw 모듈의 독립 검증 결과는 이 실행 경로의 연결 및 원본 동일성을 증명하지 않는다.

## 원본 1-1의 실제 입력

`cpppj/build/bin/Release/NetstormCpp.exe --read-game originals d/thewarbegins.english`와 `--inspect-fort originals thewarbegins`를 읽기 전용으로 실행했다. C++ 로더의 출력이므로 원본 게임 실행의 결과로 취급하지 않는다.

| 항목 | 원본 자료 |
| --- | --- |
| 미션 | `missionType="Tutorial"`, `missionNumber=1`, `aid=1`, title `The War Begins!` |
| 내 시작 SP | `myStartMoney=3000`. fort Money의 100000보다 스크립트가 우선 |
| 내 시작 지식 | `sunWalker;rainBattery;sunAviary;sunCannon` |
| 적 AI | `The Jailer`, `sunWalker;windBattery;sunArcher`, 시작 SP 0, `aiGeyserAttachments=1`, `aiCollectors=1`, 행동 간격 6초, 능력 `!ENCIRCLE_ENEMY_ISLAND;!USE_PRIEST_TO_COLLECT` |
| 내 Terr00 | 간헐천 1, 사제 1, 거주 구역 1, Rain Temple(`rainVortex`) 1 |
| 적 Terr01 | 사제 1, 거주 구역 1, 썬 디스크 발사기(`sunArcher`) 9, 썬 바리케이드(`sunBlocker`) 8, 윈드 제네레이터(`windBattery`) 3, Wind Temple(`windVortex`) 1 |
| Chaff | 다리 15, 간헐천 13, `noIsland` 117 |
| 저장 목록 | Technology 26개, Deck 38개. 시작 지식 4개와 별도 목록 |
| 브리핑 | `[A.]`, `Review Knowledge,ShowTechnology,55`, `Play Mission,DoNothing,0` |
| 성공 | `[Succeeded][BadTeamDead]`, `<$Config,Done{mission.fileName}=1>`, `Leave Missions,MissionAbort,1`, `Next Mission,MissionBegin,MasterOfWhirligigs` |
| 실패 | `[Failed]`에서 `Continue,Tell,TryAgain` |

정식 메뉴는 `UberGump::Compose`의 Campaign → `tell.UCampaign` → `guideSpec`(`\\D\\offical*.english`) → `offical3.Overview`(`Struggle For Freedom`) → `MissionBegin,TheWarBegins` 순이다. `official1`은 원본 파일명이 아니며 `offical1`은 Early Missions다.

## 현재 경로와 단절점

| 단계 | 현재 실제 호출 | 단절/차이 |
| --- | --- | --- |
| 네이티브 시작 | `Client::Run` 초기화 → `UberGump` 생성, `ClientMain.cpp:305` | 원본 전체 명령 표·Squid/spot/Graph·Guide는 초기화 댓글에서 미구현으로 기록됨(299 부근) |
| 메뉴 선택 | `UberGump::Event` → `State::Post` → 다음 루프 `Tick` → `Execute` | 현재 State는 UI 전환 하나만 유지. 게임 명령 큐와 별개 |
| 미션 로드 | `UberGump::BeginMission`, `UberGump.cpp:195` → `MissionScript` → fort parse → `MissionPlayers::Load` → `GameWorld` → Client Kernel | Tutorial 저장위치 미션만 생성. 원본 Mission 종류 객체/상태 프로세스는 `Mission.cpp`에서 아직 복원하지 않음 |
| 브리핑 | `AdvanceBriefing` → `Tell(A.)` → `ParseDialogPage` | `Supported`(94)에서 `ShowTechnology` 제외, Review Knowledge는 비활성. 첫 브리핑 프레임/종료 조건은 현재 adapter 흐름 |
| 초기 객체 | `GameWorld::Place`(58) → `objects_`에 값형 `o::Squid` | raw SID를 쓰지 않는 런타임 번호(`Squid.h` 주석)이며 Priest/Vortex/Carrier/Gun 파생 Pop/contained/Regular/HP 상태 복원 몸체를 호출하지 않음 |
| 표면 | `GameWorld::BuildSurfaces`(181) → `RawSurfaceWorld::Load` | raw seed가 isle/noIsland/bridge뿐. `RawSurfaceWorld::Load`(143)는 다른 타입을 예외로 거부. 그래프/해시/spot/프로세스 풀은 비표면 objects_ 저장소와 분리 |
| 현재 화면/선택 | `Sprites`, `Pick`, `Select` | 비표면 그림/체력은 저장시점 값형 Squid에서 읽으며 raw 파생 상태와 동일한 객체를 읽는 것이 아님 |
| 입력 이동 | `UserInput::Event`(35) → `Select` 또는 `GameWorld::MoveSelected`(305) | 원본 Walker/Path 명령 처리 대신 임시 `GroundGrid::Path`, `Squid::Advance`; `UserInput::Tick` 카메라 속도도 명시적 임시값 |
| 사제 우클릭 | `UberGump::OpenObjectMenu`(274) | `Construct >`·About를 명시적으로 비활성화. 새 raw MenuGump/Form 체인도 여기서 사용하지 않음 |
| 생산 UI | `UberGump::Compose`(438) | Game 버튼과 시작 SP만 표시. 생산 칸/Deck 시간/지식 등록/조각 추첨/미리보기 입력 없음 |
| 커서 배치 | `RawCursorDrop::Drop`, `RawConstructionLocal::Place`는 존재 | 호출처 검색상 정의/adapter helper만 존재. `UserInput`/`GameWorld`/`UberGump`가 생성 또는 호출하지 않음 |
| 서버 건설 | `RawConstructionRequest`, Confirm/Notice/Place/Builder/Process는 존재 | 여러 `MakeConstruction*Hooks`가 있으나 실행 월드에서 조립하지 않음. `RawPacketDispatch` 등록은 ConstructionData 0x37만 helper로 제공; 실제 client 인스턴스/명령 수신 흐름 없음 |
| 틱 | `GameWorld::RunFrame`(222) | 표면 Regular Kernel + 값형 객체 이동만 진행. 건설 시작/수확/전투/AI/미션 승패 갱신 없음 |
| 자원 | `MissionPlayers::Load`, `SquidReward` | 월드 UI의 SP는 MissionPlayers 초기값. raw reward 지갑/SP와 동일 저장소/경로로 연결되지 않음 |
| 승리 | 원본 스크립트 섹션만 로드 가능 | BadTeamDead 생성 없음. 성공 인라인 Config 명령 및 `page.onExit`도 현재 Execute/Tick에서 실행하지 않음 |

`ClientMain.cpp`의 원본 루프 8(튜토리얼 안내), 13(플레이어 알림)도 아직 미구현으로 명시되어 있다. 단순히 `Tell("Succeeded")`를 테스트 명령으로 여는 것은 실제 게임 승패 경로의 증거가 아니다.

## 조립만으로 끝나지 않는 필수 몸체

현재 `cpppj/src` 파일 목록에는 원본 GunProcess/Gun/Bomb 전투 실행, Carrier 수확/운반 실행, Dais 희생 실행, Deck 생산 갱신, Geyser 결정 생성, Mission/Totalmade 승패 실행 모듈이 없다. 이름만 다르게 구현한지 본문 호출도 검색했으나 현재 Client/GameWorld는 해당 행동을 호출하지 않는다. `RawCarrier*`는 Check/PostPop/PreDestroy이며 실제 운반 루프가 아니다. `RawPriest*`는 생성/상태/보호막/배치/삭제 등의 몸체이며 골렘 행동과 의식의 완료를 대신하지 않는다.

원본 정밀 출력 `extracted/refined/originals/Netstorm.c`:

- `FUN_004c34c0`(155752 부근): 미션 프레임이 `FUN_004c36c0`을 먼저 호출한다.
- `FUN_004c36c0`(155833~155918): 활성 플레이어의 신전 halfDead/dead, 사제 dead/captured/saved 상태를 조사한다. `004922e0`의 사제 생존 결과와 동맹 표로 내 팀/적 팀을 집계해 `GoodTeamDead`, `BadTeamDead`, `Failed`를 만든다.
- `FUN_004c3940`(155923 부근): 참으로 변한 조건에 대한 edge latch와 해당 스크립트 이벤트 호출을 처리한다.

원본 실제 플레이 기록 `docs/videos/the-war-begins-record-play-20260930.md:33`은 신전 파괴 → 적 사제 보호막 기절 → 골렘 포획/운반 → 제단 다섯 룬 의식 → 사제 사망/제단 소멸 → 성공 창을 확인했다. 신전 파괴에서 성공까지 약 2분 15초이며 **신전 파괴 즉시 승리 처리로 대체하면 원본과 달라진다.** F6 지식창은 전투를 정지하지 않으며 성공 스크립트 창은 게임 시계를 정지한다.

## 직접적인 다음 구현 순서

1. 현재 `RawSurfaceWorld::Impl`의 소유 풀/해시/spot/Graph/Frame/Owner/Pop/Destroy/Kernel을 비표면 생성까지 확장한 단일 월드에 보존한다. `GameWorld::objects_`와 raw SID에 같은 객체를 이중 생성하지 않는다. fort의 사제·신전·간헐천·전투 건물·거주 구역의 실제 초기화/Pop/contained/Regular를 같은 풀에 연결하고 렌더링/선택/체력도 해당 raw 슬롯을 읽는다.
2. 동일 저장소의 SP/기술/Technology/Deck를 실제 생산 목록으로 구성한다. 내 사제 Construct, 워크샾 지식 등록, 다리/생산 칸, 회전/배치 미리보기/판정/반환/비용을 실제 UI 입력에 연결한다. `ShowTechnology(55)`는 저장 Technology 목록 및 골렘 제외 규칙으로 원본 브리핑 버튼에 연결한다.
3. 이미 복원한 건설 Request → Confirm → Notice → Place → Builder/Process를 같은 월드에 조립한다. 로컬 예측 영역과 서버 SID 영역의 원본 구분은 유지한다. 단일 플레이 direct dispatch라 하더라도 요청/통지 필드·순서·실패 환불은 원본 몸체를 사용한다.
4. 원본 이동/명령, 수확/운반/결정 생성, GunProcess/투사체/피해, AI 행동을 복원하고 같은 월드의 실제 프레임에 등록한다. 기존 임시 이동을 최종 원본 동일성 근거로 삼지 않는다.
5. 골렘의 포획/사제 운반과 Dais 희생, 미션 `004c34c0/004c36c0/004c3940` 사건 및 성공 인라인 Config를 연결한다. 성공 후 저장/재시작/다음 미션 해제도 실제 경로로 검증한다.

각 단계의 검증은 새 연결 자체를 증명해야 한다. 기존 wrapper 전체 테스트를 반복 통과시키는 것만으로 UI 입력→생산→사제 건설→경제/전투→골렘 운반→의식→성공 경로를 증명할 수 없다. 최종 목표에는 원본10.78과 클론의 실제 1-1 진행/완주와 행동/표시/소리 대조가 필요하다.
