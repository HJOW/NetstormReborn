# WB04-E717: Factory 연구 명령 수신 등록

`RawFactoryResearch.cpp`의 `RegisterFactoryResearchCommand`를 이식했다. 같은 분배 표의 58h에 빈 관찰기, 버전 1·정책 0, 고정 16바이트와 실제 연구 수신기를 등록하며 선택적인 준비 함수는 수신 직전에 실행한다. 외곽 `RawCampaignSession.registerFactoryResearchCommands`는 실제 Load 이후 같은 Session의 연구 본체를 등록하고 설치 전과 반납 후 호출을 거부한다.

원본 216행의 기존 연구 검사와 실제 패킷 분배·준비 순서·잘못된 선언 길이·Session 설치 관문을 포함한 집중 검사 23건이 통과했다. 이 연구 등록은 아직 로그인 Load 콜백에서 호출하지 않는다. 원본은 그보다 먼저 Carrier 명령 전체와 Construction 명령을 등록하므로 그 순서의 실제 연결을 4D-3e에서 먼저 완성해야 한다. 따라서 E716의 명시 미연결 경계와 서버 후반·응답 왕복 미완료 상태는 그대로다.

새 검사는 공중 유닛 전용 생성·행동, 사제 포획·희생·승리나 실제 게임·브라우저 창을 실행하지 않았다. Native 장기 수신기와 외부 분배표 대여 반납은 4D-8·4D-5에서 별도 처리한다.
