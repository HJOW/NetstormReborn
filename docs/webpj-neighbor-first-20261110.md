# webpj 일반 해시 첫 이웃·다리 끝 칸 변환 원본 대조(RawSquidNeighbors 마감) — WB04-E243

2026-11-10, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 단계 3-B의 첫 항목이다. [E39](webpj-squid-neighbors-20261108.md)가 옮긴 `RawSquidNeighbors`(원본 004b23e0/004b1d70/004b1e80/004b1e70)가 PORT_MAP에서 `partial`로 남긴 두 경계를 닫았다. 새 모듈은 없고 검사 하나(`tests/neighbor.test.mjs`)와 원본 관측 fixture 등록만 더했다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 `RawSquidNeighbors.cpp/.h`와 대상 시험 `NeighborTests.cpp`에 없다(`git diff --numstat 1f00a25a HEAD`가 빈 결과, 저장소 루트에서 확인).

## 1. 남은 시험이 이미 재생됐는지 확인한 결과

PORT_MAP 메모의 "남음:" 두 갈래를 소비자와 fixture 로 추적했다.

| 남음 항목 | 이미 재생됐는가 | 처리 |
| --- | --- | --- |
| 울타리 이웃 `fence-neighbors-x86.tsv` 1,904행 | **예.** E88 `tests/fence-neighbors.test.mjs`가 같은 fixture를 전부 재생한다(manifest `fence-neighbors`) | 이 단위에서 다시 하지 않음 |
| 일반 해시 flag 0/4 의 다리 이벤트 장면 `neighbor-x86.tsv` (First 288·Event 288, 10.78) | **아니오.** E43 `bridge-events.test.mjs`는 `bridgeevent-x86.tsv`(첫 이웃을 가짜 값으로 공급)만 읽고, 어떤 웹 검사도 `neighbor-x86.tsv`를 읽지 않았다 | **이 단위에서 재생** |

## 2. 옮긴 검사 (`tests/neighbor.test.mjs`, 4건)

cpppj `NeighborTests.cpp`의 `Scene`/`Replay`/`neighbor_first_stops_before_unvisited_damage_and_does_not_mutate`를 그대로 옮겼다. 원본 fixture는 `cpppj/tests/fixtures/neighbor-x86.tsv`를 직접 읽고(10.78 `originals` 행만; `originalCD`·`original1037`은 범위 밖) 복사하지 않는다.

- **First 288행:** 노드 열(번호·타입·상태·extra·x/y 비트·발자국·flags1/2·프레임·해시 단계)을 실제 raw 풀 슬롯과 4단계 해시 버킷에 놓고 `RawSquidNeighbors.first(50)`의 반환을 대조한다. 변형 3종 합성 프레임 코드 표(`Frames(variant)`)를 쓴다.
- **Event 288행:** 같은 장면에서 `RawBridgeEvents.handle(50, 이벤트, 0, payload)`을 부르고 ① 반환 float 비트 ② 외부 효과 기록의 순서와 인자(`F:` 임시 프레임 낮은 바이트·`I:` 첫 이웃·`N:`·`C:`·`D:`·`O:`·`P:`) ③ 다리 슬롯과 새 슬롯 50바이트 전체를 대조한다. 첫 이웃 훅은 `makeBridgeNeighborHooks`(E43)가 만든 실제 연결에 사건 기록만 덧씌웠으므로 RawSquidNeighbors ↔ RawBridgeEvents 연결을 실제로 통과한다.
- **읽기 전용·손상 체인:** 첫 이웃 뒤의 손상된 체인(`next` 65535)/방문하지 않은 슬롯을 읽지 않고, 풀 전체 바이트가 변하지 않으며, SID 0·빈 지도는 거부한다.
- **경계 계약(아래 3절):** fixture가 구별하지 못한 두 입력.

## 3. 최초 실패와 교정, 대조 강도

최초 실패는 없다. 처음부터 통과했다(First 288·Event 288). 그래서 모듈 문자열 17개를 하나씩 바꾼 변형(서로 다른 파일을 복구하는 도우미, 실행당 제한 시간 60초)으로 대조 강도를 확인했다.

- 새 검사가 단독으로 탐지: 내부 비트(8)·교차 XOR 생략·접합 검사 생략·`anyType` 플래그 반전·탐색 사각형의 왼쪽/아래 한 칸 확장·내부 발자국 원천 시작.
- 새 검사만으로는 생존했으나 기존 검사(E39 `squid-neighbors`·E43 `bridge-events`·E88 등 소비자 9개 파일)가 탐지: 방향 동률 규칙·세로 부호·발자국 보존 조건·칸 보정 하한·dead/abstract 비트·중심 반 칸.
- **어느 검사도 탐지하지 못한 것 둘 → 경계 계약을 추가:**
  - 후보 기준점의 spot 칸은 좌표에 float `0.9999`를 더해 0 쪽으로 자른 칸이다(변형: `0.9998`). `20.00015`는 21 칸, `20.00005`는 20 칸으로 갈리는 입력 두 쌍을 `accept`에 직접 넣어 닫았다.
  - 프레임 번호가 코드 표 밖이면 지연 읽기가 아닌 호출은 `RangeError`이고 지연 읽기는 기본 코드(0,0,0,0)로 둔다(변형: 거부 제거).
- **동등 변형 1:** 방향 계산의 가로 차이 부호 `dx > 0` → `dx >= 0` 은 `|세로| < |가로|`가 참일 때만 도달해 dx가 0일 수 없으므로 결과가 같다.

## 4. 경계와 다음 시작점

- 프레임 코드 표는 웹 `TypeTable`에 코드 배열이 없어 소비자(검사)가 `frames[type]` 배열을 공급한다. 실제 World 연결(`bindCurrentTypes`)과 CD 10.72는 범위 밖이다(E39와 같음).
- 이 단위로 `RawSquidNeighbors`는 PORT_MAP `port`다. 3-B의 다음 항목은 `RawBridgeFallParent`(ScheduleBridgeFallChecked, SquidProcessHost 연결)다.
