# webpj 건설 예측 정리(RawConstructionClear) 이식 — WB04-E137

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백서른일곱 번째 부분이다. 배치 통지(원본 `004441b0`)가 조각을 놓기 직전에 부르는 로컬 예측 조각 정리(Construction.cpp `00441fd0`)를 옮긴 `o/RawConstructionClear`(cpppj 기준선 `1f00a25a`, 기준선 뒤 변경 없음)이다. 통지가 놓을 칸마다 그 자리에 먼저 놓여 있던 클라이언트 영역의 abstract 예측 조각을 하나씩 지운다. 선행은 E136 `decodeCanonType`·기존 `RawSquidFinder`·E134 통지 어댑터다.

## 1. 옮긴 것

`www/core/raw-construction-clear.js`

- `cellArea(x, y, footX, footY)` (0041d770) — 칸 좌표와 타입 발자국으로 탐색 범위를 구한다. `(x−(폭−1), y−(높이−1))` 을 단정도로 저장해 1~255 로 자른 점과 칸 좌표를 잇는 사각형이며 칸 좌표가 지도 밖(0 이하 또는 256 이상)이면 자른 점 하나가 된다. 작은 쪽은 0방향 절삭, 큰 쪽은 `0.9999899864196777f` 를 더해 절삭한다.
- `ConstructionClearState` — `authority`(00540bc4)·`bridgeType`(005411a0, 82)·`patternTypes`(디코더가 패턴으로 분기하는 여덟 타입 전역)·`sharedAuthority`(실제 Client boss)·`decoderDiagnostics`(디코더 checking·진단 장치). `authorityNow()` 가 후보 비교마다 현재 권한을 읽는다.
- `RawConstructionClear(pool, hash, types, frames, state, hooks)` — 10.78 `SidPool`, 타입별 `{ frames, defaultFrame }`, 필수 경계 `destroy(sid, flags)`(관찰용 `scan(area)` 선택)를 생성 때 확인한다.
  - `clear(request)` (00441fd0): 요청 `{ type, x, y, argument, direction, player }` 로 타입별 디코더를 만들고, 디코더의 유효 칸마다 [칸 범위 → `scan` → 실제 `RawSquidFinder` 탐색 → 후보 판정]을 한다. 후보는 extra(+40) abstract 비트가 있고 번호가 5 이상 서버 영역(15000) 미만이며 타입이 같거나(권한이 없으면 타입 무관) 해야 하고, 조건에 맞는 첫 후보 하나만 지운 뒤 그 칸을 끝낸다. 삭제 플래그는 후보가 통지와 같은 조각(타입 같고 프레임 DWORD[+36]이 디코더 프레임과 같거나, 다리 타입이면 프레임 코드의 방향 글자가 같음)이면서 소유자(+34)가 통지 플레이어와 같을 때만 `0x2000000`, 아니면 0 이다. 타입 번호가 표 밖이거나 255 초과면 거부한다.
  - 후보를 지운 뒤 해시에서 빼는 일은 `destroy` 경계의 몫이다(finder 가 next 를 미리 저장하므로 삭제 중에도 순서가 유지된다).

## 2. 검증

`tests/construction-clear.test.mjs`(7건):

1. `constructionclear-x86.tsv` 의 `originals` 420행(`originalCD`·`original1037` 은 다른 판본이라 제외) — 통지 입력 13개(타입·인자·방향·좌표·플레이어·권한·발자국·프레임 표 윤곽 0~3·기본 프레임·다른 타입 발자국)와 후보 목록에서 칸마다의 탐색 범위(A)와 지운 후보·삭제 플래그(K)의 순서가 모두 같다.
2. 칸 범위: 발자국 확장, 1~255 자르기(`0.5, 2칸 → 0,0,1,1`), 칸 좌표가 0 이하/256 이상/음수일 때 자른 점 하나.
3. 한 칸에서 첫 후보 하나만 지움(서버 영역·abstract 아님·다른 타입 제외, 체인 머리가 마지막 등록), 소유자·프레임이 다르면 플래그 0, 권한이 없으면 다른 타입도 후보. 삭제 경계 누락·다른 판본·타입 번호 300 거부(효과 없음).
4. 다리 타입: 프레임이 달라도 방향 글자가 같으면 같은 조각 플래그(`118` 디코더 프레임 ↔ 후보 `119`), 방향 글자가 다르면 0(`131`), 같은 프레임은 비교 없이 같은 조각, 후보 프레임이 표 밖이면 거부.
5. 공유 boss 가 바뀌면 다음 호출의 후보 비교에서 다른 타입 후보가 지워진다.
6. 정리가 finder 와 삭제보다 먼저 같은 현재 Canon 진단(금 간 프레임 0x325)을 선택한다(중단 시 효과 없음).
7. E134 `makeConstructionNoticeHooks` 로 실제 정리를 통지에 이으면 `K:60:33554432` → 배치 → 갱신 순서가 된다.

최초 실패와 교정: 새 모듈은 420행 fixture 를 첫 실행에 통과했다. 다리 시험의 프레임 번호(방향 글자마다 13개)를 한 번 고쳤다.

## 3. 경계와 남은 것

- 실제 예측→확정→통지→정리→배치 전체를 잇는 cpppj 통합 시험(`ConstructionClear_ReplacesLocalPredictionWithConfirmedPieces`)은 `RawConstructionPlace`·`RawConstructionConfirm` 이식 뒤 옮긴다.
- 10.72(CD)·10.37 경로(프레임 BYTE +34, 소유자 +32)는 범위 밖이다.

## 4. 다음 시작점

`RawPriestPlacementGeometry` → `RawConstructionPlace`, 이어서 `Confirm`·`Request`·`Local`·`Command`·`Data`·`Builder`, `RawManaBoltArrival`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
