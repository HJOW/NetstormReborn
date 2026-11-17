# Session AI 이동 앞단 (WB04-E653)

기준선 `225553df` `RawCampaignSession.cpp`의 `ConnectAiMovement` 2955..2959에서 객체 가상 이름과 `RawAiMovement`를 실제 Session의 AI 본체·Pool·현재 권한·접근점·경로·의식·내부 전송에 연결했다. 사용자 운반 이동과 동일한 `CarrierMoveState` 헤더를 빌리며 로그 종류와 AI owner도 호출 시 읽는다. AI 자원 선택·나머지 행동·전체 Runtime은 아직 설치하지 않았다.

신규 검사는 선행 설치 관문, 같은 Pool, 비서버 AI 좌표 이동에서 사용자와 공유하는 변경된 헤더가 실제 송신에 들어가는 것을 확인한다. 최초 실행에서 테스트용 AI 할당 바이트 제공을 빠뜨려 실패했고, 실제 할당 계약을 제공해 집중 검사 24건이 통과했다. 독립 원본 fixture는 이 AI 이동 부품에 없으며 기존 cpppj 계약을 옮긴 검사다. 실제 AI 플레이·사제 포획 및 전체 Load는 실행하지 않았고 `loaded=false`다.

Node v22.23.3·v26.13.0 전체 각각 6,074건 통과(실패·취소·건너뜀 0), v24는 사용자 지시대로 생략한다. 전체 로그는 Git 제외 `extracted/webpj-e653-node22.log`·`extracted/webpj-e653-node26.log`다.
