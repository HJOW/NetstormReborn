# WB04-E828 워크샾 연구와 생산 동기화

기준선 `225553df`의 `RawCampaignSession::CommandFactory`, `SelectFactoryKnowledge`, `CurrentTechnologyParent`, `UpdateProductionContents`와 `RawCampaignProductionBindings::Refresh`를 대조했다. Session의 실제 Factory 입력, 현재 기술 부모와 생산 dirty 관문을 공개한다. 기존 내용물 통지·연구·전송·Deck·동기화 객체를 그대로 사용한다.

E827의 같은 캠페인 1-1 Login·Start·워크샾 배치와 전체 Kernel 자연 완공 뒤 연구를 선택한다. 클라이언트 예측 내용물과 Deck 반영, 내부 58h 패킷 수신, 서버 내용물로 대체와 예측 삭제, 생산 자리 수, challenge 관문의 dirty 보존, 일반 모드의 기술 내용물 동기화와 반복 호출 생략을 검사했다. 할당 바이트와 설정은 명시된 검사 입력이다.

처음에는 연구 내용물이 생성되었지만 Deck이 비어 있었다. 생산 재계산이 `bookkeeping.factories`를 읽어 실제 월드가 PostPop에서 사용하는 장기 공유 목록을 놓친 것이 원인이었다. C++의 `Factories()`와 같은 `currentFactories()`로 교정했으며 기존 원본 242행 재계산 대조도 유지했다. 브라우저 HUD 입력과 전체 World 프레임·이후 유닛 생산은 아직 미연결 또는 미검증이다. 실제 게임 창은 실행하지 않았다.

검증: Node v26.13.0 전체 6,321건 통과(실패·취소·건너뜀 0), 대응표 1,160행 오류 0. E827의 Node v22 전체 검사 뒤 v26 회귀 1/8회다. 별도로 생산 재계산/Session 생산 15건과 자연 완공·연구 통합 1건을 통과했다.
