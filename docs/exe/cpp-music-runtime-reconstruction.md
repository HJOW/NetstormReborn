# 두 음악 채널·작업 스레드 초기화/종료 복원

2026-10-10, 마지막 디컴파일 수행 PC **HJOW-Athlon**(192.168.0.94). [musicruntime 목록](../../tools/ghidra/musicruntime-functions.json) **3/3/3개**를 같은 PC의 세 Ghidra 프로젝트에서 읽기 전용으로 내보냈다. 원본 게임·복사본·클론 창·실제 오디오 장치는 실행하지 않았다. C++ 검사는 **실제 Windows 이벤트/스레드와 임시 Winmm 파일**을 사용한다.

[MusicRuntime](../../cpppj/src/client/SoundMusicRuntime.h)을 기존 [음악 채널/갱신](cpp-music-stream-reconstruction.md)·[열린 파일 소유자](cpp-music-open-reconstruction.md)·[상위 곡 선택](cpp-music-selection-reconstruction.md)에 연결했다. 초기화/종료 정책은 원본과 대조하고 실제 Windows 경계의 강제 스레드 중단은 **협조 종료와 완전 합류**로 바꿨다. GUI 옵션·HWND/장치·클라이언트 부착과 실제 소리 청취는 후속이다.

## 주소와 원본 정책

| 역할 | 10.78 | CD/추가 10.37 |
|---|---|---|
| 두 채널 초기화 | `004aadd0` | `00439000` |
| 기본 채널 Stop·스레드 종료 | `004aaf00` | `00439130` |
| 스레드 진입 | `004aad90` | `00438fc0` |
| 스레드/종료 이벤트/스레드 번호 | `005c7b50` / `005c7b54` / `005c7b58` | `0051a758` / `0051a75c` / `0051a760` |
| 음악 준비 | `005c7b5c` | `0051a764` |
| 두 채널 raw96 | `005c7b60` / `005c7bc0` | `005650b8` / `00565118` |

초기화는 이미 음악 준비 상태이면 아무 일도 하지 않는다. 그렇지 않으면 **두 채널**의 Buffer/BufferBytes/Flags/File/Length·Duration·읽기/쓰기 위치·format 18바이트·DataOffset/StopDepth를 0으로 만들고 각 잠금을 초기화한다. **+0x14의 4바이트·+0x3a의 2바이트 패딩은 보존**한다. raw의 +0x44 잠금 공간은 명시 API 경계이며 실제 호스트 mutex를 이 바이트에 쓰지 않는다.

소리 준비가 없으면 진단만 남기고 음악 준비는 세우지 않는다. 소리가 준비됐다면 익명 수동 리셋/초기 비신호 이벤트→스레드 순서다. 스레드 생성 실패는 로그와 assert(10.78 줄 0x6da/CD 0x6d5), 이벤트 실패는 스레드가 성공한 경우 assert(0x6e0/0x6db)를 기록한다. **보고가 돌아오면 실패한 경우도 음악 준비=1**인 원본 정책을 보존한다. ID 출력은 성공 여부와 별개로 공급한 커널 응답을 반영한다.

worker는 이벤트가 없으면 0을 반환한다. 이벤트가 있으면 기본 채널을 먼저 갱신하고 **500ms** 대기한다. `WAIT_TIMEOUT=0x102`에서만 반복하며 신호·포기·실패는 종료한다. 10.78은 준비 검사 후 채널 FillBuffer, CD는 준비 검사가 들어 있는 공개 갱신을 호출한다. 두 번째 채널은 초기화/잠금 삭제에 포함되지만 **주기 갱신과 공개 종료 Stop은 기본 채널만** 수행한다.

종료는 음악 준비가 있을 때 기본 채널 Stop을 먼저 한다. 스레드가 있고 0ms 조회가 비신호이면 이벤트를 신호한다. 신호 실패는 중단 요청을 한 번 하고, 그 뒤 **5000ms** 대기한다. TIMEOUT에서 중단/5000ms 대기를 최대 두 번 더 요청한다. **두 번째 중단 뒤 성공해도 마지막 “will not die” 로그를 남기는 원본 분기**를 유지한다. 다른 대기 실패 값은 TIMEOUT 재시도를 하지 않는다. 두 채널 잠금 삭제 관찰→스레드 핸들 닫기/0→이벤트 닫기/0→음악 준비=0 순서이며 ID는 보존한다.

