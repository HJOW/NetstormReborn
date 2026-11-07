# 1052 — SubscriberGlobalsTests.cpp C2607 수정

사용자가 다른 PC의 cpppj 테스트 빌드에서 보고한 `SubscriberGlobalsTests.cpp(40,50): error C2607`을 같은 파일로 재현하고 수정했다. 제품 클래스와 실행파일은 바꾸지 않았다. 현재 실행 제품은 **1051 / 11.01**이며 SHA-256 `ac9dd246e2bf5c574fedbf90871cf08ae50d9cecb3e6d5e13bfbe480d89e6fd8`가 유지됐다. 직전 신전 단위는 `da2bf5b8 / 20261107-68 CP`다.

40행의 `sizeof(ClientSubscriberGlobals)==8`은 두 Subscriber DWORD만 소유하던 시점의 단언이다.1011의 `bed549ff / 20261107-25 CP`에서 현재 사용자 이름의64BYTE Handle이 같은 Client 수명으로 추가됐으나 이 단언이 갱신되지 않았다. 다른 PC 설정의 차이가 아니라 현재 타입 정의와 구형 테스트의 불일치다. 클래스 전체 크기를 새 숫자로 고정하는 대신 실제 두 DWORD의4BYTE 폭/const참조 반환, Handle의64BYTE mutable/const참조 계약을 검사하도록 바꿨다. 초기값·복사/이동 금지·같은 주소 검사는 유지했다. 기존 첫 사례에서 새 이름 버퍼 전체0/같은 const주소와 Subscriber 두 값 변경 뒤 이름 보존도 확인한다.

확인은 저장소 밖 별도 구현을 검사한 것이 아니라 **저장소의 실제 SubscriberGlobalsTests.cpp와 기존 FocusedTestMain.cpp**를 같은1051 제품 netstorm.lib에 링크하는 격리 CMake 대상으로 했다. MSVC/Visual Studio18 2026·MSBuild18.11.1·C++20·Release/x64다. 수정 전 같은 `(40,50) C2607`/빌드 종료1을 확인했고, 수정 후 **빌드 종료0·경고/오류0**이다. 같은 파일의 기존7사례가 모두 통과했다. 값 생성/alias·복사/이동/교차 대입·Chat const참조/수명·기존120행 fixture 중10개 Tutorial Init checkpoint를 검사하며 게임/창/네트워크 실행은 없다. 새 사례나 fixture를 만들지 않았다.

격리 설정/로그는 `extracted/cpp-resume-1052/`의 `focused/CMakeLists.txt`, `before-build.log`, `after-build.log`, `focused-tests.log`다. 원시는 이 PC 전용/Git 제외이며 다른 PC에 있다고 가정하지 않는다. 수정은 정상 cpppj 테스트 대상에 포함된 원래 파일 자체에 반영돼 있다. **전체 netstorm_tests/솔루션을 다시 빌드하거나 전체 검사 통과를 주장한 것은 아니다.**

AGENTS.md·원본/CD/fixture 보호 2800파일 불변, 실행 제품1051 EXE 12,219,904바이트 불변을 확인했다. 게임·통신·신규 MP 검사는0이다. 사용자 제보 건은 해결했고 전체 /goal은 미완료다. 다음은 State19/CS·메타 부모, LAN 나머지→한국어 후속→시나리오 UI다. 마지막 전체 정밀 디컴파일은 기존 **2026-11-03 / HJOW-Athlon**이며 이번에는 디컴파일을 수행하지 않았다.
