# WB04-E826 실제 로그인 뒤 World 운반 입력

기준선 `225553df`의 `CampaignSession_WorldInputActualLoginThreeCarriersAndKernel`에 대응해 캠페인 1-1의 실제 Fort와 내부 Login/Ready 후 같은 World의 선택·호버·클릭을 검사했다. 사제·골렘·벌룬은 각각 독립 Session을 사용했다. 40h 패킷의 대상 SID·좌표와 전송 전 선택 해제, 같은 Kernel의 연속 프레임에 의한 자연 도착·경로 반납, SP와 정리 깊이 보존을 확인했다. 집중 검사 1건(세 종류)이 통과했다.

Kernel 진행은 `Session.runFrame`을 명시 호출하며 아직 전체 `GameWorld::Step`이나 브라우저 사건 루프가 아니다. 워크샾의 전체 Kernel 자연 완공·후속 생산도 남는다. 기준선 뒤 `516f2d8a5`는 같은 C++ 시험 파일의 별도 그림자·공유 Kernel 검사만 바꿨고 이 대상 검사와 관련 제품 소스의 변경은 없다. 게임 창은 실행하지 않았다.

검증: Node v26.13.0 전체 6,320건 통과(실패·취소·건너뜀 0), 대응표 1,160행 오류 0. E819 Node v22 전체 검사 뒤 v26 전체 회귀 7/8회다.
