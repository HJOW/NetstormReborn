# webpj 생산 Deck(RawProductionDeck)·기술 표시·SP 통지 바인딩 이식 — WB04-E215

2026-11-10, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 "자원·생산·건설 명령" 묶음의 일부다. 현재 Player 가 소유하는 원본 0x160 BYTE 생산 Deck(66개의 5 BYTE 항목)과 그 수명, 기술 표시(004929e0), Player 링크 정리 부모(0048fcd0)의 정상 복귀 증빙, 그리고 Player 내용물 통지의 SP 접두가 쓰는 네 전역의 바인딩을 옮겼다.

- cpppj 기준선 `1f00a25a`. 소스(`RawProductionDeck.cpp/.h`·`RawSpStoreNoticeBindings.cpp/.h`)·시험(`ProductionDeckTests`·`ProductionDeckReleaseTests`)·fixture(`production-deck-x86.tsv`)·선행 모듈(`RawPlayerContentNotice`·`SpStore`) 모두 기준선 뒤 변경이 없다(`git diff --numstat 1f00a25a HEAD` 루트에서 확인).
- 근거 커밋: 소스 `b37bd01a`, fixture `05d7cb57`.
- 원본 fixture: `production-deck-x86.tsv` 248행(10.78 전용 관측). 입력은 모드·templateOwner·type·chance·개수·채움 바이트·Player 포인터 유무·PlayerKnown·battleOwner, 출력은 반환·352 BYTE Deck 전체·Player 9기록 전체(1620 BYTE)·요청한 하위 경계 순서(할당 바이트 `N`, 진단 `A:줄`, 종속 부착 `C:…`)다.
- 공유 장치(모두 기존 모듈 재사용): E105 `raw-deck-draw.js`(`RawDeckDraw.initialize/append`), E8 `raw-player-links-cleanup.js`(`cleanupPlayerLinks`), E55 `raw-player-world.js`, E56 `raw-carrier-pre-destroy.js`·`raw-contained-finder.js`, D5a `contained-create.js`, D5b `sp-store.js`·`content-form-host.js`·`raw-player-content-notice.js`.

## 1. 옮긴 것

`www/core/raw-production-deck.js`(DOM 없음, 10.78 전용)

- `RawProductionDeckStorage(firstToken)`: 실제 352 BYTE 블록과 opaque 주소(토큰)만 장기 소유한다. `get(players, owner, fill)`(0044f700)은 현재 Player+68 을 먼저 읽어 있으면 그대로 돌려주고(블록이 없으면 오류), 없을 때만 새 블록을 할당해 `fill` 입력을 보존하고 다섯 DWORD 만 0 으로 초기화한 뒤 Player+68 에 부착한다. `bytes`·`release`(0 은 `free(NULL)`처럼 무효과, 모르는/중복 토큰은 `RangeError`)·`has`·`isEmpty`·`clear`. 다음 토큰은 되감지 않는다.
- `RawProductionDeck(player, fill, assertion, firstToken, storage)`: PATCH Player 9기록을 확인하고 Client 의 장기 저장소를 빌릴 수 있다.
  - 조회·초기화: `get`·`bytes`·`release`·`initialize`(0044f200, 다섯 DWORD)·`clearEntries`(0044f230, 개수/가중합/남은 수만, seed/open 유지)·`contains`(0044f970, 현재 templateOwner 의 Deck 에서 type BYTE 비교, 첫 일치 뒤 중단).
  - 변경: `append`(0044f290, 중복 진단 → 용량 진단 → 5 BYTE 초기화·가중합 DWORD·개수)·`activate`(0044f3d0, 최초 일치 항목의 signed power 가 음수이면 0, increment 비영이면 +1, 없으면 chance 1/power 1 추가)·`compact`(0044f430, 제외할 항목도 먼저 앞으로 복사하고 signed power 음수만 삭제로 센다).
  - 개수는 signed 로 유지한다(음수는 빈 순회). 호스트 블록을 넘는 양수 개수(66 초과)만 거부한다.
  - 수명: `dispose()` 가 C++ 소멸자다. 증빙을 소비하지 않았다면 이 저장소가 소유한 Player+68 포인터만 0 으로 만들고(다른 모듈이 교체한 값은 보존) 블록을 모두 반납한다.
  - Player 링크 정리: `cleanupPlayerLinksForState(players)` 가 실제 부모 `cleanupPlayerLinks` 를 이 Deck 의 `release` 로 실행하고 `PlayerLinksPass` 를 발급한다. `returnPlayerLinksForState(pass)` 는 같은 Deck 이 발급한 소비 전 증빙만 받는다. 진행 중인 링크 호출이 있거나(`진행 중`), 소유 블록이 남았거나, Player 9기록의 세 DWORD(+0x9c·+8·+0x68)가 0 이 아니면 거부한다. 소비하면 이 Deck 은 닫혀 이후 조회·반납이 모두 거부된다. 증빙은 모듈 내부 열쇠(Symbol)가 있어야 만들 수 있고 수명 표식을 약한 참조로 가져 만료된 Deck 의 증빙을 구별한다.
