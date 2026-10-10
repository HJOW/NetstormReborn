# 음악 버퍼 생성·링 버퍼 갱신 복원

2026-10-10, 마지막 디컴파일 수행 PC **HJOW-Athlon**(192.168.0.94). [musicstream 목록](../../tools/ghidra/musicstream-functions.json) **2/4/4개**를 같은 PC의 세 Ghidra 프로젝트에서 읽기 전용으로 내보냈다. 게임·창·실제 오디오 장치·작업 스레드는 실행하지 않았다.

[SoundMusic.h](../../cpppj/src/client/SoundMusic.h)·[SoundMusic.cpp](../../cpppj/src/client/SoundMusic.cpp)에 버퍼 생성 `EnsureBuffer`와 링 버퍼 갱신 `FillBuffer`, 준비 상태의 공개 갱신 `SoundMusic::Update`를 추가했다. 앞 단계의 실제 `Read`/`Stop`과 연결한다. [SoundDevice](../../cpppj/src/client/SoundDevice.cpp)의 `BindMusicBuffers`·`MusicBuffers`는 파일 경계에 실제 DirectSound 음량/정지/참조 해제·생성/상태/복구/커서/잠금/해제/재생을 연결한다. **음악 파일 열기·두 채널/이벤트/작업 스레드·실제 장치/클라이언트 연결은 후속이다.**

## 주소·원본 계약

| 역할 | 10.78 | CD/추가 10.37 |
|---|---|---|
| 음악 버퍼 생성 | `004aa040` | 공개 음악 시작 `004393b0` 안의 인라인 구간 |
| 버퍼 채우기 | `004aaad0`(채널 thiscall, 성공 여부 반환) | `00439a20`(기본 채널 공개 cdecl, void) |
| CD 초기 오류의 정지/해제 helper | 채널 `004aa9c0` 호출 | `00439e60` / `00439f60`, 이어 실제 `0043a130` |

이전 인계에서 버퍼 생성으로 적은 **CD `00439260`은 실제로 음악 파일 존재 검사**다. 버퍼 생성은 `004393b0`에 있으며 이번에 주소 설명을 바로잡았다. CD 생성은 정적 소스와 비교했고 **독립 native 생성 입력은 10.78에만 포함**한다. CD 공개 갱신은 음악 준비 전역이 0이면 잠금 없이 반환한다.

생성은 재귀 잠금 안에서 현재 버퍼가 0일 때만 x86 DSBUFFERDESC(크기 `0x14`, flags `0xe2`, 버퍼 크기 `0x2af80`=176,000, reserved/outer 0, format=채널 `+0x28`)를 넘긴다. 성공 HRESULT는 버퍼 크기를 고정값으로 저장하며, 기존 버퍼가 있으면 크기/형식을 바꾸지 않는다. **COM 출력 토큰은 HRESULT와 별개로 원본 상태에 남는다.** 실패는 잠금을 푼 뒤 `Failed to create a music buffer.`을 기록한다.

갱신은 활성 비트가 없으면 잠금/해제만 하고 반환한다. 활성 채널은 다음 순서다.

1. `GetStatus`. 실패하면 원본 문장을 기록하고 중첩 `Stop`으로 정리한다.
2. status의 손실 비트 2가 있으면 `Restore`. 실패하면 같은 방식으로 정리한다. 복구 뒤 status를 다시 조회하지 않는다.
3. 현재 음악 음량을 `SetVolume`에 그대로 넘긴다. 음수 HRESULT는 `Failed to set music buffer volume in fillBuffer().`을 기록하지만 계속 갱신한다.
4. status의 재생/반복 비트 `(status & 5)`가 0이면 내부 쓰기 위치를 0으로 만들고 전체 버퍼를 채운다. 그 외에는 `GetCurrentPosition`의 **재생 커서**만 사용한다. 장치 쓰기 커서는 길이에 영향을 주지 않는다. `play < 내부 write`이면 `bufferBytes − write + play`, 아니면 `play − write`가 갱신 크기다.
5. 크기가 0이 아니면 flags 0의 `Lock`으로 두 구간을 받는다. 첫 구간을 실제 `Read`로 읽어 정확한 크기가 반환됐을 때만 내부 쓰기 위치를 증가한다. 첫 구간이 전체 요청보다 작으면 두 번째 구간도 같은 방식으로 읽는다. 쓰기 위치를 버퍼 크기로 나눈 나머지로 보정하고 **읽기 실패 때도 Unlock**한다.
6. Unlock 실패 문장을 먼저 기록하며, 읽기가 짧았으면 별도 갱신 오류 문장 없이 `Stop`한다. 파일 읽기/되감기 자체 오류와 파일 닫기는 기존 실제 채널이 수행한다.
7. 처음 status의 `(status & 5)`가 0이면 `Play(0,0,1)`로 **장치 버퍼를 반복 재생**한다. 파일 반복 비트와 별개다. Play 실패도 정리한다. 기존 재생 중이면 다시 Play하지 않는다.

