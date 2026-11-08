# webpj 아이스 타워(RawIceTower) 이식 — WB04-E109

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백아홉 번째 부분이다. 아이스 타워(147)의 파괴(48edf0)와 성장형(150)의 최초 예약(48ead0)·성장 이벤트(48ebe0)·삭제(48eb80)를 cpppj 기준선 `1f00a25a` 의 `o/RawIceTower` 에서 옮겼다. 선행은 `SidPool` 뿐이다(프레임 표는 `{ codes }` 모양의 FrameCodeTable). 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이다.

## 1. 옮긴 것

`www/core/raw-ice-tower.js` — `postPopGrowingIce`, `RawIceTower(pool, frames, types, battle, boss, loadingDepth, hooks)`

- `postPopGrowingIce(pool, frames, parent, flags, reserve, regular, base)`(48ead0): 최초 flags bit0 이고 예약(`reserve`)이 성공하면 현재 TYPE 프레임 수로 `40/프레임 수`(float)초 주기의 이벤트 0x3a6 을 부착(`regular`)하고, 어느 경우든 마지막에 공통 후처리(`base`)를 한다. 예약이 실패해도 공통 후처리는 막지 않으며 extra/editor 관문을 추가하지 않는다.
- `postPop(parent, flags)`: 위 후처리를 훅으로 연결한다.
- `postDestroy(parent, flags)`(48edf0): 완성형(147)이 extra&9 없음·삭제 flags 0x200000 없음·로딩 깊이 0 이면 표면 칸(`grid`, 절삭 칸 첫 SID)을 조회해 받침 없는 섬 TYPE(157)이고 flags 0x8000000 이면 받침 정리(`dropIsland`) 뒤 공통 후처리(`damageablePost`)로 끝낸다. 아니면 battle 일 때 공간 해제(`unpop`)→성장형(150) 생성→현재 WORD(+12)·zOrder(+35)·소유자(+34) 복사→프레임 0→현재 좌표로 재배치(`pop`)한다. 각 효과 뒤의 현재 슬롯/전역을 다시 읽고 마지막에 공통 후처리이다.
- `growingPostDestroy(parent, flags)`(48eb80): 성장형은 extra&9 없음이고 flags 에 0x8200000 비트가 있으면 현재 좌표의 일반 finder 조회(`genusAt`, genus 0x1000000)가 있을 때 받침 정리를 하고 마지막에 공통 후처리(`basePost`)이다.
- `handle(parent, event, count, payload)`(48ebe0): 이벤트 0x3a6 만 처리한다. extra bit5 가 없으면 프레임 DWORD+1 을 signed 로 `프레임 수−1` 과 비교해 작으면 프레임 표시 갱신(`setFrame`)이고, 아니면 boss 일 때 완성형(147)을 생성해 소유자를 넘기고 자신을 삭제(`destroy`)한 뒤 현재 좌표에 재배치하며 −1(반복 종료)을 반환한다. 그 밖에는 다음 주기 `40/프레임 수`(float)를 반환한다. count 는 쓰지 않는다.
- 도달한 훅이 없으면 오류이다(`reserveRegular·regular·basePop·damageablePost·basePost·grid·genusAt·dropIsland·create·unpop·destroy·owner·pop·word·setFrame`). CD 판본·188칸이 아닌 프레임 표는 거부한다.

## 2. 검증

- `tests/ice-tower.test.mjs`(2건):
  1. `ice-tower-x86.tsv` 1426행(10.78 전용). 종류 P(성장형 최초 예약)/R(성장 이벤트)/D(완성형 파괴)/G(성장형 삭제)와 입력(flags·extra·프레임 수·할당 성공·boss·프레임·TYPE·로딩·battle·표면·섬 존재·이벤트·변이 0~12)으로 외부 효과 순서(`N` 예약 40 / `R` 이벤트 부착 / `P·D·B` 공통 후처리 / `Q` 표면 칸 / `G` 일반 finder / `J` 받침 정리 / `C` 생성 / `U` 공간 해제 / `K` 삭제 / `O` 소유자 / `V` 재배치 / `T` 다리 통지 / `F` 프레임 갱신), 부모(50)·표면(60)·생성물(61) 세 슬롯 전체 raw, 반환 float 비트가 같다.
  2. 계약: 성장형 예약(최초 flag 유무·예약 실패 시 공통 후처리만)·판본/프레임 표 크기 거부·누락 훅의 지연 거부.

최초 실패와 교정: 새 모듈은 위 fixture 1426행과 계약 검사를 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 실제 예약 메모리(`Host.AddSharedRegular`)·표면 칸 조회(40eaf0)·일반 finder(4b2070)·생성(4af530)·가상 공간 해제/재배치/삭제·소유자 지정·받침 정리(46e6c0)·공통 후처리(44b9e0/4b0840/4b0d30)·프레임 표시 갱신(4acee0)은 훅 경계이며 해당 모듈 이식 뒤에 연결한다. 아이스 타워의 가상 표 연결(타입 147/150 생성자)은 `squid-constructors.js` 쪽이다.

## 4. 다음 시작점

`RawLocalGeyser`(로컬 간헐천 생성), `RawMeltdown`·`RawFlare`(선행 `RawBombTargets`/`RawGunImpact`), `RawPathProcess`(RunFrame·도착·Halt; 커널/Form 장면 선행)·`RawPathStart`·`RawPathData`·`RawPathCleanup`, `RawBalloonFrame`, `RawGunGroundTarget`·`RawGunProcess`(+`MakeGunTargetHooks`), Missile 계열, `RawPacketDispatch`, `RawTempleDraw`/캡처 계열(자산 시험 장치), `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawManaBoltArrival`(시험 장면 선행), `RawIslandRuntime` 계열, 분산 SP 정책(`ReadPlayerSpAmount`/`SetPlayerSpAmount`), 그 밖의 작은 몸체이다.
