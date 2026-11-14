# webpj 메뉴 출력 MenuGumpDraw (WB04-E430, 2026-11-14)

RawMenuControls 선행 **MenuGumpDraw 201줄(.cpp/.h, partial→port)**을 새 모듈 [`www/ui/menu-gump-draw.js`](../webpj/www/ui/menu-gump-draw.js)로 이식했다. 기준선 `1f00a25a` 전체 소스를 읽었다([정적 복원 문서](exe/cpp-menu-gump-interaction.md)의 출력 절). **전용 PE fixture는 없다.** cpppj에도 직접 시험이 없고 아래 검사는 기준선 소스 계약과 손으로 계산한 화면 좌표, 실제 `GumpCollection`(위치/경계)·`GumpClipScope`·`MenuGumpLayout.measureItem` 연결이다. 원본 직접 관측 수치로 세지 않는다. CD 10.72의 bottom Y 정수 유지(004523b0)는 옮기지 않았다.

## 기준선 뒤 변경 (3.6절)

기준선 뒤 변경은 없다.

## 구현

- **`MenuGumpDraw.draw`**: 자체 경계 클립(`GumpClipScope`) → 기본 배경 `draw(token)` → 현재 `position` → 항목마다 현재 문맥을 복사해(`context[1]=0`, `context[2]=현재 흰색`, 굵게는 `context[3]=1`, 비활성은 `context[2]=비활성 색`·`context[0]=0`) 강조/그림/글자/하위 표식을 출력하고 마지막에 클립을 복구한다(예외에서도). 항목 수는 signed로 읽고 매 반복·출력 콜백 뒤 현재 항목 수/flags/노드 필드를 다시 읽으며 40칸 범위 밖은 거부한다. 클립 보호는 부모 필터 없이 메뉴 자신의 Draw 몸체에서만 쓰고 표시/삭제 필터는 수집기에 맡긴다.
- **선택 강조**: 선택(`8`)이고 구분선(`0x10`)이 아닌 항목은 세로 `[2, offset+2, 폭-2, 높이+offset-2]`·compact `[2,2,폭-2,높이-2]`·가로 `[index×항목 폭+2, 2, (index+1)×항목 폭-2, 높이-2]`를 float32로 저장한 뒤 화면 `rect`(X/top은 정수 Y, **bottom만 float32로 한 번 저장한 Y**)로 바꾸고 클립한 다음 현재 메뉴 type/frame으로 `0x2100000` 배경을 그린다(16777217에서 top/bottom이 한 칸 달라지는 입력으로 확인).
- **구분선**: 세로에서만 `y = ftol((offset+여백[2]) - (목표 높이-현재 높이) + 높이×0.5 - 1)`, `right = ftol(폭+2-4)`의 로컬 `[2,y,right,y+1]`을 화면으로 바꿔 두 pass로 그린다. 두 번째 pass는 이전 rect의 top/bottom을 1씩 늘리며 flags `0x2100000`/`0x2080000`에 **전역 배경 type/frame**을 매 pass 읽고 pass마다 클립을 복구한다.
- **글자/체크/하위 표식**: 스타일 항목은 저장 조각을 override 없이 출력하고 일반 항목은 60바이트 항목 직렬화를 해석한 현재 문맥으로 출력한다. 가로는 `x = ftol(index×항목 폭+여백[0]+x)`, `y = ftol(y+여백[2])`이고 세로는 목표/현재 크기 차이만큼 보정한다. 체크는 글자 앞에 `(x+ftol(여백[0]×0.25+보정)+5, y+ftol(offset+여백[2]+보정)+7)`로 표식 훅(`markerType`)을 부르고(훅 없으면 글자 전에 거부), 자식/동적 자식은 글자 뒤에 `" >"`를 `measureItem` 폭만큼 오른쪽에 같은 문맥으로 `drawGump`한다. 가로 메뉴는 체크/하위 표식을 그리지 않는다.
- **`bindMenuGumpDraw`/`bindMenuGumpDrawAssets`**: 메뉴 토큰만 실제 Draw로 보내고 다른 타입은 이전 처리기를 쓴다(없으면 거부). 체크 표식은 실제 타입 번호(70~)의 **J 구간 첫 번호+2** 물리 프레임을 기존 장식 원색 픽셀 대상에 출력하며 J 구간이 없으면 -1+2=1이고 타입 범위 밖은 거부한다.

## 검사와 발견

새 `tests/menu-gump-draw.test.mjs` **15건 통과**(공용 장면 `tests/menu-gump-support.mjs`). 위치/경계 수집기·클립 상태·메뉴 레이아웃·기본 생성 저장소는 실제이고 배경/글자/StyleText/체크 표식은 호출 기록 대역이다. 시험의 메뉴는 80×60, 목표 60×26, 여백 15/8/2/5(서로 달라 인덱스 오류를 구별)로 둔다.

**최초 실패와 교정(제품 결함 아님, 시험 기대값 오류 1건):** 가로 강조의 오른쪽 끝을 `(index+1)×25-2 = 48`(화면 148)로 잘못 계산했다.

**변형 확인:** 연산자/정수 상수 변형 95개 중 91개를 잡았다(처음 생존 33개 → bit0만 켠 항목/메뉴 flags·ftol NaN/무한대/범위 밖/32비트 감김·서로 다른 여백·짝수 항목 수·전역 기본값 보강). 남은 4개는 동등 변형이다 — `ftol`의 ±2^63 정확한 경계(어느 쪽이든 하위 DWORD 0)와 같은 값으로 평가되는 리터럴 2개다.

## 남은 경계

- `MenuGumpInput`·`MenuGumpWidgets`와 `RawMenuControls` 연결. 실제 체크 표식 SHP 픽셀 출력(기존 장식 원색 캐시 연결)과 앱 메뉴(`www/ui/menu-list.js`·`popup-menu.js`)는 아직 간소화판이며 실제 미션은 플레이할 수 없다. 원본 파일 변경과 게임/브라우저 실행은 없다.
