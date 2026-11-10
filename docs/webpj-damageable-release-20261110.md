# webpj Damageable 종속 해방 이식(RawDamageableRelease) — WB04-E252

2026-11-10, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 단계 3-C의 넷째 항목이다. cpppj 기준선 `1f00a25a`의 `o/RawDamageableRelease`(원본 0044b626~0044b80c; Damageable 삭제 준비에서 권한 확인 뒤 내용물을 해방하는 구간)와 기준선 뒤 Native 진입을 옮기고, 앞 단위(E63·E68)가 이 파일 이식 때로 미뤄 둔 합성 시험 두 건을 닫았다. 게임·브라우저 실행과 디컴파일은 하지 않았다(Node 단위 검사와, 게임을 띄우지 않는 기존 cpppj 시험 실행 파일의 `--filter` 확인만).

## 1. 기준선 뒤 변경 판정 (3.6절)

`git diff --numstat 1f00a25a HEAD`는 `.cpp +23/−9`·`.h +4/−2`, 커밋은 `230528e3 20261108-162 CP`(주소·master·Flyer·Outpost 실제 접합) 하나다. cpppj 시험 `DamageableReleaseTests.cpp`와 fixture 는 기준선 뒤 변경이 없다. 변경은 모두 Native 진입 추가이며 typed 경로의 결과는 기준선과 같다(typed 1,504행 fixture 로 재확인).

- **② 재반영:** `ReleaseNative(parent, battle, bridgeType)`(부모 자격을 재검사하지 않고 현재 battle·다리 TYPE 전역을 참조), `NativeCell`(signed64 CRT 절삭 뒤 하위 DWORD 를 signed 로 재해석, 축별 보드 범위 선검사 없음), Native 의 `Center`(발자국 범위 검사 생략)·`Spot`(Y 먼저, 감긴 `Y*256+X` 의 현재 물리 한 BYTE 만 확인)을 옮겼다. C++ 의 `const bool&`/`const uint32&` 전역 참조는 웹에서 현재값 getter 함수로 받는다.
- 행 base 는 판정한 `230528e3`으로 올렸다. Native 호출자(client `RawSurfaceDamageable`·`RawCampaignSession` 의 Damageable 장기 owner)는 4-D 몫이다.

## 2. 옮긴 것

`www/core/raw-damageable-release.js` — `RawDamageableRelease(pool, types, spots, finderState, state, hooks)`, `DamageableReleaseState`, `makeDamageableReleaseHooks(pool, release, hooks)`

- 고정점: 부모 TYPE 의 발자국 X 가 1 이면 현재 raw 좌표, 아니면 발자국 중심(좌표 − 발자국×0.5, float 저장)의 각 축에 단정도 0.99999 를 double 로 더해 절삭하고 float 로 저장한다. 이 점은 순회 내내 유지된다.
- 순회(`RawContainedFinder`, dead 부모 허용): 후보마다 mask 하위 BYTE 의 0x20 → 종속 kind(WORD 전체로 TYPE 표 조회) → 전투가 아니면 kind genus 와 현재 허용 genus DWORD 교집합을 본다.
  - 사제 genus(0x200000): 부모의 현재 좌표(부모 genus 가 0x80400000 비트를 가지면 발자국 중심 + 0.5f)·kind·후보 WORD 를 `spawnPriest` 에 넘긴다(E68 `makePriestSpawnHooks` 로 실제 나선 생성에 연결).
  - 그 밖: 고정점의 spot(단정도 0.9999 를 더해 절삭한 칸)이 0x10 을 갖거나 6 비트가 없으면 건너뛴다. 아니면 생성(kind, 0) → **생성 뒤** 후보의 현재 WORD 를 새 슬롯에 복사 → 새 슬롯 TYPE BYTE 가 현재 다리 TYPE DWORD 와 같으면 표면 알림 → 소유자(new, 0) → Pop(new, 점X, 점Y, 0).
  - 하위 효과가 raw·전역·공간을 바꿀 수 있으므로 battle·허용 genus·다리 TYPE·부모 좌표/TYPE·spot·후보 WORD 는 소비 위치마다 현재값을 다시 읽는다.
- typed `release(parent)`: 할당 슬롯·상태(free|contained 거부)·자산 TYPE 하한(70)을 먼저 확인하고 발자국(1..256)·좌표(`좌표 + 편향` 이 (−1, 256) 안)를 거부한다.
- `DamageableReleaseState`: battle(00594fbc)·allowedGenus(00542640, 초기 0x40480002)·bridgeType(005411a0, 82). `bindAllowedGenus({ value })`로 실제 Client 의 현재 DWORD 를 빌린다(C++ `sharedAllowedGenus` 포인터 대신, 기준선에 이미 있음).
- `makeDamageableReleaseHooks`: E251 `RawDamageablePreDestroy` 훅 묶음의 `releaseContained` 만 같은 풀의 이 몸체(typed)로 바꾼다. 다른 풀이면 거부한다.

## 3. 검증

### 3.1 `tests/damageable-release.test.mjs` (4건)

