# C++ 소리 장치 — DirectSound와 WAVE 적재

2026-10-10, **마지막 디컴파일 수행 PC: HJOW-Athlon (192.168.0.94)**. AGENTS.md·두 인계 문서의 우선순위를 확인하고 앞 단계의 장치 경계를 구현했다. 원본 방식인 DirectSound를 직접 호출한다. 이번 작업에서 원본/복사본 게임·클론 창·실제 오디오 장치는 실행하지 않았다. AGENTS.md·원본 파일·dotnetpj 변경과 커밋/푸시도 없다.

[`SoundDevice`](../../cpppj/src/client/SoundDevice.h)는 실제 DLL·COM 버퍼와 WAV 적재를 [`SoundPlayer`](../../cpppj/src/client/Sound.h)의 여덟 장치 경계에 연결한다. 생성만으로 장치를 열지 않는다. 실제 창 핸들로 `Initialize`를 호출해야 장치를 만들며, GUI GameWorld에 이 객체를 넣는 작업은 아직 남았다. 음악도 후속이다. 따라서 이번 변경을 게임 화면에서 소리가 나는지 확인한 결과로 해석하지 않는다.

## 원본 대응

| 몸체 | 10.78 | CD·추가 10.37 | 이번 범위 |
|---|---|---|---|
| 초기화 | `004aa600` | `00437820` | DLL/Create·협조 수준·주 버퍼·음질 하향. 음악 부착 제외 |
| 종료 | `004a8ef0` | `00437d30` | 장치/버퍼 해제·이름 표의 Buffer 비우기. 음악 종료 제외 |
| 항목 파일 선택·감쇠 | `004a9170` | `00438000` | 여덟 패턴 조회·첫 파일·감쇠 sscanf |
| 효과음 버퍼 만들기·잠금·읽기 | `004a8f70` | `00437da0` | 실제 CreateSoundBuffer/Lock/복사/Unlock |
| WAVE 열기·헤더 | `004a8b50` | `004375c0` | 실제 Winmm의 WAVE→fmt→data 순서 |
| 표본 읽기 / 닫기 | `004a8cf0` / `004a8d30` | `00437790` / `00437800` | 정확한 읽기 크기·모든 경로의 파일 해제 |

`sounddevice` 목록을 **7/7/7개**, 같은 PC에서 읽기 전용으로 내보냈다. [목록](../../tools/ghidra/sounddevice-functions.json)·[독립 대조 실행기](../../tools/decomp_sounddevice_oracle.py)·[근거 기록](../../cpppj/recovery-sounddevice-evidence.json).

## 호출과 수명

1. 이름 표와 상태, 주/보조 경로의 `MakeDiskSoundResolver`를 만든다. 기본 언어는 `english`, 소리 디렉터리는 `sound`다.
2. `SoundDevice(list,state,resolver)`를 만들고 실제 살아 있는 창의 HWND로 `Initialize(window,quality)`를 호출한다. 창 0은 DLL을 읽기 전에 거부한다. 반복 초기화는 false다.
3. 원본처럼 `dsound.dll`에서 `DirectSoundCreate`를 찾고 기본 장치를 만들어 `DSSCL_PRIORITY`를 설정한다. 주 버퍼를 만든 뒤 3=22,050Hz stereo16 → 2=stereo8 → 1=mono8 순으로 낮춘다. 0은 SetFormat을 생략한다. 모든 SetFormat이 실패해도 원본처럼 초기화 상태를 켠다. COM 생성 이후 실패는 장치를 정리하고 소리 옵션을 끈다.
4. `SoundPlayer(list,state,device.Hooks())`에 실제 장치 경계를 넘긴다. 적재는 `0xe2`(정적 버퍼와 주파수·좌우·음량 제어) 버퍼를 만든다. 전역 초점 플래그는 추가하지 않는다. 잠금은 원본처럼 한 구간이 전체 크기여야 한다.
5. 프로세스/Player를 먼저 종료하고 장치와 창을 차례로 종료한다. list/state는 장치보다 오래 살아야 한다. Hooks는 장치를 참조하므로 장치보다 오래 보관하지 않는다. 단일 스레드 전용이다.

