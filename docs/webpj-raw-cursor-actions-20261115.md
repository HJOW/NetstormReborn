# 커서 잠금/해제/회전 이식 (WB04-E547)

F(입력·커서·선택)의 "커서" 중 `client/RawCursorActions`(원문 10.78 Cursor 446a10 잠금·446a40 해제·446a70 회전)를 `webpj/www/core/raw-cursor-actions.js`로 옮겼다.
기준선 `225553df`와 현재 cpppj 소스가 같음을 확인했다. 선행 부품(`raw-cursor-drop.js`의 `CursorDropState`, `raw-cursor-lifecycle.js`의 `CursorLifecycleState`)을 같은 현재 전역으로 공유한다.

## 옮긴 범위

- `lock(skipCursor)`(446a10): 현재 잠금 깊이가 0일 때만 `activate(0)`하고 `skipCursor`가 0이면 커서 1을 선택한 뒤 깊이를 1 올린다(Activate가 깊이를 바꿔도 처음 분기의 판단과 마지막 증가를 유지).
- `unlock()`(446a40): 깊이를 DWORD로 1 내리고 0이 되면 미리보기 목록을 다시 만든 뒤 현재 `legalGump`(생성 콜백이 바꾼 값)에 따라 커서 3 또는 2를 선택한다. 0에서 내리면 0xffffffff가 되어 아무 효과도 없다.
- `rotate(step)`(446a70): 처음이면 `activate(0)`·커서 1 → 깊이 증가 → 방향을 `(방향+8+step)&0x80000007`로 접고 음수면 원본의 `((x-1)|0xfffffff8)+1` 보정 → `rotatepiece.wav` → 깊이 감소 → 0이면 목록 재생성·커서 선택 →
  현재 논리 TYPE 번호가 표 크기(188) 이상이면 거부 → TYPE 그룹이 1이거나 선택 타입이 특별 타입이면 사정거리 프로세스를 삭제(`removeRange`)하고 새로 만들어(`createRange`) `cursorGump`에 저장.
- 특별 타입(541288)·프로세스 타입(54113c)은 호출 시점마다 현재 값을 읽는 `{ value }` 상자 또는 함수로 받는다. 모든 콜백 뒤 실제 저장소를 다시 읽는다. 도달한 미연결 경계는 거부한다(`/미연결/`).

## 시험

`webpj/tests/raw-cursor-actions.test.mjs` 3건:

1. 원본 관측 `cursor-actions-x86.tsv` **289행**: 세 부모(Lock·Unlock·Rotate) × 14 DWORD 독립 입력(깊이·인자·방향·Gump·TYPE 그룹·선택 타입·특별 타입·재진입 변경 조합)에서 외부 효과 호출 순서와 전체 전역 아홉 개. **첫 실행에서 289행 전부 일치**했다.
2. 계약: TYPE 표 크기 거부·미연결 경계 거부·0에서의 해제(DWORD 감김)·함수형 전역과 방향 접기.
3. 계약(변형 확인 뒤 보강): 논리 TYPE 번호 188은 `RangeError`(187은 통과).

**변형 확인:** 잠금/해제/회전의 첫 분기·인자·감소량·커서 번호·방향 8·마스크·음수 보정/보정식·음향 이름·재생성 조건·TYPE 범위·그룹·특별 타입·범위 인자·Gump 저장·SID·표 크기 등 19가지를 넣어 모두 시험이 실패함을 확인했다
(처음 살아남은 TYPE 범위 경계 하나를 3번 시험으로 잡았다).

`webpj/tests/fixtures/manifest.json`에 `cursor-actions`(289행, 원본 10.78) 항목을 추가했다.

## 남은 경계

- 실제 `Activate`(445aa0)·`createPreviews`(445b30)·커서 선택기(4a4410)·`removeRange`/`createRange`(4b1a90/474900)는 호출자가 공급한다. 앱 수명에는 아직 연결하지 않았다(PORT_MAP `port`).
