# 원본 Game 메뉴막대와 다섯 하위 부모

2026-10-23, 현재 PC와 마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-23**이다. 기존 `extracted/refined/originals/Netstorm.c`와 보호 10.78 PE를 읽었다. 새 디컴파일, 원본/클론 게임 실행 및 GUI 실행은 이 독립 조사에서 하지 않았다. 보호 파일은 변경하지 않았다.

## 실제 부모와 공통 메뉴

| 원본 10.78 | 새 공개 몸체 | 동작 |
|---|---|---|
| `004cc210` | `RawGameMenuBar::Open` | 가로 Game·조건부 Edit·View·Options·조건부 Players·About·조건부 Cheat와 폭/부착/생성순번 저장 |
| `004cbb80` | `RawGameMenus::Game` | Editor, 일반 미션, 데모, 멀티 전투, 메타맵의 각 Game 행 |
| `004cb9c0` | `RawGameMenus::Edit` | Save as..., Add Island, Set All Bridge의 Cracked/Normal/Hard |
| `004ca4d0` | `RawGameMenus::View` | 지형/건물/채팅/카메라/지식/섬 소유권/미션 목표/Player 목록 |
| `004ca850` | `RawGameMenus::Options` | 화면/해상도/음향/음량/스크롤/격자/데모/팁/정지/회전/진단 |
| `004cbf10` | `RawGameMenus::Players` | 현재 Player1..8의 전체 목록 및 기존 `RawPlayerMenu` 하위 |

부모는 기존 `MenuGumpWidgets`, `MenuGumpControls`, `MenuGump`의 같은 controlled 스택을 쓴다. Game 등의 하위는 작성 뒤 `TakeCurrent`로 자신의 토큰을 반환하며, 최상위 가로 부모도 마지막에 controlled 스택에서 꺼낸 뒤 실제 루트에 붙인다. 가운데 임시 팝업으로 배치를 바꾸지 않는다.

가로 메뉴는 `BeginHorizontal`의 원본 `0x140000`이고, `width = signed(SCREENW - worldLeft)`를 float32로 저장해 폭 setter에 넘긴다. 폭 setter 뒤 `worldLeft`를 다시 읽어 `(left, 0, root, mode1)`에 부착한다. 원본 `004cc4ab`은 **Gump+2c의 생성순번 serial**을 `005cad24`에 저장한다. 등록 배열의 칸 번호 `id(+1c)`와 다르다. `00464020`도 등록표1..999를 순회하며 **serial**을 비교하고 dead를 제외한다.

첫 구현의 바인더가 `id`를 읽던 실수를 감사에서 고쳤다. `GameMenuBarState.menuSerial`과 `GameMenuBarHooks.serial`로 이름을 명확히 했고 바인더는 현재 노드의 `serial`을 읽는다. 독립 원문 대조는 GUI 경계를 명시 장치로 제공하므로 이 바인더 오류를 검출하지 못했다. 실제 공통 Gump 검사에서는 id와 serial을 다르게 만든 상태로 열기→serial 조회→닫기를 확인해야 한다.

## SP 준비와 공유 추적 주소

`004cc210`의 첫 `00491680`은 게임 정지를 걸지 않는다. 이 몸체는 이미 복원한 **`ScrambledSpStore::Rotate`**다. 초기화가 필요하면 실제 `0040eba0` 초기화를 수행하고, SP 저장 위치 `00568af8`을3 증가시켜 signed 결과가9600을 초과하면 한 번9600을 뺀다. 현재32개 순열 배열 각각의9DWORD를3비트 순환한다. 금액 자체는 유지한다.

그 뒤 메뉴 부모는 원본 위치에서 **별개의 추적 offset `0054d4e0`**을3 증가시킨다. 기존 Session은 이미 `priestState.regen.patch.sequence`가 이를 소유하고 `sp.BindTrackingOffset`이 그 주소를 빌린다. 메뉴용 카운터를 복사하거나 프레임마다 맞추면 원본의 현재 정합성 검사와 다른 상태가 된다. `GameMenuBarState`는 외부 `int32_t&`를 필수로 받아 같은 주소를 빌린다.

`RawCampaignSession::PrepareGameMenus`는 같은 `sp.Rotate()`를 호출한다. `GameMenuTrackingOffset`은 같은 기존 사제 추적 주소를 반환한다. 초기 설치 전 호출은 명시 거부한다.

## ESC와 일반 입력 순서

원본 `004d62b0`의 현재 사건 경계는 다음과 같다.