- 원본 10.78 관측 `damageablerelease-x86.tsv`(SHA-256 `73768524…4b3e5e`) 4,512행 중 `originals` 1,504행: 직접 Damageable(D)과 Carrier 경유(C) 전체 반환에서 효과 순서(`C`·`X`·`A0`/`A1`·`F`·사제 생성 `P`·일반 생성 `N`·표면 알림 `S`·소유자 `O`·Pop `L`·공통 body `B`·Carrier 후처리 `T`)와 인자(좌표 float 비트·kind·WORD), 현재 전역 두 묶음(boss·contained TYPE·사제 TYPE / battle·허용 genus·다리 TYPE), 입력 슬롯 다섯 개(50, 60..63)와 새 슬롯 네 개(100..103) 전체 바이트를 대조한다. 첫 생성 경계에서 WORD·next·좌표·boss·전역·spot 이 바뀌는 두 변이를 포함한다. 같은 1,504행을 Native 두 진입(E251 `enableNativeBody` + `releaseNative` 의 현재 전역 getter)으로 한 번 더 재생한다. CD·10.37 행(3,008행)은 범위 밖이다.
- 계약(cpppj `damageable_release_guards_and_binding`): 하위 효과 누락·짧은 공간/TYPE 표 거부, 잘못된 SID·free 부모·비유한 좌표는 효과 전에 거부, 다른 풀 연결 거부.
- 경계 계약(변형 시험에서 살아남은 입력): 스냅 편향 경계(x 비트 `0x3727ffff` 0칸·`0x37280000` 1칸·`0x3727c5ad` 0칸), 중심 보정 genus 의 두 비트 각각, 중심의 float 저장(0.1f·발자국 2 → `0xbecccccc`, 두 축 따로), typed 부모 자격(contained 8·TYPE 69/70), 좌표 하한(`0xbfffffac` 거부·`0xbfffffab` 허용), 발자국 X=1 판정과 범위(1..256), 권한 없는 dead 부모 순회, spot 비트 2/4, 다리 TYPE 의 DWORD 비교(0x152 ≠ 82), kind 의 WORD 조회(188·414·0xffff 범위 오류), 공유 허용 genus 재조회, Native 의 음수 절삭(−5.5 → −4, spot 인덱스 `21*256−3`)·감긴 인덱스(y=2^24 → spot[x])·물리 범위 밖 거부·±2^63 경계·부모 자격/발자국 무검사·getter 필수, 10.78 풀 요구.

### 3.2 `tests/damageable-release-integration.test.mjs` (3건, 미뤄 둔 합성 시험)

- cpppj `priest_spawn_release_uses_real_factory_and_priest_owner`([E68](webpj-priest-spawn-participation-20261108.md)이 미룸): 해방 → 사제 나선 생성 → 실제 서버 SID/사제 생성자(`SquidFactory`) → 실제 사제 소유자(`RawPriestOwner`·`SquidOwner`). 생성 SID 15000·WORD `0x8181`·소유자 1·후보 WORD 불변·Pop/Carrier 검사/알림 각 1회.
- cpppj `priest_predestroy_cleans_real_regen_form_kernel_and_common_bookkeeping`([E63](webpj-priest-postpop-shield-20261108.md)이 미룸, client·server SID 각 1건): 사제 삭제에 실제 Damageable 접두(E251)·종속 해방·Carrier·공통 삭제 장부(`SquidDestroyLifecycle`)·회복 Form·Kernel·SID 반납을 연결한다. 보호막 삭제 요청 1회·중첩 삭제 무효과·해방 1회(회복 Form 은 contained TYPE 이 아니라 제외)·장부 복구·깊이 0·server 전파 2회.
- 두 cpppj 시험은 기존 빌드 `cpppj/build/bin/Release/netstorm_tests.exe --filter`로 통과를 먼저 확인했다(게임 실행 아님).

### 3.3 최초 실패와 교정

- 모듈과 fixture 두 재생·계약·합성 3건은 처음부터 통과했다.
- 경계 계약 초안의 단언 하나가 틀렸다: free(1) 부모는 이 몸체의 상태 검사보다 먼저 풀의 할당 슬롯 대여(`pool.allocated`)가 거부한다. 단언을 "free 는 오류 종류 무관, contained(8)만 이 몸체의 오류"로 고쳤다(모듈은 바꾸지 않음).
- 변형 69개: 첫 fixture/계약만으로는 37개가 살아남았고, 3.1 의 경계 계약을 추가해 63개를 탐지했다. 남은 6개는 동등 변형이다 — 공간 조회 편향 0.9999 의 값·정밀도 2종(고정점이 언제나 정수 float 라 `n + 0.9999`·`n + 0.99999` 가 같은 칸), 부모 거부 상태의 free 비트(풀이 먼저 거부), typed 좌표 상한의 정확히 256(`256 − 0.99999f` 는 float 이 아님), 스냅 정수의 float 저장(절삭값은 언제나 float 로 표현됨), 중심 + 0.5 의 float 저장(중심이 이미 float 격자라 덧셈이 정확).

## 4. 경계와 다음 시작점

- 아직 앱 수명에 연결되지 않았다(PORT_MAP session 0). 실제 생성(`SquidFactory`)·소유자/Pop 가상 분배·표면 알림·공유 허용 genus/battle/다리 TYPE 소유자·실제 spot 지도는 4-D 에서 잇는다.
- 이 단위로 `RawDamageableRelease`는 PORT_MAP `port`(base `230528e3`)다. `RawPriestSpawn`·`RawPriestPreDestroy` 메모의 미룬 합성 항목을 닫았다.
- 3-C의 다음 항목은 `RawDamageablePostDestroy`(216줄, +19; `RawCarrierArrival` 의 `genusAt` 실제 연결 포함)다.
