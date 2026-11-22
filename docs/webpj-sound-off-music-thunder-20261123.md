# 소리 끄기의 음악 무음·천둥 효과음 경로 (WB04-E1239)

E1228 읽기 전용 감사가 남긴 두 차이 — ③ "Sound On"을 끄면 원본은 장치를 닫아 음악도 무음인데 웹은 음악이 계속된다, ④ 천둥 효과음이 `SoundPlayer`를 거치지 않아 게임 메뉴 음소거 중에도 들린다 — 를 cpppj 소스(10.78 복원)에 맞췄다.

## ③ 소리를 끄면 음악도 무음, 켜면 현재 곡을 처음부터

- **원본 근거.** `ReinterpretClientAudioOptions`(원문 `4355df..435680`, cpppj `ClientAudio.cpp`)는 순서가 옵션 전역 → `options.sound`가 참이면 장치 열기 / 거짓이면 `CloseDevice()` → 효과음·음악 음량 → `Select(현재 곡, options.music)`이다. 장치를 닫으면 음악 스레드까지 합류해 음악이 멈춘다(`SoundMusic::Play`는 `musicInitialized`·`device`가 없으면 곧바로 false). 현재 곡 이름은 `MusicSelectionState::Assign`으로 계속 기억하고 장면 음악 감독(`SceneFrame`)도 소리 옵션과 무관하게 돌아 곡 요청이 쌓이므로, 다시 켜면 열린 장치에서 마지막으로 선택한 곡이 **처음부터** 재생된다. 이미 같은 곡이 재생 중이면 `Play`가 활성 채널에서 곧바로 돌아와 옵션을 다시 적용해도(음량 변경 등) 곡이 다시 시작하지 않는다.
- **웹 변경.** `AudioSystem.setSoundEnabled(false)`가 효과음만 막고 음악을 계속 틀던 것을 고쳤다. 끄면 재생 중인 음악 소스를 멈추고(요청한 곡 `wantedMusic`은 유지), 소리가 꺼진 동안 `playMusic`(장면 음악 구동기·메뉴 곡 요청)은 곡 이름만 기억하고 `false`를 돌려주며, `false → true`로 켤 때 음악 옵션이 켜져 있고 기억한 곡이 있으면 `playMusic(wantedMusic)`으로 처음부터 다시 틀고 곡이 없으면 아무것도 시작하지 않는다. 같은 값(`true → true`)의 재적용은 재생 중인 소스를 건드리지 않는다(`_applySettings`는 모든 설정 변경마다 부른다). 결과 곡(fanfare·defeat)은 켠 뒤에도 한 번만 재생한다.
- **효과 범위.** 옵션 메뉴의 Sound On과 게임 메뉴 Options 항목 두 경로 모두 `_applySettings`를 거치므로 한곳 변경으로 둘 다 적용된다. 음악 옵션(`Play Music`)이 꺼져 있으면 소리를 켜도 음악이 나지 않고, 소리가 꺼진 채 음악을 켜면 소리를 켤 때 재생한다.

## ④ 천둥 효과음은 SoundPlayer 로

- **원본 근거.** `MakeSceneMusicHooks`(`SoundSceneMusic.cpp:131`)의 `weatherSound`는 `SoundPlayer::PlayByName(이름, 0,0,0,0,0)`이다. 따라서 게임 메뉴 `PushMute`(음소거 깊이), 소리 옵션(`state.enabled`), 동시 재생 한도(`maxSimulSounds`)와 감쇠가 그대로 적용된다.
- **웹 변경.** `netstorm.js` `_beginSceneMusic`의 `weatherSound`가 `audio.playSound(name)`(해독 버퍼를 직접 재생해 음소거·한도를 우회)이던 것을 `_clientSoundPlayer.playByName(name, 0,0,0,0,0)`으로 바꿨다. 재생기가 아직 없으면(소리 끔 실행) 건너뛰고, 재생 오류는 게임을 멈추지 않고 `reportGameError('scene-music')`로 기록한다. 미션 효과음은 `preloadSounds`에서 `thunderCrack.wav`까지 미리 해독해 둔 상태다(번개 의식·주문 소리가 같은 이름으로 이미 이 경로를 쓴다).

## 시험

- `tests/audio-sound-off-music.test.mjs`(4건): 끄면 음악이 멈추고 곡이 기억되며 켜면 같은 곡을 처음부터 재생, 같은 옵션 재적용은 불변, 음악 옵션과의 조합(둘 다 꺼짐·소리만 켬·음악만 켬), 결과 곡 단회 재생과 닫은 뒤 무시. 구현을 되돌리면 4건 중 3건이 실패함을 확인했다.
- `e2e/scene-music.spec.js`: 미션 중 `sound` 설정을 껐다 켜면 현재 곡이 `null`이 됐다가 같은 곡으로 돌아오고, 난수 0.3으로 천둥 곡(thu22)을 시작하면 `playByName('thunderCrack.wav',0,0,0,0,0)` 한 번만 호출되고 직접 `playSound`는 없다.
- 영향 e2e: `web-sound-device`·`pause-menu`·`game-menu-bar`·`first-run`·`mission-leave-pause`·`smoke`.
- 실제 청취는 하지 않았다(재생 호출·정지 횟수로 판정). 원본 실기 확인은 사용자 확인 범위다.

## 대응표

`ClientAudio.cpp` 행의 E1228 차이(Sound On)와 `SoundSceneMusic.cpp` 행의 차이 ④를 닫았다. 남은 차이는 ②(State7 난수 한 칸)·③(곡 길이 고정 표)·⑤⑥(희생곡)이다.
