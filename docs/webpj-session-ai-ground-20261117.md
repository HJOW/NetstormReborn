# Session AI 지상 표적 조회 (WB04-E657)

기준선 `225553df`의 `RawCampaignSession.cpp` ConnectAiMovement 2970..2971에서 `RawGunGroundTarget`을 같은 Session의 Pool·해시·현재 TYPE 및 Gun 표적 처리기와 연결했다. 제외 TYPE·Depend TYPE·metamap·권한과 checking은 같은 현재 세션에서 다시 읽으며 원본 진단은 실제 장치에 전달한다.

신규 검사는 선행/중복 설치와 동일 Pool 및 현재 권한·TYPE 조회를 확인했다. 기존 cpppj 계약을 이식한 독립 부품 검사 포함 집중 31건이 통과했다. 이 부품의 원본 실행 fixture는 없으며 실제 적 공격·AI 플레이·전체 Load는 실행하지 않았다. 배치 표적 평가와 전체 Runtime은 후속이고 `loaded=false`다.

Node v22.23.3·v26.13.0 전체 각각 6,078건 통과(실패·취소·건너뜀 0), v24는 사용자 지시대로 생략한다. 전체 로그는 Git 제외 `extracted/webpj-e657-node22.log`·`extracted/webpj-e657-node26.log`다.
