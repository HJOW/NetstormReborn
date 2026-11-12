// 캠페인 로그인/전송 서비스 묶음의 계약을 검사한다(WB04-E326, 4-C).
//  - cpppj 에는 이 두 묶음을 직접 겨냥한 시험이 없고(`CampaignSessionTests` 등 Session 시험이 간접 사용) 실제 조립은 4-D 이므로, 기준선 소스의 계약을 단위로 옮겨 확인한다.
//  - `campaignPeerAddressText`: 원문 004e0a30(1,0,0)의 IP/wildcard/포트 표시와 SPX·길이 거부.
//  - `bindInternalCampaignTransport`: 실제 `LocalMissionTransport`(E324)와 등록소에 연결한 칸 전체. 바인더 생성만으로 연결/수신을 진행하지 않고, serve/connect/poll/assign/상태 조회/닫기가 같은 실제 끝을 쓴다.
//  - `createCampaignLoginServices`/`requireLoginService`: 칸 이름 전체(기준선 17 + 기준선 뒤 17)·형식 검증·선택 위치의 명시 거부.
// (2026-11-12 WB04-E326: 4-C CampaignLoginServices·CampaignTransportServices 이식과 함께 추가)
import assert from 'node:assert/strict';
import test from 'node:test';
import { LocalMissionTransport } from '../www/core/local-mission-transport.js';
import { RawConnectionRegistry } from '../www/core/raw-connection-registry.js';
import {
    CAMPAIGN_TRANSPORT_SERVICE_FIELDS, bindCompositeCampaignTransport, bindInternalCampaignTransport, campaignPeerAddressText,
} from '../www/core/campaign-transport-services.js';
import { CompositeMissionTransport, CompositeTransportRole } from '../www/core/composite-mission-transport.js';
import {
    CAMPAIGN_LOGIN_SERVICE_FIELDS, CAMPAIGN_LOGIN_SERVICE_FUNCTIONS, createCampaignLoginServices, requireLoginService,
} from '../www/core/campaign-login-services.js';

// 외부 시계/기록/종료/반납의 호출 수만 관찰하는 분배 장치이다(E324 TransportEffects 와 같은 형태). (2026-11-12 WB04-E326)
function effects() {
    const state = { now: 100, closes: 0, releases: 0, terminations: 0 };
    return {
        state,
        hooks: {
            now: () => { state.now += 1; return state.now; },
            record: () => {},
            terminate: (connection) => { state.terminations += 1; connection.flags |= 0x40; },
            close: () => { state.closes += 1; },
            release: () => { state.releases += 1; },
        },
    };
}

// 서비스 시험용 전송 장면이다: 등록소를 공유하고 로그인 처리기를 등록한 닫힌 전송을 만든다. (2026-11-12 WB04-E326)
function makeScene() {
    const device = effects();
    const registry = new RawConnectionRegistry();
    const transport = new LocalMissionTransport(new Uint8Array(1620), true, device.hooks, registry);
    transport.registerLogin(() => {});
    return { device, registry, transport };
}

// 20BYTE 주소를 만든다: family WORD, 포트 big-endian 두 BYTE, IPv4 네 BYTE. (2026-11-12 WB04-E326)
function address(family, port, ip) {
    const bytes = new Uint8Array(20);
    bytes[0] = family & 0xff;
    bytes[1] = family >> 8;
    bytes[2] = port >> 8;
    bytes[3] = port & 0xff;
    bytes.set(ip, 4);
    return bytes;
}

// ---------------------------------------------------------------- campaignPeerAddressText

