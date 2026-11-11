# webpj 미션 로그인 수신 부모(RawMissionLogin) — WB04-E319 (2026-11-11, 4-C)

3.1절 4-C 의 **RawMissionLogin**(`o/`, 기준선 265줄, 기준선 뒤 +47줄)을 `core/raw-mission-login.js` 로 옮겼다. PATCH 로그인 부모와 그 하위 송신기의 같은 Player9 순서·일반 전투/미션 분기·패킷 배치·Ready 이다(10.78: 004a7800 수신 부모, 004a7510 재접속 63h, 004a6920 응답 0x28, 004a6990 Begin 0x29, 004a6e70 SP 0x81, 004a6ab0/00491270 가입 통지, 004a6f70 Ready 0x6e). 앞선 서버·클라이언트·설정·Fort 패킷(E314~E317)과 E318 의 집계(`MissionLoginPlayerCounts`)를 연결하고, E314 가 보류했던 Ready boss 6행을 재생해 클라이언트 패킷 157행을 모두 닫았다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 규칙

- `new RawMissionLogin(players, participation, globals, hooks)`: `players` 는 정확히 9×0xb4=1620바이트(subscriber `+0x2c`·flags `+0x50`·watcher `+0x58`·순서 `+0x5c`·예측수 `+0x60`·시작 완료 `+0x64`), `participation` 은 `RawPlayerParticipation`(실제 flags 3·연결 재배치·거부/해제), `globals` 는 Session/타이머와 공유하는 현재 전역 묶음(속성으로 매번 현재값을 읽고 쓰는 필드는 직접 갱신, 별도 사본 없음), `hooks` 는 큰 외부 몸체뿐이다. 선택한 외부 몸체는 **도달한 위치에서 필수**이고 없으면 앞선 부분 효과를 남긴 채 명시 거부한다(패킷을 버리는 성공 처리기를 공급하지 않는다). Player/Zone(6)/옵션(87)/CoreData 캐시(10) 크기가 정확히 맞아야 한다(더 큰 저장소도 거부).
- `receive` → `receiveDirect`(수신 부모, 0x27): 서버가 아니면 pending 연결만 끊는다 → subscriber 가 같은 첫 owner 1..8 → Player 를 아직 모르면(`playerKnown` 0) 자기 구독자와 같은 Player 로 template/battle owner 와 CoreData(미션 메타맵이면 캐시, 아니면 Player `+0x6c` 의 10바이트 view)를 정한다 → owner 가 없으면 거부 응답 `Reply(0,0)` 을 **들어온 연결**로 직접 보낸다(NULL 연결은 이 위치에서만 거부) → simulconnect 이면 음수 pending 은 이전 owner 연결만 닫고 1..8 밖은 진단 → `previous = (flags>>1)&1`, previous 가 아니면 복원기 → CheckLogin → Begin(0x29) → 옵션(0x67, 94바이트·선언 4·0xff 채움) → 참여자 2 미만이면 옵션 적용 → Fort 개수 0 이면 타입 초기화 → Player(0x2b)·Fort metadata → previous 가 아니면 Fort 이미지 적재(`+0x5c`=현재 Fort 수, `+0x60`=예측수, Fort 수 증가, 서버의 battleOwner 면 현재 Fort, 아니면 패킷의 이미지) → 가입 통지 → 응답(수락 1) → 일반 자원 초기화(미션 후처리가 아니고 저장 경로가 아니며 전투 중일 때만: watcher 와 시작 SP 표 [1000, 2500, 3000]+4000, 색인 범위 거부) → SP 패킷 → 시간 패킷 1..8 → `+0x64`=1 → Fort 순서 순회(참여 Player 만, 새 자기 Fort 제외) → 서버가 아닌 owner 동기화 → previous 가 아니면 새 Fort 를 접속 대상 전체에 보내고 월드/전투섬 생성 → 설정 패킷 → 지역 Player 에 flags 0x40 → Ready → 마무리(`finish`: 로그인한 구독자가 하나뿐이면 `begin`=1).
- `reply`(0x28, 18바이트)·`sendBegin`(0x29, 34바이트)·`sendSp`(0x81, 44바이트, 선언 4)·`sendReady`(0x6e, 8바이트)·`joined`: 모두 실제 필드 위치/폭 그대로이다. 응답의 남은 시간은 현재 벽시각을 얻은 **뒤**의 `captureEnd − now` double 비트이다. 디버그 burst 는 같은 대상에 63012바이트 예약을 정확히 세 번 보내며 헤더/초기 길이(미만이면 4)를 매번 현재 생성 헤더에서 읽는다.
- `receiveReconnect`(63h): 정확히 8바이트 입력만 받고 로그 → subscriber 0 도 비교하는 owner 검색(없으면 확인 중일 때만 진단 후 반환) → CheckLogin 승인이면 응답 1 → 자기 구독자면 flags 0x18 → 가입 통지 → Fort 수 재계산(참여 Player 수)과 마지막 새 Fort 소유자(**앞서 고른 owner 번호보다 큰 `+0x5c` 일 때만 갱신** — PE 는 최대값이 아니라 이 비교를 한다. 정규화하지 않는다) → 재접속 클라이언트 정리 → 지역 Player flags 0x40 → Ready 한 대상 → 마무리. 새 Fort/월드 생성은 하지 않는다.
- `registerMissionLogin`: 0x27(관찰자 NULL·버전 1/정책 0)과 0x63(관찰자 NULL·버전 1/정책 0·길이 8)을 원본 초기화 순서대로 등록하며 pending 은 부호 있는 DWORD 로 넘긴다.

