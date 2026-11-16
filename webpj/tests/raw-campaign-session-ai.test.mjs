// 같은 실제 Session owner의 승패 입력과 AI 참여/본체/초기 갱신·지갑 결선을 검사한다(WB04-E623).
// 전체 Load·자연 플레이/포획/희생/승리·공중3종 생성/생산/행동은 실행하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { campaignOwners } from './campaign-session-support.mjs';
import { get, put, floatToBits } from './raw-support.mjs';

// 원문 생성자의 실제 논리 Template를 선택하되 Load하지 않는다.
function install(o) { o.connectWorldInstall({ sections: Array.from({ length: 35 }, () => new Uint8Array(0)) }); o.connectOutcome(); }
// 원본 helper의 명시600BYTE 입력이다. 공중3종이나 유닛 생성 명령은 포함하지 않는다.
function settings() { const bytes = new Uint8Array(600); put(bytes, 0x22c, 500); put(bytes, 0x220, floatToBits(10)); put(bytes, 0x234, 4); return bytes; }
// 기술 부모 Form152를 실제 Pool에 만들고 검사 입력 Player의 기술 포인터로 연결한다. Player Pop/미션 시작이 아니다.
function technology(o, owner) { const sid = o.world.create(152); o.core.pool.allocated(sid)[34] = owner; put(o.players, owner * 180 + 0x9c, sid); return sid; }
// 미선택 외부 효과를 성공으로 대체하지 않는다.
function forbidden() { throw new Error('미선택 실제 외부 효과'); }
// 실제 PlayerKnown 상태의 동맹 생성이 요청한 외부 채널/전송을 기록한다. 본체나 동맹 계산을 대신하지 않는다.
function channels(trace) { return { clearChat: (...args) => trace.push(['clear', ...args]), addChat: (...args) => trace.push(['add', ...args]), sendAlliance: (...args) => trace.push(['send', ...args]) }; }

test('Session 승패/AI 조립: 선행·반복·반납, 동일 Pool·난수·수명과 외곽 미공개', () => {
    const o = campaignOwners();
    try {
        assert.throws(() => o.connectAiBrains(), /초기화 전/); assert.throws(() => o.connectOutcome(), /초기화 전/);
        const bytes = o.core.pool.bytes(); const seed = o.random.state; install(o); o.connectAiBrains();
        assert.equal(o.aiModules.outcomeReady, true); assert.equal(o.aiModules.brainsReady, true); assert.equal(o.regionOwner.pool, o.core.pool); assert.equal(o.outcome.pool, o.core.pool); assert.equal(o.aiConstructor.pool, o.core.pool);
        assert.equal(o.loaded, false); assert.deepEqual(o.core.pool.bytes(), bytes); assert.equal(o.random.state, seed); assert.deepEqual(o.aiMemory.allocations, []);
        assert.throws(() => o.connectOutcome(), /반복/); assert.throws(() => o.connectAiBrains(), /반복/); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/);
    } finally { o.dispose(); }
    assert.throws(() => o.connectOutcome(), /반납/); assert.throws(() => o.connectAiBrains(), /반납/);
});

test('Session 승패 입력: 현재 hash0/TYPE·사제 상태와 값 복사한 UI, 실제 사건은 실행하지 않는다', () => {
    const events = []; let demo = 0; let blocked = 1;
    const ui = { demo: () => demo, tellBlocked: () => blocked, section: (name) => { events.push(name); return true; }, closeTransient: () => events.push('close'), onSection: () => events.push('notify') };
    const o = campaignOwners({ missionOutcome: ui });
    try {
        install(o); ui.section = forbidden; const land = o.world.create(o.terrainType); o.core.pool.allocated(land)[34] = 3; o.core.hash.view(0)[6 * 256 + 5] = land;
        assert.equal(o.regionOwner.atCell(5, 6), 3); o.terrainType = 1; assert.equal(o.regionOwner.atCell(5, 6), 0); o.terrainType = o.core.pool.slot(land)[10];
        const latch = { probe: 0 }; assert.equal(o.outcome.event(latch, 'probe', true, 'AssemblyProbe'), false); assert.deepEqual(events, []);
        blocked = 0; demo = 1; assert.equal(o.outcome.event(latch, 'probe', true, 'AssemblyProbe'), true); assert.deepEqual(events, ['DemoAssemblyProbe', 'close', 'notify']); assert.equal(latch.probe, 1);
        o.players[2 * 180 + 0x50] = 0x22; o.priestState.priests.currentOwnerState()[2] = 0; assert.equal(o.outcome.priestDead(2), false);
        o.priestState.priests.currentOwnerState()[2] = 2; assert.equal(o.outcome.priestDead(2), true);
    } finally { o.dispose(); }
    const missingUi = {}; const p = campaignOwners({ missionOutcome: missingUi });
    try { install(p); assert.deepEqual(missingUi, {}); assert.throws(() => p.outcome.event({ p: 0 }, 'p', true, 'AssemblyProbe'), /demo 미연결/); }
    finally { p.dispose(); }
});

