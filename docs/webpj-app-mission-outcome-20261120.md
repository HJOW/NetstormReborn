# WB04-E973 앱 미션 승패·AI 장치

앱 Session의 승패 판정에 현재 Guide의 데모 여부, 대화창 차단, 실제 미션 섹션 Tell, 팝업 닫기와 안내 표시 후 Tutorial 자기 정지를 연결한다. 안내 정지는 현재 Tutorial 소유자가 없으면 누락을 보고한다. 웹의 기존 TellWindow는 현재 모달을 직접 소유하므로 차단 조회에 그 상태를 사용하며 원본 Gump HTML 수명과의 완전 통합은 후속이다.

AI와 생산 Deck의 미초기화 할당 입력은 JavaScript의 새 `Uint8Array` 내용을 그대로 보존하는 기존 `currentAllocationBytes`를 사용한다. AI 차단 조회는 현재 Client Gump 직계 팝업을 읽는다. Session 설치 시 호출자 AI·생산·승패 장치는 앱 기본값과 병합한다.

검증: 실제 Tutorial1 로그인·Start 뒤 Session 승패 판정, 현재 대본 `A.` 안내·대화창 차단, 할당 바이트 보존, 원본 승패 관측 364행과 실제 Gump 팝업 등 Node 24건 통과. 실제 승패·사제 사건과 Tutorial 프레임 자동 실행은 아직 검증하지 않았다. E970 고정 Node 26 정기 전체 회귀는 실행 중이다.
