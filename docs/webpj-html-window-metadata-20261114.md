# webpj HTML 메타 처리 (WB04-E458, 2026-11-14)

기준선 `1f00a25a`의 `HtmlWindowMetadata.cpp/.h`를 `www/ui/html-window-metadata.js`로 이식했다. 기존 `html-text.js` 부분 대응은 실제 공통 부모를 사용하는 새 모듈로 교체했다. 기존 앱의 TellWindow 경로는 허브가 조립될 때 교체한다.

## 원문 버퍼와 현재 장치

고정 아홉 패턴의 종류 우선순위는 본문 위치보다 앞선다. Button/Instant/Menu/Checked/Timeout/OnExit/Key/Input/Zsend를 처리하고, 제거 또는 치환 뒤 처음 패턴부터 다시 검색한다. 실제 버튼 정리와 생성은 같은 `HtmlWindowGump`를 호출한다. 명령 조회·진단·숫자/문자열 고리는 기존 `resolveDialogExitCommand`를 공유한다.

캡처 길이와 원문 위치를 유지하고, 이미 복사한 label/command/argument와 lookup 뒤 읽는 Checked의 두 캡처를 구별했다. 정규식의 `d` 위치 정보로 후속 캡처는 같은 버퍼의 현재 BYTE에서 읽는다. 삭제도 원래 캡처 끝 대신 하위 효과 뒤의 다음 LF/$/NUL까지 수행하며, `memmove`와 같은 제자리 이동으로 물리 꼬리를 보존한다. 지도 목록의 열수 인자는 같은 부모 필드의 get/set 참조다. Timeout은 시계 뒤 현재 부모에 기록하며 Input 뒤의 닫힘 사건도 현재 값을 조회한다. 누락한 선택 장치를 빈 성공으로 대체하지 않는다.

## 기준선 뒤 NB08 판정

기준선 뒤 변경은 `4edec13c5` 한 건이다. `%` 목록 삽입의 `repairInlineList`는 User Made Campaigns에만 한정된 함수가 아니라 **공통 메타 처리 옵션**이므로 **② 반영**했다(base `4edec13c`). 기본 false는 원본 관측과 같은 부분 복사를 유지한다. true면 목록을 본문 전체에 삽입하고 물리 용량-1에서 잘라 마지막 NUL을 쓴다. 현행 C++ 제품은 `RawMenuHtml.inc:25`에서 true로 켠다. 웹 제품에서 켜는 소비자와 실제 목록은 RawMenuControls 허브에서 연결한다. 이 옵션 이식으로 커스텀 맵/캠페인 기능이나 후순위10차의 전체 버그 수정 완료를 주장하지 않는다.

## 검증

신규 집중 **8건 첫 실행 통과**(메타6·E450 이월 통합2). `cpppj/tests/fixtures/html-window-metadata-x86.tsv` **원본10.78 128행**의 현재9필드·제어20행·전체8192BYTE FNV64·남은 본문·고리 문자열·반환/중단·외부 호출 순서가 모두 일치했다. manifest `html-window-metadata`로 연결했다. 이 관측은 원본 상위 메타/버튼 몸체를 실행하고 regex compile/search는 명시 하위로 둔 자료다. 웹은 고정 ECMAScript 패턴으로 검색하며 같은 입력 결과를 대조했다. 새 PE 실행이나 fixture 생성은 없다.

NB08 검사는 동일한 본문 뒤 `%Campaigns` 목록에서 false면 버튼이 사라지고 true면 실제 HtmlWindowGump의 Play 버튼이 생성됨을 재현했다. 수정 모드의 긴 목록 NUL/절단과 원문 모드의 과대 복사 오류 뒤 제거 효과도 확인했다. 별도 입력은 lookup이 같은 버퍼의 후속 Checked 캡처를 바꾸는 경우, 실제 ScriptStringRing과 부모 열수 참조, callback 표 값 복사, 버퍼/지역 캡처 한계와 앞선 버튼 정리다.

E450에서 미룬 C++ `TechnologyListMenu_HtmlMetadataThemeDispatch`/`AllDispatch`도 이번 단위에서 완료했다. 실제 metadata가 packed 원소/대상을 같은 기존 TechnologyListMenu로 분배해 부모 열수/남은본문·조회순서와 업그레이드/SP 보상 행까지 확인했다. 이 두 건을 허브 단계로 다시 미루지 않는다.

전체 회귀는 Node v26.13.0·v22.23.3에서 각각 **4,739건 통과, 실패0**, 대응표984행 오류0이다. Node v24는 이 PC에 없어 실행하지 못했다. 실제 메뉴/지도·파일·기술 목록 소비자·벽시각/입력/송신·공장과 원본 글꼴 픽셀은 RawMenuControls/Client 허브에서 연결한다. 보호 원본/디컴파일/cpppj 파일을 수정하거나 게임/브라우저를 실행하지 않았다. 실제 미션 플레이는 아직 불가다.

다음은 HtmlWindowFactory의 두 번 파싱/전체 생성·공유 serial을 이식하고, TellSection·HtmlTextParser partial을 마무리한다.
