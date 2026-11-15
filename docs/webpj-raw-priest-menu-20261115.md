# 사제 건설 메뉴 이식 (WB04-E540)

H(사제·운반·경로)의 `client/RawPriestMenu`(원문 10.78 사제 메뉴 00494870·건설 행 00461cf0·건설 선택 00461bd0·테마 조회 00493ac0·프레임 상태 004ad710)를
`webpj/www/core/raw-priest-menu.js`로 옮겼다. 기준선 `225553df`와 현재 cpppj 소스가 같음을 확인했다.
`RawObjectMenu`(공통 객체 메뉴)·`RawPlayerMenu`가 같은 `PriestMenuArgument`(문자열 또는 int32 서식 인자) 규약을 쓴다.

## 옮긴 범위

- `RawPriestMenu.open(priest)`: 내 사제이고 막힘(0x493)이 없을 때만 구성한다(아니면 기본 객체 메뉴만). 순서: Fancy 제목 `Construct Building` → 세로 신전 세 타입(신전이 이미 있으면 비활성) →
  `Temple` 하위 → (워크샾 타입이 하나라도 허용되고 전투/특수/편집일 때) 워크샾 네 타입(원문 테마 인자 3,0,1,2)과 `Workshop` 하위 → Outpost(내용물이 있거나 미션 비차단/편집일 때) →
  Dais(이미 있으면 비활성) → `Construct` 하위(이동 불가면 `~r`, 건설 중/프레임 10~14면 비활성) → (조건이 맞으면) `Prayer`/`Devastation` → `View Netstorm Knowledge`·구분선 → 마지막에 기본 메뉴.
- `addBuild(type, disabled, priest, manaIcon)`: 허용 타입만, 비용과 SP를 매번 새로 읽어 `~r`(금액 부족)·`for %s%d`·마나 아이콘(테마 +4)·Dais/Altar의 `Build Level %d`를 만들고, 행 활성화는 명시 금지와 제외 목록(0052f7e0)만 본다.
  비교는 x87처럼 정수 비용을 단정도로 줄이지 않고 SP와 비교하며 NaN은 지불 불가이다. 명령 인자는 `(type << 16) | priest`.
- `selectBuild(type, priest)`: 워크샾 테마 관문 → 첫 비용(`float`로 축소)·SP → 둘째 비용(축소 안 함)·SP 순서로 두 번 조회하고 실패 안내(`NoTechTheme`/`BuildNoBucks`) 뒤 미리보기 생성과 커서 소유권 이동(0x160).
- `hasTheme(theme)`·`frameState(priest)`: 현재 Technology 내용물 체인의 flags4·동일 테마·flags2 0x800 아님 / 현재 물리 프레임의 signed 첫 글자 − 'A'(범위 밖은 RangeError).
- `createPriestMenuState`: 모드·타입·명령 전역의 보호 PE 초기값(신전 123/124/125·워크샾 85~88·Outpost 126·Dais 165·Altar 164·Devastation 168·명령 세 개). 읽기 함수가 있으면 사용 시점마다 현재 전역을 읽는다.
- `bindPriestMenuControls`/`bindPriestMenuWorld`: 이미 옮긴 `MenuGumpWidgets`/`Controls`/`MenuGump`와 같은 SID 풀의 사제 상태·프로세스·플레이어·운반·보상·생성 처리기에 위임하고 서로 다른 풀은 거부한다.
- 도달한 미연결 효과는 성공/빈 메뉴로 대체하지 않는다(`/미연결/` Error). 번역 결과는 첫 NUL까지만, 255바이트까지만 허용한다(원본 256바이트 스택 버퍼).

## 시험

`webpj/tests/raw-priest-menu.test.mjs` 6건:

1. 원본 관측 `priestmenu-x86.tsv`의 **Open 329**: 입력(소유자·막힘/건설 중/이동 불가·프레임·모드·Player 신전/Dais 필드·허용 mask·테마 mask·내용물·비용·SP·제외 mask)에서 전체 메뉴 구성 호출 순서 문자열. **첫 실행에서 일치**.
2. **Select 64**: 두 금액 조회·NaN·float 축소 경계·워크샾 테마 관문의 생성/안내/커서 효과와 실제 SP 조회 횟수. 첫 실행 일치.
3. **Theme 256·Frame 256**: 같은 기술 체인의 테마 판정과 signed char 프레임 코드. 첫 실행 일치.
4. 계약: 같은 기술 체인 변경에 대한 재조회, signed 프레임, 미연결 효과·범위·크기 거부.
5. 계약: 번역 NUL·255/256바이트 한도, 기도 분기의 막힘 사건 재조회(원본 fixture가 `hasEvent` 호출 횟수를 관측하지 않아 변형 확인 뒤 보강).
6. 연결: 컨트롤은 기존 메뉴 처리기로, 월드는 같은 풀의 조회/생성으로 위임하고 다른 풀은 거부.

**변형 확인:** 소유자 비교·막힘 사건·신전/Dais 오프셋·워크샾 테마 순서/모드 조건·Outpost 조건·프레임 비활성 범위·기도 조건·금액 비교 경계·편집 시 지불·가격 문구·제외 목록·Dais 레벨·마나 아이콘 +4·명령 인자 shift·선택 fround/NaN/둘째 조회/테마 관문·프레임 signed·테마 flags2/flags4·
포맷 NUL/한도·신전 접두·하위 활성 등 30가지를 넣어 시험이 모두 실패함을 확인했다. 처음 살아남은 3개(기도 분기의 막힘 재조회·포맷 NUL·포맷 한도)를 5번 시험으로 잡았다.

`webpj/tests/fixtures/manifest.json`에 `priestmenu`(원본 10.78, 905행) 항목을 추가했다.

## 남은 경계

- 기본 객체 메뉴(`baseMenu`)는 `RawObjectMenu`(다음 단위들), 번역/기술 설명/비용 표현·Player 메뉴는 호출자가 공급한다. Outpost 분기는 원문 그대로이며(멀티플레이 전용 건물은 호스트 쪽 허용 조회가 결정) 앱 수명에는 아직 연결하지 않았다(PORT_MAP `port`).
