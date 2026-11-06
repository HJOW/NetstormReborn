# 995 — 공유 게임 시계의 보정 주소 수명

2026-11-07, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 호스트의2026-11-03이다. 993에서 연결한10.78 `00460cd0`(CD `00401130`)의 GameTicks 바인딩 수명을 후속 검토했다.

Client는 실제 GameClock을 shared_ptr로 외부 전송/소비자에 빌린다. 따라서 GameTicks의 원시 주소만 보관하면 공유 시계가 Client의 LocalState 소유권보다 오래 남을 수 있다. `BindSynchronizationTicks`가 같은 LocalState owner도 유지하도록 바꾸고 실제 Client 초기화에 전달했다. 값이나 시계를 복제하지 않으며 현재 DWORD 조회·정지/재개/Reset의 기존 원문 동작은 유지한다. 독립 명시 호출은 소유자를 생략할 수 있고 해당 caller가 주소 수명을 보장한다. 보정 저장소는 GameClock을 참조하지 않으므로 새 소유권 순환은 없다.

Release 제품 빌드는 종료0·경고/오류0이다. 활성 솔루션이나 검사 소스를 수정하지 않고 `extracted/cpp-resume-995/nonmp` 임시 CMake 대상으로 기존 `FocusedTestMain.cpp`/`GameClockResetTests.cpp`만 현재 제품 `netstorm.lib`에 링크했다. 별도 검사 빌드도 종료0·경고/오류0이며 기존 비멀티 Reset4검사는 모두 통과했다. 원시138행 중 공개호출114행, 원점/Frame 이력, 중첩 정지/재개, 밀리초 감김을 확인한 범위다. 새 보정 수명 자체는 실제 owner 연결과 참조 방향을 정적으로 검토했으며 새 MP 검사로 실행하지 않았다.

제품11.01은11,648,000바이트, SHA-256은 `7fa28b8bc7e38f15377d32f443eba3d603bd3c7d3fc45c3439b098de2a84e078`다. 로그는 `extracted/cpp-resume-995/build.log`, `nonmp-configure.log`, `nonmp-build.log`, `nonmp-tests.log`이고 보고서는 `cpppj/clock-owner-lifetime-20261107.json`이다. 검사 뒤 주소 설명 주석만 보완했다.

신규 검사·fixture·MP/시간패킷/통신/게임 실행은0이다. 보호 AGENTS.md·원본/CD·fixture2800파일은 모두 불변이다. 일반 State6/Fort 완료 계약·Multiplayer Options/일반 State7 호스트/roster/LAN UI 등 기존 잔여를 해소한 단위는 아니다.
