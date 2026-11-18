# WB04-E782 공통 튜토리얼 게이트와 Session 조회

cpppj `RawCampaignSession.cpp`의 `CheckMissionOutcome`, `MissionPriestDead` 공개 경로를 같은 Session의 기존 승패 판정기에 연결했다. 기존 `RawCampaignTutorialOneBindings`가 실제 Session의 튜토리얼 1 판정기를 생성해 같은 공통 게이트에 연결할 수 있다.

TheWarBegins 실제 Session에서 시작 전 공통 프레임이 효과 없이 반환하는 것과, 시작 뒤 Home 입력이 공통 게이트를 거쳐 실제 튜토리얼 1의 다음 단계·잠금을 예약하는 것을 확인했다. 이 검사에서는 `checkMissionOutcome`을 관찰 대역으로 교체해 호출 순서만 확인했으며 실제 승패 판정, 사제 포획·희생, 적 공격 및 자연 완주를 실행하지 않았다. 본체 승패 분기·실제 UI와 GameWorld·앱 연결은 여전히 남는다. Session 반납 뒤 두 공개 경로가 낡은 소유자 접근을 거부하는 것도 확인했다.

Node v26.13.0 전체 회귀 6,285건이 통과했고 실패·취소·건너뜀은 0건이다. 대응표 오류는 0건이며 E779의 Node v22 전체 회귀 뒤 세 번째 v26 회귀다.
