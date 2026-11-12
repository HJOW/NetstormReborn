# webpj 맨 오 워 풀 프로세스 RawRainAviaryProcess (WB04-E354, 2026-11-13, 7-P1 비행 유닛 부품)

3.1절 7-P1 셋째 항목 "RawRainAviaryProcess 180줄(+19)"(기준선 소스의 레인 아비어리 프로세스)을 `core/raw-rain-aviary-process.js` 로 옮겼다. 맨 오 워 풀(TYPE 60, 레인 아비어리)이 맨 오 워(TYPE 144)를 생산하는 **48바이트 생산 상태 객체**의 생성(0045d810)·후처리(0045ed10)·프레임(inner 0045dde0)·상태 writer(0045d870→0045b2a0)와, 플라이어 사망을 홈에 알리는 **홈 가상 34 통지**(0045b5b0, 게임 난수 소비)를 담는다. 직전 E353 의 훨리베이스(`raw-sun-aviary-process.js`)와 같은 뼈대이며 다른 점은 아래 규칙에 적었다.

## 1. 규칙

- **`RainAviaryProcessFields`:** `bytes`(48바이트: +8 dirty·+16 다음 프레임 시각 double·+24 AP 상태·+32 inner VT·+36 inner Form·+40 inner 부모)를 소유한다. 훨리베이스와 달리 흔들림 횟수(+44)는 읽거나 쓰지 않는다. 생성자가 쓰지 않는 padding 과 비서버 상태도 호출자의 입력 그대로 보존한다. `form`/`parent` 는 각각 inner Form/부모 DWORD 의 하위 WORD 이다.
- **`initialize(fields)` (0045d810):** 시각 0·dirty 0xff·inner VT 0x5019ec → 0x509e00·부모/Form 0·외부 VT 0x509e20 을 쓰고, **서버일 때만** 시각 = now·dirty |= 4·상태 1 을 쓴다.
- **`postPop(parent, flags)` (0045ed10):** **서버이고 (편집 fort 이면서 친군 사격이 아님)이 아니며 flags bit0** 이면(원문 읽기 순서) 이미 부착된 타입 프로세스가 있을 때 진단 0x257 후(정상 복귀 시 계속) 새 48바이트를 확보한다. 확보 실패(NULL/undefined)도 NULL 부착 요청으로 전달한다. 초기화한 필드를 flags 8 로 부착하고, 마지막은 항상 Damageable 후처리이다. 부착 때 processType 을 다시 읽는다.
- **`frame(fields)` (inner 0045dde0):** 부모(+40 의 0x20 정지 비트)가 정지했거나 편집 모드면 무동작이다. 상태 1(홈 프레임 0·AP 2·`manOWarBuild.wav` 소리)·2(프레임 11 초과면 상태 6, 아니면 기한이 될 때 개발자 진단 0x7fa 뒤 프레임 +1·0.2초 대기)·6(**서버만** 플라이어 생성·소유자 지정·홈 연결·Pop(x − 0.5, y − 2)·연결, **서버와 무관하게** AP 5·홈 프레임 0·3초 대기)·10(대기 시각이 되면 AP 2)·그 밖(개발자 진단이 켜졌고 상태가 1..10 밖이면 0x820)을 실행한다. 진입 때 읽은 프레임·부모 포인터를 유지하고 효과 뒤 상태/시각/부모는 원본 위치에서 다시 읽는다(부모 포인터는 `pool.physical` 살아 있는 보기). Pop 좌표는 float 좌표를 배정도로 0.5/2.0 빼고 float 로 줄인 값이다.
- **`flyerDied(fields, random, refresh)` / `flyerDiedIn(fields, now, random, refresh)` (홈 가상 34=0045b5b0):** 서버만: 최초 dirty 를 보존한 채 AP 10(dirty |= 4) → **첫 시각 조회**·저장(dirty |= 6) → 게임 난수 `next(5000) + 10000` 밀리초 → **둘째 시각 조회**(난수 뒤) + 정수 초(밀리초의 정수 나눗셈, 10..14) 저장 → Form 갱신. 시각·난수·갱신 훅의 누락은 그 앞의 쓰기를 남기고 거부한다.
- **`writeProcess(fields, packet, includeParent, force, protocol, message = 0x73)` (0045d870→0045b2a0):** 변경 비트(2 시각 8바이트·4 상태 DWORD)와 부모 DWORD(`includeParent` 또는 `force` 이고 부모가 0 이 아닐 때 dirty |= 1)를 쓰고 길이 WORD·dirty 바이트를 기록한 뒤 필드 dirty 를 0 으로 되돌린다. 기본 message 는 훨리베이스(0x38)와 달리 0x73 이다. **총 길이는 쓰기 전에 검사**하고 모자라면 필드를 바꾸지 않고 거부한다.
- **`RainAviaryProcess`:** `SquidProcess` 파생. `runFrame` 이 프레임을 로직에 전달하고 `onAttached`/`writePacket` 이 Host 의 현재 Form/부모를 필드에 기록한다(부착 전 writer 도 같다). `notify` 는 원본 inner+1c(00480260, ret 12) 빈 몸체이다.

## 2. 웹 범위와 기준선 뒤 변경

