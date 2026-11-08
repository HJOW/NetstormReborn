# webpj 건설 효과(RawConstructionEffects)·선택 어댑터(RawConstructionSelection) 이식 — WB04-E135

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백서른다섯 번째 부분이다. 건설 요청이 쓰는 유닛 밀어내기(원본 `00443800`)와 섬 실패 정리(`0046e6c0`)를 옮긴 `o/RawConstructionEffects`와, 확정의 그래프 기준점·통지의 소유 사제 조회를 실제 몸체에 잇는 두 어댑터 `o/RawConstructionSelection`(cpppj 기준선 `1f00a25a`, 기준선 뒤 세 파일 모두 변경 없음)이다. 선행은 기존 `RawSquidFinder`·`SidPool`·`SquidHash`, E134 통지·E132 건설 프로세스 어댑터, E57 `RawPlayerPlacementAnchor`, E66 `RawPriestLookup`이다.

## 1. 옮긴 것

`www/core/raw-construction-effects.js`

- `ConstructionEffectsState` — 관계 표 `alliances`(00595200, `[요청 플레이어×9+후보 소유자]` 81칸)·섬/표면/종유석/그루터기 타입(94/157/95/96)·`blocked`(0054db84)·`server`(00540bc0)와 같은 이름의 `shared*` 함수. `*Now()` 접근자가 사용 위치마다 현재 값을 읽는다.
- `RawConstructionEffects(pool, hash, types, spots, state, hooks)` — 10.78 `SidPool`·타입 표 188칸·spot 지도(256×256 이상)·필수 경계 9개(`pathType`·`clearPath`·`unpop`·`pop`·`fall`·`sound`·`destroy`·`notify`·`broadcast`; `assertion`·`scan` 은 선택)를 생성 때 확인한다.
  - `displace(x, y, type, player)` (00443800): float 발자국 사각형(시작점이 지도 밖이면 자른 반대 점 하나, 폭이 3 이상이면 사방 1칸 축소)을 작은 쪽 절삭/큰 쪽 0.99999 더해 절삭해 실제 finder 에 넘긴다. 후보마다 현재 genus(0x218000)를 다시 읽고, 가운데 우선(0x200000)이면 `(x−(폭−1)/2 정수 나눗셈, y+1)` 칸을 먼저, 이어서 아래 세 줄(오른쪽부터 폭+1칸 역순)과 오른쪽 세 열(아래부터 높이+1칸 역순)에서 열린 spot(0x10 없음, 6 중 하나)을 찾는다. 칸이 없으면 건너뛰고, 소유자가 요청 플레이어와 다르면 관계 표가 0 이고 적대 허용(0x8000)이 없을 때 거짓을 돌려준다(앞서 옮긴 후보는 되돌리지 않음). 옮길 때는 경로 타입 조회 → 경로 삭제 → Unpop(0) → genus 재조회 → Pop 순서다.
  - `dropIsland(x, y, flags, player)` (0046e6c0): 억제면 무동작. 서버 역할이고 `flags&0x50` 이 없으면 좌표를 `ftol(좌표×65536)` 16.16 DWORD 로 방송한 뒤 현재 섬 타입 발자국으로 범위를 구해, 소유자가 같은 표면 조각/종유석은 추락 표시와 소리(`flags&0x1000` 없을 때, 표시 뒤 현재 좌표) 뒤 `flags|0x48` 로 삭제, 그루터기는 `flags|0x48` 로 삭제, 섬은 `flags` 로 삭제, 삭제 뒤 현재 genus 에 0x10000 이 있으면 알림한다. 조각이 있었는데 9/1/1 이 아니면 원본 assert 보고(0x7b/0x7c/0x7d)를 한다.
- `makeConstructionEffectsRequestHooks(pool, effects, hooks)` — 요청의 `displace`/`displaceCd`/`cancel` 을 이 몸체에 잇는다(풀 불일치 거부, 나머지 요청 경계 보존).

`www/core/raw-construction-selection.js`

- `makeConstructionAnchorHooks(pool, anchor, hooks)` — 확정의 `findBuilder(player, type, x, y)` 를 `anchor.locate` 로 잇고 SID 를 WORD 로 전달한다.
- `makeConstructionPriestLookupHooks(pool, lookup, hooks)` — 통지의 `findPriest(player)` 를 `lookup.find` 로 잇는다. 둘 다 풀 불일치를 거부한다.

## 2. 검증

`tests/construction-effects.test.mjs`(8건):

1. `constructioneffects-x86.tsv` 의 `originals` 1178행(`originalCD`·`original1037` 은 다른 판본이라 제외) — 밀어내기 770행·섬 정리 408행의 입력 15개(타입·좌표·발자국·플레이어·flags·억제·서버·spot 윤곽 13가지·관계 표·변동 0 1162행/1 8행/2 8행)와 후보 목록에서 반환값과 효과 순서(A/R/C/U/P/F/S/K/N/I·B/E)가 모두 같다.
2. 계약: 필수 경계 9개 각각·지도 크기·타입 표 171칸·다른 판본·다른 풀의 요청 어댑터·타입 표 밖·비유한 좌표 거부(효과 없음), 억제 중에는 비유한 좌표도 무동작.
3. 요청 어댑터: 세 효과가 몸체로 가고(`A:18:19:20:21`) 나머지 경계는 그대로이며, 억제 중에는 `cancel` 이 무동작이다.
4. Dude 의 foot 0 을 `x..x+1` 사각형으로 펼치고(`A:20:21:21:22`) 음수 발자국은 거부한다.
5. 추락 뒤 현재 좌표(소리 인자)·삭제 뒤 현재 genus(알림)·방송 뒤 현재 섬 발자국(`A:17:19:20:21`)을 다시 읽는다.
6. 공유 server 역할의 현재 값으로 방송을 결정하고 `flags&0x50` 이 있으면 방송하지 않는다(10.78 만).
7. 선택 어댑터: 기준점·사제 조회를 같은 풀에서만 잇고 SID 를 WORD 로 줄여 전달하며 나머지 경계를 보존한다.
8. E132 `makeConstructionProcessHooks` 의 유닛 밀어내기 경계가 실제 `displace` 로 이어지고(적대 거부 반환은 버림) 다른 풀의 효과는 거부한다.

최초 실패와 교정: 새 모듈은 1178행 fixture 를 첫 실행에 통과했다. 시험 쪽 기대 문자열(요청 어댑터의 탐색 범위)을 한 번 고쳤다.

## 3. 경계와 남은 것

- 실제 요청→밀어내기→확정 실패→섬 정리→공통 삭제/풀 반납을 잇는 cpppj 통합 시험(`ConstructionEffects_ConnectsRequestMovementAndFailureCleanup`)은 `RawConstructionRequest` 이식 뒤 옮긴다.
- 선택 어댑터의 `ConstructionConfirm`·`ConstructionNotice` 와의 통합(cpppj `PriestLookupTests`)도 `RawConstructionConfirm` 이식 뒤 시험한다.
- 10.72(CD) 경로(탐색 거리 2, 소유자 +32, void 반환)와 10.37 은 범위 밖이다.

## 4. 다음 시작점

`RawConstructionClear`·`RawConstructionPlace` 는 `CanonTypeDecoder`(패턴 분기 여덟 타입·방향·프레임 해석)와 `RawPriestPlacementGeometry` 가 선행이므로 그 두 파일부터 옮긴다. 이어서 `Confirm`·`Request`·`Local`·`Command`·`Data`·`Builder`, `RawManaBoltArrival`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면, `RawPathProcess` 계열 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
