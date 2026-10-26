# 미션 작업본 reader와 script/Fort의 같은 수명 연결

단위482는 Test Battle에 필요한 **실제 미션 파일 경계**를 연결했다. `MissionScript`가 선택한 reader를 소유하고 추가 설정 파일·언어 script·선택 Fort를 같은 장치에서 읽는다. 실제 `UberGump::LoadMissionWorld`의 자산 VFS 직접 읽기도 이 API로 교체했다. **c2의 실제 편집 월드/State7 왕복은 아직 연결하지 않았다.** 이 단위의 신규 검사는 창 없는 파일/Config/Fort/Player 통합이며 실제 Uber 게임 창의 실행 증거가 아니다.

작업 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. `AGENTS.md`·공통/닷넷 인수인계를 다시 확인했다. 원본/CD·기존 디컴파일은 수정하지 않았고 게임·GUI·음향은 실행하지 않았다.

## 변경한 경계

| 대상 | 동작 |
| --- | --- |
| [MissionScript](../../cpppj/src/client/Mission.h) | 선택 reader의 소유 복사를 미션에 둔다. 명시 source가 없으면 기존 Configuration.Reader를 사용한다. 전역 Config reader·DataDir·missionSpec·fortSpec을 교체하지 않는다. |
| [생성/읽기 구현](../../cpppj/src/client/Mission.cpp) | 추가 설정의 `@파일`과 현재 언어의 missionSpec을 같은 source로 읽는다. `ReadFortImage(selectedFort)`는 실제 설치 요청이 이미 고른 이름을 받아 fortSpec을 계산한다. 이후 loadFort가 바뀌어도 이름을 다시 고르지 않는다. |
| Guide 공장 | `CreateGuideMissionScript`가 source를 넘긴 뒤 기존 Native/Config 공개 순서를 유지한다. 미션 반납 시 script/reader가 먼저 해제되고 Native0→Config0이 이어진다. |
| [실제 미션 로더](../../cpppj/src/client/UberGump.cpp) | `CreateMission`이 명시 source를 받을 수 있고, LoadMissionWorld는 같은 미션의 ReadFortImage 결과를 기존 공유 원문 이미지/실제 Fort 파서·Session 설치에 전달한다. 이 코드 연결을 빌드했으며 신규 검사에서 Uber 창을 실행하지는 않았다. |

source의 명시 부재/예외를 전역 자산 reader로 우회하지 않는다. 빈 벡터는 성공한 빈 파일이며 부재와 구별한다. Fort 부재의 기존 `Game file not found: <path>` 진단도 유지했다. 일반 캠페인은 기본 자산 reader를 사용한다.

Test Battle 연결에서는 기존 `EditorCatalogWorkspace`를 reader lambda가 공유 소유하면 된다. 이 장치의 canonical 경로 매핑, 기존 작업본 유지, 완료된 삭제 항목의 원본 부활 방지, 현재 파일 재읽기를 그대로 사용한다. Editor Runtime의 멤버 참조를 빌려 World 반납 뒤 reader가 만료되는 방식은 쓰지 않는다. `.english`와 다른 언어 파일은 사용자가 별도 텍스트 편집기로 작성하는 원본 방식을 유지하며 게임 안에 새 script 작성기를 만들지 않았다.

## 검증 결과와 범위

[신규 검사](../../cpppj/tests/MissionFileSourceTests.cpp) 5개는 실제 canonical 작업 폴더/장부와 원래 Config·Fort 파서·Player 입력을 사용한다.

- 새 사용자 script의 `myStartMoney=12345`, AI2 SP987/이름/능력/동맹/기술, A. 브리핑과 loadFort 별칭을 읽고, 원본 TEST01과 **다른 전체 Fort 바이트**가 같은 미션의 설치 API로 전달되는지 확인했다. 이 수치는 명시 검사 입력이며 자연 플레이나 원본 초기 SP 관측이 아니다.
- 이관 완료된 script/Fort를 작업 폴더에서 삭제했을 때 보호 원본이 존재해도 부재가 유지되고, 실제 재작성한 작업 파일은 다시 읽히는지 확인했다.
- 외부 workspace 소유자를 해제한 뒤에도 미션이 같은 reader/잠금을 유지하고 실제 Guide 반납/Config0 알림 전에 해제되는지 확인했다. 실제 Editor World 교체 자체는 후속이다.
- `.korean`과 `.english`의 원래 missionSpec 치환, 일반 미션의 원본 reader, 추가 설정 파일의 원문 `@` 포함 경로, 명시 부재·빈 파일·읽기 예외와 생성 실패를 구별했다.

최초 집중은 **5개 중 CHECK1실패**였다. 신규 입력이 `Tutorial2.fort`가 있다고 가정한 것이 원인이다. 보호 Tutorial2 script의 실제 loadFort/FortPath로 비교 입력을 읽도록 신규 검사만 고쳤다. 존재하지 않는 파일을 만들거나 제품/기존 검사·기대 조건을 약화하지 않았다. 첫 실패 원시는 `extracted/cpp-mission-file-source-482/focused1.*`에 보존했다.

최종 Release build2 **경고/오류0**, 집중 **102고유/108관측·실패0**이며 신규5개가 모두 포함됐다. 기존 Config/MissionScript·Guide·Tutorial Init·Editor 설치42개·작업본/삭제·미션 Start·Tutorial Session·실제 섬 창·토글 표시 검사를 실행했다. 기존 **3,263등록 전부 유지**, 신규5개로3,268등록이다. 기존 검사 본문·원본/CD/AGENTS **3,192파일 바이트동일**이며 기존 검사 CMake1파일에는 등록만 추가했다.

build1→2에서 바뀐 소스는 **신규 MissionFileSourceTests.cpp 하나**다. 제품 소스와 게임은 동일하며, 최종 소스/검사2,844파일은 build2 전후와 최종 검사 때 동일하다. 게임 SHA256 `741e33af31e0cb7c7d4d3fd6833dc88a50080c0ce3fb586b2470134245197019`, 검사 실행 파일 `1aae3a27a30c30ca47e868816716c1c407e92a85c75138ba43d1cbe9d666de6a`다. 신규 Native fixture/CPU 실행은 없고 기존 원문 fixture는 해당 집중 검사에서 재생했다. 마지막 전체 C++ 검사는480의3,260/실패0이며 이번에는 반복하지 않았다.

[실행 보고서](../../cpppj/mission-file-source-runtime-20261027.json), 원시 `extracted/cpp-mission-file-source-482/`을 본다. 두 빌드와 모든 집중 검사 프로세스는 종료했다.

## 다음 연결

481의 c2 부모에서 실제 snapshot 뒤 선택하는 새 미션 공장에 `[workspace=editorCatalog_]` reader를 전달한다. 먼저 실제 Guide 해제와 Editor World 반납을 분리하고, 살아 있는 Session에서 Tutorial Init의 Template 정리와 State7 Runtime 반납을 검증한다. `RetireWorldForState`가 새 미션의 pending Fort 요청/Zone까지 비우지 않도록 원문 수명 위치를 맞춘다. 그 뒤 일반 입력의 c2→실제 State7 설치→Close→pending/선택2→State1 편집 복귀를 연결한다.

실제 게임의 Add Island GUI·저장 재진입·Test Battle/Return·전체 미션 자연 완주 및 화면/음향 동일성은 계속 남았다. 실제 구동과 사용자 개입 검증은 사용자 지시대로 뒤로 미룬다.
