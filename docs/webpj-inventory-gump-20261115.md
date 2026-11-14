# webpj 생산 격자·항목 InventoryGump (WB04-E472, 2026-11-15)

기준선 `1f00a25a`의 `client/InventoryGump.cpp/.h`(207줄)를 `www/ui/inventory-gump.js`로 옮겼다. 생산 패널의 내용물 격자(가상 표 `506800`)와 항목(`5068f4`)의 생성·배치·통지·점멸이다. 항목의 아이콘 자식 부착(`43ce40`)·Draw·클릭·비용 판정은 다음 단위(InventoryItemGump)다.

## 이식한 것

- `InventoryGridNode`(+a0..c0: 최소 열/행·칸 크기·목록 mask·허용 커서·클릭 요구·고정 배치·항목 배율)와 `InventoryItemNode`(+a0..bc: 내용물 SID·미작성 DWORD·점멸 기한 double·단계/장식 BYTE·미작성 WORD·클릭 요구·초기 0 필드·아이콘 배율). 기본 Gump 저장소를 그대로 상속한다.
- `InventoryGump(create, factory, pool, finderState, state, hooks)`: 10.78 풀만 받는다. `state`는 현재 커서 전역(`cursorPoint`·`cursorFlags`)을 호출 때마다 읽는다.
  - `constructGrid`(43d790): 기본 생성 뒤 vtable·flags `0xa00`·최소 열/행·mask·클릭 요구 1을 쓰며 0열은 진단 뒤에도 계속한다. +a8/+ac는 쓰지 않는다.
  - `constructItem`(43e120)·`beginBlink`(43d690)·`idleItem`(43d6e0): 내용물 flags `0x200`만 점멸을 시작하고(단계 4·idle 등록·현재 시각+0.1), Idle은 단계 0이면 해제→표시→다시 그리기→내용물 점멸 비트 정리 순서이며 그 밖은 두 번의 시각 조회와 표시 전환 순서를 지킨다.
  - `attachGrid`(43e1e0): 자식 제거→기본 부착→두 번의 현재 목록 순회. 첫 순회는 mask에 맞는 slot BYTE+1의 최댓값이고 자동 배치는 열=`ceil(배정도 비율×float32 sqrt)`·행=몫(+1 보정)·최소값 보정, 칸 크기는 배정도 나눗셈을 float32로 저장한다. 두 번째 순회는 현재 목록을 다시 읽어 실제 내용물 SID로 항목을 만들고 가상 부착한다.
  - `handleGrid`(43d850)·`parentGone`(43d9c0): 내용물 통지(재부착은 부모 전달 반환값을 무시)·커서 질의·mask 질의·전달·왼쪽 취소·기본 처리, 삭제 통지의 진단→Push→연결 해제→재부착.

## 기준선 뒤 판정 (3.6절)

`1f00a25a` 뒤 `InventoryGump.cpp/.h`·fixture·cpppj 시험에 대한 변경이 없다(base `1f00a25a`).

## 검증

- 원본 10.78 독립 관측 **격자 170행**(`inventory-grid-x86.tsv`)과 **점멸 110행**(`inventory-blink-x86.tsv`)의 격자/항목 전체 노드 바이트(0xc4/0xc0)·생성 항목·외부 호출 순서와 반환·점멸 단계/Idle 500칸 표/시각 조회 횟수/내용물 비트/다시 그리기 사각형이 **첫 실행에서 모두 일치**했다. 입력 열만 읽고 관측 열은 비교에만 썼으며(순회 경계 `Next`/`Begin,50`은 비교 제외) 기존 fixture를 수정하거나 다시 실행하지 않았다. manifest `inventory-grid`·`inventory-blink`로 연결했다.
- 공통 트리/수명 통합 1건(cpppj `InventoryGump_SharedTreeRebuildAndLifetime`): 같은 등록표에 격자와 항목을 두 번 부착하고 이전 자식의 지연 삭제와 정리까지 확인한다. 독립 시험 6건: 노드 기본값·생성 검사·저장소 없음, 0열 진단·SID 범위·점멸 비트(0x200만), 부모 없음·고정 배치 0열/행·할당 실패·항목 배율 전달, 열/행 보정(1열/1행 최소값), float32 sqrt가 갈리는 열 수·음수 열의 0 쪽 절삭 위치, 점멸 종료의 다시 그리기→내용물 비트 정리 순서.
- 변형 확인: 자동 변형 120개 중 원본 관측만으로 생존이 37개(노드 기본값·풀 범위 경계·고정 배치 0열/행·열/행 최소값 보정·항목 배율 등 관측 입력이 닿지 않는 경로)였고 독립 시험으로 모두 잡았다. 남은 동등 변형은 `command` 인자 0→1(바로 뒤 덮어씀)·flags `0xa00/0x1a00`의 bit0(생성이 이미 켬)·행/열 보정 `<`↔`<=`(같으면 대입이 같음)·release 마스크 bit0(코드가 0x10000이면 bit0이 있을 수 없음) 네 종류다. 수동 의미 변형 15개 중 `count` float32 변환(최댓값은 정수라 정확)과 행 몫의 `Math.round`(뒤 보정 때문에 결과 동일)는 동등이고 나머지는 모두 잡혔다.
- 첫 실행에서 어긋난 곳은 없었다(관측 280행·통합 모두 일치). 모듈을 고치지 않았고 독립 시험의 첫 실패는 없었다.

## 남은 것

`InventoryItemGump`(항목 아이콘 자식 부착·Draw·클릭·비용, fixture `inventory-item-x86.tsv` 132행)와 `InventoryImage`(`inventory-image-x86.tsv`)·`RawProductionControls` 연결은 후속 단위다.
