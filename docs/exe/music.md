# 배경음악 선택과 제단 희생 의식의 소리 (exe 정적 분석 + 녹음 대조)

2026-10-01, 원본 게임 실행 없음. 근거는 전체 디컴파일(`extracted/decomp/Netstorm.c`)과
`originals/Netstorm.exe`(10.78)의 데이터 표, 그리고 사용자 플레이 녹음
(`playingVideos/20260930T154921831Z-8bdcc06b6539/audio-*.wav`)이다. 녹음 판독 방법은 [녹화 관찰 노트](../videos/the-war-begins-record-play-20260930.md) 5절.

## 0. 사용자 설명 (2026-10-01)

1. 상대 High Priest를 포획해 제단(Altar)에 넣고 **의식을 진행하면 배경음악이 바뀐다.** 의식 음악은 모든 스테이지에서 같다.
2. 의식이 끝나고 **다른 적 사제가 아직 남아 있으면** 본래의 배경음악 루프로 돌아온다(적이 여럿인 스테이지가 있다).
3. **적 사제가 더 이상 없으면** 승리 브리핑 창(메인 메뉴로 돌아가거나 다음 미션으로 이동하는 창)이 뜨고 **게임이 일시정지**된다.

아래 exe·녹음 결과는 세 설명과 모두 맞는다. 2번의 "돌아오는 시점"은 exe 기준으로 **의식 음악 곡이 끝날 때**다(5절).

## 1. 음악 파일

| 파일 | 길이 | 쓰임 (exe) |
|---|---|---|
| `ser22.mus` | 203.9초 | 전투 밖(메인 메뉴 등). 전투 결과가 없는 메뉴 복귀 |
| `wind22.mus`·`rain22.mus`·`thu22.mus`·`sun22.mus` | 206.8 / 214.1 / 225.0 / 232.8초 | 전투(미션) 중 순환 재생 |
| `sacrifice.mus` | 139.9초 | **내 제단**에서 희생 의식이 시작될 때 |
| `anticipation.mus` | 106.5초 | 멀티플레이 대기실(다른 플레이어 입장 등). 싱글 플레이에서는 안 씀 |
| `fanfare.mus`·`defeat.mus` | 21.6 / 81.1초 | 전투 결과 화면 `victory`/`defeat` 상태(멀티플레이 결과 화면으로 추정). 캠페인 `Success!` 스크립트 창에서는 재생되지 않았다(녹음 확인) |

모두 RIFF WAV(PCM 16bit 스테레오 22,050Hz)다.

## 2. 함수

| 주소 | 역할 |
|---|---|
| `FUN_00469fc0` | 음악 시작. 전투 중(`DAT_00594fbc != 0`)이면 색인 = `FUN_004558f0(0,4000) / 1000`(0~3 난수)로 두고 `FUN_00469f00` 호출. 아니면 색인 3, `ser22.mus`. 맵 모드 초기화(`FUN_00438dc0`·`FUN_004395df`)와 전투 시작(`FUN_004b6dd0`, `FUN_004b2df0`)에서 부른다 |
| `FUN_00469f00` | 다음 곡. 색인 = (색인 + 1) mod 4. **내 희생 의식이 진행 중**(`FUN_00449220(내 플레이어)`)이면 `sacrifice.mus`, 아니면 표 `0x5409e8[색인]`을 재생하고 날씨 효과 `FUN_00469c80` |
| `FUN_00469f60` | 매 프레임 확인. 실시간 ≥ 곡 끝 예정 시각(`_DAT_00565e68`)이면 전투 중 → `FUN_00469f00`, 멀티 대기실 → `anticipation.mus`, 그 밖 → `ser22.mus` |
| `FUN_00469db0(이름)` | 재생 요청. 지금 곡과 이름이 같으면 무시. `fanfare`/`defeat`가 아직 끝나지 않았으면 `anticipation` 외 요청 무시. `fanfare`는 결과 상태 1일 때만. 곡 끝 예정 = 지금 + 길이(길이 > 30초) 또는 지금 + 180초(30초 이하) |
| `FUN_00469c80` | 곡에 딸린 날씨 효과: `ascendancyPalette` 설정이 켜져 있으면 색인별 팔레트(`windy/rainy/thundery/sunny.col`)로 바꾸고, 표 `0x5409d8[색인]`의 소리를 재생한다. **천둥 곡(색인 2)만 `thunderCrack.wav`**를 함께 튼다 |
| `FUN_00494eb0` | 제단 희생 시작 처리. 제단 소유자가 내 플레이어면 `sacrifice.mus`를 즉시 요청 |
| `FUN_00460d70` | 음악이 쓰는 시계. `timeGetTime` **실시간**이며 게임 시계(`FUN_00460cd0`)와 달리 안내 창 정지의 영향을 받지 않는다 |

