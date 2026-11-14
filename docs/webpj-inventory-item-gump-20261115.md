# webpj 생산 항목 InventoryItemGump (WB04-E473, 2026-11-15)

기준선 `1f00a25a`의 `client/InventoryItemGump.cpp/.h`(188줄)를 `www/ui/inventory-item-gump.js`로 옮겼다. E472의 `InventoryGump`가 만든 항목(`InventoryItemNode`, 가상 표 `5068f4`)의 아이콘 자식 부착(`43ce40`)·Draw(`43cdc0`)·입력(`43d4c0`)·비용 판정(`4af010`)이다. 실제 자산에 연결하는 `BindInventoryItemAssets`는 `PriestPlacementAssets`(todo)가 필요해 이번에는 옮기지 않았고 PORT_MAP 상태는 **partial**이다.

## 이식한 것

- `InventoryItemGump(pool, state, hooks)`: 10.78 내용물 풀만 받는다. `state`는 원본 현재 전역(`localOwner`·`editor`·`stockEnabled`·`inputDisabled`·`lastType`(쓰기 가능)·`bridgeType`·`pixelsX/Y`·`backgroundType/Frame`)의 live 객체다.
- `attach`: dead 진단 네 곳(기본 부착·자식 정리·삭제 큐 정리 사이마다 현재 플래그 재확인)과 `isSidRobust` 진단 뒤 내용물 종류/인자/명시 프레임(`gumpFrame`이 −1이 아니면 인자를 대체)을 읽고, 종류 0이면 자식 없이 끝낸다. 아이콘 경계가 노드 크기를 넘고 항목 배율이 0이면 큰 쪽 비율의 역수(비율은 배정도 계산→float32 저장, 역수도 float32)로 줄이고, 중심은 `(노드 크기 − 경계×배율)×0.5+2`를 배정도로 계산해 float32로 저장한다. 패턴 칸마다: 다리 TYPE이면 프레임 코드의 부호 확장 플래그에 품질(1→`0x20`, 3→`0x40`)을 더해 프레임을 다시 찾고, flags `0x100`은 모드 1, flags `0x418`이 있고 살 수 없으면 모드 2로 ImageGump 자식(0xb4)을 만들어 배율/크기를 현재 기본 크기에서 정해 부착하며, 재고 표시가 켜져 있고 flags `8`이면 `~w~E<품질>` TextGump 자식(0xa4)을 맞춰 그림 오른쪽 아래(`px + 폭`, `py + 높이 − 10`)에 붙인다. 내용물 raw는 호출 중 바뀔 수 있어 live 슬롯(`physical`)을 읽는다.
- `draw`: 장식 비트가 있을 때만 현재 화면점+3에서 시작해 기본 크기+화면점을 CRT 절삭한 끝점으로 질감 장식을 그린다.
- `handle`: 로컬 좌표를 먼저 구하고 지식/입력 관문을 읽으며, 반복 입력은 마지막 선택과 같은 종류일 때만 선택을 전달한다. 좌/우클릭 누름만 처리하며 현재 자식 목록에서(그림 hit 요구가 있으면 hit가 성공한 첫 그림 자식) 좌클릭은 정책이 허용하면 마지막 선택을 갱신하고 선택을 전달, 거부면 `NoBridgeYet`, 우클릭은 부모 전달 0·지식 닫힘이면 정보 메뉴를 연다. 자식 순환은 거부한다.
- `affordable`: 비용표 값→float32, 현재 SP→float32 순으로 조회하며 에디터여도 SP 조회를 생략하지 않는다.

## 기준선 뒤 판정 (3.6절)

`1f00a25a` 뒤 `InventoryItemGump.cpp/.h`·fixture·cpppj 시험에 대한 변경이 없다(base `1f00a25a`).

## 검증

- 원본 10.78 독립 관측 **132행**(`inventory-item-x86.tsv`: Attach·Handle·Draw·Afford)의 항목 노드 0xc0 바이트·실제 생성한 그림(0xb4)/글자(0xa4) 자식 바이트·마지막 선택·반환·호출 순서가 **첫 실행에서 모두 일치**했다. 같은 풀/등록표와 실제 `ImageGump`·`TextGump` 생성자·`FrameCodeTable`을 쓰고, 입력 열(내용물 raw·노드 필드·장치 응답)만 읽고 관측 열은 비교에만 썼다. manifest `inventory-item`으로 연결했다. 실제 자산 경로 시험(cpppj `InventoryItemGump_ActualAssetsTreePixelsAndClick`: 실제 TYPE·패턴 측정·적중 버퍼)은 `PriestPlacementAssets` 이식 때 옮긴다.
- 독립 시험 5건: 생성/저장소/내용물 SID 범위와 비용 조회의 종류 WORD, 짝수 인자와 명시 프레임이 패턴 질의/측정에 그대로 전달, 다리 프레임(품질 1/3 재검색·부호 확장·범위 밖/표 없음), 아이콘 모드와 수량 글자 조건(낮은 비트만 있으면 모드 0), 정밀도(배율 비율의 float32 저장·위치/폭/수량 글자 x의 float32 저장 위치·부착 장치가 받는 숫자의 float32성·현재 SP의 float32 비교; 줄임 위치가 결과를 가르는 입력은 무작위 탐색으로 찾았다).
- 변형 확인: 자동 변형 110개 중 원본 관측만으로 생존이 21개(내용물 SID 범위·다리 프레임 범위 검사·품질 플래그 부호 확장·flags 낮은 비트·짝수 인자 등 관측 입력이 닿지 않는 경로)였으나 독립 시험으로 모두 잡았다. 남은 동등 변형은 `rx`/`ry` 경계 비교(같은 값에서 결과 동일) 세 곳과 release 마스크 bit0(코드에 bit0이 있으면 이미 거부) 한 곳이다. 수동 의미 변형 22개 중 `x×pixelsX×배율`의 결합 순서만 남았고, 16·11 배수와 무작위 float32 500만 건에서 가르는 입력을 찾지 못했다(동등으로 본다).
- 첫 실행에서 어긋난 곳은 없었다(관측 132행 일치). 독립 시험의 첫 실패는 부착 이벤트 번호(기본 부착이 앞에 오므로 그림 자식은 두 번째) 시험 기대값 실수 한 건뿐이다.

## 남은 것

`BindInventoryItemAssets`(실제 `GameAssets`의 TYPE `gumpFrame`·프레임 코드·`RawCanonPixelShape` 측정·`DecodeCanonType` 방문)와 그에 대한 실제 자산 시험은 `PriestPlacementAssets` 이식 뒤 연결한다. `inventory-image-x86.tsv`(ImageGump 222행)는 3.8절 이월 목록(이미 이식한 `ImageGump`의 원본 관측 연결)에서 처리한다.
