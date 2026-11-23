# webpj 신전 파편 캡처의 원본 관측 181행 대조 (WB04-E1275, 2026-11-23)

[LEFT_JOBS 점검](webpj-left-jobs-audit-20261123.md)(E1263)이 5-A "E197·E198·E201 재대조"와 7단계 "전용 시험이 없었던 시스템"에 남긴 "Shrapnel 표시 시스템을 cpppj Session 시험과 대조한다" 중 첫 부분이다. cpppj `CampaignTempleShrapnel_Native181CapturePixels` 가 읽는 원본 10.78 관측 `temple-capture-x86.tsv`(181행)는 `manifest.json`에도 웹 시험에도 없었다.

## 관측

보호 원본 10.78 의 `ShapeToBuffer`·가상 Draw·보호 SHP·전체 RLE 로 신전 파편(windVortex·rainVortex 등 다섯 TYPE)의 캡처 그림을 만든 관측이다. TYPE 기준점 (-2,3)·단위 배율·일반 색. 행: TYPE 이름·프레임(+36)·단어(+12, 2바이트)·추가 비트(+40)·소유자(+34)·그림 `너비,높이,기준점X,기준점Y`·전체 픽셀(16진)·픽셀 수·효과 생성 여부·파편 소리 횟수.

## 웹 시험

새 `tests/temple-shrapnel-native-fixtures.test.mjs`: 실제 Session 생성자 순서를 `connectBuilding`(건물/유닛 캡처 설치)까지 조립하고, 행마다 신전 객체를 만들어 입력 열(프레임·단어·추가 비트·소유자, 기준점 -2·3)을 쓴 뒤 `particleModules.shrapnel.lifetime.capture(sid)` 의 그림을 비교한다. **181행 모두 크기·기준점·전체 픽셀이 같고 픽셀 수 합계 2,684,495 가 같다.** 변형 확인: 그리기 기준점을 한 칸 어긋나게 하면 실패한다(원복 확인).

## 범위

- 파편 소리 횟수와 의존 더미 정리는 cpppj Session 시험(`captureSounds`·`core.dependencies`)의 범위이며 웹은 그림만 대조한다.
- E197(Shrapnel) 시각 시스템의 이 부분이 원본 관측으로 확인됐다. 같은 항목의 E198(EvilRay)·E201(Burning)은 `DamageableBurningSessionTests` 등 별도 cpppj 시험과의 대조가 남아 있다.
