// 복합 전송 조정자(단독 미션 InternalMission 역할)의 계약을 검사한다(WB04-E327, 4-C).
//  - cpppj 에는 이 조정자를 직접 겨냥한 시험이 없고(`MakeClientCampaignTransport` 로 Session 시험이 간접 사용, 4-D) 기준선 소스와 기준선 뒤 ② 반영분을 단위 계약으로 옮겨 확인한다.
//  - 현재 ID 별 하위 전송 선택(Missing/Internal/Unknown), serve 의 이미 열린 listener 유지, connect/connectPeerText, poll 의 수락→pending→정상 순서와 중첩·오류 반납, 송신/flush/상태 조회, 방 종료가 남의 노드를 건드리지 않는 것.
// (2026-11-12 WB04-E327: 4-C CompositeMissionTransport 이식과 함께 추가)
import assert from 'node:assert/strict';
import test from 'node:test';
import { CompositeMissionTransport, CompositeTransportRole } from '../www/core/composite-mission-transport.js';
import { LocalMissionTransport } from '../www/core/local-mission-transport.js';
import { RawConnectionRegistry } from '../www/core/raw-connection-registry.js';

// 외부 시계/기록/종료/반납의 호출만 관찰하는 분배 장치이다(E324 TransportEffects 와 같은 형태). (2026-11-12 WB04-E327)
function effects() {
    const state = { now: 100, records: 0, closes: 0, releases: 0 };
    return {
        state,
        hooks: {
            now: () => { state.now += 1; return state.now; },
            record: () => { state.records += 1; },
            terminate: (connection) => { connection.flags |= 0x40; },
            close: () => { state.closes += 1; },
            release: () => { state.releases += 1; },
        },
    };
}

// 20BYTE 주소(내부 State8 주소는 내용을 해석하지 않음)이다. (2026-11-12 WB04-E327)
const ADDRESS = new Uint8Array(20);

// 조정자 장면을 만든다: 서버 권한 상자를 공유하고 로그인 처리기를 분배기에 등록한다. (2026-11-12 WB04-E327)
function makeScene({ server = true } = {}) {
    const device = effects();
    const registry = new RawConnectionRegistry();
    const players = new Uint8Array(1620);
    const serverBox = { value: server };
    const lifetime = { name: 'Client 물리' };
    const transport = CompositeMissionTransport.create(CompositeTransportRole.InternalMission, players, serverBox, device.hooks, registry, lifetime);
    transport.dispatch.register(0x27, () => {});
    return { device, registry, players, serverBox, lifetime, transport };
}

// 열린 listener 와 접속, 첫 poll 수락까지 마친 장면이다(서버 끝은 pending -1). (2026-11-12 WB04-E327)
function connectedScene() {
    const scene = makeScene();
    scene.transport.serve(6799);
    scene.transport.connect(ADDRESS, 6799);
    scene.transport.poll();
    return scene;
}

// ---------------------------------------------------------------- 생성

