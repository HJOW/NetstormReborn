# dotnetpj R3i-1 — 문자 편집 버퍼와 실제 소유 저장소

2026-10-29, HJOW-Athlon. AGENTS.md·현행 인계/공통 제약을 다시 읽었다. 직전 목표 턴은 메뉴 입력/출력 두 단위의 구현·검증·커밋을 완료한 진행이며 시작 기준은 `a4b04553`이다. 현재/마지막 정밀 디컴파일 PC는 HJOW-Athlon이다. cpppj `TextEntryBuffer.h/.cpp`·`TextEntryStorage.h/.cpp`·`TextEntryBufferTests.cpp`와 기존 원본 관찰을 사용했다. 원본 게임/창/PE 에뮬레이터/시스템 클립보드는 실행하지 않았다.

## 1. 구현

표시용 StyleTextBuffer와 구별해 원본0x28바이트 편집 상태10DWORD와 본문/줄offset 가상 주소를 옮겼다. 현재 메모리 조회는 순수 경계이며 alloc/free/resize/clipboard는 현재 상태를 바꿀 수 있다. 원본 signed/unsigned DWORD 감김·NUL 범위·4바이트 선행 복사·memmove·현재 필드 재조회·부분 실패/중단을 유지한다.

Construct는 요청0일 때2048바이트를 요청하지만 성공한 capacity 필드는 요청 크기와 무관하게2048을 쓴다. 본문 첫BYTE는 초기화하지 않는다. Destroy는 data0일 때 줄 표도 남기는 조건과 본문 반납→현재 data/capacity0→현재 줄 표 반납→0 순서를 보존한다. 선택 정렬/제한, 교체/재할당, 자기 본문 Reset, LF 줄offset/마지막length·확장 실패, 행/전체 복사의 출력 보존과 선택쌍, 빈 선택 Copy/Cut·256바이트 Paste/제외 문자/dirty를 연결했다.

TextEntryStorage는 객체 토큰과 본문/줄 표 주소를 구별한다. 같은 버퍼 객체/상태를 고정 소유하고 내부 주소+offset과 정확한 끝의 빈 뷰를 제공한다. 새 저장소 성공 뒤 내용 보존/이전 반납을 수행하며 반납한 주소는 다시 조회할 수 없다. 가상 주소는 원본 C++와 같은0x20000000 시작/정렬/0x60000000 상한이다. 실제 배열은 GC.AllocateUninitializedArray를 사용하며 임의0/sentinel로 채우지 않는다. CLR/OS가 돌려주는0은 그대로 둔다. 이전 뷰는 재할당/해제 뒤 사용하지 않는 계약이다. 전체 시스템 클립보드/위젯 소유/장치 조립은 후속이다.

## 2. 독립 원본 대조

기존 `cpppj/tests/fixtures/text-entry-buffer-x86.tsv`의 **570행**(실제285입력의 두 관찰)을 읽기 전용 Link로 연결했다. 앞11열의 상태/기존 블록/인자/입력/실패/명시 콜백만 장치 입력에 사용했다. 뒤 관찰 열은 기대값으로만 읽는다.

13개 진입의 정상 완료/명시 중단, 전체10DWORD, 반납 메모리를 포함한 모든 할당/가드 바이트, 출력2048바이트·선택쌍·클립보드, 외부 효과 순서/인자, 의미 있는 반환값이 전부 일치한다. 명시 중단10건도 일치한다. void/미완료 EAX를 의미 있는 반환으로 비교하지 않는다. 생성 완료는 기존 C++ 대조기와 같이 정상 완료1로 정규화하며 C# 함수의 실제 EAX가 아니다. 원문 stackdelta/명령 수/CRT 우회0은 자료의 원본 메타 검증이며 관리 코드의 ABI 일치 주장으로 사용하지 않는다. 원본 기대값은 수정하거나 재생성하지 않았다.

fixture 목록은102개이며 새 파일은 머리행을 포함한571행으로 기록한다. 전체 reflected-commit은 기존 `f95a8db2...`를 유지한다. 포트 맵은220행(L1 103/port117)이며 편집 버퍼2행만 L1, 실제 소유 저장소2행은 port다. 이 대조를 TextEntryGump 위젯/시스템 클립보드/실제 화면·플레이 완료로 세지 않는다.

## 3. 실제 저장소 검사

집중 **9개 통과**다. 원본 전수 대조1개 외에 실제 소유 저장소에서 LF/CR/끝 빈 줄·선택쌍/출력 거부 보존, 빈 선택 Copy/Cut·제외 문자/빈 Paste, 본문 재할당/객체 주소 보존,131줄/줄 표 확장·이전 표 반납, 겹친 입력의 REP DWORD 순서와 자기 본문 Reset, 요청 물리폭/원본 capacity 차이·예외 앞 쓰기, 내부 주소/끝 뷰/0크기/중복·반납 순서·도달 클립보드 오류를 확인했다.

최초 Client Release 컴파일과 집중 검사는 수정 없이 통과했고 원본 기대값 변경은 없다. 전체 회귀 **1,888개(Assets363+Core1030+Client495) 통과**, 실패/건너뜀0이다. Release 전체 빌드는 경고/오류0으로 종료했다. Git 제외 로그는 `extracted/dotnet-text-entry-buffer-compile-20261029.log`, `dotnet-text-entry-buffer-native-initial-20261029.log`, `dotnet-text-entry-buffer-focused-initial-20261029.log`, `dotnet-text-entry-buffer-tests-20261029.log`, `dotnet-text-entry-buffer-build-20261029.log`다.

## 4. 다음

TextEntryGump의 실제 생성/Draw/캐럿·선택/키보드·마우스/클립보드·소유 수명에 이 버퍼를 연결한다. 이어 이미지/HTML·대화창·실제 메뉴 상위 소유/분배/전체 자산/ConfigInterface/음향/GIF/호스트와 R4 이후 전체 미션/세션/월드/HUD/Edit·후순위 멀티플레이 사전 구현이 남는다. 전체 목표는 계속 진행 중이다.

기존 코드/검사 은퇴0·실제 게임/메뉴 캡처0이며 cpppj·보호 원본/CD·AGENTS.md·공통 인계·외부 TODO.md를 수정하지 않았다. 금지된 실제 플레이 재현과 멀티플레이 검사는 하지 않았다.