원본은 COM 포인터를 이름 표의 DWORD에 넣는다. C++은 64비트 포인터를 잘라 쓰지 않도록 장치가 소유한 **32비트 토큰 → 실제 COM 포인터** 표를 사용한다. 0과 `0xffffffff`의 뜻은 그대로다. 종료 뒤 토큰을 재사용하지 않는다.

`Shutdown`은 모든 복제/원본 버퍼와 주 버퍼·장치·DLL을 반납하고 표의 Buffer와 initialized/device/playing만 비운다. 이름/Root/Next/감쇠/Volume/Serial/우선 인자는 유지하여 재초기화 때 원본 적재와 기존 복제 항목 복원이 가능하다. 원본 종료가 빠뜨린 버퍼/DLL도 이 객체의 소유 자원으로 정리한다. 음악 초기화/종료(`004aadd0`·`004aaf00`)는 연결하지 않았다.

## 파일 선택

`a.wav` 요청은 `a-*.wav` 다음 `a*.wav`를 찾는다. 두 번째 패턴의 `*`도 원본에 있다. 주 경로의 `sound/english` → 주 경로의 `sound` → 보조 경로의 `sound/english` → 보조 경로의 `sound`, 각 위치에서 두 패턴을 순서대로 조회한다. 첫 일치에서 끝난다. 디스크 resolver는 실제 FindFirstFile 결과를 쓰며 주/보조 결과를 합치거나 이름순으로 정렬하지 않는다.

적재 후 감쇠는 **전체 경로**에 원본 `sscanf("%*[^-]-%ld.%*[^-]")`를 적용한다. 변환 하나가 성공하면 뒤 패턴 성공 여부와 관계없이 숫자를 쓰며, 숫자가 없으면 0이다. 따라서 상위 디렉터리 이름에 '-'가 있으면 소리 파일의 숫자를 못 읽을 수 있다. 이 원본 특성도 보존한다. 파일 없음/적재 실패는 소리 없음 표식, 장치 없음은 0이다. 디스크에 없는 아카이브 자료의 임시 추출 경로와 클라이언트 설정 연결은 후속이다.

## WAVE와 호환성

읽기 전용 `mmioOpenW`를 사용하여 Windows 경로를 읽고, WAVE RIFF·fmt·data를 차례로 찾는다. fmt는 최대 18바이트다. 16/17바이트면 원본처럼 PCM 조건을 보고한 뒤 `cbSize=0`, `bits=(avgBytes<<3)/(channels*sampleRate)`로 보정한다. 18바이트 이상은 필드를 그대로 보존한다. 빈 data/불완전 fmt·0 분모·파일 밖 범위·지원 자료 없는 비PCM 확장은 거부한다.

실제 **218개 WAV는 모두 PCM**이다. 이 중 `dropPiece-500.wav`·`getPiece-500.wav`·`moveBeetle.wav`·`rotatePiece-500.wav` 네 파일의 18바이트 fmt는 **cbSize=20**을 담는다. 처음 구현에서 이를 코덱 확장으로 거부해 전수 읽기가 실패했고, PCM 헤더의 값을 그대로 보존하도록 고쳤다. 장치에 넘길 PCM 형식은 추가 자료가 필요하지 않도록 cbSize만 0으로 설정한다. 실제 COM 실행은 아래 선택 검사로 확인해야 한다.

Winmm은 잘린 data나 과장된 청크 크기를 파일/RIFF 끝까지 **줄여 반환**한다. C++도 그 반환 크기로 읽는다. 실제 디스크 크기를 추가 검사하여 범위 밖 읽기와 거대한 확보를 막는다. 파일은 지역 소유자가 성공/실패/예외 모두 닫는다. 표본을 먼저 읽고 COM 버퍼를 만들기 때문에 원본의 실패 경로에 있던 mmio/버퍼 누수를 피한다. 원본 ANSI 파일 경로 대신 Unicode 경로를 받고 현대 Windows의 ABI 크기인 `sizeof(DSBUFFERDESC)`를 사용한다.

## 검증 범위

