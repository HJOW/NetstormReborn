# 앱 GameWorld 소유 수명 (WB04-E888, 2026-11-19)

앱이 현재 Guide의 미소비 Fort 요청을 확인하고 같은 미션 Script의 선택된 Fort를 파싱해 `CampaignGameWorld`를 설치한다. Session에는 앱 소유의 SurfaceStorage·Kernel·난수·Player·Template·옵션·기술 제한·Fort 요청과 시계를 우선 전달한다. 중복 World 및 미션·요청 누락을 거부하며 생성 실패 시 GameWorld가 얻은 Session 대여를 반납한다. 앱 종료 시 Guide와 World를 장기 저장소보다 먼저 정리한다.

실제 Tutorial1·BridgeTheGap에서 같은 저장소와 미션 토큰을 확인하고, 테스트의 외부 로그인·카메라 장치를 넣어 로컬 로그인·Ready·요청 소비와 반납까지 검증했다. 로컬 로그인 장치 누락 및 중복 설치도 검사했다. 관련 실제 Client·GameWorld 검사 19건과 Chromium 생성·반납 1건이 통과했다. 브라우저 첫 실행에서는 검사 장치의 필수 사운드 효과 누락이 드러나 테스트 경계를 보완한 뒤 재실행했다.

State7 부모와 로그인 장치, 렌더러·입력·앱 프레임은 아직 이 설치 함수를 호출하지 않는다. 따라서 사용자 메뉴에서 게임이 시작되는 상태는 아니다.
