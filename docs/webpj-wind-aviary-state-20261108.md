# webpj 데빌메이커 상태·윈드 원주 점·윈드 소환 주문 이식 — WB04-E121

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백스물한 번째 부분이다. 데빌메이커(윈드 아비어리) 전용 80바이트 본체의 생성자와 변경 필드 패킷 쓰기/읽기(0045d730/0045d7a0·0045b150/0045b210), 원주 좌표(41e020), 세 윈드 소환 주문(41ecf0/41fb60/41fcc0)을 cpppj 기준선 `1f00a25a` 의 `o/RawWindAviaryState`·`o/RawWindCircle`(원주 점)·`o/RawWindSpell` 에서 옮겼다. 선행은 `SidPool` 뿐이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이다.

## 1. 옮긴 것

`www/core/raw-wind-aviary-state.js` — `WindAviaryProcessFields`, `RawWindAviaryState.initialize/writeProcess/readProcess`

- `WindAviaryProcessFields`: 원본 outer0/inner+38 의 80바이트 이미지. 생성자가 쓰지 않는 필드와 padding 도 호출자 입력으로 보존하며, 후속 생산/비행 상태 기계와 전송이 공유할 유일한 원문 이미지이다.
- `initialize(fields, server, now)`: +10/+20/+28 double 을 0 으로, dirty(+08) 0xff, 내장 표 0x509ddc(0x5019ec 를 먼저 씀)·가상 표 0x509dfc·Form(+3c)/부모(+40) 0 을 쓰고, **서버일 때만** 시각(+10, double 비트)·상태 1(+18)·dirty 비트 4 를 더한다. 비서버의 미정 상태와 두 예약 double 주변(0x1c/0x30/0x34/inner 꼬리)은 그대로 둔다(전체 0 초기화로 대신하지 않음).
- `writeProcess(fields, packet, includeParent, force, protocol = 1, message = 0x74)`: 부모는 `includeParent` 이거나 `force` 일 때만(비영이면) 삽입하며 dirty 1 을 켠다. `force` 이면 dirty 를 0xff 로 취급한다. 선택 필드(+10 double·+18 DWORD·+20 double·+28 double·+30 DWORD, dirty 비트 2/4/8/10/20)를 계산 없이 비트로 복사하고(NaN payload·음수 0 유지) 길이 WORD 를 쓴 뒤 dirty 만 0 으로 바꾼다. 반환은 쓴 길이이며 버퍼가 모자라면 첫 쓰기 전에 거부한다.
- `readProcess(fields | bytes, packet)`: 머리 9바이트·선택 필드 길이를 먼저 확인(모자라면 거부)하고 부모(dirty 1) DWORD 를 반환한다. 선택 필드만 덮고 선택되지 않은 상태·예약 시각·목표·padding 은 보존하며, dirty 와 inner 소유권은 바꾸지 않는다. 객체 저장이 없는 입력(PID NULL 의 빈 뷰)은 선택 필드가 없을 때만 부모를 반환하고, 선택 필드가 있으면 객체 미연결로 거부한다.

`www/core/raw-wind-circle.js` — `windCirclePoint(x, y, count, index)`

- 원주를 `count` 등분(각 간격 = `360/count` 정수 나눗셈)한 `index+1` 번째(index 는 DWORD 감김) 점이다. 각도 `(360/count) × (index+1) × 3.141593 × 0.0055555556900799274(float)`, 결과 `(cos(각) × count + x, sin(각) × count + y)` 를 각각 float 로 저장한다. `count` 가 0 이면 거부한다. 표준 pi 나 VFX 높이로 치환하지 않는다.
- 부모의 현재 중심과 SHP 헤더로 보정하는 `windCirclePosition` 은 윈드 생산 프로세스(`RawWindAviaryProcess`) 이식 때 288행 원주 프레임 관측(`wind-circle-frame-x86.tsv`)과 함께 옮긴다.

`www/core/raw-wind-spell.js` — `RawWindSpell(pool, env, state, hooks)`

- `mayTarget(spell)`: 주문 객체의 현재 가상 표(0x5025d8/0x502e68/0x502f30)로 세 변형 중 하나를 고르고(모르는 표는 거부), 서버가 아니면 아무것도 하지 않고 false 를 돌려준다. 서버이면 주문의 운반체 WORD(+0c)에 이미 생산 프로세스(현재 processType)가 있는지 `exists` 훅으로 조회하고(있으면 false), 없으면 변형의 소환 주문 TYPE 의 spawns 만큼(부착 뒤 바뀌는 현재 한도를 매번 읽음) 80바이트 생산기를 확보해(실패는 거부) 서버 생성자로 초기화하고 +44 에 원주 개수, +48 에 번호를 쓴 뒤 현재 운반체/processType/flags 8 로 부착한다. 대상 검색 중의 부수 효과이며 항상 false 를 반환한다. 서버 여부·게임 시각(`env`)은 확보 뒤에도 다시 읽는다.

## 2. 검증

- `tests/wind-aviary-state.test.mjs`(4건): `wind-aviary-state-x86.tsv` 1894행 전부 — 생성자 18(서버별 상태와 padding 포함)·쓰기 1108(모든 dirty BYTE와 별도 부모 경계)·읽기 768(수신 부분 갱신과 미작성 필드)의 전체 80바이트 결과·패킷 64바이트·부모 출력 DWORD·반환 길이 WORD 가 같다. 계약: 손상 host 길이를 첫 쓰기 전에 거부(필드 불변), 기본 프로토콜 01/74, PID NULL 읽기.
- `tests/wind-spell.test.mjs`(3건): `wind-circle-point-x86.tsv` 232행(두 float 저장, 기본 53비트 열), `wind-spell-x86.tsv` 300행(변형·서버·기존 프로세스·spawns·변화로 반환, 조회/확보/부착의 호출 순서와 인수, 부착한 80바이트 이미지). 계약: 미연결 하위·판본/표 크기·모르는 가상 표 거부, 비서버는 훅을 요구하지 않음.
- 이 fixture 들은 모두 10.78(원본) 단일 판본이다.

최초 실패와 교정: 새 세 모듈은 모든 fixture 행을 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 80바이트 생산기의 실행 프레임(`wind-aviary-frame-x86.tsv` 1640행)·PostPop·삭제와 원주 프레임(`wind-circle-frame-x86.tsv` 288행, `windCirclePosition`)은 `RawWindAviaryProcess` 단위, 비행체 몸체(`wind-flyer-x86.tsv`)는 `RawWindFlyer` 단위에서 이식한다. 존재 조회(4b1a90)·확보(new50)·부착(41bd20)은 훅 경계이다.

## 4. 다음 시작점

`RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면(`RawLightningLifetime`·`RawLightningFrame`), `RawArcherTypeData`, `RawGunProcess`(+`MakeGunTargetHooks`), `RawLocalGeyser`, `RawGunImpact`, Missile 계열(`RawMissile`), `RawPathProcess`(커널/Form 장면 선행)·`RawPathStart`·`RawPathData`·`RawPathCleanup`, `RawBalloonFrame`, `RawPacketDispatch`, 사제 배치 계열, `RawIslandRuntime` 계열, 분산 SP 정책, 그 밖의 작은 몸체이다.
