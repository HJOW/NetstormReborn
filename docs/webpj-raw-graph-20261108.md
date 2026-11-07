# webpj 월드 Graph 연결(RawGraph/RawGraphLive) 이식 — WB04-E40

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 마흔 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawGraph`(10.78; 원본 00462b20/00462d40/00462f10/004633c0/004637b0/00463110 계열)와 `o/RawGraphLive`(실제 부모 실행)를 옮겼다. [E12](webpj-graph-20261108.md)의 정수 Graph와 [E39](webpj-squid-neighbors-20261108.md)의 이웃 탐색, [SID 풀](webpj-sid-pool-20261104.md)·해시를 실제 월드 풀에 연결하는 마지막 부품이다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다. 마지막 디컴파일 PC는 기존 기록(HJOW-Athlon)을 유지한다.

## 1. 옮긴 것

`www/core/raw-graph.js` — `RawGraph`, `RawGraphStorage`, `resetRawGraphTable`
`www/core/raw-path-surface.js` — `rawPathSurfaceAt`, `pathCell`(RawPathSearch의 위치→표면 조회만 먼저)

- **장기 물리** `RawGraphStorage`: 기록 255행(`{surfaces,inUse,reserved}` 부호 있는 WORD, reserved와 틈 3행 포함) + flood 스택 4096 DWORD. `resetRawGraphTable`은 원본 00462b20: 앞 251행의 두 WORD만 지우고 0번 예약·reserved 하위 BYTE=1·sentinel(254=0x7dfd)만 쓴다.
- **생성** `new RawGraph(pool, hash, spots, types, frames, {storage, records, stack, recovery})`: `storage`를 주면 장기 물리를 생성 효과 없이 빌리고, 아니면 `records`/`stack` 입력(없으면 sentinel만 가진 빈 표)으로 독립 사본을 만든다. 타입(188)·프레임 코드 표는 값으로 복사해 호출자 수명과 분리한다. `bindCurrentTypes`로 실제 월드의 현재 TYPE을 빌릴 수 있다.
- **사전 계산** `#calculate(sid, 연산, ...)`: 풀·해시 0단계·spot을 복사해 정수 `Graph`(E12)에 같은 연산을 미리 돌린다. 연산은 Add·Flood·Region·RegionAdd·Detach·Rebuild·Allocate·Inspect. 손상/소진/잘못된 프레임·타입을 **공간을 쓰기 전에** 거부한다(`validateAdd`/`validatePostPop`/`validateDetach`/`validatePostDestroy`). `pop`(예정 좌표·단계·타입)을 주면 Pop 뒤 지도·spot(genus OR)을 예측하고, 비표면의 소수 위치는 영역 칸만 절삭한다.
- **영역 통지** `invalidateRegion`/`#region`: 4단계 해시를 y 바깥/x 안쪽으로 훑고 단계별 끝 버킷을 한 칸 넓혀(표면 이웃 탐색과 다른 경로) 발자국과 겹치는 내부 spot 표면의 번호를 254로 무효화한다. 실제 실행은 진단 복귀 뒤 현재 슬롯·기록을 직접 읽고 쓴다.
- **삭제 연결** `#detachConnections`: 원천을 한 칸 넓힌 일반 해시 순회, 가로/세로 경계 접촉 교차 XOR, `Bridge::Connects`의 두 번째 코드 보정(원천 프레임이 무효일 때만 거부). `sourceType` 치환 정보는 삭제 위치 정보만 바꾼다.
- **실제 실행(RawGraphLive)** — 모두 현재 풀·기록·스택을 직접 소비하고, 진단(assertion)·로그(trace)·시계가 돌려준 뒤의 값을 매번 다시 읽는다:
  - `flood`: 스택 0 번째에 시작 SID, `RawSquidNeighborWalk`(flag 8)의 후보를 LIFO로 쌓는 중복 push 허용 flood. 그래프 번호 읽기/쓰기는 PATCH의 타입 진단(0x739/0x74b/0x750), 자기 후보 진단 0x176, 스택 오버플로 진단 0x17a, 로그 `N graphs in use`, 앞뒤 sentinel 검사.
  - `rebuild`/`allocate`: 소진 판정 → 사전 계산 → (resetAll) Reset → 임시 번호 확보 → SID 2부터 두 순회(free만 건너뜀, void/dead/contained는 제외하지 않음) → 임시 행 해제 → 마지막 sentinel. 소진 시 전체 재구성 뒤 번호를 확보하고, 그래도 없으면 진단 0x25d.
  - `add`: 위치/상태 진단(0xbd/0xc2), 이웃의 **현재** 번호로 가장 큰 그래프(초기값 0x80000001, 동률은 첫 연결) 선택, 새 번호 flood(0xf3), 이긴 행 inUse 진단(0x100), 진 그래프 병합(0x113), 병합 수 진단(0x11f).
  - `detach`: 활성/분할 전역을 현재값으로 읽고(`bindDetachState`), 위치 머리 SID(`rawPathSurfaceAt`)의 graph로 원본 행을 정하며 일반 finder의 SID를 그래프 중복 제거 없이 최대 64개 저장(0x1ef), 두 시계 읽기 사이에서 각 연결의 현재 번호를 다시 읽어 분할/마지막 연결 유지(`remaining<2` 및 원천 TYPE flags2 비트 8)/감소(1 또는 9)한다. 연결 목록 0 항목은 0x20b/0x93.
  - `postDestroy`: 먼저 풀 복사본(`GraphPoolSnapshot`)에서 순차 Add를 시험하고(원본 풀·기록·스택은 쓰지 않음), 현재 일반 finder가 돌려주는 표면마다 실제 Add를 소비한다.
  - `free`/`validateFree`/`setNumber`/`frame`/`validateSpace`/`validateTypes`/`resetTable`/`allocate`.
