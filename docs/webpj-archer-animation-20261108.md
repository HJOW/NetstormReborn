# webpj 아처 애니메이션(RawArcherAnimation) 이식 — WB04-E115

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백열다섯 번째 부분이다. 썬 디스크 쓰로워(SunArcher71)의 회전(00467940)과 각속도 발사(00467c80)·sunDisc83 생성 인자(launch)·방향표/초기각/발사 프레임 helper·생성자 접두를 cpppj 기준선 `1f00a25a` 의 `o/RawArcherAnimation` 에서 옮겼다. 선행은 [RawBoardAngle](webpj-board-angle-20261108.md)(`normalizeBoardAngle`)·`SidPool`·`SquidProcess` 이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 의 BYTE 프레임/가상 표(0x5054d0 등)는 제외했다.

## 1. 옮긴 것

`www/core/raw-archer-animation.js` — `ArcherAnimationFields`, `ArcherAnimationState`, `RawArcherAnimation(pool, types, animationTypes, state, hooks)`, `ArcherAnimationProcess`

- `ArcherAnimationFields`: 캐논의 64바이트와 구별되는 원본 0x58바이트 이미지(미초기화 padding 포함). form(+4)/parent(+8)/gunForm(+2a)/target(+2c)/time(+30 double), dirty(+28), 단계(+38), 각(+40 float)·속도(+44)·가속도(+48)·스프링 상태(+4c)·발사 상태(+50)이다(위는 모두 16진 오프셋).
- `initialize(fields, fire, gunForm, target)`: 공통 생성자 접두(가상 표 0x50b1c0 발사/0x50b450 회전, 내장 표 0x50b1bc/0x50b448, dirty 0xff, gun/target WORD, 현재 시각, 발사면 단계 +38 을 0)만 쓴다. `initializeFire(fields)`: 부착 뒤 초기 각(부모 프레임→22.5도 단위)·속도·가속도·단계 0 을 쓰고 출력 WORD bit0 을 해제한 뒤 공간 통지를 부른다.
- `rotate(fields)`(00467940): 표적 0 이면 `aimDirection × 90`, 아니면 표적을 추적(live 이면 위치/중심 재기록)해 부모→표적 각을 구하고 가장 가까운 20칸 비균등 방향표로 한 방향 칸씩 회전한다(시각 도달 시). 같으면 완료(통지 info 3/value 4 + 삭제 0x10), 아니면 프레임을 바꾸고 float 0.1 뒤로 예약하며 소리 `sunDiscThrowerRatchet0<난수/1000>.wav` 를 낸다. 방향 자료(방향 수·물리 프레임 수)가 없으면 거부한다.
- `fire(fields)`(00467c80): 프레임 경과(`delta`, 0 초과 2 미만일 때만)로 각속도/각을 float 저장 순서로 적분하고, 임계(`감김×360 + 표적 각 − 발사 편향`)에서 스프링(`sasprung.wav`, 가속도 부호 반전)과 발사(출력 bit0 켬+공간 통지+`launch`+`safired.wav`)를 하며, 각에 따라 프레임(`fireFrame`)과 소리(`sarachet.wav`/`sunDiscThrowerWhoosh.wav`)를 바꾸고 종료 조건(각속도 ≤ finishSpeed 이고 정규화 각이 finishAngle ± finishTolerance 안, 또는 각 < 0)에서 완료(통지 info 6/value 7 + 삭제)한다. 표적 0 은 원본 assert 로 거부한다.
- `launch(fields)`: live HP 표적(상태 &3 = 0, 타입 flags1 0x10)일 때만 sunDisc Missile 요청(속도 35·표시 83·효과 0·포구 기본)을 훅에 넘긴다. 생성 경계가 없으면 성공으로 우회하지 않고 거부한다.
- 정적 helper: `nearestDirection(angle)`(20칸 표에서 가장 가까운 칸, 동률은 앞선 칸·끝 칸 360도 보정), `initialAngle(frame)`, `fireFrame(angle, sprung)`(fmod → float 저장 → `+0.5f` 절삭, sprung 이면 +2 프레임·+16), `degrees(from, to)`, `directionAngle(from, to, diagnostics)`.
- `ArcherAnimationProcess(SquidProcess)`: `runFrame`(fire 면 `fire`, 아니면 `rotate`)·`onAttached`(Form/Parent 기록)·`fields`.
- `ArcherAnimationState`: 현재 시각·프레임 경과·TYPE·표시 TYPE·초기 속도/가속도·감김/편향/스프링/리셋 각·종료 속도/각/오차·각도 진단 장치를 호출 시점에 다시 읽는 가변 상태이다(`currentType = { value }` 면 빌린 DWORD).

## 2. 검증

- `tests/archer-animation.test.mjs`(2건):
  1. `gunarcher-x86.tsv` 의 `originals` 845행(`originalCD` 845행은 10.72 라 제외). 몸체(회전/발사/가장 가까운 방향/초기 각/발사 프레임/생성자+Fire 초기화/launch)와 프레임·delta·각상태·표적 상태·시각 도달·방향 자료로 반환(또는 Missile 생성 접두), 외부 효과 순서(`C` 중심/`U`·`P` 공간/`S` 소리/`N`·`F` 난수/`M` 통지/`K` 삭제/`T` 표적점/`A`·`J` Missile 인자/`Q` 출력 통지/`B` 부착), 0x58바이트 필드 이미지, 부모 슬롯 50바이트, float 반환 버퍼가 같다.
  2. 계약: sunDisc 생성 경계 누락을 성공으로 우회하지 않음(효과 없이 거부), 판본/표 크기/현재 TYPE 오류, 초기화 필드(padding 보존), helper 범위 오류, 프로세스 보조.

최초 실패와 교정: 새 모듈은 845행 fixture 를 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 중심·공간·소리·난수·Missile 생성·출력 통지·완료 통지·삭제는 훅 경계이다. Host 연결(`MakeArcherAnimationHostHooks`/`CreateArcherAnimation`/`MakeArcherAnimationProcessHooks`/`MakeArcherMissileHooks`)은 `RawGunProcess`·`RawMissile` 이식 뒤에 한다.
- `RawArcherTypeData`(TYPE 정의의 방향 수/물리 프레임 수 읽기, `archertype-x86.tsv`)는 타입 정의 파서의 `orientations` 속성/특수 프레임 접근이 필요해 이번 단위에서 제외했다. 지금은 `animationTypes[type] = { directions, frameCount }` 로 호출자가 공급한다.

## 4. 다음 시작점

`RawArcherTypeData`, `RawCrossbowAnimation`, `RawGunProcess`(+`MakeGunTargetHooks`), `RawLocalGeyser`, `RawGunImpact`(`Clamp`는 `raw-meltdown.js` 비공개 함수와 같은 식), Missile 계열(`RawMissile`), `RawPathProcess`(커널/Form 장면 선행)·`RawPathStart`·`RawPathData`·`RawPathCleanup`, `RawBalloonFrame`, `RawPacketDispatch`, 사제 배치 계열, `RawIslandRuntime` 계열, 분산 SP 정책(`ReadPlayerSpAmount`/`SetPlayerSpAmount`), 번개 계열, 그 밖의 작은 몸체이다.