### 2.1 표 (`.data`)

| 색인 | `0x5409e8` 곡 | `0x5409c8` 팔레트 | `0x5409d8` 소리 |
|---|---|---|---|
| 0 | `wind22.mus` | `windy.col` | 없음 |
| 1 | `rain22.mus` | `rainy.col` | 없음 |
| 2 | `thu22.mus` | `thundery.col` | `thunderCrack.wav` |
| 3 | `sun22.mus` | `sunny.col` | 없음 |

순환 순서는 **wind → rain → thunder → sun → wind …** 이다. 첫 곡은 난수로 고른 색인의 **다음** 곡이다.

## 3. 제단 희생 의식 단계 (`Dais`/`Altar`)

도움말 `help.english`의 `sacrificeOutline`·`immobileHelp`·`pickupHelp`·`moveyourpriestHelp`·`performsacrificeHelp` 절이 절차를 설명한다.

1. 적 사제를 체력 절반 이하로 만들면 멈추고 들고 있던 것을 떨어뜨리며 얇은 보호막(force field)에 싸인다. 사제는 계속 체력을 회복하므로 공격을 이어야 한다.
2. 수송 유닛(Transport)으로 사제를 집어 내 제단으로 데려간다(사제는 사제를 집을 수 없다. 체력이 50% 이상이면 저항한다). 제단에 오면 희생의 원(Sacrificial Circle)에 묶인다.
3. 내 사제가 제단에 있으면 의식이 시작된다. 싱글 플레이에서는 **사제를 희생해 승리**하고, 멀티플레이에서는 지식 또는 제단 업그레이드를 얻는다.
4. 내 사제가 **다섯 룬(Sun·Wind·Rain·Thunder·Storm)**을 지키면 완료된다. 도중에 내 사제가 움직이지 못하게 되거나 제단이 큰 피해를 입으면 적 사제가 달아난다.

exe 는 `FUN_00449f40`(단계 진행)·`FUN_00448080`(룬 소멸 연출)에서 제단 오브젝트의 프레임 값 − 가상 함수 +0x80 이 고르는 기준값(`0x532620` 표 `1,3,64,120,0,…`)을 11 단위로 나눠 단계를 정한다.

| 값 | 단계 | 처리 |
|---|---|---|
| 0~10 | 0 | 의식 전 |
| 11~43 | 1~3 | `altarBurnCollapse.wav` + 룬 소멸 연출 |
| 44 이상 | 4~5 | 완료: `itIsDone2.wav` → **4.0초 뒤** `priestSacrifice2.wav`(`FUN_00448170` 조건이 참이면 1.5초 뒤 `priestSacrifice4.wav` — 조건의 뜻은 미확인) → 2.8초 뒤 `thunderCrack.wav` |

`FUN_00449220`(희생 진행 중 판정)은 단계 1 이상을 참으로 본다. 완료 뒤 적 팀 사제가 모두 없어지면 미션 스크립트의 `[Succeeded][BadTeamDead]`가 열린다(The War Begins!). 스크립트 창이므로 게임 시계가 멈춘다([dialog-pause.md](../gameplay/dialog-pause.md)).
룬 하나에 걸리는 시간은 녹음에서 약 14.8초다(4절). exe 의 프레임 값 증가 속도는 아직 찾지 않았다.

## 4. 녹음 대조 (캠페인 1-1, 영상 경과 시각)

