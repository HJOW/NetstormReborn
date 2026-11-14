# webpj Slider 생성과 화살표/손잡이 Draw ScrollGumpWidgets (WB04-E426, 2026-11-14)

RawMenuControls 선행 **ScrollGumpWidgets 262줄(.cpp/.h, partial→port)**을 새 모듈 [`www/ui/scroll-gump-widgets.js`](../webpj/www/ui/scroll-gump-widgets.js)로 이식했다. 기준선 `1f00a25a` 전체 소스를 읽었다([CPP 정적 복원 문서](exe/cpp-scroll-gump-widgets.md)). 두 소스의 기준선 뒤 변경은 없다. **전용 PE fixture는 없다.** cpppj에도 직접 시험이 없다. 아래 검사는 기준선 소스 계약·손으로 계산한 기하이며 원본 직접 관측 수치로 세지 않는다. CD 10.72의 vtable·손잡이 할당 크기(0xd4)·FILD 중간값 보존은 옮기지 않았다. 앱 메뉴의 간소화판 `www/ui/slider.js`는 그대로 둔다(WB02-H 오라클에서 확인한 "cpppj 스크롤 화살표 삼각형은 붉은 갈색(99,38,22)·원본 스크린샷과 웹은 검정" 차이는 이 클래스가 삼각형 색을 `textContext[2]`에서 읽어 오는 방식에서 비롯된다. 색 번호는 Slider 생성자의 frame 인자로 들어오며 실제 팔레트 연결은 Client 조립 단위의 몫이다).

## 구현

- **`ScrollGumpArrowNode`**(돌 버튼 노드 + 방향)·**`ScrollGumpWidgetState`**(Slider/손잡이/화살표 종류 BYTE 초기값 39/51/1·손잡이 기본 배경 type 0/frame 1).
- **`constructArrow`**: 빈 라벨과 `{0,0,color,0,0,0,1,1}` 문맥으로 돌 버튼 전체 생성자를 호출한 뒤 파생 가상 표(0x5116a8)·팝업 유지 비트(0x20, 다른 버튼 플래그는 보존)·signed 정렬·방향을 쓴다. Slider 생성 안에서는 **type 인자가 `unused`, frame 인자가 `color`**로 전달된다.
- **`constructThumb`**: 기본 배경 `(0,0,0x3ec0000,0x201)`을 전역 기본 type/frame으로 만든 뒤 파생 표(0x511668)·손잡이 rect 0·signed 정렬·인자 사본(`detailType/Frame`)·float32 기본 크기와 저장 전 정수×scale 배율 크기·키보드 비트를 쓴다. 이전 y/x는 보존한다.
- **`construct`**: 기본 `(0,0,0,1)` 생성 뒤 값 필드(최소/최대/위치/page)·+b0·방향·자식 토큰·보조 칸을 0으로 쓰고(+bc는 보존) signed 정렬을 쓴다. 0 크기는 축마다 40, **높이<폭이면 가로**(scrollFlags 1), 키보드 비트와 step 1, float32 크기(배율 높이는 저장한 float32 높이를 다시 읽는다). 두께는 가로면 저장한 높이, 세로면 **저장 전 폭**의 CRT 절삭이다. 화살표 두 개와 손잡이를 `new` 크기 0x178/0x178/0xd8로 할당·생성하고 **C++17 대입 순서(우변 먼저)**로 현재 노드에 토큰을 쓴다. 가로 화살표 방향은 왼쪽 3/오른쪽 1, 세로는 위 0/아래 2이며 손잡이는 `폭-2×두께`(가로)/`높이-2×두께`(세로)다. 부착은 감소(0,0)·증가(`float32(길이-두께)`)·손잡이(`두께`) 순서로 mode 0이다. 마지막에 position 0 → 범위 0..100 → page 10 → position 50을 실제 Slider(E425)에 통지한다. 할당 실패(0)는 자식 토큰 0이 되고 이어지는 부착이 호스트 예외로 거부한다.
- **`drawArrow`**: 돌 버튼 Draw 뒤 현재 Bounds에 안쪽 여백 2를 두고 방향별 삼각형 세 선을 그린다(방향 1 오른쪽·2 아래·3 왼쪽·그 밖 위). 중점 합은 DWORD로 감기고 `/2`는 0 방향 절삭이며 색은 기본 문맥 `textContext[2]`다. **`drawThumb`**: 배경 Draw 뒤 현재 Bounds+handle을 기본 배경 `type/frame`으로 `0x3e00000` 플래그로 다시 그린다(양 끝 포함 rect를 +1 없이 half-open 배경 함수에 전달). **`deleteScalar`**/`bindScrollGumpWidgetDraw`/`bindScrollGumpWidgetLifetime`: 기본 소멸 위임, Slider는 빈 Draw, 화살표/손잡이 분배, 이전 처리 보존.

## 검사와 발견

새 `tests/scroll-gump-widgets.test.mjs` **10건 통과**. 돌 버튼은 호출 기록 대역, 기본 배경은 실제 `GumpBackgroundCreate`/`GumpCreate`, Slider는 기록 대역과 실제 `ScrollGump`(E425) 두 경우를 쓴다.

생성자/노드 검증, `constructArrow`(문맥·플래그 보존·signed 정렬), `constructThumb`(기본 배경 인자·이전 y/x 보존·float32 반올림·자산 경계), `construct` 세로/가로(할당 크기·방향·부착 좌표·범위 통지 순서·0 크기·정사각·큰 두께의 음수 손잡이 높이), 경계(할당 실패·할당/부착 경계 없음·실제 Slider로 0..100/page 10/position 50), `drawArrow`(방향 5종 선 좌표·음수 중점 절삭·DWORD 감김), `drawThumb`(rect·갱신된 type/frame), 분배 연결, 새 노드/전역 기본값과 필드 초기화.

**최초 실패와 교정(제품 결함 아님, 시험 기대값 오류 3건):** 크기 0(40×40)의 두께는 폭 40이라 증가 버튼 y가 0이고 손잡이 높이가 -40이며, 음수 좌표의 중점(`-2.5`)은 0 방향으로 `-2`이고, DWORD 감김 시험의 방향 선택을 맞췄다. 그리고 변형 확인은 기준 시험이 실패하던 상태에서 한 번 돌려 거짓 "생존 0"이 나왔으므로 기준이 초록인 상태에서 다시 돌렸다.

**변형 확인:** 연산자/정수 상수 변형 125개 중 109개를 잡았고(처음 생존 25개 → 새 노드/전역 기본값·필드 초기화(이전 값 지움/+bc 보존)·정확한 flags 보강), 남은 16개는 동등 변형이다 — 이미 bit0가 켜진 `flags`에 더하는 `0x200→0x201`, 정수 범위 입력만 받는 `ftol`의 ±2^63/33비트/무효 변환, 곧바로 덮어써지는 기본 생성 인자(command/width/height)와 `scrollFlags`/자식 토큰 초기값이다.

## 남은 경계

- `ScrollingTextGumpWidgets`·`StyleTextBufferStore`(다중 줄 생성자·입력)와 `MenuGump*`·`RawMenuControls` 연결.
- 앱 메뉴(`www/ui/slider.js`·`scroll-text.js`·`style-text.js`)는 아직 간소화판이며 실제 미션은 플레이할 수 없다. 원본 파일 변경과 게임/브라우저 실행은 없다.