- **독립 원본 1,305개**: 세 PE 각 435개. 실제 열기 435·읽기 288·닫기 288회씩, 총 **3,033회 정상 반환**. 16/17/18/24바이트 fmt·PCM cbSize 0/20·채널/주파수/bits·보정·홀수 JUNK/패딩·빈 data·누락 청크를 교차한다. 정수 처리로 x87 정밀도 대조는 하지 않는다.
- 에뮬레이터가 대체한 것은 **mmioOpenA/Descend/Read/Ascend/Close 다섯 Winmm 경계**다. 실제 원본의 헤더 보정/반환/쓰기와 표본 읽기/닫기 몸체를 실행한다. 실행 범위·허용 쓰기·cdecl 스택·보존 레지스터·진입/반환·assert/OS 0·SHA/정확한 입력/행/경계 횟수를 감사한다. DirectSound·파일 검색·COM은 이 독립 기대값의 범위 밖이다.
- 새 C++ 검사 **7개 통과**: 세 판본 fixture를 실제 mmio 파일 읽기로 재생, 손상/코덱/PCM 특성, 여덟 경로 순서/감쇠, 실제 디스크 대소문자/보조 경로, 무장치 Hooks·종료/필드 보존·거부 계약. 기본 CTest는 장치를 열지 않는다. `--filter SoundDevice_`로 이 모듈만 선택할 수 있다.
- 원본 자산 **218 WAV·9,758,513 표본 바이트·감쇠 24개**의 읽기와 원본 data 바이트 일치 통과. 원본 파일을 변경하지 않았다.
- 최종 Release 경고/오류 **0**, 전체 CTest 내부 **484개·실패 0**(131.49초). [최신 인계](../../LEFT_JOBS.md). 원본 근거 감사 **74종 모두 통과**. 누적 독립 입력 **422,164개**. 기존 fixture/감사 도구는 변경하지 않았다. 이번 단계의 추가 변이 검사는 실행하지 않았다.

```powershell
cmake --build cpppj/build --config Release
cpppj/build/bin/Release/netstorm_tests.exe --filter SoundDevice_
cpppj/build/bin/Release/netstorm_tests.exe --inspect-sound-files originals
ctest --test-dir cpppj/build --build-config Release --output-on-failure
python -X utf8 tools/decomp_sounddevice_oracle.py --verify
```

기록: `extracted/sounddevice-{export,oracle,build-final,tests,assets,ctest-final}.log`와 `sounddevice-audits.json`.

## 실제 장치 검사와 후속

다음 명령은 **실제 오디오 장치와 숨긴 협조 창을 연다**. 음량은 -10000으로 고정한다. 기본 CTest/파일 검사에서 호출하지 않으며 **이번 작업에서는 실행하지 않았다**. 앞 인계의 기본 방침(장치 실행에 답이 없으면 코드만 작성하고 실행은 인계)을 이어 적용했다. 이 무음 검사는 실제 청취 확인을 대신하지 않는다.

```powershell
cpppj/build/bin/Release/netstorm_tests.exe --inspect-sound-device originals
```

**2026-10-10 갱신:** HJOW-Athlon에서 사용자 허가 아래 이 명령을 처음 실행해 통과했다(`DirectSound device: initialize/load/duplicate/play/stop/shutdown/reinitialize passed; volume -10000`). 클라이언트 부착 뒤의 반복 검사는 [cpp-client-audio.md](cpp-client-audio.md)를 따른다.

이 명령은 초기화→원본 보호막 WAV 적재→반복 재생→복제/좌우 설정→상태 조회/정지→종료→음질 0 재초기화→기존 복제 항목 복원→정지/반복 종료를 검사한다. COM 실패 때는 오류로 끝내고 자원을 반납한다. 실제 장치 실행·음질 하향 실패·장치 소실/다른 Windows 환경·청취는 아직 미검증이다.

다음 구현은 `Sound.cpp`의 재생 수 재계산(`004a8e60`), 이름 정지/조회/켜고 끄기(`004a9d20`·`004a9da0`·`004a9de0`), 반복 소리 정지(`004a9e50`), 전체 음량(`004a9f10`), 음악(`004a9fa0` 이후)이다. 이어 GameWorld의 실제 HWND/장치 수명·프레임/벽시계·화면/카메라·옵션/언어/소리 경로와 나머지 소리 요청을 연결한다. raw 월드/GUI 건설·경제·전투·승패는 아직 남아 미션 완주는 불가능하다. outpost/LAN은 3차, 한국어/요구사항/MCP는 4차이며 영어=원본 글꼴/한국어=D2Coding 정책을 유지한다.
