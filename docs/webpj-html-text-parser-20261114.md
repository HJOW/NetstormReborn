# webpj HTML 기본 본문 변환기 (WB04-E460, 2026-11-14)

기준선 `1f00a25a`의 `HtmlTextParser.cpp/.h`를 `www/ui/html-text-parser.js`로 옮겼다. 원본 HtmlGump의 HTML 유사 본문을 StyleText 제어문자열로 바꾸는 기본 변환기(PATCH `0046bea0/bed0/bf00/c090/c810`)이며, 한 글자가 한 BYTE인 원문 BYTE 문자열을 다룬다. CD 10.72의 H 글꼴 차이(판본 인자)는 웹 범위 밖이라 옮기지 않았다.

## 이식한 것

- `HtmlTextParser` 클래스(`parse(source, capacity = 8192)`): `<` `>` 순회, 조건 가림(`shown`/`hiddenDepth`)과 닫힘 조건 태그의 예외 허용, 소문자 `a` 열림/닫힘의 `textAllowed` 증감(signed 판정), 태그 첫 글자만 ASCII 소문자로 정규화하는 분기(`! $ ? a b c h i p q ~`), `<br>`/`<p>`/`<h1~3>`와 `afterHeading`, 소스 LF→공백 규칙, 출력 논리 한도(용량−256)와 물리 한도(출력+길이 ≥ 용량 → `RangeError`), 마지막 LF.
- 콜백이 바꾼 현재 `inputOffset`/`output`을 다음 조회 전에 다시 읽고(`#validate`) 버퍼 밖이면 거부한다. 파생 슬롯 `insideTag`·`beforeClose`·`afterTag`는 선택된 분기에서만 호출한다. 훅 표는 생성 때 값복사한다.
- 선택된 실제 효과(`picture`·`command`·`condition`·`link`·`background`)가 없으면 `Error('HTML 실제 효과 미연결: 이름')`로 거부한다(C++ `std::logic_error`). 숨긴 비조건 태그는 효과를 부르지 않는다.
- `htmlPictureText`(첫 따옴표 안을 `~[I…]`로), `bindHtmlPictureText`, `bindHtmlBackground`(80BYTE 저장소에 NUL까지 복사, 뒤쪽 기존 바이트 보존, 80BYTE 이상·BYTE 아닌 글자는 아무것도 쓰기 전에 `RangeError`), `htmlBackgroundName`(NUL 없으면 거부), `createHtmlTextParserState/Globals`(전역은 문자열이 아닌 `Uint8Array(80)`).
- `ClientHtmlWindowGlobals.text`를 기존 문자열 전역에서 이 파서 전역(실제 80BYTE 배경 + P 억제)으로 바꿨다. 소비자가 아직 없어 앱 경로에는 영향이 없다.
- 기존 `www/ui/html-text.js`의 `parseHtml`은 같은 알고리즘을 따로 갖지 않고 새 `HtmlTextParser`에 위임하는 앱 경로 어댑터로 줄였다. 앱 경로의 계약(미공급 효과 건너뛰기, 문자열 `globals.background`, 80BYTE 미만 거부)은 유지한다. 그림 helper는 새 모듈에서 다시 내보낸다. 실제 앱 결선은 RawMenuControls 허브 몫이다.

## 기준선 뒤 판정

변경은 `1d5b1f437`(20261111-122) 한 건이다. `HtmlTextParserGlobals.extendedText`와 `o::DecodeExtendedSequence`로 사설 표시 4BYTE를 원자 문자로 보존하는 분기가 추가됐다. 이 표시 부호화는 E269 정책대로 **① 웹 제외**다(웹은 UTF16/D2Coding 자체 한국어 경로). 원문 집계/fixture는 기본 `false`라 원본 관측과 충돌하지 않으며, PORT_MAP base를 판정 커밋 `1d5b1f43`으로 올렸다. 웹 UTF16 표시 경로의 연결은 허브 이후 후속이다.

## 검증

- 원본 10.78 관측 전체 일치(첫 실행): 본문 **408행**(`htmltext-x86.tsv`의 originalCD 408행은 웹 범위 밖이라 제외), 배경 80BYTE **648행**(`html-background-x86.tsv`), 그림 helper **138행**(`htmlpicture-x86.tsv`의 originalCD 제외). 각 행의 출력·`inputOffset`·`shown`·`hiddenDepth`·`inTag`·`textAllowed`·`afterHeading`·P 억제 DWORD, 배경은 80BYTE 최종 저장소까지 대조했다. manifest `htmltext`·`html-background`·`htmlpicture`로 연결했고 기존 fixture를 수정하거나 다시 실행하지 않았다.
- cpppj `HtmlTextParserTests` 7개를 옮겼다: 공유 배경 수명/물리 한도, 명시 효과 경계, 조건/명령 콜백의 현재 상태 재조회, 파생 슬롯 순서와 커서 이동, 인라인 StyleText 전달(실제 `StyleText`로 색/굵기 문맥 확인).
- 관측이 닿지 않는 지점은 독립 입력 시험 10건으로 보강했다: 기본값/기본 용량 8192의 논리 한도(7937글자+LF)·용량 32비트 감김, 첫 NUL, 그림 helper 경계, 79/80BYTE 배경과 BYTE 아닌 글자, 제목 닫힘 뒤 공백/`<br>`/`<p>`, 소스 LF 규칙, 콜백 위치/출력 검증 지점(끝 위치는 허용), 훅 값복사와 슬롯 호출 조건, `a` 링크 카운터와 signed 비교.
- 변형 확인: 자동 변형 142개 중 생존 13개와 수동 의미 변형 32개 중 생존 4개를 모두 분석했다. 남은 것은 모두 동등 변형이다: 배열 끝에서 `undefined`를 읽는 `<`→`<=`(그림/배경/루프 경계 4건), 소문자화 상한(`Z`/`[`는 어느 분기도 아님), 배경 태그 `=`/`>` 위치가 0일 수 없음(태그 원문이 `b`로 시작, 6건), `textAllowed = shown`의 부호 없는 변환(입력 domain이 uint32), `remaining` 절단의 `Math.min`(JS `slice`는 범위 밖에서 빈 문자열), `beforeClose` 뒤 위치 검사(뒤따르는 태그 읽기가 입력 밖에서 효과가 없고 바로 뒤 검사가 같은 오류), 출력 길이 8191 절단(출력 최대 길이가 8191).
- 첫 실행이 일치하지 않은 곳은 시험 기대값 예측 오류 두 곳뿐이었고(작은 용량에서 평문은 논리 한도가 먼저 막아 물리 초과가 안 남, 숨김 상태에서도 태그 안 문자의 `insideTag`는 호출됨) 모두 C++ 알고리즘 추적으로 기대값을 바로잡았다. 모듈은 고치지 않았다.

## 남은 것

앱의 기존 Unicode 경로(`tell-window`·`help-topic`·`prepareDialogContent`)는 RawMenuControls 결선 전까지 `parseHtml` 어댑터를 쓴다. 다음은 TellSection의 현재 256BYTE 이름/설정 등록부를 마무리한 뒤 RawMenuControls 허브로 이어간다.