- 진단은 `{checking:()=>DWORD, assertion:(식,파일,줄)}`로 주입하며 필요한 진단 지점에서 장치가 없으면 `Error('… 진단 미연결')`로 멈춘다(그 앞에서 이미 쓴 효과는 되돌리지 않음).

## 2. 검증 (`tests/raw-graph.test.mjs`, 24건)

원본 10.78 관측(`cpppj/tests/fixtures`를 직접 읽음, originalCD 행 제외)을 모두 재생한다.

| fixture | 재생 행 | 확인 |
| --- | --- | --- |
| graph-reset | 16 | 1530 BYTE 입력의 Reset 후 기록 전체와 풀·스택 보존 |
| graph-live-checking / graph-live-free-callback | 72 / 12 | Add·AddZero·AddVoid·Flood·Rebuild0/1의 진단·로그 6종 변경 후 전체 기록·4096 DWORD 스택·6 슬롯·checking·이벤트 순서 |
| graph-detach-live / graph-region-free | 216 / 4 | 삭제의 정책/시계/로그/진단 변경(12 장면×18), 영역 Remove 중 free/재형 슬롯의 같은 graph BYTE 쓰기 |
| graph-reference-checking | 360(+Zero 20 제외) | `removeGraphSurface`의 signed WORD·전체 checking·식/파일/행 순서 |
| graph-sentinel-checking | 540 | sentinel 진단 복귀, 직접 Free의 현재 기록 쓰기, 무효 254 자연 반환 |
| graphforms | 32 | 전체 풀 재구성이 내부 Form 6개를 생략하고 NaN/stale 필드를 보존 |
| graphlookup / graphrebuild | 128 / 96 | 위치 머리 번호·원천 프레임(Detach), 무효 수집·전역 재구성·소진 할당; 슬롯 7개·기록/스택/해시4단계/spot/풀 Adler·통계표·표시 목록 |

cpppj 계약 테스트 중 Pop 장치 없이 성립하는 것을 옮겼다: 빠른 Allocate가 무관한 프레임을 읽지 않음, 전체 재구성 실패의 무변경, 위치 머리 번호의 조기 반환, 삭제 사전 계산의 거부(소진·손상 머리·순환 next), 정상 사전 계산(projected)의 무변경, 다른 void 표면의 손상 프레임/타입 검출, 비표면 소수 위치 영역, 대여 vs 독립 snapshot 소유, postDestroy 임시 사본 분리, 영역 통지가 진단 중 고친 행/다음 후보를 읽음, 진단 장치 누락의 쓰기 경계.

최초 실패와 교정: 모듈은 첫 실행에서 24건 중 19건이 통과했고 모두 **테스트 기대값/보조 쪽**이었다. graphlookup/graphrebuild 행 수는 총 행의 1/3(128/96)이며(내가 추정한 192/144이 틀림), `RawSquidFinder`가 풀의 `edition`을 요구해 복사 풀에 판본 getter를 추가했다. 기록 행을 테스트가 객체째 교체하면 실행기의 캐시된 참조가 어긋나므로 실행기는 기록 행을 호출마다 인덱스로 다시 읽고, 테스트는 제자리 변경(`Object.assign`)으로 진단 콜백을 흉내 낸다.

## 3. 경계

- Pop/PostPop/Unpop/Release와 `SquidDisplay`(dirty 목록)를 함께 재생하는 fixture(rawgraph 1796행·regiongraph 1926행·graphremove 1549행·graphrecovery 391행, 각각 `SquidPostPop` 장부/깊이·통계표와 표시 목록 확인)는 다음 단위 `SquidPostPop` 처리기 본체와 같이 대조한다. 이 단위의 `postPop`/`validatePostPop(pop)` 몸체는 위 계약·graph-reference-checking 영역 장면으로만 확인했다.
- 사전 계획을 풀에 반영하는 원본 `Commit`은 실제 실행이 쓰지 않으므로 옮기지 않았다. CD 10.72(타입 171·graph 28·frame 1바이트)는 범위 밖이다.
- graph-reference-checking의 Zero 20행은 `RawPlayerPlacementAnchor.GraphFor(0)`을 요구하므로 Player 나머지 단위에서 대조한다.
- `RawPathSearch`는 `rawPathSurfaceAt`만 옮겼다. BFS 길 찾기 본체는 아직이다.

## 4. 다음 시작점

`SquidPostPop` 처리기 본체(Graph·Player 통지·마나 등록·종류별 분배; E24가 남긴 region/add/resetGraph 훅에 `RawGraph.postPop`/`setNumber`를 연결)와 그것을 쓰는 rawgraph/regiongraph/graphremove/graphrecovery fixture 대조. 그 뒤 `RawBridgeConnect`(다리 이벤트 이웃), Player 나머지, 종류별 PostPop, `SquidDisplay`/`SquidFrame`이다.
