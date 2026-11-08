# M6 전수 대조 준비 — 2026-11-08 / HJOW-Athlon

**1185 현행:**1184 가상 조회 뒤 n9c1 장기 HP/보상·같은 분산 SP/현재 fort·editor·현재 Player base와 선택된 실제 AI Runtime을 연결했다. 비MP26검사와 Tutorial4 생산/건설 요청·600frame 대기, Edit 진입/복귀를 확인했다. NoWorld 공통/자원 파생 삭제·ConstructionRuntime·큰 AI/Carrier owner·전체 Snapshot/JoinTestBattle와 Source/listener/표/Poll·63잔여F·M6-l/m/n/큐는 미완료다. 태그25는 전scope 완료 수가 아니며 새MP 실행0/큐 비활성이다. [1185 근거](cpp-reward-sp-owner-20261109.md).

이하 본문의 초기 조사는 제품 코드/LEFT_JOBS/추적 문서를 수정하지 않은 병렬 읽기 전용 조사다. 실행 검사·빌드·MP/소켓/원본 실행은 0이다. 마지막 전체 정밀 디컴파일은 LEFT_JOBS의 2026-11-03/HJOW-Athlon, R 전용 2026-11-07/동일 호스트로 현재 호스트와 일치한다. 새 디컴파일은 하지 않았다.

`audit.json`은 당시 HEAD/28개 근거 파일 SHA-256과 원문40b4b0의 수신 등록92행을 보존한다. `original-fragments.c`는 아래 직접 근거의 생성 C 원문 발췌다. 조사 이후 부모/다른 에이전트의 소스가 바뀔 수 있으므로 구현 직전에 현행 파일과 다시 대조한다. 후보 수를 M6 전체 완료율이나 예상 커밋 수로 쓰지 않는다.

## 1. 중복 작업하지 않을 기존 완료

- 일반 로그인/대기실/State7 연결은 985~1002, 저장/읽기/재접속/서버인계는1003~1015의 근거가 있다. 1000 문서의 당시 '다음 저장'을 그대로 재개하지 않는다.
- 일반 자원486bc0/지급·방송/제재·원격 Tell은1016~1020, 객체/프로세스 전체수신과 실제writer/등록은1021~1034,31h 입력/해석/기록재생은1035~1044, 연결종료/수락/재사용은1045~1048이다. 함수별 잔여를 아래처럼 실제 현재 소스에서 확인한 뒤 진행한다.
- Outpost Pop은 이미 `RawFactoryPostPop.cpp::PostPop`의 Kind==2에서 같은 Outposts/Residences 목록→regionOwnership→워크샾 부모를 처리한다. `RawCampaignAssetBindings.cpp`가 실제 공통 Pop에 이 모듈을 등록하고 Session이 `assetState.sharedOutposts/Residences`와 같은 owner를 빌린다. 오래된 `cpp-outpost-lifecycle-reconstruction.md`의 Pop/지역후처리 잔여를 다시 만들지 않는다.
-78h 강제동맹은 `RawCampaignSessionLogin.inc:268~271`과 `RawCampaignSession::RequestForcedAlliance`에 구현돼 있다.77h/79h와 혼동하지 않는다.
- 패킷 기록의 현재 FILE/프레임/송수신 기록과1ah/1bh Open은1151~1156 완료다. 요청 DWORD의 writer 부재는 5절처럼 판정한다.

## 2. 바로 구체화할 필수 단위 제안

아래 순서는 의존관계를 드러내는 제안이며 현행 LEFT_JOBS M4/M5의 우선순위를 바꾸지 않는다. M4.1에서 대조하는7ch **송신**과 아래7ch **수신**은 별개다.

