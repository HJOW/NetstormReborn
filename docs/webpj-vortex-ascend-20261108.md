# webpj 신전 Ascend(RawVortexAscend) 이식 — WB04-E120

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백스무 번째 부분이다. 신전(Vortex)의 Ascend 이벤트 0 본체(004db8e0)·반경별 적 자산 삭제 후보 판정(004db680)·외부 소유 섬 수(0046f480)를 cpppj 기준선 `1f00a25a` 의 `o/RawVortexAscend` 의 섬 배열 경로에서 옮겼다. 선행은 `SidPool`·`SquidHash`·`RawSquidFinder`·`GameRandom` 이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이다.

## 1. 옮긴 것

`www/core/raw-vortex-ascend.js` — `RawVortexAscend(pool, hash, spots, types, islands, random, state, hooks)`

- `candidate(sid, island, owner)`(004db680): 후보 TYPE 의 genus 가 제외 묶음(`0x128b302`: 표면/내용물/유닛)이면 거절하고, 섬 조회 훅의 현재 섬 번호가 다르면 거절한다. 이어 후보 좌표(float → ftol 절삭, 지도 밖은 `RangeError`)의 spot 비트 2 가 있으면 표면 0단계 지지 자산이 있고 현재 noIsland 타입(`state.noIslandType` 또는 빌린 `sharedNoIslandType.value`)이 아니어야 하며, 없으면 후보의 **현재** TYPE flags1 의 `0x200000` 이 있어야 한다. 마지막으로 후보의 소유자가 `owner` 와 달라야 한다. 섬 조회 훅 뒤 spot·타입·소유자를 원본 순서로 다시 읽는다.
- `tick(temple, count)`(004db8e0): 반경은 `count`(signed)이며 32 초과는 64 이다. 신전 섬 번호를 조회하고, 반경 finder 생성자처럼 신전 좌표(훅 뒤 다시 읽음)에 float `0.99999` 를 더해 절삭한 뒤 반경으로 사각형을 만들어 일반 finder 를 시작한다. 후보 버퍼를 확보(훅)한 뒤 최대 20개 후보를 모으며(실패는 첫 후보에서 거부) 20개가 되면 난수 소비 없이 중단한다. 후보가 있으면 게임 난수(limit = 후보 수) 한 번으로 하나를 flags `0x100000` 으로 삭제한다. 반환은 후보가 있었거나 반경이 64 가 아니면 true(최종 64 의 빈 반경이면 false)이며, 버퍼 해제는 예외에도 실행한다. 현재 신전 소유자는 후보마다 신전 raw 에서 다시 읽는다.
- `countForeignOwnedIslands(owner)`(0046f480): 존재하며 소유자가 0 도 `owner` 도 아닌 섬의 수이다. 섬 항목은 `{ exists, owner }`(원본 IslandList 의 존재 DWORD/+0xc 소유자).
- 훅 `{ islandNumber(x, y), reserveCandidateBuffer(), releaseCandidateBuffer(), destroy(sid, flags) }` 는 모두 필수이며 하나라도 없으면 생성자가 거부한다. 판본/타입 표 크기(188)/spot 지도 크기(65536)도 생성자에서 검사한다.

## 2. 검증

- `tests/vortex-ascend.test.mjs`(2건):
  1. `vortexascend-x86.tsv` 의 `originals` 748행(`originalCD` 748행은 10.72 라 제외). Candidate/Tick/Count 진입점과 저장된 입력 격자(genus·flags·spot·지지 타입·섬 번호·소유자·난수 시드·후보 수·버퍼 확보 성공·변화)로 반환, 게임 난수 상태, 외부 효과 순서(`I` 섬 조회/`N` 버퍼 확보/`F` 해제/`D` 삭제), 슬롯 60..99 의 상태 바이트, 신전 소유자가 같다.
  2. 계약: 후보 0 인 32→64 끝이 거짓이며 난수를 쓰지 않음, 필수 훅 누락/판본/spot 지도·타입 표 크기 거부, 빌린 noIsland 타입, 소유 섬 수.

최초 실패와 교정: 새 모듈은 748행 fixture 를 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 실제 월드의 `RawIslandRuntime`(섬 개수·존재·소유자 DWORD)을 직접 읽는 두 번째 생성자(`VortexIslandBindingTests`, `vortex-*` 계열 나머지 fixture)는 `RawIslandRuntime` 이식 뒤로 둔다. 섬 번호 조회(`getIslandId`)·후보 버퍼·삭제(vtable+10)는 훅 경계이다.
- 같은 신전의 소유자 진단/표시/삭제 전(`vortex-owner*`·`vortex-display`·`vortex-pre`·`vortex-current` fixture)은 `RawTempleRitual`/`RawVortexAscend` 어댑터 단위에서 이식한다.

## 4. 다음 시작점

번개 수명/프레임/화면(`RawLightningLifetime`·`RawLightningFrame`), `RawArcherTypeData`, `RawGunProcess`(+`MakeGunTargetHooks`), `RawLocalGeyser`, `RawGunImpact`, Missile 계열(`RawMissile`), `RawPathProcess`(커널/Form 장면 선행)·`RawPathStart`·`RawPathData`·`RawPathCleanup`, `RawBalloonFrame`, `RawPacketDispatch`, 사제 배치 계열, `RawIslandRuntime` 계열, 분산 SP 정책, 그 밖의 작은 몸체이다.
