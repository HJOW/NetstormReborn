# WB04-E849 Session Dude 부모와 통지 공개

기준선 `225553df`의 `RawCampaignSession::DudeBehavior`·`Dudes` 조회를 이식했다. 적재된 같은 Session이 생성한 Dude 부모와 피격 처리기의 통지 객체를 그대로 빌리며, 설치 전과 반납 후 접근을 거부한다. 기준선 이후 해당 C++ 파일의 변경은 없다.

cpppj `CampaignDude_ActualArrivalUsesPopulationLifetime`에 대응해 실제 TheWarBegins Fort에서 Dude를 Pop한 뒤 동일 인구 목록에 추가되고 부모 도착 시 삭제·목록 반납되는 것을 확인했다. `CampaignDude_ActualPathNoticeSpeedAndArrival`의 앞부분에 대응해 실제 지도에서 통행 가능한 칸을 찾아 통지의 경로 시작과 동일 목적지의 기존 경로 재사용을 확인했다. 새 집중 검사 2건과 Node v26.13.0 전체 6,364건이 통과했다(실패·취소·건너뜀 0). 속도별 시간표와 경로 자연 도착은 이 단위에서 대조하지 않았다.

전체 Client·브라우저 앱 결선과 Native 저장소 경계는 남아 있다. 원본 게임 창은 실행하지 않았다.
