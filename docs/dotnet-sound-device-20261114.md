# dotnetpj 0.516 — 실제 Windows 소리 파일·장치 경계

2026-11-14, HJOW-Athlon. cpppj 기준 `51b006882`의 `client/SoundDevice.h/.cpp`를 읽고 WAVE/파일 선택과 효과음 장치 수명을 옮겼다. [0.515 재생기](dotnet-sound-lifecycle-20261114.md)의 관측 장치 경계에 연결할 실제 Windows 구현이다. 원본 게임·디컴파일 소스·cpppj 소스는 바꾸지 않았다.

## 파일과 장치 구현

`SoundWave`는 원문 WAVEFORMATEX18BYTE·data 시작 위치·표본을 보관한다. `WindowsSoundFiles`는 읽기 전용 Winmm으로 WAVE→fmt→data를 읽는다. 짧은 fmt는 원문처럼 평균 바이트율/채널/주파수로 표본 비트를 다시 계산하고 cbSize를0으로 한다. 비PCM 짧은 fmt의 정상 복귀 진단과 PCM의 불필요한 cbSize 보존도 유지한다. Winmm이 RIFF/파일 끝으로 줄인 크기를 사용하며 파일 범위 밖이나 빈 data는 거부한다. 기존 `WaveFile`의 PCM 변환과 별개이며 표본을 재표본화하지 않는다.

파일은 주 경로의 언어 폴더→기본 폴더, 각 폴더의 `이름-*.확장자`→`이름*.확장자`, 이어 보조 경로의 같은 네 후보 순서로 찾는다. 실제 `FindFirstFileW`의 첫 결과와 대소문자를 보존한다. 전체 경로 첫 하이픈 뒤 signed long32 감쇠는 Windows SDK의 inline `sscanf`가 호출하는 UCRT `__stdio_common_vsscanf`로 읽는다. 부분 숫자·공백·음수와 경로 앞쪽 하이픈도 원문 규칙을 따른다.

`WindowsSoundDevice`는 `dsound.dll`→DirectSoundCreate→실제 HWND의 DSSCL_PRIORITY→주 버퍼→22050Hz 음질 하향을 실행한다. COM 포인터는 호스트 크기로 보관하고 이름표에는32비트 토큰만 쓴다. 장치/버퍼/복제는 하나의 재귀 잠금으로 직렬화한다. 효과음 적재는 실제 파일 선택/Winmm 읽기→CreateSoundBuffer→Lock→원문 표본 복사→Unlock→토큰 등록 순서다. 실패는 원문0/소리 없음 표식과 감쇠로 구별한다. 복제 함수는 ref 출력을 사용해 실패 시 기존 값을 보존한다.

상태·재생·위치·음량·좌우·정지·복제·적재의 여덟 경계를 실제 COM에 연결했다. 종료는 모든 효과음 정지/Release→주 버퍼→장치→DLL 순서로 반납하고 이름표의 Buffer DWORD만 비운다. 이름·감쇠·복제 사슬·토큰 순번을 유지하므로 재초기화 때 기존 복제 항목을 복구할 수 있다. 초기화 실패 위치/HRESULT는 별도 진단 속성으로 제공하며 원문 상태/호출 순서는 바꾸지 않는다.

**아직 연결하지 않은 범위:** 음악의 링 버퍼·채널·worker와 `BindMusicBuffers`/`MusicBuffers`, `ClientAudio` 소유 수명, 기존 `HostButtonSound` 교체, 캠페인 HostSession의 실제 사제/음향 장치 전체 조립이다. PORT_MAP의 SoundDevice 행은 이 부분 이식을 뜻한다. Windows 전용 경계는 호스트 프로젝트 안에 두었고 Linux 출력기는 R8 이후 범위다. MonoGame 화면 HWND와 함께 사용할 수 있지만 실제 재생은 원문과 같은 DirectSound다.

## 검증과 실제 출력 제한

- `sounddevice-x86.tsv` 세 실제 PE의435입력씩 총1305입력에서 성공 여부·전체18BYTE 형식·data 위치·모든 표본BYTE가 일치했다. 원본 자료 SHA256(LF) `dd72ca4d8b10600b3955282d5619233173c8059071fe0b3c0a0fe37f9a2447d5`. C# 출력으로 기대값을 교체하지 않았다.
- 짧거나 잘린 파일·너무 큰 data 선언·짧은 비PCM 정상 복귀·코덱 추가 자료 거부·PCM 추가 필드 보존, 여덟 파일 선택 위치, 실제 주/보조 디스크 탐색, 오프라인 복제 실패 출력23 보존, 반복 종료의 Buffer 외 전체BYTE 보존을 확인했다.
- 실제 배포 데이터의 WAV218개를 읽어 모든 표본이 원본 파일의 같은 위치BYTE임을 확인했다. 오디오 장치는 열지 않는다. 새 파일/수명8건과 선행 재생/소리 프로세스12건, 합계20/20 통과했다.
- 첫 검사2실패는 실제 UCRT에서 export하지 않는 `sscanf` 연결과 경로 구분자 문자열 기대값이었다. 로컬 Windows SDK의 inline 구현을 확인해 실제 공통 함수에 연결하고 테스트 경로만 GetFullPath로 비교했다. WAVE 원본 관측 자료는 최초 실행부터 통과했다.
- 짧은 fmt의 bits 보정을 생략한 변이는 새8검사 중 세 판본 fixture3건에서 검출했다(나머지5건 통과). `WindowsSoundFiles.cs` SHA256 `0777b703d0966785ae8e8562e0c867d512623b2e925bcf4eb87272c7f79e09a0`로 바이트 원복했다.

실제 장치 명시 검사는 **실패/미확인**이다. 숨긴 STATIC 협력 창에서 음량-10000으로 재생·복제·정지·재초기화를 수행하려 했으나 `DirectSoundCreate`가 `0x88780078 (DSERR_NODRIVER)`를 반환했다. 같은 PC·데이터의 cpppj `netstorm_tests.exe --inspect-sound-device`도 초기화에 실패했다. 오디오 서비스는 실행 중이나 출력 endpoint의 DeviceState에 활성1 항목이 없다. 소리 장치나 시스템 설정은 변경하지 않았다. 이 결과를 실제 소리 출력 성공으로 세지 않으며 활성 출력 장치가 있는 환경의 재검사를 확인 대기로 둔다. 파일/상태 이식과 이후 독립 구현은 계속한다.

실장치 검사는 일반 회귀와 분리한 xUnit Explicit 검사다. 실제 장치가 있는 Windows PC에서 데이터 사본을 사용해 실행한다. 창을 표시하거나 게임 입력을 보내지 않고 음량을 최저로 유지한다.

```powershell
$env:NETSTORM_DATA = (Resolve-Path 'assets/game-data').Path
dotnet dotnetpj/tests/Netstorm.Client.Tests/bin/Release/net10.0/Netstorm.Client.Tests.dll -class Netstorm.Client.Tests.WindowsSoundHardwareTests -explicit only -noColor -trx extracted/sound-hardware.trx
```

원복 뒤 전체 비증분 Release 빌드0경고/0오류·Client일반전체1192/1192(실장치 Explicit 클래스만 선택 제외)·메타데이터6/6 통과(실패0·건너뜀0·각종료0)다. PORT_MAP757행(L1 346+port411), 공유fixture256개다. 증거는 `extracted/dotnet-sound-device-0516/`이며 신규 배포 실행파일은 없다. 실제 게임 실행·MP, 사제 포획/희생·특정 적 공격·시나리오 승리 재현은 수행하지 않았다. M1 전체 일치0/34를 유지한다.
