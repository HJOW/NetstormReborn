# Test Battle의 원본 명령 부모와 Guide 수명 순서

단위481은 **c2 명령 부모와 Guide 보조 함수 복원**이다. 실제 게임의 Test Battle/Return to Editing 왕복 완료가 아니다. `UberGump`의 일반 입력에는 아직 이 부모를 연결하지 않았다. 실제 Editor World가 살아 있는 동안의 Guide 생성/Init, State7 교체, 작업본 Fort/언어 스크립트 reader를 먼저 연결해야 한다.

작업 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. `AGENTS.md`·공통/닷넷 인수인계를 확인했고 원본/CD와 기존 디컴파일을 읽기만 했다. 게임·GUI·음향·네트워크 실행은 없다.

## 원문과 제품 변경

| 원문 10.78 | 복원한 동작 |
| --- | --- |
| `005429fc = 020000c2`, 일반 입력 `004d81d0` | 사건 x와 무관하게 **y==0**만 테스트 시작, 모든 비영 DWORD는 편집 복귀다. 일반 입력의 release는 효과 없이 소비한다. |
| `004d81dc..004d81fd` | 현재 맵 snapshot이 반환한 뒤 현재 Fort 이름을 읽고 Guide.Begin을 실행한다. 공장이 정상 반환한 뒤 새 현재 미션의 **+64=1**을 쓴다. demo는 바꾸지 않는다. |
| `004d8202..004d822a` | Guide.Close 뒤 현재 Fort 이름을 다시 읽어 pending에 복사한다. 그 뒤 선택 모드2, 실제 RequestFortEdit 전체 부모 순서다. |
| `00463b20` | 현재 미션이 있을 때만 release한다. **finish 가상+1c는 호출하지 않으며**, 해제 재진입 현재도 추가로 정리하지 않는다. 새 `CloseGuideMission`이 이 조건을 제공한다. |
| `00463b70` | finish/release가 끝난 뒤 빌린 이름 주소에서 첫 NUL까지 복사한다. 기존 `BeginGuideMission`도 그 위치에서 NUL 길이를 구하도록 보정했다. |

[EditorTestBattle.h](../../cpppj/src/client/EditorTestBattle.h)·[구현](../../cpppj/src/client/EditorTestBattle.cpp)은 실제 `BeginGuideMission`/`CloseGuideMission`과 선택된 큰 하위를 조립한다. 이름 조회는 고정 버퍼의 비소유 뷰를 받을 수 있다. snapshot 뒤 포인터가 바뀌면 새 주소를 읽고, Begin 진입 뒤 release가 내용만 바꾸면 **이미 빌린 주소의 변경된 길이**를 복사한다. pending 실패 때는 이전 선택 모드가 남고, 편집 예약 실패 때는 이미 쓴2가 남는다. missing callback을 빈 성공으로 바꾸지 않는다.

CD 10.72의 기존 대응 기록은 Begin `004170f0`, Close `00417340`, RequestFortEdit `0042fe00`이다. 이번 신규 CPU 관측은 10.78만 대상으로 하며 CD 동일성을 추가로 검증한 것은 아니다.

## 독립 원문과 집중 검증

[관측 도구](../../tools/decomp_editor_test_battle.py)는 보호 PE의 원래 주소를 유지한 격리 CPU에서 입력 관문, 실제 Begin의 BYTE 복사, Close 조건, +64 쓰기, Battle0의 실제 next1 부모를 실행한다. snapshot·release/해제 재진입·미션 공장·pending 복사와 Guide/Gump 소비 결과는 **명시 외부 경계**다. Battle14 보존은 기존 FortSelection 원문 fixture/검사를 사용한다. 이 장치를 실제 Runtime나 게임 실행 증거로 합산하지 않는다.

- **87입력/174행:** 두 x87 제어어, y의0/1/2/ffffffff, x 불일치, 상위 플래그/release, 빈/기존 현재, 해제 재진입, Guide/Gump 소비, 이름 주소/길이 변경과 큰 하위6곳의 부분 중단이다. 정상 반환162행·부분 중단12행이다. `--verify`로 전체 관측을 재실행해 최초 fixture와 바이트 일치를 확인했다.
- 최초 Native 실행은 기존 입력 장치의 허용 범위가 **빈 현재 Guide 입력의 마지막 반환00463f63**을 포함하지 않아 멈췄다. 실제 functions.tsv의00463e40 전체 범위를 추가한 뒤 최초 fixture를 생성했다. 입력/기대값을 제품에 맞춰 바꾸지 않았으며 첫 실패 로그를 보존했다.
- Release build1 **경고/오류0**. 신규3개를 포함한 집중 **42고유/44관측·실패0**이다. 원문174행 재생, 선택된 경계의 실패 위치, 실제 Game 메뉴가 만든 c2 숫자/0·1 인자→실제 InputQueue→명령 부모를 검사했다. Guide/TutorialInit/FortSelection/SaveGoMain/메뉴·섬 창과 사용자 신고 토글 표시 두 검사도 통과했다.
- 기존 **3,260등록 전부 유지**, 신규3개로3,263등록이다. 기존 검사 본문과 원본/CD/AGENTS **3,190파일 바이트동일**, 기존 검사 CMake1파일에는 등록만 추가했다. 소스/검사 **2,843파일**은 빌드 전후와 최종 검사 때 동일하다.
- 최종 게임 SHA256은 `2421b871e91d5c4055ba58b0a9b21b9cc4985b2a0a816392c9d40ba721569fcd`, 검사 실행 파일은 `b59b6d9d835d3d4c94c8d23ed714406fa56d7cc4c804776adb38d74ba3c5521a`다. 직전480의 전체 C++3,260/실패0 뒤 이번 단위에서는 전체 검사를 반복하지 않았다.

[실행 보고서](../../cpppj/editor-test-battle-parent-20261027.json), [Native 보고서](../../cpppj/editor-test-battle-native-20261027.json), [최초 fixture](../../cpppj/tests/fixtures/editor-test-battle-x86.tsv), 원시 `extracted/cpp-editor-test-battle-481/`을 본다. build1과 모든 집중 검사는 종료했다.

## 다음 연결

`MissionTransitionEffects().release`는 현재 `ClearWorld()`까지 실행한다. 이를 c2의 원문 Guide.Close 대신 그대로 쓰면 World 반납을 앞당기므로 주의한다. `CreateMission`/`InitializeMission`의 빈 World 관문과 Template 고정뷰 대여, 원문 위치의 실제 State7/State1 정리를 함께 해결한다. 새 부모를 빈 callback/직접 next1 쓰기/임의 월드 재생성으로 연결하지 않는다.

Test Battle 출발 수명에 작업본 namespace를 유지하고, `.fort`와 사용자가 직접 작성한 동명 언어 파일을 원래 Config/공장 규칙으로 읽는다. 없는 script·미지원 형식·loadFort 별칭/실패 안내도 원문과 대조한다. 이후 c2 일반 입력을 연결하고 창 없는 실제 Session 왕복을 확인한다. Add Island GUI/저장 재진입·Test Battle GUI·공격 중 미션과 사용자 개입 검증은 뒤로 미룬다. 전체 미션 자연 완주와 화면/음향 동일성 목표는 계속 진행 중이다.
