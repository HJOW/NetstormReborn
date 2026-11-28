# 1.211 이후 SceneMusic 실제 연결 계획

2026-11-28, HJOW-Athlon. AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 다시 읽고 현재 DN 제품 소스와 CPP 복원 소스, 기존 [1.206 장기 음향 계획](dotnet-audio-owner-followup-1206-20261128.md)을 대조했다. 기존 ignored 읽기 메모 `extracted/dotnet-1212-scene-plan.md`의 실제 getter/호출 순서 조사를 추적 가능한 문서로 옮기고,1.211의 제품 반영 상태를 다시 확인했다. getter 부분은 독립 읽기 검토와도 교차 확인했다. 이 문서 작성에서는 제품 src/tests·다른 docs·LEFT·PORT_MAP·원본 자료를 수정하지 않았고 빌드·검사·게임 실행을 하지 않았다.

**2026-11-28 새 사용자 goal로 재개했다. Scene C의 실제 FILE·공유 loaded는 [1.217](dotnet-canon-1217-20261128.md)에서 검증했다.** 이전1.214 중지 시점: 1.213 A의 공유 값,1.214 B의 원문 두 조회/실제 월드 대여를 마쳤다. B의 감독 hook 설치와 C–H는 남은 후속이며 실제 Start/Frame·전체 State 부모 완료와 구별한다. 구현 번호는 각 단위를 시작할 때 현행 인수인계에서 정한다. 기존 ignored 파일명의1212를 실제 Scene 구현 완료/진행 번호로 해석하지 않는다.

## 1. 범위와 현재 기준

현재 CPP의 대상 파일 이름은 `cpppj/src/client/SoundSceneMusic.cpp/.h`다. `ClientSceneMusic`이라는 별도 제품 파일은 없다. DN 판단 본체는 `dotnetpj/src/Netstorm.Client/Audio/SceneMusic.cs`에 이미 있다. 기존 9,339행 적합성 자료와 시각/현재 battle 검사들은 이 판단 본체의 근거이며 실제 Game의 Start·Frame·팔레트·Thunder·State 부모가 연결됐다는 근거가 아니다.

1.211에서 장기 음악 owner가 실제 제품 소스에 반영됐다. 현재 [HostMissionAudio.cs](../dotnetpj/src/Netstorm.Game/HostMissionAudio.cs)는 partial이며, window 생성자는 준비 생성자 뒤 InitializeStartup을 호출하고 최종 Dispose는 DisposeAudioOwners를 호출한다. [HostMissionAudio.Music.cs](../dotnetpj/src/Netstorm.Game/HostMissionAudio.Music.cs)는 동일 device/player→files→두 채널→ForPrimary→Runtime→Selection을 조립하고, [HostMissionAudio.Options.cs](../dotnetpj/src/Netstorm.Game/HostMissionAudio.Options.cs)는 실제 최초 초기화·옵션 재해석·재열기·최종 반납을 구별한다. 순수 옵션 정책은 [ClientAudioOptions.cs](../dotnetpj/src/Netstorm.Client/Audio/ClientAudioOptions.cs), nonthrow 실제 로그 어댑터는 [HostAudioLog.cs](../dotnetpj/src/Netstorm.Game/HostAudioLog.cs)에 있다. 후속 Scene 작업은 이 같은 장기 owner를 빌리며 다시 만들지 않는다.

1.211의 최초 실패·교정·별도 하드웨어·집중/진입 검증은 [1.211](dotnet-canon-1211-20261128.md), 이후 Client전체3,105통과는 [1.212](dotnet-canon-1212-20261128.md)에 보존했다. 공유 값 연결은 [1.213](dotnet-canon-1213-20261128.md)에서 검증했다. 실제 파일/버퍼/worker 또는 현재 getter 검증을 Scene 감독·Start/Frame·State 부모 완료로 확대하지 않는다.

현재 [PORT_MAP](../dotnetpj/PORT_MAP.tsv)의 실제 대응은 다음 좁은 범위다.1.211 준비 초안을 완료로 간주하거나 아래 대응을 ClientAudio/ClientMain 전체의 완료 표시로 확대하지 않는다.

| 실제 기록 | 대상과 근거 | 이 계획에서의 의미 |
| --- | --- | --- |
| 기존785~786행 | SoundSceneMusic.cpp/h→Netstorm.Client/Audio/SceneMusic.cs, `99b9d5e332d20a8962b8ef206f1bb6e807be36fc` | 판단 본체 대응이다. 실제 Game getter·날씨·Start/Frame 결선을 포함하지 않는다. |
| 기존1938~1939행 | SoundMusicRuntime.cpp/h→Netstorm.Game/WindowsMusicRuntime.cs, `225f2322cf7936604306b6534fd4edf18f246c12` | 실제 worker의 커널 경계이며 Scene의 주 스레드 Frame과 다르다. |
| 1.211 추가1942~1943행 | ClientAudio.cpp/h→Netstorm.Client/Audio/ClientAudioOptions.cs, `bd2941f45080bdd3254baaa727a14ac4137d64cf` | 순수 Read·음량 표·Apply/Interpret 순서다. |
| 1.211 추가1944~1945행 | 같은 ClientAudio.cpp/h→Netstorm.Game/HostMissionAudio.Music.cs, 같은 근거 식별자 | 동일 장기 음악 owner와 현재 이름·길이 façade다. Scene 감독 생성은 없다. |
| 1.211 추가1946~1947행 | 같은 ClientAudio.cpp/h→Netstorm.Game/HostMissionAudio.Options.cs, 같은 근거 식별자 | 최초/재해석/반납의 실제 입구다. State 전체 InterpretOptions/메뉴 callback 결선은 아니다. |

행 번호는 이 문서 작성 당시의 위치다. 이후 대응 행이 추가되면 원문·대상 경로와 근거 식별자로 찾는다. HostAudioLog는 관리 환경의 기록 실패 adaptation이며 이6행의 원본 함수 이식으로 표시하지 않는다.

