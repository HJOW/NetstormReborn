# webpj 섬 받침 삭제 훅(RawIslandLifecycle) 이식 — WB04-E224

2026-11-10, HJOW-Athlon. 섬 받침(`island`)의 preDestroy 재정의(00442240)와 noIsland 표면의 postDestroy 재정의(00442640)를 옮겼다. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절의 "순서4 이후로 넘기는 `o/` 단위(Fort 적재·지형/섬 …)"에 속하며 기준선 뒤 변경이 없고 원본 관측이 있어 E220 섬 검색·E221 Fort 소유자 후처리·E222 섬 Builder·E223 섬 population 에 이어 옮겼다. 지연 낙하(E213 `squid-process.js` 의 `scheduleBridgeFall`)가 이 훅의 소비자다.

- cpppj 기준선 `1f00a25a`. 소스(`RawIslandLifecycle.cpp/.h`)·시험(`IslandLifecycleTests`)·fixture(`islandlifecycle-x86.tsv`) 모두 기준선 뒤 변경이 없다(`git diff --numstat 1f00a25a HEAD` 를 저장소 루트에서 확인, 출력 없음).
- 근거 커밋: 소스·fixture `6a0907de`(2026-10-08).
- 원본 fixture: `islandlifecycle-x86.tsv` 에서 10.78(`originals`) 520행(IslandPre 260·SurfacePost 260)과 장면 입력. 입력은 받침 SID 50 의 이웃·다리·walker·상태·extra·좌표(float 비트)·해시 단계와 삭제 flags, 출력은 훅 호출 순서와 인자(`G:다리:x비트:y비트`·`W:walker`·`X:받침:flags`·`Y:표면:flags`)다. CD 10.72·10.37 행은 웹 범위 밖이다.
- 공유 장치: `raw-squid-neighbors.js`(`RawSquidNeighbors.center`·`RawSquidNeighborWalk`), `raw-squid-finder.js`(`RawSquidFinder`), `assets/type-flags.js`(`TypeFlag2.bridge/walker`), `squid-process.js`(`scheduleBridgeFall`·`hasScheduledBridgeFall`).

## 1. 옮긴 것

`www/core/raw-island-lifecycle.js`(DOM 없음, 10.78 전용)

- `ISLAND_DESTROY_KEEPS_BRIDGES = 0x1000`: 삭제 flags 에 이 비트가 있으면 둘레 다리의 지연 낙하 예약을 건너뛴다.
- `RawIslandLifecycle(pool, neighbors, hooks)`: 이웃 탐색기가 같은 풀을 읽는지, 훅 4개(`scheduleFall`·`fallWalker`·`basePreDestroy`·`basePostDestroy`)가 모두 연결됐는지 확인한다. 훅은 호출 사실과 인자만 받는 실행기의 대체와 같은 경계이고 지연 낙하 예약·walker 낙하·공통 pre/postDestroy 의 실제 효과는 호출자가 연결한다.
- `islandPreDestroy(island, flags)`(00442240): 받침이 abstract/buried(extra 9)가 아니면 받침 중심을 탐색기를 만들기 전에 한 번 구하고 flag 0 연결 이웃을 순회한다. flags 에 0x1000 이 없으면 **죽지 않은 다리(genus bridge)** 마다 받침의 중심 좌표로 지연 낙하를 예약한다(섬이 사라지면 닿아 있던 다리가 끝 칸으로 바뀐다). 탐색기는 0x1000 이 있어도 만들고 끝까지 순회하며 다음 이웃은 예약 훅이 바꾼 뒤의 풀에서 읽는다. 그 뒤 항상 공통 preDestroy 를 부른다.
- `surfacePostDestroy(surface, flags)`(00442640): 표면이 abstract/buried 가 아니면 표면 좌표를 0 쪽으로 자른 뒤 `_ftol` 하위 32비트로 읽은 칸(x, y)에 걸린 객체를 일반 탐색(해시 0단계 건너뜀)으로 훑어 walker genus 마다 낙하 훅을 부른다(낙하 훅이 바꾼 체인/타입을 매번 다시 읽음). 그 뒤 항상 공통 postDestroy 를 부른다. 비유한/64비트 밖 좌표는 원본 정수 불확정값의 하위 0 으로 둔다.

