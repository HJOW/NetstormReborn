# Session의 Dude·해방·파생 삭제 부모 (WB04-E604)

`225553df` 생성자1653/ConnectDamageable2626..2705와 typed
PrepareDamageablePost를 같은 owner에 연결했다. Session/직접 소비
모듈의 기준선 뒤 변경은0이다. Native 장기 몸체의 대여는 실제 Client
결선에서 남겼고, 공중3종은 생성자/기존 하위만 정적으로 연결했다.

## 같은 현재 물리·효과

- Dude Spawn/PostPop/Task/Behavior/Regular/Drop을 같은 Pool/World/
  Frame/Host와 현재 population·공급 목록에 구성했다. PostPop은 같은
  공통 처리기에 등록하고 반납 때 해제한다. 생성 한도/차단은 실제 기존
  RawDudeSpawn이 실행한다. 순환 인덱스는 같은 signed owner를 빌리고
  checking은 같은 Client DWORD를 비영으로 읽는다.
- 같은 사제 Spawn·종속 Finder·현재 소유권/World Pop을 해방 몸체에
  연결했다. 허용 genus는 이미 E575가 연결한 실제 FortKeepGenus다.
  공통 Pre/Post는 현재 World.CommonDeletion을 조회한다. Pre에서
  release 직전 전투 모드를, Post 직전 전투/population을 준비한다.
- 붕괴/폭발은 같은 입자·HP 충돌 효과를 쓴다. 폭발 시각/로딩 억제와
  Kernel30001 관문 뒤 현재 좌표/TYPE 발자국을 읽어 float로 저장하며,
  시각 효과가 억제돼도 파편 호출은 원문처럼 계속한다. 소리는 현재
  SoundList/서비스의 전역6인자 또는 위치5인자를 유지한다.
- 외부 전/후처리 효과는 값으로 복사하고 Source가 덮는 내부 부모만
  교체했다. 같은 실제 Host에 Regular를 만든 뒤 초기 count DWORD를
  쓰며, 기존 다리 낙하 예약을 조회한다. 뒤 설치될 건물 효과는 호출 때
  선택하고 현재 시각을 준비한다.
- 신전 Pre는 같은 Player/신전·거주·추가 목록/로딩 깊이를 쓴다. 섬
  비움의 현재 생산 갱신과 독립 refresh의 값 사본을 구별했다.
  간헐천 수명은 실제 자산 기본 프레임/같은 목록·현재 편집 관문·AI
  등록/제거와 현재 Damageable 전/후처리를 사용한다. AI 포인터가0인
  Player 목록은 아직 미설치한 AI World를 요구하지 않는다.

## 최초 실패·검증·남은 경계

신규12건 첫 실행은9통과/3실패다. 두 시험 입력은 signed checking의
상위 비트를 unsigned 기대값에 직접 비교한 것과 읽기 사본 Pool.slot에
좌표를 쓴 것이었다. 비트 비교/실제 physical span으로 고쳤다.
제품 결선의 한 실패는 C++ definition.specialFrames.defaultFrame을
JS 자산의 평탄화된 definition.defaultFrame으로 옮기지 않은 것이다.
현재 자산 필드로 고친 뒤 신규12건이 모두 통과했다. 이어 두 표시
경계가 같은 물리 span을 빌리도록 맞추고 콜백의 현재 슬롯 쓰기가
보존되는13번째 검사도 통과했다. 원본 기계어
전용 관측을 새로 수집하지 않았고 Source 계약을 원본 fixture로 세지
않는다. manifest 직접 항목0개/총806항목은 같다.

`CampaignSessionTests.cpp`의 CampaignDude_ActualArrivalUsesPopulationLifetime,
CampaignDude_ActualRegularFallAndPopulation30·ActualPathNoticeSpeedAndArrival과
실제 건물 붕괴 검사는
먼저 session.Load()를 요구한다. 이번 검사는 같은 실제 구성/Host와
선택된 효과 계약이며 이 전체 Session 검사의 통과로 대신하지 않는다.
후속 Load/World 게시 뒤 해당 실제 도착/낙하·붕괴 검사를 다시 대조한다.
자연 대피·승패/실제 사제 포획/희생 검사는 아니다.

신규13건을 포함한 관련37건 통과. Node v22.23.3·v26.13.0
전체는 각각5812건 통과, 실패/취소/건너뜀0·종료0이다.
Node24는 사용자 지시로 생략하고 호환을 가정한다. 대응표1160행/
오류0, port846/partial39/todo122·session0, 잔여161단위/34340줄,
JS131483줄/검사774파일과 변경6파일의 UTF-8을 확인했다. 다음은 생성자1654
ConnectIceTower,1655 운반 포획·주문/펜스·이동/건물과 나머지 모듈이다.
World.BindRuntime1825·외곽 공개·Load/Login·프레임·L2/L3는 남았고
실제 미션은 아직 플레이할 수 없다. 보호 파일/cpppj 수정·디컴파일·
게임/브라우저 실행·공중3종 전용 실행 검사는 하지 않았다.

로그: `extracted/wb-e604-initial.log`, `wb-e604-focused.log`.

추가 집중 로그: `extracted/wb-e604-focused-final.log`.

최종 로그: `extracted/wb-e604-node22.log`, `wb-e604-node26.log`,
`wb-e604-port-map.log`.
