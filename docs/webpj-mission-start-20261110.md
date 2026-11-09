# webpj 공통 미션 시작(RawMissionStart) 이식 — WB04-E234

2026-11-10, HJOW-Athlon. 공통 미션 시작 순서(PATCH 004c2b20 Start)와 첫 tellAt 예약(004c3100)을 옮겼다. 설정 키(`denySalvage`·`denyAscend`·`allowAnyCapture`·`moreGeysers`·`demoRestartTimer`·`myStartMoney`·`myTech`·`randGeysers`)를 읽어 전역과 데모 기한에 쓰고, 섹션 이름 목록을 `tellAtList` 에 쓰고, 시작 SP·기술·AI 참가자·명령 Gump·카메라를 정리한 뒤 `started = 1` 을 쓰는 순서다. 같은 fixture(`mission-start-x86.tsv`)에 들어 있는 AI 설정(`RawMissionAiSettings` apply/prepare/setAbility)과 AI 생성(`RawAiCreate`) 관측도 처음으로 웹에서 대조했다. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절의 "순서4 이후로 넘기는 `o/` 단위"에 속하며 기준선 뒤 변경이 없고 원본 관측이 있어 E231 공통 프레임 문·E233 승패 조건에 이어 옮겼다.

- cpppj 기준선 `1f00a25a`. 소스(`RawMissionStart.cpp/.h`)·시험(`MissionStartTests`)·fixture(`mission-start-x86.tsv`) 모두 기준선 뒤 변경이 없다(`git diff --numstat 1f00a25a HEAD` 를 저장소 루트에서 확인, 출력 없음). 함께 대조한 `RawMissionAiSettings.cpp`·`RawAiCreate.cpp` 도 변경 없다.
- 근거 커밋: 소스 `3cea1b14`(2026-11-01), fixture `69c584bd`(2026-10-15).
- 원본 fixture: `mission-start-x86.tsv` 204개 고유 입력(10.78 전용): `start` 48(공통 Start 전체 상태·double 비트·조회/효과 순서)·`init` 20·`apply` 64·`prepare` 8·`ability` 32(AI 설정 600 BYTE 전체)·`create` 32(AiCreate 뒤 Player 180 BYTE·반환 토큰·효과 순서). 입력은 `|` 로 나뉜 정수 목록과 설정 키=hex 값 목록이다.
- 공유 장치: E231 `raw-mission-frame-gate.js`(상태), E32 `raw-mission-ai-settings.js`(`configListItem`·설정 처리기), E164 `raw-ai-brain.js`(`RawAiCreate`), E5 `raw-player-participation.js`, `assets/config.js`(`configParseLong`), `game-aux-state.js`(장기 전역 소유자).

## 1. 옮긴 것

`www/core/raw-mission-start.js`(DOM 없음, 10.78 전용)

- `createMissionStartState()`(flags·sequence·tellAtIndex·fieldC0·fieldC4)·`createMissionStartGlobals()`(독립 입력 localOwner 1·playerKnown·editor와 출력 denySalvage·denyAscend·allowAnyCapture·ordinaryGeysers, `shared` 에 같은 이름의 `{ value }` 상자를 넣으면 실제 장기 owner 에 읽고 쓴다).
- `RawMissionStart(gate, state, globals, hooks)`: 현재 설정 조회 `get` 이 있어야 생성(없으면 TypeError). 나머지 효과(`sectionNames`·`set`·`gameSeconds`·`setSp`·`refreshSp`·`clearTechnology`·`addTechnology`·`hasPriest`·`createAi`·`refreshCommandGump`·`randomGeysers`·`resetCamera`)는 도달한 위치에서 필수이며 누락은 TypeError 로 앞선 효과를 남기고 `started` 를 쓰기 전에 거부한다.
- `start()`: 보조 필드 0 → 설정 네 개(`moreGeysers` 는 논리 부정) 쓰기 → `demoRestartTimer` 비영이면 데모 종료 기한 = 값 + 현재 게임 시각(double) → `@` 섹션 목록을 `tellAtList` 에 쓰고 첫 tellAt 예약, sequence +1, flags 비트 0 지움 → 시작 금액(편집 모드면 194162, 단정도 float 로 줄임)을 PlayerKnown 이 있을 때 현재 로컬 owner 에 지정 → SP 표시 → 현재 owner 기술 제거 → `myTech` 추가 → owner 1..8 중 로컬이 아니거나 데모일 때 사제가 있으면 AI 생성 → 명령 Gump → `randGeysers` 비영이면 랜덤 간헐천 → 카메라 → 시작 시각·`started = 1`. 로컬 owner 는 효과마다 다시 읽고, `myTech` 앞의 owner 는 보관한다.
- `refreshTellAt()`: 현재 tellAtIndex 번째 항목의 첫 글자 뒤를 atol 로 읽고 항목이 없으면 0.
- `bindCurrentInputs({ localOwner, playerKnown, editor })`: 세 getter 가 모두 있어야 연결하고(부분 연결 거부), 연결 뒤에는 값형 fallback 을 읽지 않으며 갱신하지도 않는다.

