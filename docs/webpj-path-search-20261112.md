# webpj 경로 탐색 RawPathSearch (WB04-E334, 2026-11-12, 6-P 경로 부품)

3.1절 6-P 첫 항목 "RawPathProcess 362줄(+10)과 RawPathSearch partial 240줄" 중 **RawPathSearch**(기준선 `.h` 81 + `.cpp` 157 = 238줄, 기준선 뒤 변경 없음)를 `core/raw-path-search.js` 로 옮겼다. 두 파일은 서로 의존하므로 한 항목이지만, 크기를 나누어 이번에는 탐색 본체를 먼저 커밋하고 `MakePathSearchHooks`(RawPathProcess 훅 묶음이 필요)와 경로 프로세스 본체는 다음 단위(E335)로 둔다. 기존 `core/raw-path-surface.js` 의 `rawPathSurfaceAt`/`pathCell` 을 재사용한다.

## 1. 규칙

- **RawPathSearchState:** Path 가 직접 읽고 바꾸는 전역이다. `graphsEnabled`(00540414, Graph 조회 허용)·`searchEnabled`(00540be4, 원본 kenGo, 꺼져 있으면 진단)·`avoidFlagTwo`(00540be8)·`policySelector`(005318ec 초기 6)·`directionPrerequisite`(`Uint32Array` `[0,5,0,20,0,80,0,65]`, 대각선은 양옆 직선 방향 허용 비트가 필요)·`directionOrder`(`Int32Array` `[0,2,4,6,1,3,5,7]`)·`fenceMarkType` 90·`windArcherType` 140·`sharedPolicySelector`(실제 Client 의 `{ value }` 상자, null 이면 독립 입력).
- **RawPathSearchWork/Storage:** 거리표는 256×256 `Int16Array`, 큐는 16,384항목(X·Y 두 BYTE) `Uint8Array(32768)` 이다. `RawPathSearchStorage`(Client 수명 소유자 59514c)는 `work`(처음 null, 첫 검색기가 만들어 등록)와 `initialized`(실제 거리표 쓰기에 도달한 **뒤에만** true)를 가지며, 같은 소유자를 받은 검색기들은 같은 배열을 공유한다.
- **RawPathSearch:** `surfaceAt(position)` 은 0.9999 보정 칸의 해시 0단계 머리 SID(보드 밖 0), `graphAt(position)` 은 Graph 비허용·표면 0·Graph 번호 254 에서 null(번호 0 은 유효), 표면 타입이 아니고 울타리 표식/바람 궁수 번호도 아니면 진단, `connects(first, second, direction)` 은 0 이 있으면 false·같은 SID 는 프레임 조회 없이 true·그 밖은 현재 물리 프레임(슬롯 +36 의 signed DWORD)과 타입 flags2 로 `bridgeConnects` 를 부른다.
- **searchCells(output, sx, sy, dx, dy, cardinalOnly) (00488370):** 보드 밖이면 거부, 거리표만 매 호출 전부 지우고(미사용 큐 바이트는 유지) 정책 선택값이 5 면 방향 3 의 선행 비트를 5 로 바꾼다. 북/동/남/서 BFS 는 `head` 는 감기지 않고(16,384번째 읽기에서 진단) `tail` 만 0x3fff 로 감는다. 목적에 닿으면 기존 경로를 보존한 채 거리 `remaining`(또는 대각선 허용 시 `remaining-1`) 후보를 비용(직진 10·방향 전환 15·대각선 −10·회피 flag 2 의 +30)으로 비교해 되짚고, 방향이 바뀔 때 같은 좌표에 이전/새 방향 꼭짓점 둘을 추가해 `setPath` 로 쓴다. 출력은 시작→목적 순서이다(BFS 는 목적에서 시작한다).
- **search(output, parent, start, destination, flags) (00488af0):** 부모 타입이 표 밖이면 진단, 목적 Graph 가 없거나 시작과 다르면 false, 칸이 보드 밖이거나 시작 칸이 걸을 수 없으면 false(목적 칸 spot 은 보지 않음), kenGo 꺼짐이면 진단, 그 밖은 `searchCells(…, (flags & 1) !== 0)`.

## 2. 웹 범위와 기준선 뒤 변경

- 10.72(CD) 판본 분기는 옮기지 않았다(웹은 10.78 만). 기준선 뒤 추가 줄이 없다(3.6절 ①).
- SID 는 숫자, 위치는 `{ x, y }`(float32 값), 타입 표는 `{ flags1, flags2 }`, 프레임 표는 타입별 프레임 코드 `{ side, variant, number, flags }` 배열이다(`raw-graph.js` 와 같은 형식). 슬롯은 `pool.physical(sid)` 의 읽기 전용 보기로 읽는다(`slot()` 의 사본 생성을 BFS 칸마다 반복하지 않으려는 웹 어댑트이며 동작은 같다).
- `MakePathSearchHooks`(RawPathProcessHooks.search 에 탐색기를 연결하는 어댑터)는 훅 묶음을 정의하는 RawPathProcess 단위(E335)에서 옮긴다. 따라서 PORT_MAP 의 RawPathSearch 행은 `partial` 이다.
- 이 모듈은 DOM 을 쓰지 않는다. 앱 수명 연결(Client 가 `RawPathSearchStorage` 를 한 번 만들고 월드가 검색기를 만들어 경로 프로세스에 끼우는 일)은 4-D/6 단계 조립 몫이다.

