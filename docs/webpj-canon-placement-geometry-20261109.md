# webpj 일반 배치 모양 기하(RawCanonPlacementGeometry) 이식 — WB04-E146

2026-11-09, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 백마흔여섯 번째 부분이다. 일반 타입과 특수 패턴의 배치 모양(픽셀 getter 와 별개인 정수 범위와 칸별 충돌 범위)을 순회하고 실제 finder 가 받는 사각형을 만드는 `o/RawCanonPlacementGeometry`(cpppj 기준선 `1f00a25a`, 기준선 뒤 변경 없음을 `git diff --numstat` 으로 확인)이다. Canon/사제 배치 계열(PixelShape→Placement*→Priest*)의 첫 부분이며, 선행은 E136 `raw-canon-type.js`(타입별 Canon 디코더)·E1 `RawSquidFinder` 이다.

## 1. 옮긴 것

`www/core/raw-canon-placement-geometry.js`

- `canonCollisionArea(x, y, footX, footY)` — 0049b8c0..0049b95a 의 finder 인자 산술이다. 왼쪽 위는 0 방향으로 절삭한 좌표에서 (발자국−1)을 뺀 정수, 오른쪽 아래는 원래 좌표에 `0.99999f` 를 더해 절삭한 정수이다. 발자국이 음수이거나 좌표가 비유한·int32 밖이면 정의되지 않은 변환 대신 거부한다. 0 발자국도 계산한다.
- `new RawCanonPlacementGeometry(pool, types, frames, state)` — 10.78 `SidPool`, 188칸 타입 표, 타입별 `{ frames, defaultFrame }`, `{ patternTypes[8], decoderDiagnostics }` 를 받는다. 개수·판본이 다르면 거부한다.
- `bounds(query)` — 타입별 Canon 디코더로 첫 현재 좌표와 전체 순회 개수×현재 발자국에서 미리보기 범위 `{ left, top, right, bottom }` 을 구한다. 빈 디코더에서도 계산한다.
- `inspect(query, scan, hooks)` — 디코더 생성 뒤 `beginRegions` 를 한 번 부르고, 유효 칸마다 칸 정보(`frame`·`label`·`side`·`x`·`y`·`snappedX`·`snappedY`·`area`)를 만들어 (선택) `beginShape` → `scan(cell)` → `endShape(query, cell)` 을 수행한다. 하나라도 거짓이면 즉시 거짓(최종 판정 생략)이고, 모든 칸이 끝나면(빈 모양 포함) `finishRegions(query)` 의 결과를 돌려준다. 현재 발자국은 칸마다 다시 읽고 디코더 생성 때의 첫 프레임/패턴 선택은 유지한다. 요청은 값으로 복사해 훅이 바꿔도 처음 값을 유지한다. 방향 글자(`side`)는 프레임 코드가 있을 때만 조회하며 비패턴 기본 프레임의 개수 검사 생략을 보존한다.

이 모듈은 DOM 을 쓰지 않으며, 지형/겹침 정책은 훅과 `scan` 이 선택한 실제 경계에 맡긴다.

## 2. 검증

`tests/canon-placement-geometry.test.mjs`(4건):

1. `canongeometry-x86.tsv` 의 `originals` 3159행(실제 PE 관측) — 타입 전역 9종×인자×방향(홀수·signed −1 포함)×프레임 표 3종(정상·역순·빈 표)×전역 별칭×기본 프레임×발자국 변경에서 미리보기 범위와 모든 유효 칸의 프레임·원래 좌표 비트·번호·방향 글자·절삭 좌표 비트·finder 정수 사각형 네 값이 모두 같고, 준비/후처리가 칸마다 한 번, 최종 판정이 한 번이다. `originalCD`·`original1037` 행은 사용하지 않았다.
2. 다중 칸의 준비/후처리/조기 거부 순서(`begin→prepare→scan→end→prepare→scan`)와 빈 모양의 `begin→finish`, 요청의 값 캡처, 시작 뒤 기본 프레임 변경 무시.
3. 실제 해시/finder 가 패턴 모양의 차례대로 후보를 찾아 거부하고, 거부 뒤 지형 판정과 최종 판정을 생략하며 finder 상태가 걸린 후보에 남음.
4. 프레임 표 누락 구성·표 밖 타입·누락된 finder/지역 경계 거부, 음수 발자국·비유한 좌표 거부, 방향 코드 없이 기본 프레임(4)이 `side=0` 으로 계산됨, 보정 산술 예.

최초 실패와 교정: 새 모듈은 3159행 fixture 와 나머지 검사를 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 지형 판정(`RawCanonPlacementTerrain`)·권한(`Permission`)·관계(`Relations`)·주변(`Surrounding`)·충돌(`Collision`)·미리보기(`Preview`) 훅 본체와 사제 배치 계열은 별도 단위다. 이 모듈은 그 계층이 공유하는 순회 계층만 맡는다.
- 10.72(CD)의 snap/CRT 저장 차이와 10.37 관측은 범위 밖이다.

## 4. 다음 시작점

`RawCanonPixelShape`→`RawCanonPlacement*`(Terrain·Permission·Relations·Surrounding·Collision·Preview·Pipeline)·`RawPriestPlacement*`, `RawPriestArrival`·`RawCarrierArrival`, `RawPacketDispatch`→`RawConstructionCommand`, `RawBurningSources`와 `RawBurningSourceCount`, Missile Host 팩토리, `RawWindAviaryProcess`·`RawWindFlyer`, 번개 수명/프레임/화면 등 [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 0.1절 "마일스톤" 행의 목록을 따른다.
