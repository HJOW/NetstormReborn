# webpj 다리/섬 연결(RawBridgeConnect) 이식 — WB04-E42

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 마흔두 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawBridgeConnect`(10.78; 원본 004213b0/004210f0/00421240/004421c0/004421a0/00442310)를 옮겼다. [E39](webpj-squid-neighbors-20261108.md)가 미룬 일반 해시 이웃 순회(flag 0/4)와 다리 이벤트 장면을 이 단위에서 원본 관측으로 대조했다. [E41](webpj-squid-postpop-20261108.md)의 다리 접두(`bridgePrefix`)와 섬 받침 접두(`setIslandPrefix`)가 부르는 본체이다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

`www/core/raw-bridge-connect.js` — `RawBridgeConnect`, `BridgeConnectState`, `ownerColorFrame`, `setIslandOwner`, `makeOwnerDispatch`

- `RawBridgeConnect(pool, neighbors, state, hooks)`: 훅 `{create, setOwner, pop, setFrame, notifySurface}`가 모두 있어야 하고 이웃 탐색기는 같은 풀이어야 한다.
  - `connect(sid)`/`connectPreview(sid, append)` [004213b0]: flag 4(abstract 제외) 이웃을 순회하며 다리↔섬 표면 쌍(다리이면 genus가 `0x1000002`에 걸리는 이웃, 아니면 다리인 이웃)마다 `link` → `propagateOwner`. 다음 이웃은 연결 객체 생성/소유자 지정이 바꾼 풀·해시에서 읽는다. 미리보기는 등록 대신 위치/화면 좌표만 쓰고 목록에 즉시 넣는다(재진입 생성 전에 보임).
  - `link(first, second, owner, preview?)` [004210f0]: first 위치에서 second의 거의 올림한(`0x3f7fff58`) 중심 쪽으로 네 방향(동률 세로) 한 칸 옮긴 곳에 연결 객체를 만들고 프레임=방향/2, 소유자, Pop, 첫 참조(+12), (연결 객체 타입이 다리 타입이면) 표면 알림, 둘째 참조(+8) 순으로 기록한다.
  - `propagateOwner(first, second)` [00421240]: second 위치의 섬 받침이 주인이 없으면 first의 소유자를 주고 색 프레임 → 같은 위치 종유석 프레임 → 받침 발자국 안 noIsland 소유자 순서로 전파한다. 소유자 0이면 원본 assert 경로로 훅을 부르기 전에 거부한다.
  - `findTypeAt(x, y, type)` [004b1fa0]: 좌표를 자른 한 칸에서 일반 탐색 순서로 지정 타입의 첫 객체(island/bridge genus가 아니면 해시 0단계 생략).
  - `islandPostPopPrefix(island, flags)`/`islandPostPop(island, flags, base)` [004421c0]: 최초 등록(flags 비트 1)에서 종유석 생성 → 소유자 → 같은 자리 Pop → 연결 순회, 그 뒤 공통 후처리 훅.
- `ownerColorFrame(state, owner)` [004421a0]: 전투 모드이고 양수면 색 표 값 − 1(DWORD 감김), 아니면 중립 8. 표 밖 번호는 거부한다. `setIslandOwner`/`makeOwnerDispatch` [00442310]: 섬 받침(0x506d98)·종유석(0x506e50)이면 base 소유자 지정 뒤 프레임 필드에 색을 직접 쓴다(미션 중 중립 프레임은 유지).

## 2. 검증 (`tests/bridge-connect.test.mjs`, 7건)

원본 10.78 `bridgeconnect-x86.tsv`(`cpppj/tests/fixtures`를 직접 읽음, 3,600행 중 originals): Walk 1856(232 장면 × flags 0~7의 이웃 목록), Connect 464(등록/미리보기), Link 392, Owner 330, FindAt 270, Color 42, IslandOwner 96, StalagOwner 96, IslandPost 54. 각 행은 기록 훅의 사건 순서·새 슬롯 전체·풀 Adler·미리보기 목록을 원본과 대조한다. 계약: 미리보기 소비자 누락/목록 쓰기 시점, 훅 누락·다른 풀·미지원 flag·색 표 밖 번호·소유자 없는 다리 전파가 상태를 바꾸기 전에 거부됨, 섬 받침 접두의 공통 장치 누락 앞 효과 보존, 소유자 분배의 가상 표 선택, 순회 중 뒤쪽 버킷 등록의 실시간 반영.

최초 실패와 교정: 7건이 처음부터 통과했다.

## 3. 경계

- cpppj 통합 검사 `bridge_connect_real_raw_modules_capture_neutral_island`(실제 `SquidFrame`·`SquidFactory`·`SquidOwner`와 다리 실제 프레임표를 잇는 시나리오)는 `SquidFrame`(미이식)과 실제 `bridge.type` 프레임표가 필요해 `SquidFrame`/월드 조립 단위에서 대조한다.
- 다리 이벤트 처리기 `RawBridgeEvents`(`neighbor-x86` Event 행)는 다음 단위이다. CD 10.72(프레임 1바이트, 소유자 0x20/extra 0x23)와 original1037 구간은 범위 밖이다.

## 4. 다음 시작점

`RawBridgeEvents`(다리 이벤트: `neighbor-x86` Event 행·`bridgeevent-x86`)와 `RawBridgeDecay`/`RawBridgeDecayScan`/`RawBridgeCarriers`, `RawIslandPostPop`의 섬/표면 접두 본체, Player 나머지, 종류별 PostPop, `SquidDisplay`/`SquidFrame`이다.