## 실제 Windows 수명과 동기화

`MakeWin32MusicRuntimeHooks`는 `CreateEventW`·`CreateThread`·`WaitForSingleObject`·`SetEvent`·`CloseHandle`을 연결한다. worker callback은 힙에서 소유하며 원본 CreateThread의 사라지는 지역 변수 주소를 사용하지 않는다. 생성 실패는 callback을 반납하고 정상 스레드 진입은 callback 소유권을 회수한다.

원본의 **TerminateThread 위치/재시도는 정책 검사에서 그대로 관찰**한다. 실제 경계는 중단 요청을 atomic 종료 표식/이벤트 신호로 바꾼다. 표식은 신호 실패 때도 다음 이벤트 대기에서 종료시키고, 스레드 닫기는 완전 합류를 기다린다. 잠금을 가진 스레드를 강제로 없애거나 detach한 채 채널·파일·DLL을 지우지 않는다. 호스트 채널 mutex는 채널 객체가 계속 소유하므로 원본 잠금 삭제 관찰도 worker가 접근할 호스트 mutex를 파괴하지 않는다. **호스트 callback/장치 IO가 영구히 막히면 마지막 합류는 무기한 기다릴 수 있다.** 이를 원본 강제 중단과 비트 단위로 같은 OS 동작이라고 주장하지 않는다.

[SoundMusic](../../cpppj/src/client/SoundMusic.h)의 재귀 명령 잠금으로 Play/Stop/음량/공유 음소거/Update와 준비 상태 변경을 직렬화한다. 종료는 먼저 새 시작을 거부하고 기본 채널을 Stop한 뒤 **명령 잠금을 풀고 스레드를 기다린다.** worker가 같은 잠금을 기다리는 상태에서 join하는 교착을 막는다. 종료 대기 중 음량 값은 저장하되 버퍼에 적용하지 않는다. 초기화는 worker가 시작해도 명령 잠금이 풀린 뒤 준비 완료 상태를 읽게 한다.

음악 채널의 Active 조회도 채널 잠금으로 보호한다. 직접 raw 접근은 worker 종료/합류 뒤에만 한다. MusicSelection의 옵션/현재 곡은 주 스레드에서 관리하며 worker는 이름을 읽지 않는다. SoundState/효과음 표의 다른 필드를 임의로 여러 스레드에서 바꾸는 API는 아니다.

[MusicFileStore](../../cpppj/src/client/SoundMusicFile.h)는 파일 표/단조 증가 토큰과 각 Winmm 열기/읽기/seek/닫기/위치 조회를 같은 재귀 잠금으로 보호한다. 다른 파일의 병행 IO와 Open 실패 정리의 Close 재진입을 허용한다. [SoundDevice](../../cpppj/src/client/SoundDevice.h)는 효과음/음악 COM 호출과 버퍼 토큰 표를 직렬화한다. 효과음 표/Player는 주 스레드 전용이며 실제 COM 동시 재생은 이번 검사에서 실행하지 않았다.

수명 순서는 **SoundDevice.Initialize → MusicRuntime.Initialize → 곡 선택 → MusicRuntime.Shutdown → SoundDevice.Shutdown/파일 소유자 종료**다. DirectSound Lock에서 받은 span은 Unlock까지 유효해야 하므로 장치 Shutdown을 worker 종료보다 앞세우면 안 된다. 실제 GUI의 장치 연결은 아직 이 순서에 부착하지 않았다.

worker의 호스트 예외는 프로세스 밖으로 나가지 않고 `WorkerFailure`에 보존된다. 파일/버퍼 정리는 이후 Shutdown이 수행한다. 초기화 report의 진단 예외도 이미 만든 스레드/이벤트를 정리한 뒤 전달한다. 커널 생성/대기/신호/중단/닫기·잠금 관찰·log callback은 예외를 던지지 않는 계약이다.

## 독립 관찰과 검사

