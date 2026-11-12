# webpj 복합 전송 조정자(CompositeMissionTransport) — WB04-E327 (2026-11-12, 4-C)

3.1절 4-C 의 client **CompositeMissionTransport**(기준선 226줄 = `.cpp` 169 + `.h` 57, 기준선 뒤 +97줄)를 `core/composite-mission-transport.js` 로 옮겼고, 이 조정자가 생겨 미뤄 둔 E326 의 `BindCompositeCampaignTransport` 를 `core/campaign-transport-services.js` 에 더했다(`bindCompositeCampaignTransport`). 조정자는 전송별 전체 Poll 을 이어 붙이지 않고 **현재 전역 ID 별로 실제 하위 호출만 선택**하는 얇은 계층이다. 단독 미션은 `InternalMission` 역할 하나뿐이며(cpppj `UberGump.cpp:1083` 의 `MakeClientCampaignTransport(client_,InternalMission,world_)`), 웹은 그 역할만 지원한다.

## 1. 규칙

- **생성:** `CompositeMissionTransport.create(role, players, server, hooks, registry, globalsLifetime)`. Player9(정확히 1620BYTE)·등록소(`RawConnectionRegistry`)·장기 참조(C++ `shared_ptr<void>`, GC 가 대신하지만 호출 계약상 필수)가 없으면 거부하고, 한 번만 초기화한 새 등록표(`PacketRegistrationTable`)로 내부 전송(`LocalMissionTransport`, E324)을 만든다. 생성만으로 listener/client 를 열거나 시계를 부르지 않는다. 역할이 `Editor`·`LanHost`·`LanClient` 이면 웹 범위 밖(`웹 범위 밖: 복합 전송 역할 …`)으로 명시 거부하고 모르는 역할은 `TypeError`.
- **하위 전송 선택 `Select(id)`:** 현재 등록소 토큰이 없으면 Missing, 내부 전송의 끝이면 Internal, 같은 등록소의 다른 owner 노드면 Unknown(범위 밖 ID 는 `RangeError`). 모든 호출이 callback 뒤에도 현재 표를 새로 읽는다. Unknown 노드는 조회/송신/flush 에서 오류(`다른 owner의 복합 연결 …`·`소유하지 않은 현재 … 노드`), 수신 순회에서는 **살아 있는 남의 노드면 빈 결과**(그 전송의 Poll 몫), **등록소에도 없는 별칭이면 원인(id·토큰·역할)을 담아 거부**한다.
- **serve/connect:** `serve(port)` 는 서버 권한(boolean·`{ value }`·함수의 현재 값)이 없으면 거부하고, 내부 listener 가 이미 열려 있으면 다시 열지 않는다(004e2430). `connect(address20, port)` 는 State8 주소 20BYTE 정확히를 확인하고 내부 client9 만 만든다(주소 해석 없음), `connectPeerText` 는 문자 주소여도 같은 내부 끝만 연다. `listenLan` 은 웹 제외 오류.
- **재배치/송신:** `assign(pending, owner)` 는 Internal 노드에만(없으면 `현재 accepted 노드 없는 복합 전송 재배치`). `send`/`sendMutable` 은 연결이 없으면 헤더가 짧아도 `false`, 있으면 헤더 4BYTE 미만 거부 뒤 선택 전송이 보낸다(`sendMutable` 만 observer(1)). `flush()` 는 정상 0..21 을 번호순으로 Internal 만 `flushOne`. `peer(owner)` 는 호출마다 현재 ID 를 다시 조회하는 송신 함수(owner 는 32비트 부호 있는 값).
- **`poll()`:** 내부 대여 시작 → 내부 listener 수락 → 각 pending(-1..-31) 한 번 → 정상 0..21(ID 별 `again` 이 거짓일 때까지) → 대여 반납. 중첩 호출은 `복합 전송 Poll 중첩 호출` 로 거부하고, 수신 중 오류가 나도 대여를 반납한 뒤 원래 오류를 전파하며 진입 표시를 영구히 막지 않는다. 반환은 소비한 프레임 수.
- **정리/조회:** `close(id)` 는 등록소의 원문 개별 종료, `closeRoom()` 은 자기 정상 노드 → pending 을 Native 종료한 뒤 내부 두 끝을 마무리하며 다른 owner 노드는 건드리지 않는다. `connection`/`exists`/`isOpen`/`status`/`address` 는 자기 노드만(없으면 null·false·0). `bindSession(owner)`·`connectionCallbackActive()` 는 공유 등록표의 대여/콜백 관문, `ownsConnection(node)` 은 내부 두 끝 식별, `dispose()` 는 C++ 소멸자(자기 두 끝의 등록소 별칭만 반납).
- **복합 전송 바인더 `bindCompositeCampaignTransport(transport, sessionOwner)`:** 등록 대여(`bindSession`) → 공통 칸 → `composite` 칸 → 종료 패킷 2 의 기존 일반 수신기와 기술 0x69 의 기존 즉시 입구(`directReceive`)를 이번 대여로 **다시 등록**(실행 pin/정리 관문에 포함되고 detach 때 이전 전역 등록이 복원됨, 수신기가 없으면 아무 것도 등록하지 않음) → `detachSession`(자기 대여만 반납)·`callbacksActive`·`address`/`peerAddress`·`serve`/`listen`/`connect`/`connectPeerText`/`assign`/`flush`/`peer`/`connection`/`exists`/`isOpen`/`status` 위임.

