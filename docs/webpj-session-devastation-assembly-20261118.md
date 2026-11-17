# Session 파괴 주문 전체 설치와 Meltdown (WB04-E687)

기준선 `225553df`의 `RawCampaignMeltdown.inc`와 `RawCampaignDevastation.inc`의 남은 typed 설치·수신 부분을 같은 Session에 연결했다. 파괴 주문 설치는 상태·소환·대상 효과·직접 피해·검색·활성화·정기·효과 수신·Meltdown 순서를 유지한다. Meltdown은 기존 실제 World·피해·검색·요청·방송·시청각 장치를 빌린다. 활성화 `5c`는 공통 부모/가상 표 관문을 거치고, 상태 타이머 `7d`는 같은 현재 Host의 지정 Form에 읽어 부착한다. 두 cpppj 파일과 Meltdown 본문은 기준선 뒤 변경이 없다.

전체 설치 순서·수명, 활성화 앞단, 상태 타이머 Form 수신 및 기존 Meltdown 원본 fixture 442행을 포함한 집중 27건이 통과했다. 새 공중 3종 전용 생성·행동 관측이나 검사는 추가하지 않았다. Native 장기 저장소/방송 분기는 4D-8에서 이어서 처리하고, 실제 Session `Load`·미션 플레이는 아직 연결하지 않았으므로 `loaded=false`다.

Node v26.13.0 전체 6,114건 통과(실패·취소·건너뜀 0), 로그는 Git 제외 `extracted/webpj-e687-node26.log`다. Node 22는 E684 이후 여덟 번째 완료 전체 회귀에서 순차 실행한다(현재 3/8회).
