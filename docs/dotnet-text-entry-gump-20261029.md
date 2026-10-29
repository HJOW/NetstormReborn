# dotnetpj R3i-2 — 문자 입력창과 Console

2026-10-29, HJOW-Athlon. 문자 편집 버퍼/저장소 단위를 `39ec3a5a`로 커밋한 뒤 cpppj `TextEntryGump.h/.cpp`·`TextEntryGumpTests.cpp`·`RawTextEntryControls.h/.cpp`·`TextEntryClientTests.cpp`를 읽었다. 현재/마지막 정밀 디컴파일 PC는 HJOW-Athlon이다. 원본 게임/창/PE 에뮬레이터/시스템 클립보드와 금지된 실제 플레이 재현·멀티플레이 검사는 실행하지 않았다.

## 1. 구현

같은 기본/배경 객체를 상속한 TextEntryGumpNode와 ConsoleGumpNode를 만들었다. 실제0x28 편집 버퍼와 두 Slider를 연결하는 생성/부착·현재 시각/점멸·스크롤·원시 입력/Shift 선택·클립보드·Console 자동완성/제출·선택/캐럿/수신 대상 출력·소유 버퍼/기본 소멸 순서를 옮겼다. 예약 필드는 생성에서 쓰지 않는다.

스크롤은 이전 scrollLine으로 열을 제한한 뒤 두 Slider를 호출하고 현재position을 다시 읽는다. 현재 length+1과 엄격 비교하는 문자 제한은 선택 폭을 빼지 않는다. Ctrl+Backspace 전체Reset, Shift 선택의 원문 기준, multiline Enter의LF 변환 뒤 부모 통지/변경/Redraw/원시 전달 순서를 보존했다. Console Tab은 새 단어의 같은 본문 뷰와 반복 요청null을 구별한다. Enter 제출은 기본 편집 처리보다 먼저 현재 본문을 전달한다.

출력은 1024바이트 줄 저장소와 원시 선택쌍을 사용한다. 수평 제거 뒤 길이/선택쌍을 임의 보정하지 않으며 빈 줄/끝LF·선택 반전·콜론 앞 수신 대상 색·캐럿 명시 길이·현재 화면 위치 재조회를 유지한다. 점멸은 Redraw 뒤 두 번째 실제 시각으로 예약한다. 제외문자 교체는 이전 할당부터 반납하며 새 목록에서LF만 제거한다. 원문 소멸자가 별도 제외문자를 반납하지 않는 동작도 그대로 둔다. 소유자는 필요 시 SetExcluded(null)로 반납한다.

RawTextEntryControls의 공통 장치 연결을 따라 실제 배경/편집 저장소/Slider·자식/트리/부모 사건/글꼴 측정·출력/픽셀/기본 수명을 연결하는 BindTextEntryGumpDevices와 현재 종류별 Attach/사건·캡처/idle/Draw/scalar 분배를 작성했다. 객체 할당·토큰/현재 종류·클립보드·시계와 전체 수명 소유는 상위 장치가 공급한다. 원본 RawTextEntryControls 전체 소유자나 전체 메뉴 호스트를 완료한 것으로 세지 않는다.

## 2. 독립 원본 대조

읽기 전용 `text-entry-gump-x86.tsv` **2292행**(1146입력의 두 x87 관찰)을 Link했다. 앞13열의 노드/버퍼·기존 메모리/인자·문자/제외문자·클립보드/시각·후보/수신 대상/할당fill만 실제 입력으로 사용한다. 뒤10열은 기대값으로만 사용한다.

13개 진입의 전체48DWORD, 두 실제 자식11DWORD씩, 버퍼10DWORD, 반납한 블록과16바이트 가드를 포함한 전체 메모리, 클립보드/외부 효과 순서·인자와 의미 있는 반환이 전부 일치한다. void/생성자의 임의 EAX는 비교하지 않고 기존 C++ 대조기와 같이0으로 관측한다. 원문 stackdelta/명령 수는 자료 메타 검사이며 관리 코드의 실제 ABI 일치를 뜻하지 않는다. 원본 기대값은 수정/재생성하지 않았다.

최초 대조의 제외문자null1건은 검사 입력의 byte[] 삼항식이 nullable ReadOnlyMemory로 변환되는 과정에서 빈 뷰가 된 문제였다. 검사에서 null 변환을 명시해 수정했고 제품 동작은 바꾸지 않았다.

fixture103개·새 원문2293행/해시658245fb178e이며 reflected-commit은 `f95a8db2...`로 유지한다. 포트222행(L1 105/port117), 새 TextEntryGump2행의 기준은 `39ec3a5a`다.

## 3. 실제 공통 연결 검사

독립 입력 캐시를 실제 FontStore/BitmapFont 판독기·GumpText/GumpTextDraw에 공급하고 같은 기본/파생·등록표/트리/실제 Slider·버튼 자식/현재 픽셀 표면·클립·프레임idle/자식 우선 수명을 연결했다. 상태를 흉내 내는 측정/출력 성공 스텁은 통합 검사에 사용하지 않았다. 새 입력창1개가 등록 노드13개를 소유하고 소멸 뒤 버퍼/물리 할당/idle이 반납됨을 검사한다.

6개 통합 검사는 선택/실제 입력·클립보드/제외문자/복사출력, 후보 순환/현재 본문 제출, 실제 선택 반전·캐럿/점멸·읽기전용·클립과guard, 재부착/현재Slider 위치와 페이지 제한, 자식 우선 삭제/빌린 버퍼 보존을 포함한다. 원본2292행 대조1개를 더한 집중 **7개 통과**다.

조립 중 선택 인자가 있는 Fill의 메서드 그룹을 명시 람다로 바꾸고 검사 클립 속성명을 고쳤다. 초기 통합 검사2건의 가정도 고쳤다. 실제 Slider는 페이지20 요청을 최대범위2로 제한하며, 클립 안의 선택 채우기를 명시해야 한 픽셀 글리프가 없는 행에서도 출력 여부를 검사할 수 있다. 원본 기대값 변경은 없다.

전체 회귀 **1,895개(Assets363+Core1030+Client502) 통과**, 실패/건너뜀0이다. Release 전체 빌드는 경고/오류0으로 종료했다. Git 제외 로그는 `extracted/dotnet-text-entry-gump-native-initial-20261029.log`, `dotnet-text-entry-gump-native-20261029.log`, `dotnet-text-entry-gump-focused-initial-20261029.log`, `dotnet-text-entry-gump-focused-failed-20261029.log`, `dotnet-text-entry-gump-focused-20261029.log`, `dotnet-text-entry-gump-tests-20261029.log`, `dotnet-text-entry-gump-build-20261029.log`다.

## 4. 다음

다음 R3j는 ImageGump의 실제 SHP/색표/16.16 배율/투명 적중과 기존 inventory-image 원본221행 대조다. R3의 이미지/GIF·HTML/대화창·실제 메뉴 상위 소유/분배·전체 자산/ConfigInterface/음향/실제 호스트 조립을 cpppj 의존성 순서로 진행한다. RawTextEntryControls의 전체 소유/부분 생성 정리와 실제 클립보드 장치, 문자 입력창을 쓰는 Save as·숫자 창/ChatWindow 등도 해당 조립에서 처리한다. R4 이후 전체 미션/세션/월드/HUD/Edit·후순위 멀티플레이 사전 구현은 남는다. 전체 목표는 계속 진행 중이다.

실제 전체 메뉴/게임 캡처0·기존 코드/검사 은퇴0이다. cpppj·보호 원본/CD·AGENTS.md·공통 인계·외부 TODO.md를 수정하지 않았다.
