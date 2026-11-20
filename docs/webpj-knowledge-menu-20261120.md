# F6 지식 창 (WB04-E1088)

미션 중 F6 키와 게임 메뉴 View ▸ "View Netstorm Knowledge" 항목이 원본 지식 창(Storm 원소 카드 격자)을 연다. 원본 `UberGump`의 ShowTechnology(`0x020000a0`)·ShowTecSideMenu(`0x02000132`)·ShowTecMenu2(`0x02000133`) 분배와 `RawMenuControls::OpenKnowledge`를 옮긴 것이다. 이전에는 메뉴 항목이 비활성이었고 F6은 큐에 들어가지 않았다.

## 구현

- **게임 조회.** `RawCampaignSession.showKnowledge(state, hooks, kind)`가 cpppj `RawCampaignSession::ShowKnowledge`처럼 차단(메타맵)·커서 취소·현재 기술 부모·기술 보유(`carrier.findContained(parent, 4, type, 0)`)·TYPE 요구 에너지(`bindAiTypeRequiredEnergy`)·생산 중 여부·건설 제외 목록을 현재 Session에서 채워 이식해 둔 `showKnowledgeMenu`/`showWorldKnowledgeMenu`/`showKnowledgeSideMenu`를 호출한다. `knowledgeAvailable()`은 Player가 알려져 있고 메타맵이 아닐 때만 true다.
- **작성 장치.** `ClientMenuGump.knowledgeWriter(storeText)`가 원본 hooks를 만든다. 첫 카드를 만들기 직전에만 기존 팝업을 닫고 수명을 정리한 뒤 세로 controlled 메뉴를 시작하고(`begin`), 부착(`attachCurrent(x, y, 루트)`)·Push/Pop·폭/높이·위치·항목·제목·구분선을 같은 작성기에 연결한다. 항목의 문자열 인자 `'themeHelp'`는 정적 문자열 토큰으로 바꾸어(원본의 고정 `themeHelp` 주소 대신) 기존 0x17 도움말 처리기가 정적 주제로 열게 한다.
- **연결 객체.** `ClientKnowledgeMenu`가 Session·작성기·화면 크기·언어(영어 0, 한국어 7)·번역·정적 문자열·팝업 표식 장치를 묶어 `open(kind)`/`isOpen()`/`close()`/`tokens()`를 제공한다. `createClientProductionMenu`가 만들고 `ClientProductionPanel.knowledge()`로 앱에 노출한다.
- **앱.** F6 키를 큐에 넣고(`_onKeyDown` 가상 키 목록, `MISSION_F6 = 0x750000`) 입력 프레임 허용 목록과 `user.event`(선택 키 분배기가 정지·Shift/Control 관문을 거쳐 `0x020000a0`을 예약)로 보낸다. 명령 세 개를 `WIRED_MENU_COMMANDS`에 추가해 View 항목이 활성이 되고 `_openCampaignKnowledge(kind)`가 연다. 지식 창이 열려 있을 때 ESC는 팝업만 닫고 메뉴막대를 열지 않는다(원본 `KnowledgeOpen` 분기).

## 원본과 같은 동작

- 카드는 `(theme + 1) & 3` 행으로 네 줄(SUN·WIND·RAIN·THUN.)에 쌓이고 정렬된 TYPE마다 한 장이다. 보유하지 않은 기술은 `&` 프레임(어둡게), 보유·마나 없는 기술은 `#` 프레임이다. World 카드는 이 판본에 World 원소 TYPE이 없어 머리 네 개(EARTH·FIRE·WATER.·MOON)만 나온다.
- 카드·머리 메뉴는 압축(COMPACT) 메뉴라 어디를 눌러도 첫 항목만 가리킨다. 카드는 첫 항목이 TYPE 도움말 명령이라 눌리고, 머리 카드는 첫 항목이 비활성 제목이라 눌러도 아무 일이 없다(`themeHelp` 항목은 도달할 수 없는 원본 구성이다).
- 카드를 누르면 `0x02000017`(TYPE 도움말)로 지식 창이 닫히고 도움말 창이 열린다.

## 검증

- 새 `tests/client-knowledge-menu.test.mjs` 6건(열기 관문·hooks 전달·World/선택 종류·열림 판정·닫기·장치 누락). 변형 7종(관문 제거·삭제 대기/표시/부모 검사 제거·팝업 닫기 제거·언어·빈 토큰 성공)이 모두 시험 실패로 잡힌다.
- 새 `e2e/knowledge-menu.spec.js` 4건(Chromium): F6가 Storm 카드 격자를 열고 ESC가 팝업만 닫음, View ▸ 지식 보기 항목, 선택 메뉴(팝업 2개)·World 카드·Storm 카드 클릭 → 도움말 창, 머리 카드의 `themeHelp` 토큰과 비반응이 통과했다. 화면은 원본 카드 격자 구성(네 줄·머리·아이콘·두 줄 이름)과 같게 그려진다.

## 남은 경계

원본 스크린샷에 지식 창이 없어 픽셀 비교는 하지 못했다. Pause(Shift-F9)·F7 배너·F3 채팅 전환·Players 하위 항목의 실제 실행은 남아 있다.
