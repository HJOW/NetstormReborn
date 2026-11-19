# WB04-E902: Screen 색상과 브라우저 표시기

앱의 Client Screen이 팔레트를 실제 적용할 때 BGRX 256색을 같은 번호의 RGB로 바꾸어 `IndexedPresenter`에 전달하고 다음 출력을 예약한다. 화면 시작의 fortPal과 이후 `Screen.loadPalette`·`setPalette(..., true)`가 같은 경계를 사용한다. 표시기 자체가 색 변환 결과를 다시 만들도록 stale 표식도 설정한다. 표시기가 없는 독립 계산 경로는 출력만 생략한다.

이 연결은 Screen DIB와 앱의 IndexedSurface를 합친 것이 아니다. 팔레트 전환의 화면 지우기·구름 갱신·다른 UI 색상 소비자·실제 로그인 `battlePalette`는 아직 연결하지 않았다. 화면비 변경 시 Screen 재생성과 전체 Renderer 경계도 남았다.

실제 GIFCLOUD의 256색과 0/255 고정, 색 번호 37의 서로 다른 RGB 성분, 표시기의 stale 및 다시 그리기 표식과 잘못된 물리 범위를 검사했다. Screen·표시기·기존 cpppj 메뉴 오라클 연관 Node 검사 42건과 Chromium 실제 앱 색상 변경 및 재시작 1건이 통과했다.
