# Client의 메뉴·미션 정지 소유권

현재/마지막 정밀 디컴파일 PC는 **HJOW-Athlon, 2026-10-15**다. 기존 `GameClock`의 원본 중첩 정지와 [공통 미션 프레임 게이트](cpp-mission-frame-gate-20261015.md)의 `scriptPaused` 계약을 실제 Client API에 연결할 준비를 한다. 새 디컴파일·native 고유입력·원본 게임/GUI 실행은 없다.

기존 `Client::Pause(false)`는 시계가 정지됐다는 이유만으로 깊이 하나를 반납했다. 메뉴가 자기 깊이를 갖고 있지 않아도 미션의 정지를 풀 수 있었고, 이미 미션이 정지한 뒤 메뉴가 열리면 메뉴가 별도 깊이를 얻지 않았다.

`uiPaused_`는 메뉴/브리핑 UI가 보유한 한 깊이만 표시한다. `Pause(bool)`는 이 표식이 바뀔 때 같은 실제 시계의 깊이를 보유/반납하며 반복 요청은 무동작이다. 미션 게이트는 기존 `scriptPaused`로 자신의 소유권을 관리하고 `PushGamePause`/`PopGamePause`를 통해 원본00460df0/00460e30에 해당하는 중첩 동작을 호출할 수 있다. 시계나 프레임 시간을 따로 복제하지 않는다.

새 검사2개는 실제 Client를 만들고 Run을 호출하지 않은 상태에서 메뉴→미션·미션→메뉴의 해제 순서, 반복 요청, UI 소유 없이 들어온 false 요청, 중첩 미션 깊이를 확인한다. 생성자는 보호 archive를 읽기만 하며 창/장치/설정 쓰기를 수행하지 않는다. 게임 시계의 수치 계산은 변경하지 않는다.

Release 빌드 종료0·경고0·오류0, 새 실제 Client 검사2개 및 기존 프레임 제한 검사3개가 통과했다. 최초 집중 실행의 건설 Session 검사1개는 별도 파생 삭제 미연결로 실패했으며 원래 로그를 보존했다. 이후 해당 실제 VT 삭제 연결을 교정하고 같은 소스로 **집중49개 실패0·전체 CTest1/1·내부1,256개 실패0·193.66초**를 확인했다. 최초 로그는 `extracted/goal-construction-pause-{build,focused}.log`, 최종 로그는 `goal-construction-final-{build,focused}.log`와 `goal-units62-66-ctest.log`다.

실제 UberGump의 공통 미션 게이트/Guide·로그인 순서 연결과 10프레임 안내·7프레임 재개 GUI 대조는 다음 단위다. Early Missions 및 캠페인 자연 완주를 달성했다는 뜻은 아니다.
