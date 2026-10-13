# dotnetpj 원본 숫자 시계·중첩 정지 복원

작업 PC와 마지막 디컴파일 수행 PC: **HJOW-Athlon, 2026-10-12**. `clockkernel`을 읽기 전용 Ghidra로 **5/6/6개** 내보냈다. 앞 단계 cpppj GumpLifetime 변경을 유지하고 dotnetpj 인계를 이어 진행했다. AGENTS.md와 두 LEFT_JOBS를 확인했다. 원본/복사본 게임 실행·보호 파일 변경·커밋/푸시는 없다.

## 복원한 계산과 연결

원본 벽시계는 단순 경과 밀리초 나눗셈과 다르다. 다음 DWORD 계산을 끝낸 뒤 몫과 나머지를 따로 double로 변환한다.

```text
wallDWORD = (timeGetTime - origin + epochSeconds * 1000) mod 2^32
gameDWORD = (timeGetTime - origin - gameAdjustment + epochSeconds * 1000) mod 2^32
seconds = double(DWORD / 1000) + double(DWORD % 1000) * 0.001
runningGame = seconds(gameDWORD) - pausedTotal
```

`OriginalGameClock`은 이 숫자 계산과 중첩 Pause/Resume을 구현한다. 첫 Pause만 현재 게임 시각을 저장한다. 중첩 정지 중에는 저장 비트를 반환하며 마지막 Resume에서만 `(gameNow - heldGame) + pausedTotal` 순서로 누계를 갱신한다. 짝 없는 Resume은 무시한다. 유효한 비음수 깊이를 전제로 하고 C#에서는 깊이 넘침을 거부한다.

`FrameLoopClock`의 그리기 제한은 원본 double 벽시각 두 개의 차이와 `1 / maxFPS`를 비교한다. 예를 들어 `1.001 - 1.0`은 double에서 0.001보다 작으므로 1000FPS 제한이 다음 밀리초 눈금까지 기다리는 차이를 보존한다. 대기 밀리초는 Sleep/Yield의 힌트이며 실제 시각을 다시 읽고 판정한다. 정지·무제한·늦은 프레임 처리도 유지한다.

실제 클론은 현재 WINMM 원점과 `UnixSeconds - 0x32c85780` epoch를 주입한다. 원본의 OS 초기화 전체를 이식한 것은 아니다. 원본의 초 경계 바쁜 대기/프로세스 우선순위와 FPS 표본/Capture는 후속이다. 원본 DWORD 합이 감겨 벽시각 차이가 음수가 되거나 그리기 간격이 한 DWORD 주기 이상이면 호스트의 펼친 단조 간격을 사용한다. 이 경계 처리는 장시간 대기를 막는 호스트 처리이며 원본 숫자 함수의 감김 결과는 그대로 보존한다.

## 원본 몸체와 독립 근거

| 계산 | 10.78 | CD/10.37 |
|---|---|---|
| 벽시계 | `00460d70` | `004011b0` |
| 게임 시계 | `00460cd0` | `00401130` |
| Pause | `00460df0` | `00401230` |
| Resume | `00460e30` | `00401270` |
| signed 정지 조회 | `00460de0` | 이번 목록에 없음 |
| DWORD 분해/CRT 곱셈 | 본체에 포함 | `004010c0` / `004f0700` |

`tools/decomp_clockkernel_oracle.py`는 세 PE를 독립 Unicorn 메모리에 읽고 내보낸 실제 몸체만 실행한다. **timeGetTime만** 완성 입력 DWORD로 응답한다. CD의 DWORD 분해와 CRT 곱셈도 실제 기계어로 실행하며 이번 epoch 입력은 CRT의 상위 인자가 0인 경로다. Python/C++/C#으로 시간 기대값을 계산하지 않는다.

판본마다 **72개 연속 입력**을 실행했다. 최초/중첩/반복 정지, 마지막/여분 재개, 정지 중 벽/게임 조회, 작은 밀리초 눈금, 원점·epoch 곱셈·DWORD 합 감김, 게임 전용 보정을 포함한다. 각 호출 뒤 반환 double 비트·깊이·저장 시각/누계 비트·timeGetTime 응답 횟수·실제 진입·정상 반환을 저장한다. 호출 규약·보존 레지스터·x87 제어/스택·허용 실행/쓰기·읽기 전용 입력·결과 보호 바이트도 확인한다.

