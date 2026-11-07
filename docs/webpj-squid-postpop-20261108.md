# webpj 공통 후처리 처리기(SquidPostPop) 이식 — WB04-E41

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 마흔한 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/SquidPostPop`(+`SquidPostPopPatch`; 10.78)을 옮겼다. [E24](webpj-squid-postpop-parent-20261108.md)의 공통 postPop 부모(004b0d30)와 [E2](webpj-squid-owner-20261108.md)의 장부 구조, [E40](webpj-raw-graph-20261108.md)의 RawGraph를 한 객체로 묶어 Pop이 부르는 `postPopVirtual`/`activationDepth`/`validateSpace`를 제공한다. 이로써 E40이 남긴 Pop 연동 fixture(rawgraph·regiongraph·graphremove·graphrecovery)를 모두 재생했다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

`www/core/squid-postpop.js` — `SquidPostPop`, `bridgePrefix`, `POSTPOP_VTABLE`

- **공통 몸체** `postPop(sid, flags)`(=`postPopBase`): `runSquidPostPopParent`에 현재 TYPE·Graph(영역 `invalidateRegion`/`add`/noGraph 리셋 `setNumber`)·0x80b 진단·비용·공급원(`registerManaProvider`)·owner 공장·공장 목록·AI 통지·로컬 owner·Totalmade 통계·선택 재검사를 연결한다. 효과마다 값을 다시 읽는다.
- **비용** `totalCost(type)`: 로딩 정수 비용표 `trunc(cost + type*23) - type*23`(단정도 cost, 비유한/32비트 밖 거부)에 현재 전역 비용을 더해 32비트로 감는다. `bindLoadedCosts`로 최초 로딩 표를 빌리면 현재 TYPE의 비용 변경과 분리된다.
- **가상 분배** `postPopVirtual(sid, flags)`/`#dispatchPostPop`: 자기 몸체에서 공통 장부를 한 번 부르는 종류(벌룬·비행·Player·buried·Dude·다이스·Rune·사제·크랩·직접 Carrier/Damageable·자산·Factory)를 원문 순서로 먼저 고르고, 접두만 가진 종류(주문 효과·다리·섬 받침·noIsland 표면·보호막·Teleport 0x5070c0)는 접두를 실행한 뒤 공통 몸체로 내려간다. 지원하지 않는 VT는 `SquidPop.supports`로 확인해 거부한다. 파생 몸체는 `setBody(kind, body)`로 같은 풀에만 등록한다(`handles`/`validate`/`postPop`, 접두형 `prefix`, Carrier `damageablePostPop`). 섬/표면/Teleport 접두는 `setIslandPrefix`/`setSurfacePrefix`/`setTeleportPrefix`이며 flags 비트 1에서만 호출하고 실행 중 자기 해제해도 캡처를 유지한다.
- **다리 접두** `bridgePrefix(pool, sid, flags, connector)`(00422150): flags 비트 1은 extra 비트 2를 먼저 쓰고(연결 장치 요구·수명 검사보다 앞), 비트 1이거나 (비트 4이고 abstract가 아님)이면 연결 효과를 요청한다.
- **사전 검사** `validate`/`validateBase`: 독립 명시 호출과 파생 몸체 직접 호출용 무효과 검사(슬롯 상태·다리/섬/표면 장치 미연결·지원 VT·등록 몸체의 자기 검사·Graph `validatePostPop`·소유자 범위·AI/배치/비용/목록 손상). 실제 10.78 가상 부모는 가상 몸체 앞에 전체 사전 검사를 끼우지 않는다. `activate`는 독립 기존 진입(검사→깊이 증가→분배→깊이 복귀 확인)이다.
- 장치 연결: `bindCurrentTypes`·`bindLoadedCosts`·`bindAiNotice`·`bindManaDiagnostics`·`bindSelectionRecheck`·`bindLocalOwner`·`validateSpace`·`activationDepth`(state.depth를 읽고 쓰는 소유자).

## 2. 검증 (`tests/squid-postpop.test.mjs`, 15건)

원본 10.78 관측(`cpppj/tests/fixtures`를 직접 읽음, original1037·originalCD 행 제외)을 `SquidPop`·`SquidUnpop`·`RawGraph`·`SquidPostPop`이 이어지는 실제 호출 연쇄로 재생한다.

