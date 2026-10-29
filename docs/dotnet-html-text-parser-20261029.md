# dotnetpj R3k-2 — HTML 본문과 현재 조건/그림/배경

2026-10-29, HJOW-Athlon. AGENTS.md·현행/공통 인계와 현재/마지막 전체 정밀 디컴파일 PC(HJOW-Athlon, 2026-10-23)를 확인했다. 배경 GIF 단위 `fb0f7378`(DN51)을 완료한 뒤 같은 목표를 이어갔다. cpppj HtmlTextParser.h/.cpp·HtmlConditionEffect.h/.cpp·RawTextToken.h/.cpp 및 대응 검사를 읽었다. ConfigInterface.SubstituteLocal/ConfigParseLong과 기존 C# ConfigStore/ConfigNumbers·StyleText/Markup/글꼴/픽셀도 대조했다. 게임/창·PE 에뮬레이터·금지된 플레이 재현·멀티플레이 검사는 실행하지 않았다.

## 1. 구현

HTML은 일반 웹 파서로 대체하지 않고 원본 BYTE 순회를 옮겼다. 첫 태그 BYTE만 signed ASCII 대소문자로 분기하며 둘째/셋째 BYTE는 원문 literal과 비교한다. 태그 안/닫힘 slash 이동 뒤/꺾쇠 닫힘 뒤의 세 가상 슬롯과 콜백 뒤 현재 커서·출력·shown/depth를 다시 읽는다. `<a`의 소문자 여부와 textAllowed 증감, LF의 조건부 공백, 제목 닫힘 뒤 다음 P의 빈 줄, 단락 억제 DWORD도 유지한다.

평문은 출력 길이가 capacity-256보다 커진 뒤 중단한다. 태그 출력은 같은 논리 한도를 쓰지 않지만 실제 NUL 공간을 포함한 물리 용량을 넘길 수 없다. Parse는 입력/출력·afterHeading·inTag/textAllowed를 초기화하고 shown/hiddenDepth와 장기 전역을 유지한다. 입력과 출력은 원본 BYTE를 Latin1 문자열로 일대일 보존한다. 마지막 LF는 반환에 포함하고 종료 NUL은 길이에서 제외한다.

그림 helper는 첫 따옴표 내부를 `~[I...]`로 옮기며 꺾쇠/따옴표/첫 NUL에서 끝낸다. 자산 이름/프레임 해석과 소유는 실제 StyleTextMarkup/Markup에 맡긴다. 배경 helper는 같은80BYTE 전역에 이름/종료0만 쓰고 뒤쪽 기존 BYTE를 보존한다. 다른 파서/다음 페이지/숨긴 태그도 같은 전역을 빌린다. 배경 이름을 실제 보호 titleMenu GIF 저장소로 읽는 연결도 검사했다.

조건은 원문 연산자를 BYTE 단위로 읽는다. PATCH는 boolean/문자열 동등/g/ge/l/le와 앞/연산자 뒤 부정의 OR 규칙, CD는 boolean 규칙을 쓴다. 두 항은 오른쪽 치환→오른쪽 atol→현재 왼쪽 치환→왼쪽 atol 순서다. 숨긴 열림/닫힘은 설정을 추가 조회하지 않는다. 출력100BYTE 경계를 유지하고 원본 메모리 오버런을 성공으로 처리하지 않는다.

공통 `RawTextToken`은 Core.Original에 옮겼다. 앞 space/tab만 건너뛰고 인용부호도78입력 BYTE 제한을 소비한다. 한도 때문에 멈추어도 현재 연속 구분자를 건너뛰고 뒤 공백을 지워도 첫 출력 BYTE는 남긴다. 숫자는 cpppj와 같은 감김 atol을 이미 가진 ConfigNumbers를 공유한다.

## 2. 독립 원본 대조

읽기 전용 fixture4개를 Link했으며 기대값은 바꾸거나 재생성하지 않았다.

