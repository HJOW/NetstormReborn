# ESC 메뉴와 같은 SP 검산 저장소 — WB04-E1327

2026-11-24, HJOW-Athlon. RawCampaignSession 공개 경계 감사에서 C++ `PrepareGameMenus`·`GameMenuTrackingOffset`의 앱 연결 누락을 찾았다. 웹 `ClientGameMenuBar`는 지역 `{value:0}`을 검산 상자로 쓰고 prepare 장치를 제공받지 않았다. 금액은 보였지만 원본의 실제 SP 비트 회전/검산 상태를 만들지 않았다.

C++ `UberGump::OpenGameMenu`는 같은 Session의 추적 참조로 `GameMenuBarState`를 만들고 prepare에서 `PrepareGameMenus()`를 호출한다. 이것은 `sp.Rotate()`를 실행하며, `RawGameMenuBar`가 같은 추적 상자에3을 더한다. 웹 앱도 Session SP 저장소의 `trackingOwner`를 빌리고 prepare에서 같은 저장소를 회전하도록 연결했다. 독립 UI 호출자가 SP 장치를 주지 않는 기존 계약은 지역 검산 상자로 보존한다. 실제 앱은 지역 사본을 사용하지 않는다.

`225553df` 오라클 EXE(SHA-256 `5b52d96d2e53f4674a40991b78ff64c3c7e2ad8275a0a99b22d18d532b77eb46`)로 Tutorial3의240프레임 뒤260 ESC 열기·300 닫기·340 다시 열기를 수집했다. AGENTS 확인 예외 호스트에서 창 모드·음소거·고정25ms로 실행했고 원본/CD 파일은 실행·수정하지 않았다. 새 폴더 `tests/fixtures/cpppj/candidate-225553df/tutorial3-menu-sp`만 생성했다.

`e2e/menu-sp.spec.js`는 C++와280·320·360프레임의 화면0픽셀,240·280·320·360프레임의 Session 일치를 확인했다. SP offset은103→106→106→109이며 닫기에는 증가하지 않는다. 일반 L3의 넓은 sp 행 허용에 더해, 이 검사는 미초기화 owner0 한 칸만 빼고 초기화·offset·플레이어1..8의 금액 비트를 정확히 대조한다. 같은 시점의 실제 Client 추적 상자도 offset과 같고 Session/Client의 저장소·추적 소유자는 같은 객체다. 각 플레이어 금액 비트는 메뉴 여닫기 전후 그대로다.

L3 공통 도구는 C++의 esc 단계와 체크포인트의 추가 읽기 전용 inspect, 대조 결과 반환을 지원한다. 기존 입력 보정은 보존했다. 새 대조 Chromium1/1(50.9초), 원본 메뉴막대510행·SP 초기화48행·SP 보상 관련 Node32/32 통과. 번들3,429,354바이트로 한도 통과. 기존 실제 메뉴 회귀4/4도 통과했다(40.4초): ESC/우측 버튼, Game/View/Options/About, Leave/다른 미션/Restart, F2/F8/F9이다. 새9자료를 manifest에 등록해975항목이다. 최초 추적 검사11건 중1건은 PNG/TSV에 확장자 없는 같은 ID를 부여한 중복을 검출했고, 확장자를 포함한 고유ID로 교정했다. 나머지10건(번들 신선도·대응표7·해시/소비2)은 통과했고 최종 manifest3/3도 통과했다. 이번은 집중 검사로 마감하며 Node26 E1325 전체 뒤2/4, Node22 E1324 전체 뒤3/8이다.

로그는 `extracted/webpj-e1327-{oracle,browser,focused,build,menu-regression,tracking,manifest}.log`다. RawCampaignSession은 다른 공개 경계/과거 잔여 감사가 이어지므로 partial을 유지한다. 이 결과는 Tutorial3의 전투/포획/자연 완주 대조가 아니다.