- 10.72(CD)는 범위 밖이다(PATCH 만 받는다). 기준선 뒤 `20261109-96 CP`(자산 종속 통지와 현재 프로세스 장기 연결)의 변경(홈 통지의 `flyerDiedIn` 정적 본문)을 반영했다(PORT_MAP base `0b1b7c73`).
- 전역은 현재 값을 읽는 함수 묶음 `{ fort(), friendlyFire(), server(), editor(), checking(), now(), processType(), flyerType() }` 이며 생성자가 이름별로 함수인지 확인한다. `friendlyFire`·`editor`·`checking` 은 비영 DWORD 뷰로 읽는다. 훅 구현(할당·부착·소리·플라이어 생성/Pop/연결·Form 갱신)은 후속 단위와 4-D/6 단계 조립 몫이다.
- 이 모듈은 DOM 을 쓰지 않는다. 훨리베이스가 재노출하는 `findAttachedProcess`(004b1a90)는 이 모듈이 쓰지 않아 다시 노출하지 않는다(시험은 훨리베이스 모듈에서 가져온다).

## 3. 검사 (`tests/raw-rain-aviary-process.test.mjs` 21건)

- **원본 독립 관측:** `rain-aviary-x86.tsv` 1,602행 — 생성자 C 18행(미정 바이트를 포함한 모든 쓰기)·후처리 P 386행(live/dead/free 후보와 개별 할당·fort/친군 사격 관문·부착 이미지)·프레임 F 174행(현재 효과 이후 읽기와 실제 AP 상태 경계)·상태 writer W 1,024행(전체 dirty/부모/force 와 미사용 버퍼)과, `rain-home-notice-x86.tsv` 36행(홈 통지의 서버 관문·시각 저장·게임 난수 상태·정수 초·Form 갱신 요청 이미지)이 필드 48바이트·객체 250바이트·전역 28바이트·패킷 64바이트·호출 순서까지 **첫 실행에서 전부** 일치했다.
- **cpppj 시험 이식:** `RainAviary_NativeConstructor18`·`_NativePostPop386`·`_NativeFrame174`·`_NativeWriter1024`·`RainAviary_RequiredBoundaries`, `RainAviaryHomeNotice_NativeStateDelayAndRefresh`.
- **웹 계약(15건):** 생성자(PATCH·전역 조회기 이름별 검사·훅 생략)·`pool` 접근자, 훅 누락 오류 문구, 후처리의 서버/fort/친군 사격/flags 관문 조합과 중복 진단 0x257·확보 실패의 NULL 정규화, 프레임 상태별 효과(정지/편집 차단 무동작·상태 1/2/6/10·비서버 상태 6·범위 밖 진단 0x820), 첫 부모 포인터의 살아 있는 보기와 Pop 좌표의 float 정밀도, 개발자 진단 인자 0x7fa·부모 종속 WORD 진단 0xa4, 홈 통지의 읽기 순서와 난수 한계 5000·정수 초·부분 쓰기, `form`/`parent` 하위 WORD, 상태 패킷의 정확한 총 길이 경계·protocol/message(기본 0x73), Kernel 래퍼의 프레임 전달·Form/부모 기록·`writePacket`.
- 시험이 먼저 실패한 곳은 모두 시험 쪽이었다(중복 진단용 입력 배치). 모듈은 관측 대조에서 바뀌지 않았다.

## 4. 변형 확인과 남은 경계

- 한 문자열 치환 변형 159회를 적용했다. 1차에서 8개가 남았고 그중 2개는 **시험의 공백**이라 보강해 잡았다: 홈 통지의 난수 한계(5000 과 5001 이 서로 다른 정수 초를 만드는 시드를 찾아 저장 시각을 직접 비교; 원본 36행은 시드가 달라도 같은 정수 초가 나와 가리지 못했다), `writePacket` 의 기본 message 0x73. 1차 실행 전에 이미 시험에 넣은 것도 있다: 훅 묶음을 생략했을 때의 오류 문구, `allocate` 가 undefined 를 돌려줘도 NULL 부착 요청으로 정규화하는 것, Pop 좌표를 float 로 줄이는 단계(0.1f 입력으로 훅이 받은 수를 직접 비교), 생성자의 PATCH·전역 조회기 이름별 검사.
- 남은 동등 변형 6개: `add` 의 `| 0`(프레임 덧셈이 int32 를 넘는 경로가 없고 `put` 이 어차피 낮은 바이트만 쓴다)·`setTime` 의 첫 `storeTime(now)`(같은 8바이트를 곧바로 덮어씀)·`initialize` 의 inner VT 0x5019ec 쓰기(바로 뒤 0x509e00 이 덮어씀)·`initialize` 서버 분기의 `dirty |= 4`(앞서 이미 0xff)·기본 분기 진단의 상한 `stage > 10` 을 `>= 10`/`> 9` 로 바꾼 두 변형(상태 10 은 `case 10` 이 처리해 기본 분기에 오지 않는다).

- 변형하지 않은 방어선: 생성자의 PATCH 확인·전역 조회기 오류 문구 표기, 훅 누락·패킷 크기 오류 메시지 문구.
- 같은 단위에서 E353 훨리베이스 시험을 보강했다: Pop 좌표가 float 로 줄었는지(배정도 합을 그대로 넘기지 않는지)를 훅이 받은 수로 직접 비교한다(1차 변형 확인은 비트 비교라 이 단계를 가렸다).
- 앱 수명 연결(실제 할당/부착/소리/플라이어 생성 훅 구현)은 후속 7-P1 단위와 4-D/6 단계 조립 몫이다. 다음 7-P1 항목은 **RawFlyerFrame**(259줄)이다.
