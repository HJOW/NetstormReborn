# cpppj 복원 결과를 dotnetpj로 이식하는 계획

2026-10-16, 작업 PC **DESKTOP-HJOW**. 기준 커밋은 **`53113b66`**(cpppj `20261016-12`까지)이다(W01 이후의 현재 반영 커밋은 [7절](#7-cpppj-진행을-따라가는-절차-다른-ai가-cpppj를-계속-진행-중)이 가리키는 목록 파일이 소유한다). 사용자 요청은 "현재까지 진행된 cpppj 소스를 dotnetpj에 적용해 dotnetpj 게임 플레이가 기존 게임과 거의 동일한 수준이 되게 하는 계획"이다. 이 문서는 그 **실행 계획**이며 게임 코드는 바꾸지 않았다. 진행 체크리스트와 인수인계는 [dotnetpj 인수인계](../LEFT_JOBS.dotnetpj.md) 맨 위 절이 소유한다.

읽은 자료는 AGENTS.md, 양쪽 인수인계, [D01~D16 반영 점검](dotnet-cpp-audit-20261015.md), [cpppj 플레이 복원 계획](cpp-playable-plan.md), cpppj의 `src/o`·`src/client` 모듈 목록과 [RawCampaignSession.h](../cpppj/src/client/RawCampaignSession.h)·[RawSurfaceWorld.h](../cpppj/src/client/RawSurfaceWorld.h), `cpppj/tests/fixtures` 전체 목록과 행 수, dotnetpj 소스·검사 구성이다. **디컴파일 산출물은 읽지 않았다.** 이 PC는 마지막 전체 정밀 디컴파일 PC(VM-W11-Codex, 2026-10-16)와 다르다. 이식 근거는 커밋된 cpppj 소스·문서·원본 기계어 fixture로 충분하다. 원본·클론 게임과 창은 실행하지 않았다.

## 1. 목표와 "거의 동일"의 판정 단계

목표는 dotnetpj가 원본 10.78과 거의 같은 플레이를 제공하는 것이다. 근거는 cpppj가 원본 기계어로 검증한 동작이다. 플레이가 된다는 사실을 동일성으로 세지 않도록 판정을 다섯 단계로 나눈다.

| 단계 | 판정 | 근거 자료 |
| --- | --- | --- |
| **L1 모듈** | 같은 fixture의 10.78 행이 반환값·상태 바이트·float/double 비트·호출 순서·난수 소비까지 전부 일치한다. | `cpppj/tests/fixtures/*-x86.tsv`, 같은 파일을 읽는 `cpppj/tests/*Tests.cpp` |
| **L2 세션 시작** | 같은 미션의 Load/Start 뒤 월드가 cpppj 실제 Session과 같다. 객체 SID·타입·프레임·소유자·HP·좌표, Player 9개 기록, SP float 비트, 방향별 동맹 81 DWORD, Graph, 장부가 대상이다. | cpppj 세션 덤프(8절 요청 1). 그전에는 기존 `--dump-world`·`world-start-1078.tsv` 범위 |
| **L3 결정론 진행** | 같은 프레임 시각·입력열을 준 뒤 N프레임의 상태 해시가 cpppj와 같다. | cpppj 고정 입력 재생 도구(8절 요청 2) |
| **L4 화면·소리** | 영어·4:3 기본 경로에서 그리기 순서·프레임·색·UI·소리 시점이 원본과 같다. | cpppj 캡처, 원본 녹화·스크린샷. 허용 PC에서만 창 실행 |
| **L5 자연 완주** | 실제 입력으로 승리/실패 → 진행 저장 → 종료·재실행 → 다음 미션 해제가 원본과 같다. | GUI 시나리오 기록. 강제 완료 섹션·합성 전투는 세지 않는다 |

**1차 범위**는 cpppj의 현재 목표와 같은 11개 미션이다. Early Missions 1~6(Tutorial1~6), 캠페인 1-1·1-2, 데모 3개(DemoStormRages·GuardMyBack·DissolvedAlliance)가 대상이다. 데모 2·3은 캠페인 2-4·1-6과 같은 미션 키·스크립트·Fort를 쓴다([대응 기록](gameplay/demo-campaign-map.md)). 그 밖의 미션은 cpppj가 해당 객체를 복원한 뒤 넓힌다.

## 2. 출발점 (2026-10-16)

| 항목 | cpppj | dotnetpj |
| --- | --- | --- |
| 규모 | `src/o` 470파일·`src/client` 219파일, 원본 대응 `Raw*` 모듈 213개(o 189·client 24), 검사 290파일 | `src` 176파일(Core 약 14.7k·Game 약 10.8k·Assets 약 7.1k줄), 검사 88파일 |
| 원본 기대값 | fixture 280개(TSV 279·JSON 1), 주석 제외 약 **682,275행** | 이 중 **14개**만 연결: bridge, bridgedecay, config, display, graph, hash, missionframe, options, original, player-participation, renderer, sid, surface, tutorial-one |
| 통합 상태 | `RawCampaignSession` 하나가 실제 월드 로드·Player 9·SP 저장소·AI 생성/초기 갱신·사제 참여·승패 확인·로컬 간헐천·생산 Deck·Totalmade·기술·사제 이동·신전 Draw·건설/피격/궁수/이동/수명 바인딩을 소유한다. 목표 11개 미션(실제 8미션과 데모 3개) 모두 헤드리스 Load/Start가 통과한다. | `BattleSession`이 엔티티 목록과 24Hz 고정 틱 위에서 관찰·팬게임 기반 규칙을 돌린다. 원본 이식 모듈 일부만 세션에 연결됐다. |
| 검사 | Release 경고 0·오류 0, CTest 내부 **1,340개 전부 통과**(20261016-12, 맨 오 워 풀 TYPE143 프로세스 복원 포함) | **이번 기준선:** SDK 10.0.402, Release 경고 0·오류 0(16초), 단위 **1,057개 통과**(Assets 343·Core 714, 실패 0, 35초) |
| 플레이 | GUI Construct 비활성, GUI Start/StartDemo·Guide·미션 프레임 연결 전. AI 전략 미복원. 자연 완주 없음 | 캠페인 1-1·1-2 플레이 가능. Early Missions·데모는 메뉴에서 잠김(`CampaignAccess`는 1-1·1-2만 공개, Demo 버튼 비활성) |

**dotnetpj에 이미 있는 원본 이식 부분의 실제 연결 상태:**

| C# 클래스 | 원본 근거 | 연결 |
| --- | --- | --- |
| `SidPool` | sid 3,168 | `BattleMap`·`BridgeGrid`에 연결 |
| `BridgeDecayRules`·`SurfaceFinder`·`BridgeSurfaceRules` | bridgedecay·surface | `BridgeGrid`에 연결 |
| `MissionFrameGate` | missionframe 328 | 화면 경로에 연결 |
| `PlayerParticipation`·`MissionParticipation` | player-participation 608 | 세션 생성·미션 시작에 연결 |
| `TotalMade`·`TutorialOne` | tutorial-one Total 52·Stage 266 | 세션 장부·튜토리얼 1 판정에 연결 |
| `OriginalGameClock`·`BattleSessionFrameClock` | 숫자 시계 4,464 비트 | 화면 루프·세션 시계에 연결. 규칙 틱은 여전히 `FixedTimestep` |
| `DrawOrder` | renderer Order 261 | 뷰어 정렬에 사용. CRT 동률 순서는 아님(D15) |
| `SurfaceGraph`·`SpotMap`·`MsvcRandom` | graph 2,560·hash·spatial | **세션에서 쓰지 않는 라이브러리** |

## 3. 이식 원칙

1. **근거 우선순위:** 원본 기계어 fixture → cpppj 복원 보고서(`docs/exe/cpp-*.md`)의 계약·경계 → cpppj 소스 → 원본 녹화·스크린샷 → 팬게임 순이다. cpppj 소스와 fixture가 다르면 fixture를 따르고 8절 요청으로 남긴다.
2. **이식 단위는 "cpppj 모듈 + fixture + 보고서의 외부 경계"다.** cpppj가 "대체 경계·명시 외부 오류·초안·미복원"으로 둔 부분은 C#에서도 경계로 둔다. 추정으로 채워 원본 확정처럼 표기하지 않는다.
3. **의미를 옮기고 메모리 배치는 옮기지 않는다 — 단 원본 월드 코어의 저장소는 예외다(2026-10-16 W02 첫 조각에서 변경, [상세](dotnet-original-core-20261016.md#1-저장-방식-결정-슬롯-바이트를-그대로-둔다)).** 세션·`GameEntity`·화면 쪽은 슬롯 배치와 vtable 주소를 옮기지 않는다(기존 9절 방침). 그러나 코어 저장소(`Netstorm.Core.Original`)는 슬롯 50바이트를 그대로 둔다. fixture가 슬롯 전체 바이트를 대조하고 파생 생성자의 쓰기가 필드 이름 없는 오프셋이라 의미 모델로는 검증할 수 없기 때문이다. 가상 표 주소는 클래스 식별 값으로만 쓴다. 어느 쪽이든 결과로 관찰되는 것은 그대로 재현한다. SID 할당 순서, float32 비트와 저장 지점, 정수 감김(WORD·BYTE), 목록·연결 순서, CRT `qsort` 동률 순서, 난수 소비 횟수가 그 대상이다.
4. **부동소수점:** Win32 프로세스의 기본 x87 제어어는 `027f`(53비트)이고, cpppj는 `027f`·`037f`(64비트) 두 정밀도로 관찰해 왔다. C# `double` 연산이 행마다 같은 결과를 내는지 fixture로 확인한다. 원본이 `fstp dword`로 중간값을 float에 저장하는 지점은 명시적 `(float)` 변환으로 재현한다. 두 정밀도의 결과가 다른 입력은 해당 cpppj 보고서가 정한 기대값을 따른다.
5. **교체형 이식(플레이 유지):** 각 단위는 ① L1 모듈 이식 → ② 세션 연결 → ③ 근사 경로 제거 → ④ 회귀 순서로 진행한다. 이식 도중에도 1-1·1-2는 계속 플레이할 수 있어야 한다.
6. **cpppj 미복원 동작은 근사를 유지하되 등록한다.** 대상은 데빌 메이커 TYPE59의 프로세스, 플라이어 BaseFlyer(`0045c1b0`, 생성·이동·공격·귀환), 썬 바리케이드 Regular 공격, 생산 재시작의 상위 호출자, AI 전략 등이다. cpppj처럼 예외로 멈추지 않는다. 대신 [core-rules.md](core-rules.md)의 "근사·미구현" 목록에 원본 근거 부재를 적고, cpppj가 복원하면 교체한다.
7. **기존 요구사항 기능은 보존한다.** 한국어·와이드 시야·전체화면 저장 같은 기능은 되돌리지 않는다. 동일성 판정은 영어·4:3 기본 경로에서 먼저 한다(기존 10절 6번 기본값).
8. **명칭·주석 규칙:** 표시 이름은 [표시 이름·내부 타입명 대응](unit-source-name-map.md)을 따른다. 예: `sunFence`=썬 바리케이드, `sunAviary`=훨리베이스, `sunBlocker`=스톤 타워. 새 코드의 상수·함수·구조체·클래스·멤버·반복문에는 한국어 주석을 단다. 설명·사용법을 먼저 쓰고, 추가 시점과 이유는 그 뒤에 쓴다.

## 4. 구조 결정: C# 원본 월드 코어 도입

**판단(2026-10-10 사용자 결정 규칙 적용):** 현재 구조로는 원본 동작을 재현할 수 없으므로 구조를 바꾼다. 근거는 다음과 같다.

- cpppj의 검증된 모듈은 거의 모두 같은 공유 상태를 전제한다. SID 풀, 4단계 해시·spot, 타입 표, Graph, Pop/Unpop/PostPop, 공통 삭제, Kernel/Process/Regular, 장부, SP 저장소, Player 9개가 그것이다([RawWorldCore](../cpppj/src/client/RawSurfaceWorld.h)).
- `BattleSession`의 엔티티 목록과 24Hz 틱으로는 다음을 표현할 수 없다. 객체 생성·삭제의 원본 순서, Regular 타이머의 "양수 초/0 종료/음수 다음 프레임" 규칙, SP 쓰기마다 난수 160회 소비, 삭제 때의 Graph 분할·소유권 전파가 그 예다.
- 지금까지 라이브러리로만 옮긴 모듈(`SurfaceGraph`·`SpotMap` 등)은 게임 플레이를 바꾸지 못했다([점검 4절](dotnet-cpp-audit-20261015.md#4-검증-자료의-이식-방법)).

**만들 것:** `dotnetpj/src/Netstorm.Core/Original/`(이름공간 `Netstorm.Core.Original`)에 cpppj `RawWorldCore`와 `RawCampaignSession`에 해당하는 C# 코어를 둔다. 이름과 파일 분할은 구현자가 정해도 된다.

- 객체 표: SID → 타입·프레임·소유자·flags·HP·좌표(float 비트 유지)·상태 바이트·부모/종속 연결. **구현은 슬롯 바이트 풀(`RawSidPool`)이며 이 필드들은 슬롯 안의 정해진 위치에서 읽는다**(3절 3번 변경). 게임이 쓰는 모양은 나중에 이 위의 보기로 만든다.
- 타입별 가상 처리 표: vtable 대신 TYPE 번호(또는 genus)별로 PostPop·PreDestroy·PostDestroy·Event·Frame 처리기를 등록한다. 등록되지 않은 타입은 3절 6번의 근사 처리기를 명시적으로 단다.
- 기존 `SidPool`·`SurfaceGraph`·`SpotRules`/`SpotMap`·`TotalMade`·`NetstormRandom`·`OriginalGameClock`을 이 코어의 부품으로 재사용한다.
- `BattleSession`은 이 코어 위의 파사드가 되고, `GameEntity`·화면 조회는 코어 객체의 보기(view)가 된다. 한 번에 갈아엎지 않는다. 영역별로 W 단위가 끝날 때마다 기존 경로를 걷어 낸다.

## 5. 작업 단위와 순서

cpppj TSV fixture 279개를 영역별로 묶어 이식 단위 W01~W20으로 나눴다. 행 수는 주석을 제외한 줄 수다(머리글 행이 있으면 포함). 각 단위의 근거 보고서는 `docs/exe/` 아래의 같은 이름 계열(`cpp-<영역>-*.md`)이다. 기존 점검 항목 D01~D16과 새 항목 D17~D24의 대응은 6절에 있다.

### P0 — 기반 (모든 후속 단위의 선행)

| 단위 | 내용 | fixture(행) | 교체·재사용할 C# | 완료 기준 |
| --- | --- | --- | --- | --- |
| **W01** 기준선·동기화 장치 | 기준선 기록, fixture 연결 규칙, `X86Fixture` 확장(double 비트·여러 절 행), 연결 fixture의 SHA 목록 검사, 7절 동기화 절차 | — | `tests/Shared/X86Fixture.cs`, `Netstorm.Core.Tests.csproj` | cpppj fixture가 바뀌면 검사가 실패해 재동기화를 요구한다. **완료(2026-10-16):** 7절의 `cpp_sync.py`·`cpp-fixture-manifest.tsv`·`CppFixtureManifestTests`, `X86Fixture`의 `ULong`·`Long`·`DoubleBits`·`ULongs`·`Section`·`Sections` |
| **W02** 원본 월드 코어 ① 생성·등록 | 객체 표, 생성자·파생 생성, Pop/Unpop/PostPop, 해시·spot, 소유자·프레임 지정, 건물 공간, 이웃·탐색, Graph 연결 전체 | constructors 359, creation 1,648, derived 7,468, pop 6,240(+vtables 151), unpop 8,422(+vtables 151), postpop 2,140, assetpostpop 1,636, spatial 1,382, owner 3,000, setframe 5,777, squid-level 96, buildingspace 1,759, neighbor 1,728, rawfinder 1,776, rawgraph 1,796, regiongraph 1,926, graphremove 1,548, graphrebuild 582, graphrecovery 390, graphlookup 774, graphforms 64, graph-reset 17, metadisplaydirty 1,040 | `GameEntity`, `BattleMap` 점유, `Footprint`, `SpotMap`, `SurfaceGraph`, `IslandOwnership`, `BridgeReach` | L1 전부 일치. `BridgeGrid`의 Graph 표면 수 공급자가 실제 Graph를 읽는다(D14). **진행: 첫 조각 완료(2026-10-16)** — `RawSidPool`(sid 10.78 1,584)·생성자 표(constructors 188)·`RawSquidFactory`(creation 482·derived 3,300). [기록](dotnet-original-core-20261016.md) **둘째 조각 완료(2026-10-16)** — `RawSquidHash`·`RawUnpop`(unpop 10.78 1,809회, 표시 지원 표 83행; Take의 공간 해제와 `FirstPopFlags` 대조 포함). [기록](dotnet-original-core-unpop-20261016.md) **셋째 조각 완료(2026-10-20)** — `RawPop`·`OriginalPopDisplay`(pop 10.78 2,456회, 파생 지원 주소 26개·마나 대체 3개). [기록](dotnet-original-core-pop-20261020.md) **넷째 조각 완료(2026-10-20)** — `RawPostPop`(공통 postPop 10.78 492회·다리 접두 80행, 파생 몸체 미연결). [기록](dotnet-original-core-postpop-20261020.md) 다섯째 조각 전반 완료(2026-10-20) — RawCarrierPostPop(운반체 직접·Damageable 직접 호출 10.78 2,448회, 계획상 W09 소속이나 W02 자산 전제로 선행). [기록](dotnet-original-core-carrierpostpop-20261020.md) 다섯째 조각 후반 완료(2026-10-20) — RawAssetPostPop(자산 전용 후처리 10.78 818회·묶은 기록 818회). [기록](dotnet-original-core-assetpostpop-20261020.md) 여섯째 조각 완료(2026-10-20) — SquidOwner(소유자 지정 10.78 1,000회·가변 소유자 288회). [기록](dotnet-original-core-owner-20261020.md) 일곱째 조각 완료(2026-10-20) — SquidFrame(프레임 지정 10.78 1,920회). [기록](dotnet-original-core-frame-20261020.md) 여덟째 조각 완료(2026-10-20) — RawBuildingSpace(건물 옆면·수명 진단, Pop·Unpop 연결 10.78 123단계). [기록](dotnet-original-core-buildingspace-20261020.md) 아홉째 조각 완료(2026-10-20) — RawSquidFinder(탐색 회귀 10.78 584회). [기록](dotnet-original-core-finder-20261020.md) 열째 조각 완료(2026-10-20) — RawSquidNeighbors(첫 이웃 10.78 288회). [기록](dotnet-original-core-neighbor-20261020.md) 열한째 조각 완료(2026-10-20) — RawBridgeEvents(사건 경유 칸 변환 10.78 288회). [기록](dotnet-original-core-bridgeevent-20261020.md) 열두째 조각 완료(2026-10-20) — RawMetadisplayDirty(미니맵 큐 10.78 520행, Pop·Unpop 연결 지점 구현). [기록](dotnet-original-core-metadisplay-20261020.md) 열셋째 조각 완료(2026-10-20) — RawGraph 표 저장소·초기화·반납(graph-reset 10.78 16행). [기록](dotnet-original-core-graph-20261020.md) 열넷째 조각 완료(2026-10-20) — RawGraph 영역·Add/Flood·등록 후처리(rawgraph 768·regiongraph 512회, 합계 1,280회). [기록](dotnet-original-core-graphops-20261020.md 열다섯째 조각 완료(2026-10-20) -- RawGraph.Detach(삭제 준비 10.78 514회: graphremove 386, graphlookup 128). [기록](dotnet-original-core-graphdetach-20261020.md) 열여섯째 조각 완료(2026-10-21) -- RawGraph.Rebuild(전체 재구성 10.78 128회: graphrebuild 96, graphforms 32). [기록](dotnet-original-core-graphrebuild-20261021.md) 열일곱째 조각 완료(2026-10-21) -- graphrecovery FullPool 재생(10.78 64회: Add·PostPop·Pop·Detach 각 16). [기록](dotnet-original-core-graphrecovery-20261021.md) 열여덟째 조각 완료(2026-10-21) -- RawGraph.PostDestroy·Frame(합성 검사 3건, destroygraph 전체 재생은 W03). [기록](dotnet-original-core-graphpostdestroy-20261021.md) 남은 것은 W03 삭제·수명(`SquidDestroyLifecycle`·`destroygraph`), 파생 분배 허브·실제 세션 연결이다. |
| **W03** 원본 월드 코어 ② 삭제·수명 | 공통 삭제(pre/post·장부·Graph), Damageable 피격/해방, 종속 조회, 내용물·Form 수명, 신전 삭제 | destroy 1,992, destroylifecycle 1,728, destroygraph 1,728, damageablehit 5,846, damageablepredestroy 4,896, damageablepostdestroy 4,412, damageablerelease 4,512, contained 2,415, contentlifetime 396, formlifecycle 912, formtake 600, form-packet 1,152, vortex-pre 103 | `BattleSession.Combat.RemoveEntity`와 흩어진 정리 코드 | 삭제 한 번이 장부·Graph·선택·감시·SID 반납을 원본 순서로 처리한다 |
| **W04** 시간·Kernel·Process | Kernel·BaseProcess·Regular·ProcessForm, 공통 프레임 진행, 메뉴/미션 정지 소유권 | process 360, processform 1,836, processretake 388, processtransmit 376, assetregular 1,976, player-regular 658, frameadvance 23,040, campaign-frame 764, disc-regular 192 | `FixedTimestep`, 각 규칙의 `*Tick`/`TicksFor`, [D01 시간 소비 목록](dotnet-mission-frame-gate-20261015.md) | 옮긴 소비자는 원본 기한·정밀도로 동작한다. 24Hz·8틱 상한을 소비자 단위로 제거한다(D01) |
| **W05** SP·보상·난수 | float32 SP 저장소(쓰기마다 전역 난수 160회 소비), 삭제 보상·해체 환불·AI 지갑·샘 풀, SP 회전, Player 통지 | reward 7,200, sprotate 61, player-content-notice 88, player-damage-notice 144 | `PlayerState` int SP, `GameEntity` 비용·회수 25%, 전투 보상 | SP 쓰기 뒤 다리 추첨·회복의 난수열이 원본과 같다(D12) |
| **W06** 미션 로드·월드 생성 | Fort 객체/월드/메타/돈 적재, 소유자 마감, 캠페인 시드, 지형 성장·도장·농지, 섬 생성·조회·실행, 섬 PostPop·수명, 영역 소유·칠하기, 기준점, 다리 연결·효과·이벤트·PostPop | fortobjectload 1,218, fortworldload 534(+builder-consumer 2), fortmeta 950, fortmoney 408, fortfinishowner 1,438, campaign-worldload 130, campaign-seed 96, terraincontext 224, terraingrowth 108, terrainstamp 1,560, farmcluster 208, islandbuilder 1,156, islandqueries 8,490, islandruntime 2,018, islandpostpop 1,988, islandlifecycle 2,055, region-owner 1,632, regionownership 1,320, regionpaint 484, playeranchor 4,794, bridgepostpop 240, bridgeconnect 11,856, bridgeeffects 6,170, bridgeevent 4,656 | `BattleSessionFactory`의 미리보기 지면, `FortTerrainPreview`·`FortEdgeFarmPreview`·`FortIslandSupports`, `BridgeLinks` | 11개 미션의 시작 월드가 cpppj와 같다(L2 일부) |
| **W07** 미션 시작·플레이어·AI 초기화 | 미션 Start, 전투 옵션, Player 월드·기술·색, 사제 참여, AI 생성자·초기 갱신·요구 에너지, 간헐천 AI 등록, 생산 Deck·갱신, 단독 미션 로그인 상태·타이머·Guide(내부 순서로만) | mission-start 205, mission-battle-options 3,306, mission-settings-packet 165, asset-player 576, player-world 1,680, player-technology 159, player-color 217, priest-participation 304, ai-brain 161, ai-initial-update 134, type-required-energy 2,353, geyser-ai-registration 604, production-deck 248, production-refresh 242, local-mission-state 34, mission-login 49, mission-login-timer 1,985, mission-guide 1,551 | `MissionStart`의 AI SP/지식, `PlayerColors`, `ProductionDeck`, `KnowledgeCatalog`/`TechPermissions`, `BattleSession.CampaignAi`의 초기화 부분 | **이정표 M1:** 11개 미션 Load/Start 상태가 cpppj와 같다(L2) |

**네트워크 관련 fixture의 처리:** mission-client-packets 158, mission-server-packets 188, local-mission-transport 67, chat-channels 2,449, battle-bcast 1,356, fort-image-packets 220, construction-packet 87이 해당한다. 싱글플레이에서는 프로세스 내부 순서로 같은 효과 순서만 보장한다. 이 fixture는 상태·순서 확인에만 쓴다. TCP/IP 전송과 outpost(outpostlifecycle 2,304)는 cpppj 4차 목표 이후다.

### P1 — 플레이 규칙

| 단위 | 내용 | fixture(행) | 교체할 C# | 완료 기준 |
| --- | --- | --- | --- | --- |
| **W08** 사제 | 사제 Pop/PostPop/상태, 직접 피격 HP1·회복·보호막·포스필드, 낙하·정지·소유자·삭제·생성·조회, 충돌·기하·모양·지형·배치·미리보기, 의식 동작, 메뉴, Dude(대피 인원) | priestpop 3,840, priestpostpop 7,176, priestpostpoptail 6,144, prieststate 20,256, priestdamage 3,424, priestregen 4,539, priestshield 2,496, priestforcefield 10,416, forcefieldpostpop 3,624, priestfall 1,368, priesthalt 864, priestowner 7,416, priestdestroy 6,552, priestspawn 1,236, priestlookup 4,851, priestworldevents 2,872, priestcollision 11,520, priestgeometry 4,352, priestshape 8,064, priestterrain 6,720, priestplacement 3,024, priestpreview 4,860, priestmenu 906, priestritualactions 1,752, dude-space 48, dudedrop 308, dudenotice 414, dudespawn 288, dudetask 780 | `Combat`의 절반 HP 기절, `PriestRecoveryPerSecond`, `FortMapViewer.ContextMenu`의 사제 메뉴 조립 | D09·D13의 메뉴 부분 |
| **W09** 경로·이동·운반·수확 | Path 검색·Init·Start·준비·자료·실행·프로세스 타입·애니메이션·정리, 운반체 도착/검사/PostPop/PreDestroy, 간헐천 채굴, 로컬 간헐천 생성, Player 간헐천 사각형 | pathsearch 644, pathinit 64, pathstart 512, pathprepare 48, pathdata 768, pathruntime 360, pathprocesstype 718, pathanimation 6,052, pathcleanup 1,732, carrierarrival 406, carriercheck 2,028, carrierpostpop 7,344, carrierpredestroy 2,880, geysermined 2,312, local-geyser 221, player-geyser-bounds 439 | `BattleSession.Harvest`의 거리표 탐색, `Movement`, `MovementRate`, `UnitHeading`, `TutorialGeysers` | D11, D05의 간헐천 생성 |
| **W10** 배치·건설·생산 | CanonDecoder 배치 파이프라인 전체, 건설 요청→로컬 예측→확정→통지→정리→배치→Process→끝 시각·자료·빌더·효과·도착·타이머·표시, 커서 내려놓기, 생산 칸 반환, 미리보기 수집·Draw | canontype 3,927, canonshape 5,950, canongeometry 5,967, canonterrain 11,220, canonsurrounding 1,572, canonrelations 6,921, canonpermission 16,890, canoncollision 22,680, canonpreview 4,860, canonplacement 3,360, canonmayplace 432, constructionrequest 2,192, constructionlocal 1,450, constructionconfirm 1,300, constructionnotice 1,611, constructionclear 1,020, constructionplace 3,682, constructionprocess 2,868, constructiontime 1,269, constructiontimer 192, constructiondata 5,340, constructionbuilder 1,110, constructioneffects 3,534, constructionarrival 306, constructionvisual 1,776, constructionframe 1,317, constructionlifetime 1,377, cursordrop 2,058, returnpiece 1,899, previewcollection 1,560, previewdraw 1,431 | `BattleMap` 배치 검사, `PlacementCursor`, `BattleSession.Construction/Commands/Production`, `ConstructionTimes`, `BridgeCursor`/`BridgeTray`, `FortMapViewer.Placement*` | D13. **이정표 M2:** 1-1에서 원본 건설 파이프라인으로 짓고 SP·환불이 원본과 같다 |
| **W11** 전투·방어·파생 건물 | 총 Process·표적·피해 시각·충돌·투사체·애니메이션·궁수·각도·감시, 궁수 타입·미사일, 투사체 기하, 캠페인 피격, 방어, Burning, 썬 바리케이드·훨리베이스·맨 오 워 풀(TYPE143, 생산 프로세스 60)·매립·Factory·룬 PostPop | gunprocess 2,996, guntarget 3,454, gunhit 2,058, gunimpact 2,274, gunmissile 296, gunanimation 2,560, gunarcher 1,690, gun-angle 519, gun-depend 138, archertype 44, archermissile 1,888, projectile-geom 240, campaign-hit 96, squiddefense 48, burningframe 426, burningparticle 564, burningprocess-constructor 256, burningsources 652, fence-postpop 331, fence-neighbors 1,905, fence-line 100, fence-target 856, sun-aviary 1,735, rain-aviary 1,603, buried-postpop 581, factorypostpop 996, runepostpop 1,920 | `BattleSession.Combat/Defenses/Flyers`, `CannonAnimation`·`CrossbowAnimation`·`SunCannonAnimation`·`ProjectileAnimation`·`CombatImpact`, `FlyerFlight`, `CombatImmunity`, `EmplacementDirection` | D10·D20 |
| **W12** 희생·신전·제단 | 신전 의식, 제단 희생, 승천, 신전 소유자·표시, 레인 신전 Draw, 받침대(dais) 다리·내용물 통지·수명·진행·Regular | templeritual 6,720, altarsacrifice 2,866, vortexascend 1,496, vortex-owner 1,728, vortex-display 648, rain-draw 232, daisbridge 640, daiscontentnotice 1,444, daislifecycle 1,928, daislifecycle-current 1,928, daisprogress 2,964, daisregular-core 668 | `BattleSession.Sacrifice`, `StructureAnimation.VortexFrames` | D15의 신전 부분 |
| **W13** 미션 진행·안내·승패 | 승패(신전 HP·사제 상태·참여·양방향 관계), Guide, 튜토리얼 2~6 단계, HTML 조건·본문·그림, 스크립트 명령 큐·완료값, 20칸 Warning | missionoutcome 728, tutorial-two 346, tutorial-later 534, htmlcondition 754, htmltext 816, htmlpicture 276, script-command 263, warnings 254(+W07의 mission-guide) | `BattleSession.Sacrifice`의 `_seenAlive` 승패, `MissionConditions`, `MissionScript`, `TutorialDialogScript`, `TutorialStages`, `SessionEvents` | D04·D06·D07·D08·D18. **이정표 M3:** 1-1·1-2를 원본 규칙으로 자연 완주한다(AI 전략은 근사 유지) |

### P2 — 표시·UI·소리 (P1과 병행 가능)

| 단위 | 내용 | fixture(행) | 교체할 C# | 완료 기준 |
| --- | --- | --- | --- | --- |
| **W14** 표시 | CRT 정렬·병합 제출·가시 수집·객체/묶음/미리보기 수집·sprite Draw·그래픽·hotspot·셰이프 버퍼, 팔레트 색·번쩍임(날씨·번개), 건설 표시, AI 색 표와 받침·종유석 프레임 | drawsort 1,155, drawsubmission 966, rendercollection 4,305, objectcollection 1,431, tilegroup 1,059, spritedraw 1,389, graphics 709, typehotspot 1,404, shape-to-buffer 164, palettecolor 786, palettecolors 24, paletteflash 768(+player-color, constructionvisual/frame) | `DrawOrder` 안정 정렬, `DisplayBounds`, `FortMapViewer.WorldDisplay/Sprites/Effects`, `ObjectColorRemap`·`IsleColorRemap`·`Palette`, `PlayerColors` | D15·D21. L4 화면 |
| **W15** UI | Gump 생성·트리·수명·사건·입력·프레임·Draw·수집·시각·글리프·글자·배경, 버튼·돌 버튼·메뉴, UI 초기화, 창 제목, 초기 카메라, StyleText·안내 본문 배치·컨트롤 | gumpcreate 945, gumptree 1,722, gumplifetime 519, gumpevents 771, gumpinput 696, gumpframe 810, gumpdraw 1,428, gumpcollection 1,167, gumpvisual 192, gumpglyph 786, gumptext 933, gumpbackground 177, gumpbackgrounddraw 627, buttongump 742(+x87-64.json), menugump 309, uireset 242, window-title 1,152, camera-home 2,364 | `MainMenuView`, `OriginalUiSkin`, `HelpWindow`, `Rules/ButtonGump`, `FortMapViewer.PlayUi/ContextMenu/Knowledge/TutorialDialog` | D23. 메뉴·안내 창의 배치·입력 순서가 원본과 같다 |
| **W16** 소리·음악 | 재생·소리 Process·제어(장치 계층은 MonoGame/OpenAL로 대체), 음악 열기·흐름·실행·선택·제어, 장면 음악 | soundplay 14,472, soundprocess 18,381, soundcontrol 3,189, sounddevice 1,305, musiccontrol 5,292, musicopen 1,872, musicruntime 4,800, musicselection 2,064, musicstream 5,088, scenemusic 9,339 | `MusicDirector`, `PcmChannels`, `AudioPlayer`, `FortMapViewer.Audio` | D24. 곡 선택·전환·위치 효과음 시점이 원본과 같다 |

### P3 — 미션 확장과 최종 검증

| 단위 | 내용 | 선행 | 완료 기준 |
| --- | --- | --- | --- |
| **W17** 데모 3개·캠페인 1-6/2-4 | 원본 StartDemo(미션 생성 뒤 +68=1), 데모 로컬 AI, 360초 수명, 메뉴 Demo 버튼, 같은 자원을 쓰는 캠페인 1-6·2-4 공개 판단 | W07·W13, cpppj의 TYPE59·BaseFlyer 복원 | D22. 데모가 원본처럼 시작·종료되고 캠페인 쪽은 로컬 AI 없이 사람 입력·자연 승패·저장을 따로 검증한다 |
| **W18** Early Missions 공개와 자연 완주 | Tutorial1~6을 메뉴에 공개하고 11개 미션 L5 검증, 진행 저장 → 재실행 → 다음 미션 해제 | P1 전체 | **이정표 M4** |
| **W19** cpppj 대기 항목 | AI 전략, 데빌 메이커 TYPE59 프로세스, 플라이어 BaseFlyer(생성·이동·공격·귀환), 썬 바리케이드 Regular 공격·연결선 생성·소유자 변경, 생산 재시작 상위 호출자, 1-3 이후 캠페인 | cpppj 복원 | 3절 6번 근사 목록에서 하나씩 제거 |
| **W20** 요구사항 | 16:9·16:10, 60·120프레임, 한국어 마무리, 전체화면 저장·재실행 무오류, Linux | 원본 동일 동작(①) 완료 | 기존 요구사항 기능은 그동안 회귀만 유지 |

**순서의 이유:** W02~W05의 공유 코어가 없으면 W06 이후 모듈을 세션에 연결할 자리가 없다. W06~W07로 시작 상태(L2)를 먼저 맞춘다. 시작 상태가 다르면 이후 진행 비교가 모두 흔들리기 때문이다. P1은 의존 순서대로 간다. 사제와 경로(W08·W09)가 건설(W10)의 하위이고, 건설·전투가 희생·승패(W12·W13)의 하위다. P2는 규칙과 독립적인 부분이 많아 다른 작업자가 병행할 수 있다.

## 6. 기존 D01~D16과 새 D17~D24의 대응

| ID | 대상 | 2026-10-16 상태 | 이식 단위 |
| --- | --- | --- | --- |
| D01 | 시간 모델·24Hz/8틱 상한 | 소비자 목록 확정, 이행 미착수 | W04 |
| D02 | 공통 미션 프레임·정지 소유권 | **완료**(328행). `Postpone` 상위 호출자·tellAt/데모 남음 | W13·W17 |
| D03 | 참여·방향별 동맹 | **완료**(608행). 데모 로컬 AI·공통 `ai*` 키·`mission-start` 연결 남음 | W07·W17 |
| D04 | 승패·사건 잠금 | 미착수 | W13 |
| D05 | 튜토리얼 1 | 판정기 **완료**(266행). 간헐천 생성·Graph 연결 남음 | W09·W02 |
| D06 | 튜토리얼 2 부수효과 | 미착수 | W13 |
| D07 | HTML 조건·Tell 두 번 순회 | 미착수 | W13 |
| D08 | 명령 큐·비모달 Warning | 미착수 | W13 |
| D09 | 사제 HP1·회복 | 미착수. cpppj에 회복의 현재 SP 저장소 연결이 추가됨 | W08 |
| D10 | 총 피해·표적·투사체 | 미착수. cpppj에 현재 방향별 동맹 조회가 추가됨 | W11 |
| D11 | 경로·위치 | 미착수 | W09 |
| D12 | SP·보상·난수 | 미착수 | W05 |
| D13 | 배치·건설·사제 메뉴 | 미착수. cpppj에 받침 확정·로컬 간헐천이 실제 Session에 연결됨 | W10·W08 |
| D14 | 공유 월드·Graph·장부 | 장부 **완료**(52행). Graph·내용물·선택·감시 남음 | W02·W03 |
| D15 | CRT 정렬·신전/애니메이션 | 미착수 | W14·W12 |
| D16 | AI 초기화와 전략 | 초기화는 cpppj 실제 Session 연결 완료. mana 초안은 요구 에너지 2,352행으로 등록·검증돼 **보류 해제**. 전략은 계속 보류 | W07·W19 |
| **D17** | 단독 미션 로그인·Guide·Stage3 순서 | 신규. cpppj 로그인 상태 7~14·타이머·Guide 검증 완료, GUI 연결 전 | W07 |
| **D18** | Early Missions 3~6 단계 판정 | 신규. cpppj tutorial-later 원본 533개 입력 | W13·W18 |
| **D19** | 생산 Deck·Factory·금지표 | 신규. cpppj production-deck 248·refresh 242, 첫 객체 전 Factory 500 DWORD 예약 | W07·W10 |
| **D20** | 타입별 최초 후처리·Regular | 신규. 매립 580·훨리베이스 1,734·썬 바리케이드 2,333·총 각도 518·맨 오 워 풀 1,602 입력 검증. TYPE59·BaseFlyer 미복원 | W11·W19 |
| **D21** | AI 색 표·받침/종유석 프레임 | 신규. cpppj player-color 원본 216개 입력. 원본은 변경 소유자와 다른 섬도 갱신한다 | W07·W14 |
| **D22** | 데모 3개 | 신규. cpppj에서 데모 3개 모두 헤드리스 Load/Start 통과(20261016-12). GUI StartDemo는 미연결 | W17 |
| **D23** | 원본 Gump UI | 신규. cpppj Gump 계열 약 1만 행 | W15 |
| **D24** | 소리·음악 | 신규. cpppj 소리·음악 계열 약 6.6만 행 | W16 |

## 7. cpppj 진행을 따라가는 절차 (다른 AI가 cpppj를 계속 진행 중)

1. **기준 커밋:** 이 계획은 `53113b66`(cpppj `20261016-12`)까지 읽고 작성했고, W01에서 `ba8c3d83`(`20261016-19`)까지 검토를 이어 갔다. **현재 반영 커밋은 [`cpp-fixture-manifest.tsv`](../dotnetpj/tests/Shared/cpp-fixture-manifest.tsv)의 `reflected-commit` 행이 소유한다.** dotnetpj 단위를 시작할 때마다 저장소 루트에서 다음을 실행해 반영 이후의 변화를 본다. 연결 fixture가 어긋났으면 종료 코드가 1이다.

   ```sh
   python dotnetpj/tools/cpp_sync.py        # 연결 fixture 상태, 반영 이후 cpppj 커밋 목록, fixture 추가·수정·삭제 (읽기 전용)
   ```

   같은 내용을 손으로 볼 때는 `git log --oneline <반영 커밋>..HEAD -- cpppj docs/exe LEFT_JOBS.md`와 `git diff --stat <반영 커밋>..HEAD -- cpppj/tests/fixtures`다. 단위를 마칠 때 영향이 없거나 이식을 마친 변경을 검토했다면 `--update --reflect HEAD`로 반영 커밋을 올리고 인수인계에 적는다.

2. **HEAD만 쓴다.** 다른 AI의 커밋되지 않은 cpppj 변경은 근거로 쓰지 않는다. `cpppj/`·`LEFT_JOBS.md`는 수정하지 않는다. 필요한 요청은 8절에 적어 사용자에게 전달한다. 도구는 커밋되지 않은 fixture 변경이 있으면 경고한다.
3. **fixture 연결 규칙:** 기존 방식대로 `Netstorm.Core.Tests.csproj`(또는 Assets)에 `<None Include="../../../cpppj/tests/fixtures/<이름>-x86.tsv" Link="X86/<이름>-x86.tsv" .../>`로 연결한다. 사본은 만들지 않는다. **연결과 동시에** `python dotnetpj/tools/cpp_sync.py --update --add <이름>:Core`로 목록에 등록한다. 등록하지 않으면 `EveryLinkedFixtureIsRegistered`가 실패한다. 이미 연결한 fixture를 cpppj가 다시 만들면(관찰 ABI 교정 등) `LinkedFixturesMatchRecordedHashAndRows`가 실패해 재대조를 요구한다. 해당 C# 검사를 다시 대조한 뒤에만 `--update`로 목록을 갱신하며, 기대값을 C# 결과로 덮어쓰지 않는다. SHA-256은 CRLF를 LF로 바꾼 바이트로 계산해 체크아웃마다 같다.
4. **이식 대상의 조건:** CMake에 등록돼 빌드·검사되는 모듈, 원본 fixture 또는 보고서의 정적 근거, 보고서에 적힌 외부 경계가 모두 있어야 한다. cpppj 보고서가 "초안·미등록·미검증"으로 적은 모듈은 기다린다.
5. **cpppj 쪽 의미 변경 대응:** cpppj가 이미 이식한 모듈의 계약을 바로잡으면(예: 관찰기 해석 교정) 같은 단위를 다시 연다. 과거 통과 수를 지키려고 C# 기대값을 바꾸지 않는다.
6. **검토 기록: `53113b66` → `ba8c3d83`(2026-10-16, W01).** cpppj 커밋 5개(`20261016-15`~`19`)를 검토했다. 이미 연결한 fixture 15개는 하나도 바뀌지 않았고, 이식한 모듈의 cpppj 소스(Graph·SID·Hash·Surface·Bridge·참여·미션 프레임·Totalmade·튜토리얼 1)도 바뀌지 않아 C# 단위를 다시 열지 않았다. 바뀐 기존 소스는 둘뿐이다. `RawAssetRegular.cpp`는 거주 구역 좌표 1줄 정정이고, `SquidPostPop.cpp/.h`는 맨 오 워 분배 경로만 더했을 뿐 공통 postPop 집계 규칙은 그대로다(dotnetpj는 `postpop-x86.tsv`를 아직 연결하지 않았다). 새 fixture 6개는 아직 C#과 연결하지 않았고 담당 단위가 온 뒤 이식한다.

   | 새 fixture(행) | cpppj 내용 | 담당 단위 |
   | --- | --- | --- |
   | residence-coordinates 1,729 | 거주 구역(TYPE89) 이벤트 1의 난수 범위 정정. 둘째 범위의 하한은 `Y-32`(이전 "패치만 `X-32` 원본 결함" 서술은 복원 오류였다). 기존 1,976행은 보존 | W04(assetregular) |
   | buried-regular 385 | 오벨리스크(TYPE112) 사건 0·1, 2~7초 난수 | W11 |
   | fence-regular 1,373 | 썬 바리케이드 제어 0·1·2·기본(연결 수·네 방향 스냅샷·사건 2 예약) | W11 |
   | fence-marker-clear 260 | 기존 연결 표식 삭제(후보 태그·보관 next) | W11 |
   | fence-create 579 | 연결 표식 생성·포스 필드 생성(`fenceShield` 170, 내부 `fencemark` 90) | W11 |
   | flyer-postpop 1,422 | 맨 오 워(TYPE144) 최초 후처리·공통 비행 프로세스(타입 31) 생성. BaseFlyer 몸체는 아직 미복원이므로 W19 대기 | W11·W19 |

   결과적으로 실제 11개 미션 중 9개가 cpppj에서 400 Kernel 프레임을 통과한다(데모 1·3 제외). 이는 cpppj의 진행 사실이며 dotnetpj의 L2 이상 판정으로 합산하지 않는다.

7. **검토 기록: `ba8c3d83` → `b62774f2`(2026-10-16, W02 Unpop 조각).** cpppj 커밋 3개(`20261016-22`~`24`)를 검토했다. 연결 fixture는 하나도 바뀌지 않았고(18개 일치), 이식한 모듈의 cpppj 소스(SID 풀·생성·Unpop·Hash·Graph·참여·미션 프레임 등)도 실질 변경이 없다. 바뀐 것은 한국어 명칭 교정(알터→제단 등 주석·오류 메시지)과 새 모듈 `RawFlyerFrame`(+ `flyer-frame-x86.tsv` 745행, 담당 W19), 캠페인 세션 연결 일부(`RawCampaignSession.cpp`, 담당 W06·W07)다. `dudenotice-x86.tsv`는 첫 주석 줄의 명칭만 바뀌었고 데이터 행은 같다(미연결). **반영 커밋을 `b62774f2`로 올렸다.**

## 8. cpppj 작업자에게 전달할 요청 (이 문서에만 기록, cpppj 미수정)

1. **실제 Session 상태 덤프:** `RawCampaignSession`의 Load/Start 뒤(그리고 N프레임 뒤) 상태를 기계가 읽는 형식으로 내보내는 헤드리스 명령이 필요하다. 객체 SID·타입·프레임·소유자·flags·HP·좌표 비트, Player 9 원문, SP 비트, 동맹 81, Graph 표, Totalmade가 대상이다. 기존 `--dump-world`는 실제 Session 이전의 월드다. L2 판정에 쓴다.
2. **고정 입력 재생:** 프레임 시각과 명령(선택·이동·건설·우클릭) 열을 받아 매 프레임 상태 해시를 내는 재생 도구가 필요하다. L3 판정에 쓴다.
3. **fixture 열 이름:** 많은 TSV에 열 이름이 없다. C# 쪽은 같은 C++ 검사의 `Replay`를 읽어 열 의미를 맞춘다. 첫 주석 줄에 열 이름이 있으면 이식 오류가 줄어든다.
4. **연결 상태표:** 모듈별로 "독립 대조만 / 실제 Session 연결 / GUI 연결"을 한 표로 유지해 주면 이식 순서를 정확히 고를 수 있다.
5. **기존 전달 사항(2026-10-09, 아직 남음):** `cpppj/tests/GraphTests.cpp:105`의 Setup이 객체 열 `v[10]`·`v[11]`을 죽음·매장으로 읽는다. `graph-x86.tsv` 객체는 정수 11개(0~10)라 `v[11]`은 범위 밖이다.

## 9. 실행·검증 규칙

- **DESKTOP-HJOW:** `dotnet build`·`dotnet test`(헤드리스)만 실행한다. 클론 창·원본 게임은 실행하지 않는다. 10분 이상 걸릴 재생성·변이 검사는 인수인계로 넘긴다.
- 창 검사(`tools/clone_*_smoke.ps1`, `dotnet run --project dotnetpj/src/Netstorm.Game`)는 AGENTS.md의 면제 PC(vm-debian-codex·VM-W11-Codex·HJOW-Athlon)나 사용자 승인 뒤 다른 PC에서 한다([실행 확인 규칙](execution-confirmation.md)).
- 한 단위를 마칠 때 다음을 함께 기록한다: 변경 코드, 연결 fixture와 행 수, 새 검사, 세션·GUI·자연 플레이 중 어디까지 확인했는지, 남은 외부 경계, 반영한 cpppj 커밋. L1 통과를 L2~L5 완료로 합산하지 않는다.
- 커밋 메시지는 `YYYYMMDD-NN DN MSG`, 원격 push는 하지 않는다.

## 10. 남는 위험

- **이식 규모가 크다.** cpppj 원본 대응 소스는 약 6.2만 줄, fixture는 약 68만 행이다. P0을 건너뛰고 P1 일부만 옮기면 다시 라이브러리만 남는다. 4절 코어부터 만든다.
- **AI 전략은 cpppj에서도 미복원이다.** M3까지 적 행동은 근사이며 동일성 판정에서 제외 항목으로 적는다.
- **cpppj가 움직인다.** 7절 절차로 따라간다. cpppj가 같은 함수를 다시 해석하면 해당 단위를 다시 연다.
- **부동소수점 차이:** C#에는 x87 80비트 중간값이 없다. 3절 4번 방식으로 행마다 확인하고, 맞지 않는 입력은 저장 지점·연산 순서를 원본대로 바꿔 맞춘다.
- **플레이 중단 위험:** 교체 도중 1-1·1-2 회귀가 생길 수 있다. 단위마다 기존 `CampaignOneTests`·`CampaignTwoTests`와 GUI 스모크(허용 PC)를 유지한다.
