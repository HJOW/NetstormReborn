# 상위 음악 옵션·곡 전환·기본 곡 재선택 복원

2026-10-10, 마지막 디컴파일 수행 PC **HJOW-Athlon**(192.168.0.94). [musicselection 목록](../../tools/ghidra/musicselection-functions.json) **4/4/4개**를 같은 PC의 세 Ghidra 프로젝트에서 읽기 전용으로 내보냈다. 게임·복사본·창·오디오 장치·작업 스레드는 실행하지 않았다.

[MusicSelection](../../cpppj/src/client/SoundMusicSelection.h)을 기존 [공개 음악 시작/열린 파일 소유자](cpp-music-open-reconstruction.md)에 연결했다. 상위 옵션·현재 곡 이름·존재 검사·곡 전환·특수 곡 반복 여부와 실제 `demo.mus` 재선택을 처리한다. 두 채널/이벤트/작업 스레드는 [다음 단계](cpp-music-runtime-reconstruction.md)에서 완료했다. **GUI 옵션·실제 장치/클라이언트 연결은 후속이다.**

## 주소와 계약

| 역할 | 10.78 | CD/추가 10.37 |
|---|---|---|
| 전역 음악 옵션을 넘기는 래퍼 | `00435200` | `00484aa0` |
| 명시 옵션으로 곡 선택 | `00435160` | `00484a10` |
| 음악 파일 존재 검사 | `004aa480` | `00439260` |
| 실제 `strncpy` | `004e51f0` | `004f2440` |
| 전역 음악 옵션 | `00531880` | `0052e828` |
| 현재 곡 이름 256바이트 | `0054dd60` | `0052eb38` |

이름은 원본 C 로캘처럼 ASCII 대소문자를 구분하지 않고 비교한다. 다른 이름이며 **새 이름과 현재 이름 모두 비어 있지 않고 새 파일이 존재할 때만** 먼저 공개 Stop을 요청한다. 이름이 달라졌다면 옵션이 꺼져 있거나 파일이 없어도 현재 이름을 먼저 저장한다. 같은 이름이면 기존 저장 대소문자를 보존하지만 이후 공개 Play/Stop은 여전히 요청한다.

옵션은 signed DWORD이며 **0만 꺼짐**이다. 켜짐은 공개 Play, 꺼짐은 공개 Stop이다. `fanfare.mus`와 `defeat.mus`만 loop=0이고 나머지는 loop=1이다. 실제 COM Play는 스트리밍 Update의 첫 채우기에서 시작한다.

예를 들어 `old.mus`가 활성인 동안 없는 `missing.mus`를 선택하면 현재 이름은 `missing.mus`가 되지만 선행 Stop은 하지 않는다. 공개 Play의 활성 검사 때문에 **기존 음악이 계속 활성**이다. 반대로 존재하는 새 곡이면 선행 Stop으로 파일/버퍼를 반납한 뒤 새 곡을 열고 시작한다. 음악 옵션이 꺼진 경우에도 이 존재 검사/선행 정지·이름 저장 후 마지막 Stop 순서를 유지한다.

## 경로·이름 저장·fallback

존재 검사는 잠금/로그 없이 기본·보조 경로를 조회한다. [FindMusicFile](../../cpppj/src/client/SoundMusic.cpp)을 채널 Open과 공유하며 원본의 null/빈/후행 구분자 차이를 보존한다. 첫 조회 실패 뒤 보조 경로가 null이면 기존 조회 문자열에 곡 이름을 다시 붙이는 규칙도 같다. 존재 검사의 성공은 파일 헤더 열기 성공을 뜻하지 않는다.

현재 이름의 `strncpy(...,255)`는 첫 255바이트만 복사/0 패딩하고 **+255 바이트를 보존**한다. 긴 입력은 절단되지만 마지막 바이트가 NUL이라고 새로 강제하지 않는다. `MusicSelectionState::Raw`로 전체 저장 공간을 관찰하고 `Current`는 첫 NUL까지 읽는다. 긴 이름의 절단은 음악 준비 전 상태에서 native 관찰했다. 원본의 경로 overflow/null 이름/종료 없는 현재 이름은 동치 대상에서 제외하고 호스트에서 예외로 진단한다. 저장 공간과 겹치는 Assign 입력도 계약 밖이다.

공개 Play의 파일 열기 실패는 `demo.mus`가 아닐 때만 **실제 상위 래퍼**를 다시 호출한다. 이때 명시 Select 인자가 아니라 **현재 전역 음악 옵션을 다시 읽는다.** 예를 들어 `Select("track.mus",1)`이라도 전역 옵션이 0이면 fallback은 demo 이름을 저장한 뒤 Stop하며 음악을 시작하지 않는다. demo 자체의 실패는 더 재귀하지 않는다. 생성/되감기 실패에는 원본처럼 fallback하지 않는다.

