# webpj 표면 삭제 훅 분배기 이식(RawSurfaceLifecycle) — WB04-E248

2026-11-10, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 단계 3-B의 일곱째 항목이다. cpppj 기준선 `1f00a25a`의 `o/RawSurfaceLifecycle`(복원된 섬·다리·noIsland 삭제 훅을 공통 삭제와 잇는 호스트 분배기, 127줄)과 기준선 뒤 `712e7224`가 더한 Native 연결 TYPE 대여를 옮기고, cpppj `SurfaceLifecycleTests.cpp`의 통합 장면을 같은 모듈 조립으로 재현했다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만).

## 1. 기준선 뒤 변경 판정 (3.6절)

`git diff --numstat 1f00a25a HEAD`는 `.cpp +3/−1`·`.h +2/−0`, 커밋은 `712e7224 20261109-34 CP`(Native 표면 수명 가상 삭제와 현재 연결 TYPE 장기 소유권 연결) 하나다. 변경은 `BindCurrentConnectorType(const std::uint32_t&)`과 `currentConnectorType_` 포인터를 더해, 다리 preDestroy 분배에서 이 주소가 있으면 `PreDestroyNative`(현재 전역을 후보마다 다시 읽음), 없으면 typed 값 사본 경로를 쓰게 한 것이다.

- **② 재반영:** 웹에 `bindCurrentConnectorType(getter)`를 추가했다. getter 는 현재 전역 DWORD 를 돌려주는 함수이고 `null`은 typed 값 사본으로 되돌린다. getter 가 아닌 값은 `TypeError`다. cpppj 가 판본(Patch1078) 아니면 거부하는 분기는 웹 풀이 10.78 전용이라 옮기지 않았다. 행 base 는 판정한 `712e7224`로 올렸다.

## 2. 옮긴 것

`www/core/raw-surface-lifecycle.js` — `RawSurfaceLifecycle(pool, neighbors, connectorType, common, hooks)`, `NO_ISLAND_VTABLE`

- 프로세스 호스트의 asset 훅으로 공급하는 `hooks()`는 공통 삭제 사건 중 `preDestroy`/`postDestroy`만 슬롯 +0의 **실제 가상 표 기록값**으로 분배하고 나머지(`releaseDependent` 등)와 가상 표가 다른 객체는 모두 `common.emit`으로 보낸다. 선택 조회(`selected`)와 form 해제(`unpopForm`)는 `common`을 그대로 펼쳐 쓴다.
  - 다리(`0x005034c8`): pre → `RawBridgeLifecycle.preDestroy`(Native 연결이면 `preDestroyNative`), post → `postDestroy`. 다리 훅 효과는 `destroyLink` → `hooks.destroy`(연결 객체 삭제를 최종 프로세스 호스트 훅으로 재진입), 공통 pre/post → `common.emit`, `fallWalker` → `hooks.fallWalker`, 파편 요청/소리 → `hooks.bridgeEffect`.
  - 섬 받침(`0x00506d98`): pre 만 `RawIslandLifecycle.islandPreDestroy`. noIsland(`0x00506f08`): post 만 `surfacePostDestroy`. 둘의 낙하 예약/walker 낙하는 `hooks.scheduleFall`/`hooks.fallWalker`, 파생 처리 뒤 공통 pre/post 는 받은 flags 그대로 `common.emit`이다.
- 구성 때 거부: 훅 누락(`common.emit`/`selected`, `scheduleFall`/`destroy`/`fallWalker`/`bridgeEffect`) `TypeError`, 연결 객체 TYPE 번호가 첫 자산 타입(70) 미만이거나 TYPE 표 길이 이상이면 `RangeError`, 이웃 탐색기가 다른 풀이면 `TypeError`.

## 3. 검증 (`tests/surface-lifecycle.test.mjs`, 9건)

cpppj `SurfaceLifecycleTests.cpp`의 `Run`(조립)과 여덟 TEST_CASE 를 웹 모듈로 같은 순서에 옮겼다. 조립은 실제 `SquidFactory`·`SquidOwner`·`SquidPop`/`SquidUnpop`·`SquidDisplay`·`SquidFrame`·`RawBridgeConnect`·`RawIslandPostPop`·`RawBridgeEvents`·`RawSquidDestroy`/`RawSquidDestroyDispatch`·`SquidDestroyLifecycle`·`SquidProcessHost`·`Kernel`·(선택) `RawGraph`다. 입력(실제 보호 TYPE 사본에서 5개 타입의 플래그·발자국, 합성 SHP 셀)만 합성이고 기대는 모듈이 낸 결과의 관계식이다. 다리 프레임 코드는 `bridgedecay-x86.tsv`의 `Frames 0` 행(실제 `bridge.type` 60개)을 쓴다.

- 시나리오 Replace(끝 칸 O 다리로 교체·옛 다리와 연결 객체 정리·지불 비용/장부/spot)·Isolated(이웃 없는 다리 삭제)·Hard(단단한 다리는 수명만 바꾸고 예약·연결 유지, Graph 활성이면 가상 삭제 거부 뒤 편집기에서 정리)·Keep(삭제 flags `0x1000`은 예약 생략).
- 변형: 표면 칸 먼저 삭제해 walker 낙하 분배 확인, Graph 활성(표면 수 장부를 모든 슬롯과 대조), Native 연결 TYPE 주소 대여(typed 사본은 일부러 다른 값 156을 줘 현재 전역 155를 읽어야만 연결이 정리됨).
- 계약: 훅 누락·범위·다른 풀 거부와 다른 가상 표의 공통 훅 경유(기록용 `common`으로 호출 횟수와 `selected`·`unpopForm` 전달 확인), 다리 walker 낙하·파편/소리 분배, 섬/표면 삭제 flags 전달.
- 범위 밖: CD 10.72 판본 장면과 선택 검사 `InspectSurfaceLifecycle`(실제 SHP 표시 변경 영역)·Renderer 합성은 웹 범위 밖이거나 5-A 렌더러 연결 몫이다.

최초 실패와 교정: 8건이 첫 실행에 통과했다. 변형 15개(Native 무시·분기 return 제거·섬/표면 분기 제거/위치 뒤바꿈·연결 삭제를 효과로 보냄·공통 pre/post 뒤바꿈·walker 를 효과로 보냄·연결 TYPE 상/하한·풀 검사·공통 훅 펼침 제거·flags 고정·가상 표 상수 오타) 중 12개를 바로 탐지했다. 살아남은 셋 중 둘(다리 walker 가 `bridgeEffect`로 가는 변형, 섬 pre 의 flags 를 0 으로 고정하는 변형)은 통합 장면에 다리 칸 walker 와 섬 flags 가 없어서였고, 직접 분배 계약 검사 1건을 추가해 닫았다. 나머지 하나(풀 검사 제거)는 `RawIslandLifecycle`이 같은 조건을 이미 검사하는 중복 방어라 결과가 같은 동등 변형이다.

## 4. 경계와 다음 시작점

- 아직 앱 수명에 연결되지 않았다(PORT_MAP session 0). 실제 World 의 asset 훅 자리(`lifecycle.hooks()`를 프로세스 호스트에 대여)와 `bindCurrentConnectorType`에 쓸 현재 연결 TYPE 전역 소유자는 4-D 에서 잇는다.
- 이 단위로 `RawSurfaceLifecycle`은 PORT_MAP `port`(base `712e7224`)다. 3-B의 마지막 항목은 `RawRegionOwnership`(+54, outpost 분기는 웹 제외)이다.
