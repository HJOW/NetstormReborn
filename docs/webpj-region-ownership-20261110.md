# webpj 지역 소유 투표 이식(RawRegionOwnership) — WB04-E249

2026-11-10, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 단계 3-B의 마지막 항목이다. cpppj 기준선 `1f00a25a`의 `o/RawRegionOwnership`(원본 00455380; 추가 목록·신전 목록의 객체를 지역 getter 로 지역에 묶어 질의 좌표의 지역과 같은 객체의 소유자에 한 표씩 더하고 동률/중립과 신전 테마로 도장 요청을 내는 투표)과 기준선 뒤 `f0d065dc`가 더한 Native 몸체를 옮겼다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만).

## 1. 기준선 뒤 변경 판정 (3.6절)

`git diff --numstat 1f00a25a HEAD`는 `.cpp +49/−3`·`.h +4/−0`이고 이 파일을 건드린 커밋은 `230528e3 20261108-162 CP`(기준선 뒤 주소·master·Flyer·Outpost 접합)와 `f0d065dc 20261109-86 CP`(실제 섬 영역 도장과 신전 소유자 삭제 부모 장기 연결)이다. 변경은 모두 Native 몸체의 추가이고 typed `Update`의 결과는 기준선과 같다(typed 440행 fixture 로 재확인).

- **② 재반영:** `typeTheme` 훅(현재 TYPE 의 `+0x98` DWORD, 있으면 독립 테마 표를 요구하지 않음), `UpdateNative`/`VoteNative`/`NativeListValue`를 웹에 `updateNative`·`#voteNative`·`#nativeValue`로 옮겼다. 상태의 공유 월드 다시 그리기/미니맵 DWORD 는 C++ 포인터 대신 `{ value }` 상자(`sharedWorldRedraw`/`sharedMapRevision`)로 빌린다.
- 행 base 는 판정한 `f0d065dc`로 올렸다. Native 호출자(client 영역 소유자 `ClientConstructionRegions`·`RawCampaignOwnership`, 시험 `ClientRegionOwnerTests`)는 4-B/4-D 몫이다.

## 2. 옮긴 것

`www/core/raw-region-ownership.js` — `RegionOwnershipState`, `RawRegionOwnership(pool, additional, temples, themes, state, hooks)`, `makeTerrainRegionOwnershipHooks(pool, terrain, hooks)`

- `update(x, y, mode = 0)`(typed): 좌표 절삭(0 방향·int 범위·NaN 거부)을 투표 전에 한다 → 영향 기록을 지우고(mode 10 이면 지우지 않고 지역마다 표를 초기화하지 않은 누적 재투표) 두 목록을 투표 → 최대 표와 동률 판정 → 동률/중립이면 소유자 1 도장 뒤 영향 기록 삭제 → 아니면 소유자 0 도장, mode 0 일 때만 같은 소유자의 마지막 신전 테마(옵션 꺼짐 0, 종류 0→2·1→3·2→1) → 최종 도장 → 월드 다시 그리기 증가(DWORD 감김)와 미니맵 요청 1 쓰기 → 영향 지역이 2개 이상이면 mode 10 재투표.
- `updateNative(x, y, mode = 0)`: 같은 흐름이되 투표/테마 순회가 활성 항목 사본이 아니라 현재 **signed** count 와 현재 저장소 항목을 매번 읽고(음수는 배열을 읽지 않음, 저장소 물리 밖은 항목 선택 때 거부), 좌표 절삭은 실제 도장 위치에서 한다.
- `makeTerrainRegionOwnershipHooks`: 지역 getter 를 `RawCanonPlacementTerrain.regionAt` 앞에 편향 snap(float 비트 `0x3f7fff58` 을 double 로 더해 0 쪽으로 절삭, float 저장 뒤 다시 절삭) 으로 연결한다. 다른 풀이면 거부한다.
- **웹 제외:** `MakeOutpostRegionOwnershipHooks`(outpost 등록/삭제 뒤 지역 통지)는 `RawOutpostLifecycle`(멀티플레이 전용, exclude-mp)과 함께 웹 범위 밖이다. cpppj `RegionOwnership_OutpostLifecycleImmediatelyChangesVotes`(Damageable 삭제 준비까지 거치는 outpost 통합 시험)도 같은 이유로 옮기지 않았다.

## 3. 검증 (`tests/region-ownership.test.mjs`, 6건)

- 원본 10.78 관측 `regionownership-x86.tsv` 440행(`originals`): 12개 목록 장면 × 여섯 좌표(소수 경계 포함) × 세 소유자 변형 × mode/옵션/지역 경계에서 지역 getter 호출(R)·옵션 조회(C)·도장(P) 사건 순서와 인자, 누적 영향 지역, 월드 다시 그리기/미니맵 DWORD 와 입력 목록 배열 불변을 대조한다. 지역 getter 는 실제 `RawCanonPlacementTerrain` 과 snap 어댑터를 거친다. 같은 440행을 `updateNative`로 한 번 더 재생한다(건강한 목록에서 typed 와 같아야 한다). CD·10.37 행(880행)은 범위 밖이다.
- 계약: 필수 경계 누락·같은 목록 두 개·테마 표 부족·다른 풀 거부, NaN/범위 밖 좌표 거부(typed 는 상태 변경 전, Native 는 영향 기록을 지운 뒤 첫 도장에서), 소유자 9 이상·count 손상 거부, 공유 월드 다시 그리기 증가와 미니맵 1 쓰기(두 소유자가 서로 다름, 사후 사본 동기화가 아니라 마지막 효과 뒤 현재값), Native 의 음수 count·저장소 물리·현재 TYPE 테마 장치.

최초 실패와 교정: 440행 두 재생은 첫 실행에 통과했다. 직접 쓴 계약 검사가 처음 한 번 실패했는데, Native 가 NaN 좌표에서 영향 기록을 지운다는 원문 순서를 내가 typed 와 같다고 잘못 적은 기대값 때문이어서 기대를 원문 순서(Native 는 도장에서 절삭)에 맞게 고쳤다. 변형 20개를 넣어 확인했고 16개는 바로 탐지했다. 살아남은 넷 중 셋은 입력이 없어서였다: snap 편향 1 ulp 변형(`0x3f7fff59`)과 좌표 절삭 상한(`2^31` 정확히)을 경계 입력으로, Native 신전 테마 순회의 signed/unsigned 변형을 음수 count 장면으로 닫았다. 나머지 하나(표 감김 `| 0` 제거)는 `Int32Array` 저장이 이미 DWORD 로 감겨 결과가 같은 동등 변형이다.

## 4. 경계와 다음 시작점

- 아직 앱 수명에 연결되지 않았다(PORT_MAP session 0). 실제 World 의 목록(Player 연결 `bindCurrent`)과 현재 TYPE `+0x98` 장치, 월드 다시 그리기/미니맵 소유자는 4-B·4-D 에서 잇는다.
- 이 단위로 `RawRegionOwnership`은 PORT_MAP `port`(base `f0d065dc`)이고 **3-B 가 끝났다**(8/8). 다음 단계는 3-C 자산 정기 처리·피해/삭제 수명이며 첫 항목은 `EnergyProviderVtables`다.
