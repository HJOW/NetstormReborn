# WB04-E767 Client 기본 경로·압축 관찰기

기준 `225553df`의 `ClientCampaignTransport.cpp` 295~296행을 따라 Client 장기 기본 등록표에 30h 경로 자료와 7ah 압축 객체 송신 관찰기를 설치했다. 이미 이식된 `observePathData`와 `observeCompressedSquid`를 사용하며 실제 Client storage의 진단을 빌린다. 두 등록은 version 1·policy 0·가변 길이이고 기본 수신기를 추가하지 않는다.

같은 전송의 Session 등록 대여가 두 관찰기를 덮어쓴 다음 반납해도 기본 관찰기 정체성이 복구되는지 확인했다. 수신 방향은 입력을 읽지 않으며 송신 방향은 각각 현재 nodeCount와 dataLength로 선언 길이를 갱신한다. 센티넬은 같은 storage 진단에 전달하고 진단이 복귀한 뒤 현재 값으로 길이를 쓴다. 관련 Client 전송 집중 검사 20건이 통과했다. 실제 Client의 채팅·연결 종료·69h 기술 목록과 Native 고정 수신기는 아직 남는다.

Node v26.13.0 전체 회귀 6,265건이 통과했고 실패·취소·건너뜀은 0건이다. 대응표 검사 오류는 0건이며 E761의 Node v22 전체 검사 뒤 네 번째 v26 전체 회귀다.
