# 실패/중단 화면의 원본 재시작·확인·종료 예약

단위249의 시작 커밋은 `29d4682f`다. 현재 PC/마지막 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-20**로 일치한다. AGENTS와 양 인수인계를 다시 읽었고 새 디컴파일·원본/CD·AGENTS 변경은 없다. 직전 공통 프레임 연결을 바탕으로 사용자의 실패 후 재도전과 종료 확인을 이어 연결한다. 전체 지정11미션 목표는 진행 중이다.

## 원본과 빠져 있던 경로

캠페인 실패 본문의 Continue는 `Tell,TryAgain`을 연다. 보호 `tell.english`의 TryAgain/ABORT에는 `Replay Mission,MissionRestart,0`이 있지만 Client의 Supported/Execute에는 MissionRestart가 없어 버튼이 비활성화되어 있었다. MissionAbort도 인자와 무관하게 바로 MainMenu를 실행해 원본 확인·큐 처리와 달랐다.

원본 실제 이름 표 `004d52c0`에서 MissionRestart는 `0200008d`, MissionAbort는 `0200008c`, LeaveBattle은 `020000c5`다. 원본 Guide 사건 `00463e40`은 명령 상위 플래그를 제거하고 Abort이면 **세 번째 DWORD(param2/y)**를 검사한다. y0은 `Tell("ABORT")`, 비제로는 `00452fe0(LeaveBattle,2,2)`이며 항상1을 반환한다. x는 확인 분기에 사용하지 않는다. 현재 미션 포인터가 없어도 Abort 분기는 동일하다. 상위 release 표시도 이 Guide 분기에서 제거된다. 반면 실제 LeaveBattle은 하위 일반 입력의 release 거부 뒤에 처리된다.

`MissionGuideInput`에 세 이름 조회·같은 QueueScriptCommand 예약·Abort 분배를 추가했다. 페이지 버튼의 명령/인자도 기존 원본 토큰 parser,80칸 문자열 고리와40칸 입력 큐를 사용한다. 숫자로 시작하는 인자와 부호/빈 문자열의 차이를 별도 숫자 parser로 바꾸지 않는다. Supported와 inline 명령 조회가 같은 코드 조회를 쓰므로 원문 Replay Mission 버튼이 활성화된다.

Execute는 세 명령을 큐에 넣고, 실제 Event에서 Abort 확인/종료 예약, 기존 미션 재시작 수신,기존 단일 플레이 MainMenu/메뉴 음악 종료를 수행한다. 큐 포화 때 원본처럼 새 명령이 버려질 수 있으며 이때 즉시 월드를 해제하지 않는다. 기존 State/페이지 전환 타이밍과 단일 플레이 teardown 어댑터는 유지한다. 원본 버튼/Gump 전체 프레임과 전투 종료 네트워크·State의 모든 내부 명령을 새로 복원한 것은 아니다.

## 제품 작성 전 원본 관측

`decomp_mission_abort.py`가 실제 PE 메모리 사본에서 Guide Abort/미처리 사건 **960입력**과 세 제어 이름/인자의 실제 문자열 처리 **42입력**, 총 **1,002 정상 반환·580,682 실제 명령·명령 큐426회**를 보존했다. 사건은 현재 미션 유무,다른 x/y,0/양수/높은 비트·음수 DWORD,modifier/release,활성 큐0/1/39칸과 하위 입력 반환을 교차한다. 문자열은 대소문자,0/1/2,32비트 경계,부호와 빈 인자를 포함한다.

실제 `00463e40/00452fe0/00452f90`과 `004c9090/004d52c0/토큰/고리/CRT`를 실행했다. Tell `004cf960`,하위 가상 입력,TLS/할당/로그는 명시 외부 경계다. 정상 반환 주소·스택 정리·비영 네 보존 레지스터·x87 제어어를 확인하고 쓰기를 명시 입력/할당·큐/고리·스택 범위로 제한했다. 실제 UI/월드 재시작·종료/OS/게임 실행은 없다.

처음 기존 스크립트 관측기의 과거 부분 export가 이 PC에 없어 준비 단계에서 실패했다. 옛 PC의 자료를 복사하거나 새 디컴파일을 실행하지 않고, 현재 HJOW-Athlon 전체 정밀 함수 표의 같은 함수 범위를 사용했다. 이 준비 실패에는 제품/fixture 생성이 없었다. 최초 성공 원문/도구/보고서는 제품 작성 전에 `extracted/mission-abort-20261021-first-capture/`에 보존했고 fixture SHA-256은 `dd608d5f5c7bad4bcb70422c30719d2ed34e55b98473d77554dc6866e6a39889`다. 기존 스크립트262/First278/공통108·328 기대값은 덮어쓰지 않는다.

## 검사와 남는 범위

`MissionControl_Native1002`는 Abort의 반환/전체 활성 큐/외부 호출 순서960행과 세 명령 코드/인자·문자열 수명42행을 독립 원문과 비교한다. `MissionControl_AuthoredReplayAndAbortConfirmation`은 실제 보호 TryAgain/ABORT 본문을 현재 HTML 메타 parser로 읽어 Replay Mission 버튼→같은 큐의 재시작0/0,확인0의 창 선택과 비제로의 LeaveBattle2/2,지원하지 않는 이름의 무변경을 확인한다. 글자 표시/전체 Gump 클릭/월드 수명 검사는 아니다. **C++검사는 작성·컴파일만 하며 사용자 지시대로 실행하지 않는다.**

빌드·현재 소스/보호 본문·최초 원문·현재 핀 역변환은 `cpppj/mission-abort-session.json`과 `cpp_mission_abort_audit.py --verify`에 기록한다. 기존 보고서의 현재 소스 핀만 후속 이력으로 바꾸고 전체 JSON을 시작 커밋으로 역변환해 과거 files/원본/기대값/실행·바이너리/기존 이력을 보존한다.

최종 전체 Release는 **경고0·오류0·종료0**이며 라이브러리·게임/검사 EXE를 생성했다. 원문 로그는 `extracted/cpp-mission-abort-20261021-build.log`다. C++검사·게임 실행 통과로 합산하지 않는다.

현재 핀은 **100보고서·125항목**이며 전체 JSON 역변환이 시작 커밋과 일치한다. 이번 원본/연결 감사와 직전 공통 미션 감사,변경 검사가 모두 종료0이다. 과거 원문/기대값/실행 결과를 새 제품 성공 결과로 바꾸지 않았다.

Guide 전체0..6 수명/전환·Begin/Fork/Finish,다른 키와 Review Knowledge/ShowTechnology,중첩 Gump,전투 종료 State/네트워크 세부는 계속 필요하다. 실제 실패→재시작/계속/중단·11미션 자연 진행/완주·전략AI·지형 Devastation·포획/Drop·제단/희생·Dude 자연 대피와 원본 외관/음향 동일성은 통합 실제 플레이에서 확인한다. 기존 C++검사/CTest/전체 회귀와 게임 실행 이월 지시는 유지한다.
