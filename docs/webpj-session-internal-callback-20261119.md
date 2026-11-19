# 내부 미션 전송의 Session 콜백 수명 (WB04-E893, 2026-11-19)

기준선 `225553df`의 `RawCampaignSession`과 `CampaignTransportServices`는 이후 변경되지 않았다. 기존 웹 Session의 콜백 관문은 외부 전송 대여와 Runtime 핀만 확인해, 실제 내부 `LocalMissionTransport` 수신 중에는 State 정리를 차단하지 못했다. 실제 Session에서 내부 Login을 생성할 때 그 분배기의 등록 세대가 Session 전체를 대여하도록 연결하고, 같은 전송의 활성 콜백을 `connectionCallbackActive()`가 조회한다. 독립 Login 장치 검사에는 Session 대여를 강제하지 않는다. 기존 외부 전송의 콜백 조회도 보존한다.

실제 TheWarBegins 저장 Fort와 내부 로그인으로 패킷 관찰 콜백에서 `cleanupClientStateWorld` 재진입을 시도하여 정리가 차단되는 것을 확인했다. 정상 Ready와 관찰기 예외 양쪽에서 콜백 활성 표시가 복구되고 Session이 반납된다. 집중 검사 3건과 Login·전송·등록표 연관 검사 59건이 통과했다. Native 고정 수신기 대여, 앱 프레임의 Poll·패킷 진행과 실제 미션 플레이는 후속이다.

정해진 주기에 따라 Node v26.13.0 전체 회귀 6,441건이 실패·취소·건너뜀 없이 통과했다.

공유 Session 모듈을 참조하는 fixture 목록 두 항목의 계산 칸을 갱신했다. 이 커밋 전의 모듈 상태는 `worktree`로 기록되며 다음 단위에서 커밋 해시로 갱신한다. fixture 본문은 변경하지 않았다.
