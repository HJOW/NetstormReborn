# WB04-E788 일반 종료의 Player 링크 정리 순서

cpppj `RawCampaignSession.cpp`의 Impl 소멸 순서에 맞춰 실제 AI 정리 뒤 Process Host보다 먼저 Player 기술 링크·개수·Deck을 정리한다. 세션 공개 `ReleasePlayerLinks`와 일반 종료는 같은 owner의 정리 몸체를 사용한다. 정상 State 반납 증빙을 이미 소비한 Player 링크는 재정리하지 않는다. 정리 오류는 콘솔에 남기고 나머지 Process·모듈·대여 반납을 계속한다.

실제 Session에 생산 Deck과 Regular Process를 만든 검사에서, 수정 전 Process 소멸 시점에는 Player 링크·개수·Deck 토큰과 할당이 남아 있어 실패했다. 수정 후 그 시점에 세 필드와 Deck 할당이 모두 반납돼 통과했다. 잘못된 Deck 토큰 경로는 앞선 쓰기만 남긴 채 오류를 기록하면서 Process 소멸과 게임 전역 대여 반납을 계속하는지도 확인했다. E786의 State 증빙 경로와 E784의 Player 증빙 검사도 다시 통과했다. 실제 Client/앱 종료 및 Native 건설 수명은 남는다.

Node v26.13.0 전체 회귀 6,291건이 통과했고 실패·취소·건너뜀은 0건이다. 대응표 오류는 0건이며 E787의 Node v22 전체 회귀 뒤 첫 번째 v26 회귀다.
