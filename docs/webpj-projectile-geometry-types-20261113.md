# webpj 투사체 기하 표의 실제 SHP 헤더 연결 (WB04-E362, 2026-11-13, 7-P2 넷째 항목, RawProjectileGeometry 마감)

3.1절 7-P2 넷째 항목 "RawProjectileGeometry (partial 176줄)"의 남은 부분을 처리해 마감했다(PORT_MAP partial → port). 포구/치수/방향 계산 본체는 E85 에서 끝났고, 남은 두 일은 ① 다른 TYPE 분기(`muzzle` 71 외·`hotspot`·`directionFrame`·아이스/썬더 129/131 사정선)를 소비자 fixture 로 대조하는 것, ② 기하 입력 표를 **실제 TYPE 정의와 SHP 추가 헤더**에서 만드는 연결이었다.

## 1. 소비자 fixture 대조(이미 끝난 것의 확인)

`RawProjectileGeometry` 는 E126 의 세 소비자 시험에서 cpppj 와 같은 방식으로 Missile 훅에 직접 연결되어 있다(E126 문서 기록). 이번에 호출 지점을 다시 확인했다.

- `tests/crossbow-missile.test.mjs`(crossbow-missile 703행): `geometry.muzzle`(TYPE 140 크로스보우)·`geometry.hotspot`·`geometry.directionFrame`.
- `tests/ice-missile-initial.test.mjs`(ice-missile-initial 1,693행): `geometry.muzzle`(129)·`hotspot`·`dimensions`·`cannonRayEnd`(129, 같은 `RawGunMode` 방향 getter)·`directionFrame`.
- `tests/thunder-missile.test.mjs`(thunder-missile 1,118행): `muzzle`(131)·`hotspot`·`dimensions`·`cannonRayEnd`(131)·`directionFrame`.
- 썬 캐논 128 의 원시 포구와 축 정렬 종점, 썬 디스크 발사기 71 의 포구는 E85 의 projectile-geom 120행·cannon-rayend 4,040행으로 대조된다.

그러므로 `RawProjectileGeometry` 의 모든 공개 함수(`archerMuzzle`·`muzzle`·`cannonRayEnd`·`hotspot`·`directionFrame`·`dimensions`·`targetBounds`)가 원본 관측 fixture 로 대조되었다.

## 2. 실제 SHP 헤더 연결: `buildProjectileGeometryTypes`

새 모듈 `core/raw-projectile-geometry-types.js` 가 cpppj 세션이 자산 TYPE 마다 `projectileTypes[type]` 을 채우는 루프(`client/RawCampaignSession.cpp`·`tests/CampaignInstallSupport.h`)를 옮긴다. 이 루프는 `o/` 단위가 아니라 client 세션 안에 있어 지금까지 웹에 없었다.

- `buildProjectileGeometryTypes(definitions, shapes, typeCount = 188)`: 자산 TYPE 은 70 번부터 정의 순서(TYPE 70 + 블록)로 이어지며 각 칸은 `{ defaultFrame, frameCount, directions, shotOffsets, heightExtension, range }`.
  - `defaultFrame` = 정의의 기본 프레임. `frameCount`/`directions` = `readRawArcherTypeData`(TYPE+114 클러스터 수 / TYPE+110 `orientations` 방향 수; 표시 hotspot·SHP 프레임 수와 혼용하지 않음).
  - `shotOffsets` = 그 TYPE 의 **모든 물리 SHP 프레임** 헤더의 마지막 두 signed WORD(`+0x20`/`+0x22`, 포구 픽셀; 표시 기준점 아님). 썬 캐논(128)은 클러스터 29개에 물리 프레임 58개를 가진다.
  - `heightExtension` = 숫자 `height` 를 0 쪽으로 절삭한 int32(없거나 문자열이면 0, int32 밖·비유한은 x87 정수 불확정값 −2^31). `range` = 숫자 `range` 를 float 로 줄인 값(없거나 문자열이면 0).
  - 속성 이름은 대소문자를 구별하지 않고 같은 이름이 여러 번이면 마지막 값이 이기며 문자열은 숫자로 바꾸지 않는다(원본 `Number()` 계약). 정의가 TYPE 표를 넘거나 대응하는 SHP 칸이 없으면 거부하고 정의가 없는 TYPE 은 서로 다른 객체인 빈 칸이다.

## 3. 검사 (`tests/projectile-geometry-types.test.mjs` 5건)

- **합성 계약 3건:** 블록→TYPE 배치·값·빈 칸의 독립성; 속성 이름 대소문자/마지막 값/문자열 0, height 절삭(±2.9, 경계 2^31±0.5, 비유한 NaN·±Infinity)과 range float; orientations 숫자만·방향 수 절삭·프레임 수는 클러스터 수이며 SHP 프레임 수와 독립, 정의 개수 초과(119개)/SHP 칸 누락/프레임 배열 없음/typeCount 변경.
- **실제 자료 2건**(웹 준비 자료 `netstorm.tarc`·`d/_shapes.shp`): ① 자산 TYPE 116개의 모든 물리 프레임 포구 쌍(1천 개 이상)이 SHP 헤더를 `squidHeader` 로 **직접 다시 읽은 +0x20/+0x22 signed WORD** 와 같고, 기본 프레임·클러스터 수·방향 수·사거리가 독립 경로 `buildTypeRecords` 와 같으며 알려진 값(썬 캐논 사거리 20·첫 포구 (−23, −11), 썬 디스크 발사기 사거리 9, 윈드 아처 방향 20, 썬더 캐논 높이 확장 2)이 맞다. ② 이 표를 쓴 `RawProjectileGeometry` 가 실제 썬 디스크 발사기(71)의 포구를 기본 프레임과 현재 프레임 SHP 헤더 두 쌍의 정수 합 ÷ 배율 + 좌표로(다섯 프레임, float 비트까지) 계산하고, 128/129/131/140 의 가상 포구는 현재 좌표를 그대로 돌려준다.
- cpppj 의 같은 구성에는 독립 관측 fixture 가 없다(세션 조립 코드). 시험은 처음 실행에서 한 건이 실패했고 모두 시험 쪽 기대값 실수였다(정의 기본 클러스터 수 1).

## 4. 변형 확인과 남은 경계

- 새 모듈에 연산자·상수 28 + 의미 변형 29 = 57회를 적용했다. 1차에서 6개가 남았고 둘은 **시험의 공백**(NaN height, 프레임 배열이 없는 SHP 칸)이라 보강해 잡았다.
- 남은 동등 변형 4개: 상한 비교 `>= 2^31` 을 `>`/`2^31+1` 로(정확히 2^31 은 `|0` 으로 같은 −2^31), 내부 함수의 이름 소문자화 제거(호출자가 소문자만 쓴다), 속성이 없을 때 `undefined` 대신 `0`(호출자가 `?? 0`).
- 앱 수명 연결(세션 조립에서 이 표를 만들어 `RawProjectileGeometry`·`RawFenceTarget` 에 주는 일)은 후속 단계 몫이다. 이 모듈은 DOM 을 쓰지 않는다. 다음 7-P2 항목은 **RawGunImpact**(136줄 +12)이다.