`docs/dotnet-audio-owner-followup-1206-20261128.md`의 큰 방향은 유지되지만 다음 세부 누락을 보완해야 한다.

* 최초 Issue·intro·Scene Start·Chat은 현재 DN에서 원문 순서로 끼울 수 있는 네 개의 독립 입구가 아니다.
* PlayersReady/LocalPlayer의 실제 getter·PaletteDirty 공유 box와 하나의 결과 DWORD는1.213에서 연결/준비했다. 결과 메뉴/이탈/로그인 등 실제 부모가 그 상자를 빌리는 조립은 H에서 남는다.
* DN의 기존 `Screen.LoadPalette(GamePalette)`는 전체 값 decoder 경로다. CPP 10.78의 실제 파일/부분 읽기 경로 및 모든 팔레트 소비자 갱신을 그대로 대신하지 않는다.
* Thunder는 계산 본체만 있고 실제 TYPE55 부모·Form·Kernel·삭제 등록은 없다.
* true Start는 임의의 세션 생성 직후가 아니라 실제 State7 앞단에 있다. State 정리의 Start도 battle을 0으로 쓰기 전의 현재값을 소비한다.
* CPP Frame의 Wind와 SceneFrame 사이에는 MonitorTime·BridgeDecay가 있다. DN의 가까운 삽입 위치만으로 전체 Frame 순서 복원을 완료했다고 할 수 없다.

## 2. 실제 공유 소유자와 필요한 API

| 소비 경계 | 현재 확인된 실제 자료 | 최소 연결/미완료 사항 |
| --- | --- | --- |
| SceneMusicState | `ClientStateResources.Music`; `ClientHostGame.ColorMaps.cs:38~39`가 같은 객체를 HostMissionAudio에 전달 | 1.211 뒤에도 이 객체를 그대로 사용한다. Session별 새 SceneMusicState를 만들지 않는다. |
| 게임 RNG | `ClientHostGame.cs` 생성자의 `_gameRandom = startup?.Random ?? new GameRandom()`; 실제 시작 자료가 있으면 startup.Random과 동일 | 실제 SceneMusic 생성자가 이 같은 객체를 빌린다. 새 시드·복사본·소비 횟수 보정은 금지한다. 생성/바인딩 자체는 난수를 소비하지 않는다. |
| 현재 battle 00594fbc | `ClientHostGame.StateResources.cs:20`의 `Music.CurrentBattle = () => _surfaceGlobals.Storage.OwnerMode.Battle ? 1u : 0u` | 1.213의 ClientStateResources.BindCurrentMusicOwners가 같은 실제 storage를 연결한다. `StartScene(bool)`은 CPP처럼 fallback Battle만 기록하고 Start를 전달한다. 현재 Storage.OwnerMode.Battle을 인자로 덮어쓰거나 과거 Start 인자를 진단상의 현재값으로 쓰지 않는다. |
| 현재 벽시각 | ColorMaps의 기존 `_clientClock.Clock.WallSeconds(_clock.Milliseconds)` delegate | SceneMusic의 요청/비교 위치마다 재조회한다. FrameTime의 캐시로 바꾸거나 미리 한 번 읽어서 전달하지 않는다. 게임 Pause와 독립이다. |
| 현재 이름·길이·선택 | 1.211 제품 `HostMissionAudio.Music.cs:91~95`의 `CurrentMusicName`·`CurrentMusicDuration`·`SelectMusic` | 같은 SelectionState.Current/primary.Duration/Selection을 재사용한다. 이름은 좁은 영어 자산 이름 그대로다. sound/music 옵션이나 mute로 Scene 판단 자체를 생략하지 않는다. |
| SongEnd 00565e68 | `SceneMusicState.SongEnd` DoubleBox; `ClientStateResources.cs:31`이 State context에 같은 참조 전달 | 이미 공유다. State의 8BYTE 초기화와 Scene 요청이 같은 저장소를 쓴다. 새 double 복사나 다른 기한의 동시 초기화를 넣지 않는다. |
| PlayersReady 00595344 | `RawPlayerWorldState.cs:10`의 `_gameplay.Player.PlayerKnown` 전체 DWORD | 1.213에서 소비 위치의 선택적 CurrentPlayersReady와 제품 바인딩을 연결했다. 독립 fallback은 보존한다. `PlayerRecords.Initialized`·세션 존재·bool 상수로 대체하지 않는다. PlayerKnown 전체를 위해 별도 PlayerState를 만들지 않는다. |
| LocalPlayer 00540c70 | 같은 Player의 `BattleOwnerBox`(`RawPlayerWorldState.cs:13`) | 현재 owner 조회/같은 상자를 빌린다. TemplateOwner와 다르다. Next는 PlayersReady를 먼저 읽고 참인 경우에만 LocalPlayer를 조회하여 희생 판정에 넘겨야 한다. |
| PaletteDirty 0059a8b0 | `ClientKencloudGlobals.cs:94`의 `startup.Cloud.WorldRedrawBox`; StateWorldGlobals도 같은 상자를 읽고 씀 | 1.213의 BindPaletteDirty가 같은 상자 참조만 빌리고 PaletteDirty 속성은 그 view다. 매 프레임 값 복사나 MapRevision005949d4에 쓰는 방식은 다르다. |
| ResultState 005c85ac | Scene.ResultState 값속성 외에 `NativeMainMenuStateGlobals.Outcome`, `RawBattleLeave`의 Outcome, `MissionLoginTimerState.RequestPhase`가 UIntBox API | 1.213에서 SceneMusicState.ResultStateBox를 단일 owner로 준비하고 기존 ResultState 속성을 같은 view로 보존했다. Game의 세 실제 부모는 아직 미조립이므로 이후 같은 상자를 빌려야 한다. 실제 State/로그인/결과 호출자도 그 상자를 사용해야 제품 결선이 된다. |
| 날씨 Tint/Tints | Screen.Colors().Weather는 현재 팔레트의 네 색 번호; 배경은 `ClientHostGame.WorldDrawing.cs:168`에서 `_missionAudio?.Scene.Tint`를 읽음 | 최초 색표 준비와 이후 팔레트 변경에 Tints 네 칸만 갱신한다. LoadPalette 안에서 현재 Tint까지 덮어쓰면 원문 ApplyWeather의 쓰기 순서를 바꾼다. |
| SP Refresh | `ClientHostGame.ProductionDeck.cs:133`의 실제 RefreshNativeStormPower와 장기 예산 등록표 | CPP처럼 메뉴가 준비된 경우 실제 Refresh에 연결한다. 해당 함수의 초기 준비 전 호출과 메뉴 없음 분기는 원문 위치에서 구별한다. |
| WeatherSound | 동일 HostMissionAudio의 `PlayGlobal(name)`는 `_player.Play(_list.Lookup(name), 0, 0, 0, 0, 0)` | 현재 같은 player/list를 그대로 쓸 수 있다. 1.211의 공통 음량/mute 관문을 우회하는 두 번째 SoundPlayer는 만들지 않는다. |
| ascendancyPalette | 같은 `startup.Configuration.GetInt("ascendancyPalette")` | 정확히 1인지 비교한다. 누락/0과 임의의 비영 값은 같지 않다. |
| Sacrificing 00449220 | Scene 필수 hook만 있음. `BattleSession.Sacrifice.cs:IsSacrificeInProgress`는 별도 Simulation 모델 | 실제 Raw Player→제단→룬 단계 조회 부모가 미완료다. 별도 BattleSession을 조립하거나 그 모델을 캡처하지 않는다. RawRitualRegular/RawDaisRegular의 실제 물리·프레임 단계 조회를 재사용할 수 있는지 별도 작은 단위에서 원문 함수와 대조해야 한다. |
| WaitingRoom 0042cbd0 | Scene 필수 hook만 있음. Challenge·PlayerKnown·메타맵 현재 기록이 필요한 경계 | 실제 조회 부모가 미완료다. ordinary mode의 실제 조기 반환 조건과 challenge 기록 조회를 구별해야 한다. 단순 false callback으로 필수 hook을 채우지 않는다. 미복원인 선택 경계는 그 위치에서 명시 실패를 유지한다. |

