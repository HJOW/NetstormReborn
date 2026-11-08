# webpj 건설 접근점·특수 피해(RawBoardApproach·RawSpecialDamage) 이식 — WB04-E104

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백네 번째 부분이다. 건설 사제의 Graph 접근점 탐색(41db70)과 신전의 건설 중 피해 분배(4dba40)·윈드 타워의 방향 방어(48ef70)를 cpppj 기준선 `1f00a25a` 의 `o/RawBoardApproach`, `o/RawSpecialDamage` 에서 옮겼다. 선행은 이미 이식된 `RawSquidNeighbors.direction`(`raw-squid-neighbors.js`)뿐이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이다.

## 1. 옮긴 것

`www/core/raw-board-approach.js` — `RawBoardApproach(graph, assertion)`

- `find(rectangle, x, y, flags, checking)`: 운반 명령의 일반 접근점(41d530/41d400, E100 `raw-board-target.js`)과 다른 건설 사제의 Graph 순회이다. 사각형 [left, top, right, bottom](단정도, 제자리 수정)의 중심을 향한 출발점 (x, y)의 방향을 4 칸 돌려 고른 변 쪽 중간점에서 시작하고, 출발점의 Graph 번호(`graph(x, y)`; 254 도 일반 동등 비교)를 기억한다. flags bit2 이면 사각형을 1 만큼 넓히고 점을 1~255 로 보정하며 flags bit1 이고 북쪽 방향이면 x 를 중심 기준 좌/우 끝으로 옮긴 뒤 사각형 안으로 다시 보정한다. 이어 사각형 **면적(절삭 DWORD)** 번까지 같은 Graph 번호를 찾을 때까지 가장자리를 시계 방향으로 한 칸씩 순회한다(점이 가장자리 밖이면 checking 일 때 진단을 부르고 왼쪽 위로 복귀). 찾으면 사각형을 원래 값으로 복구하고 그 점을 돌려준다. 못 찾으면 사각형을 1 씩 안쪽으로 줄인 새 사각형의 면적이 4 초과일 때만 같은 방식으로 재귀 탐색하고, 아니면 마지막 점을 돌려준다(사각형은 확장된 채 남는다). 좌표가 유한하지 않거나 사각형이 거꾸로면 거부한다. checking 은 값/getter 로 호출 시점마다 현재 값을 읽는다.

`www/core/raw-special-damage.js` — `rawVortexDamage`, `rawWindTowerDefense`

- `rawVortexDamage(pool, victim, damage, owner, attacker, hooks)`(4dba40): 피해자 extra bit0(건설 중)이면 현재 건설 프로세스(30) 전역을 호출 시 읽어 같은 건설 Form 을 찾고(`find(피해자, 건설 타입, 0, false)`) 찾았을 때만 끝 시각을 피해 비트(DWORD)로 바꾸고(`resetEnd`) 같은 신전의 표시를 갱신(`refresh`)한다. 찾지 못해도 건설 중이면 항상 true 이다. 건설이 끝난 신전은 기존 일반 피해(`general`)의 반환을 그대로 돌려준다.
- `rawWindTowerDefense(pool, types, frames, victim, attacker, arcSpireType = 149)`(48ef70): 공격자(0 도 현재 풀 슬롯의 TYPE/genus 로 읽음)의 flags2 에 0x100100(flyer/balloon)이 있거나 공격자 TYPE 이 아크 스파이어이면 방어하지 않는다(피해자는 읽지 않음). 아니면 피해자와 공격자의 발자국 중심(좌표 − 발자국/2, float 저장)을 비교한 네 방향(0 북/2 동/4 남/6 서; 동률은 세로 우선)이 피해자의 현재 프레임 코드의 방향 글자(L/M/N/O)와 일치할 때만 방어한다. `frames[type]` 은 `{ codes: [{ side }] }`(FrameCodeTable 모양)이다.

## 2. 검증

- `tests/board-approach.test.mjs`(2건): `board-approach-x86.tsv` 696행(두 정밀도에서 동일). 입력 9 DWORD(사각형 네 float·출발점 x/y·flags·Graph 응답 모드 0..5·checking)로 종료 뒤 사각형 네 float 의 전체 비트(복구 또는 flags 4 확장 유지)·반환 점 두 float 비트·외부 효과 순서(`G:x:y:Graph` 모든 Graph 조회, `A:565` 가장자리 진단)가 같다. 계약: 유한하지 않은 좌표·거꾸로 된 사각형·Graph 조회 미연결 거부.
- `tests/special-damage.test.mjs`(3건): `special-damage-x86.tsv` 432행(연산 0 신전 피해·연산 1 윈드 타워 방어의 반환과 외부 효과 순서 F/C/R/H), `wind-tower-zero-x86.tsv` 216행(예약 피해의 공격자 SID 0 도 원본처럼 현재 풀 0 번 슬롯의 TYPE/좌표를 읽음), 무효 SID/판본·TYPE 범위·미연결 하위 거부 계약.

최초 실패와 교정: 새 모듈은 위 fixture 1344행과 계약 검사를 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 건설 사제 배치(`RawConstructionPlace` 계열)가 이 접근점을 쓰며 Graph 번호 조회(462b80)는 훅이다. 신전 건설 Form 조회(4b1a90)·끝 시각 변경(443dc0)·일반 피해(44c8e0)·표시 갱신은 훅 경계이다. 윈드 타워 방어의 호출부(윈드 타워 피해)는 `RawGunHit`/Damageable 이식 단위에서 연결한다.

## 4. 다음 시작점

`RawPathProcess`(RunFrame·도착·Halt; 커널/Form 장면 선행)·`RawPathStart`·`RawPathData`·`RawPathCleanup`, `RawBalloonFrame`, `RawGunGroundTarget`·`RawGunProcess`(+`MakeGunTargetHooks`), Missile 계열, `RawTempleDraw`/캡처 계열(자산 시험 장치), `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawManaBoltArrival`(시험 장면 선행), `RawIslandRuntime` 계열, 그 밖의 작은 몸체(`RawGemReward`·`RawObjectConversion`·`RawForeignRitualReward`·`RawWorldGeyser` 등)이다.
