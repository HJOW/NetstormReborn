# webpj Rune·Factory 후처리와 자산 후처리 공유 상태(RawRunePostPop·RawFactoryPostPop·AssetPostPopState) 이식 — WB04-E61

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 예순한 번째 부분이다. 공통 후처리(`SquidPostPop`)에 연결되는 종류별 후처리 몸체 두 개와 자산 후처리가 공유하는 상태를 옮겼다. cpppj 기준선 `1f00a25a` 의 `o/RawRunePostPop`(원본 Rune166 후처리: 304 예약·신전 조회·건설 시작), `o/RawFactoryPostPop`(Factory/아웃포스트 wrapper: 예약·상태 프레임 보정·중복 삭제·목록 등록), `o/RawAssetPostPop.h` 의 `AssetPostPopState`·`AssetPostPopPlayer`(Factory 가 쓰는 boss·목록·권한 상태)이다. 세 파일 모두 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 분기(Rune 의 전수 장치 사전 계약, Factory 의 CD 표 0x503298/0x503358 등)는 옮기지 않았다.

## 1. 옮긴 것

`www/core/raw-rune-post-pop.js` — `RawRunePostPop(pool, hash, types, state, base, hooks)`, `RunePostPopState`, `makeRunePostPopHooks`, `RUNE_VTABLE`

- `postPop(rune, flags)`: 최초 flag 에서 예약(`reserveRegular`)이 성공하면 `addRegular(rune, 0x304, 0.01)`. flags 비트 2 에서는 `findOwnedDais`(Rune 좌표에 `0.9999899864196777` 을 더해 0 방향 절삭한 칸 ±1 의 일반 finder, 현재 Dais TYPE 이고 현재 소유자 +34 가 Rune 과 같은 첫 후보)를 찾고, 그 Dais 위치의 `altarAt`(현재 Altar TYPE, genus & 6 에 따른 시작 단계의 첫 Altar)으로 `constructionSeconds`(Altar 없음·원래 owner WORD 0·시간 억제는 10초, 아니면 Player 플래그 bit1 이 있으면 25초, 없으면 7초)를 구해 `startConstruction(rune, 초)`. 마지막에 항상 공통 `postPopBase` 를 같은 flags 로 한 번 실행한다. 발견한 Dais 가 시간 helper 의 this 이다.
- 현재 Rune/Dais/Altar TYPE 번호와 시간 억제는 getter(`shared*`)가 있으면 소비 위치마다 다시 읽는다. 선택하지 않은 장치는 요구하지 않고, 도달한 장치가 없으면 앞선 효과를 보존한 채 오류를 낸다. 가상 표 `0x5075d8` 의 `handles` 는 현재 TYPE 과 무관하며 `validate` 는 명시 사전 검사이다.
- `makeRunePostPopHooks` 는 같은 풀의 Host(`addRegular`)와 Construction(`start`) 객체를 훅에 잇는다(실제 `RawConstructionProcess` 는 아직 웹에 없어 덕 타이핑 계약만 둔다).

`www/core/raw-factory-post-pop.js` — `RawFactoryPostPop(pool, hash, types, frames, base, damageable, state, hooks, numbers)`, `FactoryPostPopTypes`, `makeFactoryPostPopHooks`

- `postPop(sid, flags)`: 아웃포스트 wrapper(`0x509210`)가 최초 flag 이고 extra 에 abstract/buried 비트(9)가 없으면 아웃포스트·거주 목록에 중복 없이 추가하고 `regionOwnership(x, y)`(섬 소유권 재계산). 최초 flag 에서 현재 TYPE 이 첫 타이머(85)면 사건 2/8초, 둘째 타이머(87)면 사건 1/0.5초, 그리고 항상 사건 5/0.5초를 각각 독립 예약한다(실패는 해당 사건만 생략). 이어서 직접 Damageable(`RawCarrierPostPop.damageablePostPop`)과 `adjustFrame`.
- `adjustFrame`(00453780): 현재 TYPE 이 고정 프레임 TYPE(126)이면 생략. 아니면 가상 +0x80 개수(`virtualCount` 또는 확인된 Factory 표의 상태 상위 3비트)로 `'A'+개수` 글자 구간을 읽어, 저장 프레임이 그 구간 밖일 때만 첫 프레임으로 `setFrame`. 두 번째 개수 조회도 현재 TYPE 의 프레임 표를 다시 읽는다.
- boss 이면 같은 칸의 일반 finder 후보 중 같은 TYPE 이고 선택 비트 8 이 같은 중복 워크샾에 경고 로그(`warnDuplicate`)를 남기고 `destroy(candidate, 0)` 한다. finder 는 다음 후보를 삭제 전에 저장하므로 삭제 뒤에도 원본 순서로 진행한다. `setState(level, sid, value)` 는 공통 Level 저장 뒤 `updateState`(가상 +0x88 표시 갱신)를 호출한다.
- `FactoryPostPopTypes`(첫 타이머 85·둘째 타이머 87·고정 프레임 126)는 전체 DWORD 로 비교하며, 실제 Client 의 현재 값은 getter 로 빌린다. cpppj 가 값으로 받으므로 생성자가 사본을 만든다(생성 뒤 독립 호출자의 변경은 섞이지 않는다).