| 자료 | 독립 관찰 수 | 비교 범위 |
| --- | --- | --- |
| htmltext-x86.tsv | 816 | 두 PE의 기본 HTML 출력 BYTE·커서·shown/depth/inTag/textAllowed/afterHeading/P전역 |
| htmlpicture-x86.tsv | 276 | 두 PE의 실제 그림 helper/전체 순회와 같은 전체 상태 |
| html-background-x86.tsv | 648 | PATCH의 출력/상태와 배경80BYTE 전체 |
| htmlcondition-x86.tsv | 754 | 두 PE의 직접 태그/전체 HTML·실제 토큰/숫자/현재 Config 치환 입력·결과/호출 순서와 전체 상태 |

총2,494행이 일치했다. 처음 전수 실행에서 기본/그림/배경3개는 통과했고 조건 검사는 검사 코드의 동작 이름을 `Parse`로 잘못 적어 실제 `Html` 행에서 중단됐다. 입력 분류를 바로잡은 뒤 조건754행도 통과했다. 제품/fixture 기대값 수정은 없다. 조건의 앞7열은 실행 입력과 매 호출 현재 설정 계획으로만 쓰고 뒤9열은 비교에만 쓴다. 한 번의 설정 사본으로 양쪽 피연산자 기대값을 만들지 않는다.

fixture는113개, 새 SHA 접두부는 위 순서대로 `ae9353f6629c`/`123b57dcc779`/`44491cd8ae6b`/`9e65562b4903`이다. reflected-commit은 `f95a8db2...` 유지다. 포트239행(L1 121/port118)이며 새6행의 기준은 `fb0f7378`다. RawTextToken은 별도 원본 fixture를 새로 생성하지 않고 실제 조건754행의 원문/결과 trace와 파서 상태로 함께 대조했다. 기존 ConfigStore/ConfigNumbers 제품과 포트 기준은 바꾸지 않았다.

## 3. 실제 연결과 검증

집중12개가 통과했다. 원본 전수4개 외8개는 장기 배경/실제 GIF 소유, 선택 효과의 명시 경계와 현재 조건/명령 원문, 파생 가상 순서/커서 이동, 오른쪽 조회 뒤 현재 설정 변경/숨김/닫힘 무조회, 실제 Config 이름 범위/기본값/현재 값, 실제 StyleText의 본문/색/굵기와 그림 생성/반납·글리프 픽셀/guard, 공통 토큰78BYTE/인용/구분자/NUL, strict 논리 여유/콜백 뒤 물리 포인터 오류를 확인한다.

검사 전용 글꼴에는 Bold 캐시가 없으므로 픽셀 연결은 실제 공급한 Normal 캐시와 HTML i의 색상 제어로 대조한다. Bold는 생성된 실제 문맥의 style2로 별도 확인한다. 미공급 글꼴 대체를 제품에 추가하지 않았다. 검사 코드의 xUnit 분석 경고2개도 고쳤다.

전체 **1,936개(Assets363+Core1030+Client543) 통과**, 실패/건너뜀0이다. 최종 전체 Release 빌드는 경고/오류0·종료0이다. Git 제외 로그는 `extracted/dotnet-html-parser-native-initial-20261029.log`/`dotnet-html-parser-native-fixed-20261029.log`, `dotnet-html-parser-focused-initial-20261029.log`/`dotnet-html-parser-focused-final-20261029.log`, `dotnet-html-parser-tests-20261029.log`·`dotnet-html-parser-build-20261029.log`다. 실제 전체 메뉴/게임 캡처0·기존 코드/검사 은퇴0이며 전체 화면 D1이나 실제 미션 플레이 완료 판정은 아니다.

## 4. 남은 연결

명령 큐·링크 저장소/동작·TextHtml/QuickHelp 파생 슬롯은 실제 분기에서 명시 공급해야 한다. 기본 VT의 검증된 noop만 기본 제공한다. ConfigStore 바인딩은 같은 현재 설정 치환을 매번 수행하지만 전체 ConfigInterface의 임시 local 객체 생성/반납·이름 객체 소유 복원은 상위 조립에 남는다.

다음은 HtmlLinkText/QuickHelpHtml·HtmlBodyGump/HtmlWindowGump와 창 생성/레이아웃·실제 메뉴 상위 소유/분배·전체 자산/설정/음향/호스트다. R4 이후 전체 미션/세션/월드/HUD/Edit·후순위 멀티플레이 사전 구현도 남는다. 전체 목표는 계속 진행 중이다. cpppj·원본/CD·AGENTS.md·공통 인계·외부 TODO.md는 수정하지 않았다.
