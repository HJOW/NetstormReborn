# 실제 Session 기술 획득 callback (WB04-E699)

기준선 `225553df`의 `RawCampaignSession.cpp` 6080..6099와 `RawNewTechnology` 결선을 E697의 실제 Tutorial 1 Load 뒤 검증했다. 관련 C++ 소스는 기준선 이후 변경이 없다. 현재 같은 Player 기술 부모와 내용물, Battle snapshot 및 실제 명령 큐를 사용하고, 초기 미정 Deck 바이트와 외부 설정·안내는 명시적으로 제공했다.

실제 썬 캐논 TYPE 128을 획득하자 저장된 Technology가 `[1, 128, 4]`가 됐다. 다시 획득해도 기술 내용물은 중복되지 않았으며 두 번 모두 새 Battle snapshot과 이름·그림·안내가 순서대로 실행됐다. 실제 게임 창·플레이·포획·희생·공중 3종 전용 행동은 실행하지 않았다. 기존 원본 기술 fixture와 실제 적재·snapshot 검사를 포함해 집중 19건이 통과했다.

최초 점검에서는 외부 이름 조회에 표시용 보기의 존재하지 않는 속성을 사용해 안내 지점에서 실패했다. 실제 자산 정의의 이름을 사용하도록 검사 장치를 고친 뒤 통과했다. 실제 로컬/세계 간헐천·AI 등록과 보충 너겟 callback, Fort 모드 snapshot 및 앱 수명은 후속이다.

Node v26.13.0 전체 6,131건 통과(실패·취소·건너뜀 0). E698의 Node 22 통과 이후 전체 회귀는 1/8회다. 로그는 Git 제외 `extracted/webpj-e699-node26.log`다.