`www/core/raw-asset-post-pop-state.js` — `AssetPostPopState`, `AssetPostPopPlayer`

- Gun/Residence/Geyser/Vortex/Factory 재정의가 공유하는 전역(inBattle·boss·fortMode·checkingForms·로딩 깊이·신전 개수·선택·로컬 소유자·island/stalag/slot TYPE 번호·Player 신전 필드·Bridge TYPE)과 거주 건물·신전·간헐천·아웃포스트 목록을 가진다. 서버·boss 는 `RawAuthorityState`(`sharedAuthority`)를 빌릴 수 있고, 목록도 `shared*` 로 Session 보다 긴 owner 의 저장소를 빌린다. `readServer`·`readBoss`·`currentResidences`·`currentOutposts` 등은 소비 위치마다 현재 값을 읽는다. 자산 후처리 본체(`RawAssetPostPop`)는 이후 단위에서 이 상태를 그대로 쓴다.

## 2. 검증

- `tests/rune-post-pop.test.mjs`(7건): `runepostpop-x86.tsv` 10.78 960행(독립 pool/hash/Player 표마다 flags·후보 프로필·예약 성공·altar owner WORD·Player 플래그·시간 억제·변이 → 건설 시간·Dais·효과 순서·후보 세 슬롯 이미지·시간 억제 전역). 계약: 실제 PATCH 가상 표 진입의 사전 검사 비삽입, 최초 예약 누락이 finder/공통 부모보다 먼저 실패, 예약 실패 시 미연결 생성자 비요구와 생성자 누락의 부분 쓰기, 304 생성 효과 뒤 현재 Dais TYPE/소유자 재조회, 후보 없을 때 건설 장치 비요구, 시간 helper 의 타입 표 범위 오류가 건설 장치 검사보다 앞섬, 예약 뒤 공유 전역 재조회와 시간 억제의 Player 접근 선행, Host/Construction 훅 연결.
- `tests/factory-post-pop.test.mjs`(12건): `factorypostpop-x86.tsv` 10.78 498행 전체를 `Activate` 로, 그 가운데 일반 Factory 행을 가상 +0x20 분배(`postPopVirtual`)로 재생해 효과 기록·슬롯 50바이트·아웃포스트/거주 목록 전체(활성 밖 잔여 항목 포함)·후보 두 개의 TYPE/상태/extra 를 비교한다. 계약: 선택하지 않은 장치 비요구, 예약 실패 시 미연결 생성자 비요구, 늦은 프레임 장치 실패의 예약/공통 보존, 공통 부모가 바꾼 TYPE·레벨·boss 재조회, 경고→삭제 순서와 삭제 장치 누락, 일치 후보 없을 때 장치 비선택, 두 가상 개수의 현재 TYPE, 개수 실패, 물리 TYPE 읽기 시점, 타이머/제외 번호의 전체 DWORD·현재 값·독립 사본, 상태 설정과 프레임 훅 연결.
- 기존 공통 후처리·Carrier 후처리·finder 검사가 그대로 통과한다.

최초 실패와 교정: 새 모듈은 위 fixture 전부(Rune 960 + Factory 498행)를 첫 실행에 통과했다. 검사 포팅에서 `FactoryPostPopTypes` 를 참조로 받아 독립 사본 계약("생성 뒤 호출자가 바꾼 값이 섞이지 않는다")이 깨졌는데, cpppj 가 값으로 받는 점을 따라 생성자에서 사본을 만들도록 고쳤다.

## 3. 경계와 남은 것

- 아웃포스트(wrapper `0x509210`)는 공식 원본에서 멀티플레이어에서만 쓰이므로 웹 단독 경로에서는 도달하지 않는다. 대조된 분기라 그대로 옮겼지만 웹에서 새 기능을 연결하지는 않는다.
- 실제 Rune 생성자→Pop→Construction 완료→304 Regular 통합(cpppj `rawRunePostPop_real_pop_construction_complete_and_304`)은 `RawConstructionProcess` 이식 단위에서 대조한다. 실제 World 의 Rune/Dais/Altar TYPE·시간 억제 getter 와 Host 연결은 WB05 에서 한다.
- `SquidPostPop.setBody('rune' / 'factory', …)` 연결과 `RawAssetPostPop`(Gun·제네레이터·거주 구역·바리케이드·간헐천·신전 분배)은 다음 단위들이다. 이번에는 몸체의 `handles`·`postPop` 인터페이스가 `setBody` 계약과 맞는지 `postPopVirtual`/`activate` 로 확인했다.

## 4. 다음 시작점

`RawPriestPostPop`/`RawPriestPostPopTail`(RawPriestOwner·RawPriestState·RawPriestShield·RawPathAnimation 선행), `RawAssetPostPop`(Fence 분배 포함), `RawNuggetPostPop`·`RawBombPostPop` 등 남은 종류별 후처리, 후보/지형 권한(`RawCanonPlacementPermission`) 이다.
