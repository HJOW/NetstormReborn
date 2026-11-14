# webpj 미니맵 블릿 원본 관측 연결 (WB04-E494, 2026-11-15)

3.8절 이월 목록의 마지막 HUD 항목 `minimap-blit`을 원본 10.78 독립 x86 관측 **120행**에 대조했다. 120행 모두 첫 실행에서 일치했고 이식 결함은 나오지 않았다.

## 비교한 것

`tests/minimap-blit-native-fixtures.test.mjs`가 cpppj `MinimapBlitTests`(`IndependentNativeFullSurfaceAndGuardReads`)의 입력 구성을 옮긴다. 입력 열(폭/높이 float 비트·crop·대상 원점·클립·카메라/뷰/칸 배율·bitmap/가드 입력 패턴 시드·배경/잉크)만 공급하고 관측 열은 비교에만 쓴다.

- **계획**: `planMinimapDraw`가 만든 블릿 원점·소스 평면 주소(`y × 128 + x`)·행 폭 128·블릿 크기·exclusive 카메라 프레임 네 값이 원문 경계 인자와 같다. 잉크는 외부 팔레트 입력이라 일치만 확인한다.
- **출력**: 128×128 입력 bitmap(바이트 패턴)을 `GumpPixelTarget.rawBitmap`으로 192×160 표면에 복사하고, 카메라 외곽을 `linesClipped`의 네 선(끝점 포함 `right-1`/`bottom-1`)으로 그린 결과가 원문이 바꾼 구간을 배경 위에 복원한 전체 표면과 바이트 단위로 같다.
- **가드 읽기**: 할당 밖(앞뒤 guard) 소스 BYTE 를 읽는 주소 집합이 실제 CPU 의 읽기 구간 중 bitmap 밖 주소와 같고(40행), 읽지 않는 행은 비어 있으며, 현재 클립 전역은 출력 뒤에도 변하지 않는다.

## 변형 확인

이 단위가 쓰는 `www/render/gump-draw.js`의 `rawBitmap`·`linesClipped`·`lines`(+ 클립/생성자) 구간에 자동 변형 84개를 돌렸다. 기존 `gump-draw` 시험과 합치면 39개가 남는데, 모두 이 fixture 가 쓰지 않는 코드나 동등 경계다: 상태 기본값(8)·표면 용량 검증(4)·이미지 검증(13, `drawIndexedImage`)·`fill` 빈 영역 가드(3)·`lines`의 빈 선 가드(3)와 직교/대각 판정(1)·`rawBitmap`의 입력 검증/빈 크기 조기 반환(7). 이 중 `rawBitmap`의 `size <= 0` 조기 반환·빈 영역 가드는 값이 0 인 경계가 같은 무변화가 된다(동등). 이 fixture 만으로 가려지는 `rawBitmap`/`linesClipped` 본문 변형은 없다.

## 남은 것

3.8절의 HUD 계열 fixture 는 모두 연결되었다. 남은 3.8절 항목은 허브/적응 판정이 선행인 것들이다: `chat-callbacks`·`chat-input-recipients`(허브), `html-input-dialog` 생성/공장 20행(SaveFortNameDialog), `detonate-input`(RawObjectConversion), `audio-reinterpret`·`font-face-*`(적응 판정), `renderer-projection`(ClientRendererFrame), `scenemusic`(ClientAudio 적응).
