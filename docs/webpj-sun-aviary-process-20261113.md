# webpj 훨리베이스 프로세스 RawSunAviaryProcess (WB04-E353, 2026-11-13, 7-P1 비행 유닛 부품)

3.1절 7-P1 둘째 항목 "RawSunAviaryProcess 214줄(+20)"(기준선 소스의 선 아비어리 프로세스)을 `core/raw-sun-aviary-process.js` 로 옮겼다. 훨리베이스(TYPE 72, 선 아비어리)가 훨리기그를 생산하는 **48바이트 생산 상태 객체**의 생성(0045d690)·후처리(0045ec50)·프레임(inner 0045d8c0)·상태 writer(0045d6e0→0045b070)와, 플라이어 사망을 홈에 알리는 **홈 가상 34 통지**(0045b4a0)를 담는다. 같은 단위에서 E352 로 이식한 `raw-flyer-process.js` 의 훨리기그 최초 후처리 391행(`sun-flyer-postpop`)도 함께 대조했다(RawFlyerProcess 의 훨리기그 분기가 아비어리 쪽 `flyerDied`/`flyerType` 과 짝이기 때문).

## 1. 규칙

- **`SunAviaryProcessFields`:** `bytes`(48바이트: +8 dirty·+16 다음 프레임 시각 double·+24 AP 상태·+32 inner VT·+36 inner Form·+40 inner 부모·+44 흔들림 횟수)를 소유한다. 생성자가 쓰지 않는 padding 과 비서버 상태도 호출자의 입력 그대로 보존한다. `form`/`parent` 는 각각 inner Form/부모 DWORD 의 하위 WORD 이다.
- **`initialize(fields)` (0045d690):** 시각 0·dirty 0xff·inner VT 0x5019ec → 0x509db8·부모/Form 0·외부 VT 0x509dd8·횟수 0 을 쓰고, **서버일 때만** 시각 = now·dirty |= 4·상태 1 을 쓴다(비서버는 상태를 초기화하지 않는다).
- **`postPop(parent, flags)` (0045ec50):** 서버이고 flags bit0 이면 이미 부착된 타입 프로세스가 있을 때 진단 0x1e2 후(정상 복귀 시 계속) 새 48바이트를 확보한다. 확보 실패(NULL)도 NULL 부착 요청으로 전달한다. 초기화한 필드를 flags 8 로 부착하고, 마지막은 항상 Damageable 후처리이다. 서버 관문은 한 번만 읽는다.
- **`frame(fields)` (inner 0045d8c0):** 부모(+40 의 0x20 정지 비트)가 정지했거나 편집 모드면 무동작이다. 상태 1(홈 프레임 27·AP 8·`apStart.wav` 소리)·2(프레임 25 초과면 상태 6, 아니면 0.2초마다 프레임 +1)·3(대기 시각이 되면 4)·4(프레임 1 미만이면 상태 5, 아니면 0.2초마다 프레임 −1)·6(서버만: 플라이어 생성·소유자 지정·홈 연결·Pop(x + 0.45, y − 3)·훨리기그와 연결 → 상태 3·3초 대기·`refreshForm`)·8/9(0.2초마다 프레임 왕복, 흔들림 횟수 > 9 이면 8 은 프레임 11·상태 2, 9 는 상태 5)·그 밖(개발자 진단이 켜졌고 상태가 1..10 밖이면 0x75e)을 실행한다. **진입 때 읽은 프레임·부모 포인터를 유지**하고 효과 뒤 상태/횟수/시각/좌표는 원본 위치에서 다시 읽는다(부모 포인터는 `pool.physical` 살아 있는 보기이다).
- **`flyerDied(home)` / `flyerDiedIn(pool, fields, home, unpop, repop, refresh, nativePhysical)` (홈 가상 34=0045b4a0):** 서버만: 프로세스 조회 → Unpop → 홈 프레임 27(직접 DWORD) → Repop → AP 8·dirty |= 4 → Refresh. **조회 결과가 NULL 이어도 공간 효과(Unpop/프레임/Repop)를 끝낸 뒤에야 거부**한다(원본의 NULL 역참조). `nativePhysical` 이면 Native 장기 통지처럼 물리 슬롯에 쓴다.
- **`writeProcess(fields, packet, includeParent, force, protocol, message)` (0045d6e0→0045b070):** 변경 비트(2 시각 8바이트·4 상태 DWORD)와 부모 DWORD(`includeParent` 또는 `force` 이고 부모가 0 이 아닐 때 dirty |= 1)를 쓰고 길이 WORD·dirty 바이트를 기록한 뒤 필드 dirty 를 0 으로 되돌린다. **총 길이(9 + 부모 4 + 필드 폭 합)는 쓰기 전에 검사**하고 모자라면 필드를 바꾸지 않고 거부한다.
- **`SunAviaryProcess`:** `SquidProcess` 파생. `runFrame` 이 프레임을 로직에 전달하고 `onAttached`/`writePacket` 이 Host 의 현재 Form/부모를 필드에 기록한다(부착 전 writer 도 같다). `notify` 는 원본 inner+1c(00480260, ret 12) 빈 몸체이다.
- **`findAttachedProcess`:** 원문 004b1a90 은 윈드 아비어리와 같은 함수라 `raw-wind-aviary-process.js` 의 구현을 재노출한다.

## 2. 웹 범위와 기준선 뒤 변경

