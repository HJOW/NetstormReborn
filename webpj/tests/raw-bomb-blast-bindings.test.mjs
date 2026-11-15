// RawBombBlastBindings 전체를 원본41e3d0의32행과 같은 실제 World/피격/좌표 계약으로 대조한다(WB04-E583).
// 실제 게임·주문 시전·공중3종 전용 생성/행동·새 원격MP 실행은 하지 않는다.
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits } from './raw-support.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { GameAssets } from '../www/assets/game-assets.js';
import { SoundList } from '../www/audio/sound-list.js';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { RawCampaignSessionOwners } from '../www/core/raw-campaign-session-owners.js';
import { RawBombTargets } from '../www/core/raw-bomb-targets.js';
import { RawBombBlast } from '../www/core/raw-bomb-blast.js';
import { drawDevastationAdditional, makeCampaignBombBlastHooks } from '../www/core/raw-bomb-blast-bindings.js';

// 이미 저장된 실제 x86 관측이다. 없으면 읽기 단계에서 실패하며 새 관측을 생성하지 않는다.
const rows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/devastation-draw-x86.tsv'));
const data = realCaptureAssets(); const game = new GameAssets(data.table, data.database);
// 선택하지 않은 외부 소리/VFS 효과는 명시 오류다.
function forbidden() { throw new Error('선택되지 않은 외부 장치'); }
// 실제 owner의 같은 World/Player/SP/사제/Hit까지만 조립한다. 전체 Session 대체 객체는 만들지 않는다.
function scene() {
    const owner = new RawCampaignSessionOwners(game, { read: forbidden }, 'thewarbegins', { unixSeconds: () => 1700000000, soundNames: new SoundList(), sound: { wallSeconds: () => 0, play: forbidden, playOnceAt: forbidden, playLoopAt: forbidden, stop: forbidden, isPlaying: forbidden } });
    owner.initializePlayersAndSp(); owner.connectPlayerTablesAndPackets(); owner.connectIslandsAndPriests(); owner.connectPlayersAndOwnership(); owner.connectBodiesAndTypeTables(); owner.connectGunAndCursorViews(); owner.connectHitBodies();
    const ranges = new Float32Array(188); const coordinates = new RawBombTargets(owner.core.pool, owner.core.hash, owner.core.types, ranges, { boss: true }, {});
    return { owner, ranges, coordinates, hooks: makeCampaignBombBlastHooks(owner.world, owner.hits, coordinates) };
}
// 실제 자산 생성자에 고정 수치 입력만 쓴다. Pop/시전/교전은 대신하지 않는다.
function actor(o, type, owner = 2, hp = 10000) { const sid = o.world.create(type, 2); const raw = o.core.pool.allocated(sid); raw[34] = owner; put(raw, 26, hp); put(raw, 14, floatToBits(20)); put(raw, 18, floatToBits(20)); return sid; }

test('Devastation 추가 그림: 원본32입력의 TYPE/프레임·좌표 전체 DWORD와53/64 결과가 같다', () => {
    const pool = new SidPool(24000, { server: true }); const raw = pool.physical(50); put(raw, 0, 0x502370); raw[10] = 168; assert.equal(rows.length, 32);
    // 결과는 저장된 원본 두 열만 사용하며 제품 구현으로 기대 trace를 생성하지 않는다.
    for (const row of rows) {
        const input = row[0].split(',').map(Number); assert.equal(input.length, 4); put(raw, 36, input[0]); let trace = '';
        drawDevastationAdditional(pool, 50, input[1] | 0, input[2] | 0, (type, frame, x, y) => { assert.equal(type, 168); trace = `draw:${input[3]}:${frame >>> 0}:${x >>> 0}:${y >>> 0}`; });
        assert.equal(trace, row[1]); assert.equal(trace, row[2]);
    }
});

test('Devastation 추가 그림: 실제16VT·현재 TYPE/signed 프레임과 free/dead 무필터·미지원 관문', () => {
    const pool = new SidPool(24000); const raw = pool.physical(50); const tables = [0x503188, 0x5026a0, 0x502778, 0x502840, 0x502908, 0x502b70, 0x5029d0, 0x502aa8, 0x5030c0, 0x502448, 0x502370, 0x502510, 0x502ff8, 0x5025d8, 0x502e68, 0x502f30];
    // 자격 표만 바꾸며 실제 주문/유닛을 생성하거나 효과를 실행하지 않는다.
    for (let index = 0; index < tables.length; index += 1) { put(raw, 0, tables[index]); raw[10] = 100 + index; raw[11] = 3; put(raw, 36, -1); const calls = []; drawDevastationAdditional(pool, 50, -12, 34, (...args) => calls.push(args)); assert.deepEqual(calls, [[100 + index, -1, -12, 34]]); }
    assert.throws(() => drawDevastationAdditional(pool, 50, 0, 0, null), /표\/소비자/); put(raw, 0, 0x12345678); assert.throws(() => drawDevastationAdditional(pool, 50, 0, 0, forbidden), /표\/소비자/);
});

