# Factory 메뉴 이식 (WB04-E545)

I(건설·생산·경제)의 "공장·지식 메뉴와 기술" 중 `client/RawFactoryMenu`(원문 10.78 Factory 우클릭 메뉴: 454c00 Open·4540d0 현재 생산·4752b0 지식·454040 슬롯·4735c0 마나 문자)를
`webpj/www/core/raw-factory-menu.js`로 옮겼다. 기준선 `225553df`와 현재 cpppj 소스가 같음을 확인했다.
E543(`RawObjectMenuOpen.openFactory`)이 공통 +ac를 맡고 이 단위가 `populate`(파생 작성)를 채운다.

## 옮긴 범위

- `RawFactoryMenu.open(factory)`: 내 일반 Factory(Outpost 제외)만 `available`을 만들고, 어떤 분기에서도 Damageable 부모(`baseMenu`)를 마지막에 호출한다. Factory의 가상 표(0x509148/0x509210)가 아니면 거부한다.
- `currentProduction(factory)`: Fancy 메뉴 `Current Production` → 용량(가상 +b4)에서 시작해 실제 내용물 체인을 순회(flags16이 있는 Form만): 허용된 종류는 도움말 항목으로 쓰고, 금지된 종류도 슬롯은 차지하되 같은 종류는 한 번만 줄임 →
  하나도 없으면 `No Production Yet.` → `researchHelp` 항목 → `View Current Production` 하위 메뉴 → 남은 슬롯 반환.
- `available(factory)`: 현재 생산 → Fancy `Knowledge Available` → `Production Slots Available: %s`(`slots`) → 현재 기술 부모의 기술 종류를 0이 나올 때까지 순회(제외 전역 20개와 테마 불일치 제외, 테마 3 Factory는 flags2 0x800 예외, 중복 기술도 별도 행, 후보 200행·설명 81·마나 9바이트 한도) →
  공유 CRT 정렬(`ui/knowledge-sort.js`) → 생산 중이 아닌 행만 `researchCommand`(`종류<<16|Factory`; 금지 목록은 include, 슬롯 부족은 enabled) → 없으면 `All Knowledge is in Production` → `Put Knowledge into Production` 하위 메뉴 →
  레벨이 3 미만이면 `Upgrade costs`(SP/비용을 매번 읽고 편집이면 지불 가능 색).
- `formatFactoryMana`(4735c0): `wrtsWRTS.?>` 그림 순서로 `~[I103.프레임]` 토큰을 만들고 점은 건너뛰며 활성 개수만큼 +11, 250바이트 이상은 진단. `slots(count)`(454040): 0은 번역된 `None`, 음수는 빈 문자열(항상 `None`을 먼저 번역), 양수는 `?` 아이콘.
- `createFactoryMenuState`/`bindFactoryMenuControls`: 보호 PE 기본 타입·명령·제외 전역과 기존 controlled 메뉴/정적 문자열 연결.

## 시험

`webpj/tests/raw-factory-menu.test.mjs` 6건:

1. 원본 관측 `factory-menu-x86.tsv`의 **Open 39입력**: E543의 공통 `openFactory`를 거쳐 생산/지식/업그레이드의 전체 작성 순서·인자·동률 정렬을 반환값과 함께 대조. **첫 실행에서 일치**했다.
2. **Mana 32·Slots 5입력**(점·활성 개수·알 수 없는 ASCII·0/음수 슬롯). 첫 실행 일치. (76행 전부)
3. 계약: 타 소유자/Outpost는 기본 메뉴만, 잘못된 가상 표·SID 범위·저장소 크기 거부, 후보 마나 9바이트 한도·NUL 절단, 기본 메뉴 누락 거부.
4. 계약(변형 확인 뒤 보강): 마나 ASCII 127/128·25×10바이트 진단·첫 NUL, `None` 선조회, 편집의 업그레이드 지불, 후보 200/201행, 설명 81/82바이트, 번역 NUL·255/256바이트.
5. 계약: 설명의 NUL 뒤 바이트는 81바이트 한도에 넣지 않는다.
6. 연결: 컨트롤/메뉴 위임, 정적 문자열 인코딩, 인코딩 표 누락 거부.

**변형 확인:** 38가지(마나 점/활성/표/ASCII/길이, 슬롯 None/음수/상한/활성, Open 소유자/Outpost/가상 표, 생산 flags16/중복 슬롯/허용/서식/없음 머리/도움말 인자, 남은 슬롯 비교, 지식 제외/테마 일치/테마 3 예외/정렬/생산 중 숨김/include/인자/없음 항목,
업그레이드 레벨/지불/경계/NaN, 후보 행/마나/설명 한도·NUL, 포맷 NUL/한도)를 넣어 36가지가 시험에서 잡혔다. 남은 2개는 동등 변형이다: 슬롯 상한 256은 28개 이상이면 마나 길이 진단이 먼저 같은 `RangeError`를 내고,
활성 개수 `-1`과 `0`은 카운터가 0부터라 어느 쪽도 활성 프레임이 없다.

`webpj/tests/fixtures/manifest.json`에 `factory-menu`(76행, 원본 10.78) 항목을 추가했다.

## 남은 경계

- 실제 기술 부모(`4acac0`)·TotalMade·Deck·번역/비용 표·Gump 연결은 호출자가 공급한다. 생산 명령 실행(`researchCommand`)은 별도 단위이며 앱 수명에는 연결하지 않았다(PORT_MAP `port`).
