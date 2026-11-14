# webpj 미니맵 순수 기하 원본 관측 연결 (WB04-E489, 2026-11-15)

3.8절 이월 목록의 `minimap-geometry`를 원본 10.78 독립 x86/CRT 관측 **1,028행**(callback 변경이 없는 관측)에 대조했다. 1,028행 모두 첫 실행에서 일치했고 이식 결함은 나오지 않았다. `PORT_MAP`과 모듈·시험 머리의 "전용 PE fixture 없음/cpppj에도 직접 시험이 없다"는 오기였다. 같은 목록의 `minimap-cell`(4,722행)은 E204 의 `tests/minimap-raster.test.mjs` 가 이미 같은 fixture 로 대조하고 있어(manifest `minimap-cell`) 3.8절의 낡은 항목을 정리했다.

## 비교한 것

`tests/minimap-geometry-native-fixtures.test.mjs`가 cpppj `MinimapGeometryTests`의 입력 구성을 옮긴다. 1,054행 중 mutation 열이 `-`인 1,028행(rebuild 254·move 36·idle 216·attach 272·draw 250)을 다섯 순수 진입점에 분배한다.

- **입력(앞 20열)**: 폭/높이(float32 비트)·crop(signed)·scrolling, 카메라 float 비트 두 값, 지도 표시 사각형 네 값, 칸 배율 두 값, 지역 좌표(float32 비트), 대상 원점. 비정상 값(NaN/무한대/범위 밖)도 원시 비트 그대로 공급한다.
- **비교**: 실행 뒤 state 5필드·view 8필드의 원시 비트(입력 view 가 순수 호출 뒤에도 보존되는지 포함), `rebuild`는 재구성 여부(Dirty 억제), `move`/`idle`은 원문 중심 경계가 받은 두 float32 비트, `draw`는 블릿 원점·source 오프셋(`y×128+x`)·행 폭·블릿 크기·exclusive 카메라 프레임 네 값.
- **제외(cpppj 와 같음)**: callback 재조회·부분 중단이 있는 26행은 순수 API 가 소유하지 않는 외부 호출 순서라 계수만 한다. 블릿 픽셀 복사와 외곽선 출력(`minimap-blit` 122행)은 GumpDraw 연결이 필요해 후속이다.

## 변형 확인

자동 변형 63개 중 40개가 잡히고 23개가 생존했다. 기존 `minimap-geometry` 시험과 합치면 5개가 남고 모두 동등 변형이다: `ArrayBuffer(4→5)`(float 비트 보기의 크기가 더 큼)과 `std::min/max`의 `<`→`<=`·부착 clamp 의 `128 < crop+폭`→`<=` 4개(같은 값 또는 부호만 다른 0 이 나뉘고 이어지는 `_ftol` 결과가 같음). 나머지 18개는 상태/view 기본값(원문 raw 로 모두 설치)과 칸 배율 검증이며 기존 시험이 잡았다.

## 남은 것

`minimap-blit`(122행)은 GumpDraw 로 128×128 bitmap 을 복사하고 카메라 외곽선을 그리는 8BIT 출력 대조라 후속이다.
