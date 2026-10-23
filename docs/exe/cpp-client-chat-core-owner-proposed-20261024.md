# Client Chatmanager·현재 CoreData 최소 접합 제안

2026-10-24 후속 **단위385 미적용 제안**이다. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. [정적 분석 사본](../../extracted/cpp-chat-core-owner-20261024-proposed/analyze.py)과 [읽기 시점 보고서](../../extracted/cpp-chat-core-owner-20261024-proposed/static-analysis.json)는 PE/현재 소스를 읽었을 뿐 제품 실행이나 관측 결과가 아니다. 공유 파일/CMake/LEFT/git/공용 build/lib/GUI를 변경·실행하지 않았다. 신규 독립 [Chat5몸체](cpp-chat-manager-prefix-20261024.md)와 [Core2몸체](cpp-player-core-data-20261024.md)는 별도 원문 실행/검증 자료다.

현재 수명 전제를 생략하면 안전하지 않아 **자동 적용 가능한 Client/Session patch는 만들지 않았다**. 아래는 실제 연결 위치와 반드시 먼저 채울 경계다. Client 생성자에 포인터0만 추가한 뒤 모든 refresh를 자연 생략하거나 실제 없는 command 등록을 성공으로 처리하는 제안이 아니다.

## 최소 실제 소유자

Client가 다음 두 owner를 `shared_ptr`로 소유하고, Session services가 같은 owner를 보유하는 구조를 기존 RandomGlobals/LocalStateGlobals처럼 재사용한다. 복사/이동을 금지하고 Client 생성 시 한 번만 초기화한다.

- **Chatmanager owner**: 실제 current00548714 DWORD0와 각각의 new2712/new128 실제 미초기화 할당을 토큰별로 소유한다. `Current()`는 실제 DWORD 참조, `Allocate(size)`는 그 저장소 생성, `Bytes(token)`는 같은 물리뷰다. 임의0채움/ChatWindow/정적채널표 치환을 추가하지 않는다.42e050 재초기화는 이전 manager를 free하지 않으므로 current를 덮었다고 이전 할당을 즉시 반환하지 않는다. 실제 free 부모를 연결하기 전에는 owner가 할당을 계속 보유하는 범위도 문서화한다.
- **CoreData owner**:00595350의 현재 가변 span 슬롯,00595348 active DWORD0,005953f0 **lastKnownCoreData10BYTE**를 소유한다. 초기10BYTE는 정적 PE0이고 CRT004feaf0→0042ce70이4/4/2로0을 쓴다. 매 Session마다 초기화하지 않는다. 이 캐시10BYTE와 **00595360의 cachedCoreData[9]·16BYTE stride**는 다른 자료다.0048f350/0048fb30은 후자를 사용하므로10BYTE 하나를 그9기록 표라고 부르지 않는다.

원시 PE의 실제 초기 **Subscriber00540c74는0xffffffff**다. 현재 Client.CurrentSubscriber()가 월드 없을 때 반환하는0으로 Chat 초기화를 대체하면 원문의 **`-1chatFormat`/`-1chatFormat2`** 설정 키가 달라진다. 따라서 별도 현재 Subscriber DWORD를 Client 공유 수명에 옮기고 실제 기존 write 사이트들이 같은 참조를 써야 한다. 저장/Chat 초기화 직전에 값으로 복사하는 우회는 두 Config 호출 사이의 현재 Subscriber 재조회를 보존하지 못한다.

## 연결 위치와 원문 순서

| 현재 소스 위치 | 최소 변경과 선행 계약 |
|---|---|
|`ClientMain.h:204` 부근 장기 owner들·`ClientMain.cpp:162` 생성자|두 owner를 menu/world보다 먼저 생성한다. 현재 channels4800BYTE 소유자와 구별한다. Core의 NULL/active0/캐시0과 Subscriber−1은 Client 최초 한 번이다.|
|`ClientMain.cpp:334` 부근 assets 생성 뒤·Badges ResetCounts 전|실제00439840의42e050을 호출한다. 원문은 init map mode/4437f0 뒤,422490·47d860 앞이다. ctor에서 미리 초기화하거나 Run에서 생략하지 않는다. 설정/타입은 이때 준비돼 있다.|
|`CampaignClientServices.cpp:84` 부근 services 생성|두 실제 owner를 services에 주입한다. fortTechnologyUi는 같은 owner의 RefreshChatManagerRankPrefix에 연결하고 active분기에서 실제 ReadPlayerCoreData를 사용한다. menu.world_가 make_unique 반환 전 NULL일 수 있으므로 설치중 Session/Impl을 직접 빌린다.|
|`RawCampaignSessionLogin.inc:19,24`|cachedCore/currentCore/coreActive의 세션별 저장을 owner 없는 독립 검사 fallback으로만 남기고 실제 경로는 같은 owner 저장소의 **참조**로 바꾼다. 생성/파괴/Begin 때 공유 값을 임의 reset하지 않는다.|
|같은 `.inc:94,98,126`|server MissionLoginGlobals·client MissionClientPacketState·InstallLocalMissionZone 모두 동일 current/cached/active 저장소를 공급한다. 세 곳 중 하나만 바꾸거나 매 호출 사본을 생성하지 않는다.|
|`RawMissionClientPackets.cpp:70` LoginBegin|이미0048f3b0의 동일 owner 쓰기/4·4·2캐시 동작을 수작업으로 갖고 있다. 새 helper와 같은 참조를 사용해 중복 구현을 줄인다. 기존 전체수신 fixture를 유지한다.|
|`RawMissionLogin.cpp:98`·`LocalMissionTransport.cpp:86`|실제 자기 Player 선택이 같은 owner.current에 실제 Player+6c/lastKnownCore alias를 써야 한다. flags/통신/owner 순서는 기존 모듈을 재사용한다.|

