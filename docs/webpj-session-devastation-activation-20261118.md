# Session 주문 활성화 수신 (WB04-E684)

기준선 `225553df`의 `RawCampaignDevastation.inc` 120..129를 같은 Session에 연결했다. 현재 로컬 소유자와 기존 비용·SP 장치를 빌리며, 40바이트 예약에 대응하는 일반 `RegularProcess`를 확보한 뒤 원문처럼 타입 61·flags `0x50`으로 직접 부착하고 반환 후 현재 시각으로 사건을 초기화한다. 미부착 예약은 Session 반납 시 정리한다. 해당 cpppj 소스와 주문 활성화·프로세스 본문은 기준선 뒤 변경이 없다.

실제 주문에 대한 대상 효과는 실행하지 않고 예약의 Host·부착·시각·payload·일반 writer·수명과 오류 순서를 검증했다. 기존 활성화 원본 fixture 434행을 포함한 집중 10건이 통과했다. 공중 3종 전용 생성·행동 관측과 검사 및 사제 포획·희생 재현은 추가하지 않았다. 주문 정기·효과 수신과 실제 미션 Load는 후속이며 `loaded=false`다.

Node v26.13.0과 Node v22.23.3 전체 회귀가 각각 6,110건 통과했다(실패·취소·건너뜀 0). HJOW-Athlon에서 Node 26 종료 뒤 Node 22를 실행했다. 로그는 Git 제외 `extracted/webpj-e684-node26.log`, `extracted/webpj-e684-node22.log`다. 이후 Node 22 주기는 0/8회다.
