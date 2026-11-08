# webpj 썬 바리케이드 이웃 검색(RawFenceNeighbors) 이식 — WB04-E88

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 여든여덟 번째 부분이다. 10.78 썬 바리케이드(Fence)의 방향별 최근접 이웃 검색과 실제 연결 개수(`o/RawFenceNeighbors`: 00456220/00455fb0 이웃·00455f00 축 사거리·004573d0 연결 수)를 cpppj 기준선 `1f00a25a` 에서 옮겼다. E81 의 재생성 제어(`raw-fence-rebuild.js`)가 훅으로 받던 `neighbor`/`withinRange` 의 실제 몸체이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이다.

## 1. 옮긴 것

`www/core/raw-fence-neighbors.js` — `RawFenceNeighbors(pool, hash, types, ranges, alliances)`

- `searchBounds(sid, direction)`: 원본 Rect min/max 의 현재 float 경계. 현재 위치에서 `(발자국−1) float` 만큼 빼고 정수 사거리(TYPE+44 의 CRT 절삭, `imul` 하위 DWORD)×방향 벡터를 더한 점을 새 corner 로 [1, 255] clamp 해 포함한다(방향 8 이상 거부).
- `select(sid, candidates, allied)`: finder 가 반환한 순서(중복·자기 자신 포함)대로 같은 TYPE·살아 있음(state bit1 없음)·extra&9 없음·같은 축(x 또는 y 일치)인 후보 중 float 중심 사이 거리가 **엄격히** 더 가까운 첫 후보를 고른다(최대 거리는 정수 사거리의 두 배, 같은 거리는 첫 후보 유지). `allied` 이면 최근접을 고른 **뒤에만** 후보 소유자 행→현재 소유자 열의 동맹 DWORD 를 읽어 소유자가 다르고 동맹이 아니면 빈 값(0)을 돌려준다(가까운 적대를 건너뛰어 먼 동맹으로 대체하지 않음).
- `neighbor(sid, direction, allied)`: 검색 경계를 CRT 절삭한 정수 영역으로 일반 finder(flags 1: 표면 건너뜀)를 순회해 후보를 모은 뒤 `select`. `withinRange(sid, endpoints)`: 수직이면 Y 차, 아니면 X 차의 절댓값을 CRT 절삭한 값(float 변환)이 TYPE 사거리(float) 이하인지. `connections(sid)`: extra&9 이면 0, 아니면 0·2·4·6 방향 동맹 이웃이 사거리 안이고 이웃 extra bit0 이 없을 때 한 번씩 센다.
- 사거리 표(188)·동맹 행렬(81 DWORD)·풀·해시는 사본 없이 읽으며 호출마다 현재 값을 읽는다.

## 2. 검증

- `tests/fence-neighbors.test.mjs`(2건): `fence-neighbors-x86.tsv` 1904행 — 부모+네 후보의 전체 250바이트·후보 순서(중복/자기 자신 포함)·발자국·동맹 방향·끝점 입력에서 최근접 선택 결과 SID 와 검색 경계 네 float 비트(N 행), 축 사거리 판정(W 행)이 같고 조회가 입력 객체를 바꾸지 않는다. 계약: 실제 해시에서 네 방향이 서로 다른 후보를 고르고 가까운 적대를 건너뛰어 먼 동맹으로 대체하지 않음, 반대 방향 동맹만 있으면 연결 0, 이웃 extra/state 에 따른 연결 수 변화, 더 가까운 제3 소유자 후보, 부모 extra 로 0, 방향/SID/판본/크기 거부.

최초 실패와 교정: 새 모듈은 위 fixture 1904행과 계약 검사를 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 실제 Session 에서 `raw-fence-rebuild.js` 의 `neighbor`/`withinRange` 훅과 `RawFenceRegular`(정기 갱신)에 이 모듈을 연결하는 조립은 해당 단위에서 한다.

## 4. 다음 시작점

`RawFenceRegular`(fence-regular fixture), `RawGunGroundTarget`·`RawGunProcess`(+`MakeGunTargetHooks`), Missile 계열, `RawTempleDraw`/캡처 계열(자산 시험 장치), `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawManaBoltArrival`(시험 장면 선행), `RawIslandRuntime` 계열, 그 밖의 작은 몸체이다.
