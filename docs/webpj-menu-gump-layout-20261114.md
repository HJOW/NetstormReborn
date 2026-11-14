# webpj 메뉴 저장소·크기 프레임과 항목 레이아웃 MenuGump·MenuGumpLayout (WB04-E428, 2026-11-14)

RawMenuControls 선행 **MenuGump 246줄·MenuGumpLayout 234줄(.cpp/.h, 둘 다 partial→port)**을 새 모듈 [`www/ui/menu-gump.js`](../webpj/www/ui/menu-gump.js)와 [`www/ui/menu-gump-layout.js`](../webpj/www/ui/menu-gump-layout.js)로 이식했다. 기준선 `1f00a25a` 전체 소스를 읽었다([프레임](exe/cpp-menu-gump-frame.md)·[레이아웃](exe/cpp-menu-gump-layout.md) 정적 복원 문서). **전용 PE fixture는 없다.** cpppj에도 직접 시험이 없고 아래 검사는 기준선 소스 계약과 손으로 계산한 값, 실제 `GumpCreate`·`GumpLifetime` 저장소 연결이다. 원본 직접 관측 수치로 세지 않는다. CD 10.72의 index 쓰기 순서·x87 unordered 분기·자식 연결 쓰기 순서(숨김→ID)는 옮기지 않았다.

## 기준선 뒤 변경 (3.6절)

두 파일의 기준선 뒤 변경은 없다. 같은 묶음의 `MenuGumpWidgets`만 `4ee69f4f8`(20261109-105)에서 바뀌었고 그 단위(E432)에서 다룬다.

## 구현

- **`MenuGumpItem`/`MenuGumpNode`/`MenuGumpState`**: 항목은 라벨 40바이트 + 자식 등록 ID/명령/인자/StyleText head/flags(모두 DWORD)다. 노드는 `GumpBackgroundNode` 파생이며 offsets/heights(40칸)·margins(4칸)는 `Float32Array`라 쓰기마다 float32로 좁혀지고, itemWidth/targetWidth/targetHeight는 쓸 때 `Math.fround`한다. `itemCount`·`index`는 DWORD이고 반복·범위 비교만 signed로 읽는다. controlled 표는 20칸(0번 예약)이다.
- **`MenuGump.setWidth/setHeight`**: 부모 크기를 float32로 쓰고 배율을 곱한다. 장식(`0x800000`) 메뉴만 장식 Push → **기본 폭+32 / 제목 기본 높이+현재 높이+48의 double 합계를 float32로 한 번만 좁힌 값**과 그 합계에 장식 배율을 곱해 좁힌 값을 쓴다(float32로 먼저 좁힌 합계에 곱하면 54.72999954… 대신 54.72999572…가 되는 입력으로 확인) → Attach(-16, `ftol(-제목 기본 높이-32)`의 float32, 메뉴, 0). `redraw`는 장식 → 기본 순서이며 장식 유무는 `menuFlags`와 무관하다. `itemPosition`은 가로는 `index×itemWidth`(signed index, 범위 검사 없음)/현재 높이, 세로는 현재 폭/저장 offset(범위 밖은 거부).
- **`startExpand`/`idle`**: 폭 방향(`0x80000`)이면 폭 목표 → 높이 0, 아니면 폭 0 → 높이 목표(첫 크기 변경의 콜백이 바꾼 목표를 다시 읽음) 뒤 실제 idle 표에 등록하고 `expanding=1`. `idle`은 목표 이상이면 `expanding`만 0으로 쓰고, 아니면 Push → (높이 부족이면) 높이 → (폭 부족이면) 폭 → Pop이며 비교는 plain float 비교다(NaN은 확장 분기, 두 축 모두 생략).
- **스택**: `current`/`itemCount`/`takeCurrent`/`attachCurrent`/`attachObjectCurrent`. index가 칸 수 이상(signed 음수 포함)이거나 예약 0번을 꺼내려 하면 변경 전에 거부하고, 부착 콜백이 바꾼 현재 index의 칸을 지운다. 객체 부착은 등록표 루트(`entries[0]`)를 함께 넘긴다. `bindMenuGumpFrame`은 메뉴만 idle/redraw를 새 몸체로 보내고 다른 토큰은 이전 처리기를 그대로 쓴다.
- **`MenuGumpLayout.append`**: signed 항목 수가 40 이상이면 조용히 생략, 구분선은 비어 있지 않은 문자열/자식을 거부하고 선두/연속이면 생략, 명령/인자는 해당 비트일 때만 쓰며 자식은 **ID 저장 → 숨김(visible 해제) → 가상 Attach(자식,0,0,메뉴,0) → 자식 비트**(분리 자식은 ID만), `flags|0x10000`을 쓰고 `setText` → 항목 수 증가 → `reflow`. 쓰기마다 현재 항목 수를 다시 읽는다.
- **`setText`/`measureItem`**: 일반 항목은 40바이트만 복사하고 NUL 뒤를 0으로 채우며 40번째 뒤에 종료 문자를 쓰지 않는다. 스타일 항목은 문맥 사본의 font를 0으로 한 뒤 Clear → (비활성이면 `~*~x` 접두) → Parse이며 256바이트(접두 포함) 이상은 Clear 뒤 Parse 전에 거부한다. 측정은 읽는 시점의 항목 flags로 styled/plain을 고르고 결과는 signed 사본이다.
- **`reflow`**: 세로 여백 15/15/1/1(compact는 좌우 3), 아니면 4/4/1/1. 항목 크기는 측정+여백의 CRT 절삭이며 세로의 자식/분리 자식은 표식(" >") 폭을 더하고 높이는 큰 쪽을 쓴다. 장식이면 제목 Push → Fit(0) → Layout(0,0) → 폭 = `ftol(max(폭, 제목 기본 폭))`(NaN은 0) → 제목 Attach. 세로는 목표 폭/높이(총합)와 누적 offset을, 가로는 `항목 수×폭`(imul)과 같은 높이/0 offset을 쓰고 방향 비트가 없으면 목표/offset을 보존한 채 항목 폭만 쓴다. 매 반복·콜백 뒤 현재 항목 수/문맥/플래그를 다시 읽는다.
- **`menuGumpItemBytes`/`bindMenuLayoutText`**: 항목 60바이트(라벨 + 다섯 DWORD 리틀 엔디언)와 자식 표식 `[0x20,0x3e,0]`을 전달 문맥의 실제 `GumpText.measure`로 측정한다.

