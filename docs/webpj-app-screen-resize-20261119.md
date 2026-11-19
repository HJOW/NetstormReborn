# WB04-E905: 화면비 변경 시 Screen 재생성

앱 논리 화면 크기가 바뀌면 현재 Screen의 팔레트와 모드, 같은 `paletteLoaded` 상자를 새 DIB로 옮긴다. Client의 최초 ColorSort 입력과 9표·소유자 표는 다시 만들지 않고 새 Screen의 조회기로 대여한다. 같은 크기이면 기존 Screen을 유지한다. 원본 cpppj `Client::ChangeResolution`과 같이 현재 팔레트를 새 저장 팔레트로 사용하며 웹의 Screen 구현은 RGBQUAD 예약 바이트를 팔레트 변환에서 보존하지 않는다.

새 Screen의 출력 장치와 전체 clip을 연결하지만, 해상도 변경 시 실제 게임 Renderer·카메라 재배치와 날씨 효과의 종료는 아직 연결하지 않았다. DIB 내용도 복사하지 않으며 현재 메뉴는 자체 재배치·다시 그리기 경로를 따른다.

실제 게임 팔레트와 TYPE으로 고정 입력·색상표·loaded 상자 정체성, 현재 RGB와 새 pitch/clip 및 같은 크기 자연 반환을 확인했다. Screen·색상 캐시를 포함한 Node 집중 검사 47건과 Chromium의 4:3·16:10·16:9 전환 1건이 통과했다. Node v26.13.0 전체 회귀 6,456건이 통과했다(실패·취소·건너뜀 0).
