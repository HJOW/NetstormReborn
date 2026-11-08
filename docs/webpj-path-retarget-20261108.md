# webpj 경로 대상 재확인(RawPathRetarget) 이식 — WB04-E103

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백세 번째 부분이다. 경로(Path) 프로세스의 Halt 안에서 운반 대상이 죽었을 때 새 귀환 신전/간헐천을 고르고 새 이동을 시작하는 분기(489ff0)를 cpppj 기준선 `1f00a25a` 의 `o/RawPathRetarget` 에서 옮겼다. E98 의 `raw-path-runtime.js`(경로 이미지)가 선행이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이다.

## 1. 옮긴 것

`www/core/raw-path-retarget.js` — `RawPathRetarget(pool, nuggetType, hooks)`

- `update(runtime, initialParent)`: 대상이 살아 있으면 새 이동/내용물 조회 없이 현재 경로를 계속 진행한다. 경로 이미지의 두 도착 대상(귀환 targetB·간헐천 targetA)과 현재 부모 DWORD(+8)를 첫 조회 전에 보관한다. 이어
  1. 귀환 대상이 0 이 아니고, 보관한 귀환 대상이 죽었고(dead 비트 2; free 비트 1 과 별개), 현재 부모의 내용물 조회(`find(부모, mask 0x20, 현재 너겟 타입, 0)`)가 있으면 → 귀환 후보를 다시 골라 targetB 를 새 후보로 쓰고 후보가 있으면 귀환 이동(`goReturn`)을 시작하고 끝낸다.
  2. 아니면 간헐천 대상이 0 이 아니고, 보관한 간헐천 대상이 죽었고, **Halt 전 부모**(`initialParent`)에게 내용물이 없으면 → 간헐천 후보를 다시 골라 targetA 를 쓰고 후보가 있으면 targetB 를 0 으로 지운 뒤 간헐천 이동(`goGeyser`)을 시작한다.
  이동 호출 중 기존 경로가 삭제될 수 있으므로 호출 뒤에는 경로를 다시 읽지 않는다. 너겟 타입은 각 조회 직전에 다시 읽고, 대상 SID 는 실제로 조회할 때만 물리 범위를 확인한다(0 번도 물리 슬롯이며 주소 계산만 했던 무효 SID 를 미리 조회하지 않음).
- 도달한 훅이 없으면 오류이다(`find·chooseReturn·chooseGeyser·goReturn·goGeyser`). CD(10.72) 판본은 거부한다.

## 2. 검증

- `tests/path-retarget.test.mjs`(2건):
  1. `path-retarget-x86.tsv` 4801행(10.78 전용 단일 판본, 53/64비트 열 동일). 입력 12 DWORD(귀환/간헐천 대상 SID·두 대상 상태·현재 부모·첫 조회/둘째 조회 결과·두 후보·삭제 여부·변이 11종·너겟 타입)로 외부 효과 순서(F 내용물 조회/C·Y 후보 선택/R·G 이동 시작, 인자는 부모·대상·그 시점의 두 대상·부모 DWORD)와 상태(삭제 여부·삭제 직전 마지막 두 대상과 부모 DWORD·너겟 타입·조회 횟수·앞 0x10000바이트 Adler-32)가 같다. 호출 중 경로 삭제·대상/부모/너겟 타입 변경 변이를 포함한다.
  2. 계약: 선택하지 않은 경계는 요구하지 않고(대상이 살아 있으면 훅 없이 통과), 실제 죽은 대상의 미연결(부모 SID 범위)과 CD 판본은 거부한다.

최초 실패와 교정: 새 모듈은 위 fixture 4801행과 계약 검사를 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 전체 Halt 의 권한/부모 가상/살아 있음 검사는 `RawPathProcess` 단위에서 한다. 내용물 조회(4ac9f0)·후보 선택(`RawCarrierChoice`, E102)·이동 시작(`RawCarrierTarget`, E100)은 훅으로 받으며 실제 연결은 `RawPathProcess` 이식 뒤에 한다.

## 4. 다음 시작점

`RawPathProcess`(RunFrame·도착·Halt; 커널/Form 장면 선행)·`RawPathStart`·`RawPathData`·`RawPathCleanup`, `RawBalloonFrame`, `RawGunGroundTarget`·`RawGunProcess`(+`MakeGunTargetHooks`), Missile 계열, `RawTempleDraw`/캡처 계열(자산 시험 장치), `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawManaBoltArrival`(시험 장면 선행), `RawIslandRuntime` 계열, 그 밖의 작은 몸체이다.