// 원문 004e0a30(1,0,0): IPv4 점 표기와 big-endian 포트, 네 BYTE 가 모두 0 이면 wildcard `*`, family 6(SPX)은 범위 밖이다.
test('전송 주소 표시: IPv4 점 표기·wildcard·포트 폭·SPX/길이 거부', () => {
    assert.equal(campaignPeerAddressText(address(2, 6799, [127, 0, 0, 1])), '127.0.0.1:6799');
    assert.equal(campaignPeerAddressText(address(2, 80, [0, 0, 0, 0])), '*:80');
    assert.equal(campaignPeerAddressText(address(2, 65535, [255, 255, 255, 255])), '255.255.255.255:65535');
    // 한 BYTE 만 0 이 아니어도 wildcard 가 아니다.
    for (let index = 0; index < 4; index += 1) {
        const ip = [0, 0, 0, 0];
        ip[index] = 9;
        assert.equal(campaignPeerAddressText(address(2, 1, ip)), `${ip.join('.')}:1`);
    }
    // 포트의 상위/하위 BYTE 는 서로 다른 자리이다.
    assert.equal(campaignPeerAddressText(address(2, 0x0102, [10, 0, 0, 1])), '10.0.0.1:258');
    // family 가 2 가 아니어도 6 만 거부하고 나머지는 표시한다(0 은 빈 칸의 wildcard 표시).
    assert.equal(campaignPeerAddressText(new Uint8Array(20)), '*:0');
    assert.equal(campaignPeerAddressText(address(0x0102, 5, [1, 2, 3, 4])), '1.2.3.4:5');
    // family 는 두 BYTE 전체로 비교한다: 하위 BYTE 가 6 이어도 상위 BYTE 가 있으면 SPX 가 아니다.
    assert.equal(campaignPeerAddressText(address(0x0106, 5, [1, 2, 3, 4])), '1.2.3.4:5');
    assert.throws(() => campaignPeerAddressText(address(6, 5, [1, 2, 3, 4])), /SPX 주소는 범위 밖/);
    assert.throws(() => campaignPeerAddressText(new Uint8Array(19)), TypeError);
    assert.throws(() => campaignPeerAddressText(new Uint8Array(21)), TypeError);
    assert.throws(() => campaignPeerAddressText([2, 0]), TypeError);
});

// ---------------------------------------------------------------- bindInternalCampaignTransport

// 필수 실제 owner/현재 Registry 가 없으면 거부한다.
test('내부 전송 바인더: 전송 owner/Registry 누락은 거부한다', () => {
    assert.throws(() => bindInternalCampaignTransport(null), /실제 전송 owner\/현재 Registry가 필요합니다/);
    assert.throws(() => bindInternalCampaignTransport({}), TypeError);
    assert.throws(() => bindInternalCampaignTransport({ registry: null }), TypeError);
});

// 칸 구성: 내부 바인더가 채우는 칸과 채우지 않는 칸(LAN listen·복합 composite·세션 대여)을 구별하고, 바인더 생성만으로 열기/접속/시계 호출을 하지 않는다.
test('내부 전송 바인더: 칸 구성과 생성 시 무효과', () => {
    const { device, registry, transport } = makeScene();
    const before = device.state.now;
    const services = bindInternalCampaignTransport(transport);
    assert.deepEqual(Object.keys(services).filter((name) => !CAMPAIGN_TRANSPORT_SERVICE_FIELDS.includes(name)), []);
    const expected = ['lifetime', 'registry', 'dispatch', 'serve', 'connect', 'connectPeerText', 'assign', 'send', 'sendMutable', 'flush', 'poll', 'peer', 'connection', 'exists', 'isOpen', 'status', 'address', 'peerAddress', 'close'];
    assert.deepEqual(Object.keys(services).sort(), [...expected].sort());
    // 내부 단독 미션은 LAN listener·복합 backend·세션 대여가 없다.
    for (const name of ['listen', 'composite', 'detachSession', 'callbacksActive']) assert.equal(services[name], undefined);
    // owner/등록소/분배 표는 실제 전송의 것이다.
    assert.equal(services.lifetime, transport);
    assert.equal(services.registry, registry);
    assert.equal(services.dispatch, transport.dispatch);
    // 바인더는 열기/접속/시계 호출을 하지 않는다.
    assert.deepEqual([transport.isOpen, transport.hasClientConnection, device.state.now], [false, false, before]);
});

