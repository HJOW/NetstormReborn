# dotnetpj R3k-3 — 실제 HTML 링크와 전용 도움말 본문

2026-10-29, HJOW-Athlon. AGENTS.md·현행/공통 인계와 현재/마지막 전체 정밀 디컴파일 PC(HJOW-Athlon, 2026-10-23)를 확인했다. 직전 목표 턴은 GIF/HTML2단위의 구현·검증·커밋을 완료한 진행이며 시작 HEAD는 `c58ddc1f`다. cpppj HtmlLinkText/QuickHelpHtml.h/.cpp·RawPriestMenu.h의 서식 인자 typedef·대응 검사·기존 HTML/StyleText/다중 줄을 읽었다. GameAssets용 TYPE/AST/요구 에너지 바인딩의 남은 의존성도 확인했다. 게임/창·PE 에뮬레이터·금지된 플레이·멀티플레이 검사는 실행하지 않았다.

## 1. 구현

링크는 같은 소유 BYTE 배열의 현재 커서에 원문 대상과 NUL을 붙이고 시작 토큰을 `~[T숫자]`로 돌려준다. 첫 인용 내부의 첫#만 생략하고 공백/대소문자/프로토콜은 바꾸지 않는다. 닫힘/인용 없음은 T0이며 비활성 링크는 T태그도 만들지 않는다. 커서를 되돌리면 접두부만 덮어쓰고 뒤의 기존 BYTE를 보존한다. 현재 물리 용량/유효 토큰/NUL이 없으면 명시 거부한다.

QuickHelp는 기본 상태 초기화→현재 TYPE 조회→shown0으로 생성한다. 소문자n에서만 name을 찾고 나머지 주제 비교는 ASCII 대소문자를 무시한 원문 접두 비교다. 선택되면 `~5~w`를 쓰며 현재 info 태그에서 별도 정보 HTML을 만든다. 닫힘 슬롯은 현재 커서 바로 앞 LF/CR만 textAllowed0으로 바꾼다. 파일 스트림은 다음 평문/다음 태그를 원문 BYTE 단위로 기록한다.

정보는 제목/원소/클래스/HP/거리/초당 피해/SP/마나 순서다. 각 TYPE 조회와 번역/치환/서식 효과의 시점을 유지하여 번역이나 HP 조회가 바꾼 현재 필드를 다음 읽기에 반영한다. 숫자는 기존 원문 signed64 절삭의 하위 DWORD를 사용하며 NaN은0이다. 마나 그림의 실제11문자/128프레임 표·점 건너뛰기·앞쪽 강조+11·signed 감김·원문250/256BYTE 경계를 옮겼다.

정보 HTML은 같은 기본 파서/단락 전역/링크 BYTE를 사용한다. 중첩 해석 전 infoMode0, 정상 반환 뒤1을 쓰고 부모 링크 커서를 복구한다. 중첩 해석 예외에는 모드/커서를 임의로 복구하지 않는다. paneText→scroll0→현재 pane flags 조회→필요 시 Push/재부착 순서도 유지한다. 생성 인자인 마나 표는 원문 값 전달처럼 복사한다.

## 2. 독립 원본 대조

읽기 전용 `quick-help-html-x86.tsv`83행과 `quick-help-html-nested-x86.tsv`3행을 Link했다. 최초부터 모두 일치했다. 앞 열의 JSON/설명 getter만 입력으로 쓰고 원본 결과는 비교에만 쓴다. 보호 `d/help.english`는 같은 읽기 전용 VFS에서 원시 BYTE로 읽는다. 원본 fixture를 수정/재생성하지 않았다.

기본/파생 HTML 출력·현재 커서/가림/단락/정보 모드·링크 원문 접두부·파일 스트림·pane 효과 순서를 전수 대조했다. 중첩3행은 설명 getter 안의 링크들이 같은 BYTE를 쓰고 다음 부모 링크가 같은 위치에 덮어쓰는 순서를 확인한다. 번역/현재 TYPE/고정%s·%d/pane/파일은 원본 관찰 때와 같은 명시 장치 경계이며 새로운 게임 상태를 계산하는 대체 모델이 아니다.

fixture는115개이며 새 SHA 접두부는 `348a5d91dce8`/`0e100af50604`다. reflected-commit은 `f95a8db2...` 유지다. 포트244행(L1 125/port119), 기준 `c58ddc1f`다. 링크/도움말4행은 위 파서·표시 효과 범위의 L1이며 RawPriestMenu.h1행은 **서식 인자 typedef만 옮긴 port**다. 사제 메뉴 본체는 구현 완료로 세지 않는다.

## 3. 실제 장치·현재 상태 검사

집중10개가 통과했다. 원본 전수2개 외8개는 원문 링크 대상/연속 NUL·토큰/되돌린 커서의 뒤쪽 보존, 비활성/물리 용량/포인터/NUL, 실제 HTML→다중 줄 조각/표시 복제/적중 메타데이터→링크 대상 조회·글리프 픽셀/guard/조각 해제, 생성 초기화와 주제 접두부, 번역/HP/paneText 후 현재 TYPE/pane 재조회, 중첩 모드와 정상 복구/예외 부분 상태, 마나/문자표/강조 감김/물리 경계, 필수 장치/지역 버퍼를 확인한다.

최초 실제 조립 검사에서 비활성 링크도 실제 helper로 연결해야 하는 두 입력을 고쳤다. 미연결 효과를 제품에서 무시하지 않았다. 링크 뒤 일반 글자의 적중은 그림자 여백과 줄 아래 정렬의 strict 위쪽 경계를 구별하도록 검사 좌표를 고쳤고 제외되는 정확한 위 경계도 따로 확인했다. 원본 기대값과 제품 동작은 바꾸지 않았다.

전체 **1,946개(Assets363+Core1030+Client553) 통과**, 실패/건너뜀0이다. 전체 Release 빌드도 경고/오류0·종료0이다. Git 제외 로그는 `extracted/dotnet-quick-help-html-native-initial-20261029.log`, `dotnet-quick-help-html-focused-initial-20261029.log`/`dotnet-quick-help-html-focused-final-20261029.log`/`dotnet-quick-help-html-focused-verified-20261029.log`, `dotnet-quick-help-html-tests-20261029.log`·`dotnet-quick-help-html-build-20261029.log`다. 실제 전체 메뉴/게임 캡처0·기존 코드/검사 은퇴0이며 D1 전체 화면 판정이 아니다.

## 4. 남은 연결

`ReadAssetQuickHelpType`의 전체 GameAssets TYPE/AST·requiredEnergy 후처리 바인딩은 아직 옮기지 않았다. 현재 Type/Description/HP/번역/Config/서식은 같은 실제 장치를 공급해야 하는 명시 경계다. 이 단위의 파서/출력 L1을 전체 자산/월드·도움말 창 완료로 확대하지 않는다.

다음은 HtmlBodyGump/QuickHelpWindow·HtmlWindowGump의 생성/배치·현재 다중 줄 자식/링크 버퍼 수명과 실제 메뉴 상위 소유/분배·전체 자산/설정/음향/호스트다. R4 이후 전체 미션/세션/월드/HUD/Edit·후순위 멀티플레이 사전 구현도 남는다. 전체 목표는 계속 진행 중이다. cpppj·원본/CD·AGENTS.md·공통 인계·외부 TODO.md는 수정하지 않았다.
