# webpj StyleText 대괄호 명령 `~[...]` (WB04-E421, 2026-11-14)

RawMenuControls 선행 **StyleTextMarkup 426줄(.cpp/.h, partial→port)**을 이식했다. 기준선 `1f00a25a` 전체 소스와 [CPP 정적 복원 문서](exe/cpp-style-text-markup.md)를 읽었다. **전용 PE fixture는 없다.** cpppj에도 대괄호 파서 전용 시험이 없고(`KnownBugTests`의 `StyleTextOffsetAllowed` 4건뿐), CPP 문서도 당시 시험/원본 직접 대조 미수행을 기록한다. 아래 검사는 기준선 소스 계약·독립 구현 전수 대조·실제 자산 연결이며 원본 직접 관측 수치로 세지 않는다. CD 10.72는 웹 10.78 범위 밖이라 `I→W→T` 순서·W 소리 형식·판본 인자를 옮기지 않았다.

## 구현 범위

새 모듈 [`www/ui/style-text-markup.js`](../webpj/www/ui/style-text-markup.js)가 `StyleText.parse`의 `bracket` 훅(문자열 `~[…]` 안쪽)을 처리한다. 앱 메뉴가 쓰는 간소화판 `www/ui/style-text.js`/`picture-resolver.js`는 그대로 두고, 실제 `StyleTextState`/`Markup` 저장소 위에 같은 규칙을 연결했다.

- **검색**: 원본 정규식 여섯 개를 엔진 없이 같은 순서로 옮겼다 — 시작/끝 앵커 없는 **부분 검색**, 대소문자 구분, 공백 반복은 ASCII space만, 캡처는 `.`·CR·LF·space만 제외(**탭 포함**), 정규식의 점은 이스케이프되지 않아 **임의 글자 한 개**(CR/LF/space 포함)를 소비, 각 반복은 가장 긴 길이부터 되돌리며 성립하는 첫 조합을 선택. 닫는 대괄호 안쪽의 **첫 NUL**에서 끊고 캡처는 **첫 80바이트**만 숫자/이름에 쓴다.
- **순서**: `I→S→T→C→B→E`에서 처음 성립한 형식 하나만 처리한다. 옵션이 꺼져 있어도 성립한 형식은 소비하고 다음 형식으로 넘어가지 않는다. 모르는 형식은 문맥/태그/체인을 바꾸지 않는다.
- **I 그림**: 타입 캡처의 CRT 정수가 0이면 같은 문자열을 이름 표에서 다시 조회하고 `-1`일 때만 생략한다. 프레임 식은 `*`(gumpFrame, `-1`이면 defaultFrame)·`#`(65×60 상자 맞춤)·`&`(맞춤 + 색 치환 BYTE 1)·글자 구간(첫 글자가 **signed 비교**로 `A ≤ 글자 < A + letterKinds`이면 글자 구간 첫 인덱스 + 숫자 suffix, DWORD 감김)·그 밖은 숫자 식 전체다. 클램프는 0 미만 → 0, **`count` 이상 → `count`**(`count-1`이 아님)이며, 패치의 타입 helper를 호출마다 다시 조회하는 순서(`*` 최대 5회·글자 5회·음수 3회)를 시험이 호출 횟수와 바뀌는 값으로 고정한다. 그림 생성은 현재 태그 → mapped BYTE → 필요하면 Fit → 공통 꼬리 추가 순서다.
- **옵션**: `S`(허용 옵션 비0이면 80바이트 이름으로 소리 요청)·`T`(unsigned 공통 태그, 이후 문자열/그림/`~~`/마지막 빈 조각에 전달되고 `~.`은 문맥만 복구)·`C`/`B`(문맥 +8 전경/+14 그림자 색 번호, 허용 옵션 0이면 소비만)·`E`(허용 옵션 비0 + 두 좌표가 **모두** -4..4일 때만 그림자 비트 2·검정 별칭·좌표 저장).
- **다중 줄 `applyScrolling`**: 한 줄 파서와 같은 순서에서 `I`는 `*`/`#`만 특수 처리하고(접두 소비 뒤 suffix의 `#`가 맞춤), 숫자 타입은 현재 타입 수 이상이면 그림을 만들지 않으며, `restricted()`가 참이면 타입 1 이상·타입 수 미만·프레임 < count·SHP 헤더 +1a signed 높이 값이 있고 **16 이하**일 때만 99999×12로 맞춘다(높이 0·음수 허용). 실제 버퍼 추가(`appendPicture`)와 제한 필드(`restricted`)는 StyleTextBuffer/TextGump 단위가 연결한다(client 계약, 함수 인자).
- **연결 함수**: `bindStyleTextMarkupAssets`(실제 `GameAssets`: 188타입 수·이름·특수/글자 프레임·SHP 헤더 높이, 내장 무자산·여분 타입은 빈 보기, 표 밖은 `RangeError`), `bindStyleTextMarkupPalette`(검정 별칭 0번), `loadStyleTextMarkupOptions`(`allowzonesounds`/`zonebgcolor`/`zonefgcolor`/`zoneoffset`가 있을 때만 갱신), `bindStyleTextMarkupSound`(`playByName(name, 0,0,0,0,0)`; 브라우저 음향 장치는 ClientAudio 몫).

## 기준선 뒤 변경 (3.6절 ②)

