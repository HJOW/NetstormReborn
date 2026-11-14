# webpj 다중 줄 스타일 입력 파서 ScrollingTextGumpMarkup (WB04-E424, 2026-11-14)

RawMenuControls 선행 **ScrollingTextGumpMarkup 97줄(.cpp/.h, todo→port)**을 새 모듈 [`www/ui/scrolling-text-gump-markup.js`](../webpj/www/ui/scrolling-text-gump-markup.js)로 이식했다. 기준선 `1f00a25a` 전체 소스와 [CPP 정적 복원 문서](exe/cpp-scrolling-text-gump-markup.md)를 읽었다. 두 소스의 기준선 뒤 변경은 없다. **전용 PE fixture는 없다.** cpppj에도 직접 시험이 없고 CPP 문서도 당시 시험/원본 직접 대조 미수행을 기록한다. 아래 검사는 기준선 소스 계약과 실제 `ScrollingTextGump`(E423)·`StyleText`·`StyleTextMarkup`(E421)·`StyleTextBuffer`(E422) 연결이며 원본 직접 관측 수치로 세지 않는다. CD 10.72(004b6c80·`W` 소리)는 옮기지 않았다.

## 구현

`parse(token, text)`가 문자열의 **첫 NUL까지**만 처리한다. 노드의 현재 기본 문맥을 복사해 시작하고(복사본이라 `~B` 등이 노드 기본 문맥을 바꾸지 않는다) 매 구간마다 현재 `+c8` 태그를 다시 읽는다.

- 명령 앞 구간 → `appendText(token, 나머지, 문맥, 구간 길이, 현재 태그)`. 구간이 비면(`~`로 시작하거나 `~` 연속) 길이 0 입력을 만들지 않는다. `~` 없는 나머지는 길이 -1(NUL 탐색)로 마지막에 한 번 추가하며 비어 있어도 호출한다(문자열 통지·dirty·redraw를 남김).
- `~~` 리터럴도 길이 1의 버퍼 입력(통지·redraw 포함)이다.
- `~[...]`는 닫는 `]`가 있을 때만 `StyleTextMarkup.applyScrolling`에 위임한다. 태그는 노드 `+c8`의 소유 필드 별칭이라 `~[T9]`가 호출 간 유지되고, 제한 모드 판정은 그림 검사 시점의 현재 `+d4`(callback ≠ 0)를 읽으며, 그림 추가는 `ScrollingTextGump.appendPicture`(현재 태그·`#` 맞춤 65×60 또는 제한 99999×12)로 간다. 닫히지 않은 대괄호는 `~[` 두 글자만 소비하고 나머지는 본문이다.
- 그 밖의 `~x`는 `StyleText.applyShortCommand(문자, 문맥, 현재 기본 문맥, callback ≠ 0)`다. 색 명령은 StyleText 팔레트 장치를 쓰고 `~.`/`~n`은 **현재** 기본 문맥을 다시 읽으며(문자열 통지가 바꾼 값 반영) 제한 모드에서는 글꼴 0/1/3/4를 무시한다. 모르는 명령은 `~` 한 글자만 버리고 뒤 글자를 본문으로 잇는다. 끝의 `~`는 마지막 빈 입력을 남긴다.
- 노드 조회는 `{ node, base }` 두 경계이며 두 주소가 같은 노드여야 한다(복사본/파서 전용 노드를 만들지 않음).

## 검사와 발견

새 `tests/scrolling-text-gump-markup.test.mjs` **4건 통과**(공용 장면은 E423 시험에서 `tests/text-gump-support.mjs`로 분리해 두 시험이 쓴다).

1. 생성자/노드 검증(두 경계 필수·미등록/0 토큰·기본/파생 주소 불일치 거부).
2. 구간별 호출 순서(길이 -1/명시 길이·dirty·redraw 횟수)·`~B ~I ~N` 문맥 변화·노드 기본 문맥 불변·NUL/Uint8Array 입력·첫 글자 NUL·`~` 시작·빈 문자열.
3. 짧은 명령(색·`~.`/`~n` 현재 기본 문맥 재조회·제한 모드 글꼴 무시·모르는 명령·끝 `~`·`~~` 리터럴 호출 횟수).
4. 대괄호(태그 유지·그림 호출 인자 `[태그, 폭, 높이]`·제한 모드 99999×12와 높이 17 거부·소리·모르는 대괄호·미닫힘·80바이트 오류 전파·`E` 오프셋이 이후 구간 문맥에만 반영).

**최초 실패와 교정:** 없음(첫 실행 4건 통과). **변형 확인:** 연산자/정수 상수 변형 39개 중 31개를 잡았고(처음 40개 중 28개 → 첫 글자 NUL·`~` 시작 시험 보강, 미사용 `need` 보조 함수 삭제), 남은 8개는 동등 변형이다 — 2 GiB 문자열이 필요한 signed 상한, 마지막 NUL을 쓰는 `terminated`의 범위 한 칸(이미 0), `Uint8Array` 변환의 범위 한 칸(NUL 앞에서 잘림), 길이 1로 복사해 뒤 NUL을 읽지 않는 리터럴의 두 번째 바이트, `]` 위치가 항상 2 이상이라 `close >= 0`의 경계.

## 남은 경계

- `ScrollingTextGumpWidgets`(다중 줄 생성자·입력·slider 재부착, `StyleTextBufferStore`)는 선행 `ScrollGump`/`ScrollGumpWidgets`(slider 실제 클래스)가 먼저 필요하다. 이후 `MenuGump*`·`RawMenuControls` 연결이 남는다.
- 앱 메뉴(`www/ui/scroll-text.js`·`style-text.js`)는 아직 간소화판이며, 실제 미션은 플레이할 수 없다. 원본 파일 변경과 게임/브라우저 실행은 없다.
