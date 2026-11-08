# webpj 건설 표시 시스템(ConstructionVisualSystem) 이식 — WB04-E194

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백아흔네 번째 부분이다. E128~E130 에서 옮긴 건설 표시 몸체(생성·프레임·수명)를 E192 공통 표시 저장소 위에서 실제 ProcessForm·Kernel 소유권에 연결하는 `o/ConstructionVisualSystem`(process 17)과, 건설 Start 의 표시 효과를 이 시스템에 잇는 `MakeConstructionVisualSystemHooks` 를 옮겼다. 이 단위에서 E192/E193 연기·분해 시스템의 Host 부착 예외 경로도 함께 보강했다(3절).

- cpppj 기준선 `1f00a25a`. 소스·시험 모두 기준선 뒤 변경 없음(`git diff --numstat 1f00a25a HEAD` 0줄). 근거 커밋 `3fda610d`(소스·`ConstructionVisualSystemTests.cpp` 같은 커밋).
- 공유 장치: E192 `RenderProcessSystem`, E128 `RawConstructionVisual`, E130 `RawConstructionFrame`, E129 `destroyConstructionVisual`·`makeConstructionRegistryHooks`, E131 수집 항목, E108 `RawContainedLookup`·`makeContainedConstructionHooks`, E132 `RawConstructionProcess`/`ConstructionProcess`, D5c `SquidProcessHost`.

## 1. 옮긴 것

`www/core/construction-visual-system.js`(DOM 없음)

- `ConstructionVisualProcess`(SquidProcess 파생) — `fields`(건설 프레임 필드: visual·delta·scaledDelta)·`registration`(공통 등록 필드)·`ready`. `runFrame`/`draw`/`destroy` 를 시스템으로 넘기며 C++ 소멸자는 `destroy()` 다.
- `ConstructionVisualSystem(render, frameState, palette, random, visualHooks, frameHooks, release)`(다른 효과와 같은 저장소 공유)·`ConstructionVisualSystem.standalone(pool, host, visualState, …, registryHooks, …)`(저장소 직접 소유, cpppj 첫 생성자). 생성 전에 같은 풀·반납 관찰과 실제 표시/프레임 몸체의 필수 경계를 검사한다.
- `add(parent, instant)` — 중첩 거부 → 대기 객체 → `track` → `RawConstructionVisual.create`(공통 목록 등록 안에서 실제 client Form 부착 → 캡처/입자 초기화) → 생성 중 삭제 확인 → 필드 저장·등록 영역 동기화 → 준비 완료. 실패하면 부착된 객체는 Host 삭제(flags 0x10), 부착 전 객체는 직접 소멸한다.
- 프레임은 준비 완료 객체만 실행하고(kill 은 그 프로세스의 Host 삭제), 프레임 뒤 토큰 조회로 생존을 확인한 객체에만 영역/깊이를 등록 필드로 돌려 쓴다. 출력은 kill 이 없는 몸체로 현재 출력 버퍼를 제출한다. `draw`/`collect`/`drawCollected`(16바이트 수집 항목, kind 3 이 아니면 거부, 삭제된 토큰은 생략)/`drawToken`/`slots`/`size`/`dispose`.
- `makeConstructionVisualSystemHooks(pool, system, hooks)` — 건설 Start 의 `effect` 를 `system.add` 로 잇는다(다른 풀 거부).

## 2. 검증

`tests/construction-visual-system.test.mjs`(10건). cpppj `ConstructionVisualSystemTests.cpp` 15건 중 `o/` 부품만 쓰는 10건을 10.78 판으로 1:1 옮겼다.

- 부착/실행/출력(카메라 0 → (326,230), 카메라 4/5 → (322,225))/HP 0 자기 삭제와 빈 칸 재사용(일반/즉시, 목록 확보 160 한 번), Start 의 표시(17)+HP 진행(30) 두 프로세스가 기본 10초/지정 8초로 완주(HP 40/50)하고 다음 순회에서 표시 종료, contained 조회가 가장 최근 표시만 지움(Regular/부모 링크 보존), 부모 삭제의 전체 반납과 삭제 깊이 0, 시스템 정리, 생성 실패 세 가지(객체 136바이트 확보·캡처 예외·TYPE 100 부착 예외)의 되돌림, 캡처 중 재진입 차단, 출력 중 삭제와 목록 40→80 확장(확보 160/320), 수집 뒤 삭제/재사용 토큰과 다른 종류 거부, 다른 풀/경계 누락 거부와 캡처 중 취소.
- 합성 TYPE 표는 cpppj `Types()` 에 웹 Host 가 ProcessForm 을 만들 때 읽는 내장 Form 생성자 주소 넷(cpppj `MakeFormTypeInputs` 와 같은 입력)을 더했다. 기대값 계산에는 쓰지 않는다.
- 옮기지 않은 5건: `RenderCollection_VisibleConstructionSubmission`·`RenderCollection_SmallTiesAndInvalidSortCoordinates`·`DrawSort_ConstructionUsesNativeTieOrder`·`DrawSubmission_ActualConstructionMergesWithOtherKinds`·`DrawSubmission_ConstructionDeletionDuringMixedDispatch` 는 client `ConstructionVisualRenderer`(`DrawVisibleConstruction`)·`DrawSubmission`(`DrawVisibleScene`)을 쓰므로 그 client 단위(PORT_MAP todo)에서 옮긴다. 10.72(CD) 반복도 범위 밖이다.

최초 실패와 교정: 10건 중 9건은 첫 실행에 통과했다. 생성 실패 시험의 TYPE 100 단계에서 웹 Host 의 `attach` 가 null 이 아니라 **예외**로 실패하자, 대기 객체를 넘긴 뒤 소멸시키지 않아 등록·조회가 남고(목록 1번 칸) 이후 정리에서 부착되지 않은 객체를 Host 삭제하려다 다시 실패했다. cpppj 는 `Attach` 의 `unique_ptr` 매개변수가 스택 해제 중 소멸해 `Destroy` 가 돈다. 그래서 부착 callback 에서 `host.attach` 예외도 잡아 객체를 소멸시킨 뒤 다시 던지도록 고쳤다.

## 3. E192/E193 보강

같은 누락이 `smoke-visual-system.js`(E192)와 `dissolve-visual-system.js`(E193)의 부착 callback 에도 있어 같은 방식으로 고쳤고, 웹 추가 회귀 시험을 하나씩 붙였다(`tests/smoke-visual-system.test.mjs`·`tests/dissolve-visual-system.test.mjs`: Host 부착 예외 뒤 등록/조회/Kernel 0·객체 반납 1회·분해는 짝 없는 더미 정리·다음 생성 정상). cpppj 이식 시험 결과는 바뀌지 않았다.

## 4. 경계와 남은 것

- 실제 Session/Client 월드에 시스템을 만들어 건설 시작에 연결하는 일, client 쪽 가시 수집 정렬/출력(ConstructionVisualRenderer·DrawSubmission)·장치 출력은 순서4~5 몫이다(PORT_MAP session 0 유지).
- 10.72(CD)와 10.37 관측은 범위 밖이다.

## 5. 다음 시작점

[LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 "입자 표시 마무리": `FieryTailVisualSystem`(cpppj 시험이 이 시스템과 같은 저장소를 씀) → `LightningVisualSystem` → `ShrapnelVisualSystem`·`EvilRayVisualSystem`, 그리고 선행 부품이 필요한 `RawBurningFrame`→`BurningVisualSystem`.