[실행기](../../tools/decomp_musicruntime_oracle.py)는 **세 판본 각각 1,600개, 총 4,800개**를 두 x87 제어값에서 정상 반환까지 실행한다. 판본마다 초기화 512회·종료 1,536회·worker 1,728회, 전체 직접 호출 **11,328회**와 초기 Lookup 포함 root **30,528회**다. 이번 두 정밀도 관찰은 일치한다. 앞 음악 열기 단계의 64비트 중간 연산 차이는 그대로 남는다.

각 판본 입력은 초기화 64개·종료 576개·worker 864개·두 번 초기화/종료 96개다. 준비/소리·활성/high flags·기존 버퍼·핸들/생성 실패·ID 출력·이벤트 신호 실패·0/포기/실패/TIMEOUT/재시도·COM 실패 후 Stop·기본 채널 파일 읽기와 패딩 보존을 교차한다. 예상한 생성 실패 assert는 두 제어값에서 **각 판본 96회, 총 288회** 기록했고 그 외 assert는 0이다.

커널 API·COM·메모리 파일/잠금·기록/assert/패치 CRT 로캘·초기 배치만 명시 대체한다. **초기화/종료/worker·공개 Stop/FillBuffer·Read/Rewind/helper는 실제 원본 명령**이다. 두 번째 채널 raw96와 기본 채널 raw96, 출력128·준비/핸들/ID·메모리 파일 위치/열림·사건 순서·worker 반환을 [fixture](../../cpppj/tests/fixtures/musicruntime-x86.tsv)에 보존했다. 초기화로 잊힌 입력 파일을 0/다른 토큰의 닫기로 반납했다고 처리하지 않는다. 실제 스레드 스케줄링/Win32 구조체 배치는 Native fixture의 동치 주장 범위 밖이다.

[근거](../../cpppj/recovery-musicruntime-evidence.json)는 정확한 전체 입력/행·직접 반환/정책 진입·ABI/스택/보존 레지스터/SEH/x87·허용 명령/쓰기·잠금 균형·명시 실패 보고/예기치 않은 assert 0·OS 0·SHA를 감사한다. 기존 fixture/도구는 수정하지 않았다. 누적 독립 관찰 **444,469개**다.

[SoundMusicRuntimeTests.cpp](../../cpppj/tests/SoundMusicRuntimeTests.cpp)의 새 검사 **8개**는 원본 관찰 전수 비교, **실제 Windows worker·이벤트·Winmm 파일의 3회 시작/첫 채우기/종료/재시작**, worker 예외의 진단/합류·파일 닫기, 종료 대기 중 새 Play 거부/잠금 반납, 두 읽기 스레드의 **400회 파일 수명·토큰 미재사용**, 연결 누락/초기화 진단 예외 정리를 확인한다. 테스트 검사 전역을 실제 worker에서 수정하지 않는다. 기본 CTest에 원본 자산/PE/Python/오디오 장치/창은 필요 없다.

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name musicruntime
python -X utf8 tools/decomp_musicruntime_oracle.py
python -X utf8 tools/decomp_musicruntime_oracle.py --verify
cmake --build cpppj/build --config Release --parallel 4
cpppj/build/bin/Release/netstorm_tests.exe --filter SoundMusicRuntime_
ctest --test-dir cpppj/build -C Release --output-on-failure
```

Release 경고/오류 **0**, CTest 내부 **525개·실패 0**(148.36초, 전체 148.40초 — 근거 감사와 병행), 근거 감사 **80종 모두 통과**. 최종 회귀/감사 결과는 [LEFT_JOBS.md](../../LEFT_JOBS.md)에 기록한다. 다음은 HWND/실제 장치·GUI 옵션·장면별 음악/실시간 곡 전환과 클라이언트 수명 연결이다. 실제 오디오 장치·음질 하향·장치 소실·청취·반복 창 실행도 미검증이다. raw GUI 건설·경제·전투·AI·승패가 남아 **미션 완주는 아직 불가능하다.** 영어 원본 글꼴·outpost/LAN 3차·한국어/D2Coding/화면 요구사항/MCP 4차 순서를 유지한다.