한 번 재생의 EOF에서 `Read`가 0으로 채워도 실제 읽은 크기가 구간 길이보다 작으면 갱신은 Unlock 후 즉시 Stop한다. 파일 반복도 원본 `Read`의 한 번 되감기 계약을 유지한다. 중첩 Read/되감기/Stop의 잠금 사건과 정리 뒤 반복 비트 보존·파일/버퍼/format/duration 초기화를 대조했다.

호스트의 범위 밖 커서·너무 큰 버퍼·잘못된 Lock 구간은 예외다. IO 예외/잘못된 구간도 획득한 COM 잠금과 재귀 잠금을 풀어 준다. 이는 원본의 정의되지 않은 입력 밖에서 자원 누수를 막는 동작이며 native 기대값으로 주장하지 않는다. unlock 경계는 예외 정리 중 예외를 던지지 않아야 한다.

## 실제 장치 경계

`SoundDevice::MusicBuffers`는 토큰을 기존 COM 표로 바꾸어 Windows 호출에 전달한다. 잠금의 두 포인터는 호스트 span으로만 보관한다. raw 채널에 호스트 주소를 넣지 않는다. 버퍼 생성의 PCM `cbSize`는 0으로 보정하며, 소유하지 않는 추가 코덱 형식 바이트를 요구하는 비PCM 입력은 COM 호출 전에 거부한다. 음악 버퍼 참조 해제는 토큰 표에서 해당 항목을 제거하여 장치 종료의 재해제를 막는다.

사용 순서는 파일 seek/read/close 경계 준비 → `device.BindMusicBuffers(files)`로 채널 구성 → 파일 헤더/채널 상태 설정 → `EnsureBuffer(device.MusicBuffers())` → `Start` → 주기적인 `Update` → **장치 Shutdown 전에 채널 Stop**이다. 현재 장치는 단일 스레드 전용이며 작업 스레드 수명/동기화는 아직 연결하지 않았다. 생성자나 경계 준비만으로 DLL/장치를 열지 않는다.

## 독립 관찰·검증

[실행기](../../tools/decomp_musicstream_oracle.py)는 **10.78 1,704개, CD/추가 10.37 각 1,692개, 총 5,088개**를 두 x87 제어값 `0x027f`·`0x037f`에서 실행했다. 갱신/생성 직접 호출 **10,176회**, 초기 실제 Lookup까지 포함한 root **30,528회**가 정상 반환했다. 10.78 생성 입력은 12개(두 제어값에서 24회)다.

활성/준비 0·1, status 0~7, 재생/내부 쓰기 커서의 같음/앞/뒤, 일곱 COM 실패 위치, 구간 분할 없음/첫 구간 0/첫 구간 5, 파일 길이 0/1/8/16/64와 파일 안/끝·정확한 EOF, 한 번/반복 재생, 부분 읽기/IO/되감기 실패를 교차했다. status·복구·커서·Lock/Unlock·Play·생성의 **COM 결과와 파일 IO/잠금/기록만 명시 대체**한다. 채우기/Read/Stop/중첩 정리와 패치/구판의 실제 helper는 원본 명령이다.

[musicstream-x86.tsv](../../cpppj/tests/fixtures/musicstream-x86.tsv)는 사건 순서/패치 반환, 전체 raw96·두 출력 구간을 포함한 output128·파일 위치/열림을 기록한다. [근거](../../cpppj/recovery-musicstream-evidence.json)는 원본/실행기/내보내기/관찰 SHA, 전체 입력/행, 몸체 진입/정상 반환, ABI·스택/보존 레지스터/SEH/x87·허용 쓰기·잠금 균형·예기치 않은 assert/OS 0을 감사한다. 기존 fixture/감사 도구는 수정하지 않았다.

[SoundMusicStreamTests.cpp](../../cpppj/tests/SoundMusicStreamTests.cpp)의 새 7개 검사는 모든 판본 관찰과 실제 시작→전체 채우기/Play→부분 갱신→링 끝의 두 구간→짧은 읽기→Unlock/Stop, 호스트 IO 예외/잘못된 구간의 해제, 미준비/비활성/기존 버퍼 보존, 미초기화 장치 경계의 생성 실패·파일 경계 보존을 확인한다. 일반 CTest와 이번 선택 검사는 실제 오디오 장치/파일/작업 스레드를 열지 않는다.

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name musicstream
python -X utf8 tools/decomp_musicstream_oracle.py
python -X utf8 tools/decomp_musicstream_oracle.py --verify
cmake --build cpppj/build --config Release --parallel 4
cpppj/build/bin/Release/netstorm_tests.exe --filter SoundMusicStream_
ctest --test-dir cpppj/build -C Release --output-on-failure
```

누적 독립 관찰 **435,733개**. 최종 회귀/감사 결과와 다음 작업은 [LEFT_JOBS.md](../../LEFT_JOBS.md)의 최신 인계에 기록한다. 다음은 음악 파일 열기/format·duration/경로·fallback, 두 채널·이벤트/스레드 초기화/종료와 장치 동기화, 실제 장치/클라이언트 연결이다. GUI 건설·경제·전투·AI·승패가 남아 미션 완주는 아직 불가능하다. 영어 원본 글꼴·outpost/LAN 3차·한국어/D2Coding/화면 요구사항/MCP 4차 순서를 유지한다.