현재값 API는 독립 적합성 검사에 필요한 fallback 속성을 유지할 수 있지만, 실제 Game 조립은 실제 owner/현재 getter가 설치됐는지 확인해야 한다. 독립 검사용 fallback 0을 제품 준비 완료의 근거로 쓰지 않는다. ResultState/PaletteDirty는 쓰는 경계이므로 getter만 추가해서 끝나지 않는다.

CPP도 완전한 실제 연결의 근거는 아니다. `ClientMain.cpp:548`은 CurrentBattle을 실제 Storage에 연결하고, `ClientAudio.cpp:198`은 결과 DWORD를 참조로 제공한다. 그러나 CPP의 playersReady/localPlayer/paletteDirty 자체 필드를 기존 Player/Cloud에 직접 결합한 경로는 이번 검색에서 확인되지 않았다. `ClientAudio.cpp:27~35`는 희생·대기실에 기본 false, 미제공 표시 효과에 빈 함수를 채운다. 실제 ClientMain은 refresh/palette/thunder를 공급하지만 희생·대기실은 공급하지 않는다. 이 공백을 DN의 실제 현재값이라고 복제하지 않는다.

## 3. 생성·최초 시작·매 프레임의 정확한 삽입 위치

### 3.1 감독의 생성과 바인딩

작은 새 부분 파일 후보는 `HostMissionAudio.SceneMusic.cs`와 `ClientHostGame.Audio.cs`다. 전자는 같은 SceneMusicState·GameRandom·실제 owner hook으로 SceneMusic 하나를 보유하고 `StartScene(bool)`·`SceneFrame()`·`RequestMusic(string)`·`NextSceneMusic()`·`SetSceneTints(...)`를 얇게 전달한다. 후자는 Game가 실제 공유 owner와 화면/월드 callback을 제공한다. 생성/바인딩은 곡 선택·벽시각 읽기·RNG 소비·Start를 수행하지 않아야 한다.

1.211의 prepared constructor는 원래 무장치/무worker 준비 경계다. 거기에 아직 Scene Start를 섞지 않는다. 최초 장치 초기화와 mute 초기 적용은1.211 순서를 지키고, 실제 공통 RNG/hook 설치는 첫 Start 이전까지 한 번만 완료한다. 설정 재해석/장치 재열기로 SceneMusicState·감독·RNG를 새로 만들지 않는다. getter는 이전 Session을 캡처하지 않고 Game의 현재 장기 owner를 조회한다.

### 3.2 최초 메뉴 Start

CPP `ClientMain.cpp:554~557`의 순서는 다음과 같다.

1. InitializeClientIssueCommands
2. DetectIntroAvailability
3. audio.StartScene(false)
4. InitializeClientChatManager
5. 최초 메뉴 생성

DN `ClientHostGame.LoadContent`는 DrawStartupScreen→PrepareNativeRenderer→InitializeNativeChat→HostMainMenu 생성이다. 그런데 `ClientHostGame.Chat.cs:InitializeNativeChat`은 하나의 `_nativeChatInitialized` 아래 Issue 네 등록과 ChatManager 초기화를 연속 실행한다. RawIntroMenu.DetectIntroMovie는 현재 메뉴/UI를 만든 뒤의 늦은 위치에 있다. 따라서 InitializeNativeChat 앞/뒤에 Start 한 줄을 추가하면 CPP 순서를 보존할 수 없다.