| 제안 | 확인된 차이/실제 활성 경로 | 원문 | 구현 위치/끝 조건 |
|---|---|---|---|
| M6-a | 정상 로그인 송신4종의 observer가 무조건 throw |40a540/40a590/40ae80/40b460 | `RawMissionClientPackets.cpp:175~181`의 sending guard를 실제 mutable packet observer로 바꾼다.2a/2b/67/81 선언길이·진단 정상복귀를 실제같은전송에 연결한다. |
| M6-b | 원문 전역 등록과 누락12수신/정적 observer·버전 감사 |40b4b0/4ded50/4ded70/4ded90 | 아래3절의 원문92행을 실제 Client 장기표/Session 임대별로 분류한다. no-op C3 수신과 receiver NULL은 구별한다. observer만 설치하고 수신완료로 세지 않는다. |
| M6-c |3fh 원격 건설통지 decoder/수신 등록 없음 |4441b0/40a800 | `RawCampaignConstructionBindings::Register`는 현재3e만등록한다. 기존 `RawConstructionNotice`/현재PrepareConstruction/같은통지 이력을 대여해 wire decoder→실제Handle을연결한다. Typed19개 선검사 경계도 원문과 재대조한다. |
| M6-d |53h 강제단일삭제·62h 다중삭제의 원격 수신 없음 |4436b0/433120/40ad70 | 같은Pool/현재VT/실제world.Destroy/공통 송신버퍼·해제수명을 연결한다. 단일 pointer 범위·free와 다중loop 현재count/SID를 원문 위치에서 읽는다. |
| M6-e |69h Core/기술 전체 동기화 수신 없음 |4b49e0/41c340/4929e0/40af10 | 기존 Client CoreData의 같은9×16/현재Player/기술검색·원문목록을 연결한다. 새Core사본/항상성공으로대체하지않는다. |
| M6-f |6ah DaisProcess 전체 decoder/실제 writer 미연결 후보 |44a280/상응 프로세스 가상표 | 현재DaisRegular/실제 생성자·Host·Kernel·Form sid 저장소를 사용한다. Session process송신 decorator는 현재Gun/Animation/Aviary/Flyer/BombTimer를 처리하지만Dais전용branch가없다. 전체VT writer 전수와 함께마감한다. |
| M6-g |77h 동맹요청/79h 동맹해제·6fh Draw합의와 실제입력 없음 |48f7f0/48f8f0/4900e0 | 기존78h 강제동맹/3dh 전체동맹은 유지한다. 같은Player/alliance/이름·현재Chatbusy/Tell·MessageBoard·boss중계·Drawdeadline을 연결한다. Game메뉴의 Declare a Draw는 이미행을생성하므로 명령db의현재소비자도끝까지연결한다. |
| M6-h |7bh 현재참가자 게임주소 수신 없음 |4b4790/40b2f0 | 새R `root/ChallengeSend.cpp::SendAddress`가 실제7b를송신한다. 현재CS participant ID가packet과같을때NUL포함복사/주소40BYTE·observer signed BYTE길이를복원한다. |
| M6-i |7ch master확인 응답/실제연결재배치 수신 없음 |4b2cc0 | 현재MetaConnectionRuntime에는Observe/Version/Length만있다. 원문master선택→자기인지/ID·두gameDWORD검증→응답1/2·정상슬롯으로Assign/현재server임시1/캐시무효화·1fh후속을같은CS/TCP owner로연결한다. M4.1의송신1095존재만으로닫지않는다. |
| M6-j |86h Flyer명령 수신 없음 |426490→45b320/45b3b0/45b430 | `RawBattleBroadcast::WriteFlyerCommandPacket`과송신은있다.현재프로세스TYPE005410ec/Contained조회·현재Flyer기한/다음주기변경과callback진단을같은실제FlyerProcess에연결한다. 공중3종전용실행검사는제외한다. |
| M6-k |Outpost의고유삭제전처리/현재실제삭제Caller없음 |455840/VT509210 | 아래4절. 두목록제거→지역Update→DamageablePre를원문순서로연결하고공통가상Destroy/피해Post·실제선택수명을마감한다. Pop은재작성하지않는다. |
| M6-l |활성기본지연callback queue가없음 |4def40/4df010/4dee60/4def20/4df5e0/4dfc00 | 아래6절. 현재Client장기분포/OS시계·rand/각노드queue·현재주소복원/전송수명·같은등록소유자를연결한다. 기본callback지연을단순false로닫지않는다. |
| M6-m |개발자이름제한JoinTestBattle 직접Caller없음 |4b8be0→1bh4b8b20 | 이름4개제한·현재주소·server0/current29→기존실제1bh전체수신을직접호출한다. 일반명령0200002b는선행4b3620을수행한다. Edit의TestBattle(c2)와구별한다. |
| M6-n |마감시전수현재Caller/송수신/선택적기록·소유권재감사 |전역등록92행/모든실제processVT | 제품표의원문receiver/observer/version/길이와실제입력/송신·현재월드임대/파일/해제 owner를대조해각위치의빈장치/throw/성공상수를분류한다. MP실행검사는0을유지한다. |

