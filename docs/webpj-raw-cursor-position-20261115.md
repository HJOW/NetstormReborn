# 커서 위치 갱신 이식 (WB04-E548)

F(입력·커서·선택)의 "커서" 중 `client/RawCursorPosition`(원문 10.78 Cursor 446b50의 정수 마우스 입력·4971e0 역투영·41d770 절삭·497490 카메라 픽셀)을 `webpj/www/core/raw-cursor-position.js`로 옮겼다.
기준선 `225553df`와 현재 cpppj 소스가 같음을 확인했다. E547(`RawCursorActions`)과 같은 `CursorDropState`·`CursorPlacementState`를 공유하고, 기존 `RawCursorLifecycle`·`RawCursorPreviews`·`RawCursorPlacement`와 사슬로 이어진다.

## 옮긴 범위

- `update(mouseX, mouseY)`(446b50): 두 int32 인자로 화면점을 `((마우스 + (기준점 − 그림 크기/2)) − 16, … − 11)`(DWORD 감김, 크기/2는 0쪽 절삭)로 갱신한다. 월드 위치는 `screenToWorld`로 구하고 `snapToGrid`(55a2d8)가 켜져 있으면 `truncatePoint`를 적용한다.
  위치가 바뀌면 잠금 깊이를 다시 읽어 0이면 `activate(0)` → 위치 쓰기 → 0이면 목록 생성과 커서 선택(`legalGump`이면 3, 아니면 2)을 한다(`Activate`·생성 콜백이 깊이/Gump를 바꿀 수 있어 매번 재조회).
- `screenToWorld`(4971e0): `fround((정수+11)×역배율Y + 카메라Y)`, `fround((정수+16)×역배율X + 카메라X)`. 역배율은 단정도 비트 0x3d800000(X)·0x3dba2e8c(Y)이며 정수를 먼저 float로 좁히지 않고 마지막 저장에서만 좁힌다.
  `cameraFromPixels`(497490)는 scroll pixel을 같은 방식으로 float 카메라 비트로 바꾼다. `truncatePoint`(41d770)는 `_ftol`(signed64 하위 DWORD, 불확정값 0)을 float로 다시 저장한다(반올림/floor 아님).
- `makeCursorPositionHooks(lifecycle, previews, selectCursor)`: 같은 풀의 실제 `activate`·`create` 두 몸체와 커서 선택기를 연결하고 서로 다른 풀·선택기 누락은 거부한다. `bindCursorPosition`은 Take 꼬리가 읽은 마우스 두 정수를 그대로 전달한다.
- 카메라/스냅은 사본을 만들지 않고 현재 값을 읽는다(카메라는 제자리 갱신하는 두 칸 배열 또는 함수, 스냅은 `{ value }` 상자 또는 함수). 도달한 효과 누락은 거부하고 10.72 판본은 거부한다.

## 시험

`webpj/tests/raw-cursor-position.test.mjs` 3건:

1. 원본 관측 `cursor-position-x86.tsv`의 **Point 623행**: 갱신·역투영·절삭 세 연산 × 18 DWORD 독립 입력(마우스·기준점·그림 크기·카메라·스냅·이전 위치·잠금·Gump·재진입 변경)에서 효과 호출 순서와 15 DWORD 전체 상태·결과 비트. **첫 실행에서 일치**했다.
2. **Chain 64행**: 실제 `RawCursorLifecycle`·`RawCursorPreviews`·`RawCursorPlacement`를 이은 두 번 이동 직후의 52+11 DWORD 상태와 접근한 슬롯 바이트 전체(기존 `CursorPreviewScene` 공급자 재사용). 첫 실행 일치.
3. 계약: 현재 카메라/스냅 비복사·Take 꼬리 인자 순서·필수 의존·판본·단일 풀·선택기 거부, `cameraFromPixels`/`truncatePoint` 직접 값.

**변형 확인:** 역배율 X/Y·오프셋 11/16·카메라 순서·중간 float 저장(X/Y)·절삭 범위/비유한/하위 DWORD·카메라 픽셀 배율·기준점 −16/크기 절반(trunc↔floor)·스냅 조건·위치 비교 X/Y·첫/둘째 잠금 재조회·활성 인자·위치 쓰기 순서·커서 번호·판본/단일 풀/선택기 거부·Take 꼬리 등
25가지를 넣어 23가지가 시험에서 잡혔다(처음 살아남은 Take 꼬리 인자 순서는 3번 시험으로 잡았다). 남은 2개는 동등 변형이다: ① 입력이 단정도 float라 2^56 이상은 하위 32비트가 0이어서 `_ftol` 범위를 2^62로 줄여도 같고, ② `Math.trunc(v)|0`은 `BigInt` 하위 32비트와 정확히 같다.

`webpj/tests/fixtures/manifest.json`에 `cursor-position`(687행: Point 623·Chain 64, 원본 10.78) 항목을 추가했다.

## 남은 경계

- 카메라 원본 소유자(`MetadisplayDirtyView.camera`)와 실제 `Take` 호출, `selectCursor`의 하드웨어 커서 선택은 호출자가 공급한다(PORT_MAP `port`). 앱 수명에는 연결하지 않았다.