최소 후속은 실제 Issue와 Chat manager 입구/성공 표식을 분리하고 intro 감지를 Issue 뒤로 옮긴 다음, 실제 Start를 둘 사이에 배치하는 단위다. 기존 `_nativeChatInitialized` 하나를 중간 성공 표식으로 쓰면 뒤 작업 실패/재호출 때 Issue 중복 또는 Chat 누락을 감출 수 있다. Content 재적재/실패 재시도에서 각각 실제 완료된 단계의 한 번만 실행 계약도 별도로 확인해야 한다. 최초 메뉴 Start를 생성자, 첫 Draw, 매 LoadContent에 무조건 실행하는 형태로 넣지 않는다.

최초 Tints는 DrawStartupScreen의 첫 팔레트/fort 팔레트 준비를 거친 현재 Screen.Colors().Weather에서 가져온다. 이는 해당 로더가 완성된 시점에서 공급하며, 아직 네 칸이 0인 상태를 실제 검색색으로 취급하지 않는다.

### 3.3 매 프레임

CPP `ClientMain.cpp:607~621`은 MovieUpdatePauseDepth가 양수인 동안만 해당 프레임 가지를 생략한다. 해당 가지는 SaveOptions→연결 감시→Ambient→Wind→MonitorTime→BridgeDecay→SceneFrame→Minimap→CleanupGumps→IdleGumps→Kernel 준비/실행 순서다.

DN `ClientHostGame.Update`의 현재 실제 위치는 `RawAmbientSound.Step`(:803)→`Storage.UpdateAmbientWind`(:811) 뒤, NativeMinimap.Refresh(:815) 앞이다. 같은 `clock.Initialized && !movieLocked` 가지에 실제 SceneFrame을 연결하는 것이 가까운 최소 입구다. 메뉴에서도 호출하고 게임 Pause 중에도 벽시각은 읽어야 한다. `_missionSession != null`, 효과음 Enabled, music 옵션, mute 상태로 SceneFrame을 막지 않는다. Kernel/500ms worker/Draw/PrepareNativeFrameInput에 넣으면 원문 시점·스레드가 다르다.

다만 DN에는 이 위치의 MonitorTime·BridgeDecay 실제 호출자가 아직 없고, 현재 Ambient hook의 CurrentDialog/Flash도 각각0/빈 함수다. SceneFrame 연결 단위는 이 이웃 미완료를 명시하고 전체 Client Frame 완료를 주장하지 않는다. Thunder를 만들면 Ambient Flash도 같은 실제 번개 입구로 연결할 수 있지만, 현재 Dialog getter까지 복원됐다는 의미는 아니다.

## 4. 전투 Start와 State 정리의 실제 호출자

CPP `UberGump.cpp:1016`의 LoadMissionWorldBody 앞단은 CaptureCampaignMissionStarting→CleanupNativeStateWorld(1)→CD 확인→명령창 숨김→RestoreLocalMissionStarting→`ownerMode.battle=true; audio.StartScene(true)`(:1033)→guided pause 등의 순서다. true Start는 Zone/월드 설치보다 앞이다. :1183의 늦은 Start는 10.78이 아닌 CD 경로만 남긴 이전 어댑터다. DN의 기준으로 그 늦은 위치를 복제하면 안 된다.

DN에는 `RawLocalMissionState.Starting()`이 이미 이 부모 순서를 갖고 있다. `RawLocalMissionState.cs:71~78`의 `SetInBattle(true)→StopMusic()`이 바로 이 00469fc0 입구다. `LocalMissionStateTypes.cs`의 StopMusic은 이름은 정지처럼 보이지만 주석 주소가 00469fc0이며 단순 채널 Stop이 아니다. 실제 Game의 RawLocalMissionState 조립/호출자는 아직 미완료다. 이 hook은 같은 장기 음향의 StartScene(true)에 연결해야 한다. 1.211의 `HostMissionAudio.StopMusic()`과 이름만 보고 연결하면 틀린 동작이다.

현재 개발용 `ClientHostGame.StartMission`은 fort/script/players 준비 후 :478에서 Battle=true를 직접 기록하고 :491에서 HostSession을 만든다. 원문 State7 cleanup/capture/restore 부모를 거치지 않는 직접 진입이다. Battle=true 직후에 Start 한 번을 추가하면 적어도 세션/Zone 전이지만, 그 자체로 실제 State7 순서가 복원되는 것은 아니다. 정확한 호출자를 먼저 조립하고 기존 직접 진입을 그 부모 아래로 옮기는 방향을 우선한다. 원문 난수 차이를 맞추기 위한 임의 추가 Start나 LCG 상태 변경은 하지 않는다.

State 정리도 별도 호출자다. `cpppj/src/client/ClientStateWorldCleanup.cpp:71`은 기존 battle을 0으로 쓰기 전의 현재 모드로 StartScene을 실행한다. DN `ClientStateWorldCleanup.cs:111`은 이미 선택 위치에서 `devices.StartSceneMusic(surface.Storage.OwnerMode.Battle)`을 호출하지만 Game의 실제 devices 완성은 별도다. 같은 장기 SceneMusic/현재 battle getter로 연결해야 한다. SongEnd의 raw8BYTE 쓰기, 이전 Runtime 반납, Start, 뒤 battle0 쓰기를 재정렬하지 않는다. 결과 메뉴는 `NativeMainMenuState.cs:178~185`의 실제 RequestMusic(fanfare/defeat)와 위 ResultState 상자까지 한 묶음으로 확인해야 한다.

## 5. 날씨 팔레트 부모의 누락

CPP `ClientMain.cpp:1099~1124`는 다음 두 경계를 구분한다.

* LoadScenePalette(name): `configuration.PathSpec("DataDir") + "\\" + name`→LoadPaletteFile(path,0)→RefreshPaletteConsumers. 이미 `.col`이 붙은 이름이므로 다른 palette spec 함수로 확장자를 다시 붙이지 않는다.
* RefreshPaletteConsumers: 현재 Screen.Colors.weather를 Scene Tints에 복사→소프트웨어 커서이면 현재 팔레트로 재생성→menu.PaletteChanged, 메뉴가 없으면 renderer.InvalidateAll.

