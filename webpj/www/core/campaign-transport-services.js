// 캠페인 로그인이 고정 zlocal 양끝 대신 **현재 ID 기반 실제 전송**을 빌리는 좁은 계약(서비스 묶음)과 내부 미션 바인더를 옮긴 모듈이다
// (cpppj `client/CampaignTransportServices.cpp/.h`, 10.78 전용, 기준선 1f00a25a 뒤 변경 중 웹 범위 반영: 문자 주소 연결 칸 connectPeerText `f7210282`, 공용 주소 표시 `CampaignPeerAddressText` 공개 `269de895`).
// 서비스 묶음은 plain 객체이며 각 칸은 실제 backend 호출만 연결한다(빈 성공·Player 성공값·소켓 실행을 추가하지 않는다). 바인더를 만드는 것만으로 연결/수신을 진행하지 않는다.
// 칸: lifetime(backend 소유자)·composite(복합 backend, 다음 State7 인계)·detachSession·callbacksActive·registry(Client State/Chat/cleanup 과 같은 22/32칸)·dispatch(현재 실제 등록 표)·serve·listen·connect(주소 20BYTE/포트)·
//   connectPeerText(원문 State8 의 +54..7b 문자열 영역/포트)·assign·send·sendMutable·flush·poll·peer(owner → 패킷 송신 함수)·connection·exists·isOpen·status·address·peerAddress·close.
// 사용: const services = bindInternalCampaignTransport(localMissionTransport); services.serve(port); services.connect(address20, port); services.poll();
// 웹 범위(LEFT_JOBS 1절): 단독 미션이 쓰는 **내부 미션 바인더**(`bindInternalCampaignTransport`)와 공용 주소 표시(`campaignPeerAddressText`)만 옮긴다. 실제 LAN TCP 바인더(`BindTcpCampaignTransport`)와 Editor 바인더(`BindEditorCampaignTransport`)는
//   LAN/Edit 제외라 만들지 않는다. 복합 전송 바인더(`bindCompositeCampaignTransport`)는 복합 전송 조정자(`composite-mission-transport.js`, E327)가 생긴 뒤 같은 단위 흐름에서 더했다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-12 WB04-E326: 4-C CampaignTransportServices 이식, WB04-E327: 복합 전송 바인더 추가)

// 서비스 묶음이 가질 수 있는 칸 이름 전체이다(기준선 칸 + 기준선 뒤 composite·connectPeerText). 이름 밖 칸은 오타로 보아 거부하는 데 쓴다. (2026-11-12 WB04-E326)
export const CAMPAIGN_TRANSPORT_SERVICE_FIELDS = Object.freeze([
    'lifetime', 'composite', 'detachSession', 'callbacksActive', 'registry', 'dispatch', 'serve', 'listen', 'connect', 'connectPeerText',
    'assign', 'send', 'sendMutable', 'flush', 'poll', 'peer', 'connection', 'exists', 'isOpen', 'status', 'address', 'peerAddress', 'close',
]);

// 원문 004e0a30(1,0,0)의 IP 또는 wildcard 와 포트를 DNS 없이 작성한다. 20BYTE 주소의 +0 WORD 가 6(SPX)이면 웹 범위 밖이라 거부한다. (2026-11-12 WB04-E326: 기준선 뒤 CampaignPeerAddressText 공개 반영)
export function campaignPeerAddressText(address) {
    if (!(address instanceof Uint8Array) || address.length !== 20) throw new TypeError('전송 주소는 20BYTE 여야 합니다.');
    const family = address[0] | (address[1] << 8);
    if (family === 6) throw new Error('LAN 전송 바인더의 SPX 주소는 범위 밖입니다');
    // +4..+7 이 모두 0 이면 wildcard 이다.
    const wildcard = address[4] === 0 && address[5] === 0 && address[6] === 0 && address[7] === 0;
    const host = wildcard ? '*' : `${address[4]}.${address[5]}.${address[6]}.${address[7]}`;
    return `${host}:${(address[2] << 8) | address[3]}`;
}

// 필수 실제 owner/표만 요구하며 생성 때 연결이나 수신을 진행하지 않는 공통 칸을 만든다. 전송은 registry/dispatch 접근자와 send/sendMutable/poll 을 가져야 한다. (2026-11-12 WB04-E326)
function basicServices(transport) {
    if (!transport || !transport.registry) throw new TypeError('실제 전송 owner/현재 Registry가 필요합니다');
    const registry = transport.registry;
    const services = {
        lifetime: transport,
        registry,
        dispatch: transport.dispatch,
        send: (id, packet) => transport.send(id, packet),
        sendMutable: (id, packet) => transport.sendMutable(id, packet),
        poll: () => transport.poll(),
        address: (id) => registry.address(id),
        close: (id) => { registry.disconnect(id); },
    };
    // 주소 표시는 현재 칸의 20BYTE 를 읽어 만든다(저장 사본 없음).
    services.peerAddress = (id) => campaignPeerAddressText(services.address(id));
    return services;
}

