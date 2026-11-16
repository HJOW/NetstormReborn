# Session의 현재 typed Runtime 훅 값 (WB04-E595)

`225553df`의 `RawCampaignSession.cpp`1611..1649와
`RawCampaignRitual.inc`2..5, `RawCampaignDevastation.inc`34..48의
준비/수신 앞단을 같은 owner에 연결했다. 해당 cpp/h/inc의 기준선 뒤
변경은0이다. `InstallContents`는1650, 실제 `World.BindRuntime`은1825다.
이번 단위는 훅 값을 준비하며 Runtime을 게시하거나 Session을 만들지 않는다.

## 같은 현재 모듈·원문 순서

- Vortex/거주 구역/공장/피해 함수는 E590에서 보관한 실제 함수 사본이다.
  Gun의 표적 중심 뒤에도 같은 물리 슬롯을 읽고 현재 방향/owner/프레임의
  signed DWORD 문맥을 실제 MayTarget에 넘긴다. DamageHooks의 실제 피해,
  현재 입자 AddBeam(shortZap.wav)/AddDissolve, 사제 이동 종류와 Dude의
  진입 기하/값형 목적지를 연결했다.
- 뒤에서 생성할 Dais/제단/의식·내용물/포획 안내·플레어·주문 대상/수신기는
  원문처럼 호출 때 현재 포인터를 조회한다. 미설치 상태가 선택되면 오류다.
  다른 몸체나 빈 성공으로 대체하지 않는다. copied 함수와 현재 포인터
  조회의 차이도 유지했다.
- 의식은 같은 두 포인터 관문 뒤 Rune의 Player+50 BYTE9개 → Dais의
  방향별 동맹81 signed DWORD 순서로 준비한다. 후단의 실제 module 호출은
  그 다음이며, 의식은 강한 사제 검색 → Ritual(sid,priest,1) 순서다.
  애니메이션 제거는 별도 현재 callable을 먼저 보관해 호출한다.
- Player 연결은 현재1..8의 flag bit0이다. 대상 송신은 int32 범위0..8의
  현재 서비스 슬롯을 선택하고 NULL은 자연 생략한다. 최초 battlePeers와
  deletion 서비스 표는 C++ 값 인수처럼 복사하며 이후 현재 서비스 슬롯의
  교체는 다음 호출에 반영한다.
- 삭제 Transmit은 E594의 같은 World 공용+58로 보낸다. 다른 삭제/선택
  효과의 함수는 생성 때 값으로 보관한다. Form/선택 함수가 비면 실제
  FillMissing 위치1629/1630을 가진 선택된 오류를 설치한다. 앞 생성자에서
  설치한 진짜 기본 Form writer가 있으면 그것을 그대로 보존한다.
- 의존 통지는 현재 TYPE110/VT50eb98 두 값이 맞으면 World.Destroy(flags0),
  그 외는 실제 Host.NotifyDependency다. Native 장기 가상 분배는 잔여다.
- typed 주문 준비는 같은 현재 boss/로컬 owner와 **값 복사** 동맹81개를
  공급한다. C++ BombBlastState의 std::array를 Player 배열 별칭으로 바꾸지
  않았다. 검색은 준비 뒤 같은 예약/용량/count0을 넘기고 현재 count를
  uint32로 반환한다. notice의 count는 signed32로 해석한다.
- 효과 수신은 부모 WORD 최소6BYTE → free/dead 또는 같은16VT 자격 → 현재
  상태 준비 → 같은 후속 Receive 순서다. sender/connection은 읽지 않는다.
  16VT 판정은 기존 파편의 같은 Source 목록과 공유한다.

## 근거·첫 실패·검증

새 Runtime 부모의 전용 원본 실행 관측은 없다. 보호 자산/실제 Pool·기존
모듈과 명시 소비자 계약으로 대조했다. 후속 의식/주문 소비자 검사는
인자·준비 순서·선택된 오류의 계약이며 실제 의식/주문 시전 증거가 아니다.

기존 연결42건은 처음부터 통과했다. 신규14건의 첫 실행은10건 통과/
4건 실패였다. 기본 Form writer가 앞에서 이미 설치됐는데 누락으로 가정한
1건과, 검사 입력을 slot 사본에 쓴3건이었다. 실제 쓰기 대여로 입력을
고쳤고, 표적 중심 사이 효과를 대조하면서 제품도 C++ span처럼 같은
물리 슬롯을 보유하도록 교정했다. 진단/미설치 관문은 완화하지 않았다.
최종 신규14건 및 Runtime/자산/기반/입자·공용 송신·Dais/주문 검색의
**관련75건이 모두 통과**했다.

Node v22.23.3·v26.13.0 최종 전체는 각각 **5,701건 통과, 실패·취소·건너뜀0**이다. 대응표1160행/오류0도 확인했다. 새 fixture/원본
실행은 없으며 manifest에서 직접 연결된 기존 항목은0개라806항목을
그대로 유지했다. Source 계약을 원본 관측으로 새로 등재하지 않았다.
JS130605행·검사765개다. RawCampaignRitual은 PrepareRitual만 옮겨
**todo→partial**로 바꾸었다. 전체 port845/partial37/todo125·잔여162/
34373 C++행이며 4-D36/51은 같다. 7단계의 나머지 의식 생성/요청은 남는다.
Node24는 사용자 지시로 생략하고22/26 통과 시 호환을 가정한다.

로그는 `extracted/wb-e595-initial.log`, `wb-e595-new.log`,
`wb-e595-focused.log`, `wb-e595-node22.log`, `wb-e595-node26.log`,
`wb-e595-port-map.log`다.

## 다음 시작점·남은 경계

생성자1650 `InstallContents(worldHooks.assets)` → ConnectFlare/
Construction/Damageable/IceTower/CarrierSteal/BombPostPop/AcidZap/
AcidFence·이동/건물 → 전체 모듈 생성으로 이어간다. 실제 World.BindRuntime
1825 및 상위 Runtime 공개·Load·프레임/L2/L3는 여전히 남았다. Native
장기 Runtime/송신/기본 Form writer·수신/동기화는 실제 Client 결선 때
옮긴다. 현재 모듈을 전체 Session 완료로 세지 않는다.

cpppj/보호 파일 수정·디컴파일·게임/브라우저·자연 완주·실제 포획/희생·
특정 적 공격/승리·공중3종 전용 관측은 하지 않았다. 실제 미션은 아직
플레이할 수 없다.
