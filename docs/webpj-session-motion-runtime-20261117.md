# Session 채취·전달·이동 본체 (WB04-E643)

기준선 `225553df`의 `RawCampaignSession.cpp` **3889..3929, 41줄**을
기존 실제 owner에 설치했다. Session cpp/h·RawCarrierMining/Transfer·
RawGeyserMined의 기준선 뒤 변경은 없다. E640~E643으로 4D-2b의
3782..3929 앞부분148줄을 마쳤다. 전체 ConnectMotion·Load는 아직 아니다.

같은6BYTE 채취 헤더/방송 표/로컬 수신을 실제 간헐천 처리기에 연결하고,
기존 World·Pool·현재 너겟 TYPE/cost·난수·위치 음향을 사용한다.
너겟 전달은 같은 Form을 분리/부착하고 실제491 미리보기를 예약한다.
이미 설치된 크랩 회수/정지·Dude 행동·운반 후보와 현재 TYPE의 속도/
회전 속도/기본 프레임/stepSound를 묶은 뒤 RawCampaignMotionBindings를
마지막에 생성한다. 함수 표는 같은 owner의 순환 참조를 닫으며 별도
월드나 임시 성공 경로를 만들지 않는다.

보호 TYPE의 평탄화된 defaultFrame을 사용한다. 사제/Dude 속도는
현재 dudeSpeeds 표를 읽고, 다른 종류는 필수 speed를 읽는다.
의식·panic과 경로 송신 설치는 뒷부분3930..3981에서 이어가므로
미설치 panic/stepSound 경계는 선택 시 명시 오류로 유지한다.

초안4건과 정식 owner 결선 뒤 신규5건이 첫 실행에 통과했다.

- 선행·반복·반납, 같은 실제 Pool/Host/시계, 설치 시 물리/난수 보존.
- 현재 사제 속도와 보호 TYPE 회전 속도·stepSound0·미설치 panic 경계.
- 실제 채취 통지→로컬 수신→현재 cost 차감과 같은 위치 소리/난수.
- 실제 내용물 Form 전달·491 예약·SP 보존.
- 실제 Motion의 자원 후보→4f 요청→수신→같은 내용물/입력 큐·SP 보존.

마지막 검사는 E641의 최소 Motion 계약을 이번 실제 MotionBindings로
대체해 도착 getter의 객체 반환→pair 배열 변환도 다시 대조한다.
내부 zlocal 전송은 아직 없으며 외부 송신 경계를 기록한 뒤 서버 수신을
직접 호출한다. 전체 내부 전송의 완료 증거로 세지 않는다.

원본 하위 소비는 기존 carrier-mining864행·carrier-transfer1110행·
geysermined의10.78 1156행 및 RawCampaignMotionBindings 검사를 유지한다.
신규5건은 원본 fixture의 직접 소비자로 등록하지 않고 manifest 모듈/
범위만 확장한다. 관련 집중 검사는 **42/42 통과**했다.
원본 SHA-256은 carrier-mining `b2a0bcaf…9c77`,
carrier-transfer `11ea45cd…9fd9`, geysermined `29abf720…d5aa`이며
파일 변경은 없다.

최종 Node v22.23.3·v26.13.0 전체는 각각 **6,056/6,056 통과**,
실패·취소·건너뜀0이다(`--test-concurrency=4`). v24는 지시대로 생략하고
호환을 가정한다. 대응표1,160행 오류0·manifest823항목과 공백 오류0을
확인했다. 로그는 Git 제외 `extracted/webpj-e643-node22.log`·
`webpj-e643-node26.log`에 보관했다.

cpppj `CampaignSession_CarrierMiningNaturalArrivalUsesSameWorld`·
`CampaignSession_PathRetargetRunsRealCarrierFrames`와 실제 회피/깊이 검사는
경로 송신·실제 Load가 선행이다. 4D-2h·4D-10 및6단계의 저장 미션 경로
재대조에 남긴다. 게임/브라우저·실제 사제 포획/희생 재현·특정 적 공격·공중3종
전용 행동은 실행하지 않았고 실제 loaded=false를 유지한다.

다음은 **4D-2c 3930..3981**의 Panic/Ritual·Salvage·CarrierMove·
AI 이동 설치·경로 송신 합성·BridgeCarriers다. AI 선행이 막히면
4D-2d와 함께 현재 계획 안에서 필요한 조각만 앞당긴다.
