# cpppj 클라이언트 소리·음악 부착

> 후속 갱신(2026-10-10, HJOW-Athlon): [날씨 팔레트·색 표·화면 갱신](cpp-weather-palette-integration.md)을 실제 화면 경계에 연결하고 해상도 변경 시 색 손실을 수정했다. 현재 자료에는 네 날씨 COL이 없어 별도 합성 사본으로 연결을 검증했다. 희생·대기실·번개는 남아 있다. 아래 미연결 설명은 소리 부착 단계 당시 이력이다.

> 후속 갱신(2026-10-10, HJOW-Athlon): [위치 효과음의 화면·카메라 연결](cpp-sound-view-integration.md)을 완료했다. 아래 SoundView 미연결 설명은 부착 단계 당시 이력이다. 원본 가변 패널 영역과 나머지 효과음 호출자/월드 프로세스 연결은 남는다.

2026-10-10, 실행·분석 PC **HJOW-Athlon**(이 PC에서 클론 창 실행이 허용됐다). 복원해 둔 효과음·음악·장면 음악 모듈을 **클라이언트(`Client::Run`)에 붙였다.** 이제 `NetstormCpp --run`이 원본처럼 소리 장치를 열고, 메뉴에서 `ser22.mus`를 틀고, 미션에 들어가면 원소 곡을 틀고, 옵션 메뉴의 소리·음악·음량 버튼이 장치에 반영된다.

디컴파일 소스(`extracted/`)는 바꾸지 않았다. 새로 읽은 근거는 기존 정밀 디컴파일 `extracted/refined/originals/Netstorm.c`와 원본 실행 파일의 기계어 직접 판독이다(`python tools/exe_callscan.py --dis/--str/--refs`). **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-10**(변동 없음).

## 원본에서 확인한 순서

| 원본 | 하는 일 | 클론 |
|---|---|---|
| `00438dc0` "init sound" | `004aa600(soundQuality)` 장치 → `004a9520("thunderCrack.wav")` 미리 적재 → `004aadd0()` 음악 스레드 | `ClientAudio::Initialize` (`Client::Run`의 화면 모드 설정 뒤, Renderer 앞) |
| `00438dc0` "loadContactList" 다음 | `00469fc0()` 장면 음악 시작(전투 밖 → `ser22.mus`) | `ClientAudio::StartScene(false)` (UberGump 생성 앞) |
| 메인 루프 3단계 | `004a8e60` 재생 중인 효과음 수 다시 세기 | `ClientAudio::CountPlaying` |
| 메인 루프 갱신 목록 | `00469f60` 곡 끝 확인(프로세스 커널 `00471a30`보다 앞) | `ClientAudio::SceneFrame` |
| Interpret Options `00435605`~`0043566a` | 소리가 켜져 있으면 장치 열기 아니면 닫기 → 효과음 음량 `004a9f10` → 음악 음량 `004aa5d0` → `00435160(현재 곡 이름, 음악 옵션)` | `ClientAudio::Interpret` (옵션 메뉴가 부른다) |
| `004b6dd0`의 미션 시작 절차 | 전투 여부를 켜고 `00469fc0` | `UberGump::BeginMission` 끝에서 `StartScene(true)` |

**프레임 갱신 정지**(`DAT_0054dc58`)는 영상 창이 열렸을 때만이며 게임 시계 정지(안내 창·브리핑)와 다르다. 그래서 곡 끝 확인은 `Client::Paused()`와 상관없이 매 프레임 부른다.

### 읽은 설정 키와 음량 단계 표

설정 읽기는 `00435220`이 `FUN_00441270`(= `ConfigInterface::ReadInt`)으로 읽는 이름이다: `sound`(`0054daa0`)·`music`(`00531880`)·`soundQuality`(`005318fc`)·`soundVolume`(`00531900`)·`musicVolume`(`00531904`)·`maxSimulSounds`(`005424a8`)·`swapLeftRightSpeakers`(`0054db8c`). 기본값은 setup.cfg·options.cfg의 `sound=1`·`music=1`·`soundQuality=3`·`maxSimulSounds=8`·`soundVolume=3`·`musicVolume=2`다.

**`ReadInt`는 값이 변수의 현재 값과 다를 때만 변수를 바꾼다.** 처음 구현에서 변수를 0으로 시작해 읽어 설정이 `0`일 때(소리·음악 끄기)를 놓쳤고(옵션 메뉴 스모크가 잡았다), 필드의 기본값에서 시작하도록 고쳤다.

음량 단계는 점프 표 `004359d0`(효과음)·`004359e0`(음악)이다: `단계 - 1`을 부호 없이 3과 비교하므로 **1 → -4000, 2 → -2000, 3 → -1000, 4 → -500, 그 밖(0·음수·5 이상) → 0**(`VolumeFromStep`).

## 구현