// serve 는 서버 끝만 열고(open(port,false)), connect/connectPeerText 는 주소 해석 없이 실제 내부 client9 만 만든다.
test('내부 전송 바인더: serve/connect 가 실제 내부 두 끝을 연다', () => {
    const { transport } = makeScene();
    const services = bindInternalCampaignTransport(transport);
    services.serve(6799);
    assert.deepEqual([transport.isOpen, transport.hasClientConnection, transport.port], [true, false, 6799]);
    // 20BYTE 가 아닌 State8 주소는 연결을 만들기 전에 거부한다.
    assert.throws(() => services.connect(new Uint8Array(19), 6799), RangeError);
    assert.equal(transport.hasClientConnection, false);
    // 포트와 주소는 해석하지 않고 내부 client9 에 붙는다.
    services.connect(address(2, 1, [8, 8, 8, 8]), 1);
    assert.equal(transport.hasClientConnection, true);
    assert.equal(transport.client.sender, 9);
    // 이미 접속했으면 전송의 새 접속 계약이 그대로 거부한다.
    assert.throws(() => services.connect(new Uint8Array(20), 6799));
    assert.throws(() => services.connectPeerText('127.0.0.1', 6799));
    // 문자 주소 경로도 같은 내부 끝만 연다(다른 전송으로 새 접속을 시작).
    const other = makeScene();
    const otherServices = bindInternalCampaignTransport(other.transport);
    otherServices.serve(6800);
    otherServices.connectPeerText('ignored.example', 1);
    assert.equal(other.transport.hasClientConnection, true);
});

// poll 이 서버 끝을 pending -1 에 수락하고, 상태 조회는 존재/열림/완료(1)를 같은 실제 끝에서 읽으며 assign 이 재배치한다.
test('내부 전송 바인더: poll 수락·상태 조회·재배치·닫기가 같은 실제 끝을 쓴다', () => {
    const { registry, transport } = makeScene();
    const services = bindInternalCampaignTransport(transport);
    services.serve(6799);
    services.connect(new Uint8Array(20), 6799);
    // 접속 직후 client9 은 있으나 서버 끝은 아직 poll 에서 수락되지 않았다.
    assert.deepEqual([services.exists(9), services.isOpen(9), services.status(9)], [true, true, 1]);
    assert.deepEqual([services.exists(-1), services.isOpen(-1), services.status(-1)], [false, false, 0]);
    services.poll();
    assert.deepEqual([services.exists(-1), services.isOpen(-1), services.status(-1)], [true, true, 1]);
    assert.equal(services.connection(-1), transport.server);
    assert.equal(services.connection(9), transport.client);
    // 존재하지만 닫힘 표시(0x40)가 있는 끝은 열린 상태가 아니며 완료(1)도 아니다.
    transport.client.flags |= 0x40;
    assert.deepEqual([services.exists(9), services.isOpen(9), services.status(9)], [true, false, 0]);
    transport.client.flags &= ~0x40;
    // 다른 칸/범위 밖은 없음 또는 오류이다.
    assert.equal(services.connection(5), null);
    assert.throws(() => services.connection(22), RangeError);
    // 같은 pending 노드를 owner 1 로 재배치하면 그 칸에서 같은 끝이 보인다.
    services.assign(-1, 1);
    assert.equal(services.connection(1), transport.server);
    assert.equal(services.connection(-1), null);
    assert.equal(services.exists(-1), false);
    assert.equal(registry.cleanup().transceivers[1] !== 0, true);
    // 닫기는 등록소의 개별 종료이며, 닫은 끝은 열린 상태가 아니다.
    services.close(9);
    assert.equal(services.isOpen(9), false);
});