// 기존 단독 미션은 같은 내부 두 끝을 사용하며 buffered flag 를 연결 완료로 오인하지 않는다. serve 는 서버 끝만 열고(`open(port, false)`), connect 는 주소 해석 없이 기존 zlocal 서버에 붙는다. (2026-11-12 WB04-E326)
export function bindInternalCampaignTransport(transport) {
    const services = basicServices(transport);
    services.serve = (port) => { transport.open(port, false); };
    // 내부 선택 모드만 주소 해석 없이 기존 zlocal 서버에 붙는다(주소는 20BYTE 여야 함).
    services.connect = (address) => {
        if (address.length !== 20) throw new RangeError('현재 내부 State8 주소는20BYTE여야 합니다');
        transport.connect();
    };
    // 같은 명시 내부 역할은 문자 주소(State8 저장 문자열)에서도 실제 내부 client9 만 연다. (기준선 뒤 connectPeerText, 3.6절 ②)
    services.connectPeerText = () => { transport.connect(); };
    services.assign = (pending, owner) => { transport.assignConnection(pending, owner); };
    services.flush = () => { transport.flush(); };
    services.peer = (owner) => transport.peer(owner);
    services.connection = (id) => transport.connection(id);
    services.exists = (id) => services.connection(id) !== null;
    services.isOpen = (id) => transport.connectionOpen(id);
    // 실제 내부 Connect/Accept 결과와 상대 수명을 읽고 bit2 는 사용하지 않는다.
    services.status = (id) => transport.connectionStatus(id);
    return services;
}

// 실제 Session/World 전체 owner 를 pin 하는 등록 대여까지 포함한 복합 전송 바인더이다. 하나의 조정자(CompositeMissionTransport)를 모든 writer/State/Chat/배치 부모에 공급하고 Session 수신 대여도 반납한다.
// 순서: 등록 대여(bindSession) → 공통 칸 → composite 칸 → 종료 패킷 2 와 기술 0x69 의 기존 등록을 이번 대여로 다시 등록(실행 pin/정리 관문에 포함되고 detach 때 이전 전역 등록이 복원됨) → 대여 반납/콜백 활성 관문/주소/serve/listen/connect/배치/flush/peer/조회 칸.
// 기준선 뒤 cpppj 변경 판정(LEFT_JOBS 3.6절): ② composite 칸·connectPeerText·종료 패킷 2/기술 0x69 재등록(`a76499e3`)을 옮긴다. 고정 Native 수신기(`canonicalReceive`)는 웹에 없어(3.6절 ③ 4-D) 그 분기는 두지 않고,
//   일반 수신기(`receive`/`directReceive`)가 있을 때의 재등록만 한다. 웹 범위는 InternalMission 역할이라 `listen` 은 조정자의 명시 웹 제외 오류를 그대로 낸다. (2026-11-12 WB04-E327)
export function bindCompositeCampaignTransport(transport, sessionOwner) {
    if (!transport) throw new TypeError('복합 전송 실제 owner 누락');
    const lease = transport.bindSession(sessionOwner);
    const services = basicServices(transport);
    services.composite = transport;
    // 종료 패킷 2 의 기존 일반 수신기를 이번 대여로 다시 등록해 대여 표식을 붙인다(없으면 등록하지 않는다).
    const ended = transport.dispatch.registration(2);
    if (ended.receive) transport.dispatch.register(2, ended.receive);
    // 기술 0x69 는 독립 즉시 등록의 기존 호환 입구만 있으면 같은 즉시 입구로 다시 등록한다.
    const technology = transport.dispatch.registration(0x69);
    if (technology.directReceive) transport.dispatch.registerDirect(0x69, technology.directReceive);
    // 자기 세대만 즉시 반납하며 실행 중 callback 은 같은 owner pin 으로 정상 복귀한다.
    services.detachSession = () => { lease.detach(); };
    services.callbacksActive = () => transport.connectionCallbackActive();
    // 다른 하위 전송의 노드 주소도 현재 ID 검증을 거친다.
    services.address = (id) => transport.address(id);
    services.peerAddress = (id) => campaignPeerAddressText(services.address(id));
    services.serve = (port) => { transport.serve(port); };
    services.listen = (port) => { transport.listenLan(port); };
    services.connect = (address, port) => { transport.connect(address, port); };
    services.connectPeerText = (address, port) => { transport.connectPeerText(address, port); };
    services.assign = (pending, owner) => { transport.assign(pending, owner); };
    services.flush = () => { transport.flush(); };
    services.peer = (owner) => transport.peer(owner);
    services.connection = (id) => transport.connection(id);
    services.exists = (id) => transport.exists(id);
    services.isOpen = (id) => transport.isOpen(id);
    services.status = (id) => transport.status(id);
    return services;
}
