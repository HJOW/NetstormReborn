# DestroyPatch 진단 정상 복귀 — WB04-E1321

2026-11-24, HJOW-Athlon. 사용자 결정(원본처럼 진단 후 계속)에 따라 E1273의 보고 후 중단 적응을 해제했다. 비교 근거는 C++ `RawSquidDestroy.cpp`의 `DestroyPatch`·`ReleasePatch`와 `SquidDestroyLifecycle.cpp`의0x771이다. 원본을 새로 실행/디컴파일하거나 cpppj를 변경하지 않았다.

`RawSquidDestroy.bindPatchDiagnostics`로 선택하는 실제 PATCH 경로를 추가했다. 원본처럼 dead를 먼저 표시하고, 진단 복귀 뒤 현재 free/권한을 재조회한다. pre 깊이를 늘린 뒤0x27 수준의 `destroying NAME SID` 로그를 남기고, 가상 pre→현재 종속 순회→전파/선택→현재 VT의 Unpop→post→반납 순서를 유지한다. 종속 next는 두 번째 free 진단 전에 보존한다. Form Unpop 뒤 원문에 없는 void 결과 검사를 추가하지 않는다.

`SidPool.releasePatch`는 void/free 진단이 복귀한 뒤 현재 TYPE를 삭제 기록에 넣고 원래 FIFO/예약 꼬리 순서로 반납한다. 물리 범위·종속 순환·알 수 없는 VT·도달한 미연결 장치는 기존 호스트 오류 경계로 남는다. 독립 `release`·진단을 켜지 않은 삭제기와 장부의 typed 선검사는 유지해 기존 원본 fixture를 변경하지 않는다.

실제 앱은 첫 World 전에 `RawSurfaceStorage.enablePatchDestroyDiagnostics()`를 켜고 같은 장기 Client의 표시 진단과 로그를 빌린다. 장부는 이 경로에서 `!isForm()`0x771을 보고한 뒤 현재 TYPE를 읽고, 원문보다 먼저 typed 선검사를 실행하지 않는다. 이는 일반 프레임 예외를 자동 복구하는 정책과 별개다.

새 삭제 시험8건에서 진단13지점(0x24d·269·276·27d·27e·280·28f·2a2·2a9·15c·15d·123·771)을 전부 실행했다. 진단 콜백이 free·TYPE·next·깊이를 바꾸는 입력을 포함해 복귀 후 재조회/저장 순서와 SID/깊이 결과를 단언한다. 기존 삭제·장부·가상 분배·SID 검사41/41, 신규8+장기 저장소13 검사21/21 통과. 기존 destroy fixture의10.78 Destroy600·Bridge64행도 보존된다. `webpj/extracted/webpj-e1321-{existing,focused,patch}.log`.

Chromium5/5 통과: 실제 앱의 PATCH 모드 선택과 Tutorial1 4:3·16:9의90프레임 화면0픽셀/90·130프레임 덤프 일치, 고정 시각 가드, Tutorial3 실제 Heal 시전 효과/반납과 주문 보유 골렘의 포획/Drop. `webpj/extracted/webpj-e1321-browser.log`. webpack 번들3,427,789바이트로 한도 통과. 정기 Node26.13.0 전체 회귀는6,845/6,845 통과·실패/취소/미실행0(463초, `webpj/extracted/webpj-e1321-node26.log`)다. E1316 전체에서 남았던 시간 동기화의 낡은 partial 기대값 교정도 이번 전체에서 통과했다.

같은 사용자 지시에 따른 C++1339 주문/포획 교정(`13e74579b`)은 앞선 E1318/E1319와 대조해 대응표 두 행의 base를 갱신한다. DestroyPatch 외 Native 전역·할당 소진과 일반 예외 복구 정책은 해당 후속 범위로 남긴다.
