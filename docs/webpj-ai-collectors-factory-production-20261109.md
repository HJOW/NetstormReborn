# webpj AI 수집자(RawAiCollectors)·시설 생산(RawAiFactoryProduction) 이식 — WB04-E179

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백일흔아홉 번째 부분이다. AI 수집자 확보와 자기 영역 후보 배치를 맡는 `o/RawAiCollectors`(패치 `00415a10` 수집자 확보·`00415920` 자기 영역 후보 배치·`004108f0` 일반 배치 통지·`004115e0` 비행 수집자 검사·`00411ca0` 취약 사제 검사)와, 원소별 기술 선택과 시설 생산/업그레이드 대기 목록을 맡는 `o/RawAiFactoryProduction`(패치 `00413570` 기술 선택·`004acc60` 생산 총수·`004136b0` 생산 관리·`00411180` 업그레이드 완료)을 옮겼다(cpppj 기준선 `1f00a25a`, 기준선 뒤 `RawAiCollectors.cpp/.h`·`RawAiFactoryProduction.cpp/.h`·두 시험의 변경 없음을 `git diff --numstat` 으로 확인). 두 몸체 모두 독립 원본 관측 fixture 가 없고 cpppj 계약 시험이 원본 분기 순서를 손으로 쓴 입력/기대값으로 직접 단언한다.

## 1. 옮긴 것

`www/core/raw-ai-collectors.js`(DOM 없음)

- `new RawAiCollectors(memory, pool, types, random, globals, hooks)` — `memory` 는 E164 `RawAiBrainMemory`(본체 0xb08 + 목록 블록), `pool` 은 같은 PATCH SidPool, `types` 는 현재 TYPE 의 `{ flags2 }` 표(비어 있지 않아야 함), `random` 은 같은 게임 공용 난수(`next(limit)`), `globals = { now(), localOwner(), invalidGraph() }`(앞 둘은 생성자에서, 마지막은 도달할 때만 요구), `hooks = { product, canAfford, maxHitPoints, enemyAvailable, mayPlace, deploy, log, dropSound }` 는 이미 옮긴 실제 하위와 장치 경계로 도달한 위치에서만 요구한다(없으면 그 위치에서 거부하고 앞선 효과는 취소하지 않음).
- `ensure(token)` — 신전(+0x294)/graph(+0x298)/후보(+0x28c) 관문을 통과하면 (1) 취약한 적 사제용 비행 수집자 경로(기한 +0x5a8 → 난수 1000 중 100 미만 → +0x248 의 0x100 비트 → +0x38c < +0x388 → 적 사제 SID 가 1 이상 용량 미만 → 참여 → 취약 → 기한 +0x5e0 → 기존 비행 수집자 없음)에서 첫 생산 보유 비행(flags2 0x20000) 타입을 자기 영역에 배치하고 끝내거나, (2) 이미 충족(+0x2cc)이면 끝내고 아니면 난수로 고른 후보 타입(생산이 없으면 첫 후보)을 생산 재조회·기한·대기 조각(pending 0x100) 제외·비용 확인 뒤 자기 영역에 배치한다. 비행 경로는 일반 비용/대기 조각 판정을 추가하지 않는다.
- `placeInHome(token, type)` — 매 시도마다 자기 경계(+0x270/+0x274/+0x278/+0x27c)를 다시 읽고 y → x 순서로 두 난수를 소비해 후보를 만들고, 전체 `mayPlace`(질의 `{ type, argument: 0, x, y, flags: 0, owner: 본체 owner & 255, mode: 1 }`)가 허용한 첫 후보만 배치한다. 최대 20회 뒤에는 아무 조각도 만들지 않는다.
- `deployWithNotice(token, type, x, y, argument, direction)` — 실제 배치 → 생각 기한 두 개(현재 rate + 현재 시각, rate + 1 + 현재 시각을 +0x5a8/+0x5b0 QWORD 에 기록)와 +0xb00 이 켜졌을 때의 Lay 로그(좌표는 y → x 절삭 정수) → 현재 로컬 소유자와 같으면 드롭 음향. `hasFlying`(현재 수집자 목록의 flags2 0x20000 검사)·`vulnerable`(현재 HP 선저장 → 최대 HP 정수 절반 signed 비교)도 공개한다.

`www/core/raw-ai-factory-production.js`