`StyleTextMarkup.cpp/.h`의 기준선 뒤 변경은 `b810ebe0`(20261111-147, 알려진 버그 **NB02**) 하나다. 원문은 E 오프셋을 두 좌표 중 **하나만** -4..4 안이어도 둘 다 적용해 다른 좌표를 크게 줘서 글자를 뒤로 밀 수 있었다. 이를 `styleTextOffsetAllowed(x, y)`(두 좌표 모두 범위 안일 때만)로 옮겼고, 시험이 격자 15×15 전수와 cpppj `KnownBugTests` 4건을 대조한다. 대응표 base를 `b810ebe0`으로 올렸다.

## 검사와 발견

새 `tests/style-text-markup.test.mjs` **17건 통과**.

1. NB02 격자 전수·T 형식(대소문자/부호/탭/DWORD 감김/80바이트 숫자)·S/C/B 공통 형식(점=임의 글자·공백 되돌리기 1~5칸·CR/LF/탭 캡처·NUL 절단)·우선순위/비활성 소비/모르는 형식.
2. C/B/E 문맥 칸·E 형식·검정 훅 실패 시 이미 더한 그림자 비트 보존·S 장치 요구 시점.
3. I 그림(숫자/이름/`0` 재조회/`-1` 감김/80바이트 경계 79·80)·프레임 식(짝수/홀수 특수 프레임, gumpFrame `-2`는 `-1`과 다름, 글자 구간 경계와 signed 비교, suffix 감김, count 클램프)·타입 helper 재조회 호출 횟수.
4. `StyleText.parse` 통합(태그 전달·`~.`), 다중 줄 그림(맞춤 상자 값·타입 수 경계·제한 모드 경계), 실제 `GameAssets` 연결과 가짜 자산 경계(정의 없는 타입·높이 상위 바이트/부호·프레임 수 signed 상한).
5. **독립 구현 전수 대조**: 116개 타입 × 프레임 식 14종(1,000여 건)을 기존 독립 구현 `picture-resolver`와 같은 프레임·폭·높이·기준점으로 대조하고 프레임 번호도 독립 계산식과 대조했다(글자 구간 50건 이상). `#` 맞춤 배율은 모든 타입에서 독립 float32 공식(세로 비율 먼저 float32, 선택 비율 1 미만만 갱신)과 같다.
6. 실제 StyleText→대괄호→Markup→글꼴: 본문 폭에 그림 폭이 더해지고 높이는 `146 + 본문 상승`이며 공통 Clear가 두 저장소를 비운다.

**최초 실패와 교정(제품 결함 아님, 시험 기대값 오류 4건):** 프레임 식 79/80바이트 경계를 한 글자 잘못 셌고, `fround(0.1)`를 쓰지 않았으며, `T`가 `B`보다 먼저 검사되는 우선순위를 몰랐고, fortGump의 `*`가 gumpFrame 20(프레임 0이 아님)이라 높이 계산 기대값을 바로잡았다.

**변형 확인:** 연산자/정수 상수 변형 194개 중 181개를 잡았고, 처음 31개 생존에서 시험 공백(공백 3칸 이상 되돌리기, 짝수 특수 프레임의 `| 0`→`| 1`, `-2` 특수 프레임, 글자 종류 0, 제한 모드 경계 타입 1/프레임 0/타입 수 같음, 맞춤 상자 값, 선행 NUL, 정의 없는 타입, 높이 상위 바이트, 프레임 수 상한)을 보강했다. 남은 13개는 동등 변형이다 — 범위 밖 읽기가 `undefined`라 같은 결과(`skipSpaces`·`separated` 끝·`mark` 반복·숫자 끝의 `<`→`<=`), 숫자 캡처의 첫 숫자 구간 밖은 `ConfigParseLong`이 읽지 않아 `end += 2`가 같음, 첫 캡처 되돌리기는 항상 `끝-2`에서 성공하거나 `끝`에서 성공해 `end -= 2`가 같음(무작위 18만 입력 차이 0, 대조군 `separator -= 2`는 1,106건 차이), `count`/`frame` 클램프의 `| 1`/`<=`/`> `는 경계에서 같은 값을 돌려줌, 덮어써지는 초기값 `frame = 0`, 빈 suffix의 `>= 0`.

**원문 규칙 관찰(CPP 문서와 다른 점):** `E` 형식은 `E<x>.<y>`다. 첫 캡처가 점으로 시작할 수 없어 문서의 `E.x.y` 표기는 성립하지 않는다(시험이 고정). `S.droppiece.wav`처럼 확장자가 있는 이름은 캡처가 첫 점에서 끝나므로 소리 이름은 `droppiece`다(소리 장치 단위가 이 이름 규칙을 알아야 한다).

## 남은 경계

- `StyleTextBuffer`·`TextGump`/`ScrollingText`가 `applyScrolling`의 `restricted`/`appendPicture`를 실제 버퍼에 연결한다.
- 앱 메뉴(`www/ui/style-text.js`)는 아직 간소화판을 쓰며, `RawMenuControls`·`ClientRendererFrame`·`GameWorld`·`ClientGameplayGlobals`/Session 연결이 남아 실제 미션은 플레이할 수 없다.
- 10.72 CD 판본의 `I→W→T`·`W` 형식은 옮기지 않았다(웹 범위 밖, 판본 인자 없음).
- 원본 파일 변경과 게임/브라우저 실행은 없다.
