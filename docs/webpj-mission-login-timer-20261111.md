# webpj 로그인 타이머·집계(RawMissionLoginTimer) — WB04-E318 (2026-11-11, 4-C)

3.1절 4-C 의 **RawMissionLoginTimer**(`o/`, 기준선 210줄 = `.cpp` 130 + `.h` 80, 기준선 뒤 +71줄)를 `core/raw-mission-login-timer.js` 로 옮겼다. PATCH 로그인 타이머의 같은 Player9 집계·미션 종료 선택·Begin/Abort 순서와 일반 로그인 진행 문구이다(10.78: 004b67c0 Run, 004180a0 Expired, 004b61f0 Begin, 004b6780 Abort, 004b68d6..004b6b95 진행 문구, 집계 00490970·00490a00·00490ab0·00490b20·00490bc0·00490c50·00490cf0). **체크 목록 순서를 Timer → Login 으로 바꿨다**: `RawMissionLogin` 이 이 단위의 `MissionLoginPlayerCounts` 집계를 쓴다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 규칙

- `MissionLoginPlayerCounts(players)`: 정확한 9×0xb4 바이트를 빌리고 owner 1..8 의 subscriber(`+0x2c`)/flags(`+0x50`)만 읽는다(Player 0 방문 없음, 사본 없음). `subscribers`(subscriber≠0) · `participating`(+flags 2) · `disconnected`(+flags 2 와 0x100) · `ready`(+flags 2, 1, 0x40) · `loggedIn`(+flags 2, 1) · `pending`(subscriber, `(flags & 0x103) == 2`) · `connecting`(subscriber, flags 2 없음) · `record(owner)`.
- `new RawMissionLoginTimer(state, counts, reads, hooks)`: `state` 는 기존 게임 상태의 실제 저장소(`end` 벽시각 예약 double·`timeout` signed·`begin`·`scriptBreak`·`requestPhase`·`clock`), `reads` 는 원문이 분기마다 다시 읽는 현재 모드 여섯(`savedGame`·`server`·`mission`·`missionPost`·`initialGeysers`·`checkingAssertions`, bool 을 DWORD 로 재해석하지 않음), `hooks` 는 시간/전송/사제/UI 소비자뿐이며 선택하지 않은 경계는 부르지 않고 가짜 성공으로 채우지 않는다.
- `run(true)`(Start): 시작 로그 → 설정 `loginTimeout` → `timeout` 저장 → 실제 벽시각 효과 → `end = timeout + wall`. `run(false)`: `end == 0` 이면 아무것도 읽지 않고 반환. 아니면 (저장 경로가 아니면) CRT clock 저장 → (서버가 아니면) 연결 조회(9)가 없으면 Abort, 있으면 서버를 **다시 읽어** 아직 아니면 원격 상태 요청 → 서버 사슬: `begin == 0` 이면 `subscribers − disconnected ≠ ready` 일 때 미만료면 Begin 생략, 만료이고 로그인 수 2 미만(저장 경로 아님)이면 Abort → Begin → `missionPost || begin ≠ 0` 이면 (저장 경로가 아니면) 일반 초기화(미션 아님)와 초기 간헐천 로그/생성, 사제 상태 로그 → owner 1..8 사제 재설정 → `begin = 0`, `scriptBreak = 0` → 재접속 저장 → `end = 0` → 진행 창 해제 → 카메라 로그/초기화(missionPost 아님) → 완료 로그 후 반환, 아니면 일반 진행 창 꼬리.
- `begin()`: (확인 중이고 서버가 아니면 진단) → `end = 0` → 실제 Begin 8바이트(`[1, 0x6e, 8, 0, 0, 0, 0, 0x80]`) 송신 → `begin = 1`(송신 효과 뒤). `abort()`: `begin = 0`, `end = 0`, `scriptBreak = 1`, `requestPhase = 4` → 실제 상태 정리 → 진행 창 해제. 요청 phase 는 하위 효과가 다시 바꿀 수 있으므로 마지막에 덮어쓰지 않는다. `expired()`: `end <= 현재 벽시각`.
- `progressText(frameWall, translate)`: 명단(subscriber 가 있는 owner 1..8 의 이름 + 상태 문구 키 `: ~iConnecting~.`/`Disconnected`/`Pending`/`Transferring`/`Ready` + LF), 남은 시간(`end − frameWall` 64비트 절삭 하위 DWORD 의 signed, 음수는 0), 제목(기본 `Starting Game`, 서버면 `Becoming Server`), 추가 안내(남은 시간 1 미만이면 지연 안내, 서버이고 `subscribers − disconnected − pending − connecting == ready` 이며 `timeout − 남은 > 10` 이면 강제 시작 안내)를 번역된 서식 `~.~3%s~*~.\n\n%s\nTime Remaining: ~i%d~.%s` 의 네 인자(문자열·문자열·정수·문자열)로 채운다. 번역이 미공급 인자를 읽게 하는 서식은 명시 거부하고 `%%` 와 상수 폭/정밀도/플래그는 유지하며 명단 또는 결과가 1024바이트 이상이면 거부한다.

