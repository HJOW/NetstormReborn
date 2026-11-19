# WB04-E981 미션 Renderer 화면 합성 경계

실제 미션 표시 수집·원본 구름 배경 선택·정렬 제출을 `Renderer.draw`의 현재 dirty 사각형에 연결한다. 수집 전에 같은 Screen clip과 현재 지도 clip을 설정하고, 배경은 실제 Screen의 잠금과 Kencloud·게임 난수·카메라를 빌린다. 지형 덮임 및 UI 전용 dirty는 원본 배경 관문을 따른다. 색상 tint·카메라 분모·UI 수집/출력은 호출자가 실제 현재 장치를 공급한다.

검증: 실제 Tutorial1 World와 Screen DIB를 잠가 Renderer Draw→Present→앱 IndexedSurface까지 출력했다. 빈 지면의 실제 구름이 지도 안에만 그려지고 UI 전용 dirty에서는 배경이 유지되는 것도 검사했다. 테스트에서는 창이 보이지 않는 레이아웃과 tint 0을 명시했으며 실제 HUD UI 수집기는 연결하지 않았다. 원본 구름 695행·Renderer dirty/정렬과 실제 Kencloud 검사를 포함한 Node 50건 통과.

앱 프레임의 자동 Renderer 호출·구름 갱신/dirty 앞단·실제 HUD와 Canvas 표시, 화면비별 픽셀 동일성은 아직 미완료다. E974 고정 Node 26 전체 회귀가 실행 중이다.
