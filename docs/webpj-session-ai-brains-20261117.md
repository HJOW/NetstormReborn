# 지역/승패와 AI 본체 부모 결선 (WB04-E623)

기준선 `225553df`의 RawCampaignSession 생성자1871..1880·ConnectAi2777..2834를
같은 owner에 연결했다. 생성 전 난이도가 요구하는 AiAllianceSide도10단계에서
4-D로 앞당겼다. Session.cpp/.h·AiAllianceSide.h는 기준선 뒤 변경0이다.
외곽 Session/Load와 AI 통지·전략·행동의 다음 구간은 미완료다.

## 구현

- 실제 hash0의 살아 있는 WORD뷰와 현재 terrainType로 RegionOwner를 만들고,
  같은 사제/Player9/81방향 동맹·현재 로컬/데모/Tell 차단과 HP 부모를 승패에
  연결했다. UI 함수는 C++처럼 services 사본에만 FillMissing을 채운다.
  사건 계산과 실제 승리·실패는 실행하지 않는다.
- 참여는 실제 현재 모드/Player를 읽는다. 연결 재배치·동맹 요청은 후설치한
  Login 장치를 우선하며 그 전에는 캡처한 외부 함수를 사용한다. brain 수명은
  외부 관찰자에 맡기지 않고 같은 실제 본체 생성자/소멸자에 연결한다.
- 같은 AI memory·Player 기술/ContainedCreate·현재 TYPE/AST로 생성자와
  초기 갱신을 만들었다. 현재 그래프/noRegion·사제/지역·미션/분류 제외 타입과
  실제4워크샾을 공유한다. AiCreate의 색 변경은 실제 프레임 처리 뒤 관찰자에
  알리며, 같은 Player 포인터의 지갑을 HP 보상에 직접 연결한다.
- 난이도는 실제 데모·로컬 마스크와 아직 Player에 기록하지 않은 설정 동맹
  목록을 판정한다. `AiAllianceSide.h` 전체27줄과 cpppj의
  CampaignTestPlay_EnemyClassification6조건 및 물리 경계를 이식했다.
  원본에 없는11차 기능이며 전용 원본 관측을 합성하지 않는다.
- 종료는 원문 소멸자4027 순서처럼 owner1..8의 같은 현재 본체를 Host보다
  먼저 반납한다. State 반납 증빙을 존중하며 개별 실패는 console에 남기고
  다음 owner를 계속 정리한다. Host 뒤에 조립 부모의 대여를 닫는다.

## 검증

신규7건을 포함한 집중35건이 통과했다. 같은 Pool/난수·생성 시 할당0,
현재 hash0/TYPE와 사제 상태, UI 값 복사/미연결 최초 오류, 실제 전송 함수
선택과 실패 접두, 실제18블록 본체/목록·기술 부모/이름·HP 지갑/데모 하한,
입력 불변/생성 전 동맹·적 난이도, 초기 갱신 실패 뒤 본체 반납을 확인했다.
승패 사건 장치는 미션 섹션과 겹치지 않는 명시 검사 키 AssemblyProbe로만
확인했고 Check나 실제 승패를 호출하지 않았다.

최초 신규7건은5통과/2실패였다. 검사 입력의 PlayerKnown0이 실제 기술 조회
진단을 선택했고, 별도 난이도 장면은 현재 미션의 기본 AI 이름 입력이 빠졌다.
제품의 진단/원문 순서를 유지하고 실제 알려진 Player 및 선택된 외부 채널의
기록 장치·명시 언어 입력을 공급했다. 교정 후 집중35건 첫 실행에 통과했다.

기존 원본 fixture인 지역 소유자10.78의816행, AI 본체160행,
초기 갱신133+16행, 승패364행을 기존 검사로 재대조했다. manifest806항목,
신규fixture0이다. 본체/월드의 opaque 핸들을 원본 가상 주소로 주장하지 않는다.
최초 Node26 전체는5,955건 중5,952통과/대응표3실패였다. 메타데이터 갱신 전
전체 검사를 시작해, 검사 파일이 모듈을 읽을 때의 잘못된 `[o-default]` 접두를
보관했다. 현재 대응표는 규칙의 `[rebaseline-port]`로 교정했다. 최초 Node22
전체는5,955건 모두 통과했다. 이어 Node26 재검사에서는 manifest의 소비 검사
등록 오류를 발견해 실행을 중단했다. 새 결선 검사는 fixture 자체를 읽지 않으므로
직접 소비자로 등록하지 않고, 실제 재생 검사와 연결 모듈/근거/범위만 기록했다.
교정 후 대응표/manifest 집중10건 통과다. 이후 확정한 같은 자료로 실행한
Node v22.23.3·v26.13.0 최종 전체는 각각5,955건 통과, 실패·취소·건너뜀0이다.
최종 로그는 `extracted/webpj-e623-node22-verified.log`·`webpj-e623-node26-verified.log`다.
실패/중단된 이전 로그도 보존한다. 대응표·`node --check`·`git diff --check`
오류0이며 RawCampaignSession partial·session0을 유지한다.

자연 플레이·실제 포획/희생/승리·공중3종의 새 전용 생성/생산/행동,
원본/cpppj 게임 창·Native 장기 설치는 실행하지 않았다.
다음은 ConnectAi2835의 실제 객체 통지/World 등록·준비와 전략/행동 조립이다.
외곽 Session 공개/Load·Login·앱 연결과 L2는 계속 잔여다.
