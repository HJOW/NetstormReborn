# webpj 다리 이벤트(RawBridgeEvents) 이식 — WB04-E43

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 마흔세 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawBridgeEvents`(10.78; 원본 00422740 다리 이벤트 처리기와 004215d0 끝 칸 변환)를 옮겼다. [E42](webpj-bridge-connect-20261108.md)의 다리 연결과 같은 다리 클래스(vtable 0x5034c8)의 이벤트 쪽이며, [E15](webpj-small-parents-20261108.md)가 예약한 지연 낙하(0x2692)가 이 처리기로 들어온다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

`www/core/raw-bridge-events.js` — `RawBridgeEvents`, `makeBridgeRegularHandler`, `makeBridgeNeighborHooks`, 상수

- `RawBridgeEvents(pool, state, hooks)`: `state = {authority, debugKeep, bridgeType}`, 훅 7종(`frames`·`firstNeighbor`·`notifySurface`·`create`·`destroy`·`setOwner`·`pop`)이 모두 있어야 한다.
  - `handle(bridge, event, count, payload)` [00422740]: 0x2691(자기 destroy)과 0x2692(payload의 `(x&255)|(y<<8)` float을 `_ftol` 하위 DWORD로 풀어 끝 칸 변환)를 처리해 권한 없음=0(종료)·있음=−1(유지)을 돌려주고, 그 밖의 이벤트는 payload를 그대로 돌려준다. `count`는 읽지 않는다.
  - `convertEnd(bridge, x, y)` [004215d0]: 디버그 유지/권한 없음이면 아무것도 하지 않는다. form 타입은 거부. 현재 프레임 방향 글자가 J이면 `objectY < y`로 N/L, K이면 `objectX < x`로 M/O를 고르고(비교 거짓·NaN은 L/O) 그 글자의 첫 프레임을 **임시로** 쓴 채 flag 0 이웃을 묻고 곧바로 옛 프레임을 되돌린다(이웃 조회가 던져도 되돌림). 이웃이 없으면 destroy, 있으면 수명 비트(`(5*8−8)&0x78`)를 쓰고 표면 알림, 되돌린 프레임이 단단한(0x40)이면 교체하지 않으며, 아니면 같은 타입의 새 객체를 만들어 끝 프레임·소유자·수명 단어(다리 타입이면 표면 알림)·부모 단어·플래그 비트 0x10을 옮기고 옛 칸 위치를 읽은 뒤 destroy하고 같은 위치에 Pop한다.
- `makeBridgeRegularHandler(pool, events, fallback)`: Regular 프로세스의 부모 이벤트 처리기를 슬롯 vtable로 분배한다(다리 클래스 → 다리 처리기, 아니면 fallback, 없으면 payload).
- `makeBridgeNeighborHooks(neighbors, effects)`: 같은 타입 표의 프레임 조회(`FrameCodeTable`)와 실제 flag 0 첫 이웃(`RawSquidNeighborWalk`)을 연결한다.

## 2. 검증 (`tests/bridge-events.test.mjs`, 6건)

원본 10.78 `bridgeevent-x86.tsv`(`cpppj/tests/fixtures`를 직접 읽음, originals): Handler 1,207행·End 726행(권한·디버그·변형·프레임·위치·단어·부모·소유자·플래그·이웃·이벤트·payload 입력 → 반환 float 비트(NaN은 한 이름)·사건 순서·다리/새 슬롯 Adler), Table 306행(글자 A~P의 첫 프레임·개수·글자 수; 웹 `FrameCodeTable.run`과 대조), Frames 3변형. 실제 다리 타입의 프레임 60개(`bridgedecay-x86.tsv`의 `Frames 0`행): J=35(6)·K=41(6)·L/M/N/O=47/50/53/56(3)이 보통 프레임이고 단단한 프레임은 J·K의 번호 20 두 개뿐. 계약: 훅 누락 거부, 권한/디버그 정지, form 타입·표 밖 프레임 거부, 이웃 조회 실패 시 프레임 복원, Regular 분배, 이웃 훅 연결.

최초 실패와 교정: 모듈은 작성 중 금 간 수명 상수를 4로 잘못 적었다가(원문은 5, 결과 `0x20`) 대조 전에 `Bridge.h`로 바로잡았다. 그 뒤 fixture 전 행이 첫 실행에 통과했다.

## 3. 경계

- cpppj 통합 검사 `bridge_event_runs_from_scheduled_fall_through_regular_process`(Kernel·`SquidProcessHost`·`ScheduleBridgeFall`로 예약→프레임→분배→변환)와 `bridge_event_real_raw_conversion_replaces_space_and_removes_its_process`(실제 Factory/Pop/Destroy 수명)는 프로세스 호스트와 실제 월드 조립이 필요해 해당 단위(WB05)에서 대조한다.
- CD 10.72(끝 칸 변환이 처리기 안에 인라인)와 original1037 구간은 범위 밖이다.

## 4. 다음 시작점

`RawBridgeDecay`/`RawBridgeDecayScan`/`RawBridgeCarriers`(다리 붕괴·스캔·운반체 후퇴), `RawIslandPostPop`의 섬/표면 접두 본체, Player 나머지, 종류별 PostPop, `SquidDisplay`/`SquidFrame`이다.