- 10.72(CD)는 범위 밖이다(PATCH 만 받는다). 기준선 뒤 `20261109-96 CP`(자산 종속 통지와 현재 프로세스 장기 연결)의 변경(홈 통지의 Native 물리 슬롯 쓰기와 현재 프로세스 NULL 경계)을 반영했다(PORT_MAP base `0b1b7c73`).
- 전역은 현재 값을 읽는 함수 묶음 `{ server(), editor(), checking(), now(), processType(), flyerType() }` 이다. 훅 구현(할당·부착·소리·플라이어 생성/Pop/연결·Form 갱신·Unpop/Repop)은 후속 단위와 4-D/6 단계 조립 몫이다.
- 이 모듈은 DOM 을 쓰지 않는다.

## 3. 검사

- **원본 독립 관측(`tests/raw-sun-aviary-process.test.mjs` 19건 중):** `sun-aviary-x86.tsv` 1,734행 — 생성자 C 18행(미정 바이트를 포함한 모든 쓰기)·후처리 P 98행(live/dead/free 후보와 개별 할당·부착 이미지)·프레임 F 594행(현재 효과 이후 읽기와 실제 AP 상태 경계)·상태 writer W 1,024행(전체 dirty/부모/force 와 미사용 버퍼)과, `sun-aviary-notice-x86.tsv` 128행(홈 가상 34 통지의 서버 관문·효과 뒤 현재 값 재조회·세 변이)이 필드 48바이트·객체 250바이트·전역 20바이트·패킷 64바이트·호출 순서까지 **첫 실행에서 전부** 일치했다.
- **훨리기그 후처리(`tests/raw-flyer-process.test.mjs` +1건, 총 19건):** `sun-flyer-postpop-x86.tsv` 391행 — 훨리기그 전용 생성/홈/소리/그림자 후처리가 전체 136바이트 필드·객체 250바이트·전역 16바이트·부착 직전 이미지·호출 순서까지 첫 실행에서 일치했다.
- **cpppj 시험 이식:** `SunAviary_NativeConstructor18`·`_NativePostPop98`·`_NativeFrame594`·`_NativeWriter1024`·`_NativeHomeNotice128`·`SunAviary_RequiredBoundaries`·`SunAviary_HomeNoticeNullAfterSpatialEffects`, `SunFlyerPostPop_Native391`.
- **웹 계약(10건):** Kernel 래퍼의 프레임 전달·Form/부모 기록·`writePacket` 의 Host 번호 기록과 protocol/message 전달, 프레임 상태별 효과(정지/편집 차단 무동작·상태 3/4/6/8 전환·비서버 상태 6 생략), 첫 부모 포인터의 살아 있는 보기, 개발자 진단 인자(0x724/0x751/0xa4), 홈 통지의 일반/Native 슬롯 선택, 생성자·접근자, 오류 문구, `form`/`parent` 하위 WORD, 상태 패킷의 정확한 총 길이 경계와 쓰기 전 거부.
- 시험이 먼저 실패한 곳은 모두 시험 쪽이었다(상태 3 대기/준비 시각 색인). 모듈은 관측 대조에서 바뀌지 않았다.

## 4. 변형 확인과 남은 경계

- 한 문자열 치환 변형 182회를 적용했다. 1차에서 31개가 남았고 그중 26개는 **시험의 공백**이라 보강해 잡았다: 생성자의 PATCH 확인과 `pool` 접근자, 훅 누락·NULL 프로세스 거부의 오류 문구(일반 호출 오류와 구별), `form`/`parent` 의 하위 WORD 값(writer 출력 포함), 홈 통지의 일반 연결(확보된 슬롯만)과 Native 장기 연결(free 물리 슬롯), 첫 부모 포인터가 살아 있는 보기라는 점(효과가 바꾼 좌표·소유자를 Pop 좌표와 소리가 그대로 읽음; float 0.45 상수는 1.37 좌표에서만 배정도 0.45 와 갈라짐), 개발자 진단의 프레임 검사 인자(0x724 올림 +1·0x751 내림 −1), 상태 6 의 부모 종속 WORD 진단 0xa4, writer 의 총 길이(dirty·부모·force 조합의 정확한 경계와 거부 뒤 필드 불변)와 protocol/message 전달, `writePacket` 이 부착 뒤 어긋난 필드 번호를 Host 의 현재 번호로 되돌리는 것.
- 남은 동등 변형 5개: `add` 의 `| 0`(프레임 덧셈이 int32 를 넘는 경로가 없고 `put` 이 어차피 낮은 바이트만 쓴다)·`setTime` 의 첫 `storeTime(now)`(같은 8바이트를 곧바로 덮어씀)·`initialize` 의 inner VT 0x5019ec 쓰기(바로 뒤 0x509db8 이 덮어씀)·`initialize` 서버 분기의 `dirty |= 4`(앞서 이미 0xff)·상태 8/9 의 `let next = frame` 초기값(모든 분기가 다시 대입).

- 변형하지 않은 방어선: 생성자의 PATCH 확인 문구, 훅 누락 메시지 문구, 패킷 크기 오류 메시지.
- 앱 수명 연결(실제 할당/부착/소리/플라이어 생성/공간 효과 훅 구현)은 후속 7-P1 단위와 4-D/6 단계 조립 몫이다. 다음 7-P1 항목은 **RawRainAviaryProcess**(180줄 +19)이다.
