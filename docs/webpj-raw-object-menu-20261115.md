# 객체 메뉴 이식 (WB04-E541)

G(UI 프레임워크·메뉴)의 "객체 우클릭 메뉴" 중 `client/RawObjectMenu`(원문 10.78 004b1270 객체 메뉴·0044cbd0 Damageable 메뉴·0044ca00 회수 항목)를
`webpj/www/core/raw-object-menu.js`로 옮겼다. 기준선 `225553df`와 현재 cpppj 소스가 같음을 확인했다.
`RawObjectMenuVariants`(파생 메뉴)·`RawObjectMenuOpen`·`RawObjectMenuInfo`와 `RawPlayerMenu`가 이 공통 계층을 쓴다.

## 옮긴 범위

- `RawObjectMenu.open(sid)`(004b1270): 편집이면 `Destroy`, 특수+미션 차단이면 `CHEAT Hurt Me!`(HP가 있을 때)·`CHEAT Destroy`·`CHEAT Make Yours` → 현재 도움말 단어(0054db38, NULL이면 거부)를 raw 타입 인자로 →
  편집/특수면 `CHEAT Set Owner`(정적 문자열 `SetOwner`) → 소유자가 있으면 owner→subscriber 조회 뒤 `Player` 번역과 Player 하위 메뉴를 만들어 부착.
- `openDamageable(sid)`(0044cbd0): 내 객체이면 raw 타입을 먼저 보관한 뒤 가상 개수를 조회하고 `addSalvage`, 그 뒤 `open`.
- `addSalvage(sid, type, count)`(0044ca00): 내 객체이고 raw 타입이 `flags2 0x200`/성장 비/비용 있음 중 하나일 때만, 명시 타입이 너겟/편집이 아니면 → 환불 조회(음수는 DWORD 부호 반전 뒤 `Salvage costs/gains %d~[IfortGump.3]`) →
  항목이 있으면 구분선 → 금지(`DenySalvage`) → 현재 설정 `global.salvageSid` 쓰기 → Vortex(`SalvageVortex`)·Inf(`SalvageInf`) 안내 → raw 타입이 회수 생략(122)이면 항목 없음 → 회수 명령.
- 모드·소유자·회수 금지는 사용 시점마다 현재 전역을 다시 읽고(읽기 함수가 있으면 값형 입력보다 우선), 환불·메뉴 작성 콜백이 슬롯/전역을 바꿔도 원본의 재조회 순서를 지킨다. 10.72(CD) 판본 분기는 웹 범위 밖이라 옮기지 않았다.
- `bindObjectMenuControls`/`bindObjectMenuIdentity`: 기존 controlled 메뉴(`itemCount`·구분선·항목·하위 메뉴)·`config.set`·`subscriberForOwner`에 위임하고 정적 문자열 인자는 같은 상수 표의 DWORD로 인코딩한다.
- 도달한 미연결 효과는 성공으로 대체하지 않으며(`/미연결/`) 번역은 첫 NUL까지·255바이트까지만 허용한다(원본 256바이트 스택 버퍼).

## 시험

`webpj/tests/raw-object-menu.test.mjs` 6건:

1. 원본 관측 `object-menu-x86.tsv`의 10.78(originals) **base 239·damage 239·salvage 239 = 717입력**: 입력 14 DWORD(소유자·로컬·raw 타입·명시 타입·모드·금지·flags2·비용·환불·개수·항목 수·HP·변경 조합)와
   콜백이 슬롯·전역을 바꾸는 조합에서 전체 효과 호출 순서. **첫 실행에서 717입력 전부 일치**했다(CD 10.72 판본 717입력은 웹 범위 밖).
2. 계약: helpWord NULL·Player 메뉴 장치 없음 거부, 실제 도움말 캐시(`helpWordRead`)가 값형 입력보다 우선, 판본/TYPE 표 크기 거부.
3. 계약: 번역 NUL·255/256바이트 한도, 회수 금지·로컬 소유자·편집/특수/차단 getter가 값형 입력보다 우선. 원본 fixture가 getter 우선순위와 번역 한도를 관측하지 않아 변형 확인 뒤 보강했다.
4. 연결: 컨트롤·설정·Identity 위임, 정수/문자열 인자 구별.

**변형 확인:** 편집/치트 분기·HP 조건·도움말 인자·SetOwner 조건·소유자 0 처리·Player 번역 순서/인자·Damageable 소유자/snapshot 순서·회수 조건 다섯 가지·환불 부호/절댓값·구분선·금지 문자열·설정 쓰기·Vortex/Inf 분기·
생략 타입 재조회·회수 명령·번역 NUL/한도·금지 getter 등 26가지를 넣어 시험이 모두 실패함을 확인했다(처음 살아남은 3개는 3번 시험으로 잡았다).

`webpj/tests/fixtures/manifest.json`에 `object-menu`(10.78 717입력 사용, 파일은 두 판본 1,434행) 항목을 추가했다.

## 남은 경계

- 파생 메뉴(`RawObjectMenuVariants`)·`RawObjectMenuOpen`·`RawObjectMenuInfo`·`RawPlayerMenu`와 실제 Gump 번역/조회 연결은 아직 없다(PORT_MAP `port`).