이름은 `ClientChatManagerGlobals`/`ClientCoreDataGlobals`, `Client::ChatManagerGlobals()`/`CoreDataGlobals()`, `services.chatManagerGlobals/coreDataGlobals` 정도로 좁힐 수 있다. 이 문서에서는 실제 구현/적용된 API로 표시하지 않는다. 신규 `ReadPlayerCoreData`는 진단85와 진단 뒤 현재 포인터 재조회까지 복원했으므로 core()를 단순 `return current` stub로 공급할 이유는 없다.

## 먼저 해결할 수명과 누락 경계

**Session의 players1620BYTE는 현재 Impl의 vector다(`RawCampaignSession.cpp:333`).** 공유 current를 여기에 둔 채 Session을 파괴하면 Client의 현재 포인터가 dangling 된다. 모든 과거 Player 벡터를 보유하는 방법도 원문 Player 교체/선택 효과를 대신하지 않는다. 원문004b2df0은 PlayerKnown=0을 별도로 쓴 뒤 **0048f3b0(0,1)**으로 이전 current10→lastKnown10·currentNULL·active0을 먼저 처리하고, 뒤의 **004910a0**에서 이전 Player 기록/9개 보조 할당을 free한 후 new1620/초기화한다. 그 뒤 조건에 따라0048f3b0(1,1)을 선택한다. 이 순서를 실제 Session/world 정리 부모에 연결해야 하며 임의 dtor의 무조건 clear/cache복사로 바꾸지 않는다. 공개 helper owner0 자체는 known을 지우지 않는다는 점도 보존한다.

현 `CampaignWorldGlobals::mySubscriber`는 실제 Session 값형 멤버이고 Login.Begin·RawCampaignWorldLoad·LoadExistingEditorSubscriber가 직접 쓴다. Client 공유 Subscriber를 도입하려면 이 기존 소비자도 실제 참조 또는 현재 provider로 바꿔야 한다. 현재 initLoginSubscriber/missionSubscriber의 별도 입력과 실제00540c74를 합치지 않는다. metamap은 현재 ownerMode.challenge의 bool 표현이므로 신규 helper의 실제 DWORD 참조를 매 호출 임시값으로 채우지 않는다. 이 필드의 실제 공유 owner 또는 사본 없는 순수 현재 읽기 provider API를 별도로 검토한다.

**43f660의64항목 Chat 텍스트 명령 registry**는 현재 제품의 GameMenuScriptCode/DialogScriptCode 숫자 명령 조회와 다르다. 원문은 이름32BYTE×64·callback64개·표시 포인터65개·count를 쓴다. format/back/front는42ddf0/42def0을 등록하고 이 callback은 실제 parser·42d160 갱신·현재 Config.Set까지 수행한다. 현재 이 레지스트리/소비자를 검색에서 찾지 못했다. 따라서 신규 Chat initializer의 필수 registerCommand를 이름만 저장하거나 빈 성공으로 공급하지 않는다. 실제 registry와 format/front 부모를 복원하거나 도달한 경계의 필수 service로 유지해야 한다.

rankPicture도 실제 `ChatPrefixIcons`에 연결하되 maximumRank/minorCount는 현재 icon.C/B Type의 실제 필드를 읽어야 한다. 기존 GameMenusTests의 고정 문자열 stub나 상수 아이콘 개수를 생산 경로에 넣지 않는다. 실제0048fb30은 missionPost1이면 자연 생략되지만 일반 Editor/재접속에서는 cachedCoreData9기록→실제 Player+6c 복원·active/current쓰기·조건부 Chat 재초기화가 필요하다. 그 큰 경계를 lastKnown10 복사로 대체하지 않는다.

## 단계별 검증 계획

