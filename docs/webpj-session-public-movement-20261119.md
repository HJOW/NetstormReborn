# WB04-E848 Session 운반체·Dude 이동 공개

기준선 `225553df`의 `RawCampaignSession::StartCarrierMovement`와 `StartDudeMovement`를 웹 Session에 공개했다. 설치된 같은 Session의 경로 시작기를 사용하므로 부모 종류·가상 표·이동 종류 검사, 경로 탐색과 실제 Kernel 부착 및 Dude 정지·도착 효과는 기존 구현이 담당한다. 기준선 이후 해당 C++ 파일의 변경은 없다.

실제 TheWarBegins Fort를 적재한 독립 Session에서 썬 워커와 Dude를 생성해 각 경로 Form과 Kernel이 같은 반환 객체를 가리키는지 확인했다. 설치 전과 반납 후 호출 및 사제를 잘못된 부모로 전달한 경우도 검사했다. 집중 검사 1건과 Node v26.13.0 전체 6,362건이 통과했다(실패·취소·건너뜀 0). 이는 이동 시작 경계의 검사이며 게임 화면·사용자 입력·자연 도착 전체를 검증한 것은 아니다.

남은 일은 Dude 본체 공개 조회와 다른 Session 공개 장치, 실제 Client·브라우저 앱 수명 연결이다. 원본 게임 창은 실행하지 않았다.
