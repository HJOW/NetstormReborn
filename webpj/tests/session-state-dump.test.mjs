// cpppj SessionStateDump의 순수 판번호 1 서식·SP 복호·해시 계약을 검증한다. (2026-11-19 WB04-E865)
import test from 'node:test';
import assert from 'node:assert/strict';
import { formatSessionStateDump, sessionDumpHash, decodeDumpScrambledSp } from '../www/core/session-state-dump.js';
import { GameRandom } from '../www/core/random.js';
import { ScrambledSpStore } from '../www/core/sp-store.js';

// C++ 검사와 같은 판본별 자산 슬롯·Player·동맹·Graph·TotalMade 관측값을 만든다.
function sample(edition) {
    const patch = edition === 'patch1078', pool = new Uint8Array(7 * 50), raw = pool.subarray(250, 300);
    raw[10] = 90; raw[patch ? 34 : 32] = 2;
    const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
    if (patch) view.setUint32(36, 5, true); else raw[34] = 5;
    view.setFloat32(14, 40.5, true); view.setFloat32(18, -3.25, true);
    if (patch) view.setUint32(26, 300, true); else view.setUint16(26, 300, true);
    pool[6 * 50 + 11] = 1;
    const players = new Uint8Array(9 * 180), hidden = new Uint8Array(9), alliances = new Uint32Array(81);
    players[180] = 0xab; hidden[8] = 1; alliances[10] = 1;
    const graph = Array.from({ length: 251 }, () => ({ surfaces: 0, inUse: 0, reserved: 0 }));
    graph[1] = { surfaces: 3, inUse: 1, reserved: 0 };
    const totalLocal = new Uint32Array(256), totalBuilt = new Uint32Array(256), totalGlobal = new Uint32Array(256);
    totalLocal[90] = 1; totalBuilt[90] = 2;
    return { edition, frame: 42, gameTime: 1.5, gameRandom: 4660, crtRandom: 99, world: true, kernelSize: 1, kernelCapacity: 39999,
        processes: [{ pid: 7, form: 15001, parent: 5, regular: true, event: 0x5104, count: 3, payloadBits: 1065353216, timeBits: 4628293042053316608n }],
        poolBytes: pool, poolCapacity: 7, stride: 50, freeCount: 1, firstFreeServer: 6, firstFreeClient: 0, predictableCursor: 0,
        players, hiddenPlayers: hidden, alliances, spInitialized: false, spOffset: 0, graph,
        totalCost: -5, postPopDepth: 0, productionDirty: 1, totalLocal, totalBuilt, totalGlobal };
}

// 공개 FNV 시험값은 정수 비트를 64비트로 감아야 일치한다.
test('Session 덤프 해시는 FNV-1a 64 공개 시험값과 같다', () => {
    const encode = (text) => new TextEncoder().encode(text);
    assert.equal(sessionDumpHash(encode('')), 0xcbf29ce484222325n);
    assert.equal(sessionDumpHash(encode('a')), 0xaf63dc4c8601ec8cn);
    assert.equal(sessionDumpHash(encode('foobar')), 0x85944171f73967e8n);
});

// 분산 배열에서 직접 읽은 비트는 실제 SP 저장소의 Get과 회전 전후에 같다.
test('Session 덤프 SP 복호는 저장소와 회전 전후에 같다', () => {
    const store = new ScrambledSpStore(new GameRandom(12345), () => 777);
    store.initialize();
    const values = [0, 0x3f800000, 0x41200000, 0xbf800000, 0x7fc00001, 1, 0xffffffff, 0x80000000, 0x12345678];
    // 아홉 소유자의 비트를 원본 저장소에 쓴다.
    values.forEach((value, owner) => store.set(owner, value));
    // 회전해도 같은 비트를 반환해야 한다.
    for (let round = 0; round < 3; round += 1) {
        values.forEach((value, owner) => assert.equal(decodeDumpScrambledSp(store.pools(), store.permutation(), store.offset, owner), value));
        store.rotate();
    }
    assert.throws(() => decodeDumpScrambledSp(store.pools(), store.permutation(), store.offset, 9), RangeError);
});

// 두 판본의 슬롯 폭·free 생략과 정기 프로세스·월드 장부 행을 C++ 검사와 대조한다.
test('Session 덤프 판본별 자산과 월드 행은 원본 비트를 보존한다', () => {
    // PATCH와 CD 서식의 필드 폭을 각각 확인한다.
    for (const edition of ['patch1078', 'cd1072']) {
        const input = sample(edition), result = formatSessionStateDump(input), lines = result.trimEnd().split('\n');
        assert.ok(result.startsWith(`netstorm_session_dump\t1\nedition\t${edition === 'patch1078' ? 'originals' : 'originalCD'}\nframe\t42\t4609434218613702656\nrandom\t4660\t99\nworld\t1\nkernel\t1\t39999\n`));
        assert.ok(result.includes('process\t7\t15001\t5\nregular\t7\t20740\t3\t1065353216\t4628293042053316608\n'));
        assert.ok(result.includes('object\t5\t90\t0\t2\t5\t1109524480\t3226468352\t300\n'));
        assert.equal(lines.filter((line) => line.startsWith('slot\t')).length, 6);
        assert.equal(lines.filter((line) => line.startsWith('player\t')).length, 9);
        assert.ok(result.includes('hidden_players\t000000000000000001\n'));
        assert.ok(result.includes('sp\t0\t0\t-\t-\t-\t-\t-\t-\t-\t-\t-\n'));
        assert.ok(result.includes('graph\t1\t3\t1\t0\n'));
        assert.ok(result.endsWith('totals\t-5\t0\t1\ntotalmade\t90\t1\t2\t0\nend\n'));
    }
});

// 월드가 없으면 풀·SP 입력을 읽지 않으며 잘못된 배열 크기는 거부한다.
test('Session 덤프 world 0과 입력 범위 오류', () => {
    const input = sample('patch1078');
    input.world = false; delete input.poolBytes; delete input.players;
    assert.ok(formatSessionStateDump(input).endsWith('end\n'));
    assert.ok(!formatSessionStateDump(input).includes('\npool\t'));
    const broken = sample('patch1078'); broken.poolBytes = new Uint8Array(1);
    assert.throws(() => formatSessionStateDump(broken), /풀 원문 크기/);
    const badTotals = sample('patch1078'); badTotals.totalBuilt = new Uint32Array(1);
    assert.throws(() => formatSessionStateDump(badTotals), /TotalMade/);
});
