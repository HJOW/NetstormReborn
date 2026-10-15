# cpppj 작업 중단 인수인계 — 2026-10-15

사용자의 명시 중단 요청에 따라 세 서브 AI를 모두 interrupt하여 대기시켰다. 이후 작업은 현재 변경분의 회귀 검사, 오류 교정과 문서/로컬 커밋 마감으로 제한한다. 이 문서는 재개 지시가 있을 때까지 후속 개발을 예약하지 않는다.

현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon, 2026-10-15**로 일치한다. 다른 PC에서 디컴파일 소스를 분석하려면 AGENTS.md의 재디컴파일 규칙을 먼저 적용한다. 보호 원본·AGENTS.md·dotnetpj는 수정하지 않았으며, 게임/GUI 실행과 원격 push는 없다. DirectDraw 전체화면 오류 수정은 이번 목표에서 제외한다.

## 마감 대상

직전 완료 커밋은 `18a384d` (`20261015-64 CP 미션 현재 SP와 설정 및 로그인 송수신 패킷 연결 검증`)이다. 당시 Release 경고0·오류0·집중52개 실패0·전체1,290개 실패0이었다.

이번 단위는 다음 실제 몸체와 검사를 빌드에 등록한다.

- 현재 Fort 35섹션 SaveImage와 0x2a 송신: 원본219개 입력, C++4개 검사. 실제 예약 image+20과 송신 image+17, 변경된 현재 Template/길이/watcher 조회를 구분한다.
- 전투 옵션: 원본3,305개 입력, C++2개 검사. 실제 TYPE+c4는 `cost`다. signaling NaN은 Unicorn 관찰 한계를 분리하고 실제 호스트 x87의5개 진단을 별도 보존한다.
- 생산 Deck/Indicate 및 현재 Factory 재계산: 원본248개와242개 입력, C++5개 검사. 같은 Player+68·Factory 목록·ContentForm·금지표를 사용한다.
- 로그인 설정0x32: 원본164개 입력, C++3개 검사. 같은7개 현재 DWORD의 송신 조회 순서와 수신 쓰기 순서를 분리한다.
- 같은 실제 Session의 생산 바인딩: C++2개 검사. 금지DWORD 표와 원본 세 카운터, 현재 Player/기술 내용물/생산 Deck이 같은 저장소를 사용한다. Client의 AI/Deck 초기 바이트는 현재 CRT 할당에서 복사하고 생성자가 초기화하지 않는 부분을 임의0으로 채우지 않는다.

새 원본 입력은 모듈마다 별도 보존한다. 실제 게임 시작·생산 거래·자연 완주를 수행한 횟수로 합산하지 않는다.

## 이번에 교정한 실패

생산 검사 첫5예외는 검사 월드의 빈 표면 전역이 원인이었다. 실제82/94/95/155/157을 공급했다. 다음294CHECK는 신전 입력을 rainCannon129와 혼동한 문제였으며 검사와 native의 입력을 실제 rainVortex124로 함께 교정했다. 원문242개를 두 정밀도로 재생해 정상 반환했고 결과 fixture SHA는 변하지 않았다. 비교 조건과 제품 범위 검사는 약화하지 않았다.

같은 Session 검사3CHECK는 실제 제품 결함이었다. `core.bookkeeping.factories.entries`의 크기가0이어서 공통 PostPop의 Append가 저장 신전 두 개를 조용히 누락했다. 보호 PE의 실제 `004fc990`은 `push 0x1f4; mov ecx,0055a4d0; call 0044fb20`으로500DWORD를 확보한다. 같은 Session도 첫 객체 생성 전에 정확히500칸을 예약하도록 교정했다. 목록을 나중에 스캔하여 채우거나 가짜 생산 항목을 추가하지 않았다.

초기 실패 로그는 `goal-fort-options-production-focused.log`, `goal-session-production-final-focused.log`, `goal-session-production-after-input-correction.log`다. `goal-session-production-diagnostic-focused.log`는 진단 빌드 종료 전에 이전 실행 파일을 읽은 결과라 최종 검증에서 제외한다. 일치 검사0건인 잘못된 필터도 초기 로그에 남아 있으며 통과 검사로 세지 않는다.

