# webpj DestroyPatch 진단 열두 지점과 rememberPlacement 판정 (WB04-E1273, 2026-11-23)

> 2026-11-24 E1321: 사용자가 원본처럼 진단 후 계속하도록 결정해 아래 DestroyPatch의 중단 적응을 해제했다. 독립 typed fixture는 보존하고 실제 앱에 원본 순서의 PATCH 경로를 연결했다. [현행 구현·검증](webpj-destroy-patch-continue-20261124.md)

[LEFT_JOBS 점검](webpj-left-jobs-audit-20261123.md)(E1263)이 4D-8a 에 남긴 세 가지를 원문과 다시 대조해 판정했다. 코드는 바꾸지 않았다.

## 1. Native DestroyPatch `AssertPatch` 열두 지점 + 0x771

- 원문 `RawSquidDestroy::DestroyPatch`(`cpppj/src/o/RawSquidDestroy.cpp:76~148`)와 `ReleasePatch`·`SquidDestroyLifecycle.cpp:164` 는 불변식이 깨지면 `AssertPatch` 로 **진단을 표시하고 계속한다**(주석 "원문 단언은 throw로 바꾸지 않으며, 정상 복귀하면 caller가 현재 상태를 다시 조회한다"). 지점: 0x24d(이미 free)·0x269(client SID 아님)·0x276(pre 깊이)·0x27d·0x27e·0x280(종속 체인의 free/contained)·0x28f(루트 free)·0x2a2(유효 SID)·0x2a9(post 깊이)·0x15c·0x15d(`ReleasePatch` 의 void/free)·0x123(SID 꼬리)·0x771(`!isForm()`).
- 웹 `core/squid-destroy.js` 는 같은 지점에서 오류(`Destroy pre completion`·`Destroy dependent state` 등)로 중단한다.
- **판정: 의도된 차이로 유지한다.** 이 지점은 슬롯 상태·종속 체인·깊이 불변식이 깨진 손상 상태에서만 갈리고, 정상 경로(31개 미션 연속 시작·스커미시 18분·장시간 e2e)에서는 한 번도 나지 않았다. 손상된 풀에서 효과를 이어 가는 것보다 보고 후 중단이 복제본에서 안전하고 원인 추적에도 낫다. 원본과 같은 표시 후 계속으로 바꾸려면 `destroy` 의 `throw` 열두 곳을 `hooks.assertion(expression, file, line)` 호출 뒤 계속으로 바꾸고 원문 `DestroyPatch` 의 순서(dead 표시·로그 `destroying NAME SID`·효과 방출)를 그대로 옮겨야 하며, 이는 L1 관측이 고정한 typed 경로의 재작성이라 이번에는 하지 않는다.

## 2. `rememberPlacement` 미연결

- 웹 `squid-destroy-lifecycle.js` `preDestroy` 는 선택된(`state.selected === sid`) 비정상 상태(`!ordinary`) 이동체(`flags2 & 0x30000`)가 삭제될 때 위치·종류를 기억하고 `pendingPlacement=true` 를 세운다. 그 뒤 `squid-postpop.js` 가 "배치 선택 해제 미복원" 오류로 막는다.
- cpppj 도 같다: `SquidDestroyLifecycle.cpp:169`(기억)·`SquidPostPop.cpp:291`(`std::logic_error("postPop 배치 선택 해제 미복원")`)·`SquidPostPopPatch.cpp:23`(`recheckSelection` 이 없으면 같은 오류). **원본 동작의 복원이 cpppj 에도 없다.** 웹은 cpppj 와 같고 선택된 비정상 상태 유닛이 삭제될 때만 도달한다. 웹이 cpppj 보다 먼저 복원하면 원본 확인 없이 추측을 옮기는 것이므로 cpppj 가 복원할 때 함께 옮긴다.

## 3. BridgeRegular 현재 전역 상수 스냅샷

- `BindCurrentState`·`BindCurrentConnectorType` 의 boss 항상 true·연결 TYPE 82/155 는 상수다. 웹 단독 플레이(캠페인·데모·스커미시)는 항상 서버(boss)이고 10.78 타입 번호는 고정이라 원문이 읽는 값과 같다. 멀티플레이만 값이 갈린다(웹 범위 밖).

## 대응표

`RawSurfaceDeletion`·`RawSurfaceLifetime` 두 행을 partial→port 로 올리고 위 판정과 되돌리는 방법을 행 메모에 적었다(partial 12행·12,748줄, `node webpj/tools/port-map.mjs --check` 통과). 원문 순서로 바꿀지는 사용자 결정으로 열어 둔다.
