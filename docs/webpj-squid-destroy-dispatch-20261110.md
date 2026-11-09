# webpj 다리 가상 삭제 분배(RawSquidDestroyDispatch) 이식 — WB04-E214

2026-11-10, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 "총·피해·삭제 수명" 묶음의 일부다. 다리 vtable +0x10 의 가상 삭제 재정의(004220f0)를 공통 destroy 앞에 연결했다. E44 `bridge-decay.js` 의 판정 함수(`bridgeDestroyProceeds`)를 실제 raw 풀·Graph 기록·프레임 코드 위에서 호출하는 분배기다.

- cpppj 기준선 `1f00a25a`. 소스(`RawSquidDestroyDispatch.cpp/.h`)·시험(`DestroyDispatchTests`) 모두 기준선 뒤 변경이 없다(`git diff --numstat 1f00a25a HEAD` 루트에서 확인, `RawBridgeEvents.h` 의 +3줄은 상수 선언으로 이 단위와 무관).
- 근거 커밋: 소스·시험 기준선 판. `DestroyDispatchTests` 5건은 손으로 쓴 계약 시험이고 원본 x86 fixture 는 없다. 같은 PC 의 통합 시험 실행 파일(`cpppj/build/bin/Release/netstorm_tests.exe --filter destroy_dispatch`, 게임 실행 아님)에서 5건 통과를 확인했다.
- 공유 장치(모두 기존 모듈 재사용): D2 `squid-destroy.js`(공통 삭제기), E44 `bridge-decay.js`(`bridgeDestroyProceeds`·`DEFAULT_BRIDGE_DECAY_MODE`·`BRIDGE_MINIMUM_GRAPH_SURFACES`), E43 `raw-bridge-events.js`(`BRIDGE_VTABLE` 0x005034c8), E39 `raw-squid-neighbors.js`(프레임 코드 조회·`types()`), E40 `raw-graph.js`(기록 255행·`validateTypes`), E213 `squid-unpop.js`/`squid-destroy.js` 의 `validateGraph`.

## 1. 옮긴 것

`www/core/raw-squid-destroy-dispatch.js`(DOM 없음, 10.78 전용)

- `RawSquidDestroyDispatch(base, neighbors, mode, graph)`: 삭제 몸체와 프레임 탐색기가 같은 풀인지, Graph 가 있으면 같은 풀·같은 해시/spot(삭제기의 `validateGraph`)·같은 타입 표(`graph.validateTypes(neighbors.types())`)인지 구성 때 확인한다. `mode` 는 객체 참조라 변경이 다음 호출부터 반영된다.
- `destroy(sid, flags, hooks)` — 004220f0
  1. 슬롯의 실제 가상 표(DWORD)가 다리(0x005034c8)일 때만 재정의를 적용한다(타입 번호로 고르지 않음). 다른 가상 표는 공통 `RawSquidDestroy.destroy` 로 직행한다.
  2. 편집기가 아니고 권한이 있고 extra 의 abstract/buried 비트(9)가 없을 때만 Graph 번호(+30)와 프레임을 읽는다. 하나라도 어긋나면 무효 Graph 번호(254)나 프레임 범위를 읽지 않는다(단락).
  3. Graph 가 있으면 기록 표면 수(signed WORD)를 읽는다. 번호 254 는 원본이 NULL 을 역참조하는 지점이라 쓰기 전 오류, 번호가 표 밖(255)도 오류다. Graph 가 없으면 표면 수 0 이다.
  4. 표면 수가 5 이상이면 현재 프레임 DWORD(+36, signed)로 프레임 코드를 읽는다(범위 밖은 오류). `bridgeDestroyProceeds(현재 모드, extra, 표면 수, 코드 플래그)` 가 거짓(단단한 프레임 0x40 또는 `debugKeep`)이면 **dead·장부·표시·프로세스를 바꾸지 않고 false** 를 돌려준다.
  5. 그 밖은 공통 몸체로 전달하고 true 를 돌려준다(이미 dead 이면 몸체가 반환).
