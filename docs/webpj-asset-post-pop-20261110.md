# webpj 자산 후처리 분배 본체 이식(RawAssetPostPop) — WB04-E254

2026-11-10, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 단계 3-C의 여섯째 항목이다. cpppj 기준선 `1f00a25a`의 `o/RawAssetPostPop`(초기 요새 자산의 가상 표 +0x20 재정의: 제네레이터·Gun 네 부류·장식·거주 구역·바리케이드·간헐천·신전 세 부류와 전체 몸체를 훅으로 받는 여덟 부류)의 분배 본체를 옮겼다. 상태 부분(`AssetPostPopState`)은 [E61](webpj-rune-factory-post-pop-20261108.md)에서 이미 옮겼다. 게임·브라우저 실행과 디컴파일은 하지 않았다(Node 단위 검사만).

## 1. 기준선 뒤 변경 판정 (3.6절)

`git diff --numstat 1f00a25a HEAD`는 `.cpp +15/−6`·`.h +7/−2`, 커밋은 `be567d46 20261108-187 CP`(공통 자원 소유권·현재 수명과 기본 전송 접합)와 `417f22d2 20261108-220 CP`(장기 Host 수명·전체 선택값과 자원 PostPop 접합)다. cpppj 시험 `AssetPostPopTests.cpp`와 fixture 는 기준선 뒤 변경이 없다. typed 경로의 결과는 기준선과 같다(818행 fixture 로 재확인).

- **② 재반영:** 실제 장기 간헐천 owner 용 진입 — `GeyserPostPop(sid, flags)`(Native 정수화: 비유한·범위 밖 비용은 하위 DWORD 0), `HasDedicatedVtable`(전용 +0x20 몸체가 있는 가상 표인지 인스턴스 없이 조회), 현재 섬/종유석 TYPE 주소(`currentIslandType`·`currentStalagType`; 웹은 E61 상태 모듈의 getter 로 추가), `MakeAssetPostPopFinder` 의 native 인자를 옮겼다.
- 행 base 는 판정한 `417f22d2`로 올렸다. 호출자(client Session 의 자산 장기 owner)와 훅의 실제 몸체 연결은 4-D 몫이다.

## 2. 옮긴 것

`www/core/raw-asset-post-pop.js` — `RawAssetPostPop(pool, types, base, damageable, state, hooks)`, `makeAssetPostPopFinder(pool, hash, types, native)`. `www/core/raw-asset-post-pop-state.js` 에 `currentIslandType`·`currentStalagType`·`readIslandType()`·`readStalagType()` 추가.

- 10.78 가상 표 24개 → 몸체 부류 20가지. 각 몸체는 공통 장부(E41 `SquidPostPop.postPopBase` 또는 E60 직접 Damageable `RawCarrierPostPop.damageablePostPop`)를 정확히 한 번 부른다. 모듈 머리 주석에 부류별 순서를 적었다.
- 장치(훅 29종)는 그 몸체가 실제로 도달한 위치에서만 요구한다. 없으면 그 위치에서 거부하고 앞선 효과(목록·예약·WORD·프레임·Player 필드)는 되돌리지 않는다. 모든 효과 뒤에 raw 와 전역을 다시 읽으며, 쓰기는 free/contained 와 무관하게 같은 물리 슬롯(`pool.physical`)에 한다.
- `validate`(명시 사전 검사)는 독립 호출과 `SquidPostPop.validate`(독립 호환 진입 `activate`)가 쓴다. 실제 10.78 가상 진입(`postPop`·`postPopVirtual`)은 알려진 부류에 사전 검사를 끼우지 않고, 모르는 가상 표만 사전 검사가 거부한다.
- 목록: 거주/간헐천 목록은 signed count/capacity 비교 뒤 store → inc(중복 허용), 신전의 두 목록은 같은 관문 뒤 중복 검색을 하고 처음 읽은 count 위치에 저장한다(00414450).
- `bindPlayerRecords(records, currentLocalOwner)`: 실제 Player 9기록(9×0xb4)의 신전 네 DWORD(+0x7c..+0x88)에 직접 쓰고 읽는다. 연결되면 독립 사본(`state.players`)은 쓰지 않는다. C++ 의 `const uint32*` 현재 소유자는 getter 함수로 받는다.
- 이미 옮긴 몸체를 그대로 쓴다: 아크 스파이어(E49 `postPopArcSpire`)·애시드 펜스(E50 `postPopAcidFence`). 성장형 아이스(E109)·너겟(E16)·썬 바리케이드(E60)·공중 3종 생산 건물은 전체 몸체를 훅으로 받는다(공중 3종의 행동은 실행하지 않는다).

