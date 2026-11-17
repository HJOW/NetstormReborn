# Session AI 운반 행동 (WB04-E655)

기준선 `225553df`의 `RawCampaignSession.cpp` 2945..2946·2951..2952·2964..2968에서 `RawAiCarrierActions`를 같은 Session의 Player·사제 상태·경로·간헐천·자원 선택과 현재 권한/시각/진단에 연결했다. CarrierDrop은 원본 설치 순서상 뒤 단계이므로 그 효과에 실제 도달하면 설치 전 오류를 낸다. AI 전체 Runtime을 우회해 실행하지 않는다.

신규 검사는 선행 설치·같은 Pool·빈 AI 본체 자연 반환·현재 참여 기록 및 CarrierDrop 미설치 경계를 확인했다. 기존 cpppj 계약 이식 부품 검사 포함 집중 33건이 통과했다. 새 실제 포획·희생·공중 유닛 행동 관측은 하지 않았으며 전체 Load와 AI 자연 플레이는 후속이다. `loaded=false`를 유지한다.

Node v22.23.3·v26.13.0 전체 각각 6,076건 통과(실패·취소·건너뜀 0), v24는 사용자 지시대로 생략한다. 전체 로그는 Git 제외 `extracted/webpj-e655-node22.log`·`extracted/webpj-e655-node26.log`다.