### M6-a의 실제 활성 오류

`RawMissionClientPackets.cpp:175~181`은2a/2b/67/81의sending!=0에서항상 `미션 수신 observer의 송신 버퍼 경계 미연결`을throw한다. `RawMissionLogin`/`RawMissionServerPackets`/`RawMissionLoginPackets`가이들패킷을작성하고같은 `LocalTcpTransport::Send:279`가 `NotifyObserver(writable,1)`를호출하므로 정상일반로그인송신도이경계를선택한다. writer가이미선언길이를확정했다는이유로빈observer로치환하면원문진단/후행현재길이조회가없어진다. `ObservePacket`의mutable span을사용해실제본문을읽어야한다.

## 3. 원문 수신 등록표의 현재 누락

원문 `0040b4b0`에는 `4ded50(type,receiver)`가92행이다. `audit.json.original_receivers`에모두있다. 해당중12타입은현재whole-src의등록literal과Session의동적type목록에서수신이없음을확인했다.

| type | 원문 body | 현재판정 |
|---|---|---|
|0b|4b3af0|버전조회응답(0c의10/78 WORD)을실제현재송신자에보내는수신없음. 폐기CD7f/80과별개다.|
|3f|4441b0|typed Notice본문/서버직접호출은있지만wire decoder/등록없음.|
|53|4436b0|포인터범위/살아있는SID의flags0 가상삭제수신없음.|
|62|433120|현재signed count와각WORD SID를순회하는삭제수신없음.|
|69|4b49e0|전체Core/현재기술목록처리수신없음.|
|6a|44a280|DaisProcess가변전체decoder수신없음.|
|6f|4900e0|Draw거부/모든PlayerDraw bit해제,찬성bit/안내/boss중계/기한수신없음.|
|77|48f7f0|동맹요청/목표로컬·Chatbusy·Tell수신없음.|
|79|48f8f0|동맹해제/로컬양쪽Tell·타인상태안내수신없음.|
|7b|4b4790|현재CS의ID일치참가자주소수신없음.|
|7c|4b2cc0|송신·관찰등록은있고수신없음.|
|86|426490|Flyer명령방송writer가있고수신없음.|

receiver가원문 `004437f0`(C3단일RET)인0c/42/4d/55/56/57/5a도현재명시등록이없다. 이들은기능본문을새로발명할범위가아니지만 `Validate`의handler유무/시각/record/version·observer수명은원문차이가남는다. 55/56/57은비어있지않은가변길이observer가있으므로무조건빈처리로통합하지않는다. 폐기7f/80은복구하지않는다. Root전용타입을Client에수신등록하거나관찰전용타입에receiver를발명하지않는다.

## 4. Outpost 현재 guard와 수명 차이

- `RawCampaignSession.cpp:343`의일반Damageable14VT에는509148은있고509210은없다.1547~1601의실제삭제wrapper에도Outpost고유분기없음. `RawOutpostLifecycle`의client참조는0이다.
- 원문455840은extra+28의&9만분기한다. flags와무관하게두원본Array의모든동일SID를제거하고현좌표로455380지역효과→44b4b0부모다. 부모없는분기를성공으로닫지않는다.
- 기존 `RawOutpostLifecycle::Object`는SID>=5·state&9/일반타입범위를선행검사한다. 여기state&9는free1/contained8이며dead2를포함하지않는다. 따라서 `RawSquidDestroy::Destroy`의PreDestroy앞dead2쓰기만으로이helper가거부되지는않는다. 원문455840은이선행유효성검사없이extra+28분기를읽으므로정상수명/손상입력/부분효과경계를구별해재대조한다.
- 기존helper는validatePost/validatePre/양쪽목록Items전수검사를실제목록쓰기앞에넣는다. 원문은순차Array처리/현재좌표이며손상두번째목록/콜백변경의부분효과를원문과구별해야한다. 부모사전검증을원문활성경계로오인하지않는다.
- 같은Outposts/Residences목록은이미Pop/RegionOwnership/CarrierChoice/Playerlocator가공유한다. 새로운별도목록을만들면안된다.

