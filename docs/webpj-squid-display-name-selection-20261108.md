# webpj 객체 표시 부모·이름·방어·선택(RawSquidDisplayParent·RawSquidName·RawSquidDefense·RawSquidSelection) 이식 — WB04-E58

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 쉰여덟 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawSquidDisplayParent`(원본 004ad500 표시 부모), `o/RawSquidName`(가상 +0x68 이름 004ace20/00454d30), `o/RawSquidDefense`(기본 방어 몸체 0043d9b0), `o/RawSquidSelection`(선택 비트 41c0b0/44b2b0·사정거리 종류 474240·내용물 표시 입구 426080)을 옮겼다. 웹은 10.78 전용이라 CD 10.72 분기(방어 4aed90 등)는 옮기지 않았다. `RawSquidName.cpp` 만 기준선 뒤 한 줄(`BaseName` 공개 접근자, `8ee7666b`)이 더해졌고 ①(결과 영향 없음)로 판정해 웹에서는 `baseName()` 으로 공개했다. 나머지는 기준선 뒤 변경이 없다.

같은 묶음의 `SquidFrameBinding`(프레임 지정 훅 조립)은 `SquidDisplay`·`SquidFrame`·`SquidUnpop` 연결이 필요해 이번에 옮기지 않았다(3절).

## 1. 옮긴 것

`www/core/raw-squid-display-parent.js` — `runRawSquidDisplayParent(raw, checking, hooks)`, `SQUID_DISPLAY_ASSERTION`

- 진입 때 raw 프레임(+36)을 부호 있는 DWORD 로 한 번 읽어 고정한다(진단이 raw 프레임을 바꿔도 지역값 유지). 현재 checking(getter)이 비0 이면 현재 TYPE 뷰를 조회해 프레임 범위(`frameCount × (flags1 & 0x440000 ? 2 : 1)` 를 하위 32비트로 감은 뒤 부호 있는 비교)를 검사한다.
- 범위 밖이면 checking 을 다시 읽어 비0 일 때 `typeNumber(뷰)` → `frameError(번호, 프레임, abstract)` 를 호출한다(abstract 는 번호 조회 전에 고정). 범위 밖이거나 SHP 가 없으면(null 또는 두 해제 표식 `-0x22222223`·`-0x32323233`) 원문 846 진단(`getTypeStruct()->frameCheck( isAbstract(), theFrame, true )`, `\Ns\O\Squid.cpp`)을 요청한다.
- 진단 복귀 뒤 현재 raw TYPE 번호로 새 뷰를 조회해 범위 안·SHP 있음이면 `mark(번호, 고정 프레임, 현재 x 비트, 현재 y 비트, 선택 비트<<4)`, 범위 밖이고 checking 이 비0 이면 오류 상세만 기록한다. 선택하지 않은 장치는 요구하지 않고 필요한 장치가 없으면 거부한다.
- TYPE 뷰의 `flags1`/`frameCount`/`loaded`/`header` 는 읽는 순간의 현재 필드이다(같은 레코드를 빌리는 getter 를 허용). checking 은 불리언 getter 도 받는다.

`www/core/raw-squid-name.js` — `RawSquidName(pool, types, hooks)`, `squidNameEntry`, `squidLevelName`, `formatSuffix`; 표 `www/core/squid-name-vtables.js`

- `baseName(parent)`: TYPE 번호는 언어 조회 전에 선택, 언어 0 은 한 번·그 밖은 두 번 조회해 두 번째가 1 이 아닐 때만 설명 번역, 설명은 언어 효과 뒤의 현재 값(첫 NUL 까지) → 80바이트 경계 확인 → 설정 치환 → 80바이트 경계 확인.
- `name(parent)`: 83개 가상 표(`SQUID_NAME_ENTRIES`, cpppj `SquidNameVtables.inc` 와 행 단위 같음)로 진입점을 고르고 base(0x4ace20)면 base 이름, 아니면 Factory(0x454d30): base 이름 → 현재 번역 패턴 `" Level %s"`(언어 관문 없음) → 현재 가상 레벨(`level(sid)`) → 로마숫자(범위 밖 0xff) → CRT 서식 → 접미사 256바이트 경계 → 80바이트 경계. 미확인 표는 base 이름으로 대신하지 않고 거부한다.
- `formatSuffix`: `%%`·`%[-][폭][.정밀도]s` 만 지원하는 snprintf 부분집합이며 다른 변환은 거부한다.

`www/core/raw-squid-defense.js` — `rawSquidDefaultDefense`, `makeRawSquidDefenseHooks(pool, entries, hooks)`, `SQUID_DEFENSE_ENTRIES`

- 원문 두 명령 `xor eax,eax; ret 4` 의 인자 무관 false. 분배는 현재 victim 슬롯의 +0 DWORD 와 검증된 5개 진입 표(캠페인71/128/사제/anim 가상 표)를 비교해 진입이 0x43d9b0 일 때만 false 를 돌려주고, 그 밖의 표는 기존 `rejectDamage` 경계로 보내며 경계가 없으면 거부한다. 빈 표·범위 밖 SID 를 기본 false 로 우회하지 않는다.

`www/core/raw-squid-selection.js` — `RawSquidSelection(pool, refresh)`, `squidSelectionEntry`, `selectionUsesRange`, `startSelectionContentPreview`; 표 `www/core/squid-selection-vtables.js`

- `mark(sid, value)`: 현재 가상 표의 +0x7c 진입이 Damageable 파생(44b2b0)이면 표시 갱신(+0x88)을 먼저 호출하고 그 효과 뒤의 현재 extra(+40)에서 비트2 만 `value & 1` 로 바꾼다. 기본 진입(41c0b0)은 표시 갱신을 요구하지 않는다. 표에 없거나 확인되지 않은 파생은 비트 쓰기 전에 거부한다. free/dead 필터가 없는 물리 슬롯 쓰기이다(`SidPool.physical`).
- `selectionUsesRange(type)`: group 0/1/6/3 또는 flags2 0x100 (이동 가능 판정이 아니라 사정거리 종류 판정).
- `startSelectionContentPreview(pool, parent, loadingDepth, regular, assertion)`: 로딩 깊이가 0 이 아니면 종료, extra 의 `&9` 이면 종료, TYPE 이 70 미만이면 `!s.isForm()` 진단 뒤 현재 TYPE 을 다시 검사해 여전히 70 미만이면 종료. 생성할 때는 `regular(parent, 0x491, 0.2f)` (float32 비트 `0x3e4ccccd`).

## 2. 검증

- `tests/squid-display-parent.test.mjs`(4건): `squid-display-parent-x86.tsv` 696행(804 관측 당시 번호 getter 대체 장치·첫 로그 인자만)과 `squid-display-number-x86.tsv` 696행(실제 번호 getter·세 오류 인자)에서 입력(checking·프레임·TYPE0 flags·loaded·header·변이 종류·진단 중단)이 같을 때 효과 순서·raw 50바이트·두 TYPE 필드·checking·결과(return/assertion)가 모두 같다. 계약: 선택하지 않은 장치 비요구·누락 거부·50바이트 슬롯·함수형 checking.
- `tests/squid-selection.test.mjs`(3건): `squid-selection-x86.tsv` 5600행(Mark·사정거리 종류·내용물 표시 입구)의 결과·효과 순서·현재 [VT, extra, TYPE] 이 같다. 계약: 기본 +7c 의 표시 비요구와 파생·미지 표의 비트 쓰기 전 거부. 가상 표 83칸은 cpppj `SelectionVtables.inc` 와 행 단위로 비교한다.
- `tests/squid-defense.test.mjs`(3건): `squiddefense-x86.tsv` 10.78 24행(원본 PE 의 5바이트 몸체가 인자와 무관하게 0 을 반환하고 raw 불변), 실제 `SquidFactory` 가 만든 71/128/158/160 가상 표만 기본 false 로 분배하고 다른 파생은 fallback 보존, 다른 파생·빈 표·범위 밖 SID 거부.
- `tests/squid-name.test.mjs`(5건): cpppj `SquidNameTests` 4건과 같은 입력(언어 조회 횟수·번역 선택, Factory base→패턴→레벨 순서, TYPE 선택 시점·NUL·80바이트 경계, 83표·로마숫자·CRT 서식·256바이트 경계)과 가상 표 83칸의 `SquidNameVtables.inc` 비교. cpppj 도 원본 PE 관측 없이 계약 검사만 둔다.

최초 실패와 교정: 모듈은 표시 부모 1392행·선택 5600행을 첫 실행에 모두 통과했다. 방어 24행 재생에서 `SidPool.allocated()` 가 두 번째 방문 때 "할당되지 않은 SID 쓰기"로 거부했다 — 관측의 padding 패턴이 첫 방문에서 상태 바이트까지 덮기 때문이며, cpppj 검사도 같은 이유로 `Slot()` 을 const_cast 해 쓰므로 웹 검사를 물리 슬롯(`physical`)으로 바꿨다.

## 3. 경계와 남은 것

- 한국어 이름의 바이트 길이: 원본 80/256바이트 경계는 JS 문자 수로 센다(원본 영어 이름과 같다). 한국어 표시 부호화(cpppj `EncodeDisplayText`)의 바이트 길이는 이름 표시를 월드에 연결하는 단위에서 다시 정한다.
- 실제 월드 연결(언어·번역·설정 치환 장치, Factory 가상 레벨, 표시 갱신 `+0x88`, 피해 훅 `rejectDamage`, 내용물 표시 Regular 의 실제 등록)은 해당 소비자 이식 단위와 WB05 에서 잇는다.
- `SquidFrameBinding`(SquidFrame 지정 훅을 `SquidDisplay`·`SquidUnpop`·`SquidPop` 으로 잇는 조립)은 `SquidDisplay`/`SquidFrame` 이식 뒤에 옮긴다.

## 4. 다음 시작점

`SquidDisplay`/`SquidFrame`/`SquidFrameBinding`, 종류별 PostPop 본체(Asset·Fence·Nugget·Buried·Carrier·Bomb·Factory·Priest·Rune), 후보/지형 권한(`RawCanonPlacementPermission`) 이다.
