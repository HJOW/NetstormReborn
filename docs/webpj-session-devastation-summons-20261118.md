# Session 파괴 주문 소환 연결 (WB04-E679)

기준선 `225553df`의 `RawCampaignDevastation.inc` 61..82를 typed Session에 연결했다. 두 주문은 같은 자산별 signed `spawns` 배열과 현재 풀·서버를 빌리며, Wind는 기존 실제 프로세스 Host를 사용한다. Mano는 같은 장기 `GameAuxState`의 세 주문 TYPE와 RainFlyer TYPE, 현재 SHP 진단·World 장치를 읽는다. 웹의 미초기화 할당 바이트는 기존 `currentAllocationBytes` 적응을 사용한다. 해당 cpppj 파일은 기준선 뒤 변경되지 않았다.

공중 3종 전용 생성·행동 관측이나 검사는 추가하지 않고 cpppj와 정적으로 대조했다. 기존 일반 보조 상태의 보호 PE 정적 대조에 장기 TYPE 초기값을 포함했다. Native 장기 `BombSpawns` 분기는 구현된 척하지 않고 4D-8까지 명시적으로 거부한다. 나머지 파괴 주문과 실제 Load는 미연결이므로 `loaded=false`다.

Node v26.13.0 전체 6,105건 통과(실패·취소·건너뜀 0), 로그는 Git 제외 `extracted/webpj-e679-node26.log`다. Node 22는 E677 이후 여덟 번째 완료 전체 회귀에서 순차 실행한다(현재 2/8회).
