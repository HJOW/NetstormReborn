# webpj 미션 Guide 입력 — MissionGuideInput·ScriptCommand 큐 (WB04-E343, 2026-11-12, 4-D B 구획)

3.1절 4-D B 구획의 client **MissionGuideInput**(기준선 `.h` 46 + `.cpp` 74 = 120줄, 기준선 뒤 변경 없음)을 `core/mission-guide-input.js` 로 끝까지 옮겼다(E341 이 `MissionFirst` 만 먼저 옮겼다). 이 파일의 제어 명령 예약이 쓰는 **ScriptCommand** 의 명령 큐 넣기와 80칸 문자열 고리(기준선 `.h` 41 + `.cpp` 46 = 87줄, 기준선 뒤 변경 없음)가 웹에 없어(`ui/script-command.js` partial) 같은 단위에서 `ui/script-command.js` 에 더했다(3.0절 "필요한 공유 장치가 없으면 먼저 이식"). 두 PORT_MAP 행이 모두 partial → port 가 되었다.

## 1. 규칙

- **ScriptStringRing(004c7910):** 원본 80칸 문자열 고리이다. `store(text)` 는 다음 칸의 이전 문자열을 해제한 뒤 첫 NUL 앞 문자열을 복사하고 비영 불투명 토큰을 돌려준다 — **같은 문자열도 매번 새 칸과 새 토큰**(첫 토큰 101)이며 81번째 복사부터 가장 오래된 칸의 이전 수명이 정확히 무효가 된다. 토큰 수명이 소진되면(2^31−1) 칸을 비운 채 거부한다. `resolve(token)` 은 현재 80칸 안의 실제 토큰만 해석하고 해제되었거나 잘못된 토큰(0 포함)은 `RangeError`. `index` 는 현재 고리 위치(첫 `store` 에서 1).
- **queueScriptCommand(source, queue, strings, hooks)(004c9090):** 스크립트 명령을 페이지 전환 예약과 섞지 않고 40칸 입력 큐의 명령 사건으로 예약한다. 이름(구분자 `#,;>`)과 인자(구분자 `;>`, 쉼표 보존)를 읽고 **인자 변환 전에** `hooks.lookup(이름)` 으로 명령 코드를 조회한다. 코드 0 은 `hooks.unknown` 진단 뒤 **계속**(코드 0 인 명령 사건을 예약)하고, 0xffffffff 는 인자 처리(고리 칸 소비 포함)를 마친 뒤 진단하고 예약 없이 끝낸다. 인자가 숫자 글자로 시작하면 atol(32비트 감김), 그 밖에는 고리 토큰이며 두 좌표 DWORD 에 같은 값을 쓴다. 표 조회·진단 연결이 없으면 도달한 위치에서 거부한다.
- **MissionGuideInput:** 일곱 제어 명령 코드(`MISSION_FIRST/RELOAD/ABORT/LEAVE_BATTLE/BEGIN/FORK/END_COMMAND`)와
  - `missionControlScriptCode(name)`: 여섯 이름(MissionRestart·MissionAbort·LeaveBattle·MissionBegin·MissionFork·MissionEnd)을 ASCII 대소문자 무시로 조회, 그 밖은 0xffffffff.
  - `queueMissionControlAction(name, argument, queue, strings)`: 지원 이름만 `이름,인자` 로 `queueScriptCommand` 에 예약하고 true, 미지원 이름은 상태·고리를 바꾸지 않고 false.
  - `dispatchMissionAbort(event, queue, tell)`(00463e40): 상위 플래그를 뺀 코드가 Abort 이면 사건의 y(세 번째 DWORD)가 0 일 때 `tell('ABORT')`, 아니면 LeaveBattle/2/2 를 예약하고 항상 소비한다(Tell 연결 없으면 거부).
  - `dispatchMissionFirst(event, currentFirst)`: 플래그 제거 후 First 와 일치하면 현재 미션이 없어도 true. `queueMissionFirstKey(event, queue, activeMission)`: 활성 미션의 F8 key-down(뗌·Ctrl·Shift 제외)을 큐 끝의 First/0/0 으로 예약(즉시 Tell 하지 않음).
  - `captureMissionReload(name, mode, demo)`(00463cf0): 이름을 소유 복사하고 mode/demo 를 0/1 로 정규화. `applyMissionDemo(text, demoBox)`: 빈 문자열이면 기존 직접 지정값 유지, 아니면 atol DWORD(공백/잘못된 숫자는 0).
  - `tellMissionStage(gate, shownBox, effects)`(004c2a90): demo 면 `Demo`, 아니면 단계 글자(`gate.stage` 의 하위 바이트, 0 이면 빈 섹션)+`.` 섹션을 Tell, 비영이면 팝업 닫기→정지, **그 호출의 반환값**을 `shownBox` 에 쓴다.
  - `missionFirst`(E341): 순번 0 이면 flags 값 1 만, 그 밖에는 안내 재표시.

