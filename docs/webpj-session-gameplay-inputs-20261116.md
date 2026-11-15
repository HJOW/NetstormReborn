# Session 선행 GameAux 공용 입력 (WB04-E572)

`225553df`의 `RawCampaignSession.cpp` 생성자와 `o/RawGameAuxState.h`를 대조해 공용 상태의
빠진 입력14종을 찾았다. 건설/운반/미사일의 현재 scalar13칸과 미션 설정7DWORD 배열이다.
Session 생성자는 같은 Client 주소를 참조하므로 지역 기본값이나 사본으로 채우지 않고 먼저
기존 GameAuxState의 공개 API를 보완했다. 전체 생성자·Load는 **아직 미구현**이며 해당 체크는
열어 둔다. 이 단위는 그 구성에 필요한 장기 데이터 준비다. cpppj·보호 원본·공통 인계·dotnetpj는
수정하지 않았다. 원본/클론 게임·브라우저·오디오 장치는 실행하지 않았다.

## 공용 입력과 범위

Session 생성자의 실제 현재 입력 연결(1009~1104 부근)을 기준으로 다음 scalar13칸을 추가했다.

| 현재 필드 | 원문 주소 | 폭/초기값 |
| --- | --- | --- |
| ConstructionProcessType | 005410e8 | DWORD30 |
| ConstructionBlocked | 00558e74 | DWORD0 |
| ConstructionDisplaceGenus | 005325b4 | DWORD10444200(hex) |
| EmptyCarrierType | 00541344 | DWORD187 |
| EmptyGeyserType | 005411a8 | DWORD84 |
| FirstLocalGeyserQuantity | 00540bd4 | signed DWORD3 |
| LastLocalGeyserQuantity | 00540bd8 | signed DWORD4 |
| IslandDropBlocked | 0054db84 | DWORD0 |
| IslandStumpType | 005411d8 | DWORD96 |
| QuickBuild | 0055a49c | DWORD0 |
| QuickVortex | 0055a498 | DWORD0 |
| RainCannonType | 0054125c | DWORD129 |
| ThunderCannonType | 00541264 | DWORD131 |

`missionSettings()`는 실제 같은 Uint32Array7칸을 반환한다. 초기값은 `[1,0,0,0,0,1,8]`이며
주소는 `005326cc/0055a488/0055a48c/0055a490/0055a494/005326d0/005326d4`다. 원문에서는
비연속 전역이고 배열은 cpppj의 같은 고정 주소 owner 표현이다. 원문 전체가28바이트 연속 구조라는
주장을 하지 않는다. 기존 ClientCampaignTransport의 고정32h 계약도 같은 메서드 형태를 사용한다.

일반 reset·ClientGameplayGlobals의 Acquire/AcquirePrepared·반납으로 새13칸이나 배열을
초기화하지 않는다. 새 Client 인스턴스만 PE/BSS 초기값을 가진 별도 물리를 만든다. 정수 폭과
signed 요청 수를 보존하고 현재 reader는 callback 다음 호출에서 바뀐 값을 읽는다.

자료 작성 중 TYPE 역할 이름과 파일 적재 이름을 직접 대조해 `emptyGeyser`84와 `playerBanner`96을
명시했다. 원문의 IslandStump 역할96이 파일 이름도 islandStump라는 뜻은 아니다. EmptyCarrier187은
파일 적재 이름표 밖의 추가 슬롯이므로 존재하지 않는 이름을 붙이지 않는다. 이 교정은 첫 검사 전
정적 자료 조회에서 이루어졌으며 초기값/역할 번호를 바꾸지 않았다.

## 근거와 검증

- 기준선 이후 대상 CPP 소스 변경0이다. GameAuxState 행의 기존③남은 칸 판정과 base는 유지하며
  이번13칸/미션 배열만 소비자 착수에 맞춰 보완했다. 나머지 미이식 장기 칸/Native 결선은 잔여다.
- 기존 보호 PE 대조를 scalar71칸·장식4칸으로 확대했다. 별도 미션 설정7DWORD도 각각 실제
  주소에서 읽어 대조한다. TYPE 이름 대조는40칸이며 파일 이름표 밖187을 억지로 포함하지 않는다.
  원본SHA-256은 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`이다.
  현재 환경에서는 모든 PE 파일 대조를 수행했다. 원본 없는 환경의 기존 선택 자료 정책은 유지한다.
- `raw-mission-settings-packet.test.mjs`의 원본164행 재생을 임시7칸 배열에서 실제 GameAuxState가
  소유하는 같은7칸으로 바꾸었다. 패킷32바이트·현재값·getter/setter/송신 순서·호출 중 변경이
  기존 원본 관측과 그대로 일치한다. fixture는 복사하거나 재생성하지 않았다.
- 신규4검사는 미션 설정의 비연속 PE 대조, 실제 패킷 왕복 후 PriestDamage/GunProcess/Canon
  Placement의 현재 무적/동맹 사격/강제 배치 reader, 짧은 수신의 앞선 저장과 다음 송신,
  실제 일반/준비 대여·reset 뒤 동일 물리/현재 건설 TYPE·Quick/총 TYPE 및 독립 인스턴스를 확인한다.
- 첫 집중 검사43건은 모두 통과했다. GameAux·미션 설정 패킷·ClientGameplayGlobals·건설
  프로세스 회귀를 포함한다. GameAux 줄100%/분기97.96%/함수100%다.
  로그는 `extracted/wb-e572-focus.log`(Git 제외)다. 시험이 현재 reader를 연결한 것으로 전체
  Session 설치를 대신하지 않는다. 실제 Session의 동일 연결은 생성자 이식 후 다시 대조한다.

전체 Node v22.23.3·v26.13.0 검사는 각각 **5,418건 통과·실패/취소/건너뜀0**이다.
로그는 `extracted/wb-e572-node22.log`·`extracted/wb-e572-node26.log`다. 대응표1,160행 오류0,
port839·partial33·todo132·제외156, session/oracle0을 확인했다
(`extracted/wb-e572-port-map-final.log`, 모두 Git 제외). 원본 관측과 PE의 기존 manifest SHA·
cpppjBasis를 보존하고 변경 모듈/소비자 추적만 갱신했다. manifest 항목 수802도 유지한다.
미션 설정 fixture165행(관측164)의 정규화SHA-256은
`dbd42f654348c6d4d5de37d99e421f0b2b971794061cc7a4631c7f4939616ae4`, 실제 CPP 근거는
`ae0e58b3a`다. 원본/기준선/대응표 수치와 현재 진행 상태를 대조한 뒤 완료로 기록했다.

## 다음 시작점

전체 Session 생성자는 아직 없다. 4-D D의 RawCampaignSession1에서 위 장기 owner와 현재
Player/Core·자산·RawSurfaceWorld·SP를 같은 생성/반납 순서로 구성한 뒤, 생성자에서 확인한43종
생성 대상과 내부 Connect 함수들을 실제 웹 계약에 맞춰 연결한다. 그 뒤 저장 Fort의 실제 Load와 L2를 대조한다.
`extracted/wb-e572-session-baseline.cpp`·`wb-e572-constructor.txt`·`wb-e572-constructor-audit.json`은
읽기 전용 기준선/준비 색인(Git 제외)이며 실제 Session 구현이나 완료 증거가 아니다.
