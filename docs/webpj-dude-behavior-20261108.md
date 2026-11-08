# webpj Dude 부모 행동·정기 처리(RawDudeBehavior·RawDudeRegular) 이식 — WB04-E122

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백스물두 번째 부분이다. Dude(일반 보행 유닛, 타입 70)의 이동 정지·지면 판정·추락·도착·population 삭제 후처리(450e60/450e80/450f50/450960/450920)와 정기 분배·방황·복귀·투하·낙하·목표 이동(450fa0/451190/451210/451270/451340/4508b0)을 cpppj 기준선 `1f00a25a` 의 `o/RawDudeBehavior`·`o/RawDudeRegular` 에서 옮겼다. 선행은 `SidPool`·`SquidPostPopList`·`GameRandom`·`FrameCodeTable`·`pathDirection` 이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이다.

## 1. 옮긴 것

`www/core/raw-dude-behavior.js` — `RawDudeBehavior(pool, spots, state, hooks)`

- `handles(sid)`: 타입 70 이고 가상 표 0x508cc8 인 슬롯만 분배한다(같은 이름의 다른 파생 자산은 포함하지 않음). 나머지 getter 는 free/dead 필터 없이 타입/가상 표만 확인한다.
- `halt(sid)`: dead 이거나 공간 전환(`checkSpace`)이면 12, 계속 보행이면 10 이다. 공간 효과가 현재 Path 를 지웠을 때도 원본 12 를 그대로 반환한다.
- `checkSpace(sid)`: dead 면 false, 현재 task 가 추락(0x5105)이면 true, 아니면 현재 좌표를 거의 올림(+0.9999f 절삭, 지도 밖은 `RangeError`)한 칸의 지도 비트 6(표면)이 없으면 `fall`(task 0x5105 + 이동 삭제) 하고 true, 비트 8(건물 점유)이면 task 0x5106 + 이동 삭제 후 true, 아니면 false 이다. 이동 삭제는 task 효과 뒤 현재 타입으로 이동 종류를 다시 골라 조회/삭제한다.
- `arrival(sid)`: free/dead 면 1001, task 별로 대기(0x5101)는 진단 모드면 진단 0x153, 낙하(0x5105)는 진단 0x157 을 부른 뒤(복귀하면 계속) 부모 삭제하고 1001, 복귀(0x5103)/투하(0x5104)/알림(0x5107)도 삭제 후 1001, 보행(0x5102)은 삭제 없이 1000, 알 수 없는 task 는 진단 0x15b 뒤 1000 이다.
- `postDestroy(sid, flags)`: 슬롯 +40 의 비트 9(0x9)가 없을 때 population 에서 같은 항목을 모두 압축 제거(`SquidPostPopList.removeAll`; 남은 항목 순서와 count 뒤 stale 꼬리 보존)하고 공통 후처리를 부른다.

`www/core/raw-dude-regular.js` — `RawDudeRegular(pool, frames, random, state, hooks)`, `initializeDudePopulation`

