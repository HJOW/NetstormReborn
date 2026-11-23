# webpj 파편·건물 캡처의 원본 관측 4,474행 대조 (WB04-E1275~E1276, 2026-11-23)

[LEFT_JOBS 점검](webpj-left-jobs-audit-20261123.md)(E1263)이 5-A "E197·E198·E201 재대조"와 7단계 "전용 시험이 없었던 시스템"에 남긴 "Shrapnel 표시 시스템을 cpppj Session 시험과 대조한다" 중 첫 부분이다. cpppj `CampaignTempleShrapnel_Native181CapturePixels` 가 읽는 원본 10.78 관측 `temple-capture-x86.tsv`(181행)는 `manifest.json`에도 웹 시험에도 없었다.

## 관측

보호 원본 10.78 의 `ShapeToBuffer`·가상 Draw·보호 SHP·전체 RLE 로 신전 파편(windVortex·rainVortex 등 다섯 TYPE)의 캡처 그림을 만든 관측이다. TYPE 기준점 (-2,3)·단위 배율·일반 색. 행: TYPE 이름·프레임(+36)·단어(+12, 2바이트)·추가 비트(+40)·소유자(+34)·그림 `너비,높이,기준점X,기준점Y`·전체 픽셀(16진)·픽셀 수·효과 생성 여부·파편 소리 횟수.

## 웹 시험

새 `tests/temple-shrapnel-native-fixtures.test.mjs`: 실제 Session 생성자 순서를 `connectBuilding`(건물/유닛 캡처 설치)까지 조립하고, 행마다 신전 객체를 만들어 입력 열(프레임·단어·추가 비트·소유자, 기준점 -2·3)을 쓴 뒤 `particleModules.shrapnel.lifetime.capture(sid)` 의 그림을 비교한다. **181행 모두 크기·기준점·전체 픽셀이 같고 픽셀 수 합계 2,684,495 가 같다.** 변형 확인: 그리기 기준점을 한 칸 어긋나게 하면 실패한다(원복 확인).

## 제네레이터·타워

같은 형식의 `generator-capture-x86.tsv`(제네레이터 세 TYPE 180행, 픽셀 합계 1,844,370, cpppj `CampaignGeneratorShrapnel_Native180CapturePixels`)와 `blocker-capture-x86.tsv`(스톤/아이스 타워 TYPE 78·147 120행, 합계 1,317,660, `CampaignForwardBattle_Native120CapturePixels`)도 같은 시험이 읽는다. **두 fixture 모두 모든 행이 일치한다.** 세 fixture 를 한 헬퍼(`checkCaptureFixture`)로 대조하며 행 열 수만 다르다(신전 10: 소리 횟수 열 포함, 나머지 9).

## 윈드 아비어리·워크샾·제단 (E1276)

- `wind-aviary-capture-x86.tsv`(54행, TYPE 139, 픽셀 합계 64,152)와 `plain-workshop-capture-x86.tsv`(2,115행, 다섯 워크샾 TYPE×5상태×3소유자)는 열 배치가 달라 `checkLayoutFixture` 헬퍼로 같은 시험 파일(`temple-shrapnel-native-fixtures.test.mjs`)에서 읽는다. **모든 행이 일치한다.**
- `altar-capture-x86.tsv`(1,824행, 약 95MB)는 `ShapeToBuffer_AltarNative1824Pixels` 와 같은 L1 시험이다. 새 `tests/altar-capture-native-fixtures.test.mjs` 가 `RawShapeToBuffer`(TYPE 164, 기준점 (-2,3), 표준 소유자 색표, 가상 표 0x5076b8)로 90프레임×소유자 색×추가·추상 비트×배율 입력의 메타데이터 16바이트·전체 픽셀과 색표/주 프레임/추가 그림 호출 횟수를 비교하며 **1,824행 모두 일치한다**. 변형 확인: 색표 그리기 기준점을 어긋나게 하면 실패한다(원복 확인).
- 같은 계열의 `golem-capture`(67행)·`support-color-capture`(199행)는 `GameWorld` 의 스프라이트 렌더링 대조(카메라 이동 전후)라 렌더러 장면 조립이 필요해 남겼다.

## 범위

- 파편 소리 횟수와 의존 더미 정리는 cpppj Session 시험(`captureSounds`·`core.dependencies`)의 범위이며 웹은 그림만 대조한다.
- E197(Shrapnel) 시각 시스템의 이 부분이 원본 관측으로 확인됐다. 같은 항목의 E198(EvilRay)·E201(Burning)은 `DamageableBurningSessionTests` 등 별도 cpppj 시험과의 대조가 남아 있다.
