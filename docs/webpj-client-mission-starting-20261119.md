# WB04-E839 Client 미션 State7 앞단

기준선 `225553df`의 `PrepareCampaignMissionStarting`과 `CaptureCampaignMissionStarting`을 같은 Client LocalState·카메라·구름 redraw·HTML 배경 및 요청 이미지에 연결했다. 다음 상태 7의 전체 36DWORD를 현재로 복사하고 다음 상태 번호만 지운 뒤 기본 카메라 분모를 선택하고 정리 전 Zone·재접속 시각/출처를 원본 순서로 보관한다. 판본·중복·Zone·현재/다음 상태 관문은 선택 위치에서 거부한다.

실제 Client 장기 전역과 `MissionFortLoadRequest`의 원본 요청 작성·Zone 복제를 사용해 캡처 시점과 복원 시점의 재접속 비트 4조합, 요청 사본과 현재 상태의 차이, 카메라 분모·redraw·배경 첫 BYTE 및 나머지 블록 보존을 검사했다. 잘못된 요청은 복사 전 상태를 보존한다. 최초 검사에서 카메라의 읽기 전용 좌표 사본을 쓰려다 실패해 실제 64BYTE 물리의 좌표 필드에 검사 입력을 설정했다.

같은 앞단 캡처를 실제 1-1 Session의 내부 Login에 전달하고 현재 상태가 독립 요청 사본으로 덮이지 않으며 기존 Zone이 소비되는 것을 확인했다. `RunCampaignMissionStarting`의 실제 Client 부모·진행 창 및 앱 수명은 아직 남는다. 원본 게임 창은 실행하지 않았다.

검증: Client 서비스 집중 검사 4건과 실제 로그인 통합 1건, Node v26.13.0 전체 6,338건 통과(실패·취소·건너뜀 0), 대응표 1,160행 오류 0. E835의 Node v22 전체 검사 뒤 v26 회귀 4/8회다.
