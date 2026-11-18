# WB04-E781 Session 신전 그림의 현재 슬롯

cpppj `RawCampaignSession.cpp`의 `DrawRainTemple`, `DrawTemple`, `DrawBridgeAdditional`을 같은 Session의 프레임·음향·다리 그림 몸체로 공개했다. 공통 신전 그림이 읽기 전용 `pool.slot()` 사본을 사용하던 문제를 원본 span과 같은 현재 물리 슬롯으로 고쳤다. 레인 신전의 frame 콜백이 원본 슬롯을 바꾸면 다음 frame 읽기에 그 변경이 반영된다.

실제 Session의 레인 신전에서 첫 frame 콜백이 프레임 4를 7로 바꾸는 검사를 추가했다. 기존 사본 경로로 되돌리면 `[0, 4]`가 나와 실패하고 현재 슬롯 경로에서는 `[0, 7]`이 된다. 실제 상승 상태의 세 그림 순서와 `templeAscend.wav` 음향 호출, 다리 수명 숫자가 꺼진 경로가 SID를 읽지 않는 동작, 반납 관문을 검사했다. 기존 세 신전과 레인 음향 검사도 통과했다. 개발용 다리 수명 숫자 표시 자체와 실제 Renderer/GameWorld 결선은 남는다.

Node v26.13.0 전체 회귀 6,284건이 통과했고 실패·취소·건너뜀은 0건이다. 대응표 오류는 0건이며 E779의 Node v22 전체 회귀 뒤 두 번째 v26 회귀다.
