# webpj 단독 미션 상태 블록(RawLocalMissionState) — WB04-E320 (2026-11-11, 4-C)

3.1절 4-C 의 **RawLocalMissionState**(`o/`, 기준선 211줄 = `.cpp` 114 + `.h` 97, 기준선 뒤 +18줄)를 `core/raw-local-mission-state.js` 로 옮겼다. 단독 미션 상태 7~10/13/14(그리고 15·큰 부모)의 현재 블록 복사·확정·전송 수신 시점이다(10.78: 004b2bc0 Request, 004b88c0 Poll, 004b6dd0 Run). E319 의 로그인 수신 부모 뒤에 오는 단위이며, 로그인 송신 callback 이 수신기를 이 모듈의 큐에 예약해 phase 10 확정 뒤에만 도착시킨다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 규칙

- `new RawLocalMissionState({ old, phase, immediate, current, next }, globals, hooks)`: 다섯 블록은 정확히 36DWORD(0x90BYTE) `Uint32Array` 이고 서로 겹치지 않는 실제 저장소여야 한다(첫 DWORD 만 상태 번호). 생성 때 종류·크기·겹침(같은 버퍼의 한 DWORD 겹침도, 목록 순서와 메모리 순서가 반대인 경우도)을 검사해 거부한다. `globals` 는 속성으로 매번 현재값을 읽고 쓰는 실제 전역 묶음(사본 없음)이며 `player` 가 객체여야 한다. `hooks` 는 큰 외부 몸체뿐이고 도달한 경계에 몸체가 없으면 앞선 원문 쓰기만 남긴 채 명시 거부한다(가짜 성공 없음).
- immediate+0x84 의 DWORD 는 원본 큰 진행창 전역(`menuWindow`)과 같은 칸이다. cpppj 는 참조 별칭을 생성 때 검사하지만 웹은 globals 에 이 필드를 두지 않고 그 칸을 직접 읽고 써서 별칭을 구조로 보장한다.
- `request(state)`(004b2bc0): 이미 요청이 있으면 진단 뒤에도 원문처럼 새 요청으로 덮어쓰고(진단 hook 이 없으면 덮어쓰지 않고 거부), phase 를 앞당기지 않는다. `poll()`(004b88c0): 메타맵 재구성 요청(메타맵·갱신 없음)은 첫째 효과, 갱신 요청은 둘째/셋째 효과 뒤 표식을 지우고, `next` 가 있을 때만 `current` 로 전체 복사하고 `next[0]=0` 으로 하며 Run 한다. 효과가 바꾼 현재 `next` 도 같은 위치에서 다시 읽는다.
- `run()`(004b6dd0)의 공통 확정: 반복마다 `next`/`immediate` 에 `current` 전체를 복사하고 두 상태 번호를 0 으로 쓴 뒤 로그 → 작은 상태 몸체 → `old=phase; phase=current; current[0]=0` 으로 확정한다(timeout 같은 early return 은 확정하지 않고 `current` 번호도 지우지 않는다). 즉시 요청이 있을 때만 `current=immediate` 로 같은 호출에서 이어 처리하며 예외가 나도 실행 중 표식은 해제되고 앞선 쓰기는 남는다. 상태 실행 중 `run`/`dispatchQueued` 는 재진입으로 거부한다.
- 작은 상태 몸체: **7**(정리 전 Zone 대기값과 재접속일 때만 게임 시각/출처를 보관 → 제목 첫 BYTE 0 → 전투 정리(1) → CD 확인 → 명령 숨김 → 현재 표식을 다시 읽어 시각→출처 복원 → 전투 중 표시 → 음악 정리 → 미션 월드+후처리면 미션 가상 갱신 → simulconnect 면 listener → 보관한 Zone 대기값이 있으면 현재 Zone 핸들 설치/해제 후 두 블록의 핸들을 0 으로 쓰고 맵 처리기 등록 → Renderer → PlayerKnown 0 → UI 모드 2 → 주변음 기한 `현재 벽시각+10` → immediate+1=20, 요청 8 → 재접속이면 Fort 읽기+재접속 안내, 미션 후처리면 미션 안내, 아니면 서버 밀도·Chat 0x40/0x80 정리·연결 안내 → 배틀 팔레트 적용), **8**(만료 미예약이면 벽시각+30 예약과 일반 전투 연결/재접속 안내, 연결 요청 뒤 9), **9**(상태 조회 → 비서버의 pending 연결을 9 번으로 옮김 → 만료(`connectEnd <= 벽시각`)면 출처 0·next 3 로 early return → 닫힘/오류면 진단 후 8 재요청 / 연결 중이면 남은 초(캐시 시각 기준 0 쪽 절삭, 부호 있음)를 안내하고 9 유지 / 연결됨이면 로그인 송신·진행 창·요청 10), **10**(연결이 끊기면 ServerLost 안내·출처 0·next 3), **13**(진행 창 해제·Commander·Fort 확보 뒤 Template Core 의 모드 비트(0xfb 보존+2)·주소 4바이트·포트(주소 종류 2바이트가 6 이면 12 번, 아니면 2 번 바이트, 바이트 교환)·게임 시각(0 쪽 절삭 하위 DWORD)·전투 BYTE·갱신 횟수 +1(16비트)을 직접 쓰고 Fort 갱신 → UI 모드 3 → +84 표식 지움 → 요청 14 → 재접속 SP → 연결이 닫혔으면 출처 0·next 3, 아니면 로그인 타이머 시작과 입력 모드 전환), **15**(서버 인수 부모만 부르고 몸체가 즉시 16 을 요청), **4·12·14·16**(작은 몸체 없이 공통 확정), **1·2·3·6·19~42**(큰 부모를 한 번 부르고 반환 종류가 ReturnWithoutCommit 이면 early return), 그 밖은 진단.
- `pollStarting(body)`/`runStarting(body)`: 실제 호스트가 분리해 수행하는 State7 몸체를 같은 공통 확정에 연결한다(최초 복사/로그가 몸체보다 먼저, 몸체가 `request(8)` 하면 같은 호출에서 8 이 이어짐). 다음/현재 상태가 7 이 아니거나 몸체가 없으면 아무것도 복사하지 않고 거부한다. `captureLocalMissionStarting`/`restoreLocalMissionStarting` 은 같은 보관·복원 규칙의 독립 함수이다(복원은 최초 조건을 캐시하지 않고 현재 표식을 다시 읽는다).
- `enqueueReceive(fn)`/`dispatchQueued()`: 송신 callback 이 실제 수신기를 큐에 넣고, 공통 확정(phase 10) 뒤에만 선입선출로 실행하며 실행 중 추가된 예약도 같은 호출에서 이어 실행한다.