- `new RawAiFactoryProduction(memory, pool, types, finder, globals, hooks)` — `types` 는 `{ theme, flags2, cost }` 표, `finder` 는 현재 권한/containedType 와 저장 next 를 공유하는 `ContainedFinderState`, `globals = { now(), invalidGraph() }`, `hooks = { technology, product, addProduct, level, upgrade, log, demo }`. 행동 종류는 `AI_FACTORY_ACTION { addedProduct: 0, upgrade: 1 }` 이다.
- `selectTechnology(token, theme)` — 현재 owner 기술 부모의 내용물(커서가 먼저 저장한 next 유지) 중 flags4(+20 의 0x4)인 것만 현재 TYPE 의 원소로 판정한다. flags2 0x800(battery)이면 본체의 치환 필드(+0x3b4 + 원소×4)가 1 이면 중립 원소(3)만, 2 이면 본래 원소 또는 중립, 그 밖의 0 아닌 값이면 불일치로 바꾼다. 현재 생산 조회에 이미 있는 TYPE 은 건너뛰고 첫 일치 미등록 TYPE(없으면 0)를 돌려준다. `countProducts(factory)` 는 004acc60(mask 0x10, kind 0)의 총수이며 같은 종류도 각각 센다.
- `manage(token)` — 신전/graph/기한(+0x5a8) 관문 뒤 미처리 시설 목록(+0x30c/+0x314)이 비어 있으면 충족(+0x2cc)·지갑(+0x2d8 > 1199)·업그레이드 대기(+0x318/+0x320)의 첫 시설을 업그레이드 요청 → 목록 제거 → 생각 기한 순서로 처리한다. 비어 있지 않으면 첫 시설에 선택된 기술을 추가 등록(level+2 용량 미만이면 시설 유지)하고, 가득 차면 모든 중복을 압축 제거한 뒤 현재 level 이 2 미만이고 남은 기술이 있을 때만 업그레이드 대기 목록에 추가한다.
- `upgraded(token, factory)` — 같은 시설을 미처리 목록에 추가(가득 차도 비용/데모 처리는 계속, 중복 허용) → 현재 TYPE 의 음수 비용 절삭(`-cost` 의 ftol 하위 DWORD)을 지갑(+0x2d8)에 가산 → 데모 판정 뒤 현재 지갑에 1190 하한을 적용한다. 목록 추가/제거는 원본 네이티브 0040e9b0(가득 차면 포인터를 읽지 않고 생략)/0040ea00(모든 일치의 안정 압축, 미사용 꼬리 보존)과 같다.

## 2. 검증

`tests/ai-collectors.test.mjs`(9건)·`tests/ai-factory-production.test.mjs`(11건). 두 몸체 모두 독립 원본 관측 fixture 가 없어 cpppj 시험의 입력과 단언을 그대로 옮겼다.

- 수집자: 신전/그래프/후보 관문은 난수와 선택되지 않은 하위를 요구하지 않음, 일반 선택의 1000 → 후보 종류 → y → x 난수와 세 번 생산 조회 순서, 충족/기한 전/보류 조각/자금 부족의 각 종료 지점, 난수로 고른 타입의 생산이 없으면 첫 후보로 대체, 취약 적용 비행 타입은 일반 관문을 추가하지 않고 HP 를 최대 HP 하위 전에 저장, 실패 후보 최대 20회와 매 시도의 현재 경계 snapshot, 배치 하위 뒤 현재 rate/시각과 로그 뒤 현재 로컬 소유자 재조회, 정수 절반 HP·목록 물리 범위·unordered 기한과 선택된 효과 누락, 생성자 거부.
- 시설 생산: 원소/flags4/현재 생산 보유를 내용물 순서대로 판정한 첫 일치 미등록 기술, battery 치환 필드 0/1/2/기타의 본래 원소·중립 3 치환, 생산 조회 효과로 링크가 바뀌어도 커서가 먼저 저장한 next 유지, 한 종류 추가 뒤 슬롯이 남으면 같은 첫 시설 유지, 가득 찬 시설의 모든 중복 압축과 현재 level/남은 기술로 업그레이드 대기 추가, 후보가 없으면 큐만 정리하고 행동 기한/업그레이드를 합성하지 않음, 중복 생산 총수, 미처리 0/충족/지갑 1200/업그레이드 대기와 요청 → 제거 → 기한 순서, 선택된 미복원 업그레이드/누락 등록은 앞선 큐를 소비하지 않고 실패, 완료 시설 중복 허용과 TYPE 음수 비용 절삭, 큐가 가득 차도 계속하는 지갑과 현재 데모 지갑 1190 하한, 생성자 거부.

최초 실패와 교정: 새 모듈과 시험 20건은 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 실제 생산 조회(`004103c0`/`00410360`)·기술 부모(`0048fc90`)·비용 비교(`00410280`)·최대 HP(`004adcb0`)·전체 배치 검사(`0049b510`)·일반 배치(`004106e0`)·생산 등록(`00410420`)·업그레이드 요청(`00453bf0`)·로그/음향 장치 연결은 후속 AI 세션 연결 단위이다. 이 두 몸체는 모두 hook 경계로만 연결되어 있다.
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

`RawAiCarrierActions`·`RawAiPreparation`·`RawAiStrategySetup`·`RawAiWorldRegistration`·`RawAiObjectNotice`·`RawAiDamage` 등 AI 계열 나머지(`client/RawAiDebugMenu` 포함), `RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
