# 실제 Tutorial 1 Battle snapshot 재적재 (WB04-E698)

기준선 `225553df`의 `RawCampaignSession.cpp` 5903..5910·5920..5982와 기존 snapshot 자식들을 E697에서 실제 적재한 Tutorial 1 Session에 사용했다. 관련 C++ 소스는 기준선 이후 변경이 없다. 같은 `CampaignTemplateStorage`, 실제 생산 Deck 계산과 원본 명령 큐를 사용하며, 미정 Deck 할당 바이트와 외부 설정·UI·음향은 명시 입력으로 제공했다.

Battle snapshot의 Terr00·Terr01, 120BYTE Territory, 4BYTE Money를 확인하고 별도 Session에 결과를 다시 적재했다. 두 월드의 객체 1,875개에 대해 타입·소유자·프레임·좌표 다중집합이 일치했다. SID는 재적재 순서에 따라 달라 실제 데이터와 동일하다고 주장하지 않는다. 직접 게임 구동이나 플레이·포획·희생·공중 3종 전용 행동은 실행하지 않았다.

최초 점검에서 현재 Session이 Battle인데 Fort snapshot을 호출해 원본의 Fort 모드 진단이 발생했다. 이를 우회하지 않고 실제 모드에 맞게 Battle snapshot을 선택했다. 이후 Deck 할당 초기 바이트와 실제 설정 입력이 없어 각각 선택 위치에서 중단되어 명시 입력을 공급했다. 기존 원본 snapshot fixture 2,728관측과 새 실제 재적재 검사 등 집중 9건이 통과했다. Fort 모드의 실제 Session 통합과 외곽 callback, 앱 수명·session-dump·미션 화면은 아직 남는다.

Node v26.13.0과 v22.23.3 전체 각각 6,130건 통과(실패·취소·건너뜀 0). 두 버전은 순차 실행했으며 Node 22 이후 전체 회귀는 0/8회다. 로그는 Git 제외 `extracted/webpj-e698-node26.log`, `extracted/webpj-e698-node22.log`다.
