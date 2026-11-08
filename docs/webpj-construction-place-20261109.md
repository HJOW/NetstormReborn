# webpj 건설 배치(RawConstructionPlace) 이식 — WB04-E138

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백서른여덟 번째 부분이다. 건설 배치 실행(Construction.cpp `00442c80`)과 비용 부족 처리(`00442b50`)·환불(`00442a40`)·취소 통지(`00442af0`)·SP 조회(`0040ef30`)·SP 가산(`0041dff0`)을 옮긴 `o/RawConstructionPlace`(cpppj 기준선 `1f00a25a`, 기준선 뒤 변경 없음)이다. 로컬 배치(`004433b0`)와 서버 확정(`00444590`)이 미리 확보한 SID 목록을 받아 타입의 조각마다 abstract 비트·HP·프레임·단어·소유자를 정하고 Pop 을 요청한다. 선행은 E136 `decodeCanonType`·기존 `SquidReward`/`SidPool`·E134 통지 어댑터(`place.place`/`actualPlace`/`refund` 계약)다.

## 1. 옮긴 것

`www/core/raw-construction-place.js`

- `ConstructionPlaceState` — 로컬 플레이어·server·authority·chargeBlocked·battleShown·noIsland/bridge/nugget 타입·다리 수명·패턴 타입 8개·`playerFlags`(Player +0x54)/`playerNames`(+0xc) 값형 입력과 같은 이름의 `shared*` 함수. typed `place` 는 값형 필드를, 실제 wire 경로와 SP 조회·환불·비용 부족 처리는 `*Now()` 접근자를 소비 위치마다 읽는다(원본 C++ 과 같은 구분).
- `RawConstructionPlace(pool, types, frames, state, reward, hooks)` — 10.78 `SidPool`·같은 풀의 `SquidReward`·타입 표 188칸·필수 경계 13개(`take`·`notifySurface`·`setOwner`·`pop`·`sendMoney`·`storedSp`·`addLocalSp`·`addPlayerSp`·`format`·`tellOthers`·`tellPlayer`·`tell`·`sendReject`)를 생성 때 확인한다. `assertion` 은 실제 wire 경로에서 쓸 때 확인한다.
  - `place(request)` (00442c80): 타입 번호 범위·서버 플레이어(9)·SID 개수(디코더 사본으로 끝까지 센 조각 수와 같아야 함)를 효과 전에 거부한 뒤 비용 판정(다른 플레이어의 배치만, 모자라면 `chargeShortfall` 아니면 차감) → 조각마다 [서버가 아니고 번호가 5 미만/서버 영역이면 `take` → void 확인 → abstract 비트 → HP(방금 쓴 abstract 비트로 판정, abstract 인자가 0 이 아니면 0) → 프레임 DWORD → +0xc 단어(noIsland 시작 칸 0x5d4, 다리 타입이면 표면 알림) → 다리 품질 프레임(금 간 0x20·단단한 0x40)·수명 비트(3~6)·표면 알림 → 소유자 → Pop(abstract 면 2, 아니면 0x41)]을 한다.
  - `actualPlace(request, declaredCount, sidReader)`: 같은 순서의 실제 wire 경로. 선행 전수 검사 없이 현재 SID WORD 를 읽고 예약/free 슬롯도 물리 범위 안이면 접근한다. 진단 4종(player 9·`isVoid()`·다리 품질 프레임 없음 0x16a·`i == sidCount` 0x2f2)은 `hooks.assertion` 에 보고하고 복귀하면 같은 순서로 계속한다(장치가 없으면 거부).
  - `chargeShortfall`(표시 비트를 먼저 읽고 켠 뒤 권한 쪽은 안내+reject, 비권한 쪽은 차감+로컬 안내)·`refund`(비용 정수 절삭·너겟 제외·SP 통지)·`reject`(환불 뒤 취소 통지)·`money`(1~8 번만 저장소 읽기, 전투 표시 중 로컬은 예외)·`addMoney`(로컬이면 로컬 가산) — 모두 원문 순서를 그대로 둔다.
  - `bindPlayerRecords(players)`: 실제 9×0xb4 Player 바이트를 연결하면 표시 비트와 이름(+0xc..+0x2c, NUL 없으면 거부)을 매 호출 직접 읽고 쓴다.
