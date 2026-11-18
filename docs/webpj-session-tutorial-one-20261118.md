# WB04-E778 Session 튜토리얼 1 판정 연결

cpppj `RawCampaignSession.cpp`의 `MakeTutorialOne`을 같은 Session의 현재 Player·로컬 제작 장부·Graph·SP·최근 간헐천 목록 및 미션 게이트에 연결했다. 단계와 현재 소유자/Player 공개 상태는 매번 현재 전역에서 읽으며 Home·회수 명령의 원본 초기 DWORD를 Session이 보관한다. C 단계의 간헐천 생성은 기존 실제 `generateLocalGeysers`를 사용한다.

TheWarBegins 실제 Session과 실제 `RawMissionFrameGate`에서 설치 전 거부, 회수/Home 명령, 다리 부류 제작 수 7/8, SP 199/200과 게이트 예약을 확인했다. Graph 검사는 현재 Player의 Home SID와 최근 간헐천 목록에 동일한 실제 사제 SID를 넣은 격리 입력으로 같은 Graph 조회 경로를 확인한 것이며 자연 간헐천 생성 검사가 아니다. 최초에는 장부 배열을 함수로 호출해 실패했고 실제 `bookkeeping.localSecondaryCounts()`로 교정했다. 승패·미션 전체 프레임·자연 완주와 다른 튜토리얼은 실행하지 않았으며 앱 결선은 남는다.

Node v26.13.0 전체 회귀 6,281건이 통과했고 실패·취소·건너뜀은 0건이다. 대응표 오류는 0건이며 E771의 Node v22 전체 회귀 뒤 일곱 번째 v26 회귀다.