| fixture | 재생 | 확인 |
| --- | --- | --- |
| postpop | PostPop 384·Pop 72·Unpop 36 (State 192) | 비용·알림·깊이·Totalmade 세 표·공급원/공장/owner 목록 전체·전체 슬롯 |
| bridgepostpop | 80 | 다리 접두 flags/extra/연결 요청 순서 |
| rawgraph | 768 (PostPop 320·Add 256·Flood 128·Pop 64) | 7개 슬롯·기록/스택·해시 4단계·spot·풀 Adler·장부 3표 |
| regiongraph | 512 (Region 160·PostPop 160·Pop 96·Add 96) | 일반/다리/섬 Pop의 영역 통지와 Add |
| regiongraph(checking) | 512 | 같은 관측을 실제 checking 경로에서 재생. 진단은 `graph-live-checking`의 원문 진단 목록에 있는 것만 허용 |
| graphremove | 386 (Detach 128·Unpop 256·Release 2) | 삭제 일반 finder/분할 정책, 다리·섬 Unpop·Release |
| graphrecovery | 64 | 상위 호출 안의 전체 재구성(FullPool)·이후 flood/감소/지도/통계 |

cpppj 계약 테스트 중 표시(SquidDisplay) 없이 성립하는 것을 옮겼다: 최초 등록 비용/통계와 공급원 중복, 미복원 효과·손상 목록 실패가 앞선 공간 쓰기를 되돌리지 않음, 다리 접두 7종, Teleport·섬·표면 접두 분배/자기 해제/명시 중단/늦은 공통 실패, 정수 비용표 공식(신규), 파생 몸체 등록·이중 깊이 방지·풀 불일치 거부. 비용표 수식 검사는 float32 반올림(2147480000.5 → 2147480064)을 기대값에 반영했다.

최초 실패와 교정: 모듈은 첫 실행에서 대용량 fixture 5개 전체를 통과했다. 실패한 것은 모두 **테스트 쪽**이었다: 해시 4단계 직렬화의 스프레드 인자가 스택을 넘김(청크 이어 붙이기로 교체), 계약 장면이 `factory.Create(74,2)`가 하던 타입 바이트를 쓰지 않아 슬롯이 타입 0으로 읽힘(타입 74를 직접 기록), 계약 장면의 비용 입력(원문 0)과 달리 공통 타입 표의 비용 기본값이 1.25이던 것.

## 3. 경계

- 표시 dirty 열(`postpop`의 start+9·10, Pop 연동 fixture의 14·15열)은 `SquidDisplay`/`Renderer`의 DirtyRegions가 아직 이식되지 않아 대조하지 않는다. 해시 단계용 프레임 크기는 이 fixture의 SHP 헤더 `cellWidth/Height`가 0이라 폴백(1,1)과 같은 단계를 만든다.
- 파생 몸체(자산·Factory·Priest·Rune·Dais·Dude·Player·buried·비행·벌룬·크랩·Carrier·주문 효과·보호막)와 섬/표면/Teleport 접두의 본체는 이 단위의 범위가 아니다. 등록 인터페이스와 분배 순서만 옮겼고 각 종류 단위에서 `setBody`로 연결한다.
- `SquidPostPop_TypeCost_LoadsFloatAndRejectsString`은 `RiftTypeTable` 로더 경로의 테스트라 이 단위에서 옮기지 않았다(웹 타입 로더 쪽에서 `cost`를 읽는다).
- CD 10.72(타입 171·graph 28·frame 1바이트, 호환 Validate 순서)는 범위 밖이다.

## 4. 다음 시작점

`RawBridgeConnect`(다리 이벤트 이웃: `neighbor-x86`의 Event 행과 `fence-neighbors`)와 그 다리/섬/표면 접두 본체, Player 나머지(`RawPlayerWorld`·`RawPlayerTechnology`·`RawPlayerRegular`·`RawPlayerTemplateState`·`RawPlayerGeyserBounds`·`RawPlayerPlacementAnchor`), 종류별 PostPop, `SquidDisplay`/`SquidFrame`이다. 그 뒤 LEFT_JOBS WB05(월드 조립과 미션 시작).
