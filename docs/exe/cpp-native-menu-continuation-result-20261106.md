# Native State3 continuation 반환 계약 — 단위951

제목 갱신 뒤 현재 상태가 바뀐 분기에서, 선택된 설치 callback이 정상 반환해도 항상 오류를 던지던 계약을 수정했다. callback의 **공통 확정/미션 조기 반환 결과**를 기존 `RawLocalMissionState`에 그대로 전달한다. 새3+기존8, **고유11검사/실패0**, Release 경고/오류0이다. [완료 JSON](../../cpppj/native-menu-continuation-result-20261106.json)을 따른다.

2026-11-06, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 PC/2026-11-03이다. 원본/CD·AGENTS.md·기존 fixture·디컴파일 원시는 바꾸지 않았다. 이번 단위는 창/게임/실제 통신을 실행하지 않았다.

## 원문과 변경

원문 `004b6dd0`은 State3의 이름 첫 BYTE 정리·`00460730` 제목 갱신 뒤 현재 상태가3이 아니면 `004b72eb`의 다른 Fort 설치로 이어진다. 이름 선택/파일 설치 뒤 미션이 있으면 `004b50b0`을 호출하고 공통 State 확정 전에 직접 ret한다. 일반 파일/월드 설치가 끝난 경로는 HUD/음악/팔레트·다음4 예약을 거쳐 원래 다섯144BYTE 블록의 공통 확정으로 간다.

[기존 부모](../../cpppj/src/client/NativeMainMenuState.cpp)는 다른 설치를 `std::function<void()>`로 부른 뒤 무조건 예외를 던졌다. 정상 설치와 미션 조기 반환을 표현할 수 없었다. [hook 반환형](../../cpppj/src/client/NativeMainMenuState.h)을 기존 `LocalMissionParentResult`로 바꾸고 그 결과를 직접 반환하도록 했다. 다른 설치를 끝낸 뒤 메인 전용 Guide 해제/Welcome·Tips·메인 tail을 다시 실행하지 않는다.

실제 호스트의 다른 Fort 설치 callback은 **아직 미연결**이다. [호스트](../../cpppj/src/client/UberEditor.inc)의 명시 오류는 그대로 두고 반환형만 맞췄다. 미연결 본문을 빈 성공이나 상태번호 강제로 대체하지 않았다. 기존82행 원문 대조의 명시 중단 callback도 같은 의미와 관측을 유지한 채 반환형만 맞췄다.

## 검증 범위

새 [회귀3개](../../cpppj/tests/NativeMainMenuStateTests.cpp)는 실제 State3 부모와 실제 공통 분배기를 함께 사용한다. 큰 Fort 설치 본문은 명시 callback 경계다.

- 정상 설치 결과: 변경된 현재2와 부가 DWORD가 phase 전체에 확정되고 이전 phase가 old로 이동한다. 메인 전용 안내/tail은 실행하지 않는다.
- 미션 조기 반환: 이전 phase/old를 유지하고 현재2·다음7·선택 callback의 부가 쓰기를 보존한다.
- 선택 장치 누락과 설치 예외: 앞선 prefix/이름 첫 BYTE·callback 부분 쓰기는 보존하되 공통 확정이나 메인 성공으로 바꾸지 않는다.

등록 검사3개 중 첫 검사는 두 반환 결과를 별도 입력으로 확인한다. 기존8검사에는 기본 메뉴/Welcome/Startup/Intro/현재값 재조회/퇴역 검증 비활성화와 고정 독립 PE82행의 같은 다섯 블록·trace 대조가 포함된다. 이번에 PE를 새로 실행하거나 fixture를 다시 생성하지 않았다. 이 범위는 실제 파일·월드·UI 전체 continuation 실행의 근거가 아니다.

게임과 미션 진입 집중 대상2개의 Release 빌드가 종료0·컴파일 경고/오류0이다. `netstorm_mission_entry_tests --filter NativeMainMenu`는11검사/실패0이다. 새 fixture는0개이며 보호2787개/fixture967개가 불변이다.

제품951/11.01·11,249,152BYTE·EXE SHA-256은 `9ac79063e175d869148b81ecd23eba2bf72a48efd6b583c38d2159b34e437298`이다. 마지막 실제 GUI는950 제품의 검증이며 새951의 GUI 확인으로 쓰지 않는다. 원시는 Git 제외 `extracted/cpp-resume-951/`에 보존한다.

## 다음 작업

다음952는 같은3번의 실제 Fort continuation 접합이다. 이미 실행한 prefix/HUD를 반복하지 않고 기존 파일·미션 조기 반환·후반 UI/공통 확정·호스트 공개를 공유해야 한다. State1/단독State7·cleanup Tick/연속 전환, LeaveBattle 나머지 이유/CD·State6도 계속 잔여다. 일반 전투 저장/복구·MP 부모는 기존8번 순서를 유지한다.

포획/희생·특정 적 공격·승리·공중3종 전용·신규 MP/패킷/통신·물리 입력/PCM/CD·전체 미션/CTest/ZIP은 실행하지 않았다. DN/WB 파일도 변경하지 않았다.
