# webpj 데빌메이커 생산 프로세스(RawWindAviaryProcess)·원주 위치(windCirclePosition) 이식 — WB04-E185

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백여든다섯 번째 부분이다. 데빌메이커(윈드 아비어리)의 실제 80바이트 생산 상태를 현재 Kernel 과 같은 풀에 연결하는 `o/RawWindAviaryProcess`(패치 `0045edf0` 후처리·`45f0a0` 프레임·`0045b680` 비행체 사망·`0045c090` Form 삭제와 `SquidProcess` 파생 래퍼)와, 특별 Wind 생산 12단계의 실제 원주 위치를 구하는 `o/RawWindCircle` 의 `WindCirclePosition` 을 옮겼다(cpppj 기준선 `1f00a25a`, 기준선 뒤 `RawWindAviaryProcess`/`RawSunAviaryProcess` 의 변경은 `WritePacket` 의 `force/protocol/message` 인자 추가뿐이며 E121 `writeProcess` 가 이미 같은 서명을 지원함). E121 의 `RawWindAviaryState`·`windCirclePoint` 위에 얹어 프레임/후처리/원주/삭제 실제 PE 관측 2,807행을 재생한다. 이 단위에서 cpppj `RawSunAviaryProcess.cpp` 의 `FindAttachedProcess` helper 도 처음 옮겼다.

## 1. 옮긴 것

`www/core/raw-wind-aviary-process.js`(DOM 없음)

- `findAttachedProcess(pool, parent, type)` — 부모의 종속 체인에서 첫 살아 있는(dead 비트 2 아님) 타입 Form 의 SID(없으면 0)를 찾고, 다음 링크는 현재 후보를 검사한 뒤 읽으며 손상된 무한 체인은 풀 크기로 제한해 거부한다.
- `new RawWindAviaryProcess(pool, globals, hooks)` — `globals = { fort, friendlyFire, server, boss, editor, checking, now, processType, flyerType, aviaryType }`(모두 현재 값 getter), `hooks` 는 프로세스 조회/할당/부착/Damageable/진단/프레임·소유자·높이·발자국·Pop·비행 프로세스 생성/연결·원주 위치·종료·삭제 하위이며 도달한 위치에서만 요구한다.
- `postPop(parent, flags)` — flags bit 0·(편집 fort 이면서 친군 사격이 아님이 아님)·서버이면 이미 부착된 타입 59 프로세스가 있을 때 진단 `0x275` 후 새 80바이트를 할당·초기화(E121 `RawWindAviaryState.initialize`)·부착하고 그 뒤 항상 공통 Damageable 을 실행한다.
- `frame(fields)` — 부모 raw 를 살아 있는 뷰로 읽어(`setFrame` 이 쓴 프레임을 뒤 높이 조회가 다시 읽음) 정지/편집 차단이면 무동작, 상태 1(초기화) → 2/7(생산 중: 0.3초 이내 프레임 순환, 경과 4초 비행체 생성·Pop, 6초 이후 분리와 상태 7, x87 중간 곱은 double 유지) → 5(26초 대기) → 1, 특별 부모는 상태 11 → 12(서버면 원주 위치에 비행체 생성·Pop) → 13(종료)이고 checking 일 때 상태 범위 밖과 프레임 진단을 올린다. `flyerDied(fields, refresh)` 는 서버만 상태 2 로 바꾸고 Form 갱신, `formDestroy(fields)` 는 boss·현재 DWORD 임시 포인터(주소 곱셈의 32비트 감김, bit31 별칭 포함)·dead 비트 검사 뒤 비행체 타입 불일치 진단과 가상 10 삭제를 하며 진단 뒤에도 처음 구한 포인터를 쓴다.
- `WindAviaryProcess extends SquidProcess` — 80바이트를 소유하고 `runFrame`/`onAttached`(현재 Form/부모를 inner 에 기록)/`onFormDestroy`/`notify`(빈 통지)/`fields`/`writePacket`(0x74 writer, force 면 전체 필드)을 제공한다.

`www/core/raw-wind-circle.js`(보강)

- `windCirclePosition(pool, types, parent, flyer, fields, readHeader)` — 소유자 callback 뒤 현재 생성 비행체의 SHP 헤더(`{ walkStepY, height, hotspotY }`)를 먼저 읽고, 부모의 현재 중심(발자국 절반 보정)에서 원문 count(+0x44)·index(+0x48)의 원주 점을 구해 13번 높이(`walkStepY × 0.09090909361839294`)와 현재 발자국으로 보정한다(`1.399999976158142` x 오프셋, 각 float 저장 경계 보존).

## 2. 검증

`tests/wind-aviary-process.test.mjs`(6건). 각 행은 입력만 설치하고 모듈이 계산하며 매 행 새 장면을 만든다.

- `wind-aviary-frame-x86.tsv` 1,639행(F 1,607 + P 32): 실제 4/6/26초 경과·가감속·서버/boss/편집/표시 유무의 전체 80바이트·세 슬롯(50/60/70)·효과 순서, 후처리의 편집/친군 사격/서버/최초 비트 조합.
- `wind-circle-frame-x86.tsv` 288행(실제 중심/원주/헤더를 실행한 상태 12, 헤더 callback 이 본체/부모를 바꾸는 재진입 포함), `wind-aviary-destroy-x86.tsv` 880행(정상 슬롯/주소 끝/bit31 별칭/진단 재진입과 사용하지 않는 flags).
- 계약: 래퍼의 프레임 전달/부착 필드/빈 통지/0x74 writer, PATCH 전용·전역 조회기·원주 경계·선택된 하위 누락·순환 체인 거부.

최초 실패와 교정: 새 모듈과 시험 6건은 모든 fixture(2,807행)를 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 실제 Kernel 호스트 부착(`SquidProcessHost`)·SHP 헤더 조회·`RawFlyerProcess`(비행 프로세스 생성/연결)·`RawWindFlyer`(더스트 데빌 전용 초기화/공격)·Pop/Owner/Frame 세션 연결은 후속 단위이다. 이 몸체는 hook 경계로만 연결되어 있다(`RawWindFlyer` 는 `RawFlyerProcess` 가 선행).
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

`RawFlyerProcess`→`RawWindFlyer`, `RawBurningFrame`(캡처 자산 필요)·`BurningVisualSystem`·`RenderProcessSystem`, `RawPacketDispatch`→`RawConstructionCommand`, Missile Host 팩토리, 번개 수명/프레임/화면, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
