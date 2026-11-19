# 앱의 공통 난수·SP 수명 (WB04-E878)

cpppj 기준선 `225553df`의 공통 시작은 옵션·화면 초기화 전에 Client의 장기 SP를 초기화한다. 웹 앱도 각 `init`에서 같은 `ClientTimeSource`에 연결된 `ClientRandomGlobals`·`ClientSpGlobals`를 만들고 SP를 초기화한 뒤 Client 시계를 만든다. 실패하면 DOM 루트에 연결하기 전 종료한다. `destroy`는 현재 앱의 네 공통 소유자 참조를 반납하고 재시작은 새 소유자를 만든다.

`_campaignSharedServices()`는 현재 Client 난수·SP·시간 보정 전역과 시계 공급자를 같은 수명으로 빌려준다. 실제 TheWarBegins Session에 이 서비스들을 전달해 SP 값과 offset이 재초기화되지 않고 같은 난수·SP를 사용하며 Session 반납 뒤에도 보존됨을 확인했다. 이는 독립 Session 결합 검사이고 실제 앱의 미션 생성·화면·프레임 수명은 아직 연결하지 않았다. 메뉴 구름·바람 등 원본과 같은 난수 소비 순서도 후속이다.

Node v26 집중·연관 검사 30건과 Chromium의 시작·반납·재시작 및 최초 실행 검사 2건이 통과했다. 정해진 주기에 따라 이번에는 전체 회귀를 실행하지 않았다. 게임 창은 실행하지 않았다.