// 지원 역할과 필수 인자: 웹은 InternalMission 만 지원하고 나머지 역할/인자 누락은 효과 전에 거부한다. 생성만으로 listener/접속/시계 호출이 없다.
test('복합 전송: 생성 계약과 지원 역할', () => {
    const { device, transport, registry, lifetime } = makeScene();
    assert.equal(transport.role, 'InternalMission');
    assert.equal(transport.registry, registry);
    assert.equal(device.state.now, 100);
    assert.deepEqual([...registry.cleanup().transceivers].filter(Boolean), []);
    assert.equal(transport.dispatch.registration(0x87).version, 1);
    const hooks = device.hooks;
    const players = new Uint8Array(1620);
    // 지원하지 않는 역할은 웹 범위 밖 오류, 모르는 역할은 형 오류이다.
    for (const role of [CompositeTransportRole.Editor, CompositeTransportRole.LanHost, CompositeTransportRole.LanClient]) {
        assert.throws(() => CompositeMissionTransport.create(role, players, { value: true }, hooks, new RawConnectionRegistry(), lifetime), /웹 범위 밖: 복합 전송 역할/);
    }
    assert.throws(() => CompositeMissionTransport.create('Other', players, { value: true }, hooks, new RawConnectionRegistry(), lifetime), /지원하지 않는 복합 전송 역할/);
    // 실제 Client 물리/등록소/Player9 누락.
    assert.throws(() => CompositeMissionTransport.create('InternalMission', players, { value: true }, hooks, new RawConnectionRegistry(), null), /실제 Client 물리\/Registry\/Player9 누락/);
    assert.throws(() => CompositeMissionTransport.create('InternalMission', players, { value: true }, hooks, null, lifetime), /실제 Client 물리\/Registry\/Player9 누락/);
    assert.throws(() => CompositeMissionTransport.create('InternalMission', new Uint8Array(1619), { value: true }, hooks, new RawConnectionRegistry(), lifetime), /실제 Client 물리\/Registry\/Player9 누락/);
    assert.throws(() => CompositeMissionTransport.create('InternalMission', players, { value: true }, hooks, {}, lifetime), TypeError);
    assert.throws(() => CompositeMissionTransport.create('InternalMission', players, 'yes', hooks, new RawConnectionRegistry(), lifetime), TypeError);
});

// ---------------------------------------------------------------- serve/connect

// 서버 권한이 있을 때만 내부 listener 를 열고, 이미 열린 listener 는 다시 만들지 않는다(004e2430). LAN TCP listener 는 웹 범위 밖이다.
test('복합 전송: serve 는 서버 권한이 있을 때 한 번만 열고 LAN listener 는 거부한다', () => {
    const { serverBox, transport, device } = makeScene({ server: false });
    assert.throws(() => transport.serve(6799), /서버 권한 없는 내부\/LAN host listener 요청/);
    assert.equal(device.state.now, 100);
    // 서버 권한은 호출 때마다 현재 값을 읽는다.
    serverBox.value = true;
    transport.serve(6799);
    // 이미 열린 listener 는 유지하며 오류 없이 반환한다.
    transport.serve(6800);
    transport.serve(6799);
    assert.throws(() => transport.listenLan(6799), /웹 범위 밖: LAN TCP listener/);
});

// connect 는 State8 20BYTE 주소만 받아 내부 client9 를 만들고(주소 해석 없음), 문자 주소 경로도 같은 내부 끝만 연다.
test('복합 전송: connect/connectPeerText 는 내부 client9 만 만든다', () => {
    const { transport, registry } = makeScene();
    transport.serve(6799);
    assert.throws(() => transport.connect(new Uint8Array(19), 6799), RangeError);
    assert.throws(() => transport.connect(null, 6799), RangeError);
    assert.equal(transport.exists(9), false);
    transport.connect(ADDRESS, 6799);
    assert.deepEqual([transport.exists(9), transport.isOpen(9), transport.status(9)], [true, true, 1]);
    assert.equal(registry.cleanup().transceivers[9] !== 0, true);
    // 두 번째 접속은 내부 전송의 새 접속 계약이 거부한다.
    assert.throws(() => transport.connect(ADDRESS, 6799));
    const other = makeScene();
    other.transport.serve(6800);
    other.transport.connectPeerText('저장된 주소 문자열', 6800);
    assert.equal(other.transport.exists(9), true);
});

// ---------------------------------------------------------------- poll

// 첫 poll 이 내부 listener 의 서버 끝을 pending -1 에 수락하고, assign 이 같은 노드를 owner 칸으로 옮긴다.
test('복합 전송: poll 수락과 assign 재배치', () => {
    const { transport, registry } = connectedScene();
    assert.deepEqual([transport.exists(-1), transport.isOpen(-1), transport.status(-1)], [true, true, 1]);
    const server = transport.connection(-1);
    assert.ok(server);
    assert.notEqual(server, transport.connection(9));
    // 현재 노드 없는 ID 에는 재배치할 수 없다.
    assert.throws(() => transport.assign(-2, 1), /현재 accepted 노드 없는 복합 전송 재배치/);
    assert.throws(() => transport.assign(5, 1), /현재 accepted 노드 없는 복합 전송 재배치/);
    transport.assign(-1, 1);
    assert.equal(transport.connection(1), server);
    assert.equal(transport.connection(-1), null);
    assert.equal(registry.cleanup().transceivers[1] !== 0, true);
    // ID 범위 밖은 등록소 도구가 거부한다.
    assert.throws(() => transport.connection(22), RangeError);
    assert.throws(() => transport.connection(-32), RangeError);
    assert.equal(transport.address(5)[0], 2);
    assert.throws(() => transport.address(22), RangeError);
});

