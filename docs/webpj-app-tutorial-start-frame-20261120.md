# WB04-E975 앱 프레임의 Tutorial Start

메인 메뉴 상태의 현재 World가 로컬 Ready가 되면 같은 Guide·Session의 Start와 Tutorial 실행기를 준비하고 `completeLocalConnection()`으로 표시용 객체·팔레트를 확정한다. 이어 Guide의 원본 단계 3이 같은 Start를 실행하며 단계 4는 같은 Tutorial 프레임을 사용한다. 연결 콜백 재진입 중에는 완료하지 않는다. 이 자동 완료는 현재 기존 웹 메뉴 흐름에 한정한다.

검증: 실제 Tutorial1 Guide의 Fort 요청 → 음향 준비 → State7/8 → 내부 Ready → Start → 첫 `A.` 안내 창과 게임 정지 → 창 닫기 후 7프레임 재개까지 앱 프레임을 사용했다. 실제 시작 SP와 명령 Gump 알림도 확인했다. 원본 미션 수명 831행·공통 프레임 328행·시계/음향 경계 등 Node 25건 통과.

아직 메뉴 `MissionBegin`/`StartDemo`는 Guide에 연결되지 않았고, 월드 Kernel 프레임·합성·입력·HUD·미션 교체도 미완성이다. 이 검사는 Guide를 직접 시작하고 Node에서 앱 프레임을 실행하므로 브라우저 플레이 가능성이나 자연 완주를 증명하지 않는다. E970 고정 Node 26 회귀가 실행 중이며 E974 고정 Node 22 회귀는 그 뒤 실행할 준비를 마쳤다.