## 2. 기준선 뒤 변경 판정 (+97줄, 판정 커밋 `a40c0028`·헤더 `a76499e3`)

기준선 뒤 바꾼 cpppj 커밋은 8개다(`git log 1f00a25a..HEAD`). 변경의 대부분이 웹 제외 대상(LAN TCP/UDP·Meta·Editor)이다.

- **② 반영(InternalMission 에 닿는 것):** `serve` 의 `IsOpen()` 관문(`f98d50d52`), 수락 순서 내부 → Editor → TCP(`6b700418c`, 내부만 있을 때 같음), `ConnectPeerText`(`f72102823`), `ReceiveOne` 의 Unknown 노드 처리와 원인 담은 오류(`a40c00280` — Test Battle 동안 남은 Editor 노드를 거부하던 결함 교정), `OwnsConnection`(`a76499e35`), 그리고 바인더의 `composite` 칸/종료 패킷 2·기술 0x69 대여 재등록(`a76499e35`).
- **① 결과 영향 없음(GC 가 대신):** `shared_from_this` pin, `Registration(type).Pin()` 의 Session shared_ptr 수명 보유, `inheritedNetwork_` 의 전체 Meta callback owner 보유.
- **웹 제외(명시 오류 또는 만들지 않음):** `Editor`/`LanHost`/`LanClient` 역할과 TCP/UDP backend 분기(`Backend::Tcp/Udp/Editor` 선택·수락·송신·flush·수신·방 종료·조회), `SharedNetwork`·`SharesNetwork`·`BindMetaSource`·`MetaSource`·`RebindWorld`(Meta 재진입·LAN 서버 이전, 비LAN 역할이면 항상 거부)·`EditorTransport`·`ListenLan`/`LanClient→LanHost` 전환. 내부 끝을 늦게 만드는 경우가 없어 Poll 의 `beginLateLocal` 도 옮기지 않았다.
- **③ 4-D:** 바인더의 고정 Native 수신기(`canonicalReceive`)에 대한 `RegisterCanonical` 분기 — 웹에 canonical 수신기가 없으므로(3.6절 ③ 4-D) 일반 수신기가 있을 때의 재등록만 한다.

## 3. 검사 (`tests/composite-mission-transport.test.mjs` 19건 + `tests/campaign-services.test.mjs` 의 복합 바인더 5건)

cpppj 에는 이 조정자를 직접 겨냥한 시험이 없어(Session 시험이 `MakeClientCampaignTransport` 로 간접 사용, 4-D) 기준선 소스와 ② 반영분을 단위 계약으로 옮겨 확인했다. 조정자 12건 + 변형 확인 보강 7건: 생성 계약/역할, serve 의 서버 권한·이미 열린 listener, connect/connectPeerText(주소 20BYTE 정확히), poll 의 수락/assign/조회, 중첩 거부와 소비 수, 수신 오류의 대여 반납, 송신 계열(없는 연결의 짧은 패킷, observer 차이), 다른 owner 노드의 Unknown 처리, 등록표에 없는 별칭 오류, closeRoom, 대여/콜백 관문, dispose, pending 수신 계수와 -31 번 칸, 연속 프레임 소비, 닫힌 끝의 상태, 콜백 중 활성 관문/`ownsConnection`, 방 종료 뒤 같은 조정자의 재열기/재접속. 바인더 5건: 칸 구성/위임, 문자 주소 연결, 대여 반납과 종료 패킷 2·0x69 재등록, 콜백 활성, 없는 ID/다른 owner 구분과 실제 주소 표시. **첫 실행에서 모두 통과했다**(시험 쪽 가정이 틀린 곳은 변형 확인에서 바로잡음: 바인더 시험에서 같은 토큰을 두 칸에 넣어 만든 가짜 "남의 노드"는 실제로는 자기 노드로 선택돼 별도 전송으로 다시 구성, 시험 파일에 남은 잔여 줄은 구문 오류로 드러나 제거).

## 4. 변형 확인과 남은 경계

- 한 문자열 치환 변형 70개(조정자 48·바인더 22)를 돌려 조정자는 첫 실행에서 13개가 남았다(주소/Player9 크기의 정확한 경계, 짧은 패킷 순서, send/sendMutable 구분, pending 순회·계수, `again` 반복, 상태/콜백 관문, `ownsConnection`). 시험을 보강한 뒤 **1개가 남았고 동등**하다: `closeRoom` 의 pending 순회 끝 칸(-31)을 빼는 변형(내부 전송은 첫 빈 pending 하나만 쓰므로 -31 은 단독 미션에서 닿지 않음). `closeRoom` 끝의 `internal.closeAll()` 제거는 처음에는 동등으로 보았으나 `closeAll` 이 내부 전송의 열림 표시(`#open`)를 내린다는 것을 확인해 재열기 시험으로 잡았다. 바인더 22개는 보강 뒤 모두 잡혔다(첫 실행 7개 생존: 대여 표식 유무·다른 owner 거부·없는 ID 조회·실제 주소 표시).
- **앱 수명:** 아직 Session 이 이 조정자를 만들지 않는다(PORT_MAP session 0). `ClientCampaignTransport`(다음 항목)가 `MakeClientCampaignTransport` 로 `create` + `bindCompositeCampaignTransport` 를 부른다.
- 3.1절 4-C 는 27/30 이다. 다음: client `ClientCampaignTransport`.
