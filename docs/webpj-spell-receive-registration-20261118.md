# WB04-E738 AcidZap과 주문 수신 등록

현재 Session 분배기에 `61h` AcidZap, `5ch` 주문 활성화, `5fh` 주문 효과, `7dh` 상태 타이머를 등록했다. 각 패킷은 이미 설치된 실제 Session 처리기와 현재 주문 상태·Process Host로 전달된다. typed 경로의 고정 또는 가변 길이와 빈 관찰기 계약을 유지한다.

근거는 기준선 `225553df`의 `RawCampaignSession.cpp`와 `RawAcidZap.cpp`, `RawBombActivation.cpp`, `RawBombEffect.cpp`, `RawBombTimerPacket.cpp`다. 이 파일들의 기준선 이후 변경은 없다. 해당 본체들은 이전 단위에서 10.78 fixture 각각 927·434·1,676·602행과 대조했다. 새 fixture는 추가하지 않았다.

Tutorial 1 Session 분배기에서 빈 대상의 AcidZap·활성화·효과 수신과 타이머 권한 진단을 확인했다. 집중 검사 26건이 통과했다. 실제 공격·주문 효과 재현과 미션 왕복은 이번 검사가 아니다.

Node v26.13.0과 v22.23.3 전체 회귀를 순차 실행해 각각 6,215건이 통과했다. 대응표 오류는 0건이며 다음 Node v22 전체 재검사 주기는 0/8회다.

다음은 `6bh` 의식 선택, `6dh` 사제 기도와 `75h`/`76h` Meltdown 수신 등록이다. 나머지 Carrier 등록, 로그인 왕복과 앱 미션 연결은 남아 있다.
