# AI 배치·생산·다리 탐색/건설 선택 결선 (WB04-E625)

기준선 `225553df`의 ConnectAi2880..2902와 현재 참여/생존 조회2946·2950을
같은 실제 owner에 연결했다. Session.cpp와 해당 일곱 AI 하위 cpp는 기준선 뒤
변경0이다. 생성자 안의 ConnectAi 구성은 여기까지 연결됐으며, 외곽 Session
생성/공개·Load와 이후 Motion 뒤 ConnectAiMovement/Runtime·실제 행동은 잔여다.

## 구현

- 같은 기술 부모/생산 조각·ContainedCreate·현재 TYPE/지갑과 실제 Regular
  확보 객체를 배치에 연결했다. 같은 Host에 부착한 뒤 원문 payload/현재 시각을
  기록한다. 서버 요청/전체 MayPlace는 같은 건설 준비 부모를 쓰고, 아직 없는
  로컬 Cursor는 호출 위치에서 미설치 오류를 유지한다.
- 콜렉터·현재 프레임/해시/spot의 다리 후보·같은 난수의 다리 탐색, 실제 시설
  생산/업그레이드 요청, 에너지 부족 좌표와 현재 역할의 건설 선택을 연결했다.
  가상 level은 원문 두 VT만 허용하고 실제 VirtualCount에 넘긴다.
- 로그는 세 종류의 장기 진단 DWORD를 호출마다 다시 읽는다. 에너지 장치는
  같은 EnergySupply의 반환 DWORD/수정 문자열을 원문 box에 전달한다.
  각 큰 장치/실제 배치는 호출자가 나중에 선택하며 조립 시 실행하지 않는다.

## 조립에서 드러난 교정

최초 신규5건은3통과/2실패였다. 다리 탐색 모양이 비었고 진단용 조회에서도
GameAux.bridgeType가 없음을 확인했다. C++의 실제 입력은 GameAux가 아니라
`ContainedCreateState.bridgeType` 값형 필드(초기82)다. 이 칸을 owner에 선언해
E624 전략 준비와 현재 다리 후보/탐색이 같은 칸을 읽게 했다. 실제 Form 생성의
sharedBridgeType는 계속 기존 World links를 읽는다(C++3133, 두 입력을 구별).
실제 부모에 다리 재고 Form6을 만들어 TYPE82를 대조했다.

그 교정 뒤 신규+E624의11건은10통과/다리1실패였다. 기존 RawAiBridgeSearch가
실제 FrameCodeTable 객체를 코드 배열로 직접 해석하고 있었다. 실제 현재
`.codes`를 넘기고 독립 원본 대조의 배열 입력도 그대로 받게 교정했다.
이후 신규5건·E6246건과 기존 일곱 AI 몸체를 포함한 집중81건 모두 통과했다.

실제 같은 Pool/난수·조립 시 본체 할당0, 미부착 Regular의 정체성/부착/반납,
현재 기술/지갑·워크샾/신전 역할 변경, 미연결 전체 MayPlace/로컬 배치/가상
level 관문, 실제 보호 FrameCodeTable/패턴의 다리 모양과 난수 불변,
실제 에너지 평가와 현재 진단/본문을 확인했다. 보호 다리 재고와 모양은
검사 입력이며 자연 플레이나 실제 미션 시작 성공을 뜻하지 않는다.

## 원본 자료와 잔여

기존 실제 PE 다리 후보210장면/220반환·시작점399행·모양208행·점유416행·
끝점640행·검색72장면/288호출을 기존 소비 검사로 재대조했다. 나머지 생산/
콜렉터·부족 좌표·건설 선택 등은 기존 cpppj 계약 단위이며 전용 원본 관측을
만들지 않았다. manifest806항목/신규fixture0이다. 새 결선 검사는 직접
fixture 소비자로 등록하지 않고 실제 모듈/근거/범위만 기록했다.
메타데이터 집중10건과 `port-map.mjs`·`node --check`·`git diff --check`
오류0이다. Node v22.23.3·v26.13.0 전체는 각각5,966건 통과,
실패·취소·건너뜀0이다. 로그는 `extracted/webpj-e625-node22.log`·
`webpj-e625-node26.log`에 보존했다. RawCampaignSession partial·앱 session0을
유지하며 실제 플레이나 전체 Session 완료로 세지 않는다.

원본/cpppj 게임 창·자연 플레이/실제 포획·희생/승리와 공중3종 새 전용
생성/생산/행동·Native 장기 설치는 하지 않았다. 현재 검사는 기존 AI 단위
검사의 재생이며 새로운 공중3종 전용 검사를 추가하지 않았다.

다음은 외곽 생성자4318..4330의 같은 Session callback/공개 수명과 Load다.
공개 전에 RawSurfaceWorld347..365의 실제 Runtime pin 전달과 공통
PostPop/contained Unpop의 호출 범위 소유 보유를 먼저 연결해야 한다.
Motion 뒤 ConnectAiMovement/Runtime 및 실제 플레이/앱 수명·L2는 계속 잔여다.
