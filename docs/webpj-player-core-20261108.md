# webpj Player Core 장부·조회/선택·갱신 이식 — WB04-E3

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 세 번째 부분이다. cpppj 기준선 `1f00a25a`의 Player Core 세 단위(`o/RawPlayerCoreRecords`·`o/RawPlayerCoreData`·`o/RawPlayerCoreRefresh`)를 옮겼다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 세 단위는 기준선 뒤 cpppj 변경이 없다.

## 1. 옮긴 것

| 웹 모듈 | cpppj 근거 | 내용 |
| --- | --- | --- |
| `www/core/raw-player-core-records.js` | `o/RawPlayerCoreRecords.cpp/.h` | 9×16 BYTE Core 장부(기록 끝 2 BYTE padding). `initializePlayerCore`(0042ce70: DWORD0→DWORD4→WORD8을 0으로, 짧은 물리에서는 앞선 store만 남김), `initializePlayerCoreRecords`(004feac0: 기록마다 +4 Core 먼저, 그 뒤 ID DWORD 0), `writePlayerCoreMetadata`(0048f350: ID→복제된 Core, owner 주소 32비트 감김 보존, 장부 밖 선택 store 거부) |
| `www/core/raw-player-core-data.js` | `o/RawPlayerCoreData.cpp/.h` | 전역을 속성으로 직접 빌리는 `globals`(templateOwner·battleOwner·playerKnown·active·metamap·current·cached·players). `readPlayerCoreData`(0048f0d0: metamap 비영이고 현재≠캐시면 Player.cpp 0x55 진단 뒤 현재 포인터 재조회), `selectPlayerCoreData`(0048f3b0: owner 0이면 4/4/2 캐시 복사 후 현재 NULL·active 0, 비영이면 known 1 후 캐시 또는 실제 Player 뷰), `restorePlayerCoreData`(0048fb30: local/remote, 구독자 불일치 진단 0x224/0x21f 뒤 base 재조회, 저장 게임 BYTE·Chat 재초기화), 포인터 동일성 `samePointer` |
| `www/core/raw-player-core-refresh.js` | `o/RawPlayerCoreRefresh.cpp/.h` | 0044fc00: Core+6 초기화 → 기술 순회(후보마다 현재 Pool 재조회, bit 범위 진단 0x15a 뒤에도 같은 Core에 쓰기) → free 건너뛰는 Pool 순회로 다이스 level max(첫 반환 > max이고 dead 아니면 둘째 반환으로 대체) → Core+5 하한 갱신 |

JS에는 포인터가 없으므로 원문 포인터 동일성은 같은 `buffer`의 같은 `byteOffset`으로, NULL은 `null`로 옮겼고 `null` Core는 길이 0의 물리로 보아 선택 접근에서 `RangeError`로 거부한다.

## 2. 검증

원본 x86 기대값은 `cpppj/tests/fixtures`를 직접 읽는다(복사 없음).

| 검사 | fixture | cpppj 대응 |
| --- | --- | --- |
| `player-core-records.test.mjs` | `player-core-records-x86` 192행 = Core 186 + 색 표 감사 6 | PlayerCoreRecords_Replays…, ShortPhysicalViewsPreserveOrderedPrefix, MetadataPreservesPaddingAndCopiesInputValues |
| `player-core-data.test.mjs` | `player-core-data-x86` 804행(normal 730·fault 72·stop 2): Player 1620 BYTE·캐시·외부 16 BYTE·trace 전체 | PlayerCoreData_ReplaysIndependentPe804, ReturnedViewAliases…, Rejects…AfterEarlierStores + 복원 소스 계약 1장면 |
| `player-core-refresh.test.mjs` | `player-core-refresh-x86` 지원 618행(+제외 24), `player-core-refresh-guard-x86` 지원 16행(+제외 4) | PlayerCoreRefresh_Replays…, ShortCore…, RequiredBitDiagnostic…, ReplaysGuard16… |

최초 실패와 교정: 구현 직후 큰 재생(804·618·186행)이 처음부터 통과했다. 실패한 것은 검사 쪽 두 가지(32비트 감김 입력의 내 기대 오류, `null` Core를 길이 0으로 처리하지 않아 TypeError가 난 구현 누락 한 곳)였다.

## 3. 경계

- **복원(`restorePlayerCoreData`)은 cpppj에도 직접 원본 관측이 없다**(`RestorePlayerCoreData`를 호출하는 cpppj 검사 없음). 웹은 소스를 그대로 옮기고 소스 계약 검사 1장면만 두었으며, 원본 대조는 실제 Client 단위(WB05~06)에서 확인한다.
- 제외 행: Core 갱신의 논리 span 쓰기 정지 24행은 store 커밋 시점의 별도 관측이고, guard의 Unicorn 부분 store 4행은 host 단일 store 거부와 같다고 하지 않는다(cpppj와 같은 판정).
- 색 표 6관측은 Core 장부가 아닌 owner 색(`43b540`)의 귀속 감사이며 이 단위의 제품이 아니다.
- 실제 Client의 Player 레코드(`ClientPlayerRecords`)·Chat 재초기화·Session 연결은 WB05~06에서 이 모듈들에 `globals`/records/hook을 공급한다.

## 4. 다음 시작점

Player 묶음의 나머지: `RawPlayerIdentity`(111줄)·`RawPlayerParticipation`(154줄)·`RawPlayerTechnology`·`RawPlayerInitialization`·`RawPlayerColor.h`(RawBridgeConnect 선행)·알림 계열, 그리고 WB04-E의 `SquidPostPop` 처리기·`SquidDisplay`·`SquidFrame`.
