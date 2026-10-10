# cpppj 소리 제어 복원

2026-10-10, 마지막 디컴파일 수행 PC **HJOW-Athlon**. 10.78을 구현 기준으로 삼고 CD·추가 10.37 PE를 비교했다. 세 판본의 여섯 몸체를 읽기 전용 Ghidra로 내보냈다. 이전 단계의 [재생 계층](cpp-sound-play-reconstruction.md)과 [장치 코드](cpp-sound-device-reconstruction.md)에 이어 `SoundPlayer`의 효과음 제어를 복원했다.

## 함수 대응

| C++ 함수 | 10.78 | CD / 추가 10.37 | 동작 |
|---|---|---|---|
| `Recount` | `004a8e60` | `00437c80` | 실제 상태로 재생 수를 다시 세어 저장·반환 |
| `StopByName` | `004a9d20` | `00438bc0` | 이름 조회·유일성 보고 후 원본 항목만 정지 |
| `IsPlayingByName` | `004a9da0` | `00438c30` | 이름 조회 후 기존 재생 여부 경로 호출 |
| `SetNamePlaying` | `004a9de0` | `00438c80` | 0이면 정지, 비영이면 멈춘 항목만 재생 |
| `StopLoops` | `004a9e50` | `00438d20` | 표의 모든 재생 중 반복 버퍼 정지 |
| `SetMasterVolume` | `004a9f10` | `00438e50` | 전체 효과음 음량 저장·적재 버퍼 갱신 또는 예약 |

구현: [Sound.h](../../cpppj/src/client/Sound.h)·[Sound.cpp](../../cpppj/src/client/Sound.cpp). 목록: [soundcontrol-functions.json](../../tools/ghidra/soundcontrol-functions.json). CD `00438e40`은 빈 반환 함수로, 전체 음량 진입은 **`00438e50`**이다.

## 원본 계약

`Recount`는 초기화·옵션을 검사하지 않고 재생 수를 먼저 0으로 만든다. 물리적 이름 표를 끝의 빈 이름까지 순회하므로 복제 항목도 독립적으로 센다. 미적재 0과 소리 없음 `0xffffffff`를 제외하고 각 버퍼의 상태 비트 1을 조회한다. 한 번 소리가 스스로 끝나도 일반 재생 수는 줄지 않으므로 이 호출이 동시 재생 한도를 회복한다. 자동 호출 시점은 후속 클라이언트 연결에서 원본 호출자를 확인해야 한다.

`StopByName`과 `IsPlayingByName`은 소리 옵션이 꺼졌거나 초기화 전이어도 이름을 먼저 등록한다. 이름 정지는 `Root == 자기 자신 && Next == 0`을 검사해 유일하지 않으면 `sound->isUnique()`를 보고한 후 계속 실행한다(패치 줄 `0x3f8`, CD `0x3f3`). 원본 항목만 정지하고 복제 항목은 그대로 둔다.

`SetNamePlaying`은 초기화 전·옵션 꺼짐·null 이름이면 조회 없이 0을 반환한다. 그 외에는 **입력 on을 그대로 반환**하므로 on=8/-1과 재생 실패를 bool 성공으로 바꾸면 안 된다. 이미 재생 중이면 반복 여부·음량을 갱신하지 않는다. 멈춰 있으면 이름 재생을 호출하며 좌우·우선·같은 소리 한도 인자는 모두 0이다. 동시 재생 한도에 막혀도 on은 유지한다.

`StopLoops`는 준비·옵션과 무관하게 표 전체를 검사한다. 첫 상태 조회에서 재생 비트 1, 두 번째 조회에서 반복 비트 4를 확인한 뒤 기존 정지 도우미가 세 번째로 잃음 비트 2를 검사한다. 호출 사이 상태가 바뀌는 경우도 같은 순서를 유지한다. 한 번 소리는 유지하며, 재생 수를 먼저 다시 세지 않으므로 오래된 수가 0일 때 정지하면 원본처럼 DWORD 감소로 -1이 될 수 있다.

`SetMasterVolume`은 원본 재생의 `Gain`과 달리 **범위를 자르지 않는다**. 현재 항목의 `Volume − Attenuation + masterVolume`을 DWORD로 계산해 장치에 넘기고 실패 HRESULT를 무시한다. 초기화됐다면 옵션/재생 여부와 무관하게 멈춘 버퍼·복제 버퍼에도 적용하며, 상태 조회나 로그 출력은 없다. 표의 필드는 보존한다. 복제 항목의 감쇠가 0인 기존 계약도 그대로 따른다. 전역 음량은 루프마다 다시 읽는다.

