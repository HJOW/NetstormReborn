# webpj raw 풀 위의 다리 붕괴(RawBridgeDecay·RawBridgeDecayScan) 이식 — WB04-E45

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 마흔다섯 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawBridgeDecay`(10.78; 원본 004227e0 한 칸 처리와 00421c30 수명 감소)와 `o/RawBridgeDecayScan`(00422bc0 프레임 스캔)을 옮겼다. [E44](webpj-bridge-decay-20261108.md)의 순수 논리(정수 스냅샷)를 같은 원문 순서로 **현재 raw 슬롯**에 직접 적용하는 실행기이다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

`www/core/raw-bridge-decay.js` — `RawBridgeDecay`, `RawBridgeDecayScan`

- `RawBridgeDecay(pool, types, frames, surface, spots, graphs, mode, hooks, {authority, checking})`: 슬롯·표면 지도(`SquidHash` 0단계 머리)·Graph 표는 호출 중에도 현재값을 읽는다. 훅 `{destroy, notifySurface, refresh, carriers, crackSound, assertion}`는 도달한 지점에서만 요구하며 누락 시 앞선 실제 쓰기를 남긴 채 오류를 낸다.
  - `cell(root)` [004227e0]: Graph 번호 254/표 밖 거부 → 표면 수 5 미만이면 `destroy(0)` → 단단한 프레임이면 끝 → flag 8 이웃(`RawSquidNeighborWalk` 표면 지도 순회) 없음이면 수명 1 감소 → 앞 10개 이웃으로 열린 방향 판정 → 방문 목록(`#collect`: 부모만 제외·고리는 호스트 안전 중단) → 비영 수명 최소값(checking이면 7 초과에서 진단 0x268, 복귀 뒤 7로 제한) → 방문 칸마다 현재 dead가 아니면 목표 수명까지 감소 → 금 소리.
  - `applyLife(sid, reduction)` [00421c30]: signed 산술·음수는 0, 서버(현재 서버 전역 우선)이고 0이면 `destroy`(J·K는 판자 제거 플래그), 아니면 수명 4비트(`(remaining*8)&0x78`)를 쓰고 표면 알림 → 금 간 전환(5 이상→5 미만)이면 금 간 프레임 직접 쓰기+`refresh`, 아니면 디버그 갱신 → 새 수명 5 이하면 `carriers`. 정상 7 초과를 미리 거부하지 않고 원문 4비트 쓰기를 그대로 둔다.
- `RawBridgeDecayScan(pool, types, state, period, mode, cell)`: 정지·편집기·권한 관문 뒤 기한을 넘기면 범위 전체, 아니면 `trunc(delta/period × range)`개만 방문(끝 슬롯 포함). 매 칸 효과가 바꾸는 커서/끝 번호/주기/기한을 다음 방문에 다시 읽고, 각 칸의 예외는 삼키되 이미 발생한 효과는 되돌리지 않는다. 방문 조건은 free·dead·void가 아니고 bridge genus이며 extra의 abstract(1)가 아닌 슬롯(buried(8)은 방문 대상). 재설정(`next = period + now`, 커서 복귀)은 모든 칸 처리 뒤이다.

## 2. 검증 (`tests/raw-bridge-decay.test.mjs`, 6건)

- `bridgedecay-x86.tsv`: Cell 4241행(사건·결과 상태)과 Life 3264행(수명 7 초과 400행 포함)을 실제 슬롯의 float 이웃/WORD/프레임 쓰기로 재생. 표면 지도는 원문 fixture처럼 발자국 전체를 번호로 채운 명시 입력이고, 이웃 순회는 E39의 `RawSquidNeighborWalk` 표면 지도 모드를 재사용해 같은 결과가 나옴을 확인했다(별도 `RawCanonSurfaceWalk` 이식 불필요).
- `bridge-decay-checking-x86.tsv`: 272입력의 진단 문자열 바이트·정상 복귀/중단·진단 중 바뀐 현재 WORD 재조회·최종 checking DWORD.
- `bridge-decay-scan-runtime-x86.tsv`: 73행의 호출 순서·가변 범위·기한 비트(주기/끝 번호/커서/기한을 칸 효과가 바꾸는 변경 4종).
- 계약: 만든 뒤 바뀐 전체 checking DWORD(0x80000000)와 진단 장치 누락은 수명 쓰기 전에 실패, 구성 오류 거부, 스캔의 칸 예외 삼킴과 정지/편집기/권한 없음 무변경.

최초 실패와 교정: 모듈은 대형 fixture(Cell 4241·Life 3264·checking 272·scan 73) 전부를 첫 실행에 통과했다. 계약 테스트 한 건의 기대값만 바로잡았다(extra 8 buried 슬롯은 방문 대상이고 abstract 1 만 제외).

## 3. 경계

- `RawBridgeCarriers`(위험한 다리 위 사제/rainWalker 대피)는 `RawPathRuntime`(이동 시작/Form) 의존이라 다음 단위이다. 월드의 실제 서버/권한 전역(`RawAuthorityState`)과 Graph 저장소 연결, 실제 destroy/표시/소리 훅은 월드 조립(WB05)에서 공급한다.
- CD 10.72 분기와 `RawCanonSurfaceWalk` 배치 decoder 변형(`PriestPlainCanonType`)은 범위 밖이다.

## 4. 다음 시작점

`RawBridgeCarriers`(+`bridge-carrier-retreat`)와 `RawIslandPostPop`의 섬/표면 접두 본체, Player 나머지(`RawPlayerWorld`·`RawPlayerTechnology`·`RawPlayerRegular`·`RawPlayerTemplateState`·`RawPlayerGeyserBounds`·`RawPlayerPlacementAnchor`), 종류별 PostPop, `SquidDisplay`/`SquidFrame`이다.