## 2. 검증

`tests/island-lifecycle.test.mjs`(4건)

- 원본 관측 재생: 520행 전부(IslandPre 260·SurfacePost 260). 행마다 장면을 새로 만들고 훅 호출 문자열이 같은지, 풀 전체가 변하지 않는지 확인한다. 합성 장면이라 훅은 호출 사실과 인자만 기록한다.
- cpppj 계약 포팅 2건: 훅 누락·다른 풀의 탐색기 거부와 이웃 없는 받침도 공통 preDestroy 를 부름(`X:50:4660`), 실제 `SquidProcessHost`/`Kernel` 위에서 0x1000 이면 예약 없이 pre 만, 아니면 살아 있는 다리(동쪽)에만 예약하고 죽은 다리(남쪽)는 건너뛰며 다음 프레임에 받침 중심 칸을 포장한 payload 7196(`(28&255)|(28<<8)`)으로 사건 0x2692 가 불린다.
- 웹 계약 1건(원본 fixture 에 음수 좌표 행이 없어 추가): 표면 x = -0.5 는 0 쪽으로 잘려 칸 0 의 walker 만 낙하시킨다. floor 로 자르면 x = -1 이 탐색기의 "끝 = -1 → 월드 끝까지" 의미가 되어 같은 행의 먼 walker 까지 걸린다. abstract(1)/buried(8) 표면은 순회 없이 공통 postDestroy 만 부른다.

최초 실패와 교정: 모듈은 처음부터 520행이 모두 통과했다. 웹 계약 1건은 시험 쪽 결함이었다(음수 좌표를 `Scene.prepare` 해시에 넣으려 했고 walker y 비트를 32.0 으로 잘못 썼다. 양수로 만든 뒤 표면의 raw x 만 바꾸고 y 를 31.5 로 고쳤다). 모듈 사본을 바꿔 돌려 비교가 작동하는지 확인했다. 확인 뒤 원본으로 되돌렸다(diff 일치).

- 실패한 변형: extra 마스크 9→8, 해시 0단계 건너뛰기 제거, keepBridges 반전, 다리/walker genus 맞바꿈(각 1), 중심 x/y 맞바꿈, 공통 preDestroy 제거, 0x1000 비트 변경, 표면 x/y 읽기 오류와 +1, 표면 extra 무시, 공통 postDestroy 제거, 0 쪽 절삭→floor.
- 적용되지만 통과한 변형(동등 변형): 모듈의 죽은 다리 검사 `(state & 2) === 0` 의 제거·비트 변경. 이웃 순회(`RawSquidNeighbors` 의 후보 수락)가 이미 죽은(state & 2) 후보를 순회에서 제외하므로 모듈 쪽 검사는 순회가 내놓은 이웃에 대해 항상 참이다. 원본이 같은 검사를 두므로 원문 그대로 남긴다. 위 계약 시험의 죽은 다리(남쪽)는 순회가 걸러서 예약되지 않는 것을 확인하는 용도다.

## 3. 경계와 남은 것

- 실제 월드에 연결하지 않았다. 섬 받침의 vtable 에 이 훅을 꽂는 일(섬 타입 생성 연결)과 실제 `walker` 낙하·공통 pre/postDestroy 하위 연결은 순서4 몫이다(PORT_MAP session 0). 지연 낙하 예약은 이미 이식된 `scheduleBridgeFall`(E213)에 이어 보았다.
- CD 10.72·10.37 분기는 웹 범위 밖이다.

## 4. 다음 시작점

WB04-E225 는 3.1절에서 고른다. 기준선 뒤 변경이 없는 Fort/섬 단위(`RawFortMetaLoad`·`RawFortBadgesGlobals`·`RawFortPaletteTransition`·`RawTerrainBitmap`·`RawStateWorldCleanup`·`SquidSpatial`·`RawTerrainGrowth`)를 권한다.
