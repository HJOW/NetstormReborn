// Session 공개 AI 관문을 실제 본체 저장소와 독립 owner 계약으로 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { RawAiBrainMemory } from '../www/core/raw-ai-brain.js';
import { createCampaignAi, crankCampaignAiMoney, setCampaignAiParameter, prepareCampaignAi, initializeCampaignAi, makeCampaignAi } from '../www/core/raw-campaign-session-ai-public.js';

// 같은 9 Player 기록과 실제 본체의 현재 토큰을 제공한다.
function scene() {
    const memory = new RawAiBrainMemory(0x10000, (bytes) => bytes.fill(0)), players = new Uint8Array(9 * 180), token = memory.allocate(0xb08);
    const view = new DataView(players.buffer); view.setUint32(180 + 0xb0, token, true);
    const calls = [], owner = { loaded: true, players, aiMemory: memory,
        participation: { brain: (id) => view.getUint32(id * 180 + 0xb0, true) },
        aiCreate: { create: (id, settings) => { calls.push([id, settings]); return token; } },
        aiPreparation: { update: (value) => calls.push(['prepare', value]) },
        aiStrategySetup: { initialize: (value) => { calls.push(['initialize', value]); const raw = memory.bytes(value); new DataView(raw.buffer, raw.byteOffset, raw.byteLength).setUint32(0x258, 1, true); } },
    };
    return { owner, memory, token, calls, view };
}

// 없는 본체는 false이고 생성·준비·초기화는 같은 현재 토큰을 사용한다.
test('Session 공개 AI: 설치 관문과 실제 본체 전달', () => {
    const { owner, memory, token, calls, view } = scene();
    try {
        const settings = new Uint8Array(600);
        assert.equal(createCampaignAi(owner, 1, settings), token); assert.equal(calls[0][1], settings);
        assert.equal(prepareCampaignAi(owner, 1), true); assert.equal(initializeCampaignAi(owner, 1), true);
        assert.equal(initializeCampaignAi(owner, 1), true); assert.deepEqual(calls.slice(1), [['prepare', token], ['initialize', token]]);
        view.setUint32(180 + 0xb0, 0, true); assert.equal(prepareCampaignAi(owner, 1), false); assert.equal(initializeCampaignAi(owner, 1), false);
        assert.throws(() => createCampaignAi(owner, 9, settings), /owner1\.\.8/);
        owner.loaded = false; assert.throws(() => prepareCampaignAi(owner, 1), /설치/);
    } finally { memory.free(token); }
});

// 직접 명령은 공유 Player에서 본체를 읽고 SP 사본 대신 해당 본체만 쓴다.
test('Session 공개 AI: 직접 명령과 owner 0 선행 반환', () => {
    const { owner, memory, token } = scene();
    try {
        owner.loaded = false; setCampaignAiParameter(owner, 0); assert.throws(() => setCampaignAiParameter(owner, 1), /미설치/); assert.throws(() => crankCampaignAiMoney(owner), /미설치/);
        owner.loaded = true; crankCampaignAiMoney(owner); setCampaignAiParameter(owner, (12 << 16) | (2 << 4) | 1);
        const raw = memory.bytes(token), body = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
        assert.equal(body.getUint32(0x2d8, true), 30000); assert.equal(body.getUint32(0x230, true), 12);
    } finally { memory.free(token); }
});

// 일반 AI는 전투가 아닐 때 입력을 요구하지 않고 전투 때 현재 SP와 기본 설정을 넘긴다.
test('Session MakeAi: 전투 관문과 현재 설정', () => {
    const { owner, memory, token, calls } = scene();
    try {
        owner.core = { ownerMode: { battle: false } }; owner.objectState = { loadingDepth: 0 }; owner.globals = { missionPost: 1 };
        owner.playerState = { battleOwner: 1, playerKnown: 1, playerType: 80 }; owner.state = { missionAiSettings: { stuffRefreshRate: 3, startingSp: 2500 } };
        makeCampaignAi(owner, 1); assert.equal(calls.length, 0);
        owner.core.ownerMode.battle = true; owner.sp = { get: () => 0x44bb8000 }; makeCampaignAi(owner, 1);
        const [id, settings] = calls[0], view = new DataView(settings.buffer);
        assert.equal(id, 1); assert.equal(settings[0x1e], 0); assert.equal(view.getUint32(0x22c, true), 1500);
    } finally { memory.free(token); }
});

// 소유자 0의 이관·깊이·AI 생성·마지막 삭제 순서를 같은 현재 저장소에서 확인한다.
test('Session MakeAi: 객체 이전과 깊이 전파', () => {
    const { owner, memory, token, calls, view } = scene();
    try {
        const slots = Array.from({ length: 3 }, () => new Uint8Array(50)), types = Array.from({ length: 188 }, () => ({ flags2: 0 }));
        const local = new Uint32Array(188), built = new Uint32Array(188), trace = [];
        slots[2][10] = 71; slots[2][34] = 1; types[71].flags2 = 0x202200; local[71] = 2; built[71] = 3;
        owner.core = { ownerMode: { battle: true }, pool: { capacity: 3, slot: (sid) => slots[sid] }, types, bridgeState: { bridgeType: 82 }, bookkeeping: { localCounts: () => local, localSecondaryCounts: () => built } };
        owner.objectState = { loadingDepth: 0 }; owner.globals = { missionPost: 1 }; owner.playerState = { battleOwner: 1, playerKnown: 1, playerType: 80 };
        owner.state = { missionAiSettings: { stuffRefreshRate: 3, startingSp: 2500 } }; view.setUint32(180 + 0x78, 0, true);
        owner.ownership = { islandNumber: () => 1, setOwner: (sid, next) => { trace.push(['owner', sid, next]); slots[sid][34] = next; } };
        owner.world = { setLoadingDepth: (depth) => trace.push(['depth', depth]), destroy: (sid, flags) => trace.push(['destroy', sid, flags]) };
        owner.aiCreate.create = (id, settings) => { trace.push(['create', id]); calls.push(settings); return token; };
        makeCampaignAi(owner, 0);
        assert.deepEqual(trace, [['owner', 2, 2], ['depth', 1], ['owner', 2, 2], ['depth', 0], ['create', 2], ['destroy', 2, 0]]);
        assert.equal(local[71], 1); assert.equal(built[71], 2); assert.equal(view.getUint32(2 * 180 + 0x78, true), 1); assert.equal(owner.objectState.loadingDepth, 0);
    } finally { memory.free(token); }
});