## 5. 요청 DWORD·JoinTestBattle 판정

`DAT_0054dbac`는생성C에서설정reset0와1ah/1bh read의3참조뿐이다. 보호PE의실행섹션을Capstone으로정적역어셈블해직접메모리/즉시주소참조를검사해도 `4354b2 mov [54dbac],ebx`(435220의0값), `4b4e7b/4b8b6f read`뿐이었다. 활성writer를새Config/새CLI/성공토글로추가할근거가없다. 현Requested=0/수신조건부Open은현재원문처럼유지한다. 임의외부메모리쓰기나미작성스택은이검사의주장이아니다.

JoinTestBattle의직접입력은 `005427a4`의PE DWORD `0200002b`다.4b8be0은현재handle을 zsimpson/jgreer/kdemarest/tbratton 4개와모두비교한뒤부적합하면로그만남긴다. 적합하면원문1b헤더/seed0/현재0054dae4주소→server0→current1d(29)→기존1bh직접수신이다. 사용자Edit TestBattle 명령c2와무관하다. actualStateRun/같은메타owner·7 state인계는1156의기존경계를재사용하고새로직을복제하지않는다.

## 6. 기본 활성 지연 큐

원문Client시작의실제 `439863`은005e4784에분포00543600을쓴다. 같은호스트의PE직접바이트는00543600=`01 ff`다.435220의Config `lag`(문자열00505a44)1/2/3은각005435e0/ec/f4에분포를선택한다. 현재소스에는lag분포/queue소비자가없고 `PacketConnection.delayed`는false최초값만있다. `RawPacketDispatch.cpp:49`/`RawPacketFrame.cpp:17`은delayed에서명시미복원이다. 정상원문4dfc00의시작검사때기본queue가설치되므로디버그전용미선택으로닫지않는다.

-4def40:100칸pointer배열/읽기쓰기index0/현재분포주소·signed BYTE합계를만든다. 원문분포FF종료와50BYTE상한을지킨다.
-4df010:NULLreceiver는0반환. queue없으면depth증가→직접수신→depth감소/지연0. queue있으면현재CRT rand%분포합을소비해가중bucket선택,남은칸2미만일때현재ring을두배로늘림/순서보존,delay가-1일때만추가rand%100+bucket*100을선택한다. 같은집합후속은첫선택delay를공유한다.
-4dee60:실제observer(1)→그때선언WORD길이할당/현재packet복사→현재wall+delay*원문상수→송신자/실제receiver/같은connection/20BYTE현재주소를보관한다. enqueue를수신직접호출로바꾸지않는다.
-4def20:deadline < currentwall일때만ready다. <=로바꾸지않는다.
-4df5e0:선두ready아니면그대로반환하며뒤를앞질러실행하지않는다. actualdepth를올리고같은connection의20BYTE주소를보관주소로임시교체→저장receiver/같은connection/보관packet 호출→packet해제→주소복구→depth감소→node해제/index진행한다. closed면-1로중단한다.
-4dfc00:선택분포가있고현재분포NULL이면queue설치→지연호출실행→현재closed반납판정→원래수신폴링이다. 내부Mission전송/UDP/Root에같은정책을무조건적용하지말고각원문가상poll경로를대조한다.

원문32bitraw포인터값자체대신현재stableNode/실제주소/Session등록임대의소유권으로연결하되관찰·기록·순서·난수소비·현재callback수명은유지한다. MP검사는사용자제외이므로실행하지않는다.

## 7. 마감 조건

모든후보는정적확인이지기능통과가아니다. 새MP단위/패킷/게임/통신/기록파일실행은0을유지한다. 각구체단위의원문C/PE현재실제caller/같은owner·등록반납을대조하고Release컴파일을수행한부모가LEFT_JOBS와단위보고서/로컬커밋을갱신한다. 각부품의존재를전체LAN완성으로보고하지않는다. 새누락발견시해당M6하위종료조건을명시적으로늘린다.
