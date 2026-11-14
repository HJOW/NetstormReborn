# webpj 메뉴 선택 통지·클릭 처리 MenuGumpInput (WB04-E431, 2026-11-14)

RawMenuControls 선행 **MenuGumpInput 328줄(.cpp/.h, partial→port)**을 새 모듈 [`www/ui/menu-gump-input.js`](../webpj/www/ui/menu-gump-input.js)로 이식했다. 기준선 `1f00a25a` 전체 소스를 읽었다([정적 복원 문서](exe/cpp-menu-gump-interaction.md)). **전용 PE fixture는 없다.** cpppj에도 직접 시험이 없고 아래 검사는 기준선 소스 계약과 손으로 계산한 좌표/호출 순서, 실제 `GumpCollection`(위치/로컬 변환)·`MenuGump`(항목 위치/확장 시작)·기본 생성 저장소 연결이다. 원본 직접 관측 수치로 세지 않는다. CD 10.72의 unsigned float 비트 하한(-0 허용)은 옮기지 않았다.

## 기준선 뒤 변경 (3.6절)

기준선 뒤 변경은 없다.

## 구현

- **`broadcast`(마우스 이동의 선택 통지)**: 사건 좌표를 signed DWORD로 읽어 `screenToLocal`로 로컬 좌표를 구하고, 화면 밖 해제 비트(`0x4000000`)가 없고 **로컬 경계 안**(패치는 ordered `>= 0`, 폭/높이는 strict)이며 **메뉴 식별(`command == signed 식별 바이트`)인 자식이 점을 덮지 않을 때만** 행을 고른다. 가로는 `ftol(local.x / itemWidth)`, 세로는 offset/높이 구간(없으면 마지막 인덱스), compact는 0번이다. 선택이 바뀌면 old 행을 갱신(세로) → 강조 해제 → 새 행(비활성 제외)을 갱신 → 선택·강조 설정이며 가로는 마지막에 한 번 전체 Redraw를 요청한다. 현재 선택/항목을 단계마다 다시 읽는다.
- **`handle`(클릭)**: 확장 중이거나 종류(마스크 `0x03ffffff`)가 왼쪽/오른쪽 누름이 아니거나 release면 0이다. 그 밖에는 가상 통지 → 선택 없음이면 (외부 누름 삭제 비트가 있으면 Kill 하고) 1 → 명령 비트면 인자(명시 인자 비트가 없으면 flags 전체)로 명령 큐 + `openSubGump.wav` → `close`(자식/유지 비트가 없을 때 강조도 지움) → 동적 항목(자식 없음)은 요청 사건·메뉴 serial·인자로 가상 입력을 부르고 반환 토큰의 ID를 항목에 저장·숨김·Attach·자식 비트 → 자식 항목은 Push·visible·**항목 위치 Attach**·깊이 -1·확장 시작(부모와 반대 방향)·`openGump.wav`·Redraw → 자식/유지가 아니면 **메뉴 식별 부모 체인을 올라가며 루트 메뉴 경계에서 그 가지를 닫는다**(루트에 닿으면 기본 노드 조회 실패) → 명령 뒤 삭제 비트면 명령일 때 Kill + 부모 체인 닫힘 통지 → 명령이면 팝업 닫기, 처리하면 1이다. 콜백이 바꾼 현재 선택은 단계마다 다시 조회한다.
- **`close`/`closePopups`/`redrawItem`**: `close`는 표시 중 자식을 재귀 → redraw → 숨김 순서로 닫고 동적 자식은 숨김 여부와 무관하게 Kill 한 뒤 항목의 자식 ID/비트를 지우며 선택 인덱스는 보존한다(재귀 순환은 거부, signed 음수 항목 수는 반복 없음). `closePopups`는 실제 등록표 루트의 직계 자식 중 sticky 팝업이고 dead가 아닌 것만 Kill 하며 next를 Kill 전에 읽는다. `redrawItem`은 visible 부모 체인에서만 compact 전체/행 영역(양수 폭·높이)을 변경 영역으로 요청한다.
- **`bindMenuGumpInput`/`bindMenuGumpInputSound`**: 사건/캡처 입력의 기존 반환 DWORD를 보존한 채 메뉴 토큰만 실제 `handle`로 보내고 프레임 통지(`broadcast`)도 메뉴만 교체하며, 음향은 이름만(채널/거리/우선순위 0) 기존 SoundPlayer에 요청한다.

## 검사와 발견

새 `tests/menu-gump-input.test.mjs` **20건 통과**(공용 장면 `tests/menu-gump-support.mjs`). 수집기·메뉴 크기/확장 처리기·기본 생성 저장소는 실제이고 수명/사건/트리/가상 통지/명령 큐/소리는 호출 기록 대역이다. 시험 메뉴는 (100,50) 80×60 세로 3항목(offset 0/20/40, 높이 20)이며 메뉴 flags에는 항상 bit0를 함께 켜 방향/동작 비트가 bit0에 걸리지 않음을 확인한다.

**최초 실패와 교정(제품 결함 아님, 시험 기대값 오류 3건):** 이벤트 종류 마스크(`0x03ffffff`)가 `0x04000000`을 포함하지 않는다는 점, 기본 생성의 `flags=1`로 모든 시험 노드가 visible이라는 점, 확장 시작이 idle 등록 비트(`0x80`)를 자식에 남긴다는 점을 바로잡았다.

**변형 확인:** 연산자/정수 상수 변형 146개 중 139개를 잡았다(처음 생존 34개 → bit0를 켠 메뉴 flags·ftol NaN/무한대/범위 밖/32비트 감김·정확한 flags 값·항목 수 바로 뒤 칸·행 경계 정확값·가로 바깥 선택·좌표 signed 전달·전역 기본값 보강). 남은 7개는 동등 변형이다 — `ftol`의 ±2^63 정확한 경계와 같은 값의 리터럴 2개, 이미 mask 비트로 걸러지는 release bit0, 범위 밖 ID가 어느 쪽이든 거부되는 `>=`/`>`, 곧바로 덮어써지는 `height` 초기값이다.

## 남은 경계

- `MenuGumpWidgets`와 `RawMenuControls` 연결. 실제 OS 입력 큐/명령 큐/음향 장치 연결과 앱 메뉴(`www/ui/menu-list.js`·`popup-menu.js`)는 아직 간소화판이며 실제 미션은 플레이할 수 없다. 원본 파일 변경과 게임/브라우저 실행은 없다.
