# WB04-E904: 앱 Screen DIB 출력

창 모드 Screen의 `present` 장치를 앱의 현재 IndexedSurface와 IndexedPresenter에 연결한다. Screen이 제한한 사각형을 DIB pitch를 따라 읽어 대상의 같은 좌표로 복사하고, 보이는 Canvas에는 즉시 표시한다. 숨겨진 화면은 색 번호를 복사하고 다음 출력을 예약한다. 실제 표면·표시기 수명이나 논리 크기가 맞지 않으면 성공으로 넘기지 않고 오류를 낸다.

`loadPalette(..., 1)`은 기존 Screen의 순서대로 기존 팔레트에서 검정 DIB를 먼저 출력한 뒤 새 팔레트를 적용한다. 평소 메뉴 그림을 Screen DIB로 옮기는 전체 Renderer는 아직 없으며, 화면비 변경 시 Screen 재생성과 전투 팔레트·구름 갱신도 남아 있다.

폭 7의 패딩 DIB에서 부분 사각형만 선택된 표면에 복사하고 다른 픽셀을 보존하는지, 표면이 없으면 거부하는지 확인했다. Screen·구름·앱 적재 연관 Node 검사 84건이 통과했다. Chromium에서 실제 DIB의 `clear=1` 결과가 IndexedSurface와 Canvas의 검정 픽셀로 즉시 출력되는 것을 확인했다.
