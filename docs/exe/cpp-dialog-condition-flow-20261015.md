# 실제 Tell 조건·두 HTML 순회 연결

현재 PC와 마지막 정밀 디컴파일 PC는 **VM-W11-Codex, 2026-10-15**다. 이 단위는 이미 독립 검증된 `HtmlConditionEffect`를 실제 `UberGump::TryTell` 페이지 경로에 연결한다. 원본 게임 실행과 새 독립 native 관찰은0이며 기존754조건/816기본HTML/276그림 관찰을 새 입력 수로 합산하지 않는다. 실제 게임 완주·전체 UI/픽셀 동일성 완료를 뜻하지 않는다.

## 원본 순서

PATCH `004cf270`의 순서는 다음과 같다. 정밀 원문은 `extracted/refined/originals/Netstorm.c`의 해당 함수와 `004c83b0`, `004c8d50`이다.

1. `00441660(...,source,8192,0,0,0)`으로 **전체 Config 치환을 먼저** 수행한다. 이를 조건 효과 뒤로 옮기지 않는다.
2. `0046bea0`으로 기본 HTML 객체를 만들고 출력/입력을 설정한 뒤 `0046c810`으로 첫 순회를 실행한다. 조건은 이때 토큰마다 현재 Config를 다시 치환하며, inline 명령은 이때 실제 명령 큐에 예약된다.
3. `004c8d50`은 첫 출력 바이트에 `_isalnum((int)signedChar)`이 있는지 검사한다. alnum이 없으면 본문 창을 만들지 않고 명령 효과만 남는다. StyleText 제어의 영문 바이트도 이 검사에 포함된다.
4. `004c83b0`이 첫 출력의 메타를 제거하며 컨트롤을 만든다. 기본 버튼 보충도 이 사이에 있다.
5. 첫 출력의 LF를 `00504b0c`→`00515eb8`, 즉 `\n`→`<br>`로 바꾼다.
6. **같은 HTML 객체**에 새 출력/입력을 설정하여 두 번째 `0046c810`을 실행한다. 따라서 `shown/hiddenDepth`는 유지되고 출력 설정이 지우는 `afterHeading` 등만 기존 parser 계약에 따라 초기화된다. 끝에 LF를 보장하고 실제 ScrollingTextGumpMarkup으로 전달한다.

처음 전체 치환과 조건 토큰의 현재 Config 재치환은 모두 존재한다. 전체 치환을 제거하거나 첫 결과만으로 boolean을 미리 계산하면 원본 순서를 바꾼다. 닫히지 않은 false 조건의 경우 첫 출력에 있던 앞 평문조차 두 번째 순회에서 숨겨질 수 있다. 정상 중첩 HTML로 고치거나 두 번째 순회마다 새 shown=1 상태로 시작하지 않는다.

## 연결과 소유권

`ParseDialogPage(source,state,globals,hooks,edition)`의 source는 선행 전체 치환 결과다. 이 overload는 첫 HTML 효과를 실행하고 메타를 추출한 뒤 두 번째 HTML 입력인 `DialogPage::htmlBody`를 반환한다. `UberGump::TryTell`은 같은 state/globals/hooks를 `DialogBodyLayout::SetBody`에 넘긴다. 기존 실제 StyleTextBuffer/폰트/SHP/본문 클립과 Compose 캐시는 유지하며 Compose마다 HTML 효과를 다시 실행하지 않는다.

실제 조건은 `BindHtmlConditionConfig`가 각 선택 시 기존 ConfigInterface를 조회한다. inline 명령은 기존 `QueueScriptCommand`와 동일한 실제 InputQueue/ScriptStringRing을 사용하며 첫 순회의 선택 시점에 예약한다. 명령 실행은 그 입력 큐의 처리 시점이다. `commands/warnings` 벡터는 보고용으로도 보관되지만 이를 다시 순회하여 명령을 중복 예약하지 않는다. Background는 선택된 실제 이름을 페이지에 보관하고 그림은 기존 `BindHtmlPictureText`/실제 자산 경로를 유지한다.

한 인자 `ParseDialogPage(source)`와 `FilterDialogConditions`는 기존 독립 검사의 원문 호환용으로만 남았다. 실제 Tell이 호출하는 경로는 새 overload다. 이 구형 검사 통과를 실제 현재 Config 조건 연결의 근거로 세지 않는다.

메타는 원본의 대소문자를 유지한9패턴과 종류 우선순위 Button→ButtonInstant→Menu→Checked→Timeout→OnExit→Key→Input→Zsend를 사용한다. 매 제거 뒤 다시 Button부터 찾는다. 마지막 캡처는 `[^$ ]+`라서 첫 HTML이 LF를 space로 바꿔도 마지막 동작 인자가 뒤 평문까지 늘어나지 않는다. **삭제 범위는 캡처 끝과 다르다.** 원본 `004c8bbd` 주변처럼 일치 시작부터 다음 LF(포함), 다음 $(제외), 또는 NUL까지 지운다. 따라서 메타 뒤에 놓인 평문이 첫 순회에서 space로 이어졌다면 원본처럼 다음 $까지 삭제될 수 있다. 이를 임의 보존하지 않는다. 문서 확인 중 처음 작성했던 match 길이만 삭제하는 구현과 해당 검사 기대를 최종 검증 전 원본 범위로 교정했다.

