# webpj 다중 줄 TextGump 생성·소유·입력 ScrollingTextGumpWidgets와 StyleTextBufferStore (WB04-E427, 2026-11-14)

RawMenuControls 선행 **ScrollingTextGumpWidgets 416줄(.cpp/.h, partial→port)**을 새 모듈 [`www/ui/scrolling-text-gump-widgets.js`](../webpj/www/ui/scrolling-text-gump-widgets.js)로 이식했다(같은 파일의 `StyleTextBufferStore` 포함). 기준선 `1f00a25a` 전체 소스를 읽었다([CPP 정적 복원 문서](exe/cpp-scrolling-text-gump-widgets.md)). **전용 PE fixture는 없다.** cpppj에도 직접 시험이 없고 아래 검사는 기준선 소스 계약과 실제 `ScrollingTextGump`(E423)·`StyleTextBuffer`(E422)·`GumpLifetime`·`GumpCollection` 연결이다. 원본 직접 관측 수치로 세지 않는다. CD 10.72의 vtable·NaN 폭 비교는 옮기지 않았다.

## 기준선 뒤 변경 (3.6절 ①)

`ScrollingTextGumpWidgets.h`의 기준선 뒤 변경은 `c91d9b84`(20261110-01, webpj 요청 처리) 하나이며 **주석만** 바뀌었다 — `sliderType`/`sliderFrame`은 이름과 달리 TYPE/프레임이 아니라 현재 팔레트 색 번호(갈색 `005acd24`/검정 `005a4058`)로, 화살표 돌 버튼의 미사용 인자와 손잡이 보조 칸·삼각형 색이 된다. 동작 변경이 없어 ①로 판정하고 대응표 base를 올렸다. 웹은 같은 이름의 전역을 두고 주석으로 이 사실을 적었다.

## 구현

- **`StyleTextBufferStore`**: 버퍼는 Gump 등록 표에 넣지 않는 별도 토큰 표다(등록 수/serial 불변). `construct`는 새 토큰에 새 `StyleTextBufferState`와 `StyleTextBuffer`를 만들고(최대 줄 수 오류는 등록 전에 거부), `constructBorrowed`는 외부 상태(Client 전역 로그)를 **복사 없이** 빌리며 같은 상태의 이중 생성을 거부한다. `binding`은 같은 상태/처리기를 돌려주고, `destroy`는 처리기 소멸 → 호스트 free(필수 `release`) → 기록 제거 순서다(미완료 끝 칸 누락도 그대로).
- **`construct`**: 기본 `(0,0,0,1)` 등록 → 가상 표(0x51454c)·보조 칸·signed 종류 → **0x38 할당 → 128칸 버퍼(소유 1)** → float32 크기/문맥/overlay/요청 초기화(`+ac`/dragY와 기존 textFlags의 다른 비트는 보존) → **0xd8 할당 → 숨긴 Slider(9×100, type/frame 인자는 전역 색 번호)** → 우측 mode 1 부착(`x = float32(저장한 폭 - Slider 기본 폭)`, y 0) → 맨 아래 요청 → slider/scroll 비트 끄고 두 태그 비트 켬. C++17 대입 순서(우변 먼저)를 따른다. 버퍼 할당 실패는 거부하고, Slider 할당 실패는 토큰 0에서 거부한다.
- **`attach`**: 기본 부착 → Slider Push → 높이/배율 높이 복사 → slider·scroll 비트가 **모두** 켜졌을 때만 Slider 표시 → 우측 부착 → 대기 요청이 없을 때만 현재 scroll 요청 → 폭 변경(NaN 포함, 패치)이면 dirty와 저장.
- **`handle`**: 변화 사건(발신 Slider serial이 달라야 거부)은 Slider 위치로 스크롤 요청, step 사건은 1·2가 ±5, 3·4가 ±기본 높이(CRT 절삭) 상대 요청이며 그 밖은 무시, 갱신 사건은 dirty+redraw(false). 커서 사건은 태그 조각 위 16·scroll 가능하고 page<범위이면 15·그 밖 1을 선택 경계로 보낸다. 왼쪽 버튼은 태그 없으면 release에서 통지 해제·누름에서 y 저장/통지/캡처, 태그 위의 누름은 왼쪽 태그 사건(serial, 태그 값). 오른쪽 버튼은 태그 위의 누름만 오른쪽 태그 사건을 보낸다(태그 값 0도 조각이 있으면 보냄). 모든 조회는 현재 노드/조각의 `T` 태그를 다시 읽는다.
- **`broadcast`**(scroll 비트일 때만 y 차이의 반대 방향 상대 요청)·**`setSelectable`**(전체 DWORD 0/비0)·**`setSlider`**(최하위 비트, slider만 켜지면 scroll도, void가 없으면 Push 뒤 현재 x/y로 자기 Attach)·**`setWrap`**(noWrap만 쓰고 변경 플래그 없음)·**`setScrollable`**(끄면서 slider가 켜져 있으면 `setSlider(0)`)·**`replaceBuffer`**(null 거부, 소유한 현재 버퍼 소멸/free, 새 포인터/소유 DWORD)·**`deleteScalar`**(소유한 버퍼만 소멸, 기본 scalar 소멸 위임)과 `bindScrollingTextBuffers`/`bindScrollingTextGumpWidgets`(해당 토큰만 파생 처리기로, 이전 분배 보존).

## 검사와 발견

새 `tests/scrolling-text-gump-widgets.test.mjs` **10건 통과**. 트리/사건/입력/Slider 생성자는 호출 기록 대역이고 `ScrollingTextGump`(E423)·`StyleTextBufferStore`·`GumpLifetime`·`GumpCollection`은 실제다.

저장소(토큰/빌림/소멸 순서), 생성(할당 크기·필드·overlay·Slider 인자·부착 좌표·플래그 정확값·float32 반올림·실패/경계 없음), 부착(표시 비트 조합·요청 유무·폭 변경/NaN), 입력(변화/step/갱신/커서/태그 왼쪽·오른쪽/release/캡처/dragY), 프레임 드래그, 설정 함수, 교체/소멸(소유/빌림), 분배 연결, 기본값·플래그 정확값·홀짝 좌표·무효 높이.

**최초 실패와 교정(제품 결함 아님, 시험 기대값 오류 4건):** 위젯의 `redraw` 경계는 `construct`가 아니라 갱신 사건에서 쓰이고, 부착 시험은 Slider를 먼저 생성해야 하며, 갱신 사건 로그의 이름, 커서 경계 시험의 사건 번호를 바로잡았다.

**변형 확인:** 연산자/정수 상수 변형 164개 중 150개를 잡았고(처음 생존 37개 → 새 전역 기본값·정확한 플래그·Slider 노드 불일치·홀짝 좌표·무효 높이·dirty만 있는 노드의 태그 비트 보강), 남은 14개는 동등 변형이다 — 이미 bit0가 켜진 `flags`에 더하는 `0x400a00→0x400a01`, `ftol`의 ±2^63/33비트(상대 요청의 `| 0`이 가림)/무효 입력, 곧바로 덮어써지는 기본 생성 인자와 `scrollRequest`/`bottomPadding` 초기값, 새 버퍼로 즉시 덮이는 `buffer = 0`, 쓰이지 않는 `delta` 초기값이다.

## 남은 경계

- `MenuGump*`(메뉴 처리기 6개)·`RawMenuControls` 연결. 앱 메뉴(`www/ui/scroll-text.js`·`slider.js`·`style-text.js`)는 아직 간소화판이며 실제 미션은 플레이할 수 없다. 원본 파일 변경과 게임/브라우저 실행은 없다.