- [`ClientAudio`](../../cpppj/src/client/ClientAudio.h) — `SoundList`·`SoundState`·`SoundDevice`·`SoundPlayer`·`MusicFileStore`·두 `MusicChannel`·`SoundMusic`·`MusicRuntime`·`MusicSelection`·`SceneMusic`을 한 객체로 묶는다. 생성자는 OS 자원을 열지 않고 `Initialize`가 장치→미리 적재→스레드 순서로 연다. **종료는 스레드를 먼저 합류한 뒤 장치를 닫는다**(`CloseDevice`; 원본 `004a8ef0`에는 없는 순서 보장이다). 소리를 끄면 같은 순서로 닫고, 켜면 다시 연 뒤 현재 곡 이름을 음악 옵션으로 다시 고른다.
- [`SoundPlayer::Preload`](../../cpppj/src/client/Sound.h) — `004a9520`: 이름 조회 → 버퍼가 0이면 적재 → 적재된 버퍼가 있으면 감쇠도 적는다. 준비·옵션을 검사하지 않는다. **기계어를 직접 판독했고 x86 대조 도구는 적용하지 않았다**(10줄 남짓의 포장 함수이고 CD판 대응은 찾지 않았다).
- [`Client`](../../cpppj/src/client/ClientMain.h) — `Audio()`·`ApplyAudioOptions()`·`ReadAudioOptions()`. 소멸자가 화면보다 먼저 소리 묶음을 닫는다.
- [`UberGump`](../../cpppj/src/client/UberGump.cpp) — 미션 시작 `StartScene(true)`, 전투 이탈 `StartScene(false)`, 옵션 메뉴의 소리·음악·좌우 바꿈 토글과 음량 단계 선택 뒤 `ApplyAudioOptions()`.

### 새 명령줄·환경 변수(검사 전용)

- `--no-audio` 또는 환경 변수 `NETSTORM_CPP_NO_AUDIO`(비어 있거나 `0`이 아님): 소리 묶음을 만들지 않는다. **기존 자동 검사 도구(`tools/cpp_smoke_files.py`를 쓰는 다섯 스크립트)는 이 변수를 기본으로 켜서 소리를 내지 않는다.**
- `--mute-audio`: 장치를 열고 음악을 실제로 스트리밍하되 공유 음소거(`PushMute`, -10000)로 시작한다. 소리 검사가 PC에서 소리를 내지 않게 한다.
- `--audio-report <파일>`: `--frames` 종료 직전의 소리 상태를 `이름<탭>값` 줄로 저장한다.

## 의도적 차이와 아직 비어 있는 경계

- **희생 판정·대기실 판정·화면 갱신·날씨 팔레트·번개 효과**는 월드/화면이 없어 아직 비워 두었다(희생 판정은 항상 거짓, `ascendancyPalette` 조회도 거짓으로 둬 "색만 바꾸고 팔레트는 안 바꾸는" 중간 상태를 피했다). 천둥 곡의 `thunderCrack.wav`는 재생되고 번개 화면 효과는 없다. 효과음의 화면 영역(`SoundView`)도 채우지 않았다.
- **전투 이탈**: 원본은 `004b2df0`(전투 값이 남은 채 `Start`, 이어 전투 값 0)과 결과 화면 종료 `004b7453`(`ser22.mus` 요청)으로 나누는데 클론은 `StartScene(false)` 하나로 합쳤다. **결과 화면(fanfare·defeat)과 제단 희생 시작의 `Request`는 월드가 없어 아직 부르지 않는다.**
- 첫 원소 곡을 고르는 난수는 `ClientAudio`가 따로 소유하고 `timeGetTime()`으로 시드한다(원본은 전역 난수를 시간으로 다시 시드한다 `004559d0`; 월드 난수 연결은 월드 복원 때).
- 원본은 소리 옵션과 무관하게 "init sound"에서 장치를 연다. 클론도 같다(`sound=0`이어도 장치를 열고 재생만 하지 않는다).
- 미션 시작에서 `StartScene(true)`를 부르는 위치는 `004b6dd0`의 미션 시작 절차에 대응시킨 것이다. 원본은 브리핑 스크립트 창이 그 전투 안에서 열리므로 브리핑 중에도 전투 음악이 흐른다(녹화 대조: [music.md](music.md) 4절, 미션 진입 직후 첫 곡 시작).

## 검사

모든 창 실행은 이 PC에서 허용됐다(사용자 지시). 소리는 `--mute-audio`로 내지 않았고 설정 파일은 실행 전후로 복원했다.

