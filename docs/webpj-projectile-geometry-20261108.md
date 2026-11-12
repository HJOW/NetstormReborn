# webpj 투사체 포구/치수 기하(RawProjectileGeometry) 이식 — WB04-E85

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 여든다섯 번째 부분이다. 썬 디스크 발사기 포구(0046a780)·가상 +b4 포구·캐논 사정선 끝(00481750)·방향 프레임(0041d6e0)·표적 치수/상자(0049af10)를 한 곳에 모은 `o/RawProjectileGeometry` 를 cpppj 기준선 `1f00a25a` 에서 옮겼다. `RawFenceTarget`·Missile 계열의 선행 단위이다. 기준선 뒤 cpppj 변경이 없다. 웹은 10.78 전용이라 CD 10.72 분기(BYTE 프레임, float 비트 signed 비교)와 10.37 행은 옮기지 않았다.

## 1. 옮긴 것

`www/core/raw-projectile-geometry.js` — `RawProjectileGeometry(pool, types, geometry, scaleX = 16, scaleY = 11, gunMode = null)`

- `archerMuzzle(firing)`: TYPE 71(썬 디스크 발사기)만. 기본 프레임과 현재 프레임(+36 signed)의 SHP 포구 픽셀(signed short 쌍)을 **정수로 먼저 더해** float 배율로 나눈 뒤 현재 좌표에 더하고 float 로 저장한다(TYPE hotspot 은 빼지 않음). `muzzle(firing)`: 가상 +b4 분배로 71 은 `archerMuzzle`, 128·129·131·140 은 현재 raw 좌표를 그대로, 그 밖은 미복원 거부. `hotspot(firing)`: 현재 물리 SHP 포구 픽셀(기본 프레임과 합산 안 함).
- `cannonRayEnd(from, to, firing)`: 128/129/131 만. 목표 방향(세로 동률 우선 4방향)과 현재 프레임의 cardinal 방향(썬 캐논은 프레임 0..3·24..27 의 `[4,0,6,2]`, 아이스/썬더는 같은 현재 방향 getter `gunMode.get`)이 다르면 난수 없이 TYPE 사정거리의 정수 절삭(imul 하위 DWORD)을 출발점에 더해 [1, 255] clamp 하고, 세로(0/4)면 (출발 x, 끝 y), 가로면 (끝 x, 출발 y) 를 돌려준다. 방향이 cardinal 이 아니면 원본 진단 거부.
- `directionFrame(from, to, type)`: 표시 타입의 균등 방향 반올림(근사 원주/직각/역원주 double 상수) 뒤 방향당 물리 프레임 수를 곱한다. `dimensions(target)`: 실제 footX/footY. `targetBounds(target, position)`: 위치에서 full foot/확장 높이를 뺀 corner 를 [1, 255] clamp 하고, 위치가 보드 안 (0,256) 이 아니면 들어온 corner 로 바꾸는 원본 Rect helper 를 따른다.
- 생성자는 PATCH 풀·TYPE/기하 188칸·유한 양수 float 배율·방향 getter 의 같은 풀을 확인하고 배율은 float32 로 저장한다.

## 2. 검증

- `tests/projectile-geometry.test.mjs`(3건): `projectile-geom-x86.tsv` 10.78 120행(포구 M·치수 D·상자 B 의 float 전체 비트, 허용오차 없음)과 `cannon-rayend-x86.tsv` 4040행(썬 캐논 프레임·출발/도착·사정거리 → 끝점 두 DWORD)이 같고, 썬 캐논 가상 포구의 raw 좌표 보존·기본/발사 프레임 네 방향 종점의 축 정렬·아이스/썬더의 방향 getter 요구·판본/배율/풀 거부를 확인한다.

최초 실패와 교정: 새 모듈은 위 fixture 4160행과 계약 검사를 모두 첫 실행에 통과했다.

## 3. 경계와 남은 것

- `muzzle` 의 71 외 분기·`hotspot`·`directionFrame`·아이스/썬더(`gunMode`) 사정선은 이 단위의 fixture 로 직접 대조되지 않는다(원본 관측은 썬 디스크 발사기 포구·치수·상자와 썬 캐논 사정선이다). Missile/Cannon 소비자 단위의 fixture 가 이어서 확인한다(PORT_MAP 상태 partial). 실제 SHP 헤더 `+20/+22` 포구 읽기 자체는 SHP 자산 단위에서 연결한다. **→ 소비자 fixture 대조는 E126 에서 끝났고 실제 SHP 헤더 연결은 WB04-E362 에서 끝내 `port` 가 되었다**([docs/webpj-projectile-geometry-types-20261113.md](webpj-projectile-geometry-types-20261113.md)).

## 4. 다음 시작점

`RawFenceTarget`→`RawGunBlocker`, `RawGunGroundTarget`·`RawGunProcess`(+`MakeGunTargetHooks`), Missile 계열(포구/방향 프레임 소비자), `RawTempleDraw`/캡처 계열(자산 시험 장치), `RawCarrierArrival`, `RawAssetPostPop`, 사제 배치 계열(선행 `RawCanonPixelShape`·`RawCanonPlacement`), 후보/지형 권한(`RawCanonPlacementPermission`), `RawSquidTransmission`, `RawDudeSpawn`·`RawDudeNotice`→`RawDudeDrop`, `RawManaBoltArrival`(시험 장면 선행), `RawIslandRuntime` 계열, 그 밖의 작은 몸체이다.