- `makeConstructionPlaceHooks(pool, factory, owner, reward, store, state, hooks)` — 수신(`factory.take`)·소유자 지정(`owner.set`)·SP 읽기(`store.get`)/가산(`reward.addSp`)을 같은 풀의 실제 모듈로 바꾸고 Pop·표면 알림·문구·메시지 경계는 그대로 둔다(풀 불일치·저장소 없음 거부).

`www/assets/territory.js` — `CanonDecoder.clone()` 추가(생성자의 첫 진행과 진단을 다시 하지 않고 현재 상태를 복사; C++ 값 복사와 같음). 조각 수를 셀 때 원 반복자를 건드리지 않기 위해 쓴다.

## 2. 검증

`tests/construction-place.test.mjs`(6건):

1. `constructionplace-x86.tsv` 의 `originals` 1778행(`originalCD`·`original1037` 은 다른 판본이라 제외) — Place 1246·Charge 292·Refund 144·Reject 24·Money 48·Add 24 에서 하위 효과 순서(T/N/O/P/M/G/L/A/F/X/Y/D), 조각 슬롯 전체 바이트, Player 표시 필드·잔액 비트가 모두 같다.
2. 풀 불일치·타입 표 171칸·필수 경계 13개 각각의 누락 거부, 서버 번호·타입 범위 밖·SID 0개/2개 거부(효과 없음), void 가 아닌 조각 거부.
3. 실제 wire 경로: 위 Place 관측(서버 번호·다리 품질 프레임 없음 입력 제외)을 `actualPlace` 로 돌린 효과 순서와 슬롯이 typed 경로와 같다(1000건 넘게 비교).
4. 진단 4종이 Client 진단 장치에 보고되고 복귀하면 계속하며(서버 번호 → 그대로 Pop, void 아님 → `0x282` 뒤 계속 + 개수 불일치 `0x2f2`, 다리 품질 프레임 없음 → `0x16a` 뒤 프레임 −1 기록), 진단 장치/SID 읽기 함수가 없으면 거부한다.
5. 실제 Player 기록 연결: 이름 `Bob` 을 인자로 안내하고 표시 비트는 `0x1001 → 0x1401` 로 다른 비트를 보존한 채 켜지며, 이미 보고된 뒤에는 안내하지 않고, 이름 NUL 누락·Player 번호 9 는 거부한다.
6. 어댑터: 수신·소유자·SP 읽기/가산이 해당 모듈로 가고 풀이 다르거나 저장소가 없으면 거부하며 Pop·표면 알림·메시지 경계는 보존한다.

최초 실패와 교정: 새 모듈은 1778행 fixture 를 첫 실행에 통과했다. 시험의 조각 슬롯이 없는 행(조각 0개)을 `-` 로 표기하는 부분만 한 번 고쳤다.

## 3. 경계와 남은 것

- 실제 예측→확정→통지→정리→배치 전체를 잇는 cpppj 통합 시험(`ConstructionClear_ReplacesLocalPredictionWithConfirmedPieces`, 실제 `SquidFactory`·`SquidOwner`·확정)은 `RawConstructionConfirm` 이식 뒤 옮긴다. 어댑터는 대역으로 이름 계약만 시험했다.
- 10.72(CD) 경로(HP WORD·프레임 BYTE·비용 처리 없음)와 10.37 은 범위 밖이다.

## 4. 다음 시작점

`RawConstructionConfirm`·`Request`·`Local`·`Command`·`Data`·`Builder`(선행 `RawPriestPlacementGeometry`), `RawManaBoltArrival`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
