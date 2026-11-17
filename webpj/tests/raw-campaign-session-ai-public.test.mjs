// Session 공개 AI 관문을 실제 본체 저장소와 독립 owner 계약으로 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { RawAiBrainMemory } from '../www/core/raw-ai-brain.js';
import { createCampaignAi, crankCampaignAiMoney, setCampaignAiParameter, prepareCampaignAi, initializeCampaignAi } from '../www/core/raw-campaign-session-ai-public.js';

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
