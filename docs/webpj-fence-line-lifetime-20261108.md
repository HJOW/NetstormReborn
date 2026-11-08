# webpj 썬 바리케이드 연결선 삭제·수명(RawFenceLine·RawFenceLifetime) 이식 — WB04-E74

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 일흔네 번째 부분이다. 썬 바리케이드(Fence)가 이웃과 이어지는 연결선의 삭제 지점 계산·삭제(`o/RawFenceLine`, 00455b20 mode 0)와 연결 정리·이웃 갱신·가상 preDestroy(`o/RawFenceLifetime`, 4577f0/4578f0/457990)를 cpppj 기준선 `1f00a25a` 에서 옮겼다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 분기와 10.37 행, x87 확장 정밀도 관측은 옮기지 않았다.

## 1. 옮긴 것

`www/core/raw-fence-line.js` — `RawFenceLine(pool, hash, types, frames, markType, destroy)`

- `searchPoints(끝점 네 float)`(정적): 원본 mode 0 이 finder 에 넘기는 정수 검색점의 순서이다. 수직선(x 가 같음)이면 y 방향 ±1 로 3칸 당겨 끝점과 합치고, 수평/대각이면 양 끝 x 를 1칸씩 넓히고 y 를 1 내려 시작 보정을 한 뒤 100회 상한 안에서 끝점을 한 번 포함하는 `_ftol64` 하위 signed 정수 좌표를 낸다. 모든 float 연산은 단정도 반올림을 거친다.
- `clear(끝점)`: 시작 코드 J(0x4a, x 가 같을 때)/K(0x4b)가 맞는 현재 연결선(현재 연결선 타입 번호의 객체; getter 로 매번 읽음)만 각 지점의 일반 finder(`RawSquidFinder`, flags 0, 표면 포함)로 찾아 가상 삭제(flags 0x40)한다. 삭제 효과 뒤에도 같은 finder 의 미리 보관한 next 와 다음 현재 버킷을 쓴다. 삭제 소비자 누락은 선택 위치에서 거부한다.

`www/core/raw-fence-lifetime.js` — `RawFenceLifetime(pool, hooks)`

- `clear(부모)`(4577f0): extra 의 비트 9 가 켜져 있으면 끝. 아니면 동맹 이웃을 방향 0/2/4/6 순서로 찾아 두 끝점 표식을 지우고 효과 뒤의 현재 좌표/소유자로 연결선을 지운다.
- `refresh(부모)`(4578f0): 동맹 이웃 네 개를 먼저 모두 저장 → 자기 정리 → 자기가 dead 가 아니면 Unpop(0) → 저장한 이웃마다 현재 dead 가 아니고 현재 타입이 부모와 같으면 정리 후 재생성 → 자기가 dead 가 아니면 Repop(0x40).
- `preDestroy(부모, flags)`(457990): refresh → 비동맹 제한 없는 일반 이웃 네 개를 저장 → 첫 두 칸에만 예약(new 0x28) 성공 시 이웃 부모에 event 2/payload 0.1 Regular 를 부착 → Damageable 전처리를 같은 flags 로 부른다. 이웃의 유효성/dead/타입과 좌표/소유자는 효과 뒤 현재 raw 를 다시 읽는다.

## 2. 검증

- `tests/fence-line.test.mjs`(2건): `fence-line-x86.tsv` 99개 입력(단정도 네 끝점 → 정수 검색점 순서·끝점 포함·100회 상한), 현재 해시/방향 코드가 맞는 mark 만 삭제하는지(실제 `SquidHash` 와 `RawSquidFinder`)와 삭제 소비자·표 크기·비유한 끝점 거부.
- `tests/fence-lifetime.test.mjs`(2건): `fence-lifetime-x86.tsv` 207행(Clear/Refresh/PreDestroy 의 호출 순서 S/M/L/D/U/B/N/A/P 와 다섯 슬롯 raw 전체; 효과 안에서 좌표·소유자·dead·타입을 바꾸는 변이 포함), 판본/범위/선택한 누락 효과 거부.

최초 실패와 교정: 새 모듈은 위 fixture 306행과 계약 검사를 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 이웃 검색(`455fb0`/`456220`)·표식 삭제(`4566d0`)·연결선 생성(`RawFenceRebuild`, 00457460)·Pop/Unpop·Regular 부착·Damageable 전처리의 실제 월드 연결은 해당 파일 이식 때 대조한다. `RawFenceRebuild` 의 시험은 `FenceCreateTests`/`FenceTypeParentTests` 안에 있어 생성 부모 이식 때 함께 옮긴다.

## 4. 다음 시작점

`RawFenceRebuild`와 Fence 생성 부모, `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission` 이다.
