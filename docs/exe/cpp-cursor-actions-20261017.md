# cpppj 현재 커서의 잠금·회전과 실제 입력 — 2026-10-17

작업 PC는 **HJOW-Athlon**, 마지막 정밀 디컴파일 PC/날짜는 **HJOW-Athlon / 2026-10-16**이다. 기준 커밋은 `c7e9e8b`다. 단위136은 새 디컴파일/게임/GUI 실행 없이 기존 두 판본 정밀 C와 보호10.78 PE의 격리 CPU 실행으로 진행했다. 전체 플레이 목표는 계속 진행 중이다.

## 원본 몸체와 현재 상태

`RawCursorActions`는 실제10.78 `446a10` 잠금, `446a40` 해제, `446a70` 회전을 복원한다. 같은 CursorDrop/수명/현재188TYPE/특별 Wind 타입/사정거리 프로세스 타입을 참조한다. 잠금과 해제는 DWORD 증가·감소의 감김까지 유지한다. 처음 잠글 때 Activate0하고 skipCursor가0일 때만 커서1을 선택한다. 해제 후 깊이0에서만 실제 미리보기를 다시 만들고 현재 legalGump에 따라 커서2/3을 고른다.

회전은 방향에8과 step을 더하고 `0x80000007`을 적용한 뒤 원본 signed 꼬리를 실행한다. unsigned modulo8로 바꾸지 않는다. 음향 뒤 현재 깊이, 생성/선택 뒤 현재 TYPE 포인터, 사정거리 삭제 뒤 현재 첫SID를 다시 읽는다. group1 또는 현재 특별 Wind 타입이면 같은 `RawContainedLookup(parent,processType,0,true)`로 이전 사정거리를 삭제하고 실제 Range를 다시 생성한다. 반환0도 저장한다. 슬롯/PID/주소의 재사용을 금지하는 호스트 규칙을 추가하지 않는다.

보호 PE `5325c8`의 초기 잠금은1이다. Session의 기존 기본0을 수정해 빈 커서를 활성으로 처리하지 않게 했다.

## 실제 입력 연결

원본 거대 UserInput `4d62b0`의 활성 커서 분기를 정밀 C와 **정렬된 PE 명령 `4d9bbb..4d9c2e`**로 확인했다. 좌클릭은 Drop, 좌화살표는 -2, 우화살표는 +2, 우클릭은 `rotmode`가0일 때+2/그외-2다. modifier를 제거한 코드로 분기하며 release는 앞의 상위 필터에서 건너뛴다. ESC는 Cancel 후 팝업을 정리한다. 거대 UserInput 전체를 새로 CPU 실행했다고 주장하지 않는다.

`RawCampaignCursor::HandleInput`과 실제 `UserInput::Event`를 이 분기에 연결했다. 활성 ESC가 임시 Game 메뉴를 먼저 열지 않도록 UberGump도 수정했다. 현재 `rotmode`, `cursorSnapToGrid`, `cursorShowCannotPlace`를 해당 소비 시점에 읽는다. 누락 기본값은 각각0이다. 활성 커서의 좌우키는 카메라 이동으로 중복 소비하지 않는다.

UberGump는 사건 큐 처리 전에 현재 마우스를 한 번 폴링해 실제 정수 커서 위치 갱신에 공급하고 같은 값을 기존 UI Poll에도 사용한다. Tick은 카메라 변경 뒤 다시 위치를 갱신한다. 비활성/화면 밖/중첩 잠금 상태에서는 이 표시 갱신을 실행하지 않는다. 그 외 선택·이동·카메라 입력은 여전히 부분 복원이며 이번 변경을 전체 입력 동일성의 증거로 사용하지 않는다.

10.72 대조는 Lock `43c740`, Unlock `43c780`, Rotate `43d510`이다. CD 회전은 두 helper를 호출하고 같은 사정거리 재생성 분기를 갖는다. CD의 helper 구성/별도 전역 주소를 기록했으며 새 CPU 실행과 연결은10.78 전용이다.

## 검증

`tools/decomp_cursor_actions.py`는 **289고유 입력/578정상반환/53·64비트 정밀도 차이0/명령17,250**을 새로 관측했다. 원본 세 부모 전체만 실행하고 Activate/Create/커서/음향/ContainedLookup/Range는 명시 경계로 둔다. 각 경계의 변경 뒤 현재값 재조회를 확인하고 보존GP/스택/canary/x87 제어·균형을 검사한다. 기대 출력은 원본 반환의 전역과 효과 순서에서만 읽는다. native3가 최종이며 기존 fixture를 변경하지 않는다.

실제 Session 검사는 같은 미리보기의 잠금/중첩/해제/회전/ESC와 실제 Range의 Form·부모·Kernel 수명·등록 순환값을 확인한다. 준비75/GUI 반환8 Form/완성76 공급원은 단위134의 명시 입력이며 생산 완료나 메뉴 제공 행의 증거가 아니다. 별도 headless 검사는 실제 Client/Configuration/UserInput을 만들고 Run 없이 현재 설정/위치/우클릭/좌우키/release/ESC를 통과한다. 실제 GUI 장치 effects는 검사 경계다.

기존 배치 통합 검사도 `UserInput.Event` 좌클릭에서 시작한다. 같은 Local예측/반환/상위비용/25바이트 송신/RawPacketDispatch/서버 전체 Request/교체/실제 Builder완료를 확인한다. 실제 로그인된 로컬 전송 전체를 대신하지 않는다.

최종 Release 빌드3 종료0/경고0/오류0, 집중14개(원본 재생1/캠페인13)/실패0, 전체 **1,551개/실패0/CTest종료0/261.05초(전체261.08초)**다. 새 검사는4개다. 원본 근거42종과 새 도구 최종SHA 감사가 통과했다. 스테이징52파일/제품12개/기존33보고서의현재104핀 갱신/과거95핀 보존/보호3파일 SHA/UTF-8·LF·기존CMake접두·기존fixture불변을 감사했다. 보고서는 `cpppj/cursor-actions.json`, `cpppj/campaign-cursor-actions.json`, 로그 접두부는 `extracted/goal-cursor-actions-20261017-`다.

## 남은 실제 플레이 연결

실제 Client `SetEffects`의 전체 장치/선택/부모 Gump, 원본 객체·사제·생산 메뉴, 로그인된 로컬 전송 두 끝/등록 수명/큐잉, 생산 완료/경제/AI/전체 Kernel, Early Missions1~6·캠페인1-1~1-2·데모3의 자연 완주·승패·저장·다음 미션 및 전체 동일성은 남아 있다. 임의 성공 callback으로 대신하지 않는다. 전체화면 DirectDraw 수정은 요청대로 제외한다.
