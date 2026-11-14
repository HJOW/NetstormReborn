# webpj TextGump 한 줄·다중 줄 표시/스크롤 (WB04-E423, 2026-11-14)

RawMenuControls 선행 **TextGump 503줄(.cpp/.h, partial→port)**을 새 모듈 [`www/ui/text-gump.js`](../webpj/www/ui/text-gump.js)로 이식했다. 기준선 `1f00a25a` 전체 소스와 [CPP 정적 복원 문서](exe/cpp-text-gump-display.md)를 읽었다. 두 소스의 기준선 뒤 변경은 없다. **전용 PE fixture는 없다.** cpppj에도 직접 시험이 없고(`DialogBodyLayoutTests` 등은 이 클래스를 간접 사용), CPP 문서도 당시 시험/원본 직접 대조 미수행을 기록한다. 아래 검사는 기준선 소스 계약·손으로 계산한 기하·실제 `GumpCreate`/`GumpLifetime`/`GumpCollection`/`GumpDraw`/`StyleTextBuffer` 연결이며 원본 직접 관측 수치로 세지 않는다. CD 10.72의 vtable·float32 중간 저장·NaN 높이 순회·dirty 저장 순서는 옮기지 않았다. 앱 메뉴의 간소화판 `www/ui/scroll-text.js`는 그대로 둔다.

## 한 줄 TextGump

- **`construct`**: 기본 `(0,0,0,1)` 생성 뒤 가상 표(0x5144d4)·head 0·**signed 종류 BYTE**(초기값 45, 128 이상은 음수 DWORD)·`float32(정수 크기)`/`float32(정수×현재 scale)`·문맥 `{0,font,color,0,0,0,1,1}`·`GUMP_DRAW_CLIP` 플래그를 쓴다. 기존 head를 반납하지 않는다.
- **`appendText`**: `-1`만 범위 안 NUL 탐색, 길이 0은 조각을 만들지 않고 그 밖의 음수는 거부한다. **조각을 먼저 만든 뒤 노드를 조회**하므로 잘못된 토큰이면 조각이 남는다(원본 순서). 태그는 항상 0이다.
- **`fit`**: 전체 측정 뒤 정수 여백을 DWORD로 더해 기본 크기를 저장하고, 배율 크기는 **측정 콜백이 바꾼 현재 scale**에 저장 전 정수 합을 곱한다.
- **`draw`**: 빈 head는 조기 반환한다. 그 밖에는 현재 bounds → 독립 내부 클립 → 현재 위치 → **현재 head** 출력이며 공통 Draw의 바깥 클립 플래그가 없어도 내부 클립을 적용하고 예외에서도 복구한다.
- **`deleteScalar`**: 파생 가상 표 → `clear` 두 번 → 기본 scalar 소멸 순서다. 기본 소멸자가 마지막에 기본 가상 표(0x50ac70)를 다시 쓴다. `bindTextGumpLifetime`/`bindTextGumpDraw`는 한 줄 → 다중 줄 → 이전 분배 순서로 같은 훅에 연결한다.

## 다중 줄 ScrollingTextGump

- **배치**: 노드 dirty(bit0) 또는 버퍼 dirty 중 하나일 때 두 플래그를 **먼저 비운 뒤**(패치 순서) 현재 폭으로 `StyleTextBuffer.layout`을 호출한다. 폭은 기본 폭(소수 가능)에서 slider 사용 비트(bit1)이면 slider의 **기본 폭**을 빼고 CRT 절삭의 하위 DWORD다. 폭 0 이하이면 이전 표시 목록이 남는다. `fitHeight`는 전체 높이+1(빈 버퍼도 1), `fitContent`는 임시 폭 10000(배율 폭은 10000×scale)으로 배치한 뒤 `최대 폭+가로 여백+slider 폭+1`·`전체 높이+세로 여백`을 저장하고 배율 폭은 float32 좁힘 전 합계로 계산한다.
- **추가/교체**: 버퍼 추가 → **현재** `textFlags`에 dirty → `redraw(false)` 순서이며 그림은 문자열 통지를 호출하지 않는다. 기본 문맥 입력은 추가 전에 문맥을 복사한다. `replaceText`는 완료 줄만 `removeLine(0)`으로 반복 제거하고(미완료 끝 줄·기존 표시 복제 보존, 스타일 명령 미해석) 일반 문자열을 추가한다.
- **스크롤**: `setScroll`은 현재 높이/뷰포트(CRT 절삭, 음수 0)로 위치를 제한한 뒤 slider range → page → position 쓰기 → **현재 값 재조회** 순서로 호출하며 각 호출 직전에 노드/slider 토큰을 다시 읽고 필요한 경계는 도달한 때만 거부한다. `scrollBottom`은 `내용 높이 - 기본 높이`(실수)를 절삭하고 인자(여백)를 읽지 않는다. `requestScroll`/`requestRelativeScroll`(현재 scroll+delta, 음수는 0)/`requestBottom`은 저장 뒤 `redraw(false)`를 부르고, `processRequest`는 -1 무동작·-2 맨 아래·0 이상 `setScroll`을 한 뒤 -1로 쓰며 다른 음수는 거부하고 요청을 지우지 않는다. `atBottom`은 `내용-기본 높이-10 <= scroll`의 ordered 비교다(NaN 거짓).
- **선택/출력**: `hitTest`는 보기(left/top 포함, right/bottom 제외) 안에서만 조각 토큰을 돌려주고 줄의 첫 보이는 줄은 `y+높이 < scroll`인 줄만 건너뛴다(끝점이 같으면 유지). 조각의 위쪽 경계(localY 0)는 제외하며 세로 유효 범위는 **줄 높이-조각 높이**부터다. 폭은 줄 안에서 **누적**하므로 낮은 조각의 위쪽 빈 칸은 뒤에 오는 더 높은 조각(그림 등)에 돌아간다(원본 알고리즘 그대로). `draw`는 layout → 요청 적용 → 내부 클립 → 줄마다 첫 순회로 최대 상승을 구하고 둘째 순회에서 기준선(`상승+y`)을 맞춰 조각을 이어 그린다. 줄 y 는 첫 줄에만 저장 y를 쓰고 이후 누적한다. 보이는 영역 끝과 같은 y의 줄은 그리지 않는다.
- **client 계약**: 다중 줄 노드의 전체 생성자·입력·slider 동작은 `ScrollingTextGumpWidgets` 단위의 몫이다. 이 모듈은 `hooks`(`node`/`baseNode`/`buffer(token)→{state,handler}`/`sliderRange`/`sliderPage`/`sliderSetPosition`/`sliderPosition`/`redraw`)를 받는다. 앱 메뉴(`www/ui/scroll-text.js`)는 아직 간소화판이다.

