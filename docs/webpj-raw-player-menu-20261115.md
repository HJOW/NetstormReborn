# Player 메뉴 이식 (WB04-E544)

6단계 "게임 메뉴"의 마지막 부품 중 하나인 `client/RawPlayerMenu`(원문 10.78 Player 부모 004c9700)를 `webpj/www/core/raw-player-menu.js`로 옮겼다.
기준선 `225553df`와 현재 cpppj 소스가 같음을 확인했다. 객체 메뉴(`RawObjectMenu`)의 `Player` 하위 메뉴와 게임 메뉴의 Players 목록이 이 부모를 쓴다.

## 옮긴 범위

- `RawPlayerMenu.open(subscriber)`: subscriber → owner(0이면 0 반환) → Fancy 제목 `Player %s`(미션이면 Player 기록의 이름, 아니면 Identity 이름) → 전투이고 주소가 있으면 주소 해석 → metamap이면 메타 주소 해석 →
  편집이 아니면: (전투) 상태 줄(미션이 아닐 때 Online/DISCONNECTED)·사제 상태(Dead/Captured/Alive)·동맹 줄(현재 대상→후보 방향, 반대 방향이 없으면 `(One-Way)`, 없으면 `Allied with nobody.`) →
  (비미션이거나 내 subscriber) 주소 조회·metamap이면 Finder로 metadata 번호를 찾아 Version/Reliability/Games Played·`View Player Info` → 무시/귓속말 체크 항목 → (전투) 구분선·`Find Priest!`·`Send Money` 하위 메뉴(300/700/1000/X/All)·
  동맹 요청/해제 항목 → 메뉴를 스택에서 꺼내 반환한다.
- `sendMoney`: 두 정적 asksend 버퍼(`sendMission`/`sendLobby`, 각 104BYTE)에 `asksend(%d)`를 NUL까지만 쓰고 뒤 바이트를 지우지 않는다. 행 활성화는 숨김·Player 플래그(0x21)·허용 훅으로, 금액은 색(`~r`)만 바꾼다(SP를 편집 검사보다 먼저 읽음).
  미션의 마지막(All) 행은 Tell 명령과 정적 토큰 `missionTextToken`을 쓰고 `textParameter(token)`이 최신 버퍼 문자열을 돌려준다(임시 문자열 고리로 바꾸지 않음).
- `formatPlayerMenuText`: 고정 `%s/%d/%%`만, 256바이트 한도, 인자 수/형식 불일치 거부. `bindPlayerMenuControls`/`bindObjectPlayerMenu`: 기존 controlled 메뉴·객체 메뉴 연결.
- 모드·소유자·SP·동맹·이름은 사용 시점마다 현재 값을 다시 읽고 번역은 뒤 인자 조회보다 먼저 실행한다. 도달한 미연결 효과는 성공으로 대체하지 않는다(`/미연결/`).

## 시험

`webpj/tests/raw-player-menu.test.mjs` 4건:

1. 원본 관측 `player-menu-x86.tsv` **292입력**: 대상·모드·flags·사제·동맹·숨김·허용·SP·무시/귓속말·게임 수·주소·변경 조합에서 반환값·정적 208바이트·모든 조회/GUI 효과 순서. **첫 실행에서 292입력 전부 일치**했다.
2. 계약: 같은 정적 토큰이 갱신된 asksend 문자열을 읽음, 송금 SP 조회 누락 거부, 저장소 크기 거부.
3. 계약(변형 확인 뒤 보강): 후보→자기 동맹 제외, 귓속말 BYTE(0x100→0, 0x1ff→255), 마지막 All 금액 1, 사제 번호 범위(8 유효·0/9 진단), 출력 NUL/256바이트, 비미션 주소 조회 한 번(미션 남의 subscriber는 0번).
4. 서식·연결: 고정 서식과 오류, 컨트롤/객체 메뉴 위임.

**변형 확인:** 42가지(이름/제목/주소 해석·편집·상태 줄·사제 상태·동맹 방향/단방향/없음·채팅 관문·버전 오프셋·게임 수 상한·무시/귓속말 명령·구분선·사제 찾기·송금 관문/표/활성/include·동맹 요청/해제·SP 비교·번호 진단·정적 버퍼 선택·NUL/256·토큰 해석 등) 중
39가지가 시험에서 잡혔고 3개는 동등 변형이다: ① `Dead`/`Captured` 판정은 서로 배타적이고 owner는 1..8로 제한되어 판정 순서를 바꿔도 결과가 같고, ② 동맹 해제 비트 `1<<(target&31)`은 owner ≤ 8에서 `&15`와 같으며,
③ 송금의 SP 조회는 이 부모가 편집이 아닌 분기에서만 부르므로 편집 검사와의 순서가 관측되지 않는다.

`webpj/tests/fixtures/manifest.json`에 `player-menu`(292입력, 원본 10.78) 항목을 추가했다.

## 남은 경계

- 실제 번역·Identity 전역·Player 목록(`player-list-menu` 530행)·`RawGameMenus`의 Players 목록과 Gump 연결은 아직 없다(PORT_MAP `port`).