`MusicSelection`은 이 래퍼를 `MusicOpenHooks::fallback`에 연결한다. 앞 단계의 명시 상위 callback 응답 대체를 제거했으며 콜백을 단순 재귀 Play로 바꾸지 않았다. 참조 상태/음악과 파일·버퍼 콜백 대상이 이 객체보다 오래 살아야 하고, 내부 callback의 this 주소를 보존하도록 복제/이동을 금지했다. 이 단계에서는 단일 스레드 전용이었다. 이후 [Runtime 단계](cpp-music-runtime-reconstruction.md)에서 SoundMusic 명령과 worker를 직렬화했다. 옵션/이름은 주 스레드에서 관리한다.

## 독립 원본 관찰과 검증

[실행기](../../tools/decomp_musicselection_oracle.py)는 **각 판본 688개, 총 2,064개**를 두 x87 제어값에서 실행한다. 직접 래퍼/선택 root **4,128회**, 초기 실제 Lookup을 포함한 root **12,384회**가 정상 반환했다. 재귀 fallback을 포함한 선택 본체 진입 **4,626회**, 래퍼 **642회**, 존재 검사 **3,834회**, 실제 strncpy **4,434회**, 실제 WAVE 헤더 **1,212회**를 기록했다. 이번 stereo16/22050 Hz 입력에서는 두 x87 관찰이 정확히 같다. 앞 음악 열기 단계의 64비트 중간 연산 차이를 해소했다고 주장하지 않는다.

이전/새 이름·대소문자·빈 이름·옵션/전역 옵션 차이·준비/장치/활성·기존 버퍼·기본/보조 null·빈/후행 경로·파일 존재/열기/청크 누락·demo 성공/실패·생성 HRESULT·seek 실패·255바이트 절단/패딩을 교차한다. 파일 조회·COM·Winmm 파일/청크·잠금/기록·패치 CRT 로캘·초기 채널/파일/이름 배치만 명시 대체한다. **상위 선택/존재/strncpy·공개 Play/Stop·헤더/길이/버퍼 확보/되감기/시작은 실제 원본 명령**이다. 게임이나 실제 OS API를 실행하지 않는다.

[fixture](../../cpppj/tests/fixtures/musicselection-x86.tsv)는 사건 순서·전체 raw96·현재 이름 raw256·현재 파일 위치·남은 열린 파일 수를 저장한다. 원본이 새 File 토큰으로 덮어써 이전 파일을 남기는 경우도 초기 열린 파일과 함께 관찰한다. C++ `MusicFileStore`의 소멸자는 남은 파일을 모두 반납한다. [근거](../../cpppj/recovery-musicselection-evidence.json)는 정확한 입력/행·진입/반환·허용 명령/쓰기·ABI/스택/보존 레지스터/SEH/x87·잠금 균형·OS/예기치 않은 assert 0·SHA와 **상위 fallback 대체 제거**를 감사한다. 기존 도구/fixture는 수정하지 않았다. 누적 독립 관찰 **439,669개**다.

[SoundMusicSelectionTests.cpp](../../cpppj/tests/SoundMusicSelectionTests.cpp)의 새 검사 **7개**는 실제 Winmm 임시 파일로 native 행 전체를 대조한다. 연속 곡 교체/대소문자 보존·한 번 재생·없는 파일의 기존 곡 유지·옵션 끄기·실제 demo 재진입/전역 옵션 차이·보조 디스크 경로·이름 저장/호스트 방어도 확인한다. 기본 CTest는 원본 자산/PE/Python/오디오 장치를 요구하지 않는다.

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name musicselection
python -X utf8 tools/decomp_musicselection_oracle.py
python -X utf8 tools/decomp_musicselection_oracle.py --verify
cmake --build cpppj/build --config Release --parallel 4
cpppj/build/bin/Release/netstorm_tests.exe --filter SoundMusicSelection_
ctest --test-dir cpppj/build -C Release --output-on-failure
```

Release 경고/오류 **0**, CTest 내부 **517개·실패 0**(126.36초, 전체 126.40초), 근거 감사 **79종 모두 통과**. 최종 회귀/감사 결과는 [LEFT_JOBS.md](../../LEFT_JOBS.md)에 기록한다. 두 채널/이벤트/worker·종료 대기·토큰 표 동기화는 [후속 단계](cpp-music-runtime-reconstruction.md)에서 완료했다. 다음은 장면별 음악/실시간 전환·GUI 옵션/장치/클라이언트 연결이다. 실제 소리 청취·raw GUI 건설·경제·전투·AI·승패도 남아 **미션 완주는 아직 불가능하다.** 영어 원본 글꼴·outpost/LAN 3차·한국어/D2Coding/화면 요구사항/MCP 4차 순서를 유지한다.
