# Session AI 자원 선택 (WB04-E654)

기준선 `225553df`의 `RawCampaignSession.cpp` ConnectAiMovement 2960..2963에서 `RawAiResourceChoice`를 같은 Session의 AI 본체·Pool·현재 Graph·신전 중심·운반 내용물·AI 이동과 로그/진단 장치에 연결했다. 현재 너겟 TYPE과 무효 Graph도 호출 때 읽는다. AI 운반 행동과 배치·전체 Runtime은 후속이다.

새 Session 검사는 AI 이동 선행·중복 관문, 실제 AI 본체의 신전 없는 좌표, 같은 Pool의 내용물 수와 Graph 조회를 확인한다. 기존 cpppj 계약을 옮긴 독립 부품 검사 포함 집중 29건이 통과했다. 원본 실행 fixture는 해당 부품에 없고 실제 AI 플레이·포획과 전체 Load는 실행하지 않았으며 `loaded=false`다.

Node v22.23.3·v26.13.0 전체 각각 6,075건 통과(실패·취소·건너뜀 0), v24는 사용자 지시대로 생략한다. 전체 로그는 Git 제외 `extracted/webpj-e654-node22.log`·`extracted/webpj-e654-node26.log`다.
