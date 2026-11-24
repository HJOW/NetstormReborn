# Session 상태 안내의 실제 MessageBoard 연결 — WB04-E1326

2026-11-24, HJOW-Athlon. RawCampaignSession 잔여를 점검하며 Tutorial3의 실제 로컬6ch 수신이 SP2000→2125를 쓴 뒤 `SP 통지의 실제 MessageBoard 장치 미연결`로 실패하는 것을 재현했다. 송신자 자금2000과125 SP 패킷은 명시한 검사 입력이며 자연 AI 송금/자연 완주를 확인한 결과가 아니다. 초기 탐색은 `extracted/webpj-e1326-money-probe.log`, 수정 뒤 같은 입력은 `extracted/webpj-e1326-money-fixed.log`에 있다.

`RawCampaignSession.#receiveConstructionMoney`와 동맹 수신은 Login의 `status` 장치를 요구하지만 앱 `_campaignLoginServices`가 제공하지 않았다. C++ `CampaignClientServices.cpp`의 실제 `services.localLogin->status`는 현재 `controls.Warnings()->BattleStatus(kind,owner,parameter)`를 호출하고 장치 부재를 거부한다. 웹도 현재 생산 UI의 `warnings().battleStatus`를 조회해 signed32 인자를 전달하도록 연결했다. 공통 Login 서비스 목록에 이미 있는 칸을 채웠으며 CPP 수정은 필요 없다.

미션의 MessageBoard 억제는 유지한다. kind4 수신 안내는 전달되지만 missionPost가 참이면 표시되지 않는다. 이전 오류는 이 정상 억제에 도달하기 전에 장치 자체가 없어 발생했다. 검사에서는 실제 송금 수신→`battleStatus(4,2,125)` 한 번→정확히125 SP 증가→예외 없음→후속10프레임을 확인했다. 미션 플래그를 바꾸지 않고 허용 종류16의 Game Time 안내가 실제 표에 생성되는 양성 경로와 UI 부재 시 명시 오류도 확인했다.

Chromium2/2 통과(50.5초): 새 실제 수신/메시지 표 검사와 기존 Players 메뉴의300·700·1000/All 송금·동맹 요청/해제·스팸 제한이다. 관련 MessageBoard/Warning·manifest·대응표·번들 신선도 Node39/39 통과(54.2초). webpack 번들3,429,243바이트로 한도 통과. 로그는 `extracted/webpj-e1326-{browser,focused,build}.log`다. Node 전체 회귀는 직전 E1325의6,847/6,847을 유지하며 이번은 집중 검사로 마감했다. 주기는Node26 E1325 전체 뒤1/4, Node22 E1324 전체 뒤2/8이다.

RawCampaignSession은 partial을 유지한다. 이 실제 누락은 해결했으며 과거 대응표 메모의 잔여 문장과 C++ 공개 경계 대조를 계속한다. 이 단위가 자연 플레이 전체의 동일성을 뜻하지 않는다.