| 시각 | 소리 | 해석 |
|---|---|---|
| ~00:09 이전 | `ser22.mus`(곡 25초 지점) | 메뉴 음악 |
| 00:09.1 | `rain22.mus` 시작 | 미션 진입, 첫 곡(난수) |
| 03:42.7 | `thu22.mus` 시작 + `thunderCrack.wav` | rain22 길이(214.1초) 뒤 바로 다음 곡. 천둥 곡 효과음 동시 재생 (표 2.1) |
| 07:27.9 | `sun22.mus` 시작 | thu22 길이(225.0초) 뒤 |
| 10:58.5 | `explodeSlot`+`explosion` | 적 신전 파괴 |
| 10:59.8~11:34.4 | `priestForceField.wav` 약 0.6~1.2초 간격 반복 | 적 사제 보호막(기절) 35초 |
| 11:20.8 | `wind22.mus` 시작 | sun22 길이(232.8초) 뒤 |
| 11:35.9 / 11:38.9 | `golemPickUp` / `golemMove2` | 골렘이 기절한 사제를 들고 이동 (영상: 사제 옆 골렘이 사제를 싣고 떠남) |
| 12:05.6 | `priestMove3` | 제단 도착 |
| **12:06.4** | **`sacrifice.mus` 시작** (wind22 중단) | 의식 시작 = 사용자 설명 1 |
| 12:07.6 / 12:22.4 / 12:37.4 / 12:52.0 / 13:06.5 | `forWind2` / `forSun2` / `forRain2` / `forThunder2` / `forStorm2` | 다섯 룬 음성, 약 14.8초 간격 (`forWind2`는 상관 0.52) |
| 12:09.6~13:16.9 | `jimbuild.wav` 약 1.15초 간격, 룬마다 8~9회 | 룬을 지키는 동안의 진행음 (exe `FUN_00443c40` 건설 진행음과 같은 파일) |
| 12:19.6 / 12:34.5 / 12:49.3 / 13:04.1 | `altarBurnCollapse.wav` | 룬 소멸 (다음 룬 음성 약 2.9초 전, 14.8초 간격). 다섯 번째는 13:19 무렵 `itIsDone2`와 겹쳐 약하게만 잡힌다 |
| 13:19.0 | `itIsDone2.wav` | 의식 완료 |
| 13:23.0 | `priestSacrifice2.wav` | itIsDone + **4.0초** = exe 상수와 일치 |
| 13:28.4 | `explodeSlot` | 제단 사라짐 (영상 13:30) |
| 13:34 | (`Success!` 창, 음악 변화 없음) | 곡 위치 86.6초까지 `sacrifice.mus`가 같은 시작 시각으로 이어짐. `fanfare.mus` 없음 |
| 13:37.1 | `thunderCrack.wav` | `Next Mission` 뒤 새 미션 시작으로 추정(첫 곡이 천둥 곡이면 함께 재생) — 곡 판정은 확인 못 함 |

- 원소 곡 네 개가 **앞 곡 길이만큼 지난 뒤 0.5초 이내에** 다음 곡으로 이어졌다(9.1 + 214.1 = 223.2 ≈ 222.7, 222.7 + 225.0 = 447.7 ≈ 447.9, 447.9 + 232.8 = 680.7 ≈ 680.8). 반복 없이 곡 하나씩 순환한다.
- 이번 녹화 순서 rain → thunder → sun → wind 는 표 2.1의 순환과 같다.
- 성공 창이 열린 동안 SP는 13,350으로 그대로다(게임 정지, 사용자 설명 3). 음악은 실시간이라 계속 흘렀다.

## 5. 의식이 끝난 뒤 음악 (사용자 설명 2)

exe 에는 의식 완료 때 음악을 바꾸는 호출이 없다. `sacrifice.mus`가 끝나는 시각(시작 + 139.9초, 실시간)에 `FUN_00469f60`이 `FUN_00469f00`을 불러 **색인 + 1 의 원소 곡**으로 넘어간다. 그때 다른 희생이 진행 중이면 다시 희생 음악을 요청한다.
따라서 의식(이번 녹화 약 82초 — 다섯 룬 × 약 14.8초 + 완료 연출)이 곡보다 짧으면 의식이 끝난 뒤 남은 약 58초 동안 희생 음악이 이어진 다음 원소 루프로 돌아온다. 중단된 원소 곡(이번 녹화의 wind22)은 이어 틀지 않고 다음 곡(rain22)부터 재생한다.
이번 녹화는 마지막 적 사제라 성공 창까지 희생 음악이 이어졌고, 적이 여럿인 미션에서 원소 루프로 돌아오는 장면은 녹화로는 확인하지 않았다(사용자 설명과 exe 로 확정).

## 6. 클론 구현 (2026-10-01)

