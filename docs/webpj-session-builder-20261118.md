# Session 건설 사제 연결 (WB04-E692)

기준선 `225553df`의 `RawCampaignSession.cpp` 3544..3569 `MakeBuilder`를 같은 Session의 기준점·World·건설 표면 알림·Graph 접근점·이동 경로·에너지 공급과 Regular 예약에 연결했다. 경로의 마지막 현재 도착 시각과 같은 Kernel 객체의 수명을 확인하고, 속도 조정 시 현재 TYPE의 speed·turningSpeed를 읽는다. 일반 배치 통지의 `buildingMover`는 기도 설치 뒤 같은 상태를 빌린다. 해당 C++ 소스와 기존 Builder·BoardApproach 소스는 기준선 이후 변경이 없다.

실제 생성·포획·희생과 공중 3종 전용 생성·행동을 실행하지 않는 Session 연결 검사 2건, 기존 건설 사제 fixture 370행·접근점 fixture 696입력과 기존 기도 검사 등 집중 9건을 통과했다. 전체 배치·커서와 실제 `Load`, Native 분기는 후속이며 `loaded=false`다.

Node v26.13.0 전체 6,120건 통과(실패·취소·건너뜀 0), 로그는 Git 제외 `extracted/webpj-e692-node26.log`다. Node 22는 E691에서 순차 실행해 6,118건이 통과했으며 이후 전체 회귀는 1/8회다.
