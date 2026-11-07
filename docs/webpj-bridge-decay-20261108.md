# webpj 다리 붕괴 순수 논리(Bridge 붕괴·BridgeDecayScan) 이식 — WB04-E44

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 마흔네 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/Bridge`(10.78; 원본 00421770/004217f0/004218b0/00421b60/00421bd0/00421c30/00421db0/00421e60/004220f0/004227e0)와 `BridgeDecayScan`(00422490/00422bc0)의 순수 논리를 옮겼다. [E12](webpj-graph-20261108.md)가 옮긴 `Bridge::Connects`(`surface-finder.js`)를 제외한 `Bridge.cpp` 나머지 전부이며, 외부 효과(실제 삭제·그리기·소리·이동체 확인)는 실행하지 않고 사건 목록으로 알린다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

`www/core/bridge-decay.js` — 함수들과 상수(`BRIDGE_LIFE_MASK`·`BRIDGE_CRACK_LIFE`·`BRIDGE_DECAY_OUTCOME`·`DEFAULT_BRIDGE_DECAY_MODE` 등)

- `bridgeIsOpen`/`bridgeOpenDirection` [00421770/004217f0]: 연결 마스크가 있는 방향에서 이웃 목록에 없는 표면(또는 번호 0·지도 밖)이 열린 방향. 세 갈래(B~E)는 두 번째, 모퉁이/판자(F~K)는 첫 열린 방향, 끝 칸 L·M·N·O는 연결 반대 방향. 좌표는 `fround(x+오프셋)`에 `0.9999f`를 배정도로 더해 절삭.
- `bridgeCollectDecay` [004218b0]: 부모만 제외한 재귀 방문 목록(용량 100, 깊이 256 안전 한계). 경계에서는 같은 번호를 모두 지우고 접합 고리는 `complete=false`로 알린다.
- `bridgeReduceLife` [00421c30 접두]: 수명 비트(0x78)를 빼 0 미만은 0, 서버이고 0이면 제거(J·K는 판자 제거 플래그), 금 간 전환 조건(5 이상→5 미만)과 이동체 확인 조건(5 이하). 정상 범위 0~7 밖은 쓰기 전에 거부.
- `bridgeCrackedFrame`/`bridgeNormalFrame`/`bridgeWeaken`/`bridgeRestore` [00421b60/00421bd0/00421db0/00421e60]: 번호(signed char)에 ±10을 적용한 변형 'P' 프레임 검색(플래그는 부호 확장 후 마스크).
- `bridgeDestroyProceeds`·`bridgeApplyLife`·`bridgeDecayCell` [004220f0/00421c30/004227e0]: destroy 재정의(편집기·권한·abstract/buried·표면 수 5 이상일 때 단단한 프레임/디버그 유지가 막음, 그래프 254는 원본 예외 지점), 수명 적용(제거 또는 쓰기 → 금 간 프레임/디버그 갱신 → 이동체 확인), 한 칸 처리(작은 Graph 제거 → 단단한 프레임 → 이웃 없음 → 열린 방향 → 방문 목록 → 최소 수명 → 방문별 감소 → 금 소리).
- `bridgeDecayScanReset`/`bridgeDecayScanAdvance`/`bridgeDecayScanEligible` [00422490/00422bc0]: 범위(10.78 15000..23001, CD 6000..14000), 프레임별 `trunc(delta/10 × 범위)`개 전진(기한 지나면 범위 전체), 주기 지나면 커서 복귀·다음 시각 `now+10`.

## 2. 검증 (`tests/bridge-decay.test.mjs`, 7건)

원본 `bridgedecay-x86.tsv`(`cpppj/tests/fixtures`를 직접 읽음; 이 논리는 판본에 무관해 세 PE 행 전부 재생, 스캔 초기화만 판본 이름으로 범위 선택): Cell 4241(사건 순서·결과 상태, 주요 분기 6종 모두 통과 확인)·Life 3264(수명 7 초과로 새 코드가 거부하는 400행 포함)·Destroy 1152·Crack 348·Normal 174·Weaken 696·Restore 696·Init 12·Scan 3941(처리 대상 번호·커서·다음 시각 비트). 계약: 14ms 프레임 스캔이 프레임마다 11개씩 8002개를 훑고 주기 끝에 복귀, 정지/편집기/권한 없음 무변경, 접합 고리 `Incomplete`, 그래프 254 `InvalidGraph`/원본 예외 지점, 서버 아님은 destroy에 닿지 않음, 범위 밖 프레임·입력 누락 거부.

최초 실패와 교정: 대형 fixture는 모두 첫 실행에 통과했다. 계약 테스트 한 건이 `{}`로 부분 모드를 넘겨 서버 기본값이 빠진 것은 테스트 입력 오류였다(`bridgeApplyLife`는 완전한 모드 객체를 받고 `bridgeDecayCell`/`bridgeDecayScanAdvance`는 기본값과 합친다).

## 3. 경계

- `RawBridgeDecay`(raw 풀 위의 실제 슬롯 방문·`RawCanonSurfaceWalk` flag 8 이웃·직접 WORD/프레임 쓰기·진단 0x268)와 `RawBridgeDecayScan`(raw 풀 스캔 루프)·`RawBridgeCarriers`(위험한 다리 위 사제/rainWalker 대피)는 다음 단위이다. 이 단위의 함수들은 그 바닥의 계약(기대값)이다.
- CD 10.72(수명 함수 인라인·프레임 1바이트)와 `BridgeTests.cpp`의 패턴 선택·캐논 디코더(다른 소스 `RawBridgeDraw` 등)는 범위 밖이다.

## 4. 다음 시작점

`RawCanonPlacementSurrounding`(`RawCanonSurfaceWalk`)과 `RawBridgeDecay`·`RawBridgeDecayScan`·`RawBridgeCarriers`(fixture `bridge-decay-checking`·`bridge-decay-scan-runtime`·`bridge-carrier-retreat`), `RawIslandPostPop`의 섬/표면 접두 본체, Player 나머지, 종류별 PostPop, `SquidDisplay`/`SquidFrame`이다.
