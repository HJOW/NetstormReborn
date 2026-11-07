# webpj noIsland 후처리·받침 소유자 전파(RawIslandPostPop) 이식 — WB04-E47

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 마흔일곱 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawIslandPostPop`(10.78; 원본 004423b0 noIsland 후처리, 00442350 표면 글자, 0044bd20 받침 소유자 전파)를 옮겼다. [E41](webpj-squid-postpop-20261108.md)의 `SquidPostPop.setSurfacePrefix`가 부르는 표면 접두 본체이고, [E42](webpj-bridge-connect-20261108.md)의 `RawBridgeConnect`(생성·소유자·Pop·연결 순회 훅과 `findTypeAt`)에 의존한다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

`www/core/raw-island-post-pop.js` — `RawIslandPostPop`
`www/core/raw-bridge-connect.js` 변경 없음. 시험 보조 `tests/raw-scene-support.mjs`(E42 시험의 `Scene`을 두 시험이 공유하도록 분리).

- `RawIslandPostPop(pool, surfaceMap, types, frames, typeState, state, hooks)`: 표면 지도(256×256 머리 SID)와 타입/프레임 표를 받고 훅 `{objects:{create,setOwner,pop}, connect, findTypeAt, updateDisplay, incompleteSupport, assertion, detail, typeWas}`는 도달한 지점에서만 요구한다. `state = {loadingDepth, savedGame()}`의 저장 게임 관문은 매번 현재값을 읽는다.
  - `prefix(surface, flags)` [004423b0]: 최초 등록(비트 1)만. 요새를 읽는 중이면 서쪽/북쪽 칸 글자(`C→G, F→C, A→D, B→A, E→H, I→E`, 그 밖은 북쪽 기준 `F→B, B→I`, 나머지 `F`)로 자신의 글자를 정해 변형 P·번호 1의 첫 프레임을 **프레임 필드에 직접** 쓴다(없으면 진단 `Rifttype.cpp:299` 뒤 −1 사용, 선택 뒤 글자 불일치는 `Construction.cpp:0xfa`). 글자 F이면 받침을 만들어 소유자·Pop(발자국−1 오프셋의 DEC 순환 `footDelta`). 저장 게임 관문이 꺼져 있고 글자 H이면 `(값−2)+0.9999` 칸의 왼쪽 위 받침 소유자를 `setSupportOwner`로 전파(없으면 `gSid` 진단). 끝에 표면 칸 연결 순회.
  - `setSupportOwner(x, y, owner)` [0044bd20]: `(x+0.9999, y+0.9999)` 칸이 noIsland일 때만. 절삭한 오른쪽 아래 기준점의 3×3 칸을 x 바깥·y 안쪽으로 머리 객체에 소유자+표시 갱신(없는 칸은 누락 표시), 이어 받침·종유석 소유자+표시 갱신. 누락 진단은 모든 정상 칸 효과 뒤에, 상세/assert 장치가 있으면 `Damageable.cpp:0x18b`로 중단(상세 문자열에 칸별 `X`/`[타입]`/` (No islandSid) `/` TypeWas n`), 없으면 기존 `incompleteSupport` 경계.
  - `surfaceLetter(x, y)` [00442350]: 범위 안 한 칸이 noIsland이면 프레임 방향 글자(PATCH는 signed char, form 타입이면 진단 `!isForm()` 0x854), 아니면 0.

## 2. 검증 (`tests/island-post-pop.test.mjs`, 4건)

원본 10.78 `islandpostpop-x86.tsv`(`cpppj/tests/fixtures`를 직접 읽음, originals): SurfacePostPop 402행·SupportOwner 150행의 사건 순서(생성 `C:`·소유자 `O:`·Pop `P:`·표시 `U:`·불완전 `D`·접두 호출 `T:`)·새 슬롯 전체·풀 Adler를 `RawBridgeConnect` 훅과 함께 재생. 계약: 범위 밖 글자 0과 H 글자, 비최초(비트 없음)·저장 게임 관문의 무효과, 저장 게임 주소를 매 H 칸에서 다시 읽음, 누락 진단의 순서와 상세/assert 장치 연결 시의 원문 진단 중단, noIsland가 아닌 칸의 무효과.

최초 실패와 교정: 모듈은 552행 전부를 첫 실행에 통과했다. 시험 보조에서 한 가지를 알아냈다: `RawSquidNeighbors`가 생성 때 타입/프레임 표를 복사하므로(C++은 live span) 장면 준비 뒤 noIsland 표를 심으면 이웃 탐색기를 다시 만들어야 해 `Scene.refreshNeighbors()`를 두었다.

## 3. 경계

- CD 10.72(프레임 1바이트·H 조회 1.0001 보정·별도 소유자 지도 `ownerMap`)와 original1037 구간은 범위 밖이다. 실제 월드의 표시 갱신(가상 +0x88)·상세 문자열 소유자·`typeWas` 전역(MP 수신 writer)은 월드 조립에서 공급한다.
- cpppj 통합 검사 `island_postpop_real_raw_modules_build_support_and_propagate_owner`(실제 `SquidFactory`·`SquidOwner`·`SquidPop`·`SquidPostPop`로 3×3 noIsland를 놓는 시나리오)는 `SquidFactory`/월드 조립 단위에서 대조한다.

## 4. 다음 시작점

`RawBridgeConnect`의 섬 받침 접두(`setIslandPrefix`)와 이 접두를 `SquidPostPop`에 실제로 연결하는 월드 조립은 WB05에서 한다. WB04 안의 나머지는 Player 나머지(`RawPlayerWorld`·`RawPlayerTechnology`·`RawPlayerRegular`·`RawPlayerTemplateState`·`RawPlayerGeyserBounds`·`RawPlayerPlacementAnchor`), 종류별 PostPop, `SquidDisplay`/`SquidFrame`이다.
