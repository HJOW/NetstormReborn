# webpj Damageable 삭제 후처리 이식(RawDamageablePostDestroy) — WB04-E253

2026-11-10, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 단계 3-C의 다섯째 항목이다. cpppj 기준선 `1f00a25a`의 `o/RawDamageablePostDestroy`(원본 0044b9e0 삭제 후처리·0044b830 드롭·00421db0 다리 약화·004b2070 한 칸 genus 조회)와 기준선 뒤 현재 전역 공유 소유자를 옮기고, E160 `RawCarrierArrival` 시험의 `genusAt` 대역을 실제 몸체로 바꿨다. 게임·브라우저 실행과 디컴파일은 하지 않았다(Node 단위 검사와, 게임을 띄우지 않는 기존 cpppj 시험 실행 파일의 `--filter` 확인만).

## 1. 기준선 뒤 변경 판정 (3.6절)

`git diff --numstat 1f00a25a HEAD`는 `.cpp +4/−4`·`.h +11/−0`, 커밋은 `241b013e 20261109-25 CP`(Native Damageable 해방 후처리와 간헐천 장기 수명 연결)와 `079f170e 20261109-29 CP`(Native 삭제 드롭과 이동 비행 전달의 현재 TYPE 전역 공유)다. cpppj 시험 `DamageablePostDestroyTests.cpp`도 같은 두 커밋에서 공유 소유자 시험 4건이 늘었다. 몸체의 순서·조건은 그대로이고 상태 조회만 "독립 필드 또는 공유 소유자"로 바뀌었으므로 typed 결과는 기준선과 같다(2,246행 fixture 로 재확인).

- **② 재반영:** 현재 전역 공유 소유자 다섯 종 — battle(00594fbc)·Dude population(0055a4f8, 같은 장기 목록의 현재 개수를 signed 로 해석)·피해 Regular 사건 DWORD·드롭 제외 TYPE 다섯 주소·특별(벌프) 드롭 TYPE. 기준선에 이미 있던 boss·조용한 표면 TYPE 공유와 같은 방식으로 `DamageablePostDestroyState.bind*`에 옮겼다(C++ 포인터 대신 `{ value }` 소유자, population 은 `readCount()` 를 가진 `SquidPostPopList`). 빌린 값은 각 소비 위치에서 다시 읽고 독립 필드는 바꾸지 않는다.
- 행 base 는 판정한 `079f170e`로 올렸다. 실제 소유자 연결(client `RawSurfaceWorld`/`RawCampaignSession` 의 Damageable 장기 owner)은 4-D 몫이다.

## 2. 옮긴 것

`www/core/raw-damageable-post-destroy.js` — `RawDamageablePostDestroy(pool, hash, types, frames, hp, footprint, state, hooks)`, `DamageablePostDestroyState`

- `postDestroy(sid, flags)`: ① flags 0x300000 이면 현재 genus 로 첫 드롭 → ② 일반 객체이고 TYPE flags1 0x400 이면 현재 칸의 잔해 genus(0x1000000)·자기 genus 0x10000000 없음·(붕괴 flag 또는 조용한 표면 TYPE 아님)일 때 섬 정리(현재 좌표·flags·현재 소유자 BYTE) → ③ 전투·HP 보유·일반 객체이면 주변 HP 피해 Regular 예약(붕괴 flag 없음, group 0·1 또는 볼텍스 genus; 피해 = 최대 HP/2+1, 볼텍스 400; 발자국 중심 반경 footX/2+1 의 표면 단계 생략 탐색에서 HP 보유·dead 아님·abstract 아님; count = 공격 소유자×65536 + 피해의 DWORD 덧셈, payload 0)과 주변 다리 후처리(genus 0x130000 없음; 반경 2; 금 없음 또는 낙하 예약 중이면 단단하지 않을 때 약화, 아니면 현재 boss 일 때 가상 삭제) → ④ 언제나 공통 post. 각 효과 뒤 raw·전역을 다시 읽는다.
- `drops(sid, genus, random)`: SID 0·abstract·드롭 genus(0x174200) 없음·제외 TYPE 이면 없음. 수량 1(0x4200 → 5, 0x20000 → 2, 특별 TYPE → 3), 현재 Dude 수 > 20 이면 절반. 반복 수는 처음 계산한 값을 유지한다.
- `genusAt(x, y, genus)`: 좌표를 그대로 절삭한 한 칸의 일반 탐색(genus 에 6 비트가 없으면 표면 단계 생략)에서 genus 가 겹치는 첫 후보. Carrier 도착·이동·내용물 안내·얼음 탑이 같은 몸체를 공유한다(실제 연결은 4-D).
- `weaken(sid)`: 수명 비트(WORD 의 0x78)를 (금 수명−1)로 쓰고 표면 통지 뒤 현재 프레임의 금 간 프레임(E44 `bridgeCrackedFrame`, 단단한 프레임은 번호 1 로 봄)을 찾아 있으면 프레임 DWORD 를 쓰고 표시 갱신을 부른다.
- 의존: E 단위에서 이미 옮긴 `RawSquidFinder`(WB04-B)·`RawSquidNeighbors.center`(E39·E243)·`SquidReward.hasHitPoints/maxHitPoints`(D5b)·`bridge-decay.js`(E44).

## 3. 검증

### 3.1 `tests/damageable-post-destroy.test.mjs` (10건)