// 송신 계열은 존재하지 않는 연결에 대해 전송과 같은 결과(false)를 돌려주고, peer 는 owner 로 고정된 송신 함수를 만든다.
test('내부 전송 바인더: send/sendMutable/peer/주소 표시는 실제 전송과 등록소 위임이다', () => {
    const { registry, transport } = makeScene();
    const services = bindInternalCampaignTransport(transport);
    const packet = Uint8Array.of(1, 0x24, 5, 0, 0);
    // 열기 전 송신은 전송의 계약 오류이며 바인더가 빈 성공(false)으로 바꾸지 않는다.
    assert.throws(() => services.send(3, packet), /연결을 열기 전 송신/);
    assert.throws(() => services.sendMutable(3, Uint8Array.of(1, 0x24, 5, 0, 0)), /연결을 열기 전 송신/);
    // flush 도 같은 전송 호출이라 열기 전에는 전송의 계약 오류가 그대로 나온다.
    assert.throws(() => services.flush(), /열기 전 flush/);
    // 열린 뒤에는 존재하지 않는 연결에 대해 전송과 같은 결과(false)를 돌려준다.
    services.serve(6799);
    services.connect(new Uint8Array(20), 6799);
    assert.deepEqual([services.send(3, packet), services.sendMutable(3, Uint8Array.of(1, 0x24, 5, 0, 0)), transport.send(3, packet)], [false, false, false]);
    const peer = services.peer(7);
    assert.equal(typeof peer, 'function');
    // peer 는 owner 로 고정된 송신 함수이다: 실제 전송의 send(owner, packet) 를 호출한다.
    const calls = [];
    transport.send = (destination, bytes) => { calls.push([destination, bytes]); return true; };
    assert.equal(peer(packet), undefined);
    assert.deepEqual(calls, [[7, packet]]);
    delete transport.send;
    // 빈 칸의 주소는 family 2 와 포트 0, 주소 표시는 wildcard 이다(등록소 004e2a50).
    const empty = services.address(5);
    assert.deepEqual([empty.length, empty[0], empty[1], services.peerAddress(5)], [20, 2, 0, '*:0']);
    assert.deepEqual([...services.address(5)], [...registry.address(5)]);
    // 표 범위 밖 ID 는 등록소가 거부한다.
    assert.throws(() => services.address(22), RangeError);
    assert.throws(() => services.peerAddress(-33), RangeError);
    // poll/flush 는 실제 전송의 같은 호출이다(닫힌 전송은 아무 프레임도 소비하지 않는다).
    assert.equal(services.poll(), 0);
    assert.equal(services.flush(), undefined);
});

// ---------------------------------------------------------------- CampaignLoginServices

// 칸 이름 전체: 기준선 17칸 + 기준선 뒤 17칸 = 34칸이고, 함수 칸은 30개, 값 칸은 네 개이다.
test('로그인 서비스: 칸 이름 전체(기준선 17 + 기준선 뒤 17)', () => {
    const baseline = ['configInt', 'wallSeconds', 'clock', 'refresh', 'playerUiBusy', 'playerNotice', 'observePacket', 'log', 'requestPhase', 'initializeRenderer', 'uiMode', 'battlePalette', 'closeProgress', 'startingProgress', 'initializeChat', 'transport', 'attachCommander'];
    const added = ['metaBattleDescription', 'metaBattleOptions', 'frameLoginTimer', 'gameSeconds', 'creatingIslands', 'progress', 'selectReconnectFort', 'queueAlliance', 'status', 'abortBattle', 'translate', 'frameWallSeconds', 'showTimerProgress', 'closeTimerProgress', 'reconnectFile', 'clearStatus', 'reconnectUi'];
    assert.equal(baseline.length, 17);
    assert.equal(added.length, 17);
    assert.deepEqual([...CAMPAIGN_LOGIN_SERVICE_FIELDS].sort(), [...baseline, ...added].sort());
    assert.equal(CAMPAIGN_LOGIN_SERVICE_FUNCTIONS.length, 30);
    assert.equal(new Set(CAMPAIGN_LOGIN_SERVICE_FIELDS).size, 34);
    // 값 칸 네 개는 함수 칸에 들어 있지 않다.
    for (const name of ['requestPhase', 'transport', 'reconnectFile', 'frameLoginTimer']) assert.equal(CAMPAIGN_LOGIN_SERVICE_FUNCTIONS.includes(name), false);
});

