# dotnetpj R3h-1 — StyleText 조각과 한 줄 TextGump

2026-10-29, HJOW-Athlon. 기준 cpppj는 `702917dcf7d9`의 `StyleText.h/.cpp`와 `TextGump.h/.cpp` 중 한 줄 클래스다. 메뉴 제목/스타일 항목의 선행 소유·측정·출력 기반을 같은 Gump/글꼴/픽셀 저장소에 연결했다.

## 1. 구현

`StyleText`는 안정된 자동 토큰 표의 문자열/파생/소멸 뒤 기본 분류를 보존한다. 문자열/문맥은 값 복사하고 명시 길이의 중간NUL과 마지막NUL을 구분한다. CloneText는 원본처럼 공통+8을 복사 길이로 사용하고 새+8은0으로 둔다. Append의 단일 조각 next=0과 Transfer의 체인 이전/source=0, 삭제 전 next 저장과 마지막 현재head=0도 구별한다. 토큰 소진/손상 체인/실제 미연결 경계는 명시 오류로 거부한다.

측정은 첫 최대상승 순회 뒤 현재 소유head를 다시 읽고 높이→폭 순서로 쓴다. 반면 Draw의 두 번째 순회는 시작head를 유지한다. 각 가상 호출 뒤 현재next를 읽되 CD는 측정 출력 쓰기 전에 next를 저장한다. `in`/`ref`와 실제 저장 필드로 head/폭/높이/next가 같은 주소인 경우도 보존했다. 출력override의 색/스타일/그림자는 쓰지만 font는 조각의 저장 슬롯을 유지한다.

짧은 명령의 판본별 색/폰트/스타일·그림자·문맥 복귀·리터럴~~·미인식 명령·끝 빈 조각과 대괄호 콜백의 현재head/T태그를 옮겼다. 실제 GumpText/FontStore 메트릭·첫 폭·글리프 출력과 파일 팔레트 별칭/RGB 결과를 연결했다. 대괄호 내부의 실제 그림/옵션 해석은 후속 StyleTextMarkup/Markup 경계다.

한 줄 `TextGump`는 실제 기본 생성/등록 뒤 head/가상 표/종류BYTE/정수 크기/전체32바이트 문맥/클립 비트를 쓴다. 이전head는 원본 생성자처럼 반납하지 않는다. AppendText의0길이 조기 반환, Fit의 DWORD 여백과 현재scale, 공통 Draw 보호 비트와 별개인 내부 클립, 현재Position 뒤 현재head 출력과 Clear 두 번→기본 scalar 소멸을 연결했다. 한 줄 Draw/수명 분배는 기존 다른 파생 처리기를 보존한다.

## 2. 검증과 근거의 한계

- C++의 `cpppj/styletext.json`은 정적 복원 보고서다. 이 단위에는 독립 원본 실행 기대값을 추가하지 않았고 **L1 통과로 표시하지 않는다.** 기존 fixture 목록99개/reflected-commit은 그대로이며 포트 맵만182행(L1 97·port85)으로 갱신했다.
- 집중19개가 통과했다. 모든256바이트의 짧은 명령 집합/제한 슬롯, 실제 문맥/조각 스냅숏, T태그/대괄호/미인식/NUL, 값복사/이전/소진/반납/부분 예외/순환, 두 번째head 재조회와 출력 주소 겹침의 판본 차이를 고정 입력 계약으로 검사했다. 실제 글꼴/팔레트/픽셀·생성/트리 부착/수집/클립/맞춤/지연 삭제도 두 판본에서 연결했다.
- 첫 집중 검사 중 TextGump 장면6개는 필수 GumpTree Kill 훅 누락으로 실패했다. 검사 배선만 보완했으며 구현이나 기대값을 바꿔 통과시킨 것이 아니다. 최초 로그는 `extracted/dotnet-styletext-focused-initial-20261029.log`에 보존했다.
- 측정 두 번째 순회도 시작head를 저장해 사용하는 변이는 고정 계약의 예상7/11 대신6/12를 출력해 실패했다. 변이는 제거했다. 이는 현재 콜백 계약 검사의 판별력이며 원본 실행 대조를 대신하지 않는다.
- 원복 후 전체 **1,664개 통과(Assets363·Core1030·Client271), 실패/건너뜀0**, Release 전체 빌드 경고/오류0이다. 변경9파일의 UTF-8·한국어 주석과 보호 경로 변경0을 확인했다. 실제 게임/메뉴 캡처 비교0, 기존 코드/검사 은퇴0이며 화면 동일성은 후속 메뉴 조립의 cpppj 캡처 비교로 확인해야 한다.
- 나머지 로그는 Git 제외 `extracted/dotnet-styletext-compile-20261029.log`·`dotnet-styletext-focused-20261029.log`·`dotnet-styletext-mutation-20261029.log`·`dotnet-styletext-tests-20261029.log`·`dotnet-styletext-build-20261029.log`다.

## 3. 다음 연결

R3h-2는 `StyleTextBuffer`·다중 줄 `ScrollingTextGump`와 Markup/StyleTextMarkup이다. 이어서 MenuGump의 레이아웃/컨트롤/위젯/입력/Draw를 옮기며 `BindMenuLayoutStyleText`를 실제 항목 소유 필드에 연결한다. 한 줄 Draw 바인딩의 현행2인자 형태도 다중 줄 분류를 포함하는 원본3인자 형태로 확장한다. 실제 메뉴/전체 자산/GIF/호스트/Renderer 조립과 전체 미션/Edit/멀티플레이 사전 구현은 남는다.

cpppj·공통 인계·원본/CD·AGENTS.md는 변경하지 않았다. 게임/창·금지된 실제 재현·멀티플레이 검사를 하지 않았고 외부 TODO.md는 커밋 대상에서 제외한다.
