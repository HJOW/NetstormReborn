# 단독 미션의 실제 로그인 상태 전환

현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon, 2026-10-15**다. 기준은 `05d7cb5`이며 그때 CMake 미등록으로 보존했던 `RawLocalMissionState`를 처음 검증한다. 보호 원본 파일은 읽기 전용이며 원본/클론 게임과 GUI는 실행하지 않았다.

## 원본 저장소와 순서

PATCH `004b6dd0`의 상태는 번호 하나가 아니라 **0x90BYTE 블록5개**다. old005c8630/phase005c8758/immediate005c86c8/current005c87e8/next005c8878의 부가 인자까지 각각 보존한다. `004b2bc0`은 immediate만 요청하고, `004b88c0`은 next가 있을 때 current로 복사한 뒤 Run을 실행한다. 메뉴 창005c874c는 immediate+84와 실제 같은 DWORD다.

Run은 먼저 current 전체를 next/immediate에 복제하고 두 요청 번호만0으로 만든다. 선택 상태의 효과가 끝난 뒤 old←phase, phase←current를 복사하고 current의 번호만0으로 만든다. immediate가 있으면 같은 호출에서 다음 상태를 다시 실행한다. 상태9의 만료와 명시 부모의 early return은 이 공통 확정을 생략한다.

상태7은 원본 정리 전의 Zone 예약과 재접속 입력을 읽고, 선택 정리/난수/명령창/음악/Guide 갱신/Zone 설치·해제/커서/known0/UI/활동 타이머를 원문 순서대로 처리한다. 이후 같은 Run에서8을 확정하고 next9를 예약한다.9의 실제 연결 성공은 Login 송신과 진행창을 요청한 뒤 같은 Run에서10을 확정한다.13은 현재 Template Core의 실제 필드만 수정하고 UI/파일/타이머 경계를 실행한 뒤14를 확정한다. 미복원 일반 전투/simulconnect를 성공값으로 대체하지 않는다.

따라서 Login 송신 callback 안에서 응답을 즉시 분배하면 Reply가 이전 phase를 읽을 수 있다. 제공한 수신 큐는 상태 Run이 끝난 뒤에만 실제 callback을 실행한다. 다시 Run이 선택된 경우에는 그 호출이 확정한 phase를 바깥에서 덮어쓰지 않는다. 현재 큐는 단일 스레드의 실제 전송 분배를 연결할 경계이며 TCP/IP나 전체 상태 부모를 복원했다고 주장하지 않는다.

## 독립 대조와 교정

[`관찰 도구`](../../tools/decomp_local_mission_state.py)는 보호 PE의 실제 상태·Request·Poll·시계 변환/예약/만료 명령을 실행하고 정상 스택/보존 레지스터/x87 균형과 허용된 쓰기 주소를 검사한다. **33개 고유 입력**에서720BYTE 상태,14DWORD+title BYTE,두 double 타이머,192BYTE Template 입력 전체 및 외부 호출 순서를 보존했다. 전체 정밀 소스의 선택 함수 범위만 명령 실행을 허용한다.

파일/VFS, 큰 전투 정리, Zone 설치, Template wrapper, UI/음악/팔레트/연결/송신/로그인 타이머 시작은 명시 외부 입력이다. 이 관찰은 해당 하위의 전체 복원이나 실제 미션 완주를 대신하지 않는다. 부모1/2/3 전체와 CD10.72는 검증 범위 밖이다.

첫 검사는 TSV의 헤더를 입력 행으로 잘못 읽어3CHECK가 실패했다. 헤더6칸과 전체34행을 먼저 확인한 뒤 실제33입력을 모두 비교하도록 검사 파서를 교정했다. 다음5CHECK는 제품 상태가 아니라 관찰기의 진행창 ABI 해석 오류였다. 실제 문자열 `Initializing.`을 미션 시작으로 분류하고, 한 인자 Progress의 인접 스택을 읽던 Countdown을 앞선 실제 formatter 네번째 인자에서 관찰하도록 교정했다. 최초 fixture와 보고서는 `extracted/local-mission-state-before-progress-abi*`에 보존했다. 제품 몸체와 전체 상태/전역/타이머/Core 비교는 변경하지 않았다.

새 C++3검사는33입력 전수 비교, phase10 확정 뒤 수신 callback이13→14를 실행하는 현재 큐 수명, 별칭/판본/필수 소비자 계약을 다룬다. **Release 경고0·오류0·집중3개 실패0·전체 CTest1/1·내부1,309개 실패0·187.98초**를 확인했다. 전체 경과188.01초다. `cpppj/local-mission-state.json`과 `extracted/goal-local-state-{build,parser-build,focused,ctest,tests}.log`에 근거를 보존한다. 이 단위에서 실제 Session/UberGump의 로그인 경로를 자동 활성화하지 않았다.

커밋은 AGENTS.md를 직전에 다시 읽고 `20261015-66 CP 단독 미션 로그인 상태 전환과 비동기 수신 순서 복원 검증`을 사용한다. 미등록 Start 바인딩과 보호8미션의 새 검사는 다음 단위의 편집이며 이번 빌드/대조에 합산하지 않는다.