// 만들기: 주어진 칸만 복사하고 frameLoginTimer 는 기본 false 이며 비어 있는 칸은 비어 있는 채이다.
test('로그인 서비스: 주어진 칸만 담고 빈 칸은 비워 둔다', () => {
    const empty = createCampaignLoginServices();
    assert.deepEqual(Object.keys(empty), ['frameLoginTimer']);
    assert.equal(empty.frameLoginTimer, false);
    const configInt = (key, fallback) => fallback;
    const requestPhase = { value: 0 };
    const transport = bindInternalCampaignTransport(makeScene().transport);
    const reconnectFile = { write: () => {} };
    const services = createCampaignLoginServices({ configInt, requestPhase, transport, reconnectFile, frameLoginTimer: true, log: undefined, refresh: null });
    assert.deepEqual(Object.keys(services).sort(), ['configInt', 'frameLoginTimer', 'reconnectFile', 'requestPhase', 'transport']);
    // 값은 복사가 아닌 같은 객체/함수이다(DWORD 상자와 전송은 빌린다).
    assert.equal(services.configInt, configInt);
    assert.equal(services.requestPhase, requestPhase);
    assert.equal(services.transport, transport);
    assert.equal(services.reconnectFile, reconnectFile);
    assert.equal(services.frameLoginTimer, true);
    assert.equal(services.configInt('x', 7), 7);
});

// 형식 검증: 알 수 없는 칸·함수 칸에 비함수·boolean 아님·상자 아님·객체 아님을 거부한다.
test('로그인 서비스: 알 수 없는 칸과 잘못된 형식은 거부한다', () => {
    assert.throws(() => createCampaignLoginServices({ confgInt: () => 0 }), /알 수 없는 로그인 서비스 칸: confgInt/);
    for (const name of CAMPAIGN_LOGIN_SERVICE_FUNCTIONS) assert.throws(() => createCampaignLoginServices({ [name]: 5 }), new RegExp(`${name} 은\\(는\\) 함수여야`));
    assert.throws(() => createCampaignLoginServices({ frameLoginTimer: 1 }), /boolean/);
    assert.throws(() => createCampaignLoginServices({ requestPhase: 5 }), /requestPhase 는 \{ value \} 상자/);
    assert.throws(() => createCampaignLoginServices({ requestPhase: {} }), /requestPhase 는 \{ value \} 상자/);
    assert.throws(() => createCampaignLoginServices({ transport: () => {} }), /transport 은\(는\) 객체여야/);
    assert.throws(() => createCampaignLoginServices({ reconnectFile: 'x' }), /reconnectFile 은\(는\) 객체여야/);
});

// 선택 위치에서의 명시 거부: 비어 있거나 함수가 아니면 위치와 이름을 담아 거부하고, 함수 칸이 아닌 이름을 요청하면 형 오류이다.
test('로그인 서비스: 선택 위치에서 비어 있는 칸은 이름과 위치를 담아 거부한다', () => {
    const attach = () => {};
    const services = createCampaignLoginServices({ attachCommander: attach, frameLoginTimer: true });
    assert.equal(requireLoginService(services, 'attachCommander', 'State13 비서버'), attach);
    assert.throws(() => requireLoginService(services, 'uiMode', 'State7 2'), /로그인 서비스 uiMode 누락 \(State7 2\)/);
    assert.throws(() => requireLoginService(undefined, 'uiMode', 'State7 2'), /uiMode 누락/);
    assert.throws(() => requireLoginService(services, 'frameLoginTimer', 'x'), TypeError);
    assert.throws(() => requireLoginService(services, 'nothing', 'x'), TypeError);
    // 형식이 틀리게 직접 덮은 객체도 함수가 아니면 거부한다.
    assert.throws(() => requireLoginService({ log: 3 }, 'log', 'x'), /log 누락/);
});

// ---------------------------------------------------------------- bindCompositeCampaignTransport (WB04-E327)

