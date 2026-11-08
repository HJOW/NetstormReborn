# webpj 썬 캐논 애니메이션(RawGunAnimation) 이식 — WB04-E113

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백열세 번째 부분이다. Gunanim.cpp 의 썬 캐논 회전·발사·원위치 애니메이션 프레임 몸체(467130/467330/467240)와 Missile 생성 인자 작성(466f50), 수신 전용 생성자(468830/468900), 선택 필드 복원(466cc0), 가상 writer 선택(467f40/467f80)과 공통 직렬화(466ac0/466c20)를 cpppj 기준선 `1f00a25a` 의 `o/RawGunAnimation` 에서 옮겼다. 선행은 `SidPool`·`SquidProcess` 뿐이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 의 BYTE 프레임과 CD 가상 표(0x505480 등)는 제외했다.

## 1. 옮긴 것

`www/core/raw-gun-animation.js` — `GunAnimationFields`, `RawGunAnimation(pool, types, state, hooks)`, `GunAnimationProcess`, `initializeReceivedGunAnimation`, `readGunAnimationProcessData`, `gunAnimationUsesFirePacket`, `writeGunAnimationProcessData`

- `GunAnimationFields`: 원본 0x40바이트 객체 이미지(미초기화 padding 포함)와 form(+4)/parent(+8)/gunForm(+2a)/target(+2c)/time(+30 double)/restDirection(+38) 읽기이다.
- `initialize(fields, kind, gunForm, target)`: 가상 표(0x50b260 회전/0x50b318 발사/0x50b2c8 원위치)·내장 표·dirty 0xff·gun/target WORD(원위치는 표적 0)·현재 시각을 쓰고 회전이 아니면 원위치 방향을 0 으로 둔다. 좌표·padding 은 초기화하지 않는다.
- `frame(fields, kind)`:
  - **발사(SunFire)**: 표적이 살아 있으면 위치/중심을 다시 기록(`center` 훅)하고, 시각이 되면 부모 프레임 `n<24` 는 `n+24 | 0x4000`(Missile 발사 요청), 그 밖은 `n-24 | 0x8000`(완료 통지 info 6/value 7 + 삭제 0x10)으로 진행한다. 다음 간격은 float 0.07.
  - **회전(SunRotate)**: 표적/부모 중심으로 4방향을 정하고(`|dy|` 동률은 세로 우선) 28칸 5행 방향 표로 한 프레임씩 진행한다. 소리 `sunCannonRaiseLower<난수/1000>.wav` 를 부르고, 완료 표식이면 통지 info 3/value 4 + 삭제. 다음 간격은 float 0.2.
  - **원위치(SunRest)**: 저장된 원위치 방향(/2)의 행으로 진행하고 표식이면 방향을 +2 하거나(방향 8 이면) 통지 info 9/value 10 + 삭제한다.
  - 부모 프레임 변경은 항상 `unpop(parent,0)` → 프레임 DWORD 쓰기 → `repop(parent,0x40)` 순서이며 공간 훅이 없으면 거부한다. 부모는 현재 썬 캐논 TYPE 이어야 하고 프레임 범위(발사 48, 그 밖 28)를 벗어나면 오류이다.
- `launch(fields)`: live HP 표적(상태 &3 = 0, 타입 flags1 0x10)일 때만 Missile 생성 인자 12개(발사 SID·표적·속도 20·표시 80·효과 11·표적 위치·교차 레인 배리어 코너·반사 0·포구 기본)를 `createProjectile` 훅에 넘긴다. 훅이 없으면 성공으로 우회하지 않고 거부한다(투사체 생성은 `RawMissile` 이식 전).
- `initializeReceivedGunAnimation` 네 종류, `readGunAnimationProcessData`(부모 반환·선택 WORD/8바이트 복사, 앞선 부분 쓰기는 뒤 실패에도 유지), `gunAnimationUsesFirePacket`(10.78 발사 7개 표 → 0x3b, 회전/원위치 4개 → 0x3c, 미연결 표 거부), `writeGunAnimationProcessData`(부모 비영일 때만 포함·force·dirty 선택·길이 기록·dirty 소비).
- `GunAnimationProcess(SquidProcess)`: `runFrame` → `frame`, `onAttached` → Form/Parent 기록, `writePacket` → 공통 직렬화, `fields`.

## 2. 검증

- `tests/gun-animation.test.mjs`(3건):
  1. `gunanimation-x86.tsv` 의 `originals` 1280행(`originalCD` 1280행은 10.72 라 제외). 종류(회전/발사/원위치/launch)·부모 프레임·표적 점·원위치 방향·표적 상태/HP·시각 도달·dirty 입력에서 반환/Missile ctor 접두 구별, 외부 효과 순서(`C` 중심/`T` 표적점/`U`·`P` 공간/`N` 난수/`F`·`S` 소리/`M` 통지/`K` 삭제/`A`·`J` Missile 인자 12개), 0x40바이트 필드 이미지, 부모 프레임 결과가 같다.
  2. 계약: Missile 생성 경계 누락을 성공으로 우회하지 않음, free 표적 launch 거부(false), 판본/표 크기/gunForm 범위 오류, 초기화 필드, 현재 썬 캐논 TYPE 빌림.
  3. 수신 생성자 네 종류·가상 writer 선택·공통 직렬화/복원(부모·WORD·8바이트·길이·dirty 소비)·프로세스 보조.

최초 실패와 교정: 새 모듈은 1280행 fixture 를 첫 실행에 통과했다. 시험 3의 아처 회전 기대값을 최초에 0x50b0c0 으로 잘못 적었는데, 원본 생성자가 마지막에 0x50b450/0x50b448 로 덮어쓰므로 기대값을 교정했다(모듈 변경 없음).

## 3. 경계와 남은 것

- 중심 계산(004adea0)·교차 레인 배리어 코너(004571b0)·공간 해제/재등록·난수·소리·Missile 생성·완료 통지·삭제는 훅 경계이다. Host 연결(`MakeGunAnimationHostHooks`/`CreateGunAnimation`/`MakeGunAnimationProcessHooks`)은 `RawGunProcess`(GunProcess) 이식 뒤에 한다.
- 아이스 캐논 파생(`RawIceCannonFire`)·공통 대포 발사(`RawCannonFire`)·아처 애니메이션이 같은 `GunAnimationFields`/`GunAnimationHooks` 를 재사용하므로 이 모듈이 선행이다.

## 4. 다음 시작점

`RawCannonFire`·`RawIceCannonFire`(이번 모듈 위), `RawArcherAnimation`, `RawGunProcess`(+`MakeGunTargetHooks`), `RawLocalGeyser`, `RawGunImpact`(`Clamp`는 `raw-meltdown.js` 비공개 함수와 같은 식), Missile 계열(`RawMissile`), `RawPathProcess`(커널/Form 장면 선행)·`RawPathStart`·`RawPathData`·`RawPathCleanup`, `RawBalloonFrame`, `RawPacketDispatch`, 사제 배치 계열, `RawIslandRuntime` 계열, 분산 SP 정책(`ReadPlayerSpAmount`/`SetPlayerSpAmount`), 그 밖의 작은 몸체이다.