## 2. 기준선 뒤 변경 판정 (+71줄)

- **옮김(② 적용)**: 일반 로그인 완료 초기화(`initializeGeneral`/`createInitialGeysers`, `a60d547f0`), 일반 대기 화면의 진행 문구와 강제 시작 안내(`progressText`, `25e32b7a9`).
- 옮기지 않은 변경은 없다. 진행 창 UI 자체(HTML 창, 번역 조회기 연결)는 UI 단계(5-A/6)의 몫이다. CD(10.72)는 옮기지 않았다.

## 3. 검사 (`tests/raw-mission-login-timer.test.mjs` 7건)

- **원본 독립 관측 `mission-login-timer-x86.tsv` 1984행**(집계 C·만료 E·Begin B·Abort A·Run R/S × Player 패턴 × 모드 × 연결/호출 도중 변경 트리거): 결과·전체 상태 직렬화(double 비트 포함)·현재 모드 읽기와 외부 효과 순서를 모두 대조한다.
- 손 시험(cpppj `MissionLoginTimerTests.cpp` 이식 + 진행 문구): 미ready missionPost 가 Begin 없이 종료, inactive 는 어떤 경계도 읽지 않음, 선택 소비자 누락이 앞선 쓰기만 남김, 일반 초기화 미복원 거부와 구성 검사, Abort/완료가 현재 참조를 순서대로 갱신(같은 값을 하위 효과가 바꾸어도 덮지 않음), 진행 HTML 의 명단 상태 문구·남은 시간(무한/NaN/범위 밖 포함)·서버 제목·강제 시작 안내·서식 검증(인자 개수/종류, `%%`, 폭·정밀도·`+`·`0`·`-` 플래그)·1024 한도·번역기 누락.
- 변형 확인 결과는 아래 4절을 본다.

## 4. 변형 확인과 남은 경계

- 변형 확인 96개 중 보강 뒤 **7개만 남았고 모두 동등**하다: 집계 DWORD 의 `>>> 0`(0 비교와 비트마스크만 씀)·`subscribers − dropped` 의 unsigned 감김(`disconnected` 는 항상 `subscribers` 이하)·집계 세 호출의 순서(부작용 없는 순수 읽기)·명단 1024 한도 둘(owner 8명 × 최대 약 50바이트라 도달 불가)·진행 시간 상한 경계(정확히 2⁶³ 도 하위 DWORD 0)·내부 `%d` 서식의 음수(남은 시간은 0 으로 고정). 원본 관측이 닿지 않던 경계(일반 초기화·초기 간헐천 로그 순서, 이름 NUL 누락, 강제 시작 조건의 연결 중 제외/경과/부호 감김, 1023·1024 경계, 로그 종류 값)는 시험을 보강해 잡았다.
- 3.1절 4-C 는 15/30 이다. 다음: RawMissionLogin(PATCH 로그인 004a7800 의 부모 순서 — 앞선 서버·클라이언트·설정·Fort 패킷과 이 집계를 연결하고 E314 가 보류한 Ready boss 6행을 재생).