1. 현재 완료된 독립 PE Chat644/Core804와 기존 Login/ClientPackets/LocalZone fixture를 그대로 보존한다. 새 owner 검사는 같은 실제 DWORD/할당/현재span 주소와 Client 분리를 확인하며 미초기화 BYTE 값은 읽지 않는다.
2. 실제 Run 없는 Client+두 Session headless 검사에서 공유 current가 Player/캐시 중 어느 주소를 가리키는지, 패킷/LocalZone 변경이 다음 Chat refresh에 그대로 보이는지 확인한다. owner0 정리 직전10BYTE 보존→known0의 외부 위치→Player해제→새 선택 순서를 기록한다. 잘못된순서가 dangling 읽기를 일으키지 않는지도 별도 확인한다.
3. 공통 startup의 실제 currentSubscriber−1 설정 키·명령3등록 순서와 rank formatter 뒤 현재 manager 재조회를 확인한다. 필수 registry/Core getter가 누락됐을 때만 도달 위치에서 실패하며 설정을0으로 강제해 숨기지 않는다.
4. 전체 Editor 외곽·원문 State 정상 반환 뒤 최종loaded/HUD를 별도 검사하고, 그 뒤 같은 제품 실제 메뉴·사용자 플레이 회귀를 수행한다. 이 문서/두 독립 helper의 통과는 그 통합 실행의 증거가 아니다.

## 공유 CampaignWorldGlobals 제안과 주소 귀속 교정

root의 후속 제안인 **CampaignWorldGlobals 전체를 Client가 한 개 소유하고 Session은 localfallback+참조로 빌리는 방법**은 기존 소비자의 실제 주소를 유지하는 최소 형태다. 기존 RawCampaignWorldLoad/Runtime이 `CampaignWorldGlobals&`를 받고, Login·Editor Subscriber·메뉴가 `globals.mySubscriber`를 직접 쓰거나 빌리므로 이 전체 묶음을 참조로 바꾸면 각 소비자를 별도 Subscriber 사본으로 고칠 필요가 없다. services가 먼저 선언되므로 해당 shared_ptr의 수명도 소비자보다 길다.

권장 추가 API는 `Client::WorldGlobals()->const shared_ptr<o::CampaignWorldGlobals>&`, `services.worldGlobals`, Session의 `CampaignWorldGlobals localGlobals; CampaignWorldGlobals& globals{services.worldGlobals ? *services.worldGlobals : localGlobals};` 정도다. `Client::CurrentSubscriber()`는 menu/world를 거치지 않고 이 owner.mySubscriber를 직접 읽는다. 테스트만을 위한 Subscriber getter를 더 만들 필요가 없다. Client owner 최초값은 PE의 **mySubscriber=0xffffffff/loginSubscriber=1/editor=0/missionPost=0**을 명시한다. 독립 Session의 기존 값형 기본값과 Login.Begin의 `missionSubscriber.value_or(1)` 시점은 따로 보존한다. 헤더의 기존 local mySubscriber 자체는0이며 독립1은 Begin에서 정해진다.

전체 묶음 공유는 editor/missionPost도 미션 사이에 보존하므로 **현재 ExistingEditor ctor가 editor1을 쓴 뒤 다음 Campaign ctor가 이전의 값형기본0에 의존하는 문제**가 생긴다. 따라서 실제 모드 선택/정리 부모에서 원문 위치의 editor0·missionPost선택을 연결해야 한다. `globals={}`나 전체 struct재생성은 현재 Subscriber/로그인입력을 지우므로 해법이 아니다. 원문 정리의004b3246은 editor0을 쓰지만, 이를 근거 없이 Session ctor/dtor에 반복하는 제안도 아니다. 실제 Editor→Campaign 진입 순서에서 그 부모가 실행되는지를 통합 검사해야 한다.

최초 조사에서 정리 부모를 **004b2c60**으로 잘못 적었다. functions.tsv와 PE 실제 caller를 다시 대조한 정확한 부모는 **004b2df0~004b32d7/1256BYTE**이며 caller는4b34c9/4b6eca/4b71a6/4b752a/4b7d22다. 그 안에서 **4b3127 known0→4b312d call48f3b0→4b31ad call4910a0→4b3246 editor0→조건부4b3257 call48f3b0**를 확인했다.004b2c60은 별도96BYTE localOwner 검색 몸체이고 이 정리와 무관하다. 잘못된 귀속이 든 최초 `subscriber-static.json`은 보존하며 [교정된 정적 기록](../../extracted/cpp-chat-core-owner-20261024-proposed/subscriber-static-corrected.json)을 추가했다.804관측 도구는 이 외곽을 실행하지 않았고 위48f0d0/48f3b0 두 정확한 범위만 실행했으므로, 부모 귀속 오류와 독립 복원 결과를 구별한다.
