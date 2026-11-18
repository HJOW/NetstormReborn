# WB04-E751 미션 로그인 실제 공유 상태 통합 검사

E750에서 열린 TheWarBegins(캠페인 1-1) 내부 로그인 경로를 cpppj의 세 통합 검사와 대조했다. 원본 Fort와 실제 Session·내부 전송을 쓰며 브라우저 앱이나 실제 게임 창은 실행하지 않았다.

- `CampaignSession_LocalLoginActualHandshakeAndWorldLoad`: phase 8→9→10→13→14, Player subscriber 1과 flags `0x43`, 채팅·알림·갱신, 12개 수신 패킷의 순서와 다음 Pump에서 추가 수신이 없음을 확인했다. `67h` 뒤 옵션 22개, 에너지 반경 제곱 900 및 TYPE 122 비용 3000도 같은 실제 소유자에서 대조했다.
- `CampaignSession_ActualLoginUsesClientCurrentCoreSlot`: 외부 Client의 Subscriber·Player·CoreData를 같은 Session에 빌려 실제 로그인 뒤 Core 현재 뷰가 Player 1의 `+0x6c`를 가리키고 active가 0임을 확인했다.
- `CampaignSession_InitRequestUsedByActualLogin`: `writeMissionFortRequest`/`captureMissionFortZone`이 만든 86바이트 Zone의 subscriber를 7로 설정해 실제 로그인에 사용했고 같은 Player 행·전역에서 7을 확인했다. 요청의 Zone 소비·빈 버퍼·순번과 State7 요청 보존을 검증했다.

기준은 `225553df`의 `cpppj/tests/CampaignSessionTests.cpp`에 있는 세 검사(현행 2615~2630·2667~2678행)다. 기준선 이후 같은 파일은 11행 추가·7행 삭제됐으나 세 검사 밖의 표시·Kernel 검사 변경이다. 단독 순수 로그인에 필요한 채팅·알림·SP 갱신은 관찰용 외부 효과로 공급했다. 집중 검사 85건과 Player Core 관련 7건이 통과했다. 31h 나머지 명령·다른 미션/데모·원본 프레임/입력·실제 앱 연결, `LocalLoginActualHandshakeAndWorldLoad` 검사의 사제 이동 후반은 여전히 남아 있다.

Node v26.13.0 전체 회귀 6,244건이 통과했고 실패·취소·건너뜀은 0건이다. 대응표 오류는 0건이며 E746의 v22 검사 뒤 여섯 번째 v26 전체 회귀다.
