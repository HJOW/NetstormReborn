# Session 이동 전체 설치 (WB04-E671)

기준선 `225553df`의 `RawCampaignSession.cpp` 3930·3961과 3009..3011에서 빠져 있던 설치 순서를 연결했다. `connectMotion()`은 실제 도착·이동·의식·회수·AI·경로 송신·다리 대피 부품을 같은 Session에 순서대로 설치한다. `connectAiMovementAll()`은 AI 행동과 Runtime까지 설치한다. 기준선 뒤 `RawCampaignSession.cpp`와 `RawCampaignRitual.inc` 변경은 없다.

실제 기반 owner에서 한 번의 전체 설치가 같은 Pool·World에 행동 및 송신 장치를 제공하는지, 중복과 반납을 거부하는지, 중간 실패 시 이미 설치한 접두는 남고 이후 장치는 만들어지지 않는지 신규 2건과 관련 집중 34건을 확인했다. 실제 `Load()`·로그인·미션 플레이는 미실행이며 `loaded=false`다. Native 장기 분기는 4D-8, 다음 작업은 4D-2d AI 공개 전달이다.

Node v26.13.0 전체 6,093건 통과(실패·취소·건너뜀 0), 로그는 Git 제외 `extracted/webpj-e671-node26.log`다. Node 22는 E669 후 여덟 번째 전체 회귀에서 다시 실행한다.
