# WB04-E911: Uber 루트와 현재 HUD 자식 뷰

cpppj `UberRootGump`의 0xc8BYTE 확장 필드와 `DockView`를 이식했다. 뷰는 고정된 Uber 토큰의 실제 파생·기본 주소가 같은지 확인하고 매 호출에서 현재 자식 토큰을 다시 읽는다. 자식 표 사본을 만들지 않는다. 기존 `HudDock.registerNormalLayout`은 이 뷰로 같은 실제 divider·패널에 원본 순서의 10개 관계와 화면 제한을 기록한다.

실제 Client Gump 저장소에 Uber와 세 divider, 네 기본 패널을 만들고 기존 HUD 배치 몸체가 같은 노드·serial에 기록하는지 확인했다. 뷰의 필드 변경 재조회, 파생 주소 불일치와 반납도 검사했다. 원본 HUD 2,150행 및 Gump 트리 관측을 포함한 Node 연관 검사 24건이 통과했다.

Uber 전체 생성·루트 교체·재생성, 실제 미니맵·생산·채팅 공장과 앱 HUD 표시·입력은 아직 미연결이다. 이 단위는 HUD 화면 완성을 의미하지 않는다.