// 복합 조정자 장면이다: 서버 권한 상자와 로그인 처리기를 갖춘 InternalMission 조정자를 만든다. (2026-11-12 WB04-E327)
function makeCompositeScene() {
    const device = effects();
    const registry = new RawConnectionRegistry();
    const transport = CompositeMissionTransport.create(CompositeTransportRole.InternalMission, new Uint8Array(1620), { value: true }, device.hooks, registry, {});
    transport.dispatch.register(0x27, () => {});
    return { device, registry, transport };
}

// 바인더는 Session 전체 owner 로 등록 대여를 만들고, 모든 칸을 조정자에 위임하며, 내부 전용 칸(LAN listen)은 조정자의 웹 제외 오류를 낸다.
test('복합 전송 바인더: 등록 대여·칸 구성·위임', () => {
    assert.throws(() => bindCompositeCampaignTransport(null, {}), /복합 전송 실제 owner 누락/);
    const { transport, registry } = makeCompositeScene();
    // 대여는 owner 가 있어야 한다.
    assert.throws(() => bindCompositeCampaignTransport(transport, null), TypeError);
    const owner = { name: 'Session' };
    const services = bindCompositeCampaignTransport(transport, owner);
    assert.deepEqual(Object.keys(services).filter((name) => !CAMPAIGN_TRANSPORT_SERVICE_FIELDS.includes(name)), []);
    assert.equal(services.composite, transport);
    assert.equal(services.lifetime, transport);
    assert.equal(services.registry, registry);
    assert.equal(services.dispatch, transport.dispatch);
    // 같은 조정자의 등록 대여는 한 번에 하나이다.
    assert.throws(() => bindCompositeCampaignTransport(transport, {}), /중복 대여/);
    // serve/connect/poll/조회/배치가 조정자 호출이다.
    services.serve(6799);
    services.connect(new Uint8Array(20), 6799);
    assert.throws(() => services.connect(new Uint8Array(19), 6799), RangeError);
    services.poll();
    assert.deepEqual([services.exists(9), services.isOpen(9), services.status(9), services.exists(-1)], [true, true, 1, true]);
    assert.equal(services.connection(-1), transport.connection(-1));
    services.assign(-1, 1);
    assert.equal(services.connection(1), transport.connection(1));
    assert.equal(services.peerAddress(5), '*:0');
    assert.deepEqual([...services.address(9)], [...registry.address(9)]);
    assert.throws(() => services.listen(6799), /웹 범위 밖: LAN TCP listener/);
    const calls = [];
    const send = transport.send;
    transport.send = (id, packet) => { calls.push([id, packet]); return true; };
    const packet = Uint8Array.of(1, 0x24, 5, 0, 0);
    services.peer(1)(packet);
    assert.equal(services.send(1, packet), true);
    transport.send = send;
    assert.deepEqual(calls.map(([id]) => id), [1, 1]);
    assert.equal(services.flush(), undefined);
    assert.equal(services.callbacksActive(), false);
});

// 문자 주소 연결은 조정자의 내부 끝만 연다.
test('복합 전송 바인더: connectPeerText 는 조정자의 내부 끝만 연다', () => {
    const { transport } = makeCompositeScene();
    const services = bindCompositeCampaignTransport(transport, {});
    services.serve(6799);
    services.connectPeerText('저장된 주소', 6799);
    assert.equal(transport.exists(9), true);
});

