# 미니맵 평면 복사와 전체 창 외곽선 연결 (단위375)

현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. [기존 독립 원문 관측](cpp-minimap-blit-20261023.md)의 보호 PE와 커밋된120행 fixture를 사용한다. 새 전체 디컴파일이나 보호 파일 변경은 없다.

기존 RawMinimapControls는 소스 crop을128×128의 별도 X/Y 범위로 검사했다. 원본은 sourceBase=cropY×128+cropX의 평면 주소를 사용한다. 따라서 crop(-2,1)은 유효한126번 BYTE에서 시작하지만 기존 코드는 이를 거부했다. 또한128폭의 임시 이미지 안에 카메라 외곽선을 그려 더 넓은 Gump 영역의 선을 잃었다.

`GumpPixelTarget::RawBitmap`은 화면/현재 clip/전체 Gump/복사 목적지의 교집합을 먼저 계산하고, 실제 보이는 픽셀의 평면 소스 주소만 읽는다. source span 밖의 보이는 BYTE는 명시 outside 장치가 있어야 하며, 없으면 예외를 낸다. 보이지 않는 guard는 장치를 요구하지 않는다. 원본 할당 앞뒤의 미관측 값을0이나 clamp로 만들어 성공 처리하지 않는다.

`LinesClipped`는 같은 표면에 전체 Gump clip으로 네 선을 그리며 공유 clip을 변경하지 않는다. 실제 RawMinimapControls Draw는 원본 순서대로 평면 bitmap 복사 후 위/아래/왼쪽/오른쪽 선을 제출한다. bitmap128 밖의 Gump 여백은 기존 배경을 보존한다.

기존 `MinimapBlitTests.cpp`를 공식 CMake에 등록했다. 입력24열만 공급해 독립 x86의120개 전체 표면과 실제 할당 밖 BYTE 주소 집합을 비교한다. 원본 read 명령 횟수와 C++ callback BYTE 수를 혼동하지 않는다. 추가 검사는 동일한 음수 sourceBase가 clip으로 가려지면 정상 복사되고, 보이면 outside 장치 없이 거부되는지 확인한다.

## 검증 기록

검사 대상/게임 exe Release 빌드는 모두 종료0이며 관측 경고·오류0이다. 두 빌드를 나누어 단위374 감사 중의 게임 SHA를 바꾸지 않았다. 최종 제품 SHA-256은 `656e918ad2e3cdf5625faec575949d2081495c777051e8e00c35a7f39e73cc90`이다.

관련 **25검사/실패check0**이다(Minimap4·GumpDraw11·RawMenuControls10). MinimapBlit는 독립120개 전체 표면과 guard가 있는40행의 실제 BYTE 주소 집합을 통과했다. 기존 MinimapRaster의4,722행 전체 bitmap·공통 Draw의 세 판본/예외 clip 복구·돌 창/메뉴 수명도 통과했다. fixture SHA는 `7b6eec53b0d69a138b41d7d438bc04495b5b4b1e7ee438d96194fed2930497b3`이며 변경하지 않았다. 로그는 `extracted/cpp-minimap-blit-20261023-tests.log`다.

실제 GUI는 `extracted/cpp-minimap-blit-20261023-gui1024`와 `...-gui1280`이다.1024×768/1280×720 모두 초기 모달 중 입력 억제, 실제 누름/오른쪽·위쪽 드래그, 해제/캡처 반납, ESC → Game → Leave Mission → Main Menu와 실제 미션 반납을 통과했다. 두 실행은 정상 종료0/상한 미도달·전체 원본 SHA 동일이다. 같은 bitmap CRC4274939947을 유지하며 camera/crop만 원본 입력 경로로 바뀌었다.1024 드래그 캡처도 직접 확인했다.

전체 CTest2,420/0은 직전 단위373의 결과이며 이번 변경 이후의 전체 결과로 표시하지 않는다. 이번 변경은 독립 전체 픽셀/관련 공통 UI 검사와 두 실제 화면비 회귀로 검증했다.

## 남은 경계

전체 HUD dock의 실제 생성·분할선 드래그는 아직 연결하지 않았다.120행은 넓은 Gump의 복사/선을 독립 입력으로 검증하며, 실제 GUI의 드래그로130폭을 만들었다는 증거는 아니다. 기존 실제 UI의 기본80×75 미니맵 경로에 새 Draw를 연결한 범위다. 원본 new(0x4000) 밖의 실제 heap BYTE를 공급하는 장치도 남는다.

Fury War 훈련의 `CheatAllTechnology`와 일반 안내 부모 종료 문제는 [단위374 감사](cpp-solo-entry-audit-20261023.md)의 별도 다음 작업이다. 원본 c79b0은 실제 버튼 클릭에서 명령 예약(인자 복제) 뒤 부모를 Kill한다. 기술 명령 본체00492ae0에는 안내창 종료가 없다. 희생 후 메시지 안의 같은 기술 명령까지 닫아 버리는 임의 처리를 추가하지 않는다.

Surrounded 감사의 시간 상한은 제품 렌더링 성능만의 측정이 아니다. 기존 `when`은 조건마다 전체 Report를 만들며 큰 맵의 수만 raw 오브젝트를 반복 직렬화한다. 이 검사 비용을 분리한 측정 전에는 당시 프레임/벽시계를 제품 FPS로 사용하지 않는다.

전체 사용자 완주·Edit·전체 HUD·원본 전체 동일성 목표는 계속 미완료다.