## 2. 기준선 뒤 변경 판정 (+47줄)

- **옮김(② 적용)**: 재접속 63h 수신(`7ce6f1298`), 일반 자원 초기화의 watcher/시작 SP(`a60d547f0`), 디버그 burst 와 전체 mutable 예약 송신(`5844bfb56`·`392517096`), 공유 전투 seed 상자(`ede61f05d`), Native 고정 진입값 없는 등록(`25e32b7a9`).
- **옮기지 않음(③ 4-D)**: Native Login 고정 수신기를 분배기에 대여하는 분기(E306 과 같은 고정 Native 수신기 묶음). CD(10.72)는 옮기지 않았다. 이 모듈은 DOM 을 쓰지 않는다.

## 3. 검사 (`tests/raw-mission-login.test.mjs` 15건)

- **원본 독립 관측 `mission-login-x86.tsv` 48행**(서버/알려진 Player/현재 owner/구독자/flags/다른 Player/호출 도중 변경/CRT clock): 전체 Player 1620바이트·현재 전역 9개·모든 packet 과 외부 효과 순서를 대조한다. 이 단위에서 공용 시험 장면 `tests/mission-client-scene.mjs` 에 실제 로그인을 연결해 `raw-mission-client-packets.test.mjs` 의 **Ready boss 6행을 포함한 157행 전부**가 재생된다(E314 에서 151행).
- 손 시험(cpppj `MissionLoginTests.cpp` 이식 + 기준선 뒤 경로 + 변형 확인으로 찾은 경계): 필수 소비자 누락의 부분 효과 보존·등록 호출 순서/pending 부호, 외부 모드의 미복원 분기 거부와 Ready broadcast/boss 진단, 응답 double 비트·NULL 연결 거부, 재접속 63h(owner 미발견/CheckLogin 거부 때 추가 응답 없음/자기 구독자 상위 비트/마커 선택/카운터 초기화/지역 Ready/한 구독자 begin), 일반 자원 초기화(watcher·시작 SP 표 세 항목·색인 범위·선택 필드 누락), 디버그 burst(초기 길이 NULL 거부·하한 4), 구성 크기(정확히 일치만), Player 필드 경계, 가입 통지(두 순회 대상/flags/주소 경계), 수신 부모 세부(상위 비트 구독자·CoreData view·거부 응답 연결·simulconnect pending 범위·복원기 선택·Fort 이미지 범위·도중 서버 모드 변경), Player/Fort 순서 순회, 시작 표시(`begin`) 판정.

## 4. 변형 확인과 남은 경계

- 변형 216개 중 첫 실행에서 77개가 남았고, 위 손 시험을 보강한 뒤 **11개만 남았으며 모두 동등**하다: 쓰기 폭 상한(호출부의 `put` 폭 검사가 항상 먼저)·생성자 Player 길이 `<`(`MissionLoginPlayerCounts` 가 정확히 1620바이트가 아니면 거부)·`ftol` 의 2⁶³ 경계 둘(단정도 float 는 그 이상에서 하위 32비트가 항상 0)·응답의 시계 읽기 순서(효과 없는 선언 추가)·Begin 의 owner 폭(owner 0..8)·가입 통지의 구독자 `^ 0`(하위 바이트만 쓰므로 동일)·옵션 packet 채움(94바이트 전부 덮임)·Fort 범위의 `start > length`(크기 비교가 같은 입력을 거부)·시간 차의 `baseClock` 부호(뺄셈 뒤 `| 0` 로 같은 하위 32비트)·`finish` 의 unsigned 감김(`disconnected` 는 항상 `subscribers` 이하).
- 시험이 찾은 일: 재접속 63h 의 마지막 새 Fort 소유자는 PE 가 **앞서 고른 owner 번호**와 다음 `+0x5c` 를 비교하는 것이라, `+0x5c` 의 최대값을 고르는 직관과 다르다. 이를 정규화하지 않고 PE 그대로 옮기고 시험(마커가 아닌 owner 번호보다 큰 값만 후보, 같은 값은 후보 아님, 부호 있는 비교)으로 못박았다.
- 3.1절 4-C 는 16/30 이다. 다음: RawLocalMissionState(단독 미션 상태 7~10/13/14 의 블록 복사·확정·수신 시점).