기존6종 페이지 항목/시간제한 동작을 사용한다. Key/Input/Zsend의 실제 입력 컨트롤은 아직 공급되지 않아 선택되면 명시 오류다. 실제 링크 효과도 계속 미공급 오류이며 빈 성공을 넣지 않는다. 메타 명령 ID/인자 주소 변환·원본 regex 엔진 내부·캡처80/256바이트 복사 상한·각 파생 컨트롤의 전체 수명을 이번 단위에서 독립 복원했다고 주장하지 않는다. 기본 OK 보충은 기존 클론 동작을 유지하며 원본 Config의 OkButton 전체 연결은 별도 편차다.

명령의 보고용 문자열은 따옴표 안 `>`를 보관하지만 실제 HTML cursor는 기존 기본 parser처럼 **첫 `>`에서** 태그를 끝낸다. 실제 큐 hook에는 `$` 뒤부터 NUL까지의 전체 remainder를 넘긴다. 인용 문자열을 이유로 기본 HTML cursor를 태그 끝까지 옮기지 않는다. 구형 한 인자 parser의 quote-aware 원문 추출과 이 차이를 검사에서 명시한다.

## 검증 범위와 인수인계

새 `DialogConditionFlowTests.cpp`의6개 집중 검사는 선행 전체 치환 뒤 Config 변경, 현재 condition binder, 숨긴 명령/배경/버튼, inline색/줄바꿈의 두 순회 보존, 명령 한 번 예약, false shown/depth의 두 번째 순회 유지, 메타 종류 우선순위/캡처와 삭제 범위 차이, 명령뿐인 페이지, 미공급 효과/입력 컨트롤 거부, 인용 `>`와 실제 HTML cursor를 검사한다. 기존 GUI용 controls/본문 검사도 실제 Tell을 지나므로 부모가 Release 빌드 뒤 함께 실행한다.

제품/검사를 등록한 **Release 게임/검사 빌드 경고0·오류0**가 통과했다(`extracted/pause-final-build.log`). 최초 집중은 앞선 병행 편집에서 남은 메타 match 길이 삭제 object 때문에 경계 검사1개가 실패했다. 실제 소스의 LF/$/NUL 삭제 범위를 강제 재컴파일한 뒤 **새 집중6개 실패0·빌드 경고0·오류0**가 통과했다(`extracted/pause-metadata-final-build.log`, `extracted/pause-metadata-final-focused.log`). 임시 중간 출력은 제거했다. 앞 단위 조건754행의 전체1,218개를 이 연결 뒤 결과로 대신 적지 않는다.

최종 실행 파일의 실제 클론 GUI는 기존 controls **97상태**, Early1/2 A/B/C 안내·MORE/BACK/OK/다음 입력 Warning **23상태**, 완료 Config/다음 Tutorial/옵션 저장 **9상태**를 통과했다. 자료는 `extracted/pause-settings-final-backup/result.json`과 같은 폴더의 각 도구 stdout/stderr·실제 설정 백업·원본 전체 SHA다. 첫 본문 검사에서 허용 설정파일 열기/복구가 일시 OSError22로 실패했으나 검사 전 깨끗했던 두 파일을 원상복구하고 별도 백업 아래 재검증했다. 최종 originals 전체 파일명/SHA와 허용 설정 복구를 확인했다. 실제 게임플레이 완주로 세지 않는다. 원본 게임은 실행하지 않았다. 전체 CTest 최종 결과는 중단 인수인계에 기록한다.

최종 전체 CTest는 **1/1·내부1,233개·실패0, 112.41초**로 통과했다(`extracted/pause-final-ctest.log`). 새6개를 포함하며 기존 조건4/HTML6/본문7/ScriptCommand·Warning·메뉴 회귀도 같은 실행에서 확인했다.

`hasContent`는 현재 영어 스크립트용 ASCII alnum 관문을 사용한다. 원본의 signed high byte에 대한 활성 CRT locale/table 전체 동작과 한국어만 있는 본문은 이 관문의 동일성 범위에 포함하지 않는다. 기존 `{cur.file}` 보호/복구는 유지했다. Tell 인자의 named param 생성/수명, 파생 Help/링크/소리 효과, 미복원 컨트롤·메타 주소/상한과 전체 픽셀 동일성은 별도 미완료다. 사용자 중단 요청에 따라 추가 작업 없이 대기한다.
