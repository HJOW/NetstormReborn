# Session 운반 후보·대상·회피·재탐색 (WB04-E642)

기준선 `225553df`의 `RawCampaignSession.cpp` **3845..3888, 44줄**을
기존 실제 owner에 설치했다. 이 함수 표가 호출하는 공통 종류별 경로 시작
**3012..3019, 8줄**도 4D-2d에서 앞당겨 같은 장치에 옮겼다(합계52줄).
Session cpp/h와 관련 RawCarrierChoice/Escape/Target/PathRetarget의 기준선
뒤 변경은 없다. 전체 ConnectMotion·Load 완료로 세지 않는다.
검사 중 외부 병합 `3e5369e2a`가 들어왔으나 `a4c51b7c3` 대비 변경은
dotnetpj 범위뿐이었다. cpppj·webpj·AGENTS.md·공통 인계 차이0을 확인했으며
현재 검증 범위나 기준선은 바뀌지 않았다.

BoardTarget은 같은 Storage의 실패 좌표 두 칸을 직접 빌리며 설치/반납
때 초기화하지 않는다. 후보/대상/회피/재탐색은 같은 Pool·현재 TYPE·
Player9 기록·장기 Outpost/간헐천 목록·현재 시계·공유 난수·낙하 몸체·
기준점/내용물 조회를 사용한다. checking은 원문의 RawCheckingFlag 값형
복사처럼 복사 순간의 포인터/지역 fallback 선택을 보존한다.

귀환 후보·간헐천 후보와 현재 제어 여부를 기존 Motion 함수 표에 연결하고,
회피 함수는 같은 services.motion.carrier.escape에도 게시한다.
RawPathRetarget은 현재 장기 너겟 번호를 호출 때 읽는다. StartMotion은
실제 사제/운반체/건설 경로의 기존 종류별 시작만 선택한다. 아직 Motion
본체를 설치하지 않았으므로 현재 경로 조회/새 시작은 명시적으로 거부한다.
다음 단위가 같은 실제 본체를 채우며 성공 경로나 임시 좌표로 우회하지 않는다.

신규4검사는 실제 생성자/건설/도착/기준점 뒤 다음을 검사한다.

- 선행·반복·반납 관문, 같은 Pool, 장기 실패점/물리/난수 보존.
- 실제 Player brain 변경과 후보 없음, 현재 보호 TYPE 속도, 제어 중 귀환 조기.
- 공용 난수 소비와 현재 낙하 좌표 제한, 미설치 경로 조회/시작 거부.
- 실제 내용물과 죽은 귀환 대상의 재탐색, 장기 너겟 번호 변경 직후의 조회 차이.

초안4건과 실제 owner 공개 API 결선 뒤4건 모두 첫 실행에 통과했다.
관련 집중 검사는 **17/17 통과**했다. 조립 검사에서 경로·후보 조회 결과를
주입하거나 별도 월드를 만들지 않았다. 원본 하위 소비 검사는 기존
carrier-choice721행·carrier-escape1807행·carrier-target2245행·
path-retarget4801행을 그대로 사용하며 manifest 모듈/범위만 확장한다.
신규4건을 해당 fixture의 직접 소비자로 등록하지 않는다.

cpppj `CampaignSession_PathRetargetRunsRealCarrierFrames`·
`CampaignSession_CarrierEscapeUsesRealPathsAndDepthCap`·
`CampaignSession_CarrierEscapeSurfaceFallbackUsesRealBfs`의 전체 프레임/실제
송신 경로는 아직 대조하지 않았다. 이들은 Motion 본체와3930..3981의
경로 송신 설치, 실제 Load가 선행이며 4D-2h·4D-10 및6단계의 저장 미션
경로 재대조에서 확인한다. 이번4건은 현재 결선/수명 경계만 검증한다.

최종 Node v22.23.3·v26.13.0 전체는 각각 **6,051/6,051 통과**,
실패·취소·건너뜀0이다(`--test-concurrency=4`). v24는 지시대로 생략하고
호환을 가정한다. 대응표1,160행 오류0·manifest823항목과 공백 오류0을
확인했다. 로그는 Git 제외 `extracted/webpj-e642-node22.log`·
`webpj-e642-node26.log`에 보관했다.

원본 fixture SHA-256은 carrier-choice `a13058b4…2af0`,
carrier-escape `b2fc2788…8c9f`, carrier-target `7ec562c2…3e45`,
path-retarget `89b1fc34…b122`이며 파일 변경은 없다.
실제 loaded=false이며 저장 미션/전체 경로·내부 Login/웹 Client는 미연결이다.
게임/브라우저나 사제 포획·희생·특정 적 공격·공중3종 전용 행동은 실행하지 않았다.
다음은 **4D-2b 3889..3929**의 Mining/Transfer·메타데이터·실제 Motion
본체 결선이다. 4D-2d의 AI 행동은 아직 미완료이고 공통 시작8줄만 옮겼다.
