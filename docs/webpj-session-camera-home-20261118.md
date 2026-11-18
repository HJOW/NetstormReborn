# WB04-E776 Session 초기 카메라와 사제 위치

cpppj `RawCampaignSession.cpp`의 Camera 조립과 `ResetCamera`, `CenterCameraOn`, `ViewHome`, `ViewYourPriest`를 현재 Session의 실제 풀·TYPE·Player·사제 참여 조회 및 같은 카메라/재구성/다시 그리기 전역에 연결했다. 초기화는 현재 월드의 로딩 깊이를 공유해 내용물 미리보기를 억제하고 예외에도 깊이를 복구한다. Home은 Player +7c를 우선하고 0일 때만 +84를 사용한다. 즉시 이동은 호출자가 지정한 497960 소비자를 요구한다.

TheWarBegins 실제 Session의 사제·Player로 초기 위치, 깊이 4→5→4 및 화면 적용 예외 후 복구, 깊이 0에서 491 Regular 생성, Home의 우선/대체 및 죽은 첫 SID 처리, SID 범위를 검사했다. 최초 검사에서는 `pool.slot()`이 독립 사본인데 그 사본에 상태를 써서 실패했다. 실제 쓰기 뷰인 `allocated()`로 교정한 뒤 집중 검사 두 건이 통과했다. GUI의 실제 적용 장치와 GameWorld 결선, 입자 뷰는 남는다. 사제 포획·희생·승리 등 제한 행동과 세 공중 유닛의 전용 행동은 실행하지 않았다.

Node v26.13.0 전체 회귀 6,279건이 통과했고 실패·취소·건너뜀은 0건이다. 대응표 오류는 0건이며 E771의 Node v22 전체 회귀 뒤 다섯 번째 v26 회귀다.
