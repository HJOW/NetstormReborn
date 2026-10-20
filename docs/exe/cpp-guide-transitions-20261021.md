# 실제 Guide의 미션 교체와 후속 명령

단위251의 시작 커밋은 `655794e9`다. 현재 PC와 마지막 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-20**로 일치한다. AGENTS와 양 인수인계를 다시 읽었고 새 디컴파일·원본/CD·AGENTS 변경은 없다. 지정11미션의 자연 플레이와 원본 동일성 목표는 진행 중이다.

## 교체 순서와 실제 Client 연결

원본 이름 표의 `MissionBegin=0200008b`, `MissionFork=0200008e`, `MissionEnd=0200008f`를 기존 Restart/Abort/LeaveBattle과 같은 토큰 파서·80칸 문자열 고리·40칸 입력 큐에 연결했다. `MissionFinish`라는 이름은 원본 표에 등록되어 있지 않다. 임의의 후속 명령을 만들지 않는다.

`GuideMissionTransitions`는 실제 `00463b70/00463da0/00463cf0/00463e40`의 서로 다른 순서를 유지한다.

- Begin은 현재가 있으면 가상 종료→해제를 한 번 호출하고 **그 뒤** 새 이름을 복사해 공장으로 보낸다. 첫 해제 중 생긴 현재를 다시 검사하지 않는다.
- Fork는 signed `fork%d`로 현재 설정을 조회하고 **첫 해제 전에 결과 문자열을 소유 복사**한다. 가상 종료→해제 뒤 Begin 경로로 들어가므로 해제 중 새 현재가 생겼으면 다시 가상 종료→해제한다.
- Restart는 이름과 +64/+68의 비제로 판정을 먼저 캡처하고 **첫 해제 전에 가상 종료를 호출하지 않는다**. Begin 경로의 공장 뒤 실제 `004639d0` 모드 setter를 무조건 실행하고, 현재가 있을 때 데모 값을 복원한다.
- End는 가상 종료→해제를 한 번 호출한다. 해제 중 생긴 현재는 다시 없애지 않는다.

Tutorial 가상+1c는 원본의 빈 `004437f0`다. 실제 Client는 같은 Runtime의 단계6을 이 가상 종료 효과에 공급한다. 월드/Script 해제는 현재 `ClearWorld` 어댑터이며 원본 전체 가상 소멸을 복원한 것은 아니다. Begin/Fork/Restart 도중 MainMenu와 메뉴 음악을 끼워 넣지 않는다. End 뒤 현재가 없을 때 메뉴로 돌아가는 기존 단일 플레이 어댑터는 유지한다. 다른 미션 형식의 종료는 기존 어댑터 범위다.

기존 BeginMission에서 빈 미션 생성 부분을 `CreateMission`으로 분리했다. 지정11개 Tutorial의 월드 설치는 계속 Guide 단계1,Start는 단계3에서 수행한다. StartDemo의 직접 데모 값은 생성 전에 전달하고,Restart는 새 공장 뒤 필드를 복원한다. 창 제목도 단계1에서 실제 Header를 읽고 갱신한다.

사건 분배는 상위 플래그를 `03ffffff`로 제거하고 x가 아닌 **세 번째 DWORD y**를 사용한다. 원본 Begin의 signed `param2>100` 관문을 보존하기 위해 클론의 불투명 문자열 토큰은101부터 `INT_MAX`까지의 양수를 쓴다. 이는 원본 메모리 주소 복원이 아니다. 80칸 덮어쓰기·이전 수명 거부와 입력 큐의 정수/문자열 구분은 유지한다. Client는 고리 문자열을 소유 복사한 다음 이전 미션을 해제한다. 원본 진단 callback이 반환하면 계속하는 분배도 분리했으며,실제 Client의 잘못된 토큰은 예외로 중단한다.

## 제품 변경 전 독립 원문

`decomp_guide_transitions.py`는 실제 사건 분배·Begin·Fork·Restart·모드 setter와 빈 Tutorial 가상 종료를 **교체256입력+스크립트30입력=286정상 반환·393149실제 명령**으로 관측했다. 원본 이름 조회/토큰/문자열 고리/입력 큐/CRT는 실제 PE 몸체다. 해제/공장/설정/`fork%d` 서식/진단·TLS/할당은 명시 경계이며 서식 경계100회를 별도로 센다.

해제는 빌린 설정 버퍼를 무효화하고 선택 입력에서 새 현재를 설치한다. 기대값에는 최종 Guide 이름·현재 식별자·새 모드/데모·호출 순서가 들어 있다. 입력은 signed 분기와 상위 사건 플래그,x/y 차이,현재 유무·재진입을 구별한다. 잘못된 Begin 주소0/1/100/101/음수는 격리 주소 공간에 매핑한 문자열과 반환하는 진단 경계로만 관측했다. 실제 OS 포인터 유효성이나 원본 게임 실행을 의미하지 않는다.

첫 성공 관측의 도구/fixture/JSON은 제품 수정 전에 `extracted/guide-transitions-20261021-first-capture/`에 보존했다. fixture SHA-256은 `f3f300ddfecc6b6c8b1eb769d2706a79186bade22e29206354ad66333ba20c36`다. 기존 네이티브 기대값과 저장 플레이·검사 결과는 덮어쓰지 않는다.

## 검증 범위와 다음 작업

`GuideTransitions_Native286`은 독립 원문의 순서/최종 필드와 실제 제품 함수를 비교한다. `GuideTransitions_AuthoredNextMissionQueueAndOwnedString`은 보호 `TheWarBegins` 성공 본문을 HTML 파서→완료 Config 예약→실제 Next Mission 버튼/문자열 큐→기존 Script 해제→`MasterOfWhirligigs` 생성으로 연결한다. 승리 판정을 주입하거나 실제 월드 승리를 만들지 않는다. 문자열80칸의 이전 수명 거부도 함께 작성했다. **사용자 지시대로 C++검사는 컴파일만 하고 실행하지 않는다.**

빌드/현재 소스/보호 본문/최초 관측과 과거 핀 보존은 `cpppj/guide-transitions-session.json`, `cpp_guide_transitions_audit.py --verify`에 기록한다. 기존 공통 미션/Abort/수명 감사는 새 교체 함수와 분리된 생성 함수를 실제로 검사하도록 갱신한다. 현재 핀만 후속 이력으로 바꾸고 전체 JSON을 시작 커밋으로 역변환하며 과거 실행 결과·독립 기대값·files/evidence는 보존한다.

최종 전체 Release는 **경고0·오류0·종료0**이며 로그는 `extracted/cpp-guide-transitions-20261021-build2.log`다. 현재 핀 **104보고서·144항목**의 전체 JSON 역변환과 최초 원본/교체·공통 미션/Abort/수명 정적 감사는 종료0이다. 시작 CMake의 혼합 줄바꿈은 실제 Git 바이트/LF/CRLF가 같은 코드인 경우만 현재 핀으로 인정했다. 기존 RawTutorialLater의 역사 CRLF/LF 핀 차이는 이번에 바꾸거나 새 통과로 기록하지 않는다.

전체 Init의 이름 정규화/+b0/전역·비동기 State/원본 공장/가상 소멸,ShowTechnology/Review Knowledge·중첩 창/다른 키는 계속 필요하다. 전체11미션의 자연 완주·다음 미션/F8/재시작/중단·전략AI·지형 효과/포획/희생/Dude 자연 대피·원본 외관과 음향 동일성은 후속 실제 통합 플레이에서 검증해야 한다. 이번 연결과 독립 PE 관측을 그 검증의 성공으로 계산하지 않는다.
