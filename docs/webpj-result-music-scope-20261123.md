# 승리·패배 곡(fanfare·defeat)의 웹 범위 판정 (WB04-E1238)

E1228 읽기 전용 감사가 "승리/패배 곡(`fanfare.mus`·`defeat.mus`) 미재생 — `native-main-menu-state.js`·`native-battle-return-state.js`가 앱에서 import 되지 않고 구동기의 `resultState`가 항상 0이라 미션 종료가 결과 곡 없이 바로 `ser22.mus`로 간다"를 차이로 남겼다. 이 차이가 단독 미션(캠페인 31경로·데모 3경로·스커미시)에서 실제로 도달하는지 cpppj 소스(10.78 복원)와 웹 모듈로 판정했다. **도달하지 않는다. 연결하지 않고 웹 범위 밖으로 닫는다.** 원본도 단독 미션에서는 결과 곡을 틀지 않으므로 이 항목은 차이가 아니다.

## 근거

1. **미션을 떠나면 결과 DWORD는 0이 된다.** 미션 이탈의 공통 중단(원문 `004b6350`, cpppj `RawBattleLeave.cpp` `PrepareBattleAbortState`, 웹 `core/raw-battle-leave.js` `prepareBattleAbortState`)은 `!missionPost && !editor && !metamap`일 때만 전투(`battle`)를 State 6으로 보내고, **그 밖(미션 후·Editor·메타맵)은 `outcome=0`·다음 상태 3(메인 메뉴)을 쓴다**. 웹 단독 미션은 `client-gameplay-globals.js:127`이 `world.missionPost = true`를 쓰고 스커미시도 같은 `_clientGuide.begin` 미션 경로라 모두 이 가지다.
2. **State 3은 결과 1·2나 출처가 있을 때만 결과 곡을 요청한다.** `native-main-menu-state.js`(cpppj `NativeMainMenuState.cpp:30~34`)는 `outcome === 1` → `fanfare.mus`, `outcome === 2` → `defeat.mus`, 출처(`0055a420`)가 0이 아니면 `fanfare.mus`를 요청한다. 미션 복귀는 결과가 0이고 출처는 Editor 시험 전투(웹 제외)만 쓰므로 요청이 한 번도 없다. 배경도 `victory`·`defeat`가 아닌 `titleMenu`로 돌아간다.
3. **결과가 1·2가 되는 쓰기는 전투 종료 프레임뿐이고 그 관문이 `!missionPost`다.** `RawBattleCompletion::Frame`(cpppj `RawBattleCompletion.cpp:65~96`, 웹 `core/raw-battle-completion.js:99`)은 `!battle || missionPost || !serverConnection || phase != 14`이면 곧바로 돌아온다. 이 프레임이 결과 1(승리)·2(패배)·5·6을 쓰고 State 6 복귀를 예약하므로 **멀티플레이 전투(LAN TCP, 웹 범위 밖)에서만** 도달한다. 웹 앱은 이 프레임(`pollClientBattleCompletion`)을 호출하지 않는다.
4. **음악 감독도 같은 관문을 한 번 더 둔다.** `SceneMusic.request`는 `fanfare.mus`를 `resultState === 1`일 때만 받는다(cpppj `SoundSceneMusic.cpp:67`). 웹 구동기의 `resultState`가 0으로 고정돼 있어도 단독 미션에서는 원본과 같은 결과(승리 곡 거절)다. `defeat.mus`는 이 관문이 없으나 요청하는 쪽(State 3·State 6)이 결과 2일 때만 부른다.
5. **단독 미션의 승패는 곡 없이 안내 창으로 알린다.** 승리·패배는 대본의 `[Succeeded]`·`[Failed]`·`BadTeamDead` 안내 Tell(Leave·Next·Replay 단추, E1152·E1161에서 확인)이며 장면 음악 감독은 그대로 원소 곡을 순환하다가 이탈 때 메뉴 곡으로 돌아간다(E1119).

## 시험

`tests/mission-result-music-scope.test.mjs`(4건)가 실제 웹 모듈 세 개를 이어 판정을 고정한다.

- 미션 후(`missionPost`) 이탈은 복귀 직전 결과가 0·1·2·3 어느 것이든 결과를 0으로 쓰고 다음 상태를 3으로 예약하며, State 3 부모가 곡을 요청하지 않고 `titleMenu` 배경·저장 삭제 한 번(공통 중단)뿐이다.
- 데모·스커미시처럼 전투 표시가 없는 미션 후 이탈도 같다.
- 대조군: 미션 후가 아닌 전투는 결과 0이면 3을 쓰고 State 6을 예약하며, 결과 1·2가 State 3에서 `fanfare.mus`·`defeat.mus` 요청과 `victory`·`defeat` 배경을 만든다(웹 범위 밖 경로가 모듈에서는 살아 있음을 확인).
- `SceneMusic`의 승리 곡 관문(결과 1에서만)과 패배 곡 무관문.

## 결론

- E1228 감사의 차이 ①을 닫는다. 이미 이식된 `native-main-menu-state.js`·`native-battle-return-state.js`·`raw-battle-completion.js`·`client-battle-completion.js`는 소스 계약 시험으로 남기고 앱에는 연결하지 않는다.
- 멀티플레이 전투(비미션) 경로를 구현하게 되면 그때 전투 종료 프레임 호출(`pollClientBattleCompletion`)·State 6 설치·`SceneMusicDriver`의 `resultState` 공유(`ClientAudio::ResultState()`와 같은 주소로 State 3·로그인 `requestPhase`가 함께 쓴다)·`services.requestMusic`로 결과 곡 요청을 연결한다.
- 기준선 뒤 cpppj 변경 없음(`RawBattleLeave.cpp`·`NativeMainMenuState.cpp`·`NativeBattleReturnState.cpp`·`RawBattleCompletion.cpp`·`SoundSceneMusic.cpp`).
- 실제 브라우저 청취는 하지 않았다(소스·모듈 판정). 원본 실행 영상으로 단독 미션 승리 뒤 곡을 확인하면 사용자 확인 범위에서 보강할 수 있다.

## 대응표 정리

`SoundSceneMusic.cpp` 행의 "남음: 결과 곡 요청"과 E1228 차이 ①을 위 판정으로 지웠다. 남은 차이는 ②(State7 난수 한 칸)·③(곡 길이 고정 표)·④(천둥 효과음 경로)·⑤⑥(희생곡)이다.