// poll 은 중첩 호출을 거부하고(수신 callback 안의 poll), 거부 뒤에도 다음 poll 이 가능하다. 반환은 소비한 프레임 수이다.
test('복합 전송: poll 중첩 거부와 소비 수', () => {
    const { transport } = connectedScene();
    transport.assign(-1, 1);
    let nestedError = null;
    let received = 0;
    // 서버 끝(owner 1)이 받은 0x24 에서 poll 을 다시 부른다.
    transport.dispatch.register(0x24, () => {
        received += 1;
        try { transport.poll(); } catch (error) { nestedError = error; }
    });
    const packet = Uint8Array.of(1, 0x24, 5, 0, 0);
    assert.equal(transport.send(9, packet), true);
    transport.flush();
    assert.equal(transport.poll(), 1);
    assert.equal(received, 1);
    assert.match(nestedError?.message ?? '', /복합 전송 Poll 중첩 호출/);
    // 중첩 거부가 진입 표시를 영구 봉쇄하지 않는다.
    assert.equal(transport.poll(), 0);
});

// 수신 중 오류가 나면 내부 전송의 외곽 poll 대여를 반납하고 원래 오류를 전파하며, 진입 표시도 풀린다.
test('복합 전송: 수신 callback 오류는 대여를 반납하고 그대로 전파한다', () => {
    const { transport } = connectedScene();
    transport.assign(-1, 1);
    transport.dispatch.register(0x24, () => { throw new Error('수신 처리 실패'); });
    transport.send(9, Uint8Array.of(1, 0x24, 5, 0, 0));
    transport.flush();
    assert.throws(() => transport.poll(), /수신 처리 실패/);
    // 대여가 반납됐으므로 새 poll 이 시작될 수 있다(내부 전송의 중복 beginPoll 오류가 아님).
    assert.equal(transport.poll(), 0);
});

// ---------------------------------------------------------------- 송신/flush

// 연결이 없으면 false, 헤더 4BYTE 미만은 거부, 있으면 선택된 내부 끝이 보낸다. peer 는 매 송신마다 현재 ID 를 조회한다.
test('복합 전송: send/sendMutable/flush/peer 는 현재 ID 의 실제 끝만 쓴다', () => {
    const { transport } = connectedScene();
    const packet = Uint8Array.of(1, 0x24, 5, 0, 0);
    // 없는 ID 는 자연 생략이다.
    assert.deepEqual([transport.send(7, packet), transport.sendMutable(7, Uint8Array.of(1, 0x24, 5, 0, 0))], [false, false]);
    // 있는 ID 는 헤더 길이를 확인한다.
    assert.throws(() => transport.send(9, Uint8Array.of(1, 0x24, 5)), /복합 송신의 원문 헤더 부족/);
    assert.throws(() => transport.sendMutable(9, Uint8Array.of(1, 0x24, 5)), /복합 mutable 송신의 원문 헤더 부족/);
    assert.equal(transport.send(9, packet), true);
    // pending -1 에 있는 서버 끝으로의 송신은 buffered 가 아니어서 flush 없이 상대 큐에 들어간다.
    assert.equal(transport.send(-1, packet), true);
    transport.flush();
    // peer 는 호출 시점의 현재 ID 를 다시 조회한다(없는 ID 는 조용히 생략).
    assert.equal(transport.peer(7)(packet), undefined);
    assert.equal(transport.peer(9)(packet), undefined);
    // 32비트 부호 있는 ID 로 해석한다(큰 값은 음수 pending).
    assert.equal(transport.peer(0xffffffff)(packet), undefined);
});