## 3. 검사 (`tests/path-search.test.mjs` 25건)

- **원본 독립 관측:** `pathsearch-x86.tsv` 의 `originals` 322행(profile 0~9 의 섬 장면 × flags·avoid·policy·owner·legacy·guard): 결과(성공/거절/진단)·꼭짓점 좌표 float 비트·방향·QWORD 시각·거리표 131,072바이트 FNV32·큐 32,768바이트 FNV32·방향 3 선행 비트를 모두 대조. **첫 실행에서 일치했다.** `originalCD` 322행은 10.72 범위 밖이라 사용하지 않는다. manifest 에 `pathsearch` 항목을 추가했다(소비자 `path-search.test.mjs`).
- **cpppj 시험 이식:** `rawPathSearch_current_surface_alias`(해시 변경 즉시 반영, 사본 없음), `rawPathSearch_shared_diagnostic_current_value_and_edition`(공유 진단값을 매 호출 다시 읽음, 웹은 10.78 판만). 나머지 둘(`init_uses_current_surface_before_search`·`kernel_moves_and_arrives…`)과 `native_initialization_pipeline`(pathinit fixture 32행)은 RawPathProcess/SquidPop/Kernel 이 필요해 E335 에서 옮긴다.
- **웹 계약:** 생성자 연결 검사, 공유 소유자의 지연 생성과 `initialized` 시점, 큐 넘침(head 고정·tail 감김), 경로 길이 5000 경계(4998 통과·4999 에서 진단)와 출력 이미지 크기 진단, Graph 번호 0 유효·254 없음, 표면 타입 예외 번호, 부모 타입 진단, flags 비트 0 만 cardinalOnly, 여덟 방향(대각선 포함) 첫 방향·끝점, 비대칭 시작/목적(X/Y 구별), 같은 방향 직진 선호, 회피 flag(`&6` 이 정확히 2 일 때만), spot 비트(`&6` 걷기·`&8` 섬 안쪽 차단), 직선 방향 선행 비트와 누적, 역추적 후보 없음 진단, 시작 표면 0, 큐 소진 false(빈 큐를 더 읽지 않음), 허용 지면의 표면 0 진단, 방향 전역 범위 진단(BFS 앞 네 항목·역추적 여덟 항목), 프레임 범위·같은 SID 접합, 상태 기본값.
- 첫 실행의 실패 1건은 시험 쪽 기대값 오류였다: 독립 정책 입력에 `0x100000005` 를 주면 C++ `int32` 대입처럼 5 로 줄어든다(기대값 20 이 아니라 5).

## 4. 변형 확인과 남은 경계

- 한 문자열 치환 변형 146개를 돌려 첫 실행에서 39개가 남았다(여덟 방향 중 북서·대각선 좌표, 비대칭 X/Y, 칸 spot 의 `&8`, 선행 비트 누적, 직진 선호, 회피 flag 의 마스크, 보드 Y 경계, 시작 칸 walkable, 진단 메시지 등). 시험을 보강한 뒤 **11개가 남았고 모두 동등하거나 도달 불가**이다.
  1. 거리 증가 `increment` 의 signed WORD 감김, 거리표 `Int16Array` → `Uint16Array`: 큐가 16,384항목에서 막히므로 거리는 16,385 를 넘지 못해 32,767 감김에 도달할 수 없다.
  2. 프레임 읽기의 `>>> 0`: 음수 프레임이 큰 양수가 되어도 표 밖이라 같은 `RangeError` 이다.
  3. 공유 진단값의 `>>> 0` 대 `| 0`: `=== 5` 비교에서 같은 결과이다.
  4. `head === layerEnd` 대 `>=`: head 는 한 번에 1씩 늘고 layerEnd 는 head 이상이라 같다(감긴 tail 로 layerEnd 가 작아지는 경우는 16,384번째 읽기 진단 뒤에만 가능).
  5. 역추적의 `!cardinalOnly &&`: 직선 이웃의 거리는 ±1 이라 `remaining-1` 후보가 생길 수 없다.
  6. `bestDirection` 초기값: 후보가 없으면 `bestCost` 진단이 먼저 난다.
  7. 역추적의 표면 0 진단·시작 불일치 진단: 큐에 들어간 칸은 표면이 있고 거리 1 칸은 시작 하나뿐이라 도달할 수 없는 방어선이다.
  8. `search` 의 `inside(sx, sy)`·`inside(dx, dy)`: 보드 밖 위치는 `graphAt` 이 null 이라 그 앞에서 이미 false 이다.
- `searchCells` 가 BFS 도중 진단을 던지면 거리표·큐가 일부만 채워진 채 남는다(원본과 같음, 출력 경로는 바꾸지 않음).
- 3.1절 6-P 는 이 단위로 **RawPathSearch 쪽이 끝났고** RawPathProcess(362줄(+10))가 남았다. 체크 항목은 두 파일이 모두 끝나는 E335 에서 닫는다.
