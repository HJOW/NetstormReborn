# webpj 미션 프레임 순서를 원본 주 루프에 맞춤 (WB04-E1266, 2026-11-23)

[LEFT_JOBS 점검](webpj-left-jobs-audit-20261123.md)(E1263)이 4D-4a 에서 다시 짚은 프레임 순서 차이 두 건(E1230 이 "미미"로만 적고 고치지 않음)을 원문 순서로 맞췄다.

## 원문 순서 (cpppj `ClientMain.cpp` 596~632행, 주 루프 10번)

`UpdateAmbientSound` → `UpdateAmbientWind` → `UpdateMonitorTime` → `RunClientBridgeDecay`(00422bc0) → `audio_->SceneFrame()`(469f60) → `RefreshMinimap`(481320) → `CleanupGumps`(439b13) → `IdleGumps`(439b18) → `PrepareKernelFrame` → Kernel → `FinishKernelFrame` → …

## 이전 웹 순서와 차이

- 장면 음악 `_sceneMusic.frame()` 이 입력 처리보다 앞(소리 재생 수 갱신 직후)에 있었다. 장면 음악이 곡을 고를 때 쓰는 Game 난수는 주변음·바람과 같은 줄이라 이 순서가 난수열을 정한다.
- 미니맵 갱신 `hud.refresh()` 가 Kernel 뒤에 있어 원문보다 한 프레임 앞선 모습을 그렸다.
- 다리 붕괴 검사 `scanBridgeDecay` 가 `prepareKernelFrame` 안에 있어 원문과 달리 미니맵 갱신·장면 음악보다 뒤였다.

## 바꾼 것

- `www/core/campaign-game-world.js`: `runBridgeDecay()` 를 새로 두고(현재 프레임 시각으로 다리 수명 검사를 한 번), `prepareKernelFrame()` 에서 그 호출을 뺐다.
- `www/netstorm.js` `_onFrame`: 장면 음악 호출을 입력 앞에서 빼 `_runAmbientFrame()` 바로 뒤로 옮기고, 그 앞에서 `runBridgeDecay()` 를 부른다(미션 화면일 때). 미니맵 갱신은 Gump 정리·Idle·Kernel 앞으로 옮겼다.
- 결과 순서: 주변음·바람 → 다리 붕괴 → 장면 음악 → (생산 프레임) → 미니맵 갱신 → Gump 정리·Idle → Kernel 준비·실행. 원문과 같다(생산 프레임은 원문에서 Idle 안쪽 일이라 미니맵 앞에 둔 위치는 이전과 같다).

## 검증

- `tests/raw-campaign-session-load.test.mjs`: 다리 붕괴 검사가 `runBridgeDecay` 에서 정확히 한 번 불리고 `prepareKernelFrame` 은 부르지 않음, 시각 공급자가 없으면 두 함수 모두 오류.
- e2e 11건 통과(`minimap`·`mission-build-placement`·`scene-music`·`english-tutorial2`·`mission-priest-training-flow`, 로그 `extracted/webpj-e1266-e2e.log`), `raw-campaign-session-load.test.mjs` 129건·`port-map.test.mjs` 7건 통과.
- 대응표 `ClientMain` 행을 partial→port 로 올렸다(partial 19행·13,276줄, `node webpj/tools/port-map.mjs --check` 통과). 남은 PollConnectionHealth·UpdateMonitorTime·PollBattleCompletion 은 단독 미션에서 관측 효과가 없음으로 확정했다(LEFT_JOBS 4D-7b).

## 남은 것

- 순서는 원본 실행으로 확인한 것이 아니라 cpppj 복원 코드(`ClientMain.cpp`)와의 대조다.
