# webpj ThunderProcess·SpellFrameVisibility (WB04-E381, 2026-11-13, 5-A 열 번째 단위)

5-A 의 시각 효과 소단위 중 client **ThunderProcess**(cpppj `client/ThunderProcess.cpp/.h`, 기준선 32줄)와 **SpellFrameVisibility.h**(11줄)를 `core/thunder-process.js`·`core/spell-frame-visibility.js` 로 옮겼다. 두 파일 모두 기준선 뒤 cpppj 변경이 없다(마지막 변경 `890219ac6` 는 기준선 `1f00a25a` 안, base `1f00a25a`). 같은 줄에 묶여 있던 `ConstructionVisualRenderer`(59줄)는 선행 `DrawSubmission` 을 아직 옮기지 않아 다음 단위로 남긴다.

## 1. 규칙

- **ThunderProcess(TYPE 55, 0054114c):** E376 의 `PaletteFlash`(번개 팔레트 계산)를 `SquidProcess` 하나로 감싼 프로세스다. 부착은 `host.attach(process, THUNDER_PROCESS_TYPE, 부모 SID, 0x10)` — 원문 생성자는 flags `0x10` 으로 기술 부모에 붙는다.
  - 생성 인자: `fullScreen`(지속 — 전체화면 0.5초·창 모드 1/60초), 장치 훅 묶음, 소멸 통지 `destroyed(pid)`(선택), `brightness`(signed, 0 이면 기본 +64). 팔레트는 **부착 전에는 읽지 않는다**.
  - `onAttached()`(원본 가상 +14): `PaletteFlash.init()` — 부착 직후 현재 팔레트를 두 번 읽는다.
  - `runFrame()`: `PaletteFlash.frame()` 이 끝났다고 하면 원문 0041bf10 처럼 `host.kill(this, 0)` 로 Form 삭제 경로를 쓴다. kill 뒤에는 `this` 를 다시 읽지 않는다.
  - `destroy()`(C++ `~ThunderProcess`): `cancel()` 로 화면이 살아 있는 동안(초기화됨 && 전체화면) 저장 팔레트를 복구하고, `destroyed(this.pid)` 로 Client 의 현재 PID 목록만 정리한다. 자연 종료는 PaletteFlash 가 이미 복구하고 `initialized` 를 껐으므로 추가 팔레트 변경이 없다. Form 은 중복 삭제하지 않는다.
- **SpellFrameVisibility:** `suppressSpellHelpFrame(type, special, frame)` — 폭탄 종류(`TypeFlag2.bomb` = `0x100`) && 도움말 프레임 ≥ 0 && 지금 프레임 == 도움말 프레임일 때만 true. 원본 10.78 도 Edit 비석→스펠 전환 뒤 도움말 그림을 깜빡이는데, 2026-10-28 사용자 요청으로 cpppj 가 원본과 다르게 그 그림만 임시로 숨기며 웹도 같은 규칙을 따른다. 프레임 진행·예약·효과·삭제는 건드리지 않는다. 판본 인자(`Patch1078`)는 웹이 10.78 만 다뤄 옮기지 않았다.

## 2. 웹 범위

- 실제 188 TYPE(`realPlayerAssets`)·`SquidFactory`·`RawSquidDestroy`·`SquidProcessHost`·`Kernel`·`SquidDestroyLifecycle` 을 한 풀에 묶은 장면에서 시험한다. 부모는 cpppj 와 같이 실제 다리(TYPE 82) 생성자가 만든 void 다리다. 시계·전체화면·팔레트 장치만 합성 입력이다.
- **옮기지 못한 것(후속):** ① cpppj `ThunderProcess_RealFormNaturalEndAndSummary` 의 `procSumm.txt` 요약(`WriteRawProcessSummary`) 단언 — 웹에는 개발 파일 출력이 없어 Kernel 진단 장치 몫이다. ② `PaletteFrameChecking_ActualClientClockKernelAndFormRetirement` — 실제 `Client` 프레임 시계·일시정지·checking 소유자가 필요해 4-D Client 본체와 함께 옮긴다.
- 소비자(`GameWorld`·`RawCampaignFactoryDisplay`·`RawCampaignSession` 의 스펠 파편 캡처, 번개 명령 `IssueCommands`)는 각 단위에서 이 모듈을 연결한다. 이 단위는 앱 수명에 연결하지 않았다(`PORT_MAP` session 0).

## 3. 검사 (`tests/thunder-process.test.mjs` 10건·`tests/spell-frame-visibility.test.mjs` 5건)

- cpppj 대응 4건: 자연 종료(Form 삭제·소멸 통지 번호·팔레트 불변·종속 머리 0), 중단 복구 한 번(`host.kill(…, 0x10)`), 부모 삭제로 같은 프로세스 해제(복구 1·통지 1·부모/Form 모두 삭제), signed 밝기 −17 이 실제 Form 생성자를 거쳐 색 228~245 의 `100/120/140 → 83/103/123` 에 도달.
- 웹 계약: TYPE 55 리터럴, 기본 밝기 +64(255 제한), 창 모드는 중단해도 화면 팔레트 불변, 자연 종료 프레임의 `host.kill` flags 0 한 번과 중복 복구 없음, 생성 인자 생략 시 기본값, 모드별 지속(0.1초에 창 모드는 끝나고 전체화면은 계속되며 0.5초 뒤 복구), 불완전한 장치 묶음 거부, 부착 전 번호 0.
- 스펠: 세 조건 조합표(다른 flags2 비트 전체 켬/끔 × 도움말 프레임 −1·0·5·1000 × 프레임 −1~0xffffffff), 도움말 −1·0 경계, 실제 10.78 타입 정의에서 폭탄 타입은 도움말 프레임 한 장만 숨기고 비폭탄은 아무것도 숨기지 않음, 32비트로 접히는 큰 값(`2**32+5`)은 같다고 보지 않음(C++ `int64` 비교).
- 모두 **첫 실행에서 통과**했다(원본 관측 fixture 가 없는 소스 계약 단위라 manifest 변경 없음).

## 4. 변형 확인과 남은 경계

- 자동 연산자·상수 변형 10개와 손으로 쓴 구조 변형 21개를 적용했다. 1차 생존 5개 중: 기본 밝기 `0 → 1`(내 장면 도우미가 인자를 항상 명시했음)·모드별 지속 `!fullScreen`(자연 종료 시험이 1.0초를 써서 둘 다 끝남)·`(frame | 0)` 32비트 접힘 — 시험을 더해 모두 잡았다.
- 남은 동등 변형 1개: 기본 밝기 `0 → 64`(`thunderOffset(0)` 이 64 라 같은 값).
- 다음 5-A 항목: `DrawSubmission`(145줄, 순수 수집·분배 로직)을 앞에 두고 `ConstructionVisualRenderer`(59줄) → 렌더러 프레임 본체.