- `bindCurrentModes(authority, editor)`: 현재 boss 소유자(`{ boss }`)와 편집기 DWORD getter 를 빌려 호출마다 읽는다. 외부 효과 뒤 이 진입에 도달한 현재 두 전역이 `mode` 의 값을 덮는다. 형식이 틀리면 거부한다. getter 가 없으면 `mode` 값을 쓴다.
- CD 10.72 의 BYTE 프레임(+34)·그래프 번호(+28)·extra(+35)·가상 표는 웹 범위 밖이라 옮기지 않았다.

## 2. 검증

`tests/squid-destroy-dispatch.test.mjs`(7건)

- cpppj `DestroyDispatchTests` 5건을 같은 장면(`DispatchScene`: 서버 풀 32768, 다리 TYPE 82(flags1 0x802·flags2 4), 프레임 코드 `K/P` 두 개(둘째 0x40), Graph 기록 255행 중 0번만 표면 수·reserved 23)으로 옮겼다. 거부 시 풀 전체 hex·free 수·flood 스택 Adler·콜백 수·두 깊이 불변, 전달 시 SID 반납(free 비트·free 수 +1)·콜백 2회(pre/post)를 확인한다.
  - `threshold_hard_debug_and_disabled_graph`(4/5 경계·−1·hard/보통·debugKeep·Graph 비활성), `short_circuits_graph_and_frame_reads`(네 단락), `reads_live_vtable_frame_and_keeps_direct_base_call`(프레임 변경 재조회·다른 가상 표·공통 몸체 직접 호출), `invalid_graph_and_frame_preserve_raw_state`(254·255·프레임 99), `rejects_mismatched_pool_and_graph_space`(풀 둘·Graph 풀·Graph 해시).
- 웹 계약 2건: 현재 권한/편집기 전역 대여(권한 꺼짐이 모드를 덮음·편집기 getter 호출마다 읽힘·형식 거부·`mode` 변경 다음 호출 반영), raw 좌표는 판정에 쓰이지 않음.
- 추가 한 가지: 음수 프레임(0xffffffff → −1)도 쓰기 전 오류임을 무효 프레임 시험에 넣었다.

최초 실패와 교정: 모듈은 처음부터 7건이 모두 통과했다. 비교가 실제로 작동하는지 모듈 사본을 바꿔 돌려 확인했다. 확인 뒤 모두 원본으로 되돌렸다(cmp 일치).

- 실패한 변형: 표면 수 `>=5` → `>5`(5건), 권한 대여 제거(1건), extra 관문 9 → 1(1건), 무효 Graph 번호 거부 제거(1건).
- 통과한 변형: 프레임 하한(`frame < 0`) 제거. 음수 프레임이 `codes[-1].flags` 의 TypeError 로 어차피 같은 쓰기 전 거부가 되므로 결과가 같은 동치 변형이다. 명시적인 RangeError 는 원본 접근 오류의 의도를 드러내기 위해 유지한다.

## 3. 경계와 남은 것

- 실제 월드에 연결하지 않았다. 호출 지점(다리 가상 vtable +0x10 분배)과 현재 boss/편집기 전역(`RawAuthorityState`·005c85a4)의 실제 소유자는 순서4~5 몫이다(PORT_MAP session 0).
- 원본 x86 관측이 없는 단위다. 원본과의 대조는 cpppj 계약 범위에 머문다. 판정식 자체(`bridgeDestroyProceeds`)는 E44 의 fixture 로 이미 대조됐다.

## 4. 다음 시작점

WB04-E215 는 3.1절에서 고른다. 같은 묶음에서 기준선 뒤 변경이 없는 `RawGeyserMined`(partial, E108 이 일부를 옮김)·`RawFrameChoose`(partial)의 남은 부분을 권한다. `RawRitualChoice` 는 cpppj 에서 MP 6b 입력/수신 사전 구현이라 웹 범위를 먼저 확인한다.