## 검사와 발견

새 `tests/text-gump.test.mjs` **16건 통과**. 글자 폭 1·줄 높이 10+글꼴·상승 8+글꼴인 독립 글꼴 장치와 실제 `StyleTextBuffer`·`GumpCreate`·`GumpLifetime`·`GumpCollection`·`GumpDraw`를 같은 안정 노드에 묶은 장면이다.

1. 한 줄: 생성(signed 종류 BYTE 5값·float32 큰 정수·짝수/홀수 크기·head 비반납·공유 노드 거부), `appendText`(-1/0/음수/범위·문맥 사본·조각 선행 생성), `fit`(현재 scale·DWORD 여백·빈 체인), `draw`(빈 head·내부 클립 4좌표·VFX 클립·예외 복구·플래그 없는 경우), `deleteScalar`(clear 중 가상 표·두 번 clear·반납 플래그·분배 연결), 분배 순서.
2. 다중 줄: dirty 조합·slider 폭·소수/음수 폭·폭 0 이하 표시 목록 유지, `fitHeight`/`fitContent` 수치(배율 0.5·slider·소수 slider 폭), 추가 순서·현재 flags·문맥 사본·교체의 완료 줄 제거, `setScroll`의 slider 호출 순서/값/재조회와 경계 거부, 요청 계열·`processRequest`·`atBottom` 임계값, `hitTest`(보기 경계·위쪽 경계·scroll 5/10/15/31·그림 행·좁은 보기·보기보다 짧은 내용), `draw`(clip 좌표·기준선·최대 상승·viewport 끝 동률·요청 적용·예외 복구·빈 줄·삭제된 조각), CRT 절삭 하위 DWORD(NaN/∞/±2^63/2^32+2^20 등).

**최초 실패와 교정(제품 결함 아님, 시험 기대값 오류):** 등록 개수에 루트/slider가 포함되는 것, 기본 소멸자가 마지막에 기본 가상 표를 쓰는 것, `hitTest`의 위쪽 경계가 줄마다 독립으로 제외되는 것, scroll=줄 끝점일 때 첫 줄이 유지되는 것, 낮은 조각 위쪽 칸이 누적 폭 때문에 뒤의 그림에 가는 원본 알고리즘, 삭제한 조각 오류 문구와 `bounds`가 노드 조회보다 먼저 거부하는 것을 바로잡았다.

**변형 확인:** 연산자/정수 상수 변형 165개 중 143개를 잡았고(처음 생존 63개 → 새 노드 기본값·생성 경계·기본 인자 전달·짝수 크기·빈 문자열·CRT 절삭 경계·요청 0·보기 오른쪽/위쪽 가장자리·센티널 정지·조각 경계·상승 0 글꼴로 보강), 남은 22개는 동등 변형이다 — dirty 비트가 layout 전에 비워져 slider 비트 상수(2→3)가 가려짐, 2 GiB 문자열이 필요한 signed 상한, float32 기원 값에서 하위 32비트가 모두 0인 ±2^63 경계, 모두 덮어써지는 기본 생성 인자, 이미 앞에서 처리되는 `length < 0`, `measure` 가 항상 채우는 초기 출력, 같은 값을 돌려주는 `viewport`/`position` 경계(`<=`/`<1`), 사용하지 않는 `extra` 기본값, 조각 폭 합이 줄 폭과 같아 앞서 걸러지는 `width <= localX`다.

## 남은 경계

- `ScrollingTextGumpMarkup`/`ScrollingTextGumpWidgets`(다중 줄 생성자·입력·slider)와 `MenuGump*`·`RawMenuControls` 연결.
- 앱 메뉴(`www/ui/scroll-text.js`·`style-text.js`)는 아직 간소화판이며, `RawMenuControls`·`ClientRendererFrame`·`GameWorld`·`ClientGameplayGlobals`/Session 연결이 남아 실제 미션은 플레이할 수 없다.
- 원본 파일 변경과 게임/브라우저 실행은 없다.