## 검사와 발견

새 `tests/menu-gump.test.mjs` 17건·`tests/menu-gump-layout.test.mjs` 20건 **통과**(공용 장면 `tests/menu-gump-support.mjs`). 기본 생성/수명 저장소는 실제 `GumpCreate`/`GumpLifetime`이고 가상 경계와 측정/StyleText는 호출 기록 대역이다.

**최초 실패와 교정(제품 결함 아님, 시험 기대값 오류 7건):** 장식 Push 콜백의 첫 호출만 쓰는 단언, 문맥 인자 위치(`words(1,2)`는 context[0]=1), 비활성 접두 길이(4+251까지 통과), 널 문자열은 구분선 검사를 통과해 문자열 설정에서 거부, compact 높이(4+1+1=6), float32로 좁혀지는 폭(`fround(1e9+30)`)을 바로잡았다.

**변형 확인:** 연산자/정수 상수 변형 245개(menu-gump.js 75·menu-gump-layout.js 170) 중 223개를 잡았다(처음 생존 48개 → 새 기본값·bit0만 켠 방향/장식/compact 플래그·ftol NaN/±2^63 이상/32비트 감김·짝수 좌표/측정값·첫 칸 index 0·정확한 flags 값·음수 높이 가로 메뉴·표식 높이 비교·기본 인자 보강). 남은 22개는 동등 변형이다 — `ftol`의 ±2^63 정확한 경계(어느 쪽이든 하위 DWORD 0)와 같은 값으로 평가되는 리터럴 2개, `menuGumpItemBytes` 반복의 한 칸 초과(범위 밖 쓰기 무시/다음 DWORD가 곧바로 덮어씀), 일반 라벨 복사의 `NaN & 255 = 0`과 길이 40 경계, 같은 값의 `margins[0]/[1]`·`margins[2]/[3]`, 같은 높이/폭끼리의 `<=`/`>` 경계, 곧바로 덮어써지는 측정 출력 초기값이다.

## 남은 경계

- `MenuGumpControls`·`MenuGumpDraw`·`MenuGumpInput`·`MenuGumpWidgets`와 `RawMenuControls` 연결. 앱 메뉴(`www/ui/menu-list.js`·`popup-menu.js`)는 아직 간소화판이며 실제 미션은 플레이할 수 없다. 원본 파일 변경과 게임/브라우저 실행은 없다.
