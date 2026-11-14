# webpj 메뉴 정상 생성·controlled 시작·Attach 보정·소멸 MenuGumpWidgets (WB04-E432, 2026-11-14)

RawMenuControls 선행 **MenuGumpWidgets 331줄(.cpp/.h, partial→port)**을 새 모듈 [`www/ui/menu-gump-widgets.js`](../webpj/www/ui/menu-gump-widgets.js)로 이식했다. 이로써 메뉴 처리기 6개(MenuGump·Layout·Controls·Draw·Input·Widgets, [E428](webpj-menu-gump-layout-20261114.md)~E432)가 모두 끝났다. 기준선 `1f00a25a`와 기준선 뒤 `4ee69f4f8` 전체 소스를 읽었다([정적 복원 문서](exe/cpp-menu-gump-widgets.md)). **전용 PE fixture는 없다.** cpppj에도 직접 시험이 없고 아래 검사는 기준선 소스 계약과 손으로 계산한 값, 실제 기본 생성/배경 생성/수집기/트리/`MenuGumpLayout`·`MenuGump` 연결이다. 원본 직접 관측 수치로 세지 않는다. CD 10.72의 vtable(`0x501fa0`)·x87 중간값 저장 차이·객체 연결의 쓰기 순서는 옮기지 않았다.

## 기준선 뒤 변경 (3.6절)

`4ee69f4f8`(20261109-105, Gump Form과 실제 UI 등록표 장기 연결)만이 이 묶음의 파일을 바꿨다 — `MenuGumpWidgetState`에 `currentObjectFormType`(현재 Gump TYPE 조회)과 `ObjectFormType()`을 더하고 `Reference`가 사본 `objectFormType` 대신 이 조회를 부른다(조회가 없으면 사본). **동작 변경**이라 ②로 판정해 웹에도 같은 조회(`currentObjectFormType`/`objectFormTypeNow()`)를 이식하고 대응표 base를 `4ee69f4f`로 올렸다.

## 구현

- **`construct`**: 노드 확인 → 기본 배경 생성(0×0, `0x03e00000`/`0x201`, 전역 배경 type/frame) → 파생 가상 표(`0x50c6f0`)·`fancy/title/selectionCallback=0`·식별 BYTE(signed 확장)·문맥 `[2]=현재 흰색,[5]=그림자,[6]=1,[7]=0,[0]|=2,[1]=0`(기본 생성이 문맥 8DWORD를 0으로 초기화한 뒤) → 개수 증가·`expanding/itemWidth/menuFlags/itemCount=0`·`flags|=0x20`·깊이 `0xffffffc4`·선택 -1·높이 0·**40개 항목 값 초기화**(저장소 객체는 보존) → 입력 항목 순서대로 `#entry` → 최종 메뉴 flags → 장식이면 `#fancy` → 전체 `reflow` → `flags|=0x100`. 미작성 offsets/목표 크기/여백은 이전 값을 보존하고 세로/가로 `reflow`가 방향에 맞는 값을 쓴다.
- **항목(`#entry`)**: 명령/명시 인자 비트일 때만 현재 칸에 쓰며 **조건 false여도 먼저 쓴다**. 문자열(`0x10000`)은 문자열+자식 동시 비트/실제 문자열 없음을 거부하고 조건이 참이면 `flags`를 쓴 뒤 문자열 설정으로 항목 수를 늘린다(`reflow`는 마지막에만). 항목 수가 40이 되면 거부한다. 자식(`0x20000`)은 visible 해제 → 가상 Attach(자식,0,0,메뉴,0) → **직전 항목**(있으면)에 자식 비트와 ID를 쓴다.
- **장식(`#fancy`)**: 가로 비트 거부 → 장식(`0xc4`) 할당→생성(1,1)→Attach(0,0,메뉴)→깊이 1 → 제목(`0xd8`) 할당→생성(0,0,글꼴 0,현재 흰색)→Attach(16,16,장식)→깊이 -1 → 배경 flags `|=0x40000`. 할당 실패는 토큰 0이 되어 Attach에서 거부한다.
- **controlled 시작**: 표 index를 DWORD로 하나 올려 0이 되면 거부 → 현재 칸이 비어야 함 → `0xb90` 할당 → `construct` → 표 기록 → (가로/세로만) idle 등록 → (세로만) 배경 flags에 전달 값 OR. 가로 `0x140000`, 장식 `0xa80000`(idle 없음), 세로 `0x280000`. 콜백이 바꾼 현재 index를 매 단계 다시 읽는다(증가한 index는 실패해도 되돌리지 않는다).
- **`attach`(가상 +20)**: 부모 0이면 현재 부모, 루트 부모면 `0x4000`. `mode` bit0나 노드 `0x20000`이 아니면 화면 보정 — 일반은 `limit(부모 원점+요청, 화면 크기-노드 크기)`, 장식은 `ftol((원점+장식 x)+x)`/`ftol((원점.y+y)+장식 y)`를 음수면 0으로 → 화면 크기-장식 크기로 제한 → 장식 x/y를 되돌린 정수를 부모 기준 float32로 바꿔 기본 트리 `attach`에 넘긴다(`limit`은 요청을 float32로 좁힌 뒤 최소값, NaN은 요청 값 → 0).
- **`deleteScalar`/객체 연결**: 파생 표 → 메뉴 개수 감소(DWORD) → 선택 소멸 통지(토큰이 있을 때만) → **40개 항목 조각을 39→0 순서로 소유 필드 별칭에 `clear`** → 기본 scalar 소멸. `reference`는 Form 생성(현재 TYPE, mode 2)→소유자 등록→객체 연결 뒤 기본 노드에 `flags|=0x40`·`object`·`form`만 쓴다(object 0은 아무것도 하지 않음). `attachObject`는 연결 뒤 현재 가상 Attach를 부른다.
- **분배 바인딩**: `bindMenuTitleLayout`(제목 Reflow의 Fit/Layout → `requestScroll`/`fitContent`, signed), `bindMenuGumpWidgets`(트리 Attach/수명 scalar 소멸 — 메뉴만), `bindMenuGumpObjectAttach`(객체 부착 가상 +3c).

