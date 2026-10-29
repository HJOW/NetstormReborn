# dotnetpj R3k-4 — HTML 부모와 내장 Tell의 실제 자식/버퍼 수명

2026-10-29, HJOW-Athlon. 같은 목표에서 QuickHelp 단위 `ecb9802a`(DN53)를 완료한 뒤 진행했다. AGENTS.md·현행/공통 인계와 현재/마지막 전체 정밀 디컴파일 PC(HJOW-Athlon, 2026-10-23)를 확인했다. cpppj HtmlBodyGump/EmbeddedTell.h/.cpp·EmbeddedTellTests.cpp·QuickHelpWindowTests.cpp의 부모 연결, RawMenuControls.cpp의 RenderEmbeddedTell와 기존 실제 다중 줄 생성/버퍼/입력/수명을 읽었다. 게임/창·PE 에뮬레이터·금지된 플레이·멀티플레이 검사는 실행하지 않았다.

## 1. 구현

HTML 부모는 같은 기본 노드 뒤의 표시 자식 토큰과 본문/링크 두8192BYTE 배열을 소유한다. 생성은 두 첫 NUL만 쓰고 나머지 BYTE는 보존하며 signed 화면 크기를 정수로 나눈 절반 크기를 쓴다. Clear는 현재 자식을 지연 Kill한 뒤 소유 토큰0만 쓴다. Parse는 두 첫 NUL→현재 출력 유무→필요 시 같은 배열 Bind→실제 순서다.

SetText는 이전 자식을 Kill하되 새 생성 성공 전까지 그 토큰을 남긴다. new0xd8→다중 줄(0,0,5,color) 생성→현재 자식 설치→가상 부착→styled 본문→절대 scroll0→Slider/scrollable1→현재 부모 재부착 순서를 유지한다. 각 효과 뒤 현재 자식/플래그/좌표를 다시 읽는다. Fit는 자식 Push 뒤 현재 부모/자식의 기본 치수를 읽고 동일 x87 계산값에서 기본/배율 치수를 각각 저장한다. 기본 부착 뒤 Fit(0)도 실제 가상 트리/다중 줄 장치에 연결했다.

내장 Tell은 현재 Mission 섹션을 먼저 조회하고, 없으면 매번 새 Tell 파일 섹션을 읽는다. 존재하는 빈 본문도 실제 HTML 부모를 만든다. 두 조회가 실패하면 경고/0이며 이름의 첫 점·BLANK·팝업 이력/param에 별도 의미를 추가하지 않는다. 부모 생성/SetHtml 뒤 현재 serial을 다시 읽는다. ConfigStore 바인딩은 현재 missionSpec을 치환하고 임시 ConfigText에서 새 섹션을 읽으며 기존 설정 층/이름 범위를 바꾸지 않는다. 좁은 CP1252 키와 Config 내부 UTF-8 본문 BYTE를 HTML의 Latin1 표현 사이에서 구별한다.

`HtmlBodyTextParser`는 RawMenuControls의 RenderEmbeddedTell 중 **현재 파서 게시 경계만** 옮겼다. 기본 HTML/동일 링크 저장소를 부모의 실제 두 배열에 연결한다. 명령/조건/그림/배경/링크 효과 앞뒤 및 파싱 오류 경계에서 현재 본문 접두부/NUL을 실제 부모에 게시한다. 링크 helper 직전에 같은 물리 배열을 다시 읽어 외부 효과의 변경을 보존한다. 실패 앞의 링크/본문·전역 효과와 표시 자식0을 남기며 파싱 성공으로 위장하지 않는다.

## 2. 독립 원본 대조

읽기 전용 `embedded-tell-x86.tsv`82행을 Link했다. 두 x87 제어모드의 내장 조회/생성·HtmlBody Construct/SetHtml 경로에서 반환 serial·flags/vtable/현재 자식 유무·본문/링크 전체8192BYTE FNV32·가상 효과 순서가 최초부터 일치했다. 앞5열만 입력으로 쓰고 뒤 결과는 비교에만 쓴다. 명시 파서 장치의 출력만 공급하며 기대 지문을 제품에서 만들지 않는다.

