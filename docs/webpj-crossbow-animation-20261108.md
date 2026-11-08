# webpj 크로스보우 발사(RawCrossbowAnimation) 이식 — WB04-E116

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백열여섯 번째 부분이다. 크로스보우(windArcher, TYPE 140)의 내림 발사(004674c0)와 생성자(00468050), 00466f50 의 일반 투사체(arrow81) 분기를 cpppj 기준선 `1f00a25a` 의 `o/RawCrossbowAnimation` 에서 옮겼다. 선행은 [RawGunAnimation](webpj-gun-animation-20261108.md)(`GunAnimationFields`)·[RawArcherAnimation](webpj-archer-animation-20261108.md)(공유 회전 `rotate`)·`SidPool`·`SquidProcess` 이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이다.

## 1. 옮긴 것

`www/core/raw-crossbow-animation.js` — `CrossbowAnimationState`, `RawCrossbowAnimation(pool, types, animationTypes, state, hooks)`, `CrossbowAnimationProcess`

- `initialize(fields, gunForm, target)`(00468050): 공통 생성자 접두(가상 표 0x50b120·내장 표 0x50b11c·dirty 0xff·gun/target WORD·현재 시각·원위치 방향 0)만 쓰고 Form/Parent 는 0 으로 둔다(실제 Host 부착이 채움).
- `frame(fields)`(004674c0): 시각이 도달하기 전에는 아무것도 하지 않는다(추적도 하지 않음). 도달하면 표적이 live 일 때 위치/중심을 재기록하고(중심 훅 뒤의 현재 부모/표적 바이트를 다시 읽는 live 뷰), 현재 부모 프레임이 한 방향의 첫 칸(`프레임 % 방향당 프레임 == 0`)이면 프레임을 `프레임 + 방향당 프레임 − 1` 로 바꾸고 `launch` 한 뒤 완료(통지 info 6/value 7 + 삭제 0x10)한다. launch 가 실패해도 처음 추적 당시 죽어 있던 표적이면 계속 완료하고, 살아 있던 표적이면 완료를 보류한다. 첫 칸이 아니면 프레임을 1 줄이고 dirty8 을 켜 float 지연(0.2) 뒤로 예약한다. 방향 자료(방향 수·물리 프레임 수)가 없으면 거부한다.
- `launch(fields)`(00466f50 일반 투사체 분기): live HP 표적(상태 &3 = 0, 타입 flags1 0x10)일 때만 arrow Missile 요청(속도 35·표시 81·효과 0·포구 `-10000,-10000`)을 훅에 넘긴다. 생성 경계가 없으면 성공으로 우회하지 않고 거부한다. 요청의 현재 필드/전역은 표적점 훅 뒤에 다시 읽는다.
- `CrossbowAnimationProcess(SquidProcess)`: `runFrame`·`onAttached`(Form/Parent 기록)·`fields`.
- 회전(부모 TYPE 140)은 이 모듈이 아니라 아처의 `RawArcherAnimation.rotate` 를 같은 방향 자료로 공유한다(`animationTypes[140]`). 원본 회전/생성자 몸체는 같은 fixture 로 함께 대조했다.

## 2. 검증

- `tests/crossbow-animation.test.mjs`(4건): `crossbow-animation-x86.tsv` 2638행 전부(10.78 단일 판본 — 공유 회전 812·내림 발사 1816·발사 생성자 1·회전 생성자 1·launch 8) — 몸체·부모 프레임·시각 도달·표적 상태/HP·방향 자료·표적 중심·aim 바이트·표적 WORD·재진입 입력으로 반환, 외부 효과 순서(`C` 중심/`U`·`P` 공간/`T` 표적점/`A`·`J` Missile 인자/`M` 통지/`K` 삭제/`N`·`F`·`S` 회전 소리/`B` 부착), 필드 이미지(회전 88바이트/발사 64바이트), 네 슬롯 200바이트가 같다. 계약: Missile 경계 누락을 성공으로 우회하지 않음(효과 없이 거부), 판본/표 크기/현재 TYPE 오류, 빌린 현재 TYPE, 프로세스 보조.

최초 실패와 교정: 새 모듈은 2638행 fixture 를 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 중심·공간·Missile 생성·완료 통지·삭제는 훅 경계이다. Host 연결(`MakeCrossbowAnimationHostHooks`/`CreateCrossbowAnimation`)은 `RawGunProcess` 이식 뒤에 한다.
- 크로스보우 Missile(`crossbow-missile-x86.tsv`)은 `RawMissile` 단위에서 이식한다.

## 4. 다음 시작점

`RawArcherTypeData`, `RawGunProcess`(+`MakeGunTargetHooks`), `RawLocalGeyser`, `RawGunImpact`(`Clamp`는 `raw-meltdown.js` 비공개 함수와 같은 식), Missile 계열(`RawMissile`), `RawPathProcess`(커널/Form 장면 선행)·`RawPathStart`·`RawPathData`·`RawPathCleanup`, `RawBalloonFrame`, `RawPacketDispatch`, 사제 배치 계열, `RawIslandRuntime` 계열, 분산 SP 정책(`ReadPlayerSpAmount`/`SetPlayerSpAmount`), 번개 계열, 그 밖의 작은 몸체이다.
