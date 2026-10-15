# 실제 로그인 타이머의 미션 선택과 Begin/Abort

현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon, 2026-10-15**로 같다. AGENTS와 양 LEFT의 최신 기록을 읽고 같은 PC의 전체 정밀 소스·보호 PE·capstone을 재확인했다. 새 디컴파일·원본/클론 게임·GUI·OS 진입은 없으며 보호 원본·AGENTS·dotnetpj를 변경하지 않았다.

## 실제 상태와 공유 Player 집계

`o/RawMissionLoginTimer.{h,cpp}`는 `004b67c0`의 예약/현재 타이머/미션 종료 선택, 만료 `004180a0`, Begin `004b61f0`, Abort `004b6780`을 복원한다. 같은 `9×0xb4` Player 원문을 읽는 공개 `MissionLoginPlayerCounts`를 상위 `RawMissionLogin`과 공유한다. Player0과 subscriber0의 stale flags를 현재 접속자로 추정하지 않는다.

| 함수 / 공개 메서드 | owner1..8의 실제 조건 |
| --- | --- |
| `00490ab0 / Subscribers` | subscriber!=0 |
| `00490a00 / Participating` | subscriber!=0 && flags2 |
| `00490bc0 / Disconnected` | subscriber!=0 && flags2 && flags100 |
| `00490b20 / Ready` | subscriber!=0 && flags2 && flags1 && flags40 |
| `00490970 / LoggedIn` | subscriber!=0 && flags2 && flags1 |

Player subscriber는 `+2c`, flags는 `+50`이다. 단일 집계 객체가 현재 span을 직접 읽으며 목록/참여/Ready 사본을 만들지 않는다. 특히 `Participating`은 상위 Options 자기 수신의 `<2` 판정에 필요하고 Ready와 다르다.

`MissionLoginTimerState`는 기존 실제 `end(005c85f0)`·`timeout(005c8938)`·`begin(005c7a74)`·`scriptBreak(00542544)`·`requestPhase(005c85ac)`·`clock(00594fb4)`의 **참조 묶음**이다. 실제 Session의 다른 소비자와 같은 저장소를 연결해야 한다. savedGame/server/mission/missionPost/initialGeysers/checkingAssertions는 각각 현재 bool getter로 읽어 Session bool을 DWORD 참조로 재해석하지 않는다.

## 실제 예약·선택 순서

`Run(true)`는 원본 시작 로그 → 현재 `loginTimeout` 정수 조회 → timeout 저장 → 실제 wall `00460d70` → 선택된 int32 timeout과 wall의 합을 end에 저장한다. wall 효과가 다른 상태를 바꾸어도 원본 stack local처럼 이미 선택한 timeout을 사용한다. 게임 시각이나 프레임 캐시로 바꾸지 않는다.

`Run(false)`에서 end가0이면 어떤 모드/시간/연결/효과 경계도 읽지 않는다. 활성 예약만 다음 순서를 실행한다.

1. 현재 savedGame이0일 때만 실제 CRT `_clock`을 읽어 clock에 쓴다.
2. server가false이면 실제 연결9의 존재를 조회한다. 없으면 Abort하고 종료한다. 있으면 server를 **다시 읽어** 서버 사슬/원격 `004b6180(9,0,0)`을 선택한다.
3. 서버의 begin이0이면 `Disconnected → Subscribers → Ready`를 읽는다. `Subscribers-Disconnected != Ready`에서만 실제 만료를 확인한다. 만료는 wall 경계 실행 **뒤의 현재 end**와 `end<=wall`을 비교한다.
4. 미ready·미만료이면 Begin만 생략하고 아래 완료 조건으로 간다. 만료되었고 LoggedIn<2이며 현재 savedGame0이면 Abort한다. 나머지 서버 경로는 실제 Begin을 실행한다. 이미 begin이 켜져 있어도 원문 위치의 Begin 송신을 생략하지 않는다.
5. 현재 missionPost 또는 begin이 비제로이면 종료 정리를 선택한다. **missionPost=true는 미ready·미만료에서도 정리로 간다. 추가 Ready 장벽을 넣지 않는다.**

종료 정리의 savedGame0&&!mission 선택은 큰 일반 전투 초기화 `004b4a20` 이하가 미복원이므로 해당 위치에서 명시 거부한다. 미션 분기에서는 initialGeysers가 켜져도 현재 mission을 다시 읽어 일반 랜덤생성을 선택하지 않는다. 그 사이 모드가 바뀌어 일반 랜덤생성에 도달하면 이 경계도 명시 거부한다.

지원 종료 사슬은 실제 `Seting Priest State\n` 로그 → **priestRefresh1..8** → `Done\n` → begin0/scriptBreak0 → 실제 `004b54e0` → end0 → 실제 진행 창 해제 `004c8d90` → `Moveing Camera\n` → **현재 !missionPost일 때만** resetCamera → `Done\n`이다. 로그의 원본 철자도 보존한다. `004b54e0`의 미션 자연 반환 여부는 실제 해당 몸체가 결정해야 하며 여기서 빈 성공으로 대신하지 않는다.

완료 조건이 거짓인 일반 진행 UI 꼬리 `004b68d6..004b6ba1`은 필수 선택 외부 `progress` 경계다. 아직 그 이름/IP/번역/남은시간/Start·Abort 버튼 계산을 복원했다고 주장하지 않는다. 소비자가 이 경로를 선택하고 실제 UI를 공급하지 않으면 명시 예외다.