test('Session 참여 전송: 캡처한 외부 함수와 후설치 현재 Login 함수, 실패 접두 보존', () => {
    const trace = []; const participation = { assignConnection: (...args) => trace.push(['external', ...args]), sendAlliance: (...args) => trace.push(['alliance', ...args]) };
    const o = campaignOwners({ ai: { participation, localTransport: () => false } });
    try {
        install(o); o.connectAiBrains(); participation.assignConnection = forbidden; o.playerState.playerKnown = 0;
        assert.equal(o.participation.checkLogin(7, 2), true); o.connectionAssign = (...args) => trace.push(['login', ...args]); assert.equal(o.participation.checkLogin(8, 3), true);
        o.participation.addAlly(2, 1); o.connectionAlliance = (...args) => trace.push(['current', ...args]); o.participation.addAlly(3, 1);
        assert.deepEqual(trace, [['external', 7, 2], ['login', 8, 3], ['alliance', 9, 2, 6], ['current', 9, 3, 10]]);
        o.services.ai.localTransport = undefined; assert.throws(() => o.participation.checkLogin(9, 4), /로컬 전송 조회/); assert.equal(get(o.players, 4 * 180 + 0x50) & 3, 3);
    } finally { o.dispose(); }
});

test('Session AI 본체: 실제18블록/현재 기술 부모·HP 지갑·Player 포인터와 소멸 반납', () => {
    const observed = []; const chat = []; const o = campaignOwners({ missionOutcome: { demo: () => 0 }, ai: { allocationBytes: (bytes) => bytes.fill(0), defaultName: () => 'Current AI', participation: { ...channels(chat), destructBrain: forbidden, freeBrain: forbidden } } });
    let token;
    try {
        install(o); o.connectAiBrains(); o.playerState.playerKnown = 1; technology(o, 2); o.globals.missionPost = true;
        const input = settings(); token = o.aiCreate.create(2, input); assert.equal(o.participation.brain(2), token); assert.equal(o.aiMemory.allocations.length, 18); assert.equal(o.aiWallet.balance(2), 500);
        assert.equal(Buffer.from(o.players.subarray(2 * 180 + 12, 2 * 180 + 22)).toString(), 'Current AI');
        o.hp.creditWallet(2, 7); assert.equal(o.aiWallet.balance(2), 507); o.core.ownerMode.challenge = true; o.hp.creditWallet(2, 20); assert.equal(get(o.aiMemory.bytes(token), 0x2d8), 507); o.core.ownerMode.challenge = false;
        o.services.missionOutcome.demo = () => 1; o.hp.creditWallet(2, 0); assert.equal(o.aiWallet.balance(2), 1190);
        assert.equal(o.aiCreate.create(2, input), token); assert.equal(o.aiMemory.allocations.length, 18);
        o.services.ai.changeColor = (owner, color) => observed.push([owner, color, o.core.bridgeState.ownerColors[owner]]); put(input, 0x250, 4); assert.equal(o.aiCreate.create(2, input), token); assert.deepEqual(observed, [[2, 4, 4]]);
    } finally { o.dispose(); }
    assert.equal(o.aiMemory.contains(token), false); assert.equal(o.aiMemory.frees.length, 18); assert.equal(get(o.players, 2 * 180 + 0xb0), 0);
});

test('Session AI 난이도: 생성 전 설정 동맹/현재 Player 마스크·데모·적 판정과 입력 불변', () => {
    const calls = []; const chat = []; let demo = 0; const o = campaignOwners({ missionOutcome: { demo: () => demo }, ai: { allocationBytes: (bytes) => bytes.fill(0), defaultName: () => 'Difficulty probe', participation: channels(chat), difficulty: (owner, enemy) => { calls.push([owner, enemy]); return enemy ? 1 : 2; } } });
    try {
        install(o); o.connectAiBrains(); o.playerState.playerKnown = 1; o.playerState.battleOwner = 1;
        // 실제 brain만 생성하며 기술 부모는 비어 있다. 생산/공중3종/전투 행동은 호출하지 않는다.
        for (const owner of [2, 3, 4, 5]) technology(o, owner);
        const input = settings(); const copy = input.slice(); let token = o.aiCreate.create(2, input); assert.equal(get(o.aiMemory.bytes(token), 0x220), floatToBits(40)); assert.equal(get(o.aiMemory.bytes(token), 0x234), 2); assert.deepEqual(input, copy);
        put(input, 0x23c, 2); token = o.aiCreate.create(3, input); assert.equal(get(o.aiMemory.bytes(token), 0x220), floatToBits(10));
        put(input, 0x23c, 0); put(o.players, 180 + 0x54, 2 | (1 << 4)); o.aiCreate.create(4, input);
        demo = 1; token = o.aiCreate.create(5, input); assert.equal(get(o.aiMemory.bytes(token), 0x220), floatToBits(10)); assert.deepEqual(calls, [[2, true], [3, false], [4, false]]);
    } finally { o.dispose(); }
});

test('Session AI 초기 실패: 이름 조회는 선택 때 오류, 이미 부착한 실제 본체를 소멸자가 반납', () => {
    const o = campaignOwners({ ai: { allocationBytes: (bytes) => bytes.fill(0) } }); let token;
    try {
        install(o); o.connectAiBrains(); o.playerState.playerKnown = 0; o.globals.missionPost = true;
        assert.throws(() => o.aiCreate.create(2, settings()), /defaultAiName/); token = o.participation.brain(2); assert.ok(token); assert.equal(o.aiMemory.contains(token), true);
    } finally { o.dispose(); }
    assert.equal(o.aiMemory.contains(token), false); assert.equal(o.aiMemory.allocations.length, o.aiMemory.frees.length);
});
