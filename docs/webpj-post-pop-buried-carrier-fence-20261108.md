# webpj buried·Carrier·썬 바리케이드 후처리(RawBuriedPostPop·RawCarrierPostPop·RawFencePostPop) 이식 — WB04-E60

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 예순 번째 부분이다. 공통 후처리(`SquidPostPop`, E41)에 연결되는 종류별 후처리 몸체 중 가장 작은 세 개를 옮겼다. cpppj 기준선 `1f00a25a`의 `o/RawBuriedPostPop`(원본 00462170 buried 전용), `o/RawCarrierPostPop`(Carrier 00427720·Damageable 0044bf80 비가상 몸체와 공통 장부 연결), `o/RawFencePostPop`(썬 바리케이드 00457560)이다. 세 파일 모두 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 분기(Carrier 의 전체 사전 검사, buried CD 00416ac0 등)는 옮기지 않았다.

## 1. 옮긴 것

`www/core/raw-buried-post-pop.js` — `RawBuriedPostPop(pool, boss, loading, hooks, bridgeType)`

- boss(DAT00540bc4)·로딩 깊이(005c89b8)·Bridge 번호(005411a0)는 **현재 값을 돌려주는 getter** 로 받아 매 소비 위치에서 다시 읽는다(cpppj 가 const 참조를 빌리던 방식). 추첨 콜백이 이 값을 바꿔도 이후 판단에 반영된다.
- `postPop(sid, flags)`: 최초 flag 이고 boss 이며 로딩 깊이가 0 이면 보상 타입 선택 `chooseType(5)` → 현재 QA WORD(+12) 하위 16비트를 물리 슬롯에 쓰기(free/contained 와 무관) → 현재 TYPE 가 현재 Bridge 번호(전체 DWORD 비교, BYTE 로 줄이지 않음)일 때만 `notifySurface`. 최초 flag 에서 `reserveRegular` 를 두 번 독립 호출해 성공한 것만 사건 0/0초, 사건 1/0.01초(`0x3c23d70a`)를 `addRegular` 로 등록한다. 마지막에 항상 현재 QA WORD 를 읽고(물리 SID 범위 확인) `base(sid, flags)`(공통 postPop, 깊이 1 감소)를 호출한다.
- `handles(sid)`(가상 표 `0x50a748`)·`validate(sid, flags, pop)` 는 명시 사전 검사이며 `postPop` 본문 앞에 끼우지 않는다. 선택하지 않은 장치는 요구하지 않고, 도달한 장치가 없으면 앞선 쓰기를 보존한 채 오류를 낸다.

`www/core/raw-carrier-post-pop.js` — `RawCarrierPostPop(pool, types, state, hooks)`, `CarrierPostPopState`, `makeCarrierPostPopHooks`

- `postPop`(Carrier 00427720): 최초 flag 가 없고 현재 boss 권한이 없을 때만 `carrierCheck`(가상 +0xcc)를 호출하고, 이후 항상 Damageable 몸체를 실행한다. `damageablePostPop`(0044bf80)은 carrier 확인을 거치지 않는다.
- Damageable: 최초 flag 에서 현재 TYPE 접근(genus 조회, 반환값은 버리지만 물리 범위 오류는 낸다)을 먼저 하고, 로딩 깊이가 있고 현재 TYPE 이 간헐천이 아니며 extra 에 abstract/buried 비트(9)가 없고 현재 TYPE flags1 에 지면 소유자 갱신 비트(0x400)가 있으면 현재 좌표 float 와 소유자(+34)로 `claimGround`, 그 뒤 항상 `base`. 가상 carrier 확인이 TYPE/extra/좌표/소유자를 바꿔도 이 몸체가 현재 raw 를 다시 읽는다. 10.78 은 CD 의 전체 사전 검사를 호출하지 않는다.
- `CarrierPostPopState`: boss/로딩/간헐천 TYPE 의 독립 값과 실제 전역 getter(`sharedBoss` 등)를 함께 가진다. `bindCurrentTypes` 는 실제 World 의 현재 188칸 TYPE 표를 대여한다. `makeCarrierPostPopHooks(pool, base, carrierCheck, claimGround)` 는 같은 풀의 `SquidPostPop` 의 `validateBase`/`postPopBase` 에 연결한다.

`www/core/raw-fence-post-pop.js` — `RawFencePostPop(pool, hooks)`

