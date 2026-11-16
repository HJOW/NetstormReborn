# Session의 GemReward·Buried 현재 상태 결선 (WB04-E613)

기준선 `225553df` 생성자1671 `ConnectGemReward`2020..2032와
1672..1679의 Buried 후처리를 연결했다. Session.cpp·RawGemReward.cpp/.h·
RawBuriedPostPop.cpp/.h의 기준선 뒤 변경은0이다. 독립 typed의 지역
GemRewardState(188/112)와 빈 건설 제외 목록을 실제 owner가 소유한다.
Native 장기 보상/건설 owner 대여는 실제 Client 조립에 남았다.

같은 현재 TYPE·제외 목록·게임 난수를 기존 보상 몸체에 연결했다. Deck
할당 입력과 벽시계는 원문 std::function 대입처럼 결선 시 함수 값을
복사하며, 누락된 값을 나중 서비스로 우회하지 않는다. mismatch는 같은
현재 Player 로그 장치를 사용하고 signed 두 값·줄바꿈을 보존한다.
보상 공개의 가상 생성·현재 좌표 Pop·원 오벨리스크 삭제도 같은 World다.

Buried는 같은 현재 boss/로딩 깊이/Bridge TYPE와 공통 장부를 빌린다.
원문처럼 reserve는 실제 확보를 만들지 않고 addRegular가 실제 Host에
event0/0초·event1/0.01초를 부착한다. 공통 ValidateBase/PostPopBase와
표면 통지에 같은 현재 부모를 연결하고, 공통 후처리의 buried 몸체로 등록했다.
반납은 실제 Host를 비운 뒤 등록과 대여를 닫는다.

신규7건 중 최초6건 통과·1건 실패는 시계를 제거한 시험 입력이 선행
SoundProcessSystem의 필수 효과 검사에서 먼저 거부된 경우다. 실제 소리
초기화를 마친 뒤 GemReward 결선 전에 시계를 제거하도록 입력을 교정했다.
제품 코드는 바꾸지 않았다. 이후 관련17건이 통과했다. 실제 단일 후보
추첨/현재 제외 목록·게임 난수, 함수 값 사본/선택 오류의 앞선 효과,
현재 참조·Host의 두 사건과 실패 전파·공통 부모를 확인했다.

기존 부품 시험으로 원본 fixture를 재생했으며 신규 fixture는0이다.

| fixture | 행 | SHA-256 |
| --- | ---: | --- |
| gem-reward | 1,250 | `4aa9033630b83976e528f354bb0df89e4f4226911b0a3d5e9c2c810c0a94bd09` |
| buried-postpop | 580 | `76e346fc32bfe640f615ced94d4d497198e0abc483f17de8bfbdd9fbe6517bf4` |

Node v22.23.3·v26.13.0 전체는 각각5,891건 통과했고 실패·취소·건너뜀은0이다.
두 실행 exit0·대응표 오류0·UTF-8/구문/diff 검사를 확인했다.
웹 JS132,435줄/검사783파일·manifest806항목이며 port846/partial39/todo122·session0을 유지한다.
전체 명령은 `--test-concurrency=4`를 사용하며 Node24는 기존 지시로 생략한다.

실제 World의 자산 삭제/Regular 분배·게시1825·Load/미션 보상 공개와
Native 장기 대여는 잔여다. 다음은 생성자1680의 worldHooks.assets와
destroy 래퍼·파생 가상 삭제 분배, 이어 Regular 분배와 전체 모듈 생성이다.
보호 원본/cpppj 변경·디컴파일·게임/브라우저 실행·실제 교전/포획/희생·
공중3종 새 전용 생성/생산/행동 검사는 하지 않았다.
검사 로그 접두는 `extracted/wb-e613-`이다.
