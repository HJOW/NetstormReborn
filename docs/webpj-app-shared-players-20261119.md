# 앱의 공통 Player·Template 수명 (WB04-E879)

cpppj 기준선 `225553df`의 Client는 Subscriber·Gameplay·CoreData·Player·Template을 Session보다 오래 소유하고, 공통 시작에서 같은 Player 기록과 방향별 동맹을 초기화한다. 웹 앱의 공통 시작도 기존 `initializeClientPlayerRecords`를 실제 장기 소유자에 호출하고 Player base를 같은 CoreData에 결합한다. 기존 `_campaignSharedServices()`는 같은 다섯 소유자를 Session에 빌려주며 `destroy`는 앱 참조를 반납한다.

실제 Session 두 개를 차례로 생성·반납하여 같은 Player 소유자의 재할당 뒤 CoreData가 현재 물리 뷰를 가리키고 Template 헤더와 Subscriber 값이 유지되는지 확인했다. 이 검사는 독립 조립 경로이며 앱 메뉴의 실제 미션 진입이나 전체 State 종료 수명의 완성을 뜻하지 않는다.

Node v26 집중·연관 검사 62건과 Chromium의 시작·재시작·최초 실행 검사 2건이 통과했다. 주기에 따라 이번 단위에서는 전체 회귀를 실행하지 않았다. 게임 창은 실행하지 않았다.
