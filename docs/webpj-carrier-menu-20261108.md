# webpj 운반물 메뉴(RawCarrierMenu) 이식 — WB04-E96

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 아흔여섯 번째 부분이다. 운반물(Carrier)의 운반물·주문 메뉴 부모 몸체(004288f0)를 cpppj 기준선 `1f00a25a` 의 `client/RawCarrierMenu` 에서 옮겼다. E56 의 `raw-contained-finder.js` 와 E95 의 `raw-carrier-drop-point.js`(위치 계산기 연결용)가 선행이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD(10.72) 분기(직접 SP 전역·`__ftol` 비용 절삭·float 부호 검사)는 옮기지 않았다.

## 1. 옮긴 것

`www/core/raw-carrier-menu.js` — `RawCarrierMenu(pool, types, contained, state, hooks)`, `bindCarrierMenuDropPoint(hooks, pool, point)`, `bindPriestMenuCarrier(priestHooks, carrier)`

- `open(carrier)`: 부모 TYPE flags1 의 0x1000000(메뉴 생략)이 없으면 이동 불가(extra&0x20 또는 지상 고정 `groundImmobile`, 단락 평가)를 계산하고 안내 서식 "~wNothing" 을 한 번 만든 뒤 실제 contained finder 로 내용물을 현재 체인 순서로 순회한다. 어느 경우든 마지막에 기본 객체 메뉴(`damageableMenu`)를 부른다.
  - 각 내용물마다 설명(`description`, 내용물 종류 WORD +18) → TYPE flags2 에 0x20000(비행 가능)이 있으면 이동 종류→경로→경로 활성으로 "비행 중" 판단 → 내려놓을 위치(`dropPoint`)가 두 축 모두 0 초과·현재 지도 한계 미만이면 활성 → 내용물 flags WORD(+20) 읽기 순서이다.
  - 이동 불가면 "(Not while paralyzed)", 비행 중이면 "(Not while flying)" 안내를 만들고 비활성이다. flags bit0(주문)이면 현재 정수 비용(`patchCost`)과 현재 분산 SP(`money(로컬 소유자)`)를 비교해 편집 모드이거나 비용이 SP 이하일 때 활성이다(SP 조회 뒤에 편집 전역을 읽음).
  - bit0 이 없으면(운반물) "Carrying: ~i%s~.\n" 헤더를 쓰고 부모 소유자(+34)가 로컬 소유자일 때 "%sDrop~. ~i%s~. %s" 항목(Drop 명령 0x03000076, 활성은 이동 불가 아님 && 활성)을 쓴다. bit0 이 있고 소유자가 로컬이면 Cast 항목(Cast 명령 0x0300011a, 차단 이벤트 0x493·사제 부모의 건설 이벤트 0x25d 가 있으면 비활성)과 "About %s"(도움말 명령, 내용물 종류, 항상 활성)를 쓴다.
  - 소유자·이동 종류·내용물 flags 는 외부 콜백이 바꿀 수 있으므로 쓰는 시점마다 현재 슬롯에서 다시 읽는다. 서식 결과는 NUL 뒤를 버리고 256바이트 이상이면, 안내 문자열은 128바이트 이상이면 거부한다. 도달한 훅이 없으면 오류이다(빈 성공/금액/위치로 바꾸지 않음).
- `state` 는 `{ localOwner, editor, mapSize, helpCommand, priestType(158), dropCommand, castCommand, localOwnerRead?(), editorRead?() }` 이며 호출마다 현재 값을 읽는다(getter 가 있으면 우선).
- `bindCarrierMenuDropPoint`: 메뉴의 `dropPoint` 훅을 E95 위치 계산기(`find`)에 연결하되 다른 세션 풀은 거부한다. `bindPriestMenuCarrier`: 사제 메뉴의 기본 호출(`baseMenu`)을 `open` 에 연결한다.

## 2. 검증

- `tests/carrier-menu.test.mjs`(2건): `carrier-menu-x86.tsv` 의 `originals` 314행(`originalCD` 314행은 10.72 라 제외). 입력 22 DWORD(소유자·로컬·부모 TYPE·flags1/flags2·extra·지상 고정·경로·활성·내용물 수/flags/종류·위치 x/y·지도 한계·비용·SP·편집·차단·건설·콜백 변이 7비트)로 번역 키 hex·설명/이동/경로/위치/SP/이벤트 조회 인자·헤더/항목 문자열 hex·명령·매개변수·활성 및 기본 메뉴 호출의 효과 순서가 같다(콜백이 소유자·부모 TYPE·내용물 flags·편집 전역을 바꾸는 변이 포함). 계약: 도달한 누락 하위(지상 고정/기본 메뉴)의 명시 실패, 메뉴 생략 TYPE 의 기본 메뉴 직행, 사제 기본 메뉴 연결, 잘못된 판본/TYPE 표 크기/SID 범위 거부, 위치 계산기 연결의 같은 풀 검사.

최초 실패와 교정: 새 모듈은 위 fixture 314행과 계약 검사를 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 실제 controlled 메뉴 작성(`BindCarrierMenuControls`: `AddHeader/AddText`)·기본 객체 메뉴(`BindCarrierMenuBase`: `RawObjectMenu::OpenDamageable`)·월드 연결(`BindCarrierMenuWorld`: 사제 지상 고정·분산 비용·이벤트 조회·종속 조회)은 해당 모듈 이식 뒤 잇는다. cpppj 의 `RealPipelineCenterAndMenu` 통합 검사(실제 배치 파이프라인+중심 조회+메뉴)도 그때 옮긴다.

## 4. 다음 시작점

`RawCarrierInput`(운반물 입력; 메뉴 명령 처리), `RawBalloonFrame`·`RawPathRuntime` 계열(보행/경로), `RawGunGroundTarget`·`RawGunProcess`(+`MakeGunTargetHooks`), Missile 계열, `RawTempleDraw`/캡처 계열(자산 시험 장치), `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawManaBoltArrival`(시험 장면 선행), `RawIslandRuntime` 계열, 그 밖의 작은 몸체이다.