// ---------------------------------------------------------------- 다른 owner 의 노드

// 같은 등록소의 다른 전송이 가진 노드는 이 조정자가 분배하지 않는다: 조회/송신/flush 는 오류, 수신은 그 전송 몫이라 빈 결과, 방 종료는 건드리지 않는다.
test('복합 전송: 같은 등록소의 다른 owner 노드는 Unknown 으로 구분한다', () => {
    const scene = makeScene();
    const other = new LocalMissionTransport(new Uint8Array(1620), true, effects().hooks, scene.registry);
    other.registerLogin(() => {});
    other.open(6900);
    const otherToken = scene.registry.cleanup().transceivers[9];
    assert.ok(otherToken);
    const { transport, registry } = scene;
    assert.throws(() => transport.connection(9), /다른 owner의 복합 연결 주소 조회/);
    assert.throws(() => transport.exists(9), /다른 owner의 복합 연결 주소 조회/);
    assert.throws(() => transport.isOpen(9), /다른 owner의 복합 연결 open 조회/);
    assert.throws(() => transport.status(9), /다른 owner의 복합 연결 status 조회/);
    assert.throws(() => transport.address(9), /다른 owner의 복합 연결 주소 조회/);
    assert.throws(() => transport.send(9, Uint8Array.of(1, 0x24, 5, 0, 0)), /소유하지 않은 현재 송신 노드/);
    assert.throws(() => transport.sendMutable(9, Uint8Array.of(1, 0x24, 5, 0, 0)), /소유하지 않은 현재 mutable 송신 노드/);
    assert.throws(() => transport.flush(), /소유하지 않은 현재 flush 노드/);
    // 살아 있는 남의 노드는 수신 순회에서 빈 결과로 넘어간다(원문은 각 노드가 스스로 수신).
    assert.equal(transport.poll(), 0);
    // 방 종료는 자기 노드만 닫는다.
    transport.closeRoom();
    assert.equal(registry.cleanup().transceivers[9], otherToken);
    assert.equal(registry.owns(otherToken), true);
    assert.equal(transport.ownsConnection(other.client), false);
    assert.equal(other.hasClientConnection, true);
});

// 등록표에도 없는 별칭 토큰은 실제 오류이며, 오류 뒤에도 외곽 대여와 진입 표시가 반납된다.
test('복합 전송: 등록표에 없는 별칭 토큰은 수신에서 원인을 담아 거부한다', () => {
    const { transport, registry } = makeScene();
    registry.cleanup().transceivers[3] = 0x77777777;
    assert.throws(() => transport.poll(), /복합 전송이 소유하지 않은 현재 수신 노드: id=3 token=2004318071 역할=InternalMission/);
    registry.cleanup().transceivers[3] = 0;
    assert.equal(transport.poll(), 0);
});

// ---------------------------------------------------------------- 정리/대여

// 방 종료: 자기 정상 노드와 pending 을 원문 순서로 닫고 내부 두 끝을 마무리한다. 등록소에서 자기 노드가 사라진다.
test('복합 전송: closeRoom 은 자기 노드를 모두 닫는다', () => {
    const { transport, registry, device } = connectedScene();
    transport.assign(-1, 1);
    assert.deepEqual([transport.exists(1), transport.exists(9)], [true, true]);
    transport.closeRoom();
    assert.deepEqual([...registry.cleanup().transceivers].filter(Boolean), []);
    assert.deepEqual([...registry.cleanup().pending].filter(Boolean), []);
    // 닫힌 노드는 분배 효과의 close/release 를 거쳐 반납된다.
    assert.ok(device.state.closes >= 2 && device.state.releases >= 2);
    assert.deepEqual([transport.exists(1), transport.exists(9), transport.isOpen(9), transport.status(9)], [false, false, false, 0]);
    // 개별 종료는 같은 등록소의 원문 종료이다.
    const second = connectedScene();
    second.transport.close(9);
    assert.equal(second.transport.exists(9), false);
});

