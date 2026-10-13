# 실제 Client 돌 버튼의 raw 입력/표시 연결

마지막 정밀 디컴파일 PC는 **VM-W11-CODEX**다. 2026-10-13에 같은 PC의 10.78/10.72 읽기 전용 출력을 확인했다. Windows 10/11 전체화면 DirectDraw 변경은 포함하지 않는다.

`UberGump`의 메인 메뉴·브리핑 하단 돌 버튼은 이제 `RawButtonControls`가 소유하는 실제 `GumpCreate`, `GumpTree`, `GumpLifetime`, `GumpEvents`, `GumpInput`, `GumpFrame`, `ButtonGump`, `StoneButtonGump`, `GumpBackgroundCreate`를 사용한다. 기본/돌 버튼/배경의 기본 노드 조회는 같은 주소를 반환하며 현재 부모 serial의 click 통지만 화면 명령 번호로 변환한다. 별도 사각형 캡처를 실행 조건으로 사용하지 않는다.

페이지 교체는 기존 자식의 Kill→Cleanup→새 돌 버튼/배경 생성·부착이다. 초기 루트를 반복 생성하거나 serial/count를 0으로 덮지 않는다. 판정·출력의 폭/높이는 같은 raw 노드를 읽고 배경은 실제 width/height-2와 Attach(1,1)을 읽는다. 기존 UI의 표시 높이와 입력 여유 한 행을 분리하던 경계를 제거하여 전체 입력 높이를 raw 생성자에 공급한다. 메뉴 배치·고정 버튼 폭/위치는 아직 기존 `UberGump` 공급 경계이며 원본 전체 메뉴 생성/동적 배치 복원이 아니다.

현재 Win32 폴링은 실제 버튼 Broadcast에 공급한다. 벽 시계는 `Client::WallSeconds()`가 매번 `timeGetTime`→기존 `GameClock::WallSeconds`를 읽으며 한 프레임의 시각으로 여러 원본 조회를 합치지 않는다. 게임 시간 정지 중에도 버튼 예약/idle은 벽 시계를 사용한다. 메인/브리핑 버튼 누름은 원본 버튼 몸체가 `button.wav`를 요청한다.

월드 위에서 Game 버튼을 놓았을 때에도 raw focus가 있으면 먼저 같은 캡처로 release를 전달한다. 화면 전환/비활성 취소는 raw Button release를 화면 밖 좌표에 전달하여 focus/idle/pressed를 함께 해제한다. 이미 원본 입력 큐에 release가 없는 OS 사건의 전체 원본 분배/비활성 정책 복원은 후속이다.

실제 생성자와 팔레트 초기화가 `StoneButtonGump`의 기존 문맥 색 반전을 확인했다. 패치 `004254b0`의 `005a4058`은 RGB(0,0,0), `005bec2c`는 RGB(255,255,255)다. CD `00441310`도 `0055baa8` 검정/`0055bad0` 흰색이다. 정상/눌림은 흰색 글자+검정 그림자, 비활성은 검정 글자로 교정했다. 원본 생성자는 잉크/그림자를 노드 문맥에 복사하고 Draw는 현재 외곽선 색 전역을 읽으므로 `SetPalette`는 현재 전역만 갱신하고 기존 문맥을 덮지 않는다.

`cpp_gumpvisual_smoke.py`와 `cpp_gumpbackground_smoke.py`는 실제 보고에 `RawButtons`가 있으면 입력/표시가 동일한 raw 경계와 생성된 자식의 height-2 계약을 원본 실행기에 공급한다. 기대 픽셀은 C++ 그림에서 추출하지 않고 기존 실제 PE 생성/Draw·배경/타일 호출과 독립 글꼴/SHP 해독으로 만든다. 이 원본 함수 호출은 격리된 x86 실행이며 원본 게임 GUI를 실행하지 않는다.

검증 로그와 최종 수치는 상위 `LEFT_JOBS.md` 최신 항목에서 확인한다. 새 호스트 검사는 캡처/복귀/비활성/부모 serial, 실제 배경 눌림/팔레트 전역, 1,000회 페이지 교체의 등록표/노드/serial 수명과 기존 독립 생성/Draw fixture 6개 직접 대조를 포함한다. 기존 fixture 재사용은 새 원본 관찰 수에 더하지 않는다.

최종 Release 경고/오류0·전체 CTest **883개/실패0**(204.98초), 새 raw 버튼4개·기존 버튼8개·돌 버튼 텍스트 통합6개를 통과했다. 첫 전체 검사의 기존 돌 버튼 픽셀 기대값16개 실패는 원본 색 교정에 맞춰 기대8개를 갱신한 뒤 해소했다. 기존 실패 로그와 최종 로그를 모두 보존한다. 클론 GUI의 메뉴/옵션/복귀54상태·월드 release2회/재진입10상태, 원본 함수·독립 글꼴/SHP의 선/글자32,382픽셀·배경 포함150,626픽셀 대조를 통과했다. 보호 원본1394파일의 바이트/존재 상태 보존을 확인했다. `tools/analyze_campaign_runtime.py --verify`로 전체 실행 기반 단위의 SHA를 감사한다.

메뉴 행은 기존 어댑터가 처리하며 전체 raw Menu/StyleText/GumpHelp/동적 메뉴, 상위 State의 Uber 루트 교체, UI 객체 Form과 비표면 월드 연결은 남았다. 승패 모듈은 실제 사제 ownerState와 동맹 기반 조건을 복원했지만 아직 Client 미션 실행기에 붙지 않았다. 사제 건설·수확·전투·운반/의식·캠페인 1-1 완주/원본 동일성은 이 단위의 완료 기준으로 주장하지 않는다.
