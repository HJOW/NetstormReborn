# Session 파괴 주문 상태 앞단 (WB04-E678)

기준선 `225553df`의 `RawCampaignDevastation.inc` 50..60을 같은 Session의 실제 188 TYPE·A 프레임·시전 시간·비용·자산별 음향/지연·동맹·상태 주문에 연결했다. 아직 전체 `ConnectDevastation`이 아니며 Wind/맨 오 워 주문과 나머지 파괴 효과는 후속이다. 해당 cpppj 소스와 상태/펜스 부모 소스는 기준선 뒤 변경이 없다.

원본 PE `bomb-status-x86.tsv` 612행(SHA-256 `e01a5f5499029fdeac910ed106385e9acca1f1443e98e048ce9961d62092db26`)과 실제 Session의 타입 표·상태 자격·설치/반납을 포함해 집중 14건 통과했다. 연결 중 발견한 기존 펜스 분배의 슬롯 사본 문제를 같은 물리 슬롯 대여로 고쳐 진단 callback이 바꾼 TYPE을 Rebuild와 Refresh에서 재조회하도록 했고 별도 계약 검사로 확인했다. 공중 유닛 전용 생성/행동 검사는 추가하지 않았다. 실제 미션 Load는 실행하지 않았으며 `loaded=false`다.

Node v26.13.0 전체 6,105건 통과(실패·취소·건너뜀 0), 로그는 Git 제외 `extracted/webpj-e678-node26.log`다. Node 22는 E677 후 여덟 번째 완료 전체 회귀에서 다시 실행한다.
