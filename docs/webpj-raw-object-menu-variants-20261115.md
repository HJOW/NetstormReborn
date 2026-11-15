# 파생 객체 메뉴 이식 (WB04-E542)

G(UI 프레임워크·메뉴)의 "객체 우클릭 메뉴" 중 `client/RawObjectMenuVariants`(원문 10.78 가상 `+a8` 파생 본체 여덟 개)를
`webpj/www/core/raw-object-menu-variants.js`로 옮겼다. 기준선 `225553df`와 현재 cpppj 소스가 같음을 확인했다.
E541(`RawObjectMenu`)의 공통 객체/Damageable/회수 계층 앞에 각 파생이 자기 항목을 작성한다.

## 옮긴 범위

`new RawObjectMenuVariants(pool, hooks).open(sid, originalBody)`가 현재 `+a8` 본체 주소로 분기한다(알 수 없는 본체는 기본 메뉴로 바꾸지 않고 거부).

| 본체 | 파생 | 작성 |
| --- | --- | --- |
| 0041c210 | Meltdown 객체 | 미션 차단이 아니고 내 객체이면 HP를 먼저 보관 → `Meltdown!` 번역 → 최대 HP 조회 → 명령 조회 → 항목(최대 HP/2 < HP면 활성) → Damageable 메뉴 |
| 00422000 | 상태 변경 객체 | 편집이면 `Set as Cracked/Normal/Hard` 세 항목(번역→명령 조회 순서) → 공통 객체 메뉴 |
| 00462390 | Dig Up/Turn To Spell | 편집이면 `Turn To Spell!`, 항상 `CHEAT Dig Up`(포함 여부는 특수 모드 전역) → 공통 객체 메뉴 |
| 00462550 | 도움말 객체 | 현재 도움말 단어와 raw 타입 인자의 항목 하나 |
| 00485600 | Storm Power 객체 | `Storm Power: ~i%d~.` 번역·서식(+0xc WORD) → 머리 줄 → Damageable 메뉴 |
| 0048eca0 | 회수 객체 | raw 타입 보관 → 가상 개수 → 회수 항목 → 공통 객체 메뉴 |
| 004dcca0 | AI 객체 | 뇌가 있으면 AI 디버그 메뉴 → 내 객체면 `View Netstorm Knowledge` → 구분선 → Damageable 메뉴 |
| 00420b30 | Detonate/Obelisk | 예약 존재를 먼저 보관 → 비편집/비특수면 `Detonate!`(내 객체만 포함) 아니면 `Turn To Obelisk!` → 공통 객체 메뉴 |

- `objectMenuVariantCommandValue`/`isObjectMenuVariantCommand`: 보호 PE 명령 전역 주소 9쌍의 초기 ID 조회와 허용 판정(첫 일치만, 주소는 허용 판정에 쓰지 않음).
- 번역·가상 효과 뒤에는 현재 슬롯과 전역을 원문 접근 순서대로 다시 읽는다. 10.78 풀만 받는다.

## 시험

`webpj/tests/raw-object-menu-variants.test.mjs` 2건:

1. 원본 관측 `object-menu-variants-x86.tsv` **1,024행**: 여덟 본체 × 입력 11개(소유자·로컬·타입·모드·HP·최대 HP·예약·개수·Storm Power·AI·변경)와 콜백이 슬롯을 바꾸는 변경 조합에서 전체 외부 효과·인자·순서. **첫 실행에서 1,024행 전부 일치**했다.
2. 계약: 명령 ID 표 9쌍·허용 판정, 알 수 없는 본체/SID 범위/선택 경계 누락 거부.

**변형 확인:** Meltdown 관문·소유자·HP 부호/절반·번역 순서, 상태 변경 관문/이름, Dig Up 포함·Turn To Spell 관문, 도움말 인자, Storm Power 오프셋/헤더, 회수 순서, AI 조건/소유자/구분선, Detonate 예약·포함·모드, Obelisk 라벨, 명령 표 값 등 21가지를 넣어 시험이 모두 실패함을 확인했다.

`webpj/tests/fixtures/manifest.json`에 `object-menu-variants`(1,024행, 원본 10.78) 항목을 추가했다.

## 남은 경계

- 공통 객체 메뉴의 `Open`(E543)·`Info`(제목/정보 줄)와 실제 AI 디버그 메뉴(`00413dd0`), Gump 번역·조회 연결은 아직 없다(PORT_MAP `port`).
