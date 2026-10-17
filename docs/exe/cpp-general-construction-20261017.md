# 일반 서버 건설 확정과 같은 Session의 배치 통지

작업일2026-10-17, 단위132, 작업 PC `HJOW-Athlon`. 마지막 정밀 디컴파일도 `HJOW-Athlon / 2026-10-16`이며 같은 정밀 소스와 보호 PE를 읽었다. 새 디컴파일·원본/클론 게임·GUI 실행은 없다. 기준 커밋은 `cbe9585`다.

## 실제 연결

`RawCampaignConstructionBindings::Confirm`과 `Session.ConfirmConstruction`은 원본444590의 기준점 검사→서버 SID 확보→실제0x3f 직렬화/송신→441b0 직접 통지→기록기 순서를 사용한다. 이전 LocalGeyser 전용 noIsland/Player0 입구는 유지한다. 일반 API의 호출자는 원본 MayPlace를 먼저 통과해야 한다. 현재 검사는 이 계층의 명시 요청 입력을 사용하며 실제 UI/MayPlace 선행 흐름의 검증은 남았다.

같은 실제 PlayerPlacementAnchor, 공장 목록, 현재 준비된 내용물 조건과 Graph를 읽는다. 통지의 다른 플레이어/현재 moveLocalBuilder 조건에서만 기존 Builder 전체 Move를 호출하며, 로컬 플레이어의 Builder를 임의로 다시 만들지 않는다. 사제 조회/기존 경로 첫 contained 삭제/이동 불가/실제 발자국 접근점과54 Start를 연결한다. 441b0의489080 호출은 replaceStart0·helper=대상 SID·나머지0이며, 실패 뒤 현재 경로를 성공으로 대신하지 않는다. 자동 건설은 단위131의 실제30/17 시작에 연결한다. 공개 Builder와 통지 Builder는 같은 조립 함수를 사용한다.

## 비용 부족 Player 필드

기존 값형 fixture의 State 배열은 보존한다. 실제 Session은 PATCH9×b4 Player 원문을 Place에 연결해48f7e0/48f7b0이 사용하는 **+54의0x400 표시**를 직접 읽고 쓰며 다른 비트는 보존한다. 이름은 같은 Player의+c부터subscriber+2c 직전까지의32바이트에서 안내를 선택하는 시점에 읽는다. 실제 PlayerIdentity/Participation 기록 배치와 대조해31자 이름도 검사하며 NUL 누락은 범위 밖 읽기 대신 오류다. 원문을 상태 사본에 복사했다가 덮어쓰지 않는다. +54의 다른 용도를 이유로 별도 플래그 저장소를 만들지 않았다.

SP/최대 HP/소유자/Pop/Clear는 기존 실제 몸체다. 비용 문구·다른 Player 안내·SP/취소 송신, 선택적 기록기 등의 미공급 경계는 선택 시 명시 오류를 유지한다. 서버/권한/PlayerKnown은 요청 때 같은 현재 값을 공급한다.

## 검증 및 남은 목표

최종 검증: 전체 **1,531개/실패0/CTest 종료0/231.71초(전체231.74초)**다. 다른 플레이어 검사에서는 실제 Player 기술 부모의 초기 부재를 확인했다. 원본490250/CD407400의 실제 생성 진입을 Session.CreatePlayer로 공개해 명시 초기화 부모가 사용하게 했으며 Confirm에서 자동 생성하지 않는다. 로컬 예측 교체/기존 Builder 도착/건설 완료와 비용 표시의 현재 원문 갱신은 첫 집중 검사에서 통과했다. 기존 Fort에 이미 있던75 객체는 신규 생성과 구별해 전후 목록을 대조한다. 로그 접두사는`extracted/goal-construction-confirm-20261017-`다. 원본/기존 fixture는 변경하지 않는다.

전체 DWORD 메뉴→ContentForm6 Take/위치/수집/그리기/내려놓기→로컬 예측/상위 비용 차감→실제 요청 수신→일반 Confirm, 공장의 실제 준비 부품 생산, 경제/AI와 자연 플레이·승패·저장·다음 미션은 여전히 확인/연결해야 한다. 일반 서버 확정 조립을 전체 UI 플레이 완료로 취급하지 않는다. 전체화면 DirectDraw 수정은 제외한다.

기존 CD 대응은 Confirm4d0f10/Notice4d1d00/Place4d0ca0/Clear4cff80/Builder4d1120다. 비용 부족442b50과48f7e0/48f7b0에는 기존 CD 대응이 없다. 새 Session 조립은 PATCH10.78 대상이며 새 CD 검증을 의미하지 않는다.

새 집중4개/기존 건설 Session6개/Place6개(세 PE 관찰 재생 포함)가 통과했다. 최종 빌드5 종료0·경고0·오류0다. 근거33종 중 첫32종 통과 뒤 Builder의 함수 목록 SHA 불일치를 조사했다. 현재 CRLF 작업 사본을 LF로 바꾸면 실제 HEAD 바이트와 기존 기대SHA가 완전히 일치했다. 해당 한 파일만 동일 HEAD LF로 복구하고 eol=lf를 고정한 뒤 기존 Builder1,110행 감사도 통과했다. 기대 SHA/관측 fixture/함수 목록 내용은 변경하지 않았다. Place3,682/Clear1,020/Confirm1,300/Notice1,611행도 기존 값 그대로 감사했다. 현재 HEAD에 맞던23개 보고서의64핀만 이력과 갱신하고 과거44핀은 보존했다.

인수인계를 완료한 뒤 `20261017-32 CP 일반 서버 건설 확정과 배치 통지 및 실제 Player 비용 필드 연결`로 로컬 커밋하며 push하지 않는다. 원본/기존 fixture/AGENTS/DN은 변경하지 않는다.