모드/명령 수는 관찰 메타데이터이며 관리 코드의 실제 x87/ABI 일치 판정은 아니다. Clear/Parse/Fit/기본 부착 및 Config/실제 파서 게시기는 이 fixture의 독립 전수 범위와 구별해 정적 계약/실제 장치로 확인한다. 원본 기대값을 수정/재생성하지 않았다.

fixture116개·새 목록 행 수83(머리 포함)/SHA 접두부 `a3b066240637`, reflected-commit은 `f95a8db2...` 유지다. 포트249행(L1 129/port120), 새 기준 `ecb9802a`다. 부모/내장 조회4행은 위 관찰 범위 L1, 부분 게시기1행은 port다. RawMenuControls 전체 이식으로 세지 않는다.

## 3. 실제 장치 검사

집중9개가 최초부터 통과했다. 원본 전수1개 외8개는 두 첫 NUL/미작성 뒤쪽/signed 화면 절반, 기존 외부 출력/현재 Bind 순서, 생성 실패의 이전 토큰/효과 뒤 현재 자식, Push 뒤 현재 자식/16777217 중간값과 float32 저장, 실제 부모/소유 버퍼/스크롤/Slider/보호 글꼴/글리프/링크 적중과 부모 통지/지연 반납, 실제 Config 미션 우선/빈 성공/매번 새 파일·외부 param 생존, CP1252 키/UTF-8 본문 BYTE, 명령 효과 앞 현재 접두부/예외의 부분 본문·링크/전역과 후속 성공을 확인했다.

실제 UI 연결은 같은 공통 등록/트리/수명·다중 줄/Slider/마크업을 사용한다. 글꼴은 보호 `.chfnt`를 읽고 검사 중 GDI 생성은 요구하지 않았다. 전체 GameAssets/상위 메뉴를 새로 추정한 장면이 아니며 공통 Slider 쪽의 기존 명시 자산 입력을 빌린다. 링크 클릭이 실제 T태그를 부모에게 보내며 Clear/Cleanup 뒤 표시 자식/버퍼/조각이 반납되고 부모 삭제 뒤 루트만 남는다.

전체 **1,955개(Assets363+Core1030+Client562) 통과**, 실패/건너뜀0이다. 전체 Release 빌드도 경고/오류0·종료0이다. Git 제외 로그는 `extracted/dotnet-html-body-native-initial-20261029.log`·`dotnet-html-body-focused-initial-20261029.log`·`dotnet-html-body-tests-20261029.log`·`dotnet-html-body-build-20261029.log`다. 실제 전체 메뉴/게임 캡처0·기존 코드/검사 은퇴0이며 D1 전체 화면이나 실제 미션 플레이 완료 판정이 아니다.

## 4. 남은 연결

ConfigStore 바인딩은 현재 파일/섹션/설정 층의 실제 연결이다. 전체 ConfigInterface의 등록 객체 목록·동명 객체 복구/임시 객체 등록 관찰은 아직 옮기지 않았으므로 C++ 등록부 전체 수명 일치로 확대하지 않는다. 전체 RawMenuControls 노드/창 소유자도 남는다.

다음은 QuickHelpWindow의 전체 생성/배치/요청·이력/입력과 HtmlWindowGump/Factory/Metadata/Fit·실제 메뉴 상위 소유/분배·전체 자산/설정/음향/호스트다. R4 이후 전체 미션/세션/월드/HUD/Edit·후순위 멀티플레이 사전 구현은 계속 남는다. 전체 목표는 진행 중이다. cpppj·원본/CD·AGENTS.md·공통 인계·외부 TODO.md는 수정하지 않았다.
