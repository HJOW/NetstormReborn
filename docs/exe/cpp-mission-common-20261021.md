# 캠페인·데모의 공통 프레임과 현재 미션 재시작 연결

단위248의 시작 커밋은 `62213f9f`다. 현재 PC/마지막 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-20**로 일치한다. AGENTS와 양 인수인계를 다시 읽었고 새 디컴파일·원본/CD·AGENTS 변경은 없다. 지정11미션 전체 목표는 계속 진행 중이다.

## 빠져 있던 실제 실행 경로

보호 영어 스크립트에서 Early1~6, 캠페인1-1 `TheWarBegins`·1-2 `MasterOfWhirligigs`, 세 데모 `DemoStormRages`·`GuardMyBack`·`DissolvedAlliance`는 모두 **missionType=Tutorial**이다. Early만 tutorialNumber1~6을 가지며 나머지는0이다. 원본 `004c34c0`은 이 형식의 모든 미션에서 승패/사제 사건,10호출 안내,시간 안내,자기 정지의7호출 재개,데모 재시작 기한을 처리하고, 마지막에만 비데모 tutorialNumber1~6의 전용 판정으로 분기한다.

기존 Client는 생성 조건이 tutorialNumber1~6/비데모라 캠페인·데모의 공통 프레임과 입력,승패 확인이 연결되지 않았다. `RawCampaignTutorialOneBindings`의 공개 이름은 기존 소비자를 위해 유지하면서 모든 Tutorial 형식을 허용하도록 했다. 전용 Early 판정기는 비데모1~6에서만 생성한다. Client는 이제 실제 missionType으로 연결하고 지정11미션 모두 같은 공통 프레임·현재 상태·Tell·팝업·F8·정지 깊이를 사용한다. 처음 A/Demo 안내를 직접 표시하던 캠페인/데모 우회 대신 같은10호출 관문으로 보낸다. 다른 `Battle` 형식의 수명 복원은 이번 범위 밖이다.

`DemoStormRages`의 Header에는 demo=1이 있다. `GuardMyBack`과 `DissolvedAlliance`는 원본 `tell.english` 메뉴의 StartDemo 지정으로1이 된다. 원본 Init `004c3290`은 demo 문자열이 비어 있지 않을 때만 기존 직접 지정값을 atol DWORD로 덮어쓴다. 빈 문자열과 공백/잘못된 숫자의0쓰기를 구별한 `ApplyMissionDemo`를 현재 Runtime 생성에 연결했다. `DissolvedAlliance`는 원문에 **Demo 섹션이 없으므로** 공통 TellStage는0을 반환하고 자기 정지를 추가하지 않는다. 존재하지 않는 안내를 생성하지 않는다.

원본 공통 프레임의 데모 기한은 `00452f90` 일반 입력 큐에 **00542928=0200008d/0/0**을 넣고 기한을0으로 만든다. Client도 같은 큐에 예약하고, 프레임 콜백이 반환한 뒤 입력 분배에서 현재 미션을 다시 생성한다. 기존 Script/월드가 살아 있을 때 이름을 소유 복사하고+64/+68의 비제로 값을 각각0/1로 캡처한다. `ClearWorld`의 자기 정지 반납과 기존 Script/월드/로그인/Start 경로를 사용한다. 새 Runtime의 mode64는 원본 기본0과 재시작 보존용이며 도전 모드의 전체 입력 구현을 의미하지 않는다.

## 독립 원본 근거와 검사의 범위

기존 공통 프레임328입력과 First/현재 안내278입력은 보존했다. 추가 `decomp_mission_common.py`는 제품 작성 전에 실제 Init demo 분기 `004c32f6..004c3307`과 Reload의 **해제 전 캡처** `00463cf0..00463d3b`를108입력으로 관측했다. 실제 명령9,844개와 C locale atol40회다. atol `004e66de/004e6656`은 실제 명령이며 TLS 포인터만 명시 공급한다. 빈/0/1/음수/공백/잘못된 숫자/32비트 감김,두 재시작 필드의0/비제로와 이름 복사를 포함한다.