## 2. 검증

`tests/mission-start.test.mjs`(7건)

- 원본 관측 재생: 204개 고유 입력 전부. `start` 는 상태 10개 DWORD·double 3개의 비트·조회/효과 순서, `init`/`apply`/`prepare`/`ability` 는 설정 600 BYTE 전체, `create` 는 Player 180 BYTE·반환 토큰·효과 순서를 대조한다. 이로써 E32 `RawMissionAiSettings` 의 apply/prepare/setAbility 와 E164 `RawAiCreate` 가 원본 관측으로 처음 확인됐다(PORT_MAP E32 메모의 "미션 시작 override 관측은 MissionStart 단위에서").
- 현재 getter 재생: `start` 48행을 반대 fallback(owner 1↔8, known·editor 반전)으로 오염시킨 채 getter 만으로 재생해 같고 fallback 은 불변.
- 장기 owner 재생: 같은 48행에서 allowAnyCapture·ordinaryGeysers 출력이 `GameAuxState` 에만 쓰이고 값형 fallback 은 읽지 않는다.
- cpppj 계약 포팅 2건: SP/표시/기술/AI 효과마다 owner 를 다시 읽는 정확한 순서(`M;E;K;O1;S1;R;O3;C3;O4;T;A4;O6;P1;…;V`)와 getter 부분 연결 거부, 미복원 생성자/초기 갱신 거부·기존 brain(73)과 NoTemple 은 생성자 없이 반환·선택 효과(`refreshSp`) 누락 시 `started` 유지·`get` 누락 생성 거부.
- 웹 계약 2건(원본 fixture 밖 경계): 시작 금액 16777217 → 16777216(단정도), 빈 tellAt 항목 기한 0; AI 설정 `AllyList` 는 0·음수·비수치를 무시하고 33 → 비트 1(하위 5비트)로 더하며 능력 이름은 첫 NUL 에서 끊김.

최초 실패와 교정: 모듈과 204행 재생은 처음부터 통과했다. 모듈 사본을 바꿔 돌려 비교가 작동하는지 확인했다. 확인 뒤 원본으로 되돌렸다(diff 일치). 처음 변형 39건 중 3건이 살아남았다: 시작 금액 단정도 축소(웹 계약 1건 추가로 잡음), `tellAtIndex` 의 signed 변환(동등: 큰 값이나 음수 모두 빈 항목이라 같은 결과), 사용하지 않던 `readField` 도우미(제거). AI 설정 26건도 같은 fixture 로 변형 시험해 3건이 살아남았다(`AllyList` 양수 조건·능력 이름 NUL 절단은 위 웹 계약으로 잡음, 이동 마스크 `& 31` 은 JS `<<` 가 이미 하위 5비트만 쓰므로 동등).

- 실패한 변형(시작): 보조 필드·설정 키/순서·부정·데모 기한, `@` 접두·tellAtList 키·첫 예약·sequence/flags, 편집 금액·known 게이트, SP 표시·기술 제거 owner·보관 시점, owner 범위/로컬 제외/데모 포함/AI 생성, 랜덤 간헐천 조건, 카메라·시작 시각·started, 항목 기한 읽기(첫 글자 뒤)·빈 항목 기본값, 세 getter 부분 연결 검사·fallback 선택, 장기 owner 쓰기, 생성 검사(각 1건 모두 실패).
- 실패한 변형(AI 설정): 키 접두·복사 위치, 세 atof/일곱 atol 필드 위치, 색 시작, 동맹 개수·양수 조건, 능력 `!` 처리·빈 이름·미등록 진단·비트, 초기값 필드, prepare 순서.

## 3. 경계와 남은 것

- 실제 미션/세션에 연결하지 않았다. 설정 조회(Config)·섹션 이름 목록·SP 분산 지정·기술 추가·AI 생성 사슬(600 BYTE 설정 → 실제 생성자)·간헐천 랜덤 생성·카메라 초기화와 호출 지점(미션 로드 직후)은 순서4·6 몫이다(PORT_MAP session 0).
- `RawMissionAiSettings`(E32)는 이 fixture 로 apply/prepare/setAbility 까지 대조되어 `port` 로 올렸다. 이 fixture 의 `create` 32행은 `RawAiCreate`(E164)의 생성자·초기 갱신이 미복원인 채 효과 순서와 Player 기록만 맞춘다.
- CD 10.72 분기는 웹 범위 밖이다.

## 4. 다음 시작점

WB04-E235 는 3.1절에서 고른다. 기준선 뒤 변경이 없는 단위(`RawTerrainGrowth`·`RawTotalMade`·`RawObjectTraceMain`·`RawTutorialOne`)를 권한다.