- `RawIndicateTechnology(player, carrier, create)`: 같은 풀의 Player·Carrier·생성 사슬을 확인한다. `indicate(type)`(004929e0)은 현재 기술 부모(SID 범위 확인)에서 `carrier.findContained(부모, 4, type, 0)` 로 종속을 찾아 **저장 kind WORD(+18, 2 BYTE)** 를 읽고, 0 이면(일치하는 종속이 없거나 kind 가 0) 부모를 다시 읽어 `create(부모, type, 0, 4, 0x10, 0)` 으로 진행한다. 타입 assert 는 임의로 추가하지 않는다.

`www/core/raw-sp-store-notice-bindings.js`: `bindSpStoreNotice(store)` — 첫 바인딩에서 추적 offset 미연결을 거부하고, `cursorA`(실제 offset)·`cursorB`(추적 offset 의 signed→DWORD)·`firstSpPool`(첫 분산 배열 확보 여부 1/0)·`firstPermutation`(순열 첫 signed WORD)을 호출마다 같은 저장소에서 다시 읽는 getter 로 돌려준다.

## 2. 검증

`tests/production-deck.test.mjs`(12건)

- 원본 관측 재생: 248행 전부. 행마다 새 월드(실제 보호 188 TYPE·SidPool·`SquidFactory`·`RawPlayerWorld` 9기록·`ContentFormHost`·`RawPlayerContentNotice`·`ScrambledSpStore`+`bindSpStoreNotice`·`RawContainedCreate`·Carrier 조건 순회)를 조립한다. Indicate 모드는 실제 할당으로 Technology SID 50 과 종속 SID 60 을 준비하고, 호스트 TYPE 범위(188 이상)의 생성 시도는 명시 예외로 확인한다.
- cpppj 계약 이식 10건: 현재 templateOwner 와 두 owner 의 블록 수명·소멸자의 Player 포인터 보존, 누락 할당/모르는 포인터/66 초과 물리 범위, 링크 증빙 4건(36회 getter 조회·세 DWORD 밖 보존, 실패 접두 보존·미방문 필드 보존, 다른 모듈/만료 증빙 거부, 부모 복귀 전 소비 금지), 개별 반납 3건(한 블록만·NULL 무효과·모르는/중복 토큰·원문 부모가 같은 owner 를 호출).
- 웹 계약 2건: 저장소의 첫 주소/Player 범위/할당 입력 거부와 장기 저장소 대여, kind WORD 의 높은 BYTE 만 있어도 이미 표시된 것으로 봄.

최초 실패와 교정: 모듈은 처음부터 248행과 계약 11건이 모두 통과했다. 비교가 실제로 작동하는지 모듈 사본을 바꿔 돌려 확인했다. 확인 뒤 모두 원본으로 되돌렸다(cmp 일치).

- 실패한 변형: `activate` 의 +1→+2(1건), `compact` 삭제 조건 `<0`→`<=0`(1건), `contains` 의 순회 한계를 개수→66(2건), `clearEntries` 의 남은 수 지우기 제거(2건).
- 처음 통과한 변형: kind WORD 폭 2→1. fixture 의 kind 값이 모두 한 BYTE 안이라 구별하지 못했다. 그래서 높은 BYTE 만 가진 kind(0x0100)에서 생성하지 않아야 한다는 계약을 추가했다. 첫 작성은 종속의 마스크 비트(+20 & 4)를 주지 않아 `findContained` 가 못 찾고 생성으로 진행해 실패했다(시험 입력 결함). 마스크를 4 로 고친 뒤 원본 통과·변형 실패를 확인했다.
- 동치 변형: 증빙의 `consumed` 표식 제거. 같은 Deck 은 소비 뒤 이미 닫혀 `requireLive` 로 거부되고 다른 Deck 은 `source` 로 거부되므로 결과가 같다. 이중 방어로 유지한다.
- cpppj 시험 `ProductionDeck_ReplaysNative248` 등은 같은 PC 의 통합 시험 실행 파일(2026-11-08 빌드, 게임 실행 아님)로 확인하지 않았다. 웹은 위 fixture 를 직접 재생한다.

## 3. 경계와 남은 것

- 웹 범위 밖(client 단위)으로 남긴 cpppj 시험 3건: `ProductionDeck_ReplaysFactoryRefreshNative242`(`production-refresh-x86.tsv` 242행)·`ProductionDeck_CurrentRestrictionsAndFactoriesAreShared`·`ProductionDeck_SharedFullDeckReadsAfterModeDiagnostic`. 모두 `RawCampaignProductionBindings`(재계산·Factory 상품 추가)와 `RawTotalMade`·`ClientGameplayGlobals` 를 쓰므로 그 client 모듈을 옮길 때 대조한다.
- 실제 월드에 연결하지 않았다. 장기 Deck 저장소의 Client 소유와 Session 교체 때의 `dispose`/증빙 소비 순서는 순서4~5 몫이다(PORT_MAP session 0).

## 4. 다음 시작점

WB04-E216 은 3.1절에서 고른다. 같은 묶음의 `RawProductionSync`·`RawFactoryResearch`·`RawFactoryUpgrade`·`RawFactoryUpgradeCommand`(모두 기준선 뒤 변경 없음)를 권한다. `RawGeyserMined` 의 남은 `RegisterGeyserMined` 는 `RawPacketDispatch` 이식 뒤로 미룬다. `RawFrameChoose` 는 WB03-X 가 `chooseTerrainFrame` 으로 이미 옮겼으므로 같은 의미의 시험을 보강하며 `port` 로 올릴 수 있다.
