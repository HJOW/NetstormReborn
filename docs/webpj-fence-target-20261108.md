# webpj 썬 바리케이드 표적(RawFenceTarget) 이식 — WB04-E86

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 여든여섯 번째 부분이다. 썬 바리케이드(fencemark)가 가로막는 투사체의 도착 좌표 계산(`o/RawFenceTarget`: 004571b0 Point·004568b0 Across·004819b0/004818d0 포구·시야·00462800 선분 교차)을 cpppj 기준선 `1f00a25a` 에서 옮겼다. E82 의 `raw-gun-target.js`(끝점 중심)와 E81·E83 의 fence 계열이 선행이고, `RawGunBlocker` 의 선분 교차(`intersection`)가 이 모듈에 의존한다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 분기는 옮기지 않았다.

## 1. 옮긴 것

`www/core/raw-fence-target.js` — `RawFenceTarget(pool, hash, types, geometry, target, targeting, process, fenceType = 90)`

- `RawFenceTarget.intersection(a, b, c, d)`(정적): 두 선분의 교차점. 수직/평행/사선 분기별로 첫 절편과 결과 x/y 만 float 로 저장하는 x87 순서를 따르고, 교차점이 양 선분의 포함 사각형 안(각 사각형의 min/min·max/max 대각 끝점은 제외)이 아니면 invalid(-10000,-10000)를 돌려준다.
- `across(mark, firing, target)`: fencemark 의 압축 끝점(QA WORD 와 QB 의 high24/low8 합성 번호)을 읽고, 같은 소유자이거나 방향별 동맹이면 invalid. 배리어 방향(세로 동률 우선)에서 축을 정하고 발사자/표적이 배리어 축 위이거나 폭·높이 3 초과 표적이 걸친 내부 칸이면 invalid, Cannon 이면 끝점 축 일치 검사, 축별 `-32/-16/-22/-11` 픽셀 보정, 1×1 표적은 2×2 로 보고, 배리어 띠와 발자국 겹침·8방향 양쪽 분류(축 위는 -1·모호)로 invalid/계속을 정한 뒤 두 중심 사이 선분 교차와 각도(<192, ≤60 임계)에 따른 최종 픽셀 보정 좌표를 계산한다.
- `point(firing, target)`: 발사/표적 범위를 합쳐 현재 선택 해시 단계의 칸 경계(`scale × min(trunc/scale, last)`, 끝은 `−0.001f`)로 넓히고 일반 finder(flags 1)로 순회하며 fencemark 후보마다 `across` 를 구해, 포구에 **엄격히** 가까운(거리를 float 로 저장) 유효 좌표를 고른다(없으면 invalid).
- `muzzle(firing)`/`sightBounds(firing)`: 현재 프레임의 SHP 포구 픽셀을 더하고 TYPE 핫스폿을 뺀 위치/발자국 두 corner(현재 raw·현재 픽셀 배율을 매 호출 다시 읽음; 프레임 범위 밖은 거부).
- 동맹은 값형 표(`process.allies`)나 현재 방향별 DWORD 조회기(`process.allianceRead`, 비영이면 동맹)로 읽는다.

## 2. 검증

- `tests/fence-target.test.mjs`(5건): `fence-target-x86.tsv` 856행 — Across 626(방향·끝점·발자국·동맹·Cannon/대각/픽셀 배율 보정), Point 48(전체 finder·해시 단계·후보 유무/선택), 일반 포구 24, 시야 사각형 24, 선분 교차 134 의 출력 네 float 원본 비트가 허용오차 없이 같다. Across/Point 는 값형 동맹 표와 현재 방향별 동맹 조회기(반대 fallback)로 두 번 재생한다. 계약: 판본/같은 풀/TYPE 크기 혼합 거부, 비어 있는 포구 프레임 거부, 평행 수직 선분의 invalid.

최초 실패와 교정: 새 모듈은 위 fixture 856행(+현재 동맹 재생 674행)과 계약 검사를 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- 실제 SHP 포구 헤더(`+20/+22`)와 TYPE 핫스폿의 자산 연결, 투사체(Missile) 소비자와의 조립은 해당 단위에서 한다. `RawProjectileGeometry`(E85)는 같은 포구/치수 규칙의 다른 소비자이며 이 모듈은 자체 `muzzle`/`sightBounds` 를 쓴다(원본도 별도 getter).

## 4. 다음 시작점

`RawGunBlocker`(선분 교차 `intersection` 와 `GunProcessState` 의 boss/friendlyFire/IsAlly 필요)·`RawGunGroundTarget`·`RawGunProcess`(+`MakeGunTargetHooks`), Missile 계열, `RawTempleDraw`/캡처 계열(자산 시험 장치), `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawManaBoltArrival`(시험 장면 선행), `RawIslandRuntime` 계열, 그 밖의 작은 몸체이다.