// 등록 대여는 실제 Session 전체 owner 로 만들고 콜백 활성 관문을 같은 등록표에서 읽는다.
test('복합 전송: bindSession/connectionCallbackActive 는 공유 등록표를 쓴다', () => {
    const { transport } = makeScene();
    assert.equal(transport.connectionCallbackActive(), false);
    assert.throws(() => transport.bindSession(null), TypeError);
    const owner = { name: 'Session' };
    const lease = transport.bindSession(owner);
    assert.ok(lease);
    assert.throws(() => transport.bindSession({}), /중복 대여/);
    assert.equal(transport.connectionCallbackActive(), false);
    lease.detach();
    assert.ok(transport.bindSession({ name: 'Next' }));
});

// C++ 소멸자: 자기 두 끝의 남은 별칭만 반납한다.
test('복합 전송: dispose 는 자기 두 끝의 별칭을 반납한다', () => {
    const { transport, registry } = connectedScene();
    transport.assign(-1, 1);
    const table = registry.cleanup();
    const tokens = [table.transceivers[9], table.transceivers[1]];
    assert.ok(tokens.every((token) => token && registry.owns(token)));
    transport.dispose();
    // 소멸은 자기 두 끝의 등록소 별칭/장치만 반납한다(표 칸은 Native 부모의 몫이라 건드리지 않음).
    assert.deepEqual(tokens.map((token) => registry.owns(token)), [false, false]);
});

// ---------------------------------------------------------------- 변형 확인으로 보강한 경계 (WB04-E327)

// 생성 인자의 정확한 크기·등록소 형식과 주소 길이: Player9 는 1620BYTE 정확히, 주소는 20BYTE 정확히이다.
test('복합 전송: Player9·등록소·State8 주소의 정확한 형식', () => {
    const device = effects();
    const lifetime = {};
    assert.throws(() => CompositeMissionTransport.create('InternalMission', new Uint8Array(1621), { value: true }, device.hooks, new RawConnectionRegistry(), lifetime), /복합 전송의 실제 Client 물리\/Registry\/Player9 누락/);
    assert.throws(() => CompositeMissionTransport.create('InternalMission', new Uint8Array(1620), { value: true }, device.hooks, {}, lifetime), /복합 전송의 연결 등록소 형식 오류/);
    const { transport } = makeScene();
    transport.serve(6799);
    assert.throws(() => transport.connect(new Uint8Array(21), 6799), RangeError);
    assert.equal(transport.exists(9), false);
});

// 연결이 없는 ID 는 패킷 헤더가 모자라도 자연 생략(false)이고, 있는 ID 에서만 헤더를 확인한다. send 와 sendMutable 은 observer(1) 호출 여부로 구별된다.
test('복합 전송: 없는 연결의 짧은 패킷 생략과 send/sendMutable 의 observer 차이', () => {
    const { transport } = connectedScene();
    assert.deepEqual([transport.send(7, Uint8Array.of(1)), transport.sendMutable(7, Uint8Array.of(1))], [false, false]);
    const observed = [];
    transport.dispatch.observe(0x24, (sending) => { observed.push(sending); });
    const packet = Uint8Array.of(1, 0x24, 5, 0, 0);
    assert.equal(transport.send(9, packet), true);
    // 길이가 확정된 writer 는 observer 를 부르지 않는다.
    assert.deepEqual(observed, []);
    assert.equal(transport.sendMutable(9, Uint8Array.of(1, 0x24, 5, 0, 0)), true);
    // 원시 작성기는 연결이 선택된 뒤 observer(1) 를 한 번 부른다.
    assert.deepEqual(observed, [1]);
});

