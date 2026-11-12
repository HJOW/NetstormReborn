# webpj Missile 잔여 관측(썬더·초기 아이스·플로터 회피) 대조 — WB04-E126

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백스물여섯 번째 부분이다. 직전 [Missile 생성자·프레임](webpj-missile-20261108.md)(`RawMissile`) 단위에서 남긴 원본 관측 — 썬더 캐논(131/132)·초기 아이스 캐논(129/130)의 생성자·프레임·가상 포구·사정선 끝, 썬더×스톤 타워 충돌의 명령 큐 알림, 클라우드 플로터 회피 — 을 같은 모듈에 연결해 대조했다. 새 모듈은 없고(웹 모듈 변경 없음) cpppj 기준선 `1f00a25a` 의 `o/RawMissile` 대응 시험만 추가했다. 웹은 10.78 전용이다.

## 1. 대조한 것

`www/core/raw-missile.js`(변경 없음)의 분기 중 앞 단위의 시험이 닿지 않은 곳이다.

- **초기 아이스 캐논**(TYPE 129 → 표시 130): 이식된 `RawProjectileGeometry`(포구·hotspot·치수·사정선·방향 프레임)와 `RawGunMode`(방향 getter, 발사자 현재 프레임의 signed 문자 fallback)를 그대로 쓰고, 아이스 전용 충돌 표시(코너와 같은 2번 표시)·`iceCannonImpactN.wav` 파일명 음향(정수 난수 1000..5000)·반사 선택(`ricochet` 훅)·사거리 종료 뒤의 2번 표시를 확인한다.
- **썬더 캐논**(TYPE 131 → 표시 132): 사거리 종료 여유 10(일반 4), group1 코너 종료, 썬더 × 스톤 타워(78) 충돌의 명령 큐 알림(`queueCommand(0x020000b5, 132, 78)`; 큐 훅이 없으면 거부), 음성·코너·free/dead·실패 타입·통과/충돌 억제의 효과 순서.
- **클라우드 플로터 회피**(표적 146): 충돌 뒤 공유 게임 난수를 한 번만 소비(`randomFloat(0, 1000)` 반환 < 회피 기준 800이면 충돌 취소 + 통과 표시 1)하고, 통과 표시가 켜진 두 번째 프레임은 난수를 생략하며, 난수 훅이 없으면 선행 이동(Unpop/Pop)은 남기고 거부한다. 다섯 발사자(71·140·128·129·131)의 전체 상태를 확인한다.

## 2. 검증

- `tests/ice-missile-initial.test.mjs`(4건): `ice-missile-initial-x86.tsv` 1693행(생성자 320·프레임 1029·가상 포구 8·사정선 끝 336, 10.78 단일 판본) — 외부 효과 순서(`H`/`A`/`O`/`P`·`U`/`S`/`T`/`G`/`H`/`Q`/`I`/`N`/`F`/`R`/`X`/`B`)·128바이트 필드 이미지·세 슬롯 150바이트·포구/사정선 8바이트가 같다.
- `tests/thunder-missile.test.mjs`(7건): `thunder-missile-x86.tsv` 1118행(생성자 320·프레임 454·가상 포구 8·사정선 끝 336)·`thunder-hit-event-x86.tsv` 688행(명령 큐 알림 24건 포함, 큐 훅 누락 시 거부)·`missile-floater-x86.tsv` 481행(플로터 회피·난수 상태·두 번째 프레임의 난수 생략·누락된 난수 경계의 선행 이동).

최초 실패와 교정: 모든 행이 첫 실행에 통과했다(모듈 변경 없음).

## 3. 경계와 남은 것

- 실제 Host 연결(`MakeMissileFactoryHooks`/`CreateMissile`/`MakeGunMissileHooks`)과 호스트 자기 삭제 시험(`MissileChecking_ActualHost…`)은 이 문서 작성 때는 `SquidProcessHost`·`RawGunProcess` 이식 뒤로 미뤘고 `PORT_MAP` 의 `RawMissile` 행이 `partial` 이었다. **WB04-E361 에서 모두 끝내 `port` 가 되었다**([docs/webpj-missile-host-20261113.md](webpj-missile-host-20261113.md)).

## 4. 다음 시작점

`RawArcherTypeData`, `RawGunProcess`(+`MakeGunTargetHooks`, Missile/Animation Host 연결), `RawLocalGeyser`, `RawGunImpact`, `RawWindAviaryProcess`·`RawWindFlyer`, 비행체(`RawFlyerProcess`·`RawFlyerFrame`·`RawFlyerTarget`), 번개 수명/프레임/화면, `RawPathProcess`(커널/Form 장면 선행)·`RawPathStart`·`RawPathData`·`RawPathCleanup`, `RawBalloonFrame`, `RawPacketDispatch`, 사제 배치 계열, `RawIslandRuntime` 계열, 분산 SP 정책, 그 밖의 작은 몸체이다.