- 원본 10.78 관측 `damageablepostdestroy-x86.tsv`(SHA-256 `7cbf0b93…8a4c6b`) 4,412행 중 `originals` 2,246행(Post 1,606·Drops 640): 부모 50 과 주변 후보 여섯(HP 대상 60·61, 무 HP 62, 다리 63·64, 잔해 65)을 실제 4단계 해시에 올린 장면에서 하위 효과 순서·인자(`D`·`I`·`R`·`F`·`N`·`U`·`X`·`B`)와 일곱 슬롯 전체 바이트를 대조한다. 효과 도중 TYPE·소유자·extra·dead·프레임·WORD 가 바뀌는 여섯 변이를 포함한다. 난수 상태 불변·SP 저장소 미초기화도 확인한다. CD 행(2,166행)은 범위 밖이다.
- cpppj 계약 7종: packed count 의 DWORD 덧셈과 필수 경계 누락 거부, 다리 사이의 공유 boss 재조회, 현재 population(signed 20 경계), 현재 battle·피해 사건 DWORD, 제외 TYPE 다섯(전체 DWORD), 특별 TYPE 과 반복 수 유지, 실제 Host(`SquidFactory`·`SquidProcessHost`·`Kernel`)에서 Regular 초기 count 와 첫 Kernel 호출(최대 HP 100·131073 두 장면).
- 경계 계약(변형 시험에서 살아남은 입력): 드롭 genus 여섯 비트 각각·SID 0·buried 부모, 섬 정리의 다섯 조건과 조용한 표면 TYPE 공유 소유자(전체 DWORD), 다리 제외 genus 세 비트 각각, 공격 소유자 8, 음수 최대 HP(−4 → count 65535, −3 → 65536)의 절삭·감김, 표면 단계 HP 객체·dead 후보 제외, 탐색 영역(중심 50+3ulp → 51칸, 아래/위 반경 2 포함·3 제외, 중심 2^31 의 DWORD 감김), 프레임 표 밖 거부, 프레임 DWORD 쓰기(256 → 5), `genusAt` 14점, 구성 검사 5종, SID/TYPE 범위 오류.

### 3.2 `tests/carrier-arrival.test.mjs` (기존 4건, 장면 교체)

E160 이 시험 안에 두었던 한 칸 genus 조회 대역 구현을 지우고, cpppj `CarrierArrivalSupport` 와 같이 실제 `RawDamageablePostDestroy.genusAt` 을 같은 풀·해시에 연결했다. 원본 10.78 실제 PE 203행이 그대로 통과한다(PORT_MAP `RawCarrierArrival` 메모의 "genusAt 실제 연결" 해소).

### 3.3 최초 실패와 교정

- 모듈·fixture 재생·cpppj 계약·Host 장면은 처음부터 통과했다(전체 검사가 도는 동안 임시 폴더 사본으로 먼저 확인).
- 변형 122개: fixture·cpppj 계약만으로는 46개가 살아남았고 3.1 의 경계 계약을 추가해 115개를 탐지했다. 경계 계약 첫 판에서 "표면 단계 HP 객체" 입력이 무효였다(HP 몸체가 구성 때 TYPE 표를 고정해 뒤에 바꾼 flags1 을 보지 못함) — 바꾼 표로 HP 몸체를 다시 만들어 닫았다. 남은 7개는 동등 변형이다: 반경 편향의 double/float 정밀도(발자국이 1 이상이라 중심이 2^-24 격자 위에 있어 두 상수 사이에 값이 없음), 섬 정리 직전의 좌표 재조회(사이에 콜백 없음), 약화 WORD 의 상위 BYTE 쓰기(수명 비트는 하위 BYTE 뿐), 발자국/2 의 절삭 방향(중심 몸체가 발자국 1 미만을 거부), 후보 abstract 검사의 buried 비트(탐색기가 buried 를 먼저 건너뜀), 프레임 flags 의 BYTE 마스크, 부모 SID 선검사(뒤의 조회가 같은 오류).
- **cpppj 쪽 관측(웹은 고치지 않음, cpppj 몫):** 기존 빌드 `cpppj/build/bin/Release/netstorm_tests.exe`(2026-11-08 15:38, 기준선 뒤 공유 소유자 시험 추가 전)에서 `--filter DamageablePostDestroy` 4건 중 `DamageablePostDestroy_ActualHostInitialCountAndFirstKernelCall` 이 `Factory의 선택된 원문 진단 장치 미연결`로 실패했다(나머지 3건 통과). 같은 10.78 장면을 옮긴 웹 검사는 통과한다. 빌드가 오래돼 현재 HEAD 에서도 실패하는지는 확인하지 못했다(이 세션은 cpppj 를 빌드하지 않는다).

## 4. 경계와 다음 시작점

- 아직 앱 수명에 연결되지 않았다(PORT_MAP session 0). 실제 드롭 생성(`dropDude`)·섬 정리(`RawConstructionEffects.DropIsland`)·Regular 예약·낙하 조회·표면 통지·표시 갱신·가상 삭제·공통 post 와 공유 전역 소유자는 4-D 에서 잇는다.
- 이 단위로 `RawDamageablePostDestroy`는 PORT_MAP `port`(base `079f170e`)다. 3-C의 다음 항목은 `RawAssetPostPop`(partial 525줄, +30)이다.
