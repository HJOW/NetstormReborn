# WB04-E903: 앱 Screen의 DIB 준비

최초 팔레트 적용과 ColorSort 입력 캡처 뒤 Screen DIB를 만들고 웹의 창 모드로 설정한다. 이후 전체 논리 크기로 clip을 지정한다. 같은 Screen에서 `loadPalette(..., 1)`이 선택되면 원본의 clear 관문이 DIB를 잠그고 검정으로 채운다. 최초 캡처 전의 팔레트 적재는 이전처럼 화면 모드 설정 전이다.

Screen DIB와 메뉴가 그리는 IndexedSurface는 아직 별도이다. DIB의 `present`가 Canvas에 연결되지 않았고 화면비 변경 시 Screen 재생성도 남아 있으므로, 이 단계에서 로그인 전투 팔레트나 화면 지우기가 완성된 것은 아니다.

폭 7·높이 4의 실제 앱 Screen에서 4바이트 정렬 pitch와 clip을 확인하고, 색상 적재 시 논리 픽셀만 지우며 행 패딩은 보존되는지 검사했다. 기존 Screen·구름·오류 및 언어 적재 연관 Node 검사 83건과 Chromium 앱 시작·재시작 1건이 통과했다.
