# 미션 Fort 지연 요청의 프레임 경계 (WB04-E950)

기준선 `225553df`의 `UberGump::Tick`은 State Poll 직전부터 대기한 전환만 Poll 뒤에 소비하며, Poll 중 취소되거나 새로 예약된 항목은 같은 Tick에서 실행하지 않는다. `runCampaignRequestFrame`은 같은 `ClientState.takeAfterPoll`을 사용하고 선택된 `LoadMissionFort`의 이름·현재 요청 이미지·소비 여부를 확인한 뒤 실제 State7 설치 장치에 같은 요청을 넘긴다. 다른 전환과 누락된 장치는 해당 위치에서 오류로 남긴다.

실제 앱의 Tutorial1 Guide가 `BridgeTheGap` 요청을 만들고, 검사에서 이 프레임 처리기를 명시적으로 호출해 앱의 실제 State7·State8과 내부 로그인 Ready까지 진행했다. 요청 소비 이후 State·패킷은 앱 프레임을 사용했다. Poll 중 요청 교체·취소, Poll 예외, 이미지 불일치 및 미연결 장치도 검사했다.

이 단위는 앱의 프레임이 요청 처리기를 자동 호출하도록 연결하지 않는다. 현재 World 반납·진행 창 및 실제 브라우저 장치가 남아 있으며, 메뉴에서 미션을 시작하거나 플레이할 수 있다는 뜻이 아니다. 집중 검사 4건과 같은 `ClientState` 검사 3건이 Node v26에서 통과했다.
