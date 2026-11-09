# webpj Damageable 삭제 전처리 이식(RawDamageablePreDestroy) — WB04-E251

2026-11-10, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 단계 3-C의 셋째 항목이다. cpppj 기준선 `1f00a25a`의 `o/RawDamageablePreDestroy`(원본 0044b4b0; Damageable 의 붕괴/폭발 효과와 소리, 현재 좌표 spot 의 6 비트가 열면 내용물 순회로 사제 해방 소리, 권한이 있으면 내용물 해방, 마지막에 공통 preDestroy)와 기준선 뒤 Native 몸체를 옮겼다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만).

## 1. 기준선 뒤 변경 판정 (3.6절)

`git diff --numstat 1f00a25a HEAD`는 `.cpp +34/−3`·`.h +5/−0`, 커밋은 `230528e3 20261108-162 CP`와 `241b013e 20261109-25 CP`(Native Damageable 해방 후처리와 간헐천 장기 수명 연결)이다. 변경은 모두 Native 몸체 추가이고 typed 경로의 결과는 기준선과 같다(typed 1,632행 fixture 로 재확인).

- **② 재반영:** `enableNativeBody()`(원문 위치의 검사/효과 선택, typed 선검사 생략), `preDestroyNative(sid, flags, releaseContained)`(해방 장치를 호출마다 받음), Native 의 spot 계산(Y 먼저, `_ftol` 하위 DWORD, 감긴 `Y*256+X` 의 현재 물리 한 BYTE 만 확인), 사제 TYPE 공유 소유자(`bindPriestType({ value })`, C++ `sharedPriestType` 포인터 대신)를 옮겼다.
- 행 base 는 판정한 `241b013e`로 올렸다. Native 호출자(client Damageable 장기 owner)는 4-D 몫이다.

## 2. 옮긴 것

`www/core/raw-damageable-pre-destroy.js` — `RawDamageablePreDestroy(pool, spots, finderState, state, hooks)`, `DamageablePreDestroyState`, `makeDamageablePreDestroyHooks(pool, damageable, hooks)`, `DAMAGEABLE_SOUND`

- 효과 순서: extra(abstract|buried=9)가 없으면 붕괴(0x200000) 효과→소리(그 시점 좌표)→폭발(0x100000) 효과→소리→현재 좌표 spot 이 6 비트를 가지면 `RawContainedFinder`(dead 부모 허용)로 순회하며 사제 kind(WORD)와 사제 TYPE(DWORD, 매 비교 현재값) 같음마다 해방 소리→현재 boss 이고 해방 억제 flags(0x800)가 아니면 내용물 해방. extra 와 무관하게 마지막은 공통 preDestroy(`base`). 효과가 좌표·extra·boss·TYPE·next 를 바꿀 수 있으므로 소리/spot/boss/사제 TYPE 은 효과 뒤 현재값을 다시 읽고, extra 진입 조건은 효과 중에 다시 평가하지 않는다.
- typed `validate`(독립 경로의 선검사): 할당 슬롯·자산 TYPE(70..187)·지원 상태(free|contained 아님), extra 가 없으면 좌표 유효성(`좌표 + 0.9999` 가 (−1, 256) 안), 공통 사전 검사. Native 몸체는 이를 생략한다.
- `makeDamageablePreDestroyHooks`: E56 `RawCarrierPreDestroy`의 `validateDamageable`/`damageablePre`만 같은 풀의 이 몸체로 바꾼다. 다른 풀이면 거부한다.

## 3. 검증 (`tests/damageable-pre-destroy.test.mjs`, 5건)

- 원본 10.78 관측 `damageablepredestroy-x86.tsv` 1,632행(`originals`): 직접 Damageable(D)과 Carrier 경유(C)에서 효과 순서(`C`·`X`·`A0`/`A1`·`F`·`R`·`B`·Carrier 후처리 `T`)와 인자, 현재 전역(boss·contained TYPE·사제 TYPE), 다섯 입력 슬롯(50, 60..63) 전체 바이트를 대조한다. 효과 도중 좌표·extra·boss·TYPE·next 가 바뀌는 네 변이 스타일을 포함한다. 같은 1,632행을 Native 몸체로 한 번 더 재생하고 선검사 훅이 한 번도 불리지 않음을 확인한다. CD·10.37 행(3,264행)은 범위 밖이다.
- 계약: 하위 효과 누락·짧은 공간 지도 거부, 잘못된 SID/상태/TYPE/비유한 좌표는 효과 전에 거부(풀·사건 불변), 추상 좌표는 읽지 않고 공통 body 까지 전달, 공통 사전 검사 거부는 효과 시작 전, 다른 풀 거부, Native 의 사제 TYPE 재조회.
- 경계 계약(변형 시험에서 살아남은 입력): spot 비트 2/4 어느 쪽이든 순회가 열림, 좌표 하한(정확히 `-1` 이 되는 float `0xbffffcb9` 거부·`0xbffffcb8` 허용), 효과가 바꾼 현재 boss(공유 소유자) 반영, Native 감긴 `Y*256+X`(y=2^24 → spot[x], typed 는 거부)와 y=2^63 거부, 해방 장치 필수, 자산 TYPE 하한 70.

최초 실패와 교정: 처음부터 통과했다(fixture 두 재생·계약). 변형 21개 중 12개를 바로 탐지했고, 살아남은 아홉 중 일곱은 위 경계 입력을 추가해 탐지했다. 남은 둘은 동등 변형이다: 좌표 상한 `≥ 256` 의 정확히 256 은 float 로 만들 수 없고(`256 − 0.9999f` 는 float 이 아님), Native 가 두 좌표를 읽는 순서(Y 먼저)는 두 좌표가 모두 잘못일 때 어느 쪽이든 같은 `RangeError` 이다.

## 4. 경계와 다음 시작점

- 아직 앱 수명에 연결되지 않았다(PORT_MAP session 0). 실제 Damageable 장기 owner 의 해방 장치·공유 boss/사제 TYPE 소유자·실제 spot 지도는 4-D 에서 잇는다.
- 이 단위로 `RawDamageablePreDestroy`는 PORT_MAP `port`(base `241b013e`)다. 3-C의 다음 항목은 `RawDamageableRelease`(+38)다.
