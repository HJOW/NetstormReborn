# WB04-E786 Session 자식 증빙과 State 반납 정책

cpppj `RawCampaignSession.cpp`의 `StateChildrenReturned`와 외곽 소멸자 분기를 연결했다. 같은 Session의 Process Host와 Player 링크가 각각 정상 반납되고 중립을 뺀 Player 1~8의 AI 정리 증빙도 모두 소비된 경우에만 `WithdrawForState`를 사용한다. 이때 Client의 준비된 Player·커서·편집 전역을 보존한다. 하나라도 빠지면 일반 `Withdraw`로 공개 Runtime을 먼저 반납하고 기존 일반 초기화를 수행한다. 증빙 조회 오류는 콘솔에 원인을 남긴 뒤 일반 반납을 계속한다.

실제 Session 두 개에서 Process·Player·8개 AI 증빙을 순서대로 반납하고 Session을 종료했을 때 공유 상태가 보존되는지 확인했다. 나머지는 동일하되 8번 AI 증빙만 누락한 Session은 일반 종료로 상태가 초기화됐다. 각 증빙의 교차·중복 거부 및 실제 Host/Deck 정리는 E783~E785의 집중 검사를 함께 실행했다. 테스트는 빈 게임 상태의 수명만 다루며 실제 미션 플레이·승패·포획이나 앱 전환을 실행하지 않았다. Native 건설 장치와 실제 Client State 전체 정리·앱 연결은 남는다.

Node v26.13.0 전체 회귀 6,289건이 통과했고 실패·취소·건너뜀은 0건이다. 대응표 오류는 0건이며 E779의 Node v22 전체 회귀 뒤 일곱 번째 v26 회귀다.