## 재개할 미완료 작업

실제 Session/UberGump의 로컬 Zone→0x27→worldLoad→Ready40→벽시각 로그인 종료→Guide→Stage3 연결은 아직 미완료다. GameWorld 생성자는 여전히 즉시 Load하며 GUI 브리핑 경로는 새 원본 상태 엔진을 사용하지 않는다. 자동 Stage3/미션 게이트/Outcome을 완료된 것으로 표시하지 않는다.

다음5파일은 **CMake 미등록·빌드/검사 미실행 초안**으로만 보존한다.

- `cpppj/src/o/RawLocalMissionState.{h,cpp}` 및 `tools/decomp_local_mission_state.py`: 실제0x90BYTE 블록5개(old/phase/immediate/current/next), 선택 상태7~10/13/14, Request/Poll/Run 공통 확정과 비동기 수신 큐의 초안이다. 테스트 파일·fixture·JSON·상세문서는 아직 없다. 도구의 native 생성 및 `--verify`를 완료했다고 주장하지 않는다.
- `cpppj/src/client/RawCampaignMissionStartBindings.{h,cpp}`: 같은 Session/MissionScript/게이트/실제 AI 설정을 연결하는 초안이다. 독립 검사·보고서·보호8미션 검증은 아직 없다. 현재 AI 이름을 원본 좁은문자열로 인코딩하는 경계도 재검토해야 한다. 실제 AI 기본 전역 `stuffRefreshRate=4`는 외부 장기 상태가 공급해야 하며 값형 기본3을 실제 기본으로 사용하지 않는다.

상태9의 SendLogin callback 안에서 응답을 동기 실행하면 이전 phase8/9를 읽어 Reply가 pending3을 요청할 수 있다. 원본처럼 실제 수신 큐에 보관하고 State9→10 공통부 확정 뒤 분배해야 한다. Reply의 동기 Run 뒤 phase를 바깥에서 덮어쓰면 안 된다. 로그인 타이머 종료는 pending0을 쓰지 않고 진행 창을 해제한다. Guide는 pending 상태와 실제 다이얼로그 목록을 각각 조회하며, seq3이 Start이고 seq6은 해제다. 이 관찰은 초안의 전수 검증을 대신하지 않는다.

공급원 목록과 Player별 Factory 목록의 초기 capacity도 재개 시 원본 초기화와 대조해야 한다. 이번 교정은 공통 Factory500에 한정한다. 일반 Construct/빌더23/manaBolt39 이동, 생산 GUI와 구매·연구, 다리/수확/운반/희생, 자연 승패/저장/다음 미션 및 모든 픽셀·음향 동일성은 남아 있다. 생산 Help 명령00542754를 구매로 연결하면 안 되며, Factory 연구의 실제 명령은005428f8→004547d0이다.

Early Missions1~6와 캠페인1-1~1-2의 자연 완주 및 원본 동일성이라는 목표는 **미달성**이다. 목표를 완료 처리하지 않고 사용자 요청에 따라 일시 중단한다.

## 최종 회귀

최종 고정 소스로 **Release 경고0·오류0·집중42개 실패0·전체 CTest1/1·내부1,306개 실패0**을 확인했다. CTest 검사 시간199.50초, 전체 경과199.54초다. 로그는 `extracted/goal-pause-final-{build,focused,ctest,tests}.log`다. 새 native 도구5개의 `--verify`는 모두 종료0이며 UTF-8과 초안 Python 구문 검사를 수행했다. 상태/Start 초안5파일은 이 빌드와 검사에 등록되지 않았으므로 위 통과 수에 포함하지 않는다.

AGENTS.md를 커밋 직전에 다시 읽고 **`20261015-65 CP 미션 생산과 Fort 및 설정 처리 검증과 중단 인수인계`**로 로컬 커밋한다. 목표는 사용자 요청에 따라 `paused`로 전환한다.
