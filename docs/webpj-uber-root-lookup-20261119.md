# WB04-E914: 실제 Uber HUD 등록 조회

기준 cpppj `RawUberRootControls.cpp`의 `Reference`와 `Current`를 웹의 `RawUberRootLookup`으로 옮겼다. Client가 소유한 동일한 Gump 등록표와 실제 노드를 매번 조회한다. serial 검색은 루트 0을 제외한 등록 순서를 따르고, 주소가 없거나 삭제된 노드 및 과거 세대를 제외한다. 현재 HUD 판정은 Uber 파생 루트와 일곱 필수 자식의 생존 상태를 확인하며, 이전 미니맵이 없어도 현재 미니맵 전역이 다른 토큰을 가리키는 경우 원본과 같이 유지한다.

실제 ClientGumpRoot와 UberRootNode를 생성·교체하고 실제 등록, 반납 및 플래그 변이로 등록 순서·빈 주소·삭제·미니맵 토글·루트 반납을 확인했다. 관련 Node 검사 22건이 통과했다. 기존 HudDock의 독립 원본 관측 2,150행도 같은 검사에 포함한다.

이는 HUD의 조회 경계만 구현한 것이다. 실제 HudDock 분배·구분선 공장·미니맵/생산/채팅 공장·출력 및 미션 화면은 아직 연결하지 않았고 전체 RawUberRootControls는 partial로 유지한다.
