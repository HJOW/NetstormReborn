# webpj 총 피해 시각/전달(RawGunHit) 이식 — WB04-E112

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백열두 번째 부분이다. Unit.cpp 의 압축 총 피해 시각(004d4da0/004d4e10)과 현재 표적의 실제 가상 피해 전달(004d4eb0/004d4f00)을 cpppj 기준선 `1f00a25a` 의 `o/RawGunHit` 에서 옮겼다. 선행은 `SidPool` 뿐이다(Missile 연결 `makeMissileGunHitHooks` 는 `RawMissile` 이식 전이라 훅 객체만 만든다). 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이다.

## 1. 옮긴 것

`www/core/raw-gun-hit.js` — `RawGunHit(pool, types, hitTypes, hooks)`, `makeMissileGunHitHooks(hit, state, hooks)`

- `time(firing)`(004d4da0): raw+12 WORD 의 bit0(건물 출력 상태)을 제외한 15비트 압축 시각을 `정수초 + (1/8초)×0.125` 로 읽는다(분수는 binary 0.125 이므로 원본 double 비트를 유지).
- `setTime(firing, now)`(004d4e10): 현재 시각을 `정수초×8 | (소수×8 절삭)` 로 압축해 WORD 의 상위 15비트에 쓰고 bit0 을 보존한 뒤 공간 통지(`notifySurface`)를 부른다. 시각/피해 절삭은 유한 int32 범위만 허용한다(NaN·범위 밖은 오류). 발사 SID 는 0 이 아니고 범위 안이며 TYPE 이 비행(flags2 0x100000)이 아니면 group 0/1 이어야 한다(원본 assert).
- `apply(firing, target, damage)`(004d4eb0): 실제 source+c0 전용 진입(`applyOverride`; Rain 45f940)이 처리하면 끝낸다. 아니면 표적 0 이거나 free/dead(상태 &3)면 무시하고, 아니면 현재 가상 피해(`damage(target, damage, 발사 소유자 BYTE, 발사 SID)`)를 부른다. 피해 처리기 누락은 성공으로 우회하지 않는다.
- `hit(firing, target, now)`(004d4f00): 총 타입의 float DPS(비유한은 오류)를 읽고 패치는 별도 flyer/balloon DPS 선택(표적 TYPE flags2 0x120000)을 적용한다. 양자화 현재 시각(15비트) − 저장 시각 = 경과를 `경과 × DPS` 로 곱해 1 이상이면 정수 피해로 `apply` 하되, 경과(float 저장)가 음수이거나 30초를 넘으면 `DPS × 3` 으로 바꾼다. 어느 경우든 마지막에 현재 시각을 `setTime` 한다.
- `makeMissileGunHitHooks(hit, state, hooks)`: Missile 직접 피해(`hit`)와 빗나감/배리어 코너 시각 갱신(`refresh`)을 같은 몸체로 연결한다(`state.now` 는 live).
- `hitTypes[type] = { damagePerSecond, specialFlyerDamage, flyerDamagePerSecond }`, `hooks = { damage, notifySurface, applyOverride? }`.

## 2. 검증

- `tests/gun-hit.test.mjs`(2건):
  1. `gunhit-x86.tsv` 의 `originals` 1029행(`originalCD` 1029행은 10.72 라 제외). 본체(0 Time / 1·4 SetTime / 2 Hit / 3 Apply)·저장 WORD·현재 시각 double 비트·DPS float 비트·표적 SID/상태·특수 flyer DPS·표적 genus 로 외부 효과 순서(`N` 공간 통지 / `D` 가상 피해 인자)·발사 슬롯 50바이트 raw·helper 결과 double 버퍼가 같다.
  2. 계약: 실제 피해 처리기 누락을 성공/시각 갱신으로 우회하지 않음(저장 시각 불변), 공간 통지 경계 누락·판본/표 크기·발사 SID 범위·비유한 시각 거부, Missile 연결의 `hit`/`refresh` 가 현재 `state.now` 로 같은 몸체를 부름.

최초 실패와 교정: 새 모듈은 위 fixture 1029행과 계약 검사를 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 표적의 가상 피해(+0x98)·공간 통지(004214a0)·Rain 전용 진입은 훅 경계이다. Missile 소비자(`RawMissile`)와의 실제 연결은 해당 단위 이식 뒤에 한다.

## 4. 다음 시작점

`RawLocalGeyser`(로컬 간헐천 생성), `RawGunImpact`(충돌 효과; `Clamp`는 `raw-meltdown.js` 비공개 함수와 같은 식), Missile 계열(`RawMissile`), `RawPathProcess`(RunFrame·도착·Halt; 커널/Form 장면 선행)·`RawPathStart`·`RawPathData`·`RawPathCleanup`, `RawBalloonFrame`, `RawGunGroundTarget`·`RawGunProcess`(+`MakeGunTargetHooks`), `RawPacketDispatch`, `RawTempleDraw`/캡처 계열(자산 시험 장치), `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawManaBoltArrival`(시험 장면 선행), `RawIslandRuntime` 계열, 분산 SP 정책(`ReadPlayerSpAmount`/`SetPlayerSpAmount`), 그 밖의 작은 몸체이다.
