# webpj 제네레이터 Meltdown(RawMeltdown) 이식 — WB04-E111

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백열한 번째 부분이다. 제네레이터(Whirligig 계열)의 Meltdown 명령(41c290)·자격(420340)·검색(4203a0)·피해(4204f0)·폭발 패킷 수신(4205f0/4207a0)·패킷 길이 확정(40b150)을 cpppj 기준선 `1f00a25a` 의 `o/RawMeltdown` 에서 옮겼다. 선행은 이미 이식된 `RawSquidFinder`·`SquidHash` 이다(주문 대상 검색 `RawBombTargets` 는 WB04-E54 에서 이미 이식됨). `RawGunImpact::Clamp` 는 같은 식을 `raw-meltdown.js` 안에 비공개 함수로 두었고 `RawGunImpact` 이식 때 옮긴다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이다.

## 1. 옮긴 것

`www/core/raw-meltdown.js` — `RawMeltdown(pool, hash, types, frames, state, hooks)`

- `command(parent, code, base)`: 명령 코드(상위 입력 비트 제거)가 0x030000f8 이면 헤더(요청)+부모 SID(WORD)의 6바이트를 송신(`send(9, packet)`)하고, 어느 경우든 이어서 공통 부모(`base`)를 호출한다(반환값 없음).
- `mayTarget(target)`: 다리(genus&4)는 보호 프레임 비트 0x40 만 제외한다(SID 0 은 허용). 그 밖의 대상은 HP 보유 여부(`hasHitPoints`)와 허용 genus 마스크(0x80074200)를 쓴다.
- `find(x, y, radius, output)`: 권한이 없으면 진단(0x664). 반경 사각형(내림/올림 스냅·1~255 제한)을 정수 영역으로 같은 네 단계 해시를 훑어, 예약 대역·extra bit0·거리 ≥ 허용(발자국/2+반경, 엄격 `<`)·자격 미달 후보를 거르고 목록 한도까지 담는다(dead 는 여기서 거르지 않음).
- `apply(x, y, radius, owner, target, parent)`: dead 대상은 무시. 비다리는 동맹 표(공격자×9+대상)가 0 이면 피해(`damage(target, 100, owner, parent)`)와 피해 전에 캡처한 대상 중심의 지면 효과(`ground`)를 한다. 다리는 중심 거리가 `발자국/2+반경−1` 이하이면 삭제(0x100048), 아니면 서버(boss)일 때 다리 끝 칸 변환(`edge`)이다.
- `receiveRequest(packet)`: 부모(패킷 WORD +4)가 dead 면 무시, 권한이 없으면 진단(0x6c3). 최초 중심에서 반경 4 로 검색한 뒤 **현재** 부모 중심/소유자로 220바이트 효과 패킷(검색 결과 전체를 WORD 로 복사)을 만들어 배포(`broadcast`)·로컬 수신(`receive(현재 로컬 owner, 0, 패킷)`)·부모 삭제(`destroy(parent, 0)`)한다.
- `receiveEffect(packet)`: 중심을 스냅해 0.5 를 빼고 세 위치에 시각 효과(variant 1, 0.04)와 소리(`whirligigFire-1000.wav`)를 먼저 낸 뒤, 패킷의 현재 대상 수(signed)와 대상 WORD 를 원문 위치에서 다시 읽어 dead/삭제 대상을 건너뛰고 `apply` 한다. `RawMeltdown.finalizePacket(packet, writing)`: 송신일 때만 count==−1 진단(오류) 뒤 선언 길이 WORD 를 `(count+10)×2` 로 확정한다.
- 도달한 훅이 없으면 오류이다. 동맹 표 81칸·188 TYPE/프레임 표가 아니면 거부한다.

## 2. 검증

- `tests/meltdown.test.mjs`(2건): `meltdown-x86.tsv` 442행(두 정밀도 열 동일). 입력 23 DWORD(연산 0 명령/1 자격/2 피해/3 요청 수신/4 효과 수신/5 패킷 확정·genus·HP·프레임 플래그·상태·extra·발자국·좌표·반경·owner·동맹·권한·헤더·변이)로 반환·외부 효과 순서(`hp/assert/find/send/broadcast/receive/destroy/damage/ground/edge/visual/sound/base`)·대상 슬롯(60)·220바이트 효과 패킷이 같다. 계약: 실제 검색의 엄격한 거리/예약 SID/abstract/dead/한도와 같은 해시 순서.

최초 실패와 교정: 새 모듈은 위 fixture 442행과 계약 검사를 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 실제 파생 가상 몸체(반경 +b4·자격 +bc)·HP 보유 조회(4adc60)·송신/배포/수신(4e2dc0/4e2e30)·삭제·피해(가상 98)·지면 효과(470ac0)·다리 끝 칸 변환(421f70)·시각 효과(45a750)·소리는 훅 경계이다. 폭발 주문의 다른 파생(`RawBombPostPop` 등 기존)과의 연결은 해당 단위에서 한다.

## 4. 다음 시작점

`RawLocalGeyser`(로컬 간헐천 생성), `RawGunImpact`(충돌 효과; `Clamp`를 이 모듈에서 옮김), `RawPathProcess`(RunFrame·도착·Halt; 커널/Form 장면 선행)·`RawPathStart`·`RawPathData`·`RawPathCleanup`, `RawBalloonFrame`, `RawGunGroundTarget`·`RawGunProcess`(+`MakeGunTargetHooks`), Missile 계열, `RawPacketDispatch`, `RawTempleDraw`/캡처 계열(자산 시험 장치), `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawManaBoltArrival`(시험 장면 선행), `RawIslandRuntime` 계열, 분산 SP 정책(`ReadPlayerSpAmount`/`SetPlayerSpAmount`), 그 밖의 작은 몸체이다.