## 2. 기준선 뒤 변경 판정 (+18줄)

- **옮김(② 적용, 현재 소스 그대로)**: 일반 시작 밀도·listener·연결 안내(`7f1ae00ee`), 일반 복귀 State6 의 큰 부모(`14369776e`), State15 서버 인수와 16 직행(`9986097ae`), 재접속 State7 의 Fort 읽기/진행 창(`e064f1752`), 메타 State19~42 의 큰 부모와 메타 Poll 선행 효과(`c1c8bd0ad`·`6bff48b8b`·`eb680d67d`·`23b16b54f`).
- 옮기지 않은 변경은 없다. 실제 진행 창(HTML)·Fort 파일·연결·UI 는 hooks 로 남겼으며 각각 4-D/5-A/6 의 몫이다. CD(10.72)는 옮기지 않았고 판본 인자도 없다.

## 3. 검사 (`tests/raw-local-mission-state.test.mjs` 17건)

- **원본 독립 관측 `local-mission-state-x86.tsv` 33행**(Request/Poll/Run × 선택 상태 × 모드·연결·Fort·메타 변형 × 호출 도중 변경): 다섯 블록 720바이트·전역 15개·두 벽시각 예약 double 비트·Template Core 192바이트·외부 효과 순서를 모두 대조한다(첫 실행에서 33행 전부 일치).
- 손 시험(cpppj `LocalMissionStateTests.cpp` 이식 + 기준선 뒤 경로 + 변형 확인으로 찾은 경계): 송신 위치의 phase 9 미확정과 수신 큐 순서, 저장소 구성(종류·겹침 방향·한 DWORD 겹침·player)과 필수 소비자 누락, 큰 부모의 확정/early return 과 미복원 상태 진단, State15·4·12·14·16, 일반 전투 State7(밀도·Chat·연결 안내·listener·주변음 기한), State8·9 연결 안내와 남은 초(음수·범위 밖), 호스트 State7 몸체 계약, 메타 Poll 선행 효과(표식·체인·누락), 요청 덮어쓰기 진단·Run 재진입, State13 Core 작은 필드(주소 종류·포트 교환·게임 시각 절삭·갱신 횟수 감김·크기 검사·owner·표식 지우기 시점), State7 의 CD 값·Zone 조회 시점, 캡처·복원 함수.

## 4. 변형 확인과 남은 경계

- 변형 226개 중 첫 실행에서 30개가 남았고, 위 손 시험을 보강한 뒤 **6개만 남았으며 모두 동등**하다: 생성자 겹침 검사의 마지막 반복 제외(마지막 블록은 비교 상대가 없음)·생성자 전역 묶음의 `typeof`/`null` 분기 둘(같은 `TypeError` 경로)·생성자의 `state?.` 옵셔널 체이닝(같은 `TypeError`)·State13 모드 비트의 마스크 `0xf9` 대 `0xfb`(`|2` 뒤 같은 비트)·State13 의 `next[0]=3`(바로 이어지는 요청 14 의 다음 반복이 `next` 를 `current` 로 다시 채워 관찰 불가).
- 시험이 알려 준 일: State7 이 Zone 핸들을 0 으로 쓰는 것은 `immediate` 가 이미 복사본을 가진 뒤라 최종 `current` 에는 남지 않는다(원문 그대로이며 hook 이 보는 순간에만 0). `connectEnd == 벽시각` 은 이미 만료이다.
- 3.1절 4-C 는 17/30 이다. 다음: Player(미션 시작 상태 중 자원·기술·동맹·색).
