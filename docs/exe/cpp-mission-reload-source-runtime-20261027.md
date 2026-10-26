# 실제 로그인 재로드의 작업본 source와 Session 소유 수명

단위483은 로컬 로그인 중 작업본 Fort 대신 자산 폴더의 원본을 다시 읽을 수 있던 경계를 수정했다. 482의 최초 설치뿐 아니라 **실제 Session 로그인 재로드도 같은 미션 source와 현재 fortSpec**을 사용한다. 실제 Zone/패킷/월드 설치를 거쳐 상태14를 확인했다. Test Battle의 c2 일반 입력·살아 있는 Editor World/Guide/State7 교체·State1 편집 복귀는 아직 후속이다.

작업 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. `AGENTS.md`·공통/닷넷 인수인계를 다시 읽었고 원본/CD·기존 디컴파일은 수정하지 않았다. 게임·GUI·음향 장치를 실행하지 않았다. 사용자 요청에 따라 이번 단위의 문서/로컬 커밋을 마친 뒤 목표를 일시 정지하며, 다음 기능 작업은 시작하지 않는다.

## 연결한 실제 경계

| 대상 | 변경 |
| --- | --- |
| [MissionScript::FortReader](../../cpppj/src/client/Mission.h) | 선택 source를 소유 복사한 Fort reader를 만든다. Script 객체를 나중에 다시 조회하지 않으며, 현재 Client Config로 선택 이름의 fortSpec을 해석한다. Config owner는 Session보다 오래 살아야 한다. |
| [공통 읽기](../../cpppj/src/client/Mission.cpp) | 최초 설치의 ReadFortImage와 Session reader가 같은 ReadFortSource를 사용한다. 명시 source의 부재/예외를 자산 fallback으로 바꾸지 않고, 성공한 빈 파일은 기존 Fort 파서에 전달한다. |
| [실제 Client 연결](../../cpppj/src/client/UberGump.cpp) | 같은 미션의 최초 원문 이미지와 FortReader를 Session services에 함께 전달한다. |
| [Login::Reload](../../cpppj/src/client/RawCampaignSessionLogin.inc) | 기존 고정 `d/<name>.fort` 대신 명시 missionFortRead가 있으면 그 장치로 다시 읽는다. 정상 읽기/파싱 뒤에만 기존 InstallFile을 실행한다. 명시 장치가 없는 독립 Session은 기존 자산 경계를 유지한다. |

이 변경은 기존 Session 어댑터의 파일 source 전달을 복원한 것이다. 기존 MissionLogin Native fixture에서 `00460680` 현재 경로와 `004bf550` 파일 로드는 큰 외부 경계다. 이번 단위가 그 전체 원본 파일 로더·manager 유지/캐시 시점까지 새로 복원하거나 독립 CPU로 검증한 것은 아니다. c2와 전체 State7 연결 중 해당 수명 경계를 계속 대조한다.

## 실제 Session 검증

[신규 검사](../../cpppj/tests/CampaignMissionReloadSourceTests.inc)는 기존 Session helper를 빌리며 성공/Ready/상태14 값을 직접 입력하지 않는다. 실제 로컬 Zone·양끝 전송 큐·로그인/응답·월드 설치를 실행했다. 실제 OS 게임 창이나 네트워크 서버는 띄우지 않는다.

- **정상 로그인3회:** 자산 폴더에 없는 `WorkingReload483` 이름으로 상태14·PlayerKnown1·섬 설치를 확인했다. 최초 적재 뒤 작업 파일을 서로 다른 전체 Fort로 재작성한 경우와 현재 fortSpec을 다른 작업 하위 폴더로 바꾼 경우 모두 로그인에서 정확히 한 번 새 파일을 읽었다. State/Subscriber를 제외한 모든 원본 섹션 바이트도 선택 파일과 대조했다.
- Script와 편집 외부 workspace 소유자를 해제한 뒤에도 Session이 namespace/잠금을 보유해 같은 실제 로그인과 지형 설치에 성공했다. Session 반납 뒤에는 해당 workspace가 소멸했다.
- **실패 로그인3회:** 선택 source의 부재·빈 파일·I/O 예외를 구별해 실패하고, 상태14로 진행하지 않으며 기존 Template manager와 지형을 보존했다. 실제 원본 파일은 존재하지만 fallback하지 않았다.

Release build1 **경고/오류0**, 신규3개 포함 집중 **65고유/68관측·실패0**이다. 관련 Config/MissionScript·로그인·Guide·Tutorial Init·c2 부모·토글 표시를 검사했다. 같은 최종 실행 파일로 전체 **C++3,271개/실패0·CTest1개/실패0**도 실행했다. 빌드/제품/신규 검사의 최초 실패는 없다.

전체 로그의 성공 이름3개에는 stdout 중간에 stderr 진단이 섞여 최초 집계기의 등록 이름 비교가 실패했다. 같은 성공 표식 안의 앞/뒤 이름 조각을 원시 그대로 대조했고, 해당 전체 이름3개를 별도 집중 실행해 통과를 확인했다. 현재 CTest의 성공 요약 형식/경로 escaping도 실제 출력에 맞춰 확인했다. **원시 전체 로그·제품·검사를 고치지 않았다.** 최초 집계기와 보완 기록은 원시 디렉터리에 보존했다.

기존3,268등록을 모두 유지했고 신규3개로3,271등록이다. 기존 검사 본문·원본/CD/AGENTS **3,192파일 바이트동일**, CampaignSessionTests의 기존 **759,330BYTE 접두부 동일**이며 새 include만 덧붙였다. 검사 CMake에는 새 include 파일 등록만 추가했다. 소스/검사 **2,845파일**은 빌드 전후와 최종 전체 검사 뒤 동일하다.

최종 게임 SHA256은 `e13aea9cce887e7581caed04867a9d680bd9bea71e0d50b244946173781d16fa`, 검사 실행 파일은 `ffde99aca195e70d310d71530098fa3ad8144e9c29ee6ac807e4e54a4c34b40c`다. 신규 Native CPU 실행은 없으며 전체 C++ 검사가 기존 Native fixture를 재생했다. [실행 보고서](../../cpppj/mission-reload-source-runtime-20261027.json), 원시 `extracted/cpp-mission-reload-source-483/`을 본다. build1/집중/whole1 프로세스는 모두 종료했다.

## 일시 정지 후 재개점

Guide 해제와 World 반납을 분리하는 작업을 검토하던 중 이번 로그인 파일 경계를 발견해 먼저 마감했다. **World/Guide 수명 변경은 이번에 적용하지 않았다.** 다음에는 481의 c2 부모·482/483의 source 장치를 실제 일반 입력과 연결하기 전에, 살아 있는 Session의 Tutorial Init Template 정리와 원문 위치의 State7 반납을 검증한다. `RetireWorldForState`가 새 미션의 pending Fort 요청/Zone을 지우지 않게 하고 State1 복귀까지 이어야 한다.

원래 전체 목표는 유지한다. Edit의 나머지 도구/시나리오와 실제 Test Battle 왕복, Add Island GUI/저장 재진입, Early6·Priest9·Campaign16·Demo3의 자연 완주, 메뉴/덱/미니맵/단축키/ESC와 화면·음향 동일성 검증은 남아 있다. 실제 게임 구동과 사용자 개입 검증은 사용자 지시대로 뒤로 미룬다. 다른 PC에서 재개하면 AGENTS.md의 전체 정밀 디컴파일 규칙부터 적용한다.