// poll 은 pending -1 에서 -31 까지 각 한 번씩 방문하고 소비한 프레임을 센다. 서버 끝이 아직 pending 이면 수신도 pending 에서 일어난다.
test('복합 전송: pending 수신 계수와 -31 번 칸 방문', () => {
    const { transport, registry } = connectedScene();
    let received = 0;
    transport.dispatch.register(0x24, () => { received += 1; });
    assert.equal(transport.send(9, Uint8Array.of(1, 0x24, 5, 0, 0)), true);
    transport.flush();
    // 서버 끝은 아직 pending -1 이며 이 poll 에서 한 프레임을 소비한다.
    assert.equal(transport.poll(), 1);
    assert.equal(received, 1);
    // 마지막 pending 칸(-31)도 방문한다: 등록표에 없는 별칭이 있으면 거기서 오류가 난다.
    registry.cleanup().pending[31] = 0x77777777;
    assert.throws(() => transport.poll(), /id=-31 token=/);
    registry.cleanup().pending[31] = 0;
    assert.equal(transport.poll(), 0);
});

// 같은 정상 ID 에 프레임이 여러 개 쌓여 있으면 한 poll 에서 모두 소비한다(again 이 거짓일 때까지).
test('복합 전송: 한 ID 의 연속 프레임은 한 poll 에서 모두 소비한다', () => {
    const { transport } = connectedScene();
    transport.assign(-1, 1);
    let received = 0;
    transport.dispatch.register(0x24, () => { received += 1; });
    // flush 마다 하나의 집합 프레임이 만들어지므로 두 번 flush 해 프레임 두 개를 쌓는다.
    transport.send(9, Uint8Array.of(1, 0x24, 5, 0, 0));
    transport.flush();
    transport.send(9, Uint8Array.of(1, 0x24, 5, 0, 0));
    transport.flush();
    assert.equal(transport.poll(), 2);
    assert.equal(received, 2);
    assert.equal(transport.poll(), 0);
});

// 존재하지만 닫힘 표시(0x40)가 있는 끝은 열린 상태도 완료(1)도 아니다.
test('복합 전송: 닫힌 끝의 isOpen/status', () => {
    const { transport } = connectedScene();
    const client = transport.connection(9);
    client.flags |= 0x40;
    assert.deepEqual([transport.exists(9), transport.isOpen(9), transport.status(9)], [true, false, 0]);
    client.flags &= ~0x40;
    assert.deepEqual([transport.isOpen(9), transport.status(9)], [true, 1]);
});

// 수신 callback 이 실행되는 동안 등록 대여의 콜백 활성 관문이 켜지고, 끝나면 꺼진다. ownsConnection 은 자기 노드만 식별한다.
test('복합 전송: 콜백 활성 관문과 노드 소유 식별', () => {
    const { transport } = connectedScene();
    transport.assign(-1, 1);
    const lease = transport.bindSession({ name: 'Session' });
    let during = null;
    transport.dispatch.register(0x24, () => { during = transport.connectionCallbackActive(); });
    transport.send(9, Uint8Array.of(1, 0x24, 5, 0, 0));
    transport.flush();
    assert.equal(transport.poll(), 1);
    assert.equal(during, true);
    assert.equal(transport.connectionCallbackActive(), false);
    lease.detach();
    // 자기 두 끝은 소유하고 다른 전송의 노드는 소유하지 않는다.
    assert.equal(transport.ownsConnection(transport.connection(9)), true);
    assert.equal(transport.ownsConnection(transport.connection(1)), true);
    assert.equal(transport.ownsConnection({}), false);
});

// 방 종료는 내부 전송의 열림 표시도 내린다: 종료 뒤 같은 조정자에서 다시 열고 접속하면 새 연결이 만들어진다(미션 재진입).
test('복합 전송: closeRoom 뒤 같은 조정자에서 다시 열고 접속할 수 있다', () => {
    const { transport } = connectedScene();
    transport.assign(-1, 1);
    transport.closeRoom();
    assert.deepEqual([transport.exists(1), transport.exists(9)], [false, false]);
    // 열림 표시가 내려갔으므로 serve 가 새 listener 를 연다(열려 있다고 보고 건너뛰지 않음).
    transport.serve(6801);
    transport.connect(ADDRESS, 6801);
    assert.deepEqual([transport.exists(9), transport.isOpen(9), transport.status(9)], [true, true, 1]);
    transport.poll();
    assert.deepEqual([transport.exists(-1), transport.isOpen(-1)], [true, true]);
});