기본 **0x027f(53비트)** 결과 **4,464개**를 C#에서 모두 재생해 일치했다(10.78 1,584개, CD/10.37 각 1,440개). dotnetpj 원본 대조 누적은 **53,471개**다. 대안 **0x037f(64비트)**도 별도 보존하여 두 모드 최상위 정상 반환 **8,928회**를 확인했다. 모드 간 반환/상태 차이 **388개**를 제외하거나 같은 결과라고 처리하지 않았다. C#의 비트 일치 판정은 기본 53비트 모드에 한정한다.

fixture는 `dotnetpj/tests/Netstorm.Core.Tests/Fixtures/clockkernel-x86.tsv`, SHA/범위 기록은 `dotnetpj/clockkernel.json`이다. `.gitattributes`에 LF를 지정해 다른 Windows 체크아웃에서도 SHA를 유지한다. 공통 기존 근거 도구를 수정하지 않았다.

## 검증

Release 빌드 경고·오류 **0**, 전체 단위 **990개**(Assets **331** + Core **659**), 실패·건너뜀 **0**. 새 검사 **7개**는 세 판본의 모든 53비트 반환, 중첩 정지/짝 없는 재개, 보정/감김, 그리기 제한의 double 경계 및 호스트 감김 대기를 검사한다. 새 근거 SHA/행 수/완성 입력/진입·반환·경계 감사도 통과했다.

실제 클론 GUI **12회/188개 게임 내 단언**이 모두 통과했다. 루프 네 조합 **68개**와 외부 **32개**(영어/한국어 4:3·영어 16:9·한국어 16:10, 75/30/무제한), 정지 중 선택/새 이동 명령 네 조합 **76개**, 공통 UI/전체화면 저장·재실행/창 복귀 재실행 네 실행 **44개**다. 설정 저장 검사도 통과했으며 stderr는 모두 비었다.

삽입한 700ms 지연의 바로 다음 실행 간격 **715~718ms**, 정지 간격 **705~707ms**를 모두 전달했다. 정지 시각·틱·스캔·표시 좌표 보존과 재개 후 목표 도착을 확인했다. 이 수치는 해당 PC의 클론 실행 결과이며 원본 전체 루프 실측 대조가 아니다. 결과는 `extracted/screens/dotnet-clock-20261012/{loop,pause,ui}/`의 명령·PNG·로그·JSON, `extracted/dotnet-clock-20261012-{build,tests,generation,oracle,loop,pause,ui}.log`와 `extracted/dotnet-clock-20261012-summary.json`에 남겼다. 도구의 설명 문자열을 정리한 뒤 전체 원본 관찰을 다시 생성했으며 fixture SHA는 첫 생성과 같았다.

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name clockkernel
python -X utf8 tools/decomp_clockkernel_oracle.py
python -X utf8 tools/decomp_clockkernel_oracle.py --verify
dotnet build dotnetpj/Netstorm.sln -c Release --no-restore
dotnet test dotnetpj/Netstorm.sln -c Release --no-build
python tools/clone_loop_smoke.py --output extracted/screens/dotnet-clock-20261012/loop
python tools/clone_frame_smoke.py --output extracted/screens/dotnet-clock-20261012/pause
powershell -ExecutionPolicy Bypass -File tools/clone_ui_smoke.ps1 -OutputDirectory extracted/screens/dotnet-clock-20261012/ui
```

## 이어갈 범위

**9-1 A 전체는 미완료다.** 게임 시계 숫자 함수와 중첩 정지는 독립 Core API 복원이며 현재 연결은 벽시계의 그리기 제한까지다. `BattleSession`의 절대 게임 시각·튜토리얼/지식창/Shift-F9의 중첩 정지 소유권은 아직 이 API를 소비하지 않는다. 시간 기반 게임 타이머로 옮길 때 함께 연결한다.

이동·전투·건설·생산·수확·의식·AI·튜토리얼·애니메이션의 24Hz 틱, 따라잡기 8틱 상한, `FrameSeconds`/`Seconds`의 과도 분리, 절대 시각 경로 이동과 정지 중 새 경로의 보간은 남았다. FPS 표본/Capture와 초 경계 초기화, D의 실제 대기 방식·비활성 전체화면·sleepPerLoop/우선순위도 후속이다. 64비트 x87의 전반 비트 일치, 원본 전체 화면/게임 루프 실측 일치는 이번 완료 범위가 아니다.