- `postPop`(00457560): flags 비트 1 에서 테마 번호 3 이면 사건 0 예약 → 방향 0/2/4/6 의 최근접 펜스를 네 방향 모두 조회 → 현재 소유자 BYTE 를 DWORD(+42)로 저장 → (북·남) 쌍과 (동·서) 쌍이 모두 있으면 각 이웃에 사건 3 예약 두 번 뒤 현재 두 끝점 좌표로 연결선 삭제 요청 → 연결 수를 DWORD(+46)에 저장 → 자신에 사건 2 예약. 모든 flags 에서 Damageable(`damageable`)로 끝난다. 예약 지연은 float 0.1(`0x3dcccccd`), 각 예약의 실패는 해당 사건만 생략한다. 이웃 SID 0 은 "없음"이다.
- `handles`(가상 표 `0x509470`)·`validate` 는 명시 사전 검사이다. 이 몸체를 부르는 자산 후처리(`RawAssetPostPop`)는 다음 단위들의 몫이라 `SquidPostPop.setBody` 에는 연결하지 않았다.

## 2. 검증

- `tests/buried-post-pop.test.mjs`(8건): `buried-postpop-x86.tsv` 580행(독립 풀마다 boss·로딩·flags·할당 마스크·QA·선택 타입·변이 → 슬롯 50바이트·boss·로딩·호출 기록). 계약: 미선택 추첨 불필요·예약 실패의 부분 쓰기, 최초 비트가 없는 입력, 추첨 뒤 변경한 수명/TYPE/Bridge, Bridge DWORD 전체 비교, 첫 확보 실패 뒤 두 번째 생성자, 생성자 누락 시 앞선 쓰기 보존, 명시 Validate 분리와 flags0 의 물리 SID 읽기.
- `tests/carrier-post-pop.test.mjs`(4건): `carrierpostpop-x86.tsv` 10.78 2448행(Carrier / Damageable 본체의 사건 `C:`·`G:`·`B:` 와 효과 뒤 슬롯 50바이트). 계약: 실제 `SquidPostPop` 장부(비용 133, 지역/전역 통계, 깊이)에 연결한 직접 공통 호출, AI 실패 앞의 지면/비용 기록, 억제된 공통 몸체의 깊이 한 번 감소, 현재 boss 재조회와 첫 flag 비의존, 현재 TYPE 대여·장치 누락 거부.
- `tests/fence-post-pop.test.mjs`(5건): `fence-postpop-x86.tsv` 330행(부모 50과 이웃 51~54 전체 250바이트·호출 기록). 계약: 미선택 접두의 비요구와 부분 쓰기, 명시 사전 검사 비삽입, 이웃 변이의 물리 소유자 쓰기, 연결 반환 DWORD 전체 저장, 연결선 삭제 누락·마지막 예약 누락.
- 기존 공통 후처리·Pop/Unpop 검사가 그대로 통과한다.

최초 실패와 교정: 새 모듈은 위 fixture 3358행 전부를 첫 실행에 통과했다. 검사 보조에서만 오류가 있었다 — Carrier 의 현재 TYPE 대여 검사가 생성자가 복사한 TYPE 표를 쓰고 있었던 점을 `bindCurrentTypes` 로 바로잡았다.

## 3. 경계와 남은 것

- 사제 접두·Process·Reward 와 Carrier 를 조합하는 cpppj `carrier_postpop_composes_priest_prefix_process_and_real_common_body` 는 `RawPriestPostPop` 이식 단위(다음)에서 대조한다.
- `SquidPostPop.setBody('buried' / 'carrier', …)` 연결과 실제 World 의 boss/로딩/Bridge/간헐천 getter 바인딩은 WB05 월드 조립에서 한다. 이번에는 몸체를 독립 구성으로 대조했다(`handles`·`postPop` 인터페이스는 `setBody` 계약과 같다).
- 썬 바리케이드의 이웃 조회(00456220)·연결선 삭제(00455b20)·연결 수(004573d0)와 Regular 예약 소비자는 해당 이식 단위(Fence 이웃/선/Regular)의 몫이다.

## 4. 다음 시작점

`RawPriestPostPop`/`RawPriestPostPopTail`, `RawRunePostPop`, `RawFactoryPostPop`, `RawAssetPostPop`(Fence 분배 포함), 후보/지형 권한(`RawCanonPlacementPermission`) 이다.