## 2. 웹 범위

- 기준선 뒤 cpppj 변경이 없다(`MissionGuideInput.*`, `ScriptCommand.*`). PORT_MAP: `MissionGuideInput.cpp` partial → **port**, `ScriptCommand.cpp` partial → **port**(Config 명령 `applyScriptConfig` 와 토큰 읽기는 기존 단위의 것을 재사용, 명령 큐·문자열 고리 추가). 앱 수명 연결(활성 입력 처리 루프가 사건을 Guide/큐로 분배)은 6단계 입력·4-D 조립 때 한다(PORT_MAP session 0).
- cpppj 시험 `MissionControl_AuthoredReplayAndAbortConfirmation`(보호 tell 스크립트의 TryAgain/ABORT 본문을 실제 `ParseDialogPage` 로 해석해 Replay Mission 버튼이 같은 MissionRestart/0 을 예약하는지)과 `ScriptCommand_ConfigOverwriteAndQueueLifetime` 의 DialogPage·옵션 저장 부분은 `DialogScript`(PORT_MAP todo)와 옵션 저장 단위가 필요해 그 이식 때 옮긴다. 이 단위는 보호 tell 스크립트에 해당 섹션이 있는지는 확인하지 않고(MissionScript 와 함께 E342 이후 가능) 명령 예약 계약만 확인한다.

## 3. 검사

- `tests/script-command-queue.test.mjs` 6건: 원본 관측 `script-command-x86.tsv` 의 Command 행(이름/인자 토큰·표 조회 시점·코드 0 진단·0xffffffff 진단·큐 사건·고리 위치)과 Ring 행(연속 store/resolve, 81번째 복사부터의 수명)을 대조하고 cpppj 큐 수명 시험, 연결 누락 거부·코드 0 흐름·숫자 인자 32비트 감김·부호 시작은 문자열, 고리 첫 토큰 101·같은 문자열 새 칸·80칸 뒤 덮임, 큐 가득 참(39개) 시험. 기존 fixture 라 manifest 의 `script-command` 항목에 소비자만 더했다.
- `tests/mission-guide-input.test.mjs` 8건: 원본 관측 `mission-first-x86.tsv` 278행(전체 208BYTE·반환·호출 순서), `mission-common-x86.tsv` 108행(Init demo·Reload 캡처), `mission-abort-x86.tsv` 1002행(A 960 + 제어 이름 조회/토큰/큐 S 42) — **첫 실제 실행에서 모두 일치**. cpppj 시험 `QueueOrderCapacityAndRelease`(큐 용량·release·Ctrl/Shift·비활성) 이식, 제어 이름·Abort 분기·First 분배·안내 표시(demo/단계 글자/반환값 마지막 기록)·Reload 캡처/demo 문자열 계약. manifest 에 `mission-first`·`mission-common`·`mission-abort` 세 항목을 추가했다.

## 4. 변형 확인과 남은 경계

- 한 문자열 치환 변형 62개(mission-guide-input.js 42 + script-command.js 큐 20)를 돌려 첫 실행에서 6개가 남았다. 다섯은 시험 보강으로 잡았다: demo 문자열의 `parseInt` 대체(2^64+1 같은 큰 값의 정밀도 시험), Tell 반환의 부호 있는 음수를 DWORD 로 쓰는 `>>> 0`, 그리고 연결 누락 거부 세 곳(안내 Tell·First 안내·명령 표 조회)의 진단 메시지 단언(제거해도 `undefined(...)` 호출이 `TypeError` 로 어차피 실패해 보이던 것). **남은 동등 변형은 1개**: `queueMissionControlAction` 이 `이름;인자` 로 합쳐 넘기는 변형 — 명령 이름 구분자 `#,;>` 에 `;` 가 들어 있어 이름과 인자 분리가 쉼표와 같은 결과이다.
- 남은 경계: 활성 입력 루프(6단계)에서 사건을 Guide/큐로 분배하는 호출자, DialogPage 파서와의 연동 시험(`AuthoredReplayAndAbortConfirmation`), 실제 Tell 창(5-A/6)·정지(`MissionLifecycle`/`RawMissionFrameGate`)와의 연결.