CPP Patch1078 LoadPaletteFile은 Screen.LoadNativePaletteFile의 실제 FILE/부분 읽기 경로다. DN Screen.cs:183의 LoadPalette(GamePalette)는 완성된256색 decoder 입력만 받는다. 읽기 실패 전후 saved/current/logical·paletteLoaded 효과까지 원문으로 복원할 단위가 별도로 필요하다. 일반 정상 COL을 읽는 편의 wrapper만 추가하고 전체 10.78 native loader로 표시하면 안 된다.

DN HostStartupColorMaps.Rebuild(:52~56)는 동일 초기 ColorSortInput과 현재 Basic[1]로 표를 재생성하고 Screen.ShadeMaps만 요청한다. 아직 위 RefreshPaletteConsumers와 같지 않다. 현재 `_menuDirty=true`만으로 갱신을 끝낼 수 있는지도 별도 검사가 필요하다. 예를 들어 HostMainMenu의 GumpVisual.ButtonColors, HostHtmlText의 body/style/markup 팔레트, HostHtmlMenus, 생산창 SetPalette, RawMinimapControls.SetPalette 등에는 초기 캡처/명시 재설정 경계가 있다. 현재색을 매번 읽는 Chat/배경 경계와 혼동하지 않는다. HostOriginalCursor는 SDL 단색 커서이며 CPP의 BuildSoftware 팔레트 경계를 이미 제공한다고 볼 수 없다.

SceneMusic.ApplyWeather의 보존 순서는 Refresh→옵션이 정확히1이면 기존 Tints[Index]를 Tint에 쓰기→LoadPalette→PaletteDirty=1→현재 Index 재조회→천둥이면 Flash→WeatherSound다. 팔레트 적재가 Tints를 갱신해도 이 함수 안에서 이미 쓴 Tint를 다시 바꾸지 않는다. Start의 마지막 Tint 쓰기는 그 후의 별도 원문 쓰기다. 임시 번개 SetPalette는 Weather/Tints/고정 ColorSort 입력을 재생성하는 파일 팔레트 교체가 아니다.

## 6. 실제 Thunder TYPE55 부모

기존 DN `Display/PaletteFlash.cs`는 범위228~245·64단계·전체화면0.5초/창1/60초·취소 시 저장 팔레트 복구 계산을 갖고 있다. 실제 프로세스 수명은 없다. 가장 작은 새 파일 후보는 `Netstorm.Client/Display/ThunderProcess.cs`와 `Netstorm.Game/ClientHostGame.Thunder.cs`다.

### 6.1 프로세스 본체

CPP `ThunderProcess.cpp:6~10`을 따라 실제 `SquidProcess`를 상속한다. 생성자는 요청 당시 전체화면으로 Duration을 정하고 brightness0을64로 해석하지만 시각/팔레트를 읽지 않는다. 실제 Host.Attach가 Form·부모·PID를 먼저 기록한 뒤 OnAttached에서 PaletteFlash.Init을 실행한다. RunFrame은 Flash.Frame이 끝나면 실제 Host.Kill(this,0)을 호출하고 그 뒤 자기 필드를 접근하지 않는다. Dispose는 Flash.Cancel과 같은 PID 등록 해제를 수행한다. 자연 종료 뒤 중복 Restore를 만들지 않는다.

DN의 `RawKernel.BaseProcess.Dispose`·`RawSquidProcess.SquidProcess.OnAttached/Host/Pid`·`RawSquidProcessHost.Attach/Kill`을 재사용할 수 있다. GC 최종화·별도 timer·Game의 수동 Flash.Frame 목록으로 Kernel을 대체하지 않는다. Attach가 free/dead 부모로 조용히 거절되거나 부분 실패했을 때도 실제 host의 미부착 Dispose 계약을 보존한다.

### 6.2 Game의 실제 부모 조회

CPP `ClientMain.cpp:1128~1145`의 관문/조회 순서가 기준이다.

1. 현재 Gameplay.Player.PlayerKnown==0이면 부모 기록을 읽기 전에 반환한다.
2. 같은 Surface Storage를 요구한다. Challenge이면 실제 메타맵 기술 기록 조회가 미복원이므로 명시 실패한다. 일반 Player 행을 대신 읽지 않는다.
3. 현재 `CoreData.Players`의 `BattleOwner * 180 + 0x9c`에서 전체4BYTE DWORD를 읽는다. DN ClientCoreDataGlobals.Players는 이미 현재 물리 view이며 이전 Session을 캡처할 필요가 없다. `_coreData.Records()`의 별도144BYTE 장부와 다르다.
4. parent0은 반환한다. 그 외에는 상위16BIT를 버리기 전에 parent<=65535 및 현재 Pool.Capacity 미만을 확인한다.
5. 같은 `Storage.Processes().Host()`에 TYPE55/flags0x10으로 실제 부착한다. 성공한 실제 Kernel PID만 등록한다. 등록 목록은 프로세스/부모의 소유권을 대체하지 않는다.

원문과 같은 일반 Player.CurrentTechnology helper를 재사용하려면 그 helper가 현재 공유 Player/9기록/풀을 사용하는지 확인해야 한다. challenge 분기를 지워 주거나 이전 Session의 helper를 장기 callback에 캡처하는 방식은 사용할 수 없다.

### 6.3 시각·팔레트·삭제 수명

Thunder hook의 벽시각은 CPP `Time().wall`에 맞춰 `_clientClock.Frame.Value.Wall`의 현재 프레임 캐시를 읽는 후보가 맞다. SceneMusic의 동적 WallSeconds(get milliseconds)와 의도적으로 다르다. 현재 FullScreen 비트는 매 조회 시 Screen.Flags에서 읽고, 읽기/부분 적용/전체 복구는 같은 Screen.Palette·SetPalette를 사용한다. 복구는 생성 당시 부분 배열이 아니라 `SetPalette(0,256,null,true)`의 현재 저장 팔레트다.