- `initializeDudePopulation(population)`: 원본 static initializer 의 30칸/count 0 을 빈 목록에 준비한다(중복 초기화는 거부).
- `handle(sid, event, count, payload)`: poll 사건이면 0, 슬롯이 free 이거나 사건이 0x51xx 가 아니면 진단 모드에서 진단 399 후 payload 를 돌려준다. 그 밖에는 사건이 0x5105 가 아니면 지면 판정이 참일 때 낙하로 가고, 현재 경로 종속이 있고 count 가 0 이 아니며 사건이 0x5107 이 아니면 payload 를 돌려준다. 이어 task WORD 를 쓰고 사건별(대기 0x5101=진단 0x179 후 방황·보행 0x5102=방황·복귀 0x5103·투하 0x5104·추락 0x5105·막힘/알림 0x5106/0x5107[count>10 이면 삭제, 알림이고 경로가 없으면 빠른 방황, 반환 0.2])로 분배한다.
- `wander`: count 가 0 이 아니면 확률 관문(게임 난수 100 미만 10)을 통과해야 공급자 active 목록을 순환(난수 1을 한 번 소비해 순환 값 +1)해 목표로 이동을 시도하고, 실패하면 task 0x5103 을 걸고 0.1 을 돌려준다(이동 성공이면 10).
- `return`/`drop`/`fall`/`moveTo`: 복귀(count 5 이상이면 삭제 -1, 아니면 가장 가까운 거주 구역으로 이동해 성공 10/실패 2), 투하(count 가 0 이 아니면 진단 0x127 후 1, 0 이면 원래 위치를 보존하고 거주 구역 진입점으로 옮긴 뒤 원래 위치로 걸어 나오며 성공 1, 아니면 삭제 -1), 낙하(dead 면 0, count 100 초과 삭제 -1, count 10 에서 슬롯 BYTE 35 를 20 으로, 단계 8 이면 프레임을 A 구간 첫 칸으로, y 를 `(count×0.5+1)×0.0625+y`(float, 상한 254)로 올리고 현재 단계 Run 안에서 난수로 프레임을 정해 쓴 뒤 Pop(0x40), 반환 0.05), 목표 이동(목표 0 이면 false, 죽은 목표는 진단 0xde 뒤 계속).
- 모든 float 반환은 원본 단정도 저장값이며(`0.1`/`0.05`/`0.2` 는 float 로 반올림), 진단 모드(`state.checking`)는 불리언 또는 호출 시점에 읽는 함수이다.

## 2. 검증

- `tests/dude-behavior.test.mjs`(2건): `dude-behavior-x86.tsv` 410행 전부(10.78 단일 판본, 두 x87 정밀도 열 동일) — 몸체(Halt/CheckSpace/Fall/Arrival/PostDestroy)·상태 바이트·task WORD·지도 비트·진단 모드·population 패턴(raw+40 비트 포함)으로 반환값, 효과 순서(task/type/lookup/destroy/post/assert), population count 와 count 뒤 stale 꼬리까지 포함한 5개 DWORD 가 같고 슬롯 raw 는 변하지 않는다. 계약: 판본·지도 크기·다른 파생 자산·미연결 하위 효과 거부.
- `tests/dude-regular.test.mjs`(2건): `dude-regular-x86.tsv` 1600행 전부(정기 분배·방황·복귀·투하·낙하·목표 이동 1597 + 정적 초기화 3, 10.78 단일 판본). 17개 입력(상태·사건·count·payload·공급자 수·순환 값·난수·프레임·지면/경로/거주/이동 결과·재진입 변경)으로 반환 float 비트, 큰 효과의 인자 순서(space/lookup/task/nearest/point/move/frame/destroy/unpop/pop/assert), 슬롯 50바이트 전체, 난수 상태, 순환 값이 같다(NaN payload 는 같은 부류로 비교). 계약: 판본·프레임 표 크기 거부, 다른 파생 자산 거부, 미연결 효과 거부, 중복 초기화 거부.

최초 실패와 교정: 새 두 모듈은 410행·1600행 fixture 를 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 지면 판정 이후의 task 재예약(450670)·이동 삭제 조회(4b1a90)·거주 구역 목록(48fd30)·진입점(4505a0)·이동 시작(450760)·가상 공간/프레임(4acee0)·삭제는 훅 경계이다. Dude 생성/알림/투하(`RawDudeSpawn`·`RawDudeNotice`·`RawDudeDrop`)와 경로(`dude-path*`)는 별도 단위이다.

## 4. 다음 시작점

`RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면(`RawLightningLifetime`·`RawLightningFrame`), `RawArcherTypeData`, `RawGunProcess`(+`MakeGunTargetHooks`), `RawLocalGeyser`, `RawGunImpact`, Missile 계열(`RawMissile`), `RawPathProcess`(커널/Form 장면 선행)·`RawPathStart`·`RawPathData`·`RawPathCleanup`, `RawBalloonFrame`, `RawPacketDispatch`, 사제 배치 계열, `RawIslandRuntime` 계열, 분산 SP 정책, 그 밖의 작은 몸체이다.
