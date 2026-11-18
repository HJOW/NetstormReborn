# WB04-E837 캠페인 Client 미션 시작 효과

기준선 `225553df`의 `CampaignClientServices::MakeCampaignMissionStartEffects` 중 단독 미션 Start가 사용하는 시계·SP 화면 갱신·명령 Gump 갱신·전체 세계 간헐천 생성·카메라 초기화와 AI 능력 진단을 연결했다. 새 JavaScript 할당의 현재 바이트를 그대로 사용하는 기존 `currentAllocationBytes`를 AI 설정에 넘긴다. Client·World와 제공 명령 갱신 대상은 효과보다 오래 살아야 한다.

현재 시각의 재조회, Renderer 무효화→메뉴 갱신→명령 갱신 순서와 World 간헐천·카메라 전달 및 메뉴 부재를 독립 검사했다. 실제 1-1 GameWorld의 내부 로그인·Ready 뒤 같은 Session과 원본 미션 스크립트에 이 효과를 연결하여 SP 3,000, 시작 시각 123.5, 시작 상태와 카메라 갱신을 확인했다. Client UI·시계는 외부 관찰 장치이며 브라우저 Client 전체가 아니다.

스커미시 `prepareWorld`와 나머지 `CampaignClientServices`는 이식하지 않았다. 특히 준비 계획이 없는 일반 미션에 스커미시 생성기를 임의 호출하지 않는다. 실제 앱·HUD와 12차 스커미시 연동은 남는다. 원본 게임 창은 실행하지 않았다.

검증: 효과 경계 1건과 실제 로그인·미션 시작 통합 1건, Node v26.13.0 전체 6,335건 통과(실패·취소·건너뜀 0), 대응표 1,160행 오류 0. E835의 Node v22 전체 검사 뒤 v26 회귀 2/8회다.