CPP ClearThunderFlashes(:1148~1153)는 현재 뒤 PID를 다시 읽고 실제 Kernel 객체/Thunder 타입을 검증한 뒤 Host.Kill(flags0x10)한다. Dispose callback이 목록을 지운다. State의 Kernel/부모 삭제에서도 같은 Dispose 경로를 통과해야 한다. 장기 목록에 이미 죽은 PID가 남거나 그 PID가 다른 타입으로 재사용되면 명시 오류다.

Game 최종 Dispose에서 Screen/output/장기 process binding이 살아 있을 때 먼저 정리해야 한다. 화면 변경도 `ChangeScreenMode`만이 아니라 실제 `_graphics.ApplyChanges()`를 부르는 `ApplyWindowed`(:1036)·`ApplyFullscreen`(:1048), 그리고 같은 full 여부에서 Screen.SetMode만 바꾸는 :998을 모두 검토해야 한다. 모드 변경 뒤 이미 `_saved`에 번개 팔레트를 저장한 다음 정리하면 복구 시점이 늦다. 하나의 실제 전환 입구로 정리 호출을 모으되 중복 Restore/반납이 없는지 확인한다. 현재 코드에는 그 수명 연결이 없다.

## 7. 1.212 Client 전체 재검증 이후의 후속 A–H

1.211의 집중/실제 진입과1.212 Client 전체3,105통과 재검증을 마쳤다([전체 결과](dotnet-canon-1212-20261128.md)). 다음 A–H는 완료 범위와 남은 실제 부모를 함께 기록하는 작업판이다. A와 B 조회/대여의 완료를 나머지 부모 완료로 세지 않는다. 각 단위는 현재 부족한 부모를 감추는 빈 callback이나 가짜 Session을 만들지 않는다.

- **A — 현재 공유 값 결선,1.213 완료.** SceneMusicState의 현재 Player getter·Cloud box·단일 결과 box와 실제 Game의 identity 검증을 연결했다. 기존/실제 owner 모드의9,339행·집중41/41·대표3진입/실제 Main을 확인했다. 결과 부모와 실제 감독은 남은 B–H 범위다.
- **B — 조회 본체/현재 월드 대여1.214 완료, 감독 hook 설치 F 대기.** 같은 현재 사제 목록→반경3 finder→프레임 선저장→현재 VT Count→단계와 Challenge/Known/현재 owner 뒤 현재CS+a4를 복원했다. 새 상위984행과 실제83VT 분배·current Surface 대여를 검증했다(집중69/Core2/목록4). Game의 기존 장기CS를 빌리며 BattleSession/고정 TYPE165/별도 기술 부모를 사용하지 않는다. 실제 감독 생성/Start/Frame과 일반 Session/FortLoad의 Count 소비자는 미완료다. [1.214](dotnet-canon-1214-20261128.md)
- **C — 파일 팔레트 경계, 미완료.** 10.78 native 파일/부분 읽기 로더를 기존 Screen 저장소에 연결하고 정상/부분 실패의 효과를 대조한다. 먼저 기존 값 decoder의 검사 범위를 native 파일 수명과 구분한다.
- **D — 팔레트 소비자 갱신, 미완료.** 현재 Tints 공급, Cloud dirty, 실제 메뉴/생산/미니맵/텍스트 등 캡처색 갱신, 현재 renderer invalidate를 한 실제 RefreshPaletteConsumers 입구로 모은다. 소프트웨어 커서의 실제 미완료는 별도 표시한다. 이 입구를 최초/fort/weather 변경이 공유한다.
- **E — Thunder 본체와 실제 부착, 미완료.** PaletteFlash→SquidProcess→TYPE55/Form/Kernel, 실제 기술 DWORD 부모, 자연 종료/부모 삭제/State Kernel 정리/표시 모드 변경/최종 Dispose를 집중 검사한다. 기존 PaletteFlash 계산 검사를 반복하는 데 그치지 않는다.
- **F — 감독 조립과 최초 메뉴 Start, 미완료.** 검증된1.211 owner façade와 위 실제 hook을 사용하여 감독을 한 번 조립한다. Issue→intro→Start(false)→Chat manager→최초 메뉴의 실제 초기화 순서를 분리/교정한다. 반복 LoadContent와 선택된 실패 재시도의 단계를 검증한다. 이 단위에서 실제 music off/mute에도 Scene의 날씨·기한·RNG 판단을 보존한다.
- **G — 실제 Frame 연결, 미완료.** Movie 잠금 없는 현재 Ambient/Wind 뒤·Minimap/Gump/Kernel 앞에 Frame을 연결한다. 게임 Pause에서도 동적 벽시각을 읽으며 Movie 잠금에서 생략하는 집중 검사와 실제 trace를 남긴다. 이웃 MonitorTime/BridgeDecay/ambient modal 미완료는 별도 유지한다.
- **H — State7/정리/결과의 실제 호출자, 미완료.** RawLocalMissionState.StopMusic hook의 실제 의미를 StartScene(true)로 연결하고 기존 직접 StartMission을 원문 부모 아래로 이동한다. cleanup Start의 현재 battle 읽기와 SongEnd 쓰기, 메뉴 fanfare/defeat와 같은 결과 UIntBox를 검증한다. 전체 State UI 교체·미션 적재/반납 부모가 이어지기 전에는 GUI 왕복 완료로 표시하지 않는다.

Getter/월드 조회와 palette/Thunder는 독립 집중 단위로 나눌 수 있다. 실제 감독 Start·Frame을 제품에 켜는 단위는 선택 가능한 필수 효과가 준비된 뒤에 둔다. 기존 State 부모 선행과 병합할 때 같은 음악 Start를 두 번 연결하지 않는다. R6 실제 Load9단계·AI Drops/Bridges·현재 게임 시각 비교는 [1.199 계획](dotnet-state-followup-plan-1199-20261128.md)의 실제 부모 범위를 유지한다. Scene을 확인하려고 NativeLoaded를 강제로 참으로 만들거나 완전 적재·생산 성공을 합성하지 않는다. 실제 Load/현재 owner/필수 callback이 없는 선택 위치는 그 오류를 유지한다.