## 실제 Begin 패킷과 Abort 경계

보호 PE `0052ee72`의 descriptor는 `6e 01 08 00`이다. 실제 `0040b010`은 descriptor의 type을 packet BYTE1에, version&7f를 BYTE0에 쓰고 `004df230`으로 길이를 쓴다. 따라서 실제 전송8바이트는 **`01 6e 08 00 00 00 00 80`**다. descriptor 바이트를 그대로 복사하지 않는다.

Begin은 checkingAssertions&&!server의 실제 진단 후 계속하고, **end0 → 실제8BYTE broadcast → begin1** 순서다. callback 뒤의 begin1을 송신 전에 쓰지 않는다. 진단 인자도 실제 `server` 문자열 주소 `00507248`, `.\\State.cpp` 주소 `005134a4`, line `1153`으로 native에서 확인한다.

Abort는 **begin0/end0/scriptBreak1/requestPhase4 → 실제004b6350 → 진행 창 해제**다. 큰 하위004b6350은 명시 필수 경계다. 이 하위는 현재 모드에 따라 phase를 다시 바꿀 수 있으므로 복귀 후4로 덮어쓰지 않는다. 두 함수의 selected 소비자 누락은 앞서 수행한 원본 쓰기를 보존하고 해당 위치에서 오류를 반환한다.

## 독립 보호 PE 관찰과 정확한 범위

`tools/decomp_mission_login_timer.py`는 기존 격리 PE 메모리 로더만 재사용하고 같은 PC 전체 정밀 함수표의 **11개 몸체**만 허용한다. 타이머/5집계/만료/Begin/Abort에 더해 실제 헤더 writer `0040b010`·`004df230`도 실행한다. 기록된 고유 입력은 **1,984개**다.

| 종류 | 보존 입력 |
| --- | ---: |
| Run | 1,466 |
| Start | 18 |
| 직접 Expired | 19 |
| 직접 Begin | 72 |
| 직접 Abort | 15 |
| 직접5집계 | 394 |

실제 누적 진입은 Run/Start `004b67c0`1,484회, Begin658회, Abort288회, 헤더 writer 두 함수 각각658회, Expired312회다. 집계는 Subscribers811/Participating394/Disconnected811/Ready811/LoggedIn647회다. 반복 실행한 동일 입력은 새 고유입력으로 다시 합산하지 않는다.

각 저장 표본은 정상 반환 주소·cdecl 스택 `ESP=호출전+4`와 EBX/ESI/EDI/EBP를 확인한다. 실제 명령 쓰기 guard는 한정 스택과 end/timeout/begin/scriptBreak/requestPhase/clock 전역만 허용하며 Player 기록 쓰기·미검토 함수·OS 진입을 거부한다. 원문 실제 모드 DWORD 읽기는 메모리 read hook으로 관찰하고 C++의 현재 bool getter trace와 대조한다.

시간/설정/clock/연결 존재·원격 요청/송신/사제/재접속 저장/진행 창/카메라/로그/진단/004b6350은 **명시 외부 대체**다. 보고서에 각 주소의 횟수를 별도 보존한다. callback 진입 전에 실제 end/begin/scriptBreak/phase/clock을 함께 기록해 원본 쓰기 순서도 대조한다. 일반 progress 꼬리는 해당 내부 위치에서 명시 경계를 관찰한 뒤 실제 원본 epilogue로 이동한다. 생략된 UI 계산의 동일성·큰 일반 초기화·실제 전송/게임 완주를 관찰한 근거로 확대하지 않는다. 현재 native 입력에서 큰 일반 초기화 진입은0이며 제품의 해당 선택 거부는 별도 C++ 검사로 준비했다.

fixture `cpppj/tests/fixtures/mission-login-timer-x86.tsv`에는 double의 전체8BYTE, 모든 타이머/모드 상태, 반환값과 현재 조회/외부 경계 trace를 보존한다. 직접 집계는 owner1..8×subscriber0/양수/높은비트×16 flags와 Player0의 stale 비제로 값을 포함한다. 입력에는0/음수/최대·최소int32 timeout, float로 소실될 수 있는16777217.25 wall, <=경계·NaN·무한대, 저장게임, 원격 서버 재조회, 미ready/미만료, 효과 뒤 현재 상태 변경, 실제 packet8BYTE와 Begin/Abort 쓰기 순서가 포함된다.

## 검사와 다음 연결

`MissionLoginTimerTests.cpp`의 **5개 검사**는 native1,984개 재생, 미ready·미만료 missionPost 종료, inactive0의 읽기0, Begin/Abort selected 소비자 누락의 쓰기 순서, 일반 초기화 명시 거부/원문 저장소·현재 getter 계약을 제공한다. 현재 `--verify`의 SHA/고유입력 감사와 공백 검사를 통과했다.

소스 고정 뒤 CMake 등록·Release 경고0·오류0, 이5개를 포함한 집중60개 실패0·전체 CTest1/1·내부1,283개 실패0·216.33초를 확인했다. `extracted/goal-mission-entry-final-{build,focused}.log`, `goal-mission-entry-{ctest,tests}.log`를 보존했다. 실제 Session의 같은 상태 참조, 실제 savedGame/server 등의 현재 getter, 연결9·사제 상태·004b54e0·004b6350·진행 창/카메라를 원래 수명에 연결한 뒤 상위 로그인→Guide→Stage3 순서를 설치해야 한다. 이번 모듈만으로 자동 Stage3/AI/자연 미션 완주가 활성화되지는 않는다.
