# 캠페인 생산 갱신 입력 연결 (WB04-E875)

cpppj 기준선 `225553df`의 `CampaignClientServices.cpp`는 신전 소유권과 신전 자산 PostPop에 같은 생산 갱신 함수를 제공한다. `RawButtonControls::RefreshProduction`은 현재 Gump 입력의 `Route(0x02000003, 0, 0, true)`를 호출한다. 웹 `bindCampaignProductionRefresh`도 두 경로에 같은 함수를 두고 기존 `GumpInput.route`에 같은 인자를 전달한다. 캡처 우선과 숨겨진 노드의 처리 여부는 공통 입력·이벤트 구현이 담당한다.

실제 `RawCampaignOwnership`의 신전 소유자 변경에서 Gump 입력과 숨겨진 노드로 사건이 도달하는 것을 확인했다. 같은 PostPop 콜백을 직접 실행해 캡처된 노드에 먼저 전달하고 캡처를 해제하는 것도 확인했다. 자산 PostPop 몸체와 Gump 입력의 기존 원본 대조를 포함한 집중·연관 검사 60건이 통과했다. 이 검사는 PostPop 몸체에서 새 바인더까지의 전체 수명 통합은 아니며, 실제 앱의 Gump 소유자·Session 조립과 생산 화면 갱신은 후속이다. 게임 창은 실행하지 않았다.

정해진 주기에 따라 Node v22.23.3 전체 회귀 6,416건이 실패·취소·건너뜀 없이 통과했다. Node v26 전체 회귀는 다음 주기에 실행한다. 처음 연관 검사에서 존재하지 않는 `raw-asset-post-pop.test.mjs`를 지정해 실행되지 않은 것을 확인했고, 실제 `asset-post-pop.test.mjs`를 추가 실행했다.
