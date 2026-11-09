# webpj 생산 내용물 동기화(RawProductionSync) 이식 — WB04-E216

2026-11-10, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 "자원·생산·건설 명령" 묶음의 일부다. 생산 Deck(E215)과 현재 기술 부모의 CFF_TELEPORT 내용물을 원본 43df80 순서로 동기화한다.

- cpppj 기준선 `1f00a25a`. 소스(`RawProductionSync.cpp/.h`)·시험(`ProductionSyncTests`)·fixture(`production-sync-x86.tsv`)·선행 모듈(`RawContainedFinder`·`RawProductionDeck`) 모두 기준선 뒤 변경이 없다(`git diff --numstat 1f00a25a HEAD` 루트에서 확인).
- 근거 커밋: 소스 `674e3202`, fixture `5d817c15`.
- 원본 fixture: `production-sync-x86.tsv` 98행(10.78 전용 관측). 입력은 현재 기술 부모의 내용물 목록(`kind:flags`, BYTE seen 의 255/256/257개 중복 포함)·Deck 항목(`kind:power`)·TYPE 프로필(신전 flags2 비트 4)·신전 유무·fullDeck·기술 부모 변경·기존 종속 조회 반환, 출력은 호출 순서와 인자 전체(Deck 조회 `K`·기술 부모 `T`·조회 `Q`·취소 `X`·삭제 `D`·생성 `C`·진단 `A`)다.
- 공유 장치(모두 기존 모듈 재사용): E56 `raw-contained-finder.js`(`RawContainedFinder`·`ContainedFinderState`), E215 `raw-production-deck.js`(Deck 크기/항목 상수).

## 1. 옮긴 것

`www/core/raw-production-sync.js`(DOM 없음, 10.78 전용)

- `RawProductionSync(pool, types, finderState, state, hooks)`: 10.78 풀과 188칸 TYPE 표를 확인한다. `state` 는 현재 전역 getter `{ localOwner(), fullDeck() }`(00540c70·0054db14), `hooks` 는 `deck`(현재 templateOwner 의 Deck)·`technology`(현재 기술 부모, 순회 전/후 각각 조회)·`temple`(Player+7c 포인터 비0)·`cancelCursor`(446e70)·`destroy`(내용물 가상 destroy(0))·`findKind`(4acac0, **SID 가 아닌 저장 kind WORD**)·`create`(4afa70)·`assertion`이다. 선택된 하위가 없으면 그 위치에서 오류다.
- `synchronize()` — 43df80
  1. Deck 을 얻고 기술 부모를 조회해 `RawContainedFinder` 로 종속을 순회한다. 원본처럼 다음 후보를 먼저 찾은 뒤 이전 후보의 현재 kind(+18 WORD)/flags(+20 WORD)를 읽는다. kind 가 TYPE 표(188)나 seen 표(255) 밖이면 거부한다.
  2. flags 비트 8 이면 BYTE seen 카운터를 올린다(삭제되는 기존 내용물도 먼저 올리고 255 초과는 BYTE 로 감긴다).
  3. kind 의 TYPE flags2 비트 4(신전 상품)이면 로컬 owner 의 신전 포인터가 없을 때 제거한다. 아니고 flags 비트 8 이면 Deck 첫 type BYTE 일치 항목의 signed power 가 없거나 1 미만일 때 fullDeck 이 0 인 경우만 제거한다.
  4. 제거 대상이 선택 중(flags 비트 0x100)이면 먼저 취소를 부르고 destroy(form, 0)를 부른다.
  5. 순회 뒤 기술 부모를 다시 조회하고 Deck 개수를 매번 다시 읽으며 seen 이 0 인 항목마다, 기존 종속 검색(mask 8, kind, 0)에 결과가 있으면 진단 `!me.findContainedType( CFF_TELEPORT, type )` 뒤에도 계속해 `create(부모, kind, 0, 8, 16, 3)` 을 부른다. 새 생성 뒤 seen 은 올리지 않는다.
  6. Deck 개수는 signed 로 읽어 음수면 빈 순회이고 66 초과는 호스트 물리 한계로 거부한다. 기술 부모 SID 는 풀 범위/WORD 범위를 확인한 뒤에만 쓰며 SID 0 도 실제 예약 슬롯을 읽는다.

## 2. 검증

`tests/production-sync.test.mjs`(4건)

- 원본 관측 재생: 98행 전부. 행마다 실제 `SidPool`(24000, 서버)에 기술 부모(SID 50)와 내용물(SID 100+i, TYPE 6, next 체인)을 raw 로 준비하고, 호출 순서/인자 전체와 풀 바이트 불변을 대조한다.
- cpppj 계약 이식 2건: 장기 FullDeck 전역을 Deck getter 효과 뒤 현재 값(0x80000000·0xffffffff·0)으로 읽고 비영을 보존함, 기술 부모 SID 0 이 예약 슬롯을 읽고 빈 Deck 에서 조회/생성하지 않음.
- 웹 계약 1건: 선택된 하위 누락·판본/타입 표 크기·기술 부모의 풀 밖/WORD 밖·내용물 kind 범위 밖·Deck 66 초과·Deck type BYTE 255 의 거부, 음수 개수 빈 순회.

최초 실패와 교정: 모듈은 처음부터 98행과 cpppj 계약 2건이 통과했다. 첫 작성한 웹 계약 시험이 실패했는데 시험 결함이었다(같은 장면에서 앞선 시도의 호출 기록이 쌓여 있는 채로 음수 개수 실행을 비교). 음수 개수를 새 장면으로 옮겨 고쳤다. 모듈 사본을 바꿔 돌려 비교가 작동하는지 확인했다. 확인 뒤 모두 원본으로 되돌렸다(cmp 일치).

- 실패한 변형: power 조건 `<1`→`<0`, seen 증가 조건(flags 비트 8) 제거, 신전 판정 부정 제거, 취소 비트 0x100→0x200, 생성 flags 8→4(각 1건).

## 3. 경계와 남은 것

- 실제 월드에 연결하지 않았다. 호출 지점(Factory 지식 선택·State 전환의 생산 갱신)과 실제 Deck/Player+7c/선택 취소/내용물 생성 하위는 순서4~5·6 몫이다(PORT_MAP session 0). cpppj 의 `ClientProductionSyncTests` 는 client `ClientProductionSync` 단위를 옮길 때 대조한다.

## 4. 다음 시작점

WB04-E217 은 3.1절에서 고른다. 같은 묶음의 `RawFactoryResearch`(지식 선택·서버 연구 수신·생산 내용물 통지; `RegisterFactoryResearchCommand` 는 `RawPacketDispatch` 뒤)·`RawFactoryUpgrade`·`RawFactoryUpgradeCommand`(모두 기준선 뒤 변경 없음)를 권한다.