1. **단위 검사 3개**(`ClientAudio_*`): 음량 단계 표(범위 밖 경계 포함), `Preload`의 적재 조건(처음만 적재·장치 없음 재시도·소리 없음 표식·준비 여부 무시·빈 이름 거부), 장치 없는 장면 전환(메뉴 곡·끝 시각 `+180초`·전투 곡 순환·직접 요청·0 창 핸들 거부·반복 종료).
2. **명시 검사 `netstorm_tests.exe --inspect-client-audio originals`**(숨긴 창, 음소거, 약 12초): 실제 DirectSound와 원본 `ser22.mus`로 **초기화 → 메뉴 곡 스트림 진행 → 소리 끄기(스레드 합류·장치 닫기) → 켜기(재개) → 전투 곡 → 종료를 두 번 반복**해 통과했다.
3. **창 스모크 [`tools/cpp_audio_smoke.py`](../../tools/cpp_audio_smoke.py)**(`NetstormCpp --run`, 음소거, 약 1.5분): 메뉴·전투·옵션 메뉴의 실제 마우스 버튼 조작·설정 끔·무음 환경 변수를 검사한다. 확인한 값(메뉴): 장치 열림, 음악 스레드 동작, `ser22.mus` 활성, 곡 길이 **203.947초**(데이터 17,988,096바이트 ÷ 88,200), 곡 끝 시각 = 시작 + 곡 길이, 장치 버퍼 상태 **5**(재생+반복), 재생 커서가 전진하고 **읽은 위치가 링 버퍼(176,000바이트)를 여러 번 넘음**(스레드가 장치 커서를 따라 다시 채움), 스레드 오류 0. 전투: 미션 진입 시 원소 곡 하나(색인 0~3 가운데 난수, 곡 길이는 `thu22.mus` 224.96초 등 [music.md](music.md) 표와 일치), 천둥 곡에서만 효과음 재생 횟수 증가. 옵션: 음악 끄기(채널 정지, 이름 유지)·끄고 켜기(같은 곡 재시작·스트림 전진)·소리 끄기(장치 닫기, 스레드 합류)·소리 끄고 켜기(장치 재개방과 곡 재시작)·음량 단계 4와 1이 음소거 중 예약값 **-500/-4000**으로 반영, 설정 `sound=0;music=0`은 곡을 시작하지 않음. 실행 뒤 원본 파일 **1,392개** 불변.
4. **기준선 회귀**: 변경 전·후 기존 창 스모크 4종(`cpp_window_smoke`·`cpp_renderer_smoke`·`cpp_menu_smoke`·`cpp_world_smoke`)이 모두 통과했다(무음 환경 변수 아래).
5. 변이 확인: 새 변이 10개(`audio-*`·`preload-*`)를 저장소 밖 사본에서 실행해 **10개 모두 검출, 미검출 0**(첫 실행 기준, 동등 변이 없음).

## 아직 검증하지 못한 것

- **실제 청취**(소리 크기·음질·끊김·음악/효과음 균형): 모든 검사는 음소거라 사람이 들어 보지 않았다. 장치 소실·음질 하향·긴 재생(곡 반복)·창 포커스 변화 중 동작은 확인하지 않았다.
- 효과음을 내는 호출자(건설·전투·UI 클릭 등)와 위치 기반 좌우/음량(`SoundView`)은 월드 복원과 함께 연결한다.
- 결과 화면·제단 희생·대기실의 곡 요청, 날씨 팔레트·번개 화면 효과.

## 최종 검증

Release 경고/오류 **0**, CTest 내부 **533개·실패 0**(146.06초), 원본 근거 감사 **81종 모두 통과**(36초; 이번 작업은 대조 도구·fixture·근거 JSON을 바꾸지 않았다), 새 변이 **10개 모두 검출**. 새 소리 스모크·명시 검사·기존 창 스모크 4종 통과, 실행 뒤 원본 파일 불변(스모크가 1,392개 해시 확인).

작업 중 스모크가 잡은 문제 두 가지(둘 다 고쳤다): ① `ReadAudioOptions`가 변수를 0으로 시작해 읽어 설정 0(끄기)을 놓쳤다(원본 `ReadInt` 의미, 위 참고). ② 소리 스모크 시나리오 사이에 옵션 메뉴가 바꾼 `music=0`이 `options.cfg`로 새어 다음 시나리오가 그 상태로 시작했다 — 시나리오마다 설정 파일을 보관·복원한다. 또 UI 스크립트는 클릭마다 다음 프레임에 `report`를 넣어야 다음 클릭이 새 화면 구성의 컨트롤을 찾는다.

기록: `extracted/cpp-audio-smoke-run{1..5}.log`(1~4는 위 두 문제로 실패한 중간 실행, 5가 통과)·`extracted/cpp-audio-smoke/*.txt`(시나리오별 소리 보고)·`extracted/baseline-window-smokes.log`(변경 전)·`extracted/after-window-smokes.log`(변경 후)·`extracted/clientaudio-mutation.log`·`extracted/clientaudio-audits.{json,log}`.

## 재현

```powershell
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build --build-config Release --output-on-failure
cpppj/build/bin/Release/netstorm_tests.exe --filter ClientAudio_
cpppj/build/bin/Release/netstorm_tests.exe --inspect-client-audio originals   # 실제 장치, 숨긴 창, 음소거
python tools/cpp_audio_smoke.py                                             # 창 실행, 음소거, 설정 복원
```

직접 들어 보려면(원본 `d/options.cfg`가 갱신된다 — 허용된 파일): `cpppj/build/bin/Release/NetstormCpp.exe --run originals --window`. 기본 음량은 효과음 3단계(-1000)·음악 2단계(-2000)이고 옵션 메뉴에서 바꿀 수 있다.