음량 보류 깊이 `005c7b28` / CD `0051a728`가 0 이외이면 현재 음량·버퍼 대신 예약 음량 `005c7b2c` / CD `0051a72c`만 바꾼다. 음수도 보류다. `SoundState`에 signed DWORD 두 값을 추가했다. **깊이를 증감하는 음소거 API는 음악과 공유하므로 아직 복원하지 않았다.**

CD는 재계산과 전체 음량 순회에서 버퍼를 거르기 전에 모든 이름의 마지막 글자 `'v'`를 검사한다. 보고 줄은 각각 `0x1e7`, `0x442`이며 기존 Lookup의 `0x1b6`과 다르다. 반복 일괄 정지에는 이 검사가 없다. 보고 경계가 반환하면 원본처럼 계속 실행한다.

## 독립 대조와 회귀 검사

[실행기](../../tools/decomp_soundcontrol_oracle.py)는 **각 PE 1,063개, 총 3,189개**의 입력을 두 x87 제어값 `0x027f`·`0x037f`에서 실행했다. 관찰 스크립트 6,378회와 직접 함수 호출 76,458회가 정상 반환했다. 여섯 제어 몸체 및 호출되는 기존 재생·정지·조회·ASCII 비교는 실제 기계어다. 장치 COM·적재·기록·assert 보고·CRT 로캘 선택과 초기 표 확보만 기존 명시 대체를 재사용한다. 게임/창/실제 장치/OS 호출은 없다.

재계산·반복 정지는 상태 비트 0~7과 세 복제 버퍼, 옵션/초기화, 미적재/소리 없음 항목을 교차했다. 이름 제어는 on=0/1/8/-1, 기존 재생/자연 종료/복제/소리 없음, null과 새 이름을 포함한다. 전체 음량은 보류 0/1/2/-1, 감쇠·signed 경계·실패 HRESULT·멈춘 버퍼를 포함한다. `sq:버퍼순번:상태,상태,...`는 연속 조회 사이의 상태 변화를 입력해 정지 순서와 재생 실패 시 반환을 검증한다.

저장 관찰: [soundcontrol-x86.tsv](../../cpppj/tests/fixtures/soundcontrol-x86.tsv). [근거 기록](../../cpppj/recovery-soundcontrol-evidence.json)은 PE/실행기/함수 구간/관찰의 SHA와 전체 입력 순서, 실제 진입, 정상 반환, 보존 레지스터·cdecl 스택·SEH·x87, 허용 쓰기와 OS 0을 감사한다. 보고 후 계속하는 예정된 진단은 사건으로 대조하며 예기치 않은 assert는 0이다. 기존 fixture와 감사 도구는 수정하지 않았다.

[SoundControlTests.cpp](../../cpppj/tests/SoundControlTests.cpp)의 새 검사 6개가 판본별 관찰과 실제 계층의 자연 종료→한도 회복, 옵션 꺼짐→반복만 정지, 복제 유지, on 반환, 보류/범위 미보정/재진입 음량 갱신을 확인한다. 전체 표 바이트 보존도 검사한다. 일반 CTest는 원본 PE/Python/소리 장치를 실행하지 않는다.

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name soundcontrol
python -X utf8 tools/decomp_soundcontrol_oracle.py
python -X utf8 tools/decomp_soundcontrol_oracle.py --verify
cmake --build cpppj/build --config Release --parallel 4
cpppj/build/bin/Release/netstorm_tests.exe --filter SoundControl_
ctest --test-dir cpppj/build -C Release --output-on-failure
```

최종 회귀 결과와 기록 위치는 [LEFT_JOBS.md](../../LEFT_JOBS.md)의 최신 인계를 따른다. 이번 단계 누적 독립 관찰은 **425,353개**다.

## 남은 연결

음악의 초기화/종료·재생/정지·음량·예약과 공유 음소거 깊이 증감(`004aa900`·`004aa970`)은 후속이다. 숨긴 협조 창을 쓰는 기존 `--inspect-sound-device originals`는 아직 실행하지 않았다. 실제 장치 실행/청취·장치 소실·음질 하향·반복 실행과 GUI GameWorld의 HWND/장치 수명·옵션·프레임/벽시계·시야/카메라·Recount 호출 시점·나머지 소리 호출자를 연결해야 한다. 손상된 표/사슬과 원본이 멈추는 빈 이름/null 정지 계약 위반은 native 관찰에서 제외했다.

메뉴/브리핑/초기 선택·사제 이동을 넘어선 raw GUI 건설·경제·전투·AI·승패는 여전히 미완료이며 미션 완주는 아직 불가능하다. 영어 원본 글꼴 정책, outpost/LAN 3차와 한국어/D2Coding·화면 요구사항/MCP 4차 순서를 유지한다.