// detachSession 은 자기 대여만 반납하고, 종료 패킷 2/기술 0x69 의 기존 일반 수신기는 이번 대여로 다시 등록돼 detach 때 전역 등록으로 복원된다.
test('복합 전송 바인더: 대여 반납과 종료 패킷 2·기술 0x69 재등록', () => {
    const { transport } = makeCompositeScene();
    const ended = () => {};
    const technology = () => {};
    transport.dispatch.register(2, ended);
    transport.dispatch.registerDirect(0x69, technology);
    assert.equal(transport.dispatch.registration(2).leased, false);
    const services = bindCompositeCampaignTransport(transport, {});
    // 이번 대여로 다시 등록됐다.
    assert.equal(transport.dispatch.registration(2).leased, true);
    assert.equal(transport.dispatch.registration(0x69).leased, true);
    assert.equal(typeof transport.dispatch.registration(2).receive, 'function');
    assert.equal(typeof transport.dispatch.registration(0x69).directReceive, 'function');
    services.detachSession();
    // 반납 뒤에는 새 대여를 만들 수 있다.
    const again = bindCompositeCampaignTransport(transport, {});
    assert.ok(again.detachSession);
    // 기존 수신기가 없으면 아무 것도 새로 등록하지 않는다.
    const fresh = makeCompositeScene().transport;
    bindCompositeCampaignTransport(fresh, {});
    assert.equal(fresh.dispatch.registration(2).receive, null);
    assert.equal(fresh.dispatch.registration(0x69).directReceive, null);
    // 재등록하지 않았으므로 대여 표식도 붙지 않는다.
    assert.equal(fresh.dispatch.registration(2).leased, false);
    assert.equal(fresh.dispatch.registration(0x69).leased, false);
});

// 수신 callback 이 실행되는 동안 callbacksActive 가 켜진다.
test('복합 전송 바인더: callbacksActive 는 수신 callback 중 참이다', () => {
    const { transport } = makeCompositeScene();
    const services = bindCompositeCampaignTransport(transport, {});
    services.serve(6799);
    services.connect(new Uint8Array(20), 6799);
    services.poll();
    services.assign(-1, 1);
    let during = null;
    transport.dispatch.register(0x24, () => { during = services.callbacksActive(); });
    services.send(9, Uint8Array.of(1, 0x24, 5, 0, 0));
    services.flush();
    assert.equal(services.poll(), 1);
    assert.equal(during, true);
    assert.equal(services.callbacksActive(), false);
});

// 조회 칸은 없는 ID 와 다른 owner 의 노드를 구별한다: 없는 ID 는 false/0, 같은 등록소의 다른 전송 노드는 조정자의 오류를 그대로 낸다. 주소 표시는 현재 노드의 실제 주소를 쓴다.
test('복합 전송 바인더: 조회 칸의 없는 ID·다른 owner 구분과 실제 주소 표시', () => {
    const { transport } = makeCompositeScene();
    const services = bindCompositeCampaignTransport(transport, {});
    // 없는 ID.
    assert.deepEqual([services.exists(7), services.isOpen(7), services.status(7), services.connection(7)], [false, false, 0, null]);
    // 현재 노드의 20BYTE 주소를 읽어 점 표기로 보여 준다.
    services.serve(6799);
    services.connect(new Uint8Array(20), 6799);
    transport.connection(9).address.set([2, 0, 0x1a, 0x8f, 127, 0, 0, 1]);
    assert.equal(services.peerAddress(9), '127.0.0.1:6799');
    assert.deepEqual([...services.address(9).subarray(0, 8)], [2, 0, 0x1a, 0x8f, 127, 0, 0, 1]);
    // 같은 등록소의 다른 전송이 가진 노드는 이 조정자의 소유가 아니다(연결 전의 새 조정자에서 다른 전송이 client9 를 연다).
    const foreign = makeCompositeScene();
    const foreignServices = bindCompositeCampaignTransport(foreign.transport, {});
    const other = new LocalMissionTransport(new Uint8Array(1620), true, effects().hooks, foreign.registry);
    other.registerLogin(() => {});
    other.open(6900);
    assert.throws(() => foreignServices.exists(9), /다른 owner의 복합 연결 주소 조회/);
    assert.throws(() => foreignServices.isOpen(9), /다른 owner의 복합 연결 open 조회/);
    assert.throws(() => foreignServices.status(9), /다른 owner의 복합 연결 status 조회/);
    assert.throws(() => foreignServices.address(9), /다른 owner의 복합 연결 주소 조회/);
    assert.throws(() => foreignServices.peerAddress(9), /다른 owner의 복합 연결 주소 조회/);
});
