# 실제 Session 로컬 간헐천·AI 등록 (WB04-E700)

기준선 `225553df`의 `RawCampaignSession.cpp` 5650..5653 로컬 간헐천 callback을 실제 Tutorial 1을 적재한 Session에서 검증했다. 관련 C++ 소스는 기준선 이후 변경이 없다. cpppj `CampaignSession_LocalGeysersUseActualAiRegistration`의 실제 두 Player 지역, AI 준비·간헐천 생성·AI 장부 갱신과 반납을 같은 World·Player 기록·AI 메모리로 확인했다.

원본 설정 초기화 뒤 명시한 썬 캐논 프로필은 공중 3종 전용 생산·행동이 아니다. 로컬 간헐천 생성은 실제 월드와 AI 장부에서 각각 14개에서 20개로 증가했고, Player 플래그와 UI 갱신 6회를 확인한 뒤 AI를 정상 반납했다. 실제 게임 창·플레이·포획·희생·공중 3종 전용 행동은 실행하지 않았다. 실제 Load·기존 건설 바인딩 검사를 포함해 집중 21건이 통과했다.

최초 점검은 현재 건설 설정 조회가 없어 중단됐고, 이어 UI 갱신 장치가 없어 중단됐다. 해당 외부 설정과 UI 관찰 장치를 명시 제공한 뒤 실제 생성·등록 본문을 통과했다. 세계 간헐천·너겟 callback과 Fort 모드 snapshot·앱 수명은 후속이다.

Node v26.13.0 전체 6,132건 통과(실패·취소·건너뜀 0). E698의 Node 22 통과 이후 전체 회귀는 2/8회다. 로그는 Git 제외 `extracted/webpj-e700-node26.log`다.
