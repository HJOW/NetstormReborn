# webpj 일반 픽셀 모양 getter(RawCanonPixelShape) 이식 — WB04-E147

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백마흔일곱 번째 부분이다. 일반 자산과 다리·섬·받침·전투·영역 패턴 전체의 픽셀 범위 getter(`0043cbd0`) — 디코더의 모든 유효 칸에 SHP 프레임 머리와 타입 핫스폿을 적용해 한 개의 화면 픽셀 범위와 기준점 보정으로 합치는 — 를 옮긴 `o/RawCanonPixelShape`(cpppj 기준선 `1f00a25a`, 기준선 뒤 변경 없음을 `git diff --numstat` 으로 확인)이다. E146 기하 모듈과 같은 Canon 배치 계열이며, 선행은 E136 `raw-canon-type.js` 이다.

## 1. 옮긴 것

`www/core/raw-canon-pixel-shape.js`

- `new RawCanonPixelShape(pool, types, frames, shapes, hotspots, geometryState, state)` — 10.78 `SidPool`, 188칸 타입 표(`flags1`·`maxHitPoints`), 타입별 `{ frames, defaultFrame }`, 타입별 `{ loaded, frames[{ width, height, hotspotX, hotspotY }] }`, 타입별 `{ x, y }` 핫스폿, `{ patternTypes[8], decoderDiagnostics }`, `{ scaleX, scaleY }` 배율을 받는다. 길이가 다르거나 표가 없으면 거부한다. 값은 호출 때마다 다시 읽는다.
- `measure(type, argument, direction, explicitFrame)` — 타입별 Canon 디코더가 만든 모든 유효 칸을 원본 순서로 돌며 합집합 범위 `{ bounds: { left, top, right, bottom }, anchorX, anchorY }` 를 만든다. 시작값은 `bounds = (1000, 1000, 0, 0)`, 기준점 보정은 현재 배율이다(빈 모양이면 그대로 돌려준다).
  - 핫스폿 차이 `dx/dy = (프레임 기준점 − 타입 핫스폿)`의 int32 감싸기, 칸 좌표×배율은 double 로 곱하고 Y 만 단정도로 저장한다.
  - 왼쪽/위: 넓은 값이 현재 최솟값보다 작으면 기준점 보정(`anchorX = float(dx)`, `anchorY = float(dy)`)을 갱신하고, 단정도로 줄인 값이 최솟값 이하이면 최솟값을 저장한다. 오른쪽/아래는 (단정도 왼쪽 + 폭), (넓은 위 + 높이)가 최댓값 이상이면(같은 값도) 단정도로 저장한다.
  - 프레임 확인(`00419850`): SHP 가 없거나 물리 프레임이 범위 밖이거나, 논리 프레임 수(코드 수, `0x440000` 이중 프레임 플래그면 두 배)를 넘거나 타입의 최대 체력이 두 해제 표식(`0xdddddddd`·`0xcdcdcdcd`)이면 원문 assert 대신 거부한다. 비유한 배율은 첫 유효 칸에서 거부하고 빈 모양에서는 거부하지 않는다.

이 모듈은 DOM 을 쓰지 않으며 10.72(CD)의 중간 단정도 저장·엄격 최댓값 경로는 범위 밖이다.

## 2. 검증

`tests/canon-pixel-shape.test.mjs`(3건):

1. `canonshape-x86.tsv` 의 `originals` 3150행(실제 PE 관측) — 타입 전역 9종×인자×방향(홀수·signed −1 포함)×프레임 표 3종(정상·역순·빈 표)×전역 별칭×기본 프레임×SHP 머리 입력 3종(폭/높이/기준점 변화, 16비트 경계 감싸기, 모두 0)×배율×핫스폿에서 범위 네 값과 기준점 보정 두 값의 단정도 비트 여섯 개가 모두 같다. `originalCD`·`original1037` 행은 사용하지 않았다.
2. 일반 자산(사제 플래그 없음)에서 SHP 머리·핫스폿·폭 변경이 다음 호출에 반영됨, 기본 프레임 없음+SHP 없음의 빈 범위(`1000,1000,0,0`, 기준점 = 배율), 표 밖 타입·누락된 SHP 표 거부.
3. 로드되지 않은 SHP·논리 프레임 범위 밖·물리 프레임 범위 밖·두 해제 표식 거부, 이중 프레임 플래그가 논리 프레임 수를 늘림, 비유한 배율은 유효 칸이 있을 때만 거부.

최초 실패와 교정: 새 모듈은 3150행 fixture 와 나머지 검사를 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 사제 전용 형제인 `RawPriestPlacementShape`(비패턴 사제 getter)와 SHP·핫스폿을 실제 자산 로더에서 공급하는 `PriestPlacementAssets`(client)는 별도 단위다. 이 모듈은 호출자가 넘긴 표를 읽을 뿐 자산을 읽지 않는다.
- 배치 계층의 지형·권한·관계·주변·충돌·미리보기 훅 본체는 별도 단위다. 10.72(CD)와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

`RawCanonPlacement*`(Terrain·Permission·Relations·Surrounding·Collision·Preview·Pipeline)·`RawPriestPlacement*`, `RawPriestArrival`·`RawCarrierArrival`, `RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
