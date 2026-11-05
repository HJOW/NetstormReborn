# 6차 한국어 지원 — 한글 글꼴·번역표·언어 선택 (단위938)

2026-11-06, HJOW-Athlon. AGENTS.md 6차 요구("한국어 지원 작업")와 "한국어 글꼴은 `fonts`의 D2Coding, 새 코드의 주석은 한국어"의 구현이다. 마지막 정밀 디컴파일 호스트는 변하지 않았다(HJOW-Athlon, 2026-11-03). 원본 10.78 바이너리는 한국어 경로가 없으므로 **전부 클론 추가 기능**이며 원문 일곱 언어의 번호/이름/번역 경로는 그대로다.

## 설계 요약

원본은 모든 문자열이 8비트(Windows-1252) 글꼴 바이트이며 글리프 하나가 256칸 표의 한 칸이다. 한글(약 1만 1천 자)은 이 표에 들어갈 수 없으므로 **4바이트 확장 시퀀스**를 8비트 파이프라인 안에 실었다.

| 구성 | 내용 |
| --- | --- |
| 시퀀스 | 시작 바이트 `0x1f` + 숫자 3바이트(제어 바이트 25개를 25진 숫자로 쓴다). 25³=15,625칸에 한글 음절(0xAC00..0xD7A3)·호환 자모(0x3131..0x318E)·CJK 기호(0x3000..0x303F)·전각 ASCII(0xFF01..0xFF5E)를 순서대로 대응한다. 불완전/범위 밖 시퀀스는 일반 바이트로 남는다. |
| 부호화 | **표시 경계 전용** `o::EncodeDisplayText`가 UTF-8의 위 구간 문자를 시퀀스로, `DecodeDisplayText`가 역으로 되돌린다(왕복 검사 완료). Windows-1252에 없는 다른 문자는 기존처럼 거부한다. `EncodeOriginalText`/`DecodeOriginalText`는 **원래 계약 그대로**다(한글 거부, 시퀀스 불인식): 설정 저장의 UTF-8 BOM 정책, 유니코드 게임 경로, 파일/설정 읽기 경계가 이 거부에 의존하기 때문이다. 처음에는 원문용 함수에 직접 확장을 넣었다가 설정 저장/경로 검사 12개가 깨져 표시 전용 함수로 분리했다. 전환한 곳은 GUI 글꼴 바이트 경계 39줄(`UberGump` `FontBytes`·메뉴/도움말/Tell/채팅/캠페인 UI 번역, 캡처 알림 등)뿐이며 파일·설정·타입·번역 파일 경계는 그대로다. |
| 글리프 | `BitmapFont`/`FontStore`에 확장 글꼴(`SetExtendedFace("D2Coding", 파일)`)을 두었다. 한글 글리프는 요청할 때 GDI가 D2Coding으로 한 글자씩 그려 같은 256색 글리프 형식으로 캐시한다. D2Coding은 모노스페이스라 전진 폭 = 글꼴의 한 칸 폭(A+B+C)이고 라틴과 달리 잉크를 칸 안 제자리에 둔다. |
| 폭/그리기 | `GumpText`의 폭 표는 시작 바이트에 한 글자 폭, 숫자 바이트에 0을 준다. `GumpTextDraw`/`Renderer`에 확장 글리프 콜백을 두어 같은 색 표로 합성한다. |
| 번역표 | 현재 언어가 `korean`(번호 7)이면 `Client`가 `xlat.korean`(UTF-8)을 읽어 `o::XlatTable`로 쓰고, 키가 없으면 영어 원문이다. 10.78의 다중 장부(`RawXlat`)·CD 단일 표 경로는 건드리지 않는다. |
| 언어 번호 | `OriginalLanguageName`/`FindOriginalLanguage`에 7번 `korean`을 추가했다(`kKoreanLanguageNumber`). OS 언어로 자동 선택하지 않고 옵션 메뉴나 `currentLanguage=korean` 설정으로만 고른다. |
| 옵션 메뉴 | 옵션 팝업(메인 메뉴·게임 중 모두)에 **Language** 하위 메뉴(English / 한국어, 현재 언어 체크)를 추가했다. 선택하면 원문 SetLanguage 명령(0x542988→`kSetLanguageCommand`)으로 설정이 바뀌고 `LanguageChanged`가 큐에 간다. `GameMenusHooks::compatibilityLanguages`가 비어 있으면(기본) 원문 메뉴와 조회 순서가 그대로다(원문 918 관측 불변). |
| 시나리오 파일 대체 | 원본의 시나리오/안내/도움말 파일은 `{currentLanguage}` 확장자를 쓴다(`missionSpec={DataDir}\{local.1}.{currentLanguage}`, `guideSpec`, `userSpec`, `Quick Help Filename`, 용어 파일 `config.<언어>`). 한국어 본문이 없는 지금은 `.korean`이 하나도 없어 미션이 "Missing mission"으로 죽는 것을 **실제 실행에서 확인**했다. 그래서 `BaseFileSystem`이 읽기(`TryRead`)·존재 조회(`Probe`)·읽기 모드 `Open`·와일드카드 나열(`Match`/`MatchFinder`/`MatchDisk`)에서 `.korean`이 없으면 같은 이름의 `.english`로 대신한다(한국어 본문이 있으면 그 파일이 우선, 쓰기/생성은 대체하지 않음). 원문 `.english`/`.german` 등 다른 이름의 경로는 그대로다. |
| 데이터 탐색 | `client/KoreanResources`: 실행 파일 폴더·게임 폴더와 그 상위 6단계에서 `fonts/D2Coding*.ttc`(이름순 마지막=최신)와 `lang/xlat.korean` 또는 `assets/lang/korean/xlat.korean`을 찾는다. 배포판은 exe 옆 `fonts\`·`lang\`, 개발은 저장소 루트다. |

## 번역표 `assets/lang/korean/xlat.korean`

- 원본 `xlat.german`의 고유 영어 키 774개 중 **게임 안에서 쓰이는 502개**를 번역했다. 설치 프로그램/진단 화면 문구(약 150개)와 내부 타입 이름(`dude`, `sunArcher` …)은 번역하지 않았다(없는 키는 영어 원문).
- 서식 코드(`%s %d %2.1f`, `~색상`, `~[아이콘]`, `<c>`, `{Focus}`, `$ButtonInstant=이름,명령,번호`)는 영어 키와 **같은 순서/개수**를 유지한다(생성 스크립트가 전수 검증했다). `~E~Select Server`의 `~S`는 원문에서 글자를 삼키는 오기라 한국어는 `~E서버 선택`으로 `~`를 뺐다.
- 건물/유닛 이름은 AGENTS.md 명칭 규칙과 공유 표 `assets/unit-names.korean.json`을 따른다. 사제·신전·다리·발사기·간헐천·제단·거주 구역·고갈된은 번역하고, 나머지는 발음 표기(워크샾·제네레이터·훨리베이스·훨리기그·반더 타워 …). 검사가 공유 표와 번역표의 일치를 확인한다.
- 클론이 새로 쓰는 문구 `Language`(언어)와 원문 옵션 메뉴의 `Resolution`(해상도)을 추가했다.

## 확인

- 단위 검사 `netstorm_korean_text_tests` 9개(부호화 왕복/글리프 폭/FontStore/번역표 파싱·모든 줄 왕복·공유 명칭표 일치/글꼴 탐색/언어 번호/Client 번역 전환)와 `netstorm_main_popup_tests`의 새 `GameMenus_OptionsLanguageSubmenuOnlyWithCompatibilityLanguages`가 통과했다(`main_popup`의 기존 실패 1개는 단위933 기록과 같은 `CampaignSaveGoMain_*`).
- 실제 실행(창 모드·MCP 화면 PNG): `--set currentLanguage=korean`에서 옵션 팝업이 한글로 나오고(Direct Draw / 전체 화면, 해상도, 언어, 소리 켜기, 음악 재생, 바람 소리, 스피커 좌/우 교체, 효과음/음악 음량, 가장자리 스크롤, 자동 데모, 시작 시 팁 표시) Language 하위 메뉴에서 한국어가 체크된다. 영어로 시작해 Language > 한국어를 고르면 즉시 바뀌어 옵션을 다시 열면 한글이다.
- 한국어에서 미션 진입(`--mission Battle1`)이 영어 브리핑 창과 함께 정상 시작한다(대체 전에는 "Missing mission: Battle1"과 `unable to open: \D\config.korean`으로 종료). 단위 검사 `KoreanFallback_MissingKoreanFilesUseTheEnglishFile`이 읽기/존재 조회/와일드카드/한국어 우선/다른 확장자 불변을 확인한다.
- 배포: `package-cpppj.ps1`이 최신 D2Coding `.ttc`·`License.txt`·`lang\xlat.korean`을 실행 파일 옆에 넣는다(1376→1379항목). 만든 압축을 저장소 밖에 풀어 한국어 옵션 메뉴가 뜨는 것을 확인했다. 안내문의 설정 위치도 5차의 `%USERPROFILE%\Saved Games\Netstorm\`로 고쳤다.

## 알려진 한계·후속

- **메인 메뉴 버튼 그림**(Campaign/Multiplayer/Demo/Help/Edit/Credits/Options/Quit)은 원본이 GIF 그림이라 번역되지 않는다. 한글 버튼 그림은 후속이다.
- **시작 팁 창("Did You Know?")·도움말 HTML·미션 브리핑/시나리오(`d/*.english`)는 한국어 본문이 없어 영어로 표시된다.** 각 언어 파일은 `<이름>.<언어>` 구조라 `.korean` 본문을 따로 작성하면(AGENTS.md의 english→언어 확장자 규칙) 위 대체 규칙에 따라 자동으로 우선 사용된다. 단 `.english`에는 AI/시작 SP 등 시나리오 구성도 들어 있으므로 번역본은 구성 부분을 그대로 복사해야 한다. 시작 팁 창도 영어 본문으로 나온다(번역표에 팁 문구 키가 없다).
- 사용자 보이는 한국어 문구의 번역 품질(경어/용어 통일)은 사용자 확인이 필요하다. 번역표 키/값은 UTF-8이며 같은 형식으로 고칠 수 있다.
- 글꼴 모양: D2Coding을 GDI로 그려 8비트 글리프로 만들므로 라틴 글꼴(Arial)보다 약간 가늘다. 글꼴 크기/굵기 비율은 글꼴 슬롯 높이를 따른다.
- 한글 입력(IME)은 이번 범위가 아니다. 플레이어 이름 등 한글을 직접 치는 기능은 후속이며 `EncodeOriginalText` 확장 시퀀스는 한글 이름 저장에도 쓸 수 있다.
