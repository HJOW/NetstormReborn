// 실제 Session 저장소의 Login 앞단과 Zone 설치를 검사한다. 게임 실행과 유닛 행동은 수행하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { campaignOwners } from './campaign-session-support.mjs';
import { makeLocalMissionZone } from '../www/core/local-mission-transport.js';
import { RawCampaignSessionLoginFront } from '../www/core/raw-campaign-session-login-front.js';

// 원본 설정·벽시각의 호출 순서를 기록하며 실제 전송은 내부 두 끝을 사용한다.
function devices(trace) {
    return { configInt: (key, fallback) => { trace.push(key); return fallback; }, wallSeconds: () => { trace.push('시각'); return 100.5; }, log: (message) => trace.push(message) };
}

// Zone이 실제 Player 이름·Core 슬롯·권한을 변경하고 미등록 수신기의 서버 열기는 거부한다.
test('Session Login 앞단: 같은 현재 저장소에 Zone 설치와 이름 정제', () => {
    const o = campaignOwners(), trace = [], front = new RawCampaignSessionLoginFront(o, devices(trace));
    try {
        const packet = makeLocalMissionZone(); packet.storage.set(Uint8Array.of(65, 32, 63, 66, 0), 24);
        o.globals.mySubscriber = 1; front.pendingStamp = 0x12345678;
        o.core.processState.server = false; o.state.assetState.writeBoss(false);
        assert.throws(() => front.installZone(packet.bytes()), /실제 로그인 처리기/);
        assert.equal(o.core.processState.server, true); assert.equal(o.state.assetState.readBoss(), true);
        assert.equal(o.connectionActivityGlobals.activeStamp().value, 0x12345678);
        assert.equal(o.localStateGlobals.zoneWords()[0], 0x82000004); assert.equal(o.localStateGlobals.captureEnd().value, 250.5);
        assert.equal(o.playerState.battleOwner, 1); assert.equal(o.playerState.templateOwner, 1); assert.equal(o.playerState.playerKnown, 1);
        assert.equal(Buffer.from(o.players.subarray(192, 195)).toString('ascii'), 'AB\0');
        assert.equal(o.coreDataGlobals.current.buffer, o.players.buffer); assert.equal(o.coreDataGlobals.current.byteOffset, o.players.byteOffset + 180 + 0x6c);
        assert.equal(front.transport.lifetime.port, 0); assert.equal(front.transport.lifetime.hasClientConnection, false);
        assert.deepEqual(trace, ['gameServerPort', 'loginTimeout', '시각']);
    } finally { front.dispose(); o.dispose(); }
    assert.throws(() => front.installZone(makeLocalMissionZone().bytes()), /반납/);
});

// 지정 옵션은 재초기화하지 않고 외부 등록표의 정체성을 확인한다.
test('Session Login 앞단: 현재 옵션 보존과 외부 전송 계약', () => {
    const options = new Uint8Array(87).fill(0x55), o = campaignOwners({ missionOptions: options });
    const front = new RawCampaignSessionLoginFront(o, devices([]));
    try { assert.ok(options.every((value) => value === 0x55)); } finally { front.dispose(); o.dispose(); }
    const next = campaignOwners();
    try { assert.throws(() => new RawCampaignSessionLoginFront(next, { transport: {} }), /수명\/등록표\/반납/); assert.equal(next.battleOptions[1], 2); } finally { next.dispose(); }
});

// 선택 장치가 빠지면 실제 선행 쓰기를 유지하고 성공으로 바꾸지 않는다.
test('Session Login 앞단: Zone 시계 누락의 선행 효과', () => {
    const o = campaignOwners(), front = new RawCampaignSessionLoginFront(o, { configInt: (_key, fallback) => fallback });
    try {
        o.core.processState.server = false; o.state.assetState.writeBoss(false);
        assert.throws(() => front.installZone(makeLocalMissionZone().bytes()), /wallSeconds/);
        assert.equal(o.core.processState.server, true); assert.equal(o.state.assetState.readBoss(), true);
        assert.equal(o.localStateGlobals.zoneWords()[0], 0x82000004); assert.equal(front.transport.lifetime.port, 0);
    } finally { front.dispose(); o.dispose(); }
});

// 정제 뒤 빈 이름의 대체는 원본 string_view 길이 안에서만 허용한다.
test('Session Login 앞단: 빈 이름은 원본 보기 길이 부족을 보존', () => {
    const o = campaignOwners(), front = new RawCampaignSessionLoginFront(o, devices([]));
    try {
        const packet = makeLocalMissionZone(); packet.storage.set(Uint8Array.of(63, 0), 24);
        assert.throws(() => front.installZone(packet.bytes()), /빈 이름 대체 범위/);
        assert.equal(o.players[192], 0); assert.equal(front.transport.lifetime.port, 0);
    } finally { front.dispose(); o.dispose(); }
});