## 8. 후속 검증에서 구별할 것

* 기존 9,339행 SceneMusic 적합성과1.211 owner 검증의 실제 확정 결과는 재사용하되 실제 Game 연결의 관측과 분리한다. 아직 재검증 대기인 검사나 ignored 초안을 통과로 세지 않는다. 실제 콜백이 읽는 값/주소·호출 순서·영어 곡 이름·현재 채널 identity를 추가 검증한다.
* Result box/Player/Cloud owner의 양방향 쓰기, getter가 바뀐 현재값을 소비 위치에서 다시 읽는지, 다음 미션에서 이전 Session 참조가 남지 않는지를 검사한다. 프레임 끝에 수치를 복사해 일치시키는 검사는 피한다.
* Thunder는 살아 있는 실제 기술 부모/Kernel을 둔 부착 및 실제 삭제를 검사하고, PlayerKnown0에서 CoreData 미조회, parent0, 상위비트 있는 DWORD, free/dead 부모, challenge 미복원, Attach 중 실패를 구별한다.
* Scene의 동적 wall과 Flash의 cached wall을 각각 기록한다. 정지/재개 및 Movie 보유표식은 이미 다른 수명이다. worker의500ms 주기를 Scene 프레임으로 취급하지 않는다.
* 1.187의 난수5회 추적에서 Scene Start1회가 관측됐다는 사실은 임의1회 보정의 허가가 아니다. 실제 State cleanup reason0/1과 State7의 Start 및 네 Wind가 원문 위치에서 실행되는지 확인한다. 겉보기 RNG 일치를 위해 state/seed/호출 횟수를 조작하지 않는다.
* 이후 실제 화면/덤프 검증은 기존1.173~1.176에 따라 같은 게임 시각·현재 상태를 확인한다. 고정 프레임 수만 같거나 음악 소리가 들리는 것으로 전체 L4a/State/시나리오 완료를 주장하지 않는다.

이 문서는1.212 이후 구현과 중지 시점의 재개 인수인계다.1.213/1.214의 좁은 완료 범위 외 SceneMusic 감독·전체 State 부모·GUI 왕복·희생/승리 재현은 미완료로 유지한다.


## 9. 1.214 중지 시점의 재개 준비물

사용자 요청으로1.214를 마친 뒤 중지한다. 다음은 모두 이 PC의 ignored 초안이며 제품 수입/빌드/검사 완료가 아니다. 1.217에서 C의 source SHA 대조·실제 반영/검증을 마쳤다. 아래 C 행의 미구현 설명은 중지 당시 기록이며 현행은1.217 문서다. 1.218에서 D의현재 UI·날씨4색·공통 후단을 검증했다(software/메타맵·감독은 미완료). 1.219에서 E의실제 부착·현재 기술 부모·주변음/Chat/표시 모드 선행 취소를 검증했다. 1.220에서 F의같은 실제 감독·최초시작 순서·성공/실패수명·GPU/domain분리를 검증했다(집중78·GPU2경로·정기7,720통과). 1.221에서 G의 MonitorTime·같은 현재 옵션/Chat/CRT를 검증했다(집중23·실제180초 알림·대표3진입). 다음 G의 다리 붕괴/SceneFrame·현재 모달→H 순서와 기존 R5/State 우선순위를 유지한다. 활성 번개 중 GUI 전환/새 Screen·전체 State는 미완료다.

| 준비물 | 경로·현재 한계 |
| --- | --- |
| C 공유 loaded/파일 부모17파일 — 1.217 반영·검증 완료 | `extracted/dotnet-scene-palette-draft/CLIENT_PALETTE_WIRING.md`, `STARTUP_NATIVE_PALETTE_FOLLOWUP.md`. 최초/fort가 여전히 cached GamePalette decoder를 선택한다. native 실제 입구, 중복 읽기와 Construction/Burning의 Palette 소비자, assertion callback 정책을 함께 정리해야 한다. 미빌드·미실행, 핵심4범위 peer만 확인했다. |
| D 소비자10파일 — 1.218 공통 후단 검증 | `extracted/dotnet-scene-refresh-draft/README.md`. 같은 UI/스타일/entry의 setter4개와 공통 갱신/현재9maps 별칭, 검사5case 초안이다. 실제 종단 연결·SoftwareMouse·meta/감독은 미완료다. 기본 창/fallback과 명시 SoftwareMouse 선택을 구별한다. |
| E 실제 Thunder — 1.219 부착/호출자 검증 | `extracted/dotnet-scene-thunder-draft/README.md`, 기존 process3파일은 `extracted/dotnet-thunder-draft/`. 같은 현재 기술DWORD/Surface/Host/Kernel/Screen 부착, 실제 모드3입구·최종 종료 삽입 위치와 검사8case를 준비했다. 미빌드·미실행·추가 peer 미실시다. |
| F 초기 순서 — 1.220 실제 반영·검증 | `extracted/dotnet-scene-start-draft/README.md`. 코드·검사0이다. 현재 Prefix가 Clock/Player, Chat bool이 Issue/Chat을 함께 묶고 intro가 늦다. LoadContent의 GPU와 domain 초기화 분리 및 실패 재진입 정책을 먼저 정한 뒤 성공 단계를 구별해야 한다. |

정기1.210 뒤 제품211·213·214 세 단위를 마쳤으므로 다음 제품 단위 뒤 전체 회귀를 수행한다.1.212의7,609합산은 당시 결과이며 변경된 Core의 현재 전체 재검증이 아니다. 최근 실행과 단위 결과의 범위는1.214 문서를 따른다. NativeLoaded/게임 시각/RNG를 강제로 보정하지 않는다.