이108개는 **함수 정상 반환0개·내부 도착점108개**다. 전체 Init/Reload,해제/공장/실제 UI를 관측한 것으로 합산하지 않는다. 실제 명령의 쓰기는 demo DWORD와 한정 스택만 허용하고 도착점/스택 변화/캡처한 레지스터·이름을 직접 기록했다. 최초 원문/도구/보고서는 `extracted/mission-common-20261021-first-capture/`에 제품 작성 전에 보존했다. 게임/OS/창은 없다.

`MissionGuideInput_CommonNative108`은 새108입력의 선택 필드/이름을 직접 대조한다. `CampaignMissionCommon_ActualFiveSessionsFrameReplayAndDemoQueue`는 보호 두 캠페인·세 데모의 실제 Script/월드/Start/공통 판정,10호출 안내/F8 반복/7호출 재개,데모 기한의 실제 일반 큐 예약과 중복 방지를 연결한다. 장치/UI/시간은 명시 외부 입력이며 전체 Kernel/전략AI의 시간 진행이나 실제 Client 재생성/완주를 실행한 검사가 아니다. **이 C++검사는 작성·컴파일만 하며 사용자 지시대로 실행하지 않는다.**

Release 빌드,최초 자료,보호11스크립트 Header/섹션/SHA,현재 코드와 현재 핀 역변환은 `cpppj/mission-common-session.json`·`cpp_mission_common_audit.py --verify`에 기록한다. 과거 보고서의 현재 소스 핀만 후속 이력으로 갱신하며 files/원본/fixture/실행·바이너리/기존 이력을 전체 JSON 역변환으로 보존한다. 직전 단위에서 확인한 기존 `RawTutorialLater.cpp`의 CRLF/LF 핀 차이는 역사 관측과 구별해 그대로 남긴다.

최초/최종 Release 빌드는 모두 **경고0·오류0·종료0**이며 라이브러리·게임/검사 EXE를 생성했다. 커밋 전 검사가 잡은 기존 `UberGump.h`/`CampaignMissionStartTests.cpp`의 CRLF 차이는 줄 내용 전체가 같은 것을 확인하고 LF 속성으로 고정했다. 최종 재빌드 원문 로그는 `extracted/cpp-mission-common-20261021-build2.log`다. 제품 검사·게임 실행 성공으로 합산하지 않는다.

최종 현재 소스 핀은 **100보고서·137항목**이다. LF 정리 전 이번 단위가 추가한 이력만 검증해 되돌린 뒤 최종 소스/빌드로 다시 생성했고, 전체 JSON 역변환이 시작 커밋과 일치한다. 원본/기대값/과거 실행 결과와 기존 이력은 보존했다.

최종 `decomp_mission_common.py --verify`·`cpp_mission_common_audit.py --verify`·직전 `cpp_mission_first_audit.py --verify`와 diff 검사는 모두 종료0이다. 기존 CRLF/LF 역사 핀 차이를 포함한 전체 과거 감사/회귀의 새 통과를 주장하지 않는다.

## 계속 필요한 연결과 통합 검증

전체 Guide0..6 수명/전환·Begin/Abort/Fork/Finish/승패 후속 명령,다른 키와 중첩 Gump·도전/편집/멀티플레이는 아직 별도다. 현재 수동 진입 어댑터의 순번을 임의4로 고치지 않았다. 재시작은 기존 BeginMission 수명을 사용하므로 원본 Guide 공장/가상 소멸의 전체 명령 동일성 근거가 아니다. 실제11미션의 자연 진행/완주·F8/데모 재시작,전략AI,지형 Devastation·포획/Drop·제단/희생·Dude 자연 대피와 외관/음향 동일성을 통합 실제 플레이에서 확인해야 한다. 게임/C++검사/CTest/전체 회귀 이월 지시는 유지한다.
