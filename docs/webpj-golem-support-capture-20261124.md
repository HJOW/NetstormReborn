# 골렘·받침 원본 캡처 대조 (WB04-E1312, 2026-11-24)

LEFT_JOBS 5-A 'E197·E198·E201 재대조'의 남은 캡처 둘을 닫았다. cpppj 가 GameWorld 렌더링 시험으로 읽는 실제 10.78 관측 `golem-capture-x86.tsv`(66행)와 `support-color-capture-x86.tsv`(198행)를 웹 `RawShapeToBuffer` 캡처가 같은 메타데이터 16바이트와 전체 픽셀로 내는지 본다(제단 `altar-capture` E1276 과 같은 방식, 새 시험 `tests/golem-support-capture-native-fixtures.test.mjs` 2건). **웹 코드는 바꾸지 않았고 두 fixture 의 전 행이 일치한다.** 두 fixture 를 manifest 에 올렸다(941개).

## 골렘 66행

- 입력: TYPE 133(가상 표 `0x50e568`)·소유자 0·extra 128·TYPE 기준점 (8,2)·단위 배율·프레임 0..65. 행 열은 [프레임, 메타데이터 `폭,높이,기준점x,y`, 픽셀 hex, 픽셀 수, 호출 수]이다.
- cpppj 의 GameWorld 시험은 보행 64프레임만 두 카메라로 대조하지만(`frame >= 64` 제외) 캡처 자체는 66행 모두 원본이므로 66행 전부를 본다. 색표 호출은 없다(중립).

## 받침 198행

- 입력: TYPE 94(윗면)·95(하단)·9프레임·소유자별 현재 색 번호(`ownerColors[owner]`, 소유자 번호와 다른 경우 포함)·TYPE 기준점 (-2,3). 행 열은 [TYPE, 프레임, 소유자, 현재 색 번호, 메타데이터, 픽셀]이다.
- **색표가 타입마다 다르다**(여러 가설을 같은 입력으로 돌려 가렸다): 윗면 94 는 웹이 미션에서 쓰는 **명도 전용 표**(`buildSpecialOwnerColorMaps(logical, 94)`, 실제 게임 팔레트 `d/GIFCLOUD.COL` 의 HSL 명도 기반)로 99/99 행이 맞고(일반 띠 표는 18/99), 하단 95 는 **일반 띠 표**(`buildStandardOwnerColorMaps(95, 82)`)로 99/99 행이 맞는다(명도 전용 표는 18/99 또는 0/99). 이 규칙은 `OWNER_COLOR_TYPES`(banner 에 94, standard 에 95 는 없음)와 `createOwnerColorTables` 의 분류와 따로 확인된 사실이므로 시험이 고정한다.

## 변형 확인

색표 뒤바꿈(94 일반·95 명도), 골렘 기준점 x 8→9, 받침 소유자 번호를 0 으로 바꾼 세 변형이 각각 시험 한 건을 실패시키고 원본은 두 건 모두 통과한다.

## 남은 것

5-A 의 남은 세부는 E198 EvilRay·E201 Burning·섬 투하 지지·입자의 cpp 시험 대조다(웹 시험은 이미 있고 cpp 쪽 `CampaignObjectMenuTests.cpp:1083`·`DamageableBurningSessionTests.cpp`·`CampaignSessionTests.cpp` 의 기대값을 실제 Session 입력으로 대조하는 일).
