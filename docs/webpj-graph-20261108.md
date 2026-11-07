# webpj 표면 이웃 탐색(SurfaceFinder)과 Graph 정수 연산 이식 — WB04-E12

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 열두 번째 부분이다. 공통 PostPop이 부르는 `RawGraph`/`RawGraphLive`의 바닥 계층인 cpppj 기준선 `1f00a25a`의 `o/SquidFinder`(flag 8 표면 이웃 탐색), `o/Graph`(정수 표면 연결), 그리고 `o/Bridge`의 `Connects`(프레임/방향 접합 판정)를 옮겼다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

### `www/core/surface-finder.js`

- **`bridgeConnects`/`SurfaceFinder.connects`**: 두 발자국 중심 차(`|dx|<=|dy|`이면 세로 우선)로 방향 0/2/4/6을 고르고, 짝수 방향은 프레임 `side`, 홀수는 `variant`를 연결 코드로 쓴다. flags2의 받침(0x40000)이면 `'P'`, 다리/폭탄(0x104)과 지상 물체(0x1044202) 조합이면 `'A'`로 보정한 뒤 `terrain-mask.js`의 `connectionMask`로 첫 마스크가 0이면 둘째를 조회하지 않고 거짓을 돌려준다.
- **`SurfaceFinder`**: 256×256 객체 번호 지도와 spot을 복사해 받는다. `neighbors(id)`는 발자국 전체 AND가 내부(spot bit 8)이면 빈 목록이고, 아니면 발자국을 한 칸 넓힌 사각형을 y/x 순서로 훑으며 위·아래 행의 두 모서리를 제외한다. 자기 발자국 안 기준점·표면이 아닌 것·죽은 것·기준점이 내부인 것·접합하지 않는 것을 거르고 중복 없이 반환한다. `originInterior(id)`는 기준점 한 칸만 본다(전역 재구성용).

### `www/core/graph.js`

- 표는 255행(정상 0..250, 무효 254의 sentinel `0x7dfd`, 251..253 틈), 각 행은 WORD 셋(`surfaces`·`inUse`·`reserved`), flood 스택은 4096 DWORD이며 사용 후 흔적도 보존한다.
- **`add`**: 활성 표면 이웃의 그래프 번호를 탐색 순서로 중복 없이 모아 가장 큰 그래프(동률은 첫 번째)에 붙이고 나머지는 flood로 병합(패배 레코드의 reserved 보존). 이웃이 없으면 새 번호를 확보해 flood한다.
- **`flood`**: 현재 객체 번호를 먼저 바꾸고 y/x 순서 이웃을 push하며 마지막 이웃부터 방문한다. 이미 목적 번호인 이웃은 확장하지 않고 별도 visited 표 없이 중복 push를 허용한다. WORD 증감은 16비트로 감긴다.
- **`detach`/`detachAt`**: 죽은 원천의 이웃 무리를 탐색 순서로 분리하고 마지막 무리는 기존 번호에 남긴다(특수 flags2 bit 8이면 남기지 않음). `removedSurfaces`는 1 또는 9, `rebuild`는 분할 정책, `recovery`는 번호 소진 복구 정책이다.
- **`rebuild`**: resetAll이면 앞 251행의 두 WORD만 지우고 0번 예약·reserved low byte·sentinel을 복원한 뒤, 임시 번호로 모은 표면(기준점 내부·free 제외)을 SID 오름차순으로 flood해 독립 무리마다 번호를 배정한다. 임시 번호는 마지막에 해제한다.
- 공개 연산은 모두 **복사본에서 실행한 뒤 성공했을 때만 반영**한다(`allocate`/`free`/`remove`는 쓰기 전에 거부하므로 직접 수행). 오류 시 표·번호·스택이 그대로 남는다.
- `removeGraphSurface`·`checkGraphSentinel`은 원문 진단(`inUse`·`numSurfaces > 0`·sentinel `0x7DFD`, 줄 0x59/0x5a/0x68/0x69)을 현재 checking DWORD를 진단 뒤 다시 읽어 요청한다(RawGraph가 실제 행에 쓰는 부분).

## 2. 검증 (`tests/graph.test.mjs`, 7건)

| 검사 | 근거 | cpppj 대응 |
| --- | --- | --- |
| Add·Flood·Allocate·Free·Remove 각 512행씩(10.78 구간 2개, 모두 1280개 검사 지점): 표 255행 해시, 스택 4096 DWORD 해시, 객체별 번호/상태 | `graph-x86.tsv`의 `originals` 구간(`originalCD` 두 구간 1280개는 10.72라 제외) | Graph_X86_AllocationFloodMergeAndPreservedStorage |
| 크기 동률·병합, 여러 칸 발자국·WORD 감김, 잘못된 입력/소진 거부, 삭제 분리와 원자적 거부, 독립 무리 249개 전체 재구성과 초과 시 롤백, 내부 표면 번호 보존과 소속 밖 flood 거부 | (독립 입력) | Graph_ 나머지 6건 |

최초 실패와 교정: 구현 직후 7건이 처음부터 통과했다. 원본 관측 기대값은 원문 열만 썼다. Setup 행의 객체 필드는 11개이므로 cpppj 검사의 `buried`(v[11])는 존재하지 않는 칸이라 0으로 취급했다(Graph/탐색은 buried를 쓰지 않는다).

## 3. 경계

- `Bridge.cpp`의 나머지(`CollectDecay` 등 다리 붕괴 탐색)는 이 단위에 없다. PORT_MAP의 `Bridge.cpp`는 `Connects`만 옮긴 `partial`이다.
- `RawGraph`(SID 풀·4단계 해시·spot 연결)와 `RawGraphLive`가 이 계층 위에 실제 슬롯을 붙인다. 다음 단위다.
- CD 10.72 경로(sentinel 진단 없음, 번호 규칙 동일)는 범위 밖이다.

## 4. 다음 시작점

`RawGraph`/`RawGraphLive`, 공통 `SquidPostPop` 본체, `RawSquidPoolReset`, 그리고 Player 나머지(`RawPlayerWorld`·`RawPlayerTechnology`·`RawPlayerRegular`·`RawPlayerTemplateState` 등).