## 검사와 발견

새 `tests/menu-gump-widgets.test.mjs` **21건 통과**(공용 장면 `tests/menu-gump-support.mjs`). 배경 생성/기본 생성/수집기/기본 트리 Attach·레이아웃·메뉴 크기 처리기는 실제이고 장식/제목 생성·할당·가상 Attach·Form 연결·수명·StyleText 해제는 호출 기록 대역이다.

**최초 실패와 교정(제품 결함 아님, 시험 기대값 오류 4건):** 기본 생성이 문맥 8DWORD를 0으로 초기화한다는 점, 항목 3개의 세로 목표 높이가 (측정 4+여백 2)×3인 점, 장식/제목 할당 큐 토큰, 시작 콜백이 index를 바꾸면 세로 시작은 0 토큰의 배경 flags 쓰기에서 거부된다는 점을 바로잡았다.

**변형 확인:** 연산자/정수 상수 변형 180개 중 159개를 잡았다(처음 생존 39개 → bit0만 켠 flags·정확한 flags 값·항목 초기화·ftol NaN/범위 밖/32비트 감김·제목 할당 실패·세 항목 뒤 자식·object 1·짝수 제목 인자 보강). 남은 21개는 동등 변형이다 — `ftol`의 ±2^63 정확한 경계와 같은 값의 리터럴 2개, 같은 값끼리의 `<=`/`<`, 곧바로 덮어써지는 `screenX/Y` 초기값·`menuFlags`/`itemWidth` 초기화·`flags`에 이미 켜져 있는 bit0, 쓰이지 않는 가로/장식 `backgroundFlags` 인자, 항목 수 짝수에만 의미 있는 `|0`, 정상 경로로 도달할 수 없는 항목 칸 40, 기본 생성이 이미 0으로 쓴 문맥 칸이다.

## 남은 경계

- **메뉴 처리기 6개는 모두 끝났다.** 남은 5-A 항목은 실제 `RawMenuControls`(1,345줄)와 `ClientRendererFrame`·E197/E198/E201 Session 재비교이다. 앱 메뉴(`www/ui/menu-list.js`·`popup-menu.js`)는 아직 간소화판이며 실제 미션은 플레이할 수 없다. 원본 파일 변경과 게임/브라우저 실행은 없다.
