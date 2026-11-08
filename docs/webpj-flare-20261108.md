# webpj 플레어(RawFlare) 이식 — WB04-E110

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백열 번째 부분이다. 연구·업그레이드 효과가 사용하는 원본 플레어(106) 생성(45a6c0→45a480)과 Regular 수명(45a250)을 cpppj 기준선 `1f00a25a` 의 `o/RawFlare` 에서 옮겼다. 선행은 E62 의 `raw-path-animation.js`(`pathDirection`), `FrameCodeTable`(`assets/terrain-tiles.js`), `GameRandom`(`random.js`)이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이다.

## 1. 옮긴 것

`www/core/raw-flare.js` — `RawFlare(pool, types, frames, random, state, hooks)`

- `handles(sid)`: 가상 표가 플레어 vtable 0x5096a0 인 객체만 처리한다.
- `spawn(parent, count, event)` / `spawnDirection(parent, count, event, effectType, direction)`: 억제 시각(005c85f0)이 0 이 아니거나 로딩 깊이가 있으면 아무 것도 하지 않는다. 아니면 부모 깊이(+35)와 Kernel 개수를 한 번 읽고 count(signed)번 반복한다(Kernel 개수가 30000 초과면 중단). 매번 클라이언트 효과 생성(`create(type, 2)`) → 부모 현재 좌표와 SHP 핫스팟(`sourceHotspot`; 폭/높이가 아님)으로 기준점 난수 범위(부모가 이동/비행 genus 0x30000 이면 오프셋 −0.7/−0.1)를 만들어 X·Y 난수 두 개를 뽑고, 효과의 프레임 DWORD(+36)를 방향 글자(A + direction)의 첫 프레임으로 직접 쓰고 소유자 0·extra bit3·깊이−1 로 설정한 뒤 [1, 255] 로 보정한 `부모 − 난수` 좌표에 배치(`pop`)한다. 예약(`reserveRegular`)이 성공하면 `난수(0..0.07)+0.05`초 주기의 Regular 를 붙인다(`addRegular`). `spawn` 은 기본 A 구간(현재 효과 타입), `spawnDirection` 은 주문의 C/D 방향이다.
- `handle(sid, event, count, payload)`: 현재 프레임 방향 글자 구간(first/count)을 구한다. 단계 count==0 이면 주기를 난수(0.05..0.13)로 새로 정한다. event>1 이고 마지막 전 단계이면 Kernel 개수가 30000 초과가 아닐 때 후속 플레어를 부모 좌표(+0.5 절삭, ±0.4 난수)에 만들고 구간 첫 프레임으로 배치해 `event−1` 의 Regular 를 붙인다. 단계가 구간 끝이면 마지막 그림 갱신(`update`)과 삭제(`destroy`) 뒤 −1(종료)을 반환한다. 아니면 프레임을 `구간 첫 번호 + 단계` 로 갱신(`frame`)하고, 구간 절반 단계에서 방향이 B 이면 위치 소리(`powerup.wav`)를 낸 뒤 주기를 반환한다. count/event 는 원본 signed int 비교이다.
- `handler(fallback)`: 플레어 부모만 처리하고 나머지 부모는 기존 실제 처리기(`fallback`)로 전달하는 Regular 분배기이다(처리기가 없으면 거부).
- 난수는 `난수(65536)/65536 × |high−low| + low`(53비트) 식이고 모든 효과 뒤 현재 슬롯/좌표를 다시 읽는다. 도달한 훅이 없으면 오류이다.

## 2. 검증

- `tests/flare.test.mjs`(3건):
  1. `flare-x86.tsv` 139행(Spawn 30 + Regular 109, 10.78 전용). Spawn: 억제 double 비트·로딩 깊이·Kernel 개수·부모 genus·좌표·깊이·SHP 핫스팟·난수 시드·예약 성공 비트, Regular: 프레임 단계·Kernel·좌표·시드·예약·payload → 반환 float 비트·게임 난수 상태·부모(50)와 열 개 후보(100..109) 슬롯 전체 raw·호출 순서(`K` Kernel 개수 / `C` 생성 / `H` 핫스팟 / `O` 소유자 / `P` 배치 / `R` 예약 / `A` Regular 부착 / `F` 프레임 / `U` 갱신 / `D` 삭제 / `S` 소리)가 같다.
  2. `bomb-status-flare-x86.tsv` 60행 — 같은 Spawn 입력 30개를 C/D 방향으로 각각 실행한 난수 상태·열한 슬롯 raw·예약 인자 호출 순서가 같다.
  3. 계약: 가상 표가 다른 부모의 Regular 는 기존 처리기로 전달하고 처리기가 없으면 거부, 판본/TYPE·프레임 표 크기 거부, 억제/로딩 시 훅 미요구.

최초 실패와 교정: 새 모듈은 위 fixture 199행과 계약 검사를 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 효과 생성(4af530)·SHP 핫스팟 조회(419850)·소유자 지정(4ac220)·가상 배치/갱신/삭제·Regular 예약·프레임 표시 갱신(4acee0)·위치 소리 재생은 훅 경계이며 해당 모듈 이식 뒤에 연결한다. 플레어를 만드는 연구/업그레이드 호출부(`RawFactoryUpgrade` 등)는 후속 단위이다.

## 4. 다음 시작점

`RawLocalGeyser`(로컬 간헐천 생성), `RawMeltdown`(선행 `RawBombTargets`/`RawGunImpact`), `RawPathProcess`(RunFrame·도착·Halt; 커널/Form 장면 선행)·`RawPathStart`·`RawPathData`·`RawPathCleanup`, `RawBalloonFrame`, `RawGunGroundTarget`·`RawGunProcess`(+`MakeGunTargetHooks`), Missile 계열, `RawPacketDispatch`, `RawTempleDraw`/캡처 계열(자산 시험 장치), `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawManaBoltArrival`(시험 장면 선행), `RawIslandRuntime` 계열, 분산 SP 정책(`ReadPlayerSpAmount`/`SetPlayerSpAmount`), 그 밖의 작은 몸체이다.