* `Netstorm.Core.Audio.MusicDirector` — 2절 규칙을 실시간 초 단위로 옮겼다(첫 곡 난수는 주입). 곡이 끝나는 순간 다시 같은 곡(희생 음악)을 고르면 원본은 요청을 무시하지만(재생이 멈출 수 있음) 클론은 처음부터 다시 튼다. 테스트 `MusicDirectorTests`가 녹화의 곡 순서·희생 음악 복귀·결과 음악 잠금을 재현한다.
* `Netstorm.Assets.WaveFile` — RIFF PCM 8/16bit 해석, 16bit 변환, 선형 보간 재표본화(6,000Hz 원본 3개용: `thunderCrack.wav`·`distantWindQuiet-3000.wav`·`ThunderQuietDistant.wav`).
* `Netstorm.Game.AudioPlayer` — 효과음은 메모리에 올려 최대 8개(`maxSimulSounds`)까지 겹쳐 틀고, 음악은 0.25초 조각으로 스트리밍한다. 소리 장치가 없으면 소리 없이 계속 실행한다. 설정 `SoundOn`·`PlayMusic`·`SoundVolume`(기본 3)·`MusicVolume`(기본 2)는 원본 `setup.cfg`/`options.cfg` 기본값이며 명령줄 `--no-sound`·`--no-music`으로 끌 수 있다. 볼륨 단계 → 음량은 단계/5 로 두었다(원본 변환식 미확인).
* 미션에 들어가면 전투 음악, 그 밖(개발용 기본 화면·맵 시험·스프라이트 뷰어)과 미션을 떠난 뒤는 메뉴 음악이다.
* `BattleSession.Sacrifice.cs`가 사제 포획·운반·알타 의식·승패 이벤트를 처리한다. `FortMapViewer`는 `MySacrificeInProgress`를 음악 감독에 넘기고, `SacrificeStarted` 때 `OnMySacrificeStarted`를 호출해 `sacrifice.mus`를 요청한다. 구현 근거와 임시값은 [sacrifice.md](../gameplay/sacrifice.md).
* 의식 효과음 연결: 포획 `golemPickUp.wav`, 룬 이름 음성 `forWind2`·`forSun2`·`forRain2`·`forThunder2`·`forStorm2`, 룬 소멸 `altarBurnCollapse.wav`, 완료 `itIsDone2.wav`, 희생 `priestSacrifice2.wav`, 알타 소멸 `explodeSlot.wav`. 제단·의식 UI는 [맵 뷰어 안내](../map-viewer.md#수송-사제-포획과-알타-의식).
* 날씨 팔레트(`ascendancyPalette`), 위치에 따른 효과음 좌우·크기, 결과 화면(fanfare·defeat) 연결은 하지 않았다.

## 7. 효과음과 사건 (exe 호출 위치)

| 효과음 | exe | 클론 연결 |
|---|---|---|
| `bridgeFall.wav` | 다리 칸 제거 `FUN_00422300` | `BridgeCollapsed` |
| `bridgeCrack.wav` | 붕괴 구동 `FUN_004227e0` | `BridgeCracked` |
| `buildDone.wav` + 타입 `buildDoneSound` | 건설 완료 `FUN_00443e20` (`buildDoneSound`는 `Rifttype.cpp`가 타입 +0x90 에 읽음: 템플 `templeComplete.wav`, 워크샵 `workshopComplete.wav`) | `BuildingCompleted` |
| `jimBuild.wav` | 건설 진행 `FUN_00443c40` | 미연결 (반복 주기 미확인) |
| `dropPiece.wav` | `FUN_004108f0`·`FUN_0042c2e0`·`FUN_004473e0` | `BridgePlaced` (호출 맥락은 이름과 위치로 추정) |
| `rotatePiece.wav` | `FUN_00446a70` | 다리 조각 회전(R) |
| `openGump.wav` / `openSubGump.wav` | 창·하위 창 열기 (`FUN_00476820` 등) | 지식 창·상세창 열기 |
| `upgradeComplete.wav` | `FUN_004545e0` | 미연결 (업그레이드 미구현) |
| `geyserMined%d.wav` | `FUN_004266c0` (값/1000 번호) | 미연결 |
| `collapse.wav` | `FUN_0044b4b0` | 미연결 |

녹음에서도 `workshopComplete`(00:37.0)·`upgradeComplete`(01:38.1)·`bridgeFall`(3회)·`openGump`가 확인됐다. 한 프레임에 같은 효과음이 여러 번 생기면(여러 칸이 함께 금 감 등) 클론은 한 번만 튼다.