1. Guide `00463e40`이 먼저 소비한다.
2. 공통 Gump `00466200`이 소비한 경우 현재 사건을 끝낸다.
3. Gump가 소비하지 않은 **좌/우 마우스 누름**은 `00466710`으로 현재 루트 팝업을 Kill한다.
4. release 사건을 거부한 뒤 일반 명령과 ESC를 처리한다.
5. 일반 정지 검사 `00460de0`는 ESC 처리 뒤에 있다. 이후 일반 키/QWASZX와 활성 커서 경로가 이어진다.

`00445330`은 lockDepth 자체가 아니라 **`005325c8 == 0`**, 즉 활성 건설 커서 여부를 반환한다. 따라서 ESC의 `004d9574` 분기에서 활성 커서면 `00446e70` 취소→`00466710` 팝업 Kill이다. 비활성이면 `005cad24`로 `00464020`을 조회한다. 있으면 `00466480` Kill→serial 저장0→팝업 Kill, 없으면 `004cc210`으로 연다. 활성 커서 취소와 메뉴 열기를 같은 사건에 동시에 넣지 않는다.

그보다 앞에는 `005434e4`와 현재 미션 demo 조회 `00463b50`에 따른 선택 객체/선택 목록 정리가 있다. 이 조건을 활성 건설 커서 조건이나 모든 미션 정지 조건으로 합치면 안 된다. 현재 Client의 선택 연결과 별도 대조가 필요하다.

`004cc210` 자체와 직전 `004d5490` 선택 목록 해제/`004d5bb0` 입력 모드 변경은 기존 팝업을 닫지 않는다. bar serial 조회가 실패하면 생성 부모만 호출한다. 기존 `RawMenuControls::OpenAttached`의 무조건 `Close()`를 이 부모 앞에 붙이면 현재 객체 팝업까지 먼저 죽일 수 있으므로 실제 연결은 원문 상태를 보존해야 한다.

가로 메뉴 노드에는 실제 help 마우스 필터 `flags&0x200`이 있다. 막대 또는 펼친 하위 안의 좌/우 누름은 공통 Gump가 먼저 소비한다. 밖의 좌/우 누름은 공통 Gump 미소비 뒤 팝업을 Kill하고 **같은 사건으로 월드 선택/이동/객체 메뉴를 계속 처리**한다. 모든 바깥 클릭을 메뉴가 삼키는 모달로 바꾸지 않는다. 중간 마우스는 이 좌/우 팝업 닫기 조건에 들어가지 않는다.

일반 문자는 가로 메뉴가 소비하지 않는다. 열림 중 QWASZX도 원래 일반 예약에 도달한다. 실제 slot을 취득하면 기존 커서/팝업 닫기 효과가 막대를 닫지만, 없는 slot이라는 이유만으로 별도 닫기를 추가하지 않는다. 메뉴 열기에 `Client.Pause(true)`를 넣지 않는다.

원본 edge-scroll은 상단 `y<2`에서만 `005cad24` serial 조회가 성공하면 억제한다. 좌/우/하단에는 그 메뉴 조건이 없다. dead가 되어 조회에서 빠진 stale serial을 별도 열림 bool처럼 사용하면 안 된다.

## 각 하위의 현재 조회

Game은 Editor에서 Test Battle/Main Menu, 미션에서 선택적 Return to Editing과 Review/Restart/Leave, 데모에서 Restart Demo/Exit Demo를 작성한다. Editor·미션·demo는 서로 다른 현재 전역/미션 필드다. 멀티 leave의 실제 로그인/사제/신전 보유/SP·현재 신전 비용/사제 GroundImmobile 및 두 종료 gate를 원본 단락 순서로 읽는다. `00448fd0`과 `004490d0(0)`은 멀티 전투 종료/무승부 관련 gate이며 의식 함수로 취급하면 안 된다. 해당 큰 gate가 아직 실제 연결되지 않은 경우 명시 경계로 남긴다.

View의 테마 설정 키는 원문 철자인 **`theammode`**다. 소유권/음향/스크롤 등의 반전 문자열을 정하는 첫 설정 조회와, 번역 뒤 체크 값을 읽는 두 번째 조회는 구별한다. QWASZX와 독립적인 미니맵 토글 행을 이 View 부모에 임의 추가하지 않는다.