## 3. 검증 (`tests/asset-post-pop.test.mjs`, 29건)

| fixture (10.78 사용 행) | 대조 내용 |
| --- | --- |
| `assetpostpop-x86.tsv` 818행 (SHA-256 `706afb7e…`) | 열두 가상 표의 사건 순서·슬롯 50 전체·세 목록(꼬리 포함)·Player 1 신전 네 필드·신전 개수·두 이웃 프레임. 독립 호환 진입, 공유 권한(반대 fallback·반대 전투 표시), 공유 Player 9기록, 실제 가상 진입(거주/간헐천 188·Gun 여섯 318·바리케이드 48·신전 264)으로 각각 재생 |
| `asset-player-x86.tsv` 288행 (`9562745a…`) | 콜백 뒤 owner 변경과 현재 신전 DWORD, Player 9기록 전체 바이트 |
| `gun-postpop-checking-x86.tsv` 1,344행 (`4ba2d249…`) | 네 Gun 의 checking·로딩·편집 진단 순서, 진단 복귀/중단, 전역 네 값 |
| `vortex-current-x86.tsv` 936행 (`8f7ae62f…`) | 신전 진단·WORD·중복 없는 목록·9기록 Adler·세 Player 의 신전 필드·전역 |
| `arc-spire`·`acid-fence` 의 `P` 100행, `ice-tower` 의 `P` 212행 | 펜스/성장형 아이스의 부모 예약 사건과 물리 무변경 |

- cpppj 개별 시험(거주/간헐천 18·Gun 14·바리케이드/펜스 12·전체 몸체 부류 8·신전 19)의 10.78 분을 모두 옮겼다. CD 10.72 반복과 CD 전용 assert 는 범위 밖이다.
- 경계 계약(변형 시험에서 살아남은 입력): 가상 표 별칭 두 개, 단정도 지연과 단정도 비용 절삭, 목록 count 재조회, 다른 풀의 공통 장부, 사전 검사 세 조건, Gun 각도의 signed·client Pop flags 0x10/0x40(서버 SID 15000)·썬 로딩 백업의 DWORD 폭, 거주 구역의 지면 SID 0, 간헐천의 종유석 TYPE getter·이웃 프레임 DWORD, 신전의 프레임 DWORD·소유자 9·선택 Form kind WORD·boss 아님·iterator 시작 SID 2, 모르는 가상 표, 제네레이터의 TYPE 조회, Native finder.

최초 실패와 교정:

- 모듈은 처음부터 fixture 다섯 종과 계약 27건을 통과했다. 실패 1건은 검사 쪽 입력 형식이었다 — 성장형 아이스(E109 `postPopGrowingIce`)는 TYPE 마다 `{ codes }` 프레임 표를 받는데 빈 배열을 넘겼다. 형식을 고쳐 통과했다(모듈 변경 없음).
- 변형 125개: fixture·cpppj 계약으로 100개를 탐지했고, 경계 계약을 추가해 24개를 더 탐지했다. 그 과정에서 기존 검사 하나의 빈틈을 찾았다 — "신전 목록 음수 저장" 검사의 미도달 장치 문구에 기대 정규식과 같은 낱말이 들어 있어 음수 검사를 지운 변형도 통과했다. 문구와 정규식을 고쳤다.
- 남은 1개는 동등 변형이다: 새 신전 복사 필드의 원천(`필드 2 를 다시 읽기` 대 `SID 직접 쓰기`)은 사이에 콜백이 없어 같은 값이다.

## 4. 경계와 다음 시작점

- 아직 앱 수명에 연결되지 않았다(PORT_MAP session 0). 훅 29종의 실제 몸체(Regular 확보/생성·GunProcess·지면/소유자·표면 통지·간헐천 등록·Unpop/Repop·음향·섬/영역·삭제·slot 내용물·생산 갱신·전체 몸체 여섯)와 공유 권한·목록·Player 기록·현재 TYPE getter 는 4-D 에서 잇는다. 3-D 의 `SquidPostPop` 항목이 `setBody('asset', …)` 조합을 닫는다.
- 이 단위로 `RawAssetPostPop`은 PORT_MAP `port`(base `417f22d2`)다. 3-C의 다음 항목은 `RawAssetRegular`(354줄, +72)다.
