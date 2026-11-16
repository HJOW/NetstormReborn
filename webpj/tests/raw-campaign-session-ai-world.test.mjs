// 같은 실제 Session의 AI 객체 통지/World 등록·준비/전략/경제 결선을 검사한다(WB04-E624).
// Load·자연 플레이/포획/희생/승리와 공중3종 새 전용 생성/생산/행동은 실행하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { campaignOwners } from './campaign-session-support.mjs';
import { get, put, floatToBits } from './raw-support.mjs';

// 실제 외부 채널과 로그만 기록하며 게임 본체 계산을 대체하지 않는다.
function owner(extra = {}) {
    const trace = []; const o = campaignOwners({ missionOutcome: { demo: () => 0 }, player: { log: (...args) => trace.push(['log', ...args]) },
        ...extra, ai: { allocationBytes: (bytes) => bytes.fill(0), defaultName: () => 'World probe',
            participation: { clearChat: (...args) => trace.push(['clear', ...args]), addChat: (...args) => trace.push(['add', ...args]), sendAlliance: (...args) => trace.push(['send', ...args]) }, ...extra.ai } });
    try { o.connectWorldInstall({ sections: Array.from({ length: 35 }, () => new Uint8Array(0)) }); o.connectOutcome(); o.connectAiBrains(); return { o, trace }; }
    catch (error) { o.dispose(); throw error; }
}
// 명시600BYTE AI 입력이며 실제 Player152 생성/초기 갱신은 실제 부모가 처리한다.
function brain(o, id = 2) {
    o.playerState.playerKnown = 1; const settings = new Uint8Array(600); put(settings, 0x22c, 500); put(settings, 0x240, 1);
    return o.aiCreate.create(id, settings);
}

test('Session AI 월드 조립: 선행·반복·반납과 동일 Pool/난수·생성 시 본체 할당0', () => {
    const p = campaignOwners();
    try { assert.throws(() => p.connectAiWorld(), /초기화 전/); } finally { p.dispose(); }
    const { o } = owner();
    try {
        const bytes = o.core.pool.bytes(); const seed = o.random.state; o.connectAiWorld();
        assert.equal(o.aiWorldModules.ready, true); assert.equal(o.aiObjects.pool, o.core.pool); assert.equal(o.aiPreparation.pool, o.core.pool);
        assert.deepEqual(o.core.pool.bytes(), bytes); assert.equal(o.random.state, seed); assert.deepEqual(o.aiMemory.allocations, []);
        assert.equal(o.loaded, false); assert.throws(() => o.connectAiWorld(), /반복/); assert.throws(() => o.gameplayGlobals.requireSession(), /미연결/);
    } finally { o.dispose(); }
    assert.throws(() => o.connectAiWorld(), /반납/); assert.throws(() => o.aiWorldModules.carrierTargets(5), /반납/);
});

test('Session AI 생성/삭제 통지: 실제 Player 생성/Pop·공통 삭제 전처리가 같은 현재 brain 통계를 바꾼다', () => {
    const { o } = owner();
    try {
        o.connectAiWorld(); const token = brain(o); const parent = o.player.technology(2); const raw = o.aiMemory.bytes(token);
        assert.ok(parent); assert.equal(get(raw, 0x6d4 + 152 * 4), 1); assert.equal(o.core.pool.slot(parent)[34], 2);
        // Player 가상 삭제 전체는 이 단위에서 가장하지 않는다. 실제 공통 Pre/Post 몸체만 직접 대조한다.
        o.world.commonDeletion.preDestroy(parent, 0); assert.equal(get(raw, 0x6d4 + 152 * 4), 0); o.world.commonDeletion.postDestroy(parent, 0);
        assert.equal(o.aiWallet.balance(2), 500);
    } finally { o.dispose(); }
});

test('Session AI 장치: 현재 경로 없음·원문 점 거리·SP 비트·현재 UI 잠금/진단', () => {
    const { o } = owner();
    try {
        o.connectAiWorld(); const sid = o.world.create(152); const h = o.aiWorldModules.preparationHooks;
        assert.deepEqual(o.aiWorldModules.carrierTargets(sid), [0, 0]);
        assert.equal(h.pointDistance([1, 2], [4, 6]), 5);
        o.sp.set(2, floatToBits(87.5)); assert.equal(h.storedSp(2), 87.5);
        o.cursorDrop.lockDepth = 0; assert.equal(h.knowledgeOpen(), true); o.cursorDrop.lockDepth = 1; assert.equal(h.knowledgeOpen(), false);
        assert.throws(() => h.blockingGumpOpen(), /차단 Gump/); assert.throws(() => h.missingEnemyAssertion(), /적 목표 진단/);
        assert.throws(() => o.aiWorldModules.registrationHooks.missingSurface(7), /표면 진단/);
        const calls = []; o.services.ai.assertion = (text) => calls.push(text); o.aiWorldModules.registrationHooks.missingSurface(7);
        assert.deepEqual(calls, ['간헐천 표면 누락 SID=7']);
    } finally { o.dispose(); }
});

test('Session AI 로그: 현재 b00 관문과 실제 종류29·TYPE 이름·signed owner/간헐천 번호', () => {
    const { o, trace } = owner();
    try {
        o.connectAiWorld(); const token = brain(o); trace.length = 0; const raw = o.aiMemory.bytes(token); const objects = o.aiWorldModules.objectsHooks;
        const log = o.services.player.log; o.services.player.log = undefined;
        objects.log(token, { kind: 0, type: 85, x: 1, y: 2 }); put(raw, 0xb00, 1); assert.throws(() => objects.log(token, { kind: 0, type: 85, x: 1, y: 2 }), /생성\/삭제/); o.services.player.log = log;
        objects.log(token, { kind: 0, type: 85, x: -3, y: 7 }); o.aiWorldModules.registrationHooks.fartherGeyser(token, 0xffffffff);
        assert.deepEqual(trace, [['log', 29, `AI2: Built ${o.core.types[85].name} at (-3,7)\n`], ['log', 29, 'AI2: Not closer to geyser -1.\n\n']]);
        assert.throws(() => objects.distanceSquared(0, 65536), /SID 범위/);
    } finally { o.dispose(); }
});

test('Session AI 경제·준비: 같은 본체/분산 SP와 강제 적·미준비 전략의 자연 반환', () => {
    const { o } = owner();
    try {
        o.connectAiWorld(); const token = brain(o); const raw = o.aiMemory.bytes(token);
        o.aiPreparation.selectEnemy(token); assert.equal(get(raw, 0x264), 1);
        o.aiEconomy.reconcile(token); assert.equal(o.sp.get(2), floatToBits(500));
        const bytes = raw.slice(); o.aiStrategySetup.initialize(token); assert.deepEqual(raw, bytes);
    } finally { o.dispose(); }
});

test('Session AI 통지 반납: 살아 있는 UI 토큰을 공유하고 몸체가 필요한 때만 미연결 오류', () => {
    const { o } = owner();
    try {
        o.connectAiWorld(); const current = o.gameplayGlobals.selection; current.mouseType = 0; o.aiWorldModules.recheckSelection();
        const calls = []; const link = { selection: { recheckMouse: (find) => calls.push(typeof find) } }; o.activeSelection = new WeakRef(link);
        o.aiWorldModules.recheckSelection(); assert.deepEqual(calls, ['function']); o.activeSelection = null;
        const h = o.aiWorldModules.objectsHooks; o.aiWorldModules.dispose(); assert.throws(() => h.carrierTargets(5), /반납/);
    } finally { o.dispose(); }
});