test('폭발 바인딩: 같은 실제 World/Hit/좌표 Pool을 요구하며 다른 소유자를 거부한다', () => {
    const a = scene(); const b = scene();
    try { assert.throws(() => makeCampaignBombBlastHooks(a.owner.world, b.owner.hits, a.coordinates), /Pool 불일치/); assert.throws(() => makeCampaignBombBlastHooks(a.owner.world, a.owner.hits, b.coordinates), /Pool 불일치/); } finally { a.owner.dispose(); b.owner.dispose(); }
});

test('폭발 반경: 실제502370 가상 표와 현재 좌표 반경 getter를 사용한다', () => {
    const s = scene(); const o = s.owner;
    try { const parent = actor(o, 168, 3); s.ranges[168] = 9.75; assert.equal(s.hooks.radius(parent, 0x502370), 9); s.ranges[168] = 12.25; assert.equal(s.hooks.radius(parent, 0x502370), 12); assert.throws(() => s.hooks.radius(parent, 0x502448), /반경 가상 표/); } finally { o.dispose(); }
});

test('폭발 피해: 기존 실제 가상98 분배로 같은 HP를 변경하고 무시하는 table 인자를 자격 관문으로 쓰지 않는다', () => {
    const s = scene(); const o = s.owner;
    try { const parent = actor(o, 168, 3); const victim = actor(o, 75); s.hooks.damage(victim, 0, 7, 3, parent); assert.equal(get(o.core.pool.slot(victim), 26), 9993); s.hooks.damage(victim, 0xffffffff, 9, 3, parent); assert.equal(get(o.core.pool.slot(victim), 26), 9984); } finally { o.dispose(); }
});

test('폭발 지면 효과: 실제 공유 Game 난수와 같은 Hit Impact의 충돌 억제를 사용한다', () => {
    const s = scene(); const o = s.owner;
    try { const expected = new GameRandom(o.random.state); expected.next(100); o.loginEnd = 1; s.hooks.ground(20, 30); assert.equal(o.random.state, expected.state); assert.equal(o.core.kernel.size, 0); } finally { o.dispose(); }
});

test('폭발 삭제: 실제 단단한 다리의 false 반환을 새 성공 관문으로 바꾸지 않는다', () => {
    const s = scene(); const o = s.owner;
    try {
        assert.ok(o.core.frames[82].codes[46].flags & 0x40); const seeds = [{ type: 82, owner: 1, frame: 46, x: 31, y: 29 }, { type: 82, owner: 1, frame: 41, x: 32, y: 29 }];
        // 원문 삭제 거부가 요구하는 Graph 표면5개 이상을 실제3×3 받침으로 구성한다.
        for (let y = 28; y <= 30; y += 1) {
            // 같은 행의 세 실제 noIsland 칸을 입력한다.
            for (let x = 28; x <= 30; x += 1) seeds.push({ type: 157, owner: 0, frame: 0, x, y });
        }
        o.world.load(seeds); const bridge = o.world.find(31, 29, 82); assert.ok(bridge); assert.ok(o.core.graph.records()[o.core.pool.slot(bridge)[30]].surfaces >= 5);
        // 지형 연결이 고른 프레임과 구별해 삭제 관문의 현재 hard 프레임을 직접 입력한다.
        put(o.core.pool.allocated(bridge), 36, 46); assert.equal(o.world.destroy(bridge, 0), false); const before = o.core.pool.slot(bridge).slice(); assert.equal(s.hooks.destroy(bridge, 0x12345678, 0), undefined); assert.deepEqual(o.core.pool.slot(bridge), before);
    } finally { o.dispose(); }
});

test('폭발 끝 칸: 같은 실제 BridgeEvents를 float32 인자로 호출한다', () => {
    const s = scene(); const o = s.owner; const calls = []; const convert = o.core.bridgeEvents.convertEnd.bind(o.core.bridgeEvents); o.core.bridgeEvents.convertEnd = (...args) => { calls.push(args); return convert(...args); };
    try { const bridge = actor(o, 82); const frame = o.core.frames[82].codes.findIndex((code) => code.side !== 0x4a && code.side !== 0x4b); assert.ok(frame >= 0); put(o.core.pool.allocated(bridge), 36, frame); s.hooks.edge(bridge, 12.123456789, -3.123456789); assert.deepEqual(calls, [[bridge, Math.fround(12.123456789), Math.fround(-3.123456789)]]); assert.equal(get(o.core.pool.slot(bridge), 36), frame); } finally { o.dispose(); }
});

test('폭발 실제 Apply 체인: 기존 BombBlast가 같은 HP/지면 효과/난수로 진행한다', () => {
    const s = scene(); const o = s.owner;
    try {
        const parent = actor(o, 168, 3); const victim = actor(o, 75, 2, 10000000); o.loginEnd = 1; const expected = new GameRandom(o.random.state); expected.next(100);
        const blast = new RawBombBlast(o.core.pool, o.core.types, o.core.frames.map((frame) => frame.codes), s.coordinates, { boss: true, alliances: o.alliances, priestType: 158, growingRainBlockerType: 150 }, s.hooks); blast.apply(parent, victim); assert.equal(get(o.core.pool.slot(victim), 26), 9999000); assert.equal(o.random.state, expected.state); assert.equal(o.core.kernel.size, 0);
    } finally { o.dispose(); }
});
