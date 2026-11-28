# 음악 장치 이후 장기 음향 소유자와 State 연결 계획

1.206 대기 중 CPP ClientAudio/ClientMain과 DN HostMissionAudio·SceneMusic·프레임 순서를 읽기 대조한 계획이다. 제품 반영·실행 완료가 아니다. 먼저1.207 Fill/COM·1.208 Runtime·1.209 HWND를 검증하며, 준비 파일은 [이전 후속 계획](dotnet-state-followup-plan-1204-20261128.md)에 있다.

## 서로 다른 세 입구

CPP `ClientAudio.Initialize`는 최초 globals/두 음량 적용 뒤 sound값과 무관하게 OpenDevice를 호출한다. 따라서 최초 sound0/music1은 실제 장치/worker를 준비하되 효과음만 Enabled0이고 음악은 별도 관문이다. 현재 DN ctor의 Enabled 조건부 Initialize는 이 입구와 옵션 재해석을 합친 상태다.

CPP `ClientAudio.Interpret`는 globals→sound에 따른 OpenDevice(preload:false)/CloseDevice→효과음 음량→음악 음량→현재 이름 Select 순서다. sound0으로 재해석하면 음악도 장치 종료의 영향을 받는 원문 동작을 유지한다. 최종 Dispose와 달리 재개용 CloseDevice는 FileStore를 폐기하면 안 된다.

**State의 InterpretOptions는 이 실제 음향 적용 입구가 아니다.** 현재 CPP `Client.InterpretStateOptions→Client.InterpretOptions`는 ReadAudioOptions 결과를 읽을 뿐 ClientAudio.Interpret/ApplyAudioOptions를 호출하지 않는다. State 정리에 audio.Interpret를 직접 넣으면 추가 곡 재선택/장치 수명이 발생한다. 실제 옵션 행/설정 callback의 ApplyAudioOptions와 State 전역 옵션 부모를 구별한다. 근거: `cpppj/src/client/ClientAudio.cpp:107`·`ClientMain.cpp:884`·`UberGump.cpp:1357/1366/1387/2058/2063`.

## 실제 소유자별 연결

| 경계 | 재사용/다음 작업 |
| --- | --- |
| 옵션 읽기 | 같은 startup.Configuration.ReadInt. sound/music1·quality3·두 음량5·max8·swap0에서 실제 존재 키만 덮어쓴다. |
| 최초 OpenDevice | 같은 WindowsSoundDevice.Initialize→초기 천둥 preload→같은 Runtime.Initialize. sound/music 네 조합을 검사한다. |
| 옵션 적용 | 메뉴의 현재 `_applyAudio`는 별도 HostButtonSound만 갱신한다. 장기 실제 음향의 원문 Interpret 순서를 조립한다. |
| CloseDevice | Runtime 완전 합류→device.Shutdown. 파일 저장소는 옵션 재열기를 위해 유지한다. |
| 최종 Dispose | Runtime 완전 합류→device 종료→FileStore.Dispose. State 미션 반납에서는 장기 owner를 닫지 않는다. |
| 공통 Mute | 같은 ForPrimary PushMute/PopMute와 예약 음량을 쓴다. 현재 DN의 --mute-audio 장치 생략은 CPP의 초기화 후 PushMute와 차이가 있으므로 실제 owner 작업에서 판정한다. 별도 MonoGame HostButtonSound도 공통 제어 밖에 남아 있다. |
| 실제 준비 | !MuteAudio로 대체하지 않고 SoundState.Initialized를 사용한다. |
| Scene RNG·벽시각 | `_gameRandom == startup.Random`을 생성자에 빌리고 현재 `_wallSeconds`를 소비 위치마다 조회한다. 별도 RNG/시드/사본 동기화를 만들지 않는다. |
| 현재 battle | StateResources.Music.CurrentBattle의 실제 Storage.OwnerMode.Battle getter를 유지한다. |
| 현재 이름·선택·길이 | 같은 MusicSelectionState.Current·MusicSelection.Select·primary의 현재+18 double을 공급한다. 자산 이름은 영어 그대로다. |
| 결과·Player | Scene.ResultState 및 현재 Player getter의 실제 동일 DWORD/소유자를 연결한다. 현재 독립 scalar를 프레임별 복사로 덮지 않는다. |
| SP Refresh | 메뉴가 존재하는 경우 같은 RefreshNativeStormPower/예산 등록표로 갱신한다. |
| 날씨 색표 | Screen.LoadPalette·Weather4색·Scene.Tint·현재 palette dirty/Cloud redraw 소비자를 연결한다. 현재 서로 다른 저장소를 같은 값으로 간주하지 않는다. |
| ThunderFlash | 기존 PaletteFlash 계산에 실제 TYPE55 프로세스 부착·등록·종료 부모가 필요하다. 빈 callback으로 통과시키지 않는다. |
| 희생·대기실 | 실제 Raw 월드/메타 getter를 판정한다. CPP의 현재 false 경계와 구별하며 다른 시뮬레이션 BattleSession을 가져오지 않는다. |
| 최초 Start·Frame | 최초 메뉴 Start 및 매 프레임 Wind 뒤/미니맵·Gump·Kernel 앞에 같은 SceneMusic.Frame을 연결한다. Movie 잠금에서만 생략한다. |

## 후속 세부 단위

1. `ClientAudioOptions.cs`·`HostMissionAudio.Music.cs`·`HostMissionAudio.Options.cs`: 장기 device/player→files→두 raw/channel→ForPrimary→Runtime→Selection을 조립하고 초기화/재해석·음량/음소거·재열기/최종 반납을 검사한다.
2. 날씨 표시 부모: 실제 팔레트 로더·tint/dirty 공유·Thunder 프로세스 수명을 연결한다.
3. `HostMissionAudio.SceneMusic.cs`·`ClientHostGame.Audio.cs`: 현재 getter·공통 RNG/시각·최초 Start/프레임 순서와 State 음향 장치를 연결한다.
4. 전체 State 옵션 해석/부모: ResumeGame=PopNativeGamePause, ResumeAudio=같은 음악 제어 PopMute, StartSceneMusic=현재 battle getter를 유지한 같은 Start, StopSoundLoops=현재 장기 owner, InterpretOptions=별도 전체 State 옵션 부모다. 실제 State3/7/13·UiMode2/3 이후 GUI 왕복을 검증한다.

1.207 초안의 `IoAndRegionFailuresReleaseBorrowedRegionsAndCommandLock`은 Channel.FillBuffer를 직접 호출하므로 실제 검사 범위는 ChannelLock이다. 반영할 때 이름/설명을 좁히거나 실제 Control.Update 예외 경계를 검사한다. ForPrimary의 Update/음량과 Legacy Play/Stop의 명시 devices는 실제 제품 조립에서 같은 primary.BindPlaybackDevices로 묶는다. 현재 읽기 검토에서 필수 수정 제품 결함은 발견하지 못했으나 초안은 아직 컴파일/실행 전이다.
