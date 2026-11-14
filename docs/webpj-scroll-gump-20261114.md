# webpj Slider ScrollGump 값/범위/부착/손잡이 입력 (WB04-E425, 2026-11-14)

RawMenuControls 선행 **ScrollGump 357줄(.cpp/.h, partial→port)**을 새 모듈 [`www/ui/scroll-gump.js`](../webpj/www/ui/scroll-gump.js)로 이식했다. 기준선 `1f00a25a` 전체 소스를 읽었다([CPP 정적 복원 문서](exe/cpp-scroll-gump-state.md)). 두 소스의 기준선 뒤 변경은 없다. **전용 PE fixture는 없다.** cpppj에도 직접 시험이 없다. 아래 검사는 기준선 소스 계약과 손으로 계산한 기하이며 원본 직접 관측 수치로 세지 않는다. CD 10.72의 float32 중간 저장·`SetPage` 위임·가로 손잡이 무동작은 옮기지 않았다. 앱 메뉴의 간소화판 `www/ui/slider.js`는 그대로 둔다. Slider의 생성자와 화살표/손잡이 Draw는 `ScrollGumpWidgets` 단위의 몫이다.

## 구현

- **노드/전역**: `ScrollGumpNode`(min/max/position/page/step·방향 비트·감소/증가/손잡이 토큰·보조 칸), `ScrollGumpThumbNode`(기본 배경 노드 + 생성 인자 사본·양 끝 포함 손잡이 사각형·이전 y/x), `ScrollGumpState`(click/repeat/step/change 사건 ID). 기본/파생 노드는 같은 주소여야 하며 화살표는 다른 파생 노드(돌 버튼 등)도 기본 저장소로 허용한다.
- **`setPosition`**: `최댓값-최솟값 < page`이면 거부(원본 assert) → 최솟값 미만은 최솟값 → `최댓값 < page+position`(DWORD 합이 감겨 큰 위치는 제한되지 않을 수 있음)이면 `최댓값-page` → 저장. void 비트가 없으면 가상 Push 뒤 **현재** x/y로 가상 Attach(parent 0, mode 0), 있으면 `redraw(false)`다. `setPage`는 page를 범위 길이 이하로만 제한하고(음수 하한 없음) 현재 위치를 다시 적용하며, `setRange`는 두 끝을 정렬해(같으면 최대=최소+1) 저장하고 page를 길이로 제한한 뒤 **위치를 두 번** 다시 적용한다(패치 순서).
- **`attach`**: 기본 부착 → 방향 비트(bit0 가로)별 두께(기본 폭/높이의 CRT 절삭)·float32 반올림 두께·2×두께 → 세 자식 Push → 화살표/손잡이 크기 → 자식 mode 0 부착(증가 버튼 위치는 `float32(기본 길이 - 반올림 두께)`) → 현재 값/page로 **손잡이 사각형**(양 끝 포함, 위치 분자는 `position`이며 `position-minimum`이 아님)을 계산한다.
- **`dragVertical`**: 가로는 무동작, 세로는 `imul(범위, delta+현재 손잡이 top)/(기본 높이-2×폭)`의 CRT 절삭을 위치로 삼는다. **`handle`**: 화살표 click/repeat는 부모가 step을 처리하지 않을 때만 step만큼 이동하고 값이 바뀌면 변경 사건을 통지하며(항상 0 반환), 손잡이 step은 부모 전달 뒤 값이 바뀌면 통지한다. **`thumbHandle`**: 왼쪽 버튼만 처리하며 세로 release에서만 플래그(0x100)를 해제하고, 누름은 손잡이 밖이면 page 방향 3(아래)/4(위)를 부모에 통지, 안이면 이전 y/x를 저장하고 플래그를 켜 캡처한다(패치는 가로도 캡처). **`thumbBroadcast`**: 폴링한 왼쪽 버튼이 release이면 플래그만 해제하고 아니면 전달 y와 이전 y의 차이로 부모 Slider를 갱신해 변경을 통지한다.
- **연결**: `bindScrollGump`는 Slider 토큰의 Attach/사건/입력과 손잡이의 Handle/Broadcast만 가로채고 나머지는 이전 분배를 보존한다. `bindScrollingTextSlider`는 E423 `ScrollingTextGump`의 네 slider 경계를 실제 범위/페이지/위치/현재 값에 연결한다.

## 검사와 발견

새 `tests/scroll-gump.test.mjs` **12건 통과**(첫 실행 9/11). 호출을 기록하는 트리/사건/입력/수집기 대역과 실제 `GumpCreate` 기본 생성 노드로 장면을 만들었다.

- 생성/노드 검증, `setPosition`(경계 값·DWORD 합 감김·void/Push/Attach 순서·거부와 도달 시점), `setPage`/`setRange`/`position`, 세로/가로 `attach`의 크기/위치/손잡이 사각형(두께 9·범위 30·page 10·position 5 → page 27·position 13·사각형 {1,14,8,39}), 배율/소수 두께/음수 손잡이 높이/범위 0, `dragVertical`(곱 감김·무효 변환·가로 무동작), `handle`(감소/증가/반복/마스크/부모 처리/변경 없음/손잡이 step), `thumbHandle`(세로/가로 위·아래·안, release 플래그, 부호 있는 좌표, 정사각은 세로), `thumbBroadcast`(폴링 release·차이 0·음수 차이·폴링 경계 없음), 분배 연결과 `bindScrollingTextSlider`.

**최초 실패와 교정(제품 결함 아님, 시험 기대값 오류 2건):** `setPosition(0x7fffffff)`는 `page+position`의 DWORD 합이 음수로 감겨 상한 제한이 일어나지 않고(원본 그대로), 가로 손잡이 폭 0(범위 1·page 0·position 0)의 오른쪽 끝은 `position+page-1 = -1`이다.

**변형 확인:** 연산자/정수 상수 변형 125개 중 113개를 잡았고(처음 생존 39개 → 새 노드/전역 기본값·손잡이 플래그 정확값·정사각 손잡이·CRT 하위 32비트·무효 변환으로 보강), 남은 12개는 동등 변형이다 — 범위 밖 값에서 하위 32비트가 모두 0인 ±2^63 경계, `setPosition` 입구의 `| 0`이 이미 32비트로 줄여 가리는 33비트 변형, 같은 값으로 수렴하는 `position < minimum`/`maximum < page+position`/`length < page`/끝점 정렬 `<=` 변형이다.

## 남은 경계

- `ScrollGumpWidgets`(화살표 돌 버튼·손잡이 생성/Draw·Slider 생성자)와 `ScrollingTextGumpWidgets`·`StyleTextBufferStore`, `MenuGump*`·`RawMenuControls` 연결.
- 앱 메뉴(`www/ui/slider.js`·`scroll-text.js`·`style-text.js`)는 아직 간소화판이며 실제 미션은 플레이할 수 없다. 원본 파일 변경과 게임/브라우저 실행은 없다.