Options는 DirectDraw 장치 가용성/음향 장치 활성 여부, 실제 화면 크기, 현재 음량1..5, 정지 깊이 및 fort/metamap 분기 조건을 유지한다. 이 부모 복원은 DirectDraw 모드 전환의 실제 소비자 구현을 완료했다는 뜻이 아니다.

Players는 metamap이면0을 반환한다. 현재 subscriber가 있는 Player1..8만 방문하여 실제 사제 상태를 Refresh한 뒤 `flags&0x21`을 저장한다. 미션 후처리 모드가 아니면 현재 랭크 아이콘·원문 이름·레벨·서버·관전 표시를 작성한다. 각 하위는 현재 subscriber로 기존 전체 `RawPlayerMenu::Open`을 호출하며 enabled는 관전 BYTE의 반대, checked는 앞에서 저장한 flags다. 레벨 helper의 반환은 원본 `movzx AL`에 따라 하위 BYTE를 사용한다.

fancy Player 목록은 현재 메뉴 기본 폭/높이와 전체 화면의 signed 정수 반을 읽고 기존 CRT 절삭 규칙으로 가운데 좌표를 구해 실제 `AttachCurrent`한다. 별도 표시 목록으로 현재 Player 바이트를 대체하지 않는다.

`RawCampaignSession::BindGameMenus`는 Session 소유 모드·로컬 owner·PlayerKnown·같은 Player180BYTE·숨김 BYTE·로그인 수·같은 실제 사제/포획/SP·현재 신전 비용·사제 Refresh를 현재 조회로 연결한다. `0059507c`는 현재 `missionStart.denyAscend`, `005cab30`은 현재 `energyState.userCheat`를 읽는다. UI의 번역/설정/실제 미션의 mode64/demo·화면/음향/명령 전역과 다른 큰 하위는 caller의 실제 연결을 유지한다. `RawCampaignObjectMenus::OpenPlayer`는 상단 목록도 기존 같은 Player 메뉴 체인에 연결할 입구다.

## 독립 관측과 검증 범위

| 도구/fixture | 실제 관측 | 실행한 원문 |
|---|---:|---|
| `decomp_game_menu_bar.py` / `game-menu-bar-x86.tsv` |255입력×두 정밀도=510행|`004cc210` 전체 부모|
| `decomp_game_menus.py` / `game-menus-x86.tsv` |459입력×두 정밀도=918행|Game/Edit/View/Options 네 부모 전체|
| `decomp_player_list_menu.py` / `player-list-menu-x86.tsv` |265입력×두 정밀도=530행|Players 전체와 실제 번호 getter/CRT 좌표 절삭|

합계 **1,958행**, x87 정밀도 차이0이다. 원문 조건/부모/문자열 연결/조회 순서/폭·부착·검산·현재 생성순번은 CPU 명령으로 관측한다. GUI 생성·작성/번역·Config/큰 미션·사제·SP getter 및 각 Player 하위는 명시 장치다. Cheat의 직접 new/Construct와 내부 번역은 전체 하위 경계로 구분했다. 실제 Gump 그림·입력·명령 소비자·게임/GUI를 실행한 증거로 확대하지 않는다.

부모 전용 격리 빌드6은 종료0·경고/오류0, 집중 검사4개/독립1,958행·실패0이다. 로그는 `extracted/cpp-game-menus-audit-20261023/build6.log`와 `focused3.log`다. 이 격리 실행은 기존 Release 라이브러리의 읽기 복사본에 새 부모만 링크했으며 기존 빌드 산출물을 덮지 않았다. 초기 컴파일의 `auto` 선언 형식 충돌과 감사에서 찾은 View/Options의 선조회 순서·serial 바인더를 고쳤다. 기존 원문 fixture의 기대값을 제품에 맞춰 바꾸지 않았다.

Session 연결과 두 실제 Session 검사는 뒤에 추가했으므로 위 부모 전용 빌드6 결과에 포함하지 않는다. Session 포함 격리 빌드9의 집중6검사/실제 Session2검사는 실패0이다. 직접 cl 명령줄에는 옵션 전체 적용 D9026 한 건이 있으므로 제품 경고0 판정을 이 직접 명령에 확대하지 않는다. 추가 Tell 파라미터1,070행/실제 Config 수명까지 합친 독립9검사도 실패0이다. 최종 전체 빌드/집중·회귀/실제 공통 Gump 및 GUI 결과는 통합 검증 기록에서 확인해야 한다. 모든 캠페인 완주, Editor/커스텀 맵·시나리오, 다른 메뉴와 전체 동일성은 계속 활성 목표다.
