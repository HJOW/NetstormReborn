# webpj raw 풀 연결 이웃 탐색(RawSquidNeighbors) 이식 — WB04-E39

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 서른아홉 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawSquidNeighbors`(10.78; 원본 004b23e0/004b1d70/004b1e80/004b1e70)를 옮겼다. `RawGraph`(Flood/재구성)와 다리/울타리/공급 소비자가 모두 이 이웃 탐색을 쓰므로 [E12](webpj-graph-20261108.md)의 정수 Graph와 raw 풀 사이의 다음 의존이다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

`www/core/raw-squid-neighbors.js` — `RawSquidNeighbors`, `RawSquidNeighborWalk`, `NEIGHBOR_FLAG`

- `RawSquidNeighbors(pool, hash, spots, types, frames)`: 풀·해시·spot은 조회 때마다 다시 읽는다(복사 없음). `types`는 `{flags1, flags2, footX, footY}`, `frames`는 타입 번호별 `{side, variant, number, flags}` 배열. `bindCurrentTypes`로 실제 World의 188 TYPE을 빌릴 수 있다.
  - `source(sid)`: 삭제 호출 전 값으로 포착하는 `{x, y, type, frame}`. `readObject`/`footprint`/`center`: float 발자국 사각형(왼쪽/위는 1..255 보정, 원래 두 좌표가 (0,256) 안일 때만 이전 점 보존)과 단정도로 저장한 중심.
  - `static direction`: 세로 차이는 단정도 저장값, 가로 차이는 배정도, **크기가 같거나 NaN이면 세로**(0 북 / 2 동 / 4 남 / 6 서). `interior`: 발자국 spot 비트 8의 AND.
  - `accept` [004b1e80]: 표면 타입(`anyType`이 아니면) → 가로/세로 확장 교차의 XOR(flag 8은 생략) → 중심 방향 접합(`bridgeConnects`) → dead → abstract(`skipAbstract`) → 후보 기준점의 spot(좌표 + 0.9999f를 0 쪽으로 절삭한 칸, 배열 밖만 거부)이 내부가 아닐 것.
  - `first(sid)`: 연결 이웃이 없으면 0.
- `RawSquidNeighborWalk(neighbors, sidOrSource, flags, lazyFrame)`: 원천(발자국·타입·프레임·중심)은 생성 때 한 번 읽고 후보 필드·해시 체인은 `next()`마다 다시 읽는다(순회 도중 객체 등록/삭제가 뒤 결과에 반영). 내부 발자국 원천은 시작하지 않는다(`current === 0`). flags: 1 프레임 접합 생략·2 비표면 후보 허용·4 abstract 제외·8 표면 지도 y/x 순회(004b1c20: 지도 밖 셀 건너뜀, 자기 발자국 안 기준점 제외, **signed short 중복 캐시**), 그 밖은 `RangeError`. flag 8이 아니면 기존 [`RawSquidFinder`](webpj-squid-finder-20261105.md)의 필터 인자(`accept`)로 일반 해시 단계/y/x/next 순서를 순회.

## 2. 검증 (`tests/squid-neighbors.test.mjs`, 4건)

원본 10.78 관측(`cpppj/tests/fixtures`를 직접 읽음):

- `surface-x86.tsv`의 `Neighbor` 125장면: 장면마다 원문 발자국 전체를 지도에 놓고(뒤 객체가 앞 칸을 덮음) spot/지도 덮어쓰기를 입력 순서대로 적용해 flag 8 순회의 번호 순서·중복 제외를 대조(cpppj는 두 판본 250 = 125×2, 여기서는 10.78 125).
- `surface-neighbor-current-x86.tsv` 중 10.78 16행(`originalCD` 16행 제외): 첫 반환 뒤 현재 지도 셀·후보 상태(dead/buried)·원천 좌표를 바꾸는 순회(원천 발자국/코드는 생성 시점, 나머지는 Next 시점).
- `neighbor-boundary-x86.tsv` 18행: 첫 다리 취득의 (0,0)을 포함한 유한 경계의 원본 발자국 사각형 16바이트(float 비트)와 첫 이웃.
- 계약: 지원하지 않는 flags 거부, 이웃 없음 → 0, 내부 발자국 원천 비시작, 방향 동률/NaN, 크기 불일치 생성 거부, 범위 밖 SID/자산 아닌 원천 타입 거부.

최초 실패와 교정: 4건이 처음부터 통과했다.

## 3. 경계

- 일반 해시 순회(flag 0/4)의 **다리 이벤트 장면**(`neighbor-x86.tsv`의 `Event` 행: 다리 이벤트 핸들러가 이웃 탐색을 소비)과 `neighbor-x86.tsv`의 `First` 행은 다리 이벤트(`RawBridgeEvents`/`RawBridgeConnect`) 단위에서 대조한다. 울타리 이웃(`fence-neighbors-x86.tsv` 1904행)은 울타리 단위에서 한다.
- 프레임 코드 표(`RiftTypeFrames`)는 웹 `TypeTable`에 아직 코드 배열이 없어 소비자가 `frames[type]` 배열을 공급한다. `bindCurrentTypes`의 실제 World 연결과 CD 10.72는 범위 밖이다.

## 4. 다음 시작점

이제 `RawGraph`(월드 Graph 연결: `Calculate`/`Region`/`DetachConnections`와 `RawGraphLive`의 `ExecuteFlood`/`ExecuteAdd`/`ExecuteDetach`/`ExecuteRebuild`)를 옮길 수 있다(`rawgraph-x86.tsv` 1796행, `regiongraph-x86.tsv` 1926행, `graph-reset`/`graphforms`/live-checking fixture 등). 그 뒤 `SquidPostPop` 처리기 본체, `RawBridgeConnect`, Player 나머지이다.