## 1.222 G2의 실제 붕괴 선행

현재 DN HostSession에는 CPP RawSurfaceWorld와 같은 World() 소유자가 없다. 새 독립 World를 만들거나 무효 칸 콜백으로 스캔 완료를 꾸미지 않는다. CPP RawCampaignSession의 실제 대피 부모인 RawBridgeCarriers도 없어서 먼저 같은 RawSquidFinder/Hash/Pool에 본체를 이식하고 독립 원문258행을 전수 대조했다. 현재 프레임 참조는 RawBridgeDecayScan.Frame(FrameTimeBox)로 제공한다. 값형 독립 호출 경로도 유지한다.

다음에는 같은 세션의 실제 삭제·Building.NotifySurface·Display.Update·현재 Carrier 이동과 위치 소리로 RawBridgeDecay를 조립한다. Client 전역 스캔은 메뉴의 빈 저장소에서도 실행하되 적격 칸이 선택됐을 때만 현재 Runtime을 빌린다. Wind→MonitorTime→BridgeDecay→SceneFrame→Minimap 순서와 실제 모달을 연결하고 Movie 잠금/게임 Pause/정상 진입을 검증한다.1.222는 대피 본체·시각 참조 선행만 완료이며 실제 G2와 H는 남는다.


## 1.223 G2 실제 Frame 연결

ClientBridgeDecay.Run은 실제 같은 Surface/권한과 FrameTimeBox를 사용해 메뉴에서도 전역 스캔을 수행한다. 적격 칸에서만 현재 전체 Runtime을 빌리고 HostSession.DecayBridgeCell로 전달한다. 세션은 같은 Native Surface·지형/Graph·현재 사제 이동 역할·현재 회피 경계·실제 가상 삭제/표면 통지/표시와 균열 음향을 사용한다. 풀만 공유하고 Graph/장부를 따로 가진 조합은 거부한다.

제품 Update에서 현재 모달을 읽고 Wind→MonitorTime→BridgeDecay→SceneFrame→Minimap/Gump/Kernel 순서를 연결했다. 메뉴 Kernel도 실행하며 Movie 잠금에서 생략한다. 게임 Pause에서도 Scene은 동적 벽시각을 읽는다. 실제1-1 Pause/벽시각·Movie 잠금/해제 관측과 대표3진입·화면 비교는1.223 문서를 따른다. 자연 붕괴/대피 플레이·전체 State·카메라/AI 전체 프레임 완료를 의미하지 않는다. 다음은 H State7/정리 Start·결과 공유와 실제 Load/cleanup 부모다.


## 1.224 H의 실제 요청과 공통 시작 부모

CPP CampaignClientServices의 실제 RunCampaignMissionStarting은 현재 Client/Surface/로컬720BYTE를 빌린 RawLocalMissionState.PollStarting(body)로7의 큰 몸체와 즉시8의 접속을 확정한다. 기존 DN의 MissionFortRequestGlobals.Next는 별도 배열뿐이어서 실제 State7으로 넘길 수 없었다.1.224에서 실제 next144BYTE/ZonePending/현재 주소·포트 대여와 요청의 관측 캡처를 추가했다. 독립 원문144행을 독립/실제 결선 모두에 적용했고, Prepare/Capture와 현재 모드·프레임/제목/Player 별칭 및7→즉시8 확정을 확인했다.

Prepare는 독립 호출자가 next→current 복사를 직접 맡는 경로이며 Run과 순차 호출하지 않는다. Run의 실제 body는 이미 next를 current로 복사받으므로 Capture부터 시작한다. request.NextSnapshot은 Init 관측 캡처일 뿐 현재 Native 상태를 다시 덮는 근거가 아니다. 큰 몸체 실패의 선행 쓰기를 되돌리거나 임의7/8을 합성하지 않는다.

다음 H에서 실제 Game의 cleanup/UI·옵션/State7 시작 몸체·현재 접속을 연결한다. 개발용 StartMission을 먼저 감싸거나 StartScene(true)를 진입 뒤에 임의 추가하지 않는다. 전체 Native Load9단계와 미션 플레이 완료는 여전히 별도 R6 의존이며1.224는 Game의 실제 진입 호출자를 교체한 단위가 아니다.


## 1.225 현재 Guide의 별도 Fort 요청

WriteCurrentGuide와 HostTutorialInitialization.RequestCurrentGuideFort를 같은 실제 세션의 Subscriber 본체/Template/설정에 연결했다. 같은 Guide를 보존하고 Subscriber 복귀 뒤 현재 한 슬롯/주소/포트, 복제 뒤 순번, 제목 효과 뒤 현재 Fort 이름을 관측한다. 미소비 요청은 덮지 않고 소비된 이전 요청은 정상 새 요청 뒤에만 교체한다. 원문144요청/54Subscriber를 독립/실제 결선 모두에 적용하고 실제 세션과 실패 접두까지 집중14/14를 확인했다. 현재 Game의 State1/7 호출자 교체와 전체 정리/적재 완료가 아니다. 다음 H의 실제 CD 확인·cleanup/UI/옵션·State7 몸체/즉시8 접속 순서를 유지한다.


## 1.226 실제 CD 장치와 최초 설정 순서

같은128BYTE 캐시/별도 결과·serial과 현재 파일 이름 고리를 Windows 드라이브/파일 속성에 연결했다. 실제 Program은 설정 적재 전에 최초 확인하고 Game가 같은 소유자를 인계받는다. 독립492행 포함 집중63/63, 실제 메뉴/1-1 재확인·콘텐츠 재적재와 대표3진입 회귀를 확인했다. 전체 Native 시작/SP 선행 순서나 State7의 현재 CdPresent 대입까지 완료한 것이 아니다. 다음 H는 실제 cleanup/UI/옵션을 연결하고 CD→명령창→시각/출처→현재 battle/Scene Start→Zone/세션→즉시8 순서를 유지한다. 단위 근거는1.226 문서를 따른다.
