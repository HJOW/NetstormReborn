# WB04-E777 Session 입자 뷰와 커서 영역

cpppj `RawCampaignSession.cpp`의 `SetParticleView`, `ParticleSprites`, `Particles`를 같은 Session의 카메라·입자·커서에 연결했다. 기본 경로는 목표 픽셀과 Renderer·snapshot을 적용하고 커서 viewport 및 입자 표시 장치에 전달한다. `updateCamera=false`는 공유 카메라를 건드리지 않으며 현재 snapshot에 따른 dirty 갱신만 사용한다. 커서가 아직 없을 때 viewport를 저장해 후속 Load가 사용한다.

TheWarBegins 실제 Session에서 Load 전/후 표시 영역, 같은 화면 재통지, 직접 적용하지 않는 경로, 실제 snapshot 이동에 따른 dirty, 커서의 실제 `setView` 호출을 확인했다. 제한되지 않은 사제에 불꽃 표시를 부착해 그 프로세스만 한 프레임 진행하고 공통 목록에서 직접 그리기 항목 하나가 수집되는 것을 확인했다. 최초에는 프레임 전 수집을 기대해 결과 0으로 실패했으며 실제 표시 프로세스의 첫 프레임 뒤 검사하도록 교정했다. 전체 Kernel·실제 브라우저 Renderer 및 GameWorld 연결은 남는다. 제한된 미션 행동과 세 공중 유닛 전용 행동은 실행하지 않았다.

Node v26.13.0 전체 회귀 6,280건이 통과했고 실패·취소·건너뜀은 0건이다. 대응표 오류는 0건이며 E771의 Node v22 전체 회귀 뒤 여섯 번째 v26 회귀다.
