// 다이스·제단 고유 삭제 raw-dais-deletion.js(RawDaisDeletion)의 계약 시험이다. 제단 단위(altar-sacrifice.test.mjs)의 원본 관측이 daisPre/finishForms 를 대조하고,
// 여기서는 관측이 닿지 않는 altarPre·daisPost·typed 점 조회와 각 판정의 경계(발자국 사각형·추상 비트·권한/클라이언트 번호·Player 기록 범위)를 합성 장면에서 직접 확인한다.
// 같은 풀의 실제 Host/해시/발자국을 쓰고 효과는 기록 훅이다. 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E368: 7-P3 RawAltarSacrifice 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { put, get, floatToBits } from './raw-support.mjs';
import { RegularProcess, SquidProcess } from '../www/core/squid-process.js';
import { RawDaisDeletion } from '../www/core/raw-dais-deletion.js';
import { VTABLE, makeWorld } from './dais-support.mjs';

// 다이스(5)·사제(6)·제단(7)이 있는 세계와 기록 훅을 만든다. ctl 로 조회·권한·Player 기록을 정한다. 발자국: 다이스(foot 3)는 [38.75..40.75] × [39.25..41.25] → finder 영역 [38..41] × [39..42], 제단(foot 1)은 한 점 (40.75, 41.25) → 영역 [40..41] × [41..42]. (2026-11-13 WB04-E368)
function scene(ctl = {}) {
    const world = makeWorld();
    world.insert(5, 165);
    world.insert(6, 158);
    world.insert(7, 164);
    const log = [];
    const hooks = {
        findDais: (sid) => { log.push(`findDais:${sid}`); return ctl.dais ?? 0; },
        findAltar: (sid) => { log.push(`findAltar:${sid}`); return ctl.altar ?? 0; },
        findPriest: (sid) => { log.push(`findPriest:${sid}`); return ctl.priest ?? 0; },
        finishForms: (sid, altar) => { log.push(`fin:${sid},${altar}`); },
        stopPriest: (sid) => { log.push(`stop:${sid}`); },
        refreshOwner: (owner) => { log.push(`R:${owner}`); },
        boss: () => { log.push('boss'); return ctl.boss ?? false; },
        destroy: (sid, flags) => { log.push(`des:${sid},${flags}`); },
        pre: (sid, flags) => { log.push(`pre:${sid},${flags}`); },
        post: (sid, flags) => { log.push(`post:${sid},${flags}`); },
        players: () => { log.push('players'); ctl.onPlayers?.(); return { records: ctl.records }; },
        ...(ctl.hooks ?? {}),
    };
    const altarType = ctl.altarType ?? 164;
    const runeType = ctl.runeType ?? 166;
    const deletion = new RawDaisDeletion(world.pool, world.hash, world.types, world.footprint, altarType, runeType, hooks);
    // 임의 좌표에 추상 룬(또는 지정 extra/TYPE)을 놓는다. sid 8..11 만 쓴다.
    const place = (sid, x, y, { type = 166, extra = 1 } = {}) => {
        const raw = world.pool.physical(sid);
        raw.fill(0);
        put(raw, 0, VTABLE);
        raw[10] = type;
        raw[40] = extra;
        put(raw, 14, floatToBits(x), 4);
        put(raw, 18, floatToBits(y), 4);
        put(raw, 4, world.hash.bucket(1, x, y), 2);
        world.hash.setBucket(1, x, y, sid);
    };
    return { world, log, hooks, deletion, place };
}

test('RawDaisDeletion: 생성자는 10.78 풀과 같은 풀의 발자국만 받는다', () => {
    const a = scene();
    const b = scene();
    const make = (pool, footprint) => new RawDaisDeletion(pool, a.world.hash, a.world.types, footprint, 164, 166, a.hooks);
    assert.ok(make(a.world.pool, a.world.footprint).pool === a.world.pool);
    assert.throws(() => make(a.world.pool, b.world.footprint), TypeError);
    assert.throws(() => make({ ...a.world.pool, edition: 'cd1072' }, a.world.footprint), TypeError);
    assert.throws(() => make(a.world.pool, null), TypeError);
    assert.throws(() => make(null, a.world.footprint), TypeError);
});

test('RawDaisDeletion.altarPre: 삭제 flags 에 0x400000 이 없고 다이스가 살아 있을 때만 같은 flags 로 지우고 발자국 안 추상 룬을 권한/클라이언트 번호 조건으로 지운다', () => {
    // 다이스 삭제: flags 0x400000 이 있으면 조회하지 않는다(조회 훅이 없어도 된다). 죽은(상태 비트 2) 다이스·풀 밖·0 은 건너뛴다.
    for (const [flags, dais, state, deleted] of [[0, 5, 0, true], [1, 5, 0, true], [0x1234, 5, 0, true], [0x400000, 5, 0, false], [0x400001, 5, 0, false], [0, 5, 2, false], [0, 5, 3, false], [0, 5, 1, true], [0, 5, 4, true], [0, 0, 0, false], [0, 24000, 0, false], [0, 23999, 0, true]]) {
        const s = scene({ dais });
        s.world.pool.physical(5)[11] = state;
        if (dais === 23999) s.world.pool.physical(23999)[11] = 0;
        // 삭제 flags 에 0x400000 이 있으면 다이스 조회 훅이 없어도 된다.
        const hooks = flags & 0x400000 ? { ...s.hooks, findDais: undefined } : s.hooks;
        new RawDaisDeletion(s.world.pool, s.world.hash, s.world.types, s.world.footprint, 164, 166, hooks).altarPre(5, flags);
        assert.equal(s.log.includes(`des:${dais},${flags}`), deleted, `flags ${flags} 다이스 ${dais} 상태 ${state}`);
        assert.equal(s.log.at(-1), `pre:5,${flags}`);
        assert.equal(s.log.includes('findDais:5'), (flags & 0x400000) === 0, `조회 flags ${flags}`);
    }
    // 조회 훅이 필요한데 없으면 거부한다(0x400000 이 아닐 때).
    const missing = scene();
    assert.throws(() => new RawDaisDeletion(missing.world.pool, missing.world.hash, missing.world.types, missing.world.footprint, 164, 166, { ...missing.hooks, findDais: undefined }).altarPre(5, 0), /다이스 삭제의 선택된 실제 하위 미연결/);
    // 제단(7)의 발자국 영역 [40..41] × [41..42] 안의 추상 룬(extra 비트 1)만 지운다. 안쪽 모서리 두 곳은 지우고 바깥(x 42·x 39)은 건너뛴다.
    const area = scene({ boss: true });
    area.place(8, 40.5, 41.5);
    area.place(9, 41.5, 42.5);
    area.place(10, 42.5, 42.5);
    area.place(11, 39.5, 41.5);
    area.deletion.altarPre(7, 0x400000);
    const destroyed = area.log.filter((entry) => entry.startsWith('des:')).sort();
    assert.deepEqual(destroyed, ['des:8,0', 'des:9,0']);
    // 비추상 룬(extra 비트 1 없음)·다른 TYPE 은 지우지 않는다. 추상 비트가 다른 비트와 함께여도 지운다.
    const kinds = scene({ boss: true });
    kinds.place(8, 40.5, 41.5, { extra: 0 });
    kinds.place(9, 40.5, 41.5, { extra: 2 });
    kinds.place(10, 40.5, 41.5, { extra: 3 });
    kinds.place(11, 40.5, 41.5, { type: 165, extra: 1 });
    kinds.deletion.altarPre(7, 0x400000);
    assert.deepEqual(kinds.log.filter((entry) => entry.startsWith('des:')), ['des:10,0']);
    // 현재 룬 TYPE 은 후보마다 다시 읽는다(getter).
    let rune = 166;
    const live = scene({ boss: true, runeType: () => rune });
    live.place(8, 40.5, 41.5);
    rune = 165;
    live.deletion.altarPre(7, 0x400000);
    assert.ok(!live.log.some((entry) => entry.startsWith('des:')));
    // 권한이 없으면 클라이언트 번호(5..14999)만 지운다: 서버 번호(15000 이상)와 4 이하는 지우지 않는다.
    const server = scene({ boss: false });
    server.world.pool.allocate(0);
    const serverSid = 15000;
    server.world.pool.physical(serverSid).fill(0);
    for (const [sid, expected] of [[8, true], [serverSid, false], [14999, true], [4, false]]) {
        const s = scene({ boss: false });
        s.place(sid, 40.5, 41.5);
        s.deletion.altarPre(7, 0x400000);
        assert.equal(s.log.includes(`des:${sid},0`), expected, `sid ${sid}`);
        assert.ok(s.log.includes('boss'));
    }
    // 클라이언트 번호의 아래 경계: SID 5 의 룬도 지운다(4 는 지우지 않는다).
    const five = scene({ boss: false });
    five.world.pool.physical(5)[10] = 166;
    five.world.pool.physical(5)[40] = 1;
    five.deletion.altarPre(7, 0x400000);
    assert.ok(five.log.includes('des:5,0'));
    // 권한이 있으면 서버 번호도 지운다. 권한 훅은 지울 후보에서만 부른다(룬이 없으면 부르지 않는다).
    const bossy = scene({ boss: true });
    bossy.place(8, 40.5, 41.5);
    bossy.deletion.altarPre(7, 0x400000);
    assert.deepEqual(bossy.log, ['boss', 'des:8,0', 'pre:7,4194304']);
    const nobody = scene({ boss: true });
    nobody.deletion.altarPre(7, 0x400000);
    assert.deepEqual(nobody.log, ['pre:7,4194304']);
});

test('RawDaisDeletion.daisPre: extra 9 관문 안에서만 제단 24 완료(0x400000)·604 중지·제단 삭제(0x48)·owner 갱신을 순서대로 하고 공통 전처리는 항상 부른다', () => {
    const orders = Array.from({ length: 8 }, (_, index) => `R:${index + 1}`);
    // 관문: extra 에 1 이나 8 이 있으면 전부 건너뛴다.
    for (const [extra, gated] of [[0, false], [1, true], [8, true], [9, true], [2, false], [4, false], [0x10, false], [0x80, false], [0x40, false], [0x20, false]]) {
        const s = scene({ altar: 7, priest: 6 });
        s.world.pool.physical(5)[40] = extra;
        s.deletion.daisPre(5, 0x400000);
        const expected = gated ? ['pre:5,4194304'] : ['findAltar:5', 'fin:5,7', 'findPriest:5', 'stop:6', 'des:7,72', ...orders, 'pre:5,4194304'];
        assert.deepEqual(s.log, expected, `extra ${extra}`);
    }
    // 삭제 flags 의 다른 비트(1)만 있어도 제단 조회/완료를 하지 않는다.
    const lowBit = scene({ altar: 7, priest: 6 });
    lowBit.deletion.daisPre(5, 1);
    assert.ok(!lowBit.log.includes('findAltar:5') && !lowBit.log.some((entry) => entry.startsWith('fin:')));
    // 삭제 flags 에 0x400000 이 없으면 제단 조회/완료를 하지 않는다. 사제가 없거나 풀 밖이면 604 중지를 건너뛴다.
    const plain = scene({ priest: 0 });
    plain.deletion.daisPre(5, 0);
    assert.deepEqual(plain.log, ['findPriest:5', 'des:7,72', ...orders, 'pre:5,0']);
    for (const [priest, stops] of [[6, true], [23999, true], [24000, false], [0, false]]) {
        const s = scene({ priest });
        s.deletion.daisPre(5, 0);
        assert.equal(s.log.includes(`stop:${priest}`), stops, `사제 ${priest}`);
    }
    // 발자국 안의 현재 제단 TYPE 만 지운다(getter 도 읽는다). 제단이 둘이면 둘 다 flags 0x48 이다.
    const altars = scene({ altarType: () => 164 });
    altars.place(8, 40.5, 40.5, { type: 164, extra: 0 });
    altars.deletion.daisPre(5, 0);
    assert.deepEqual(altars.log.filter((entry) => entry.startsWith('des:')).sort(), ['des:7,72', 'des:8,72']);
    const other = scene({ altarType: 165 });
    other.deletion.daisPre(5, 0);
    assert.ok(other.log.includes('des:5,72') && !other.log.includes('des:7,72'));
    // 선택한 장치가 없으면 도달한 위치에서 거부한다.
    for (const name of ['findPriest', 'destroy', 'refreshOwner', 'pre']) {
        const s = scene();
        assert.throws(() => new RawDaisDeletion(s.world.pool, s.world.hash, s.world.types, s.world.footprint, 164, 166, { ...s.hooks, [name]: undefined }).daisPre(5, 0), /다이스 삭제의 선택된 실제 하위 미연결/, name);
    }
    const noFinish = scene({ altar: 7 });
    assert.throws(() => new RawDaisDeletion(noFinish.world.pool, noFinish.world.hash, noFinish.world.types, noFinish.world.footprint, 164, 166, { ...noFinish.hooks, finishForms: undefined }).daisPre(5, 0x400000), /미연결/);
});

test('RawDaisDeletion.daisPost: 다이스 owner 의 Player 기록에서 이 SID 를 가리키는 두 칸을 비우고 보조 칸을 채우며 기록은 단계마다 다시 빌린다', () => {
    const RECORD = 0xb4;
    const make = (owner, firstHit, secondHit, onPlayers) => {
        const records = new Uint8Array(9 * RECORD);
        const s = scene({ records, onPlayers: () => onPlayers?.(s, records) });
        s.world.pool.physical(5)[34] = owner;
        if (firstHit) put(records, owner * RECORD + 0x94, 5, 4);
        if (secondHit) put(records, owner * RECORD + 0x8c, 5, 4);
        put(records, owner * RECORD + 0x98, 0x1234, 4);
        put(records, owner * RECORD + 0x90, 0x5678, 4);
        return { s, records };
    };
    // owner 0 은 Player 기록을 만지지 않고 전체 Damageable 후처리에만 위임한다.
    const zero = make(0, true, true);
    zero.s.world.pool.physical(5)[34] = 0;
    zero.s.deletion.daisPost(5, 7);
    assert.deepEqual(zero.s.log, ['post:5,7']);
    // 첫 칸만 맞으면 비우고 +4 칸(보조)에 비운 값을 복사하며 기록을 세 번 빌린다(처음·비운 뒤·복사 뒤).
    const first = make(3, true, false);
    first.s.deletion.daisPost(5, 7);
    assert.deepEqual(first.s.log, ['players', 'players', 'players', 'post:5,7']);
    assert.equal(get(first.records, 3 * RECORD + 0x94, 4), 0);
    assert.equal(get(first.records, 3 * RECORD + 0x98, 4), 0);
    assert.equal(get(first.records, 3 * RECORD + 0x90, 4), 0x5678);
    // 둘째 칸만 맞으면 두 번 빌린다(처음·비운 뒤). 둘 다 맞으면 네 번이다. 아무 것도 안 맞으면 한 번이다.
    const second = make(3, false, true);
    second.s.deletion.daisPost(5, 7);
    assert.deepEqual(second.s.log, ['players', 'players', 'post:5,7']);
    assert.deepEqual([get(second.records, 3 * RECORD + 0x8c, 4), get(second.records, 3 * RECORD + 0x90, 4), get(second.records, 3 * RECORD + 0x98, 4)], [0, 0, 0x1234]);
    const both = make(3, true, true);
    both.s.deletion.daisPost(5, 7);
    assert.deepEqual(both.s.log, ['players', 'players', 'players', 'players', 'post:5,7']);
    assert.deepEqual([get(both.records, 3 * RECORD + 0x94, 4), get(both.records, 3 * RECORD + 0x98, 4), get(both.records, 3 * RECORD + 0x8c, 4), get(both.records, 3 * RECORD + 0x90, 4)], [0, 0, 0, 0]);
    const none = make(3, false, false);
    none.s.world.pool.physical(5)[34] = 3;
    put(none.records, 3 * RECORD + 0x94, 6, 4);
    none.s.deletion.daisPost(5, 7);
    assert.deepEqual(none.s.log, ['players', 'post:5,7']);
    assert.equal(get(none.records, 3 * RECORD + 0x94, 4), 6);
    // 다른 SID 를 가리키는 칸은 건드리지 않고, 첫 칸의 SID 가 상위 바이트까지 같아야 맞다.
    const high = make(3, false, false);
    put(high.records, 3 * RECORD + 0x94, 5 + 0x10000, 4);
    high.s.deletion.daisPost(5, 7);
    assert.equal(get(high.records, 3 * RECORD + 0x94, 4), 5 + 0x10000);
    // owner 는 각 주소 계산에서 다시 읽는다: 기록을 다시 빌리는 사이 owner 가 3 → 4 로 바뀌면 보조 칸 복사는 owner 4 의 기록에서 한다.
    const moving = make(3, true, false, (s, records) => { if (s.log.filter((entry) => entry === 'players').length === 2) s.world.pool.physical(5)[34] = 4; });
    put(moving.records, 4 * RECORD + 0x94, 0xabcd, 4);
    put(moving.records, 4 * RECORD + 0x98, 0x7777, 4);
    moving.s.deletion.daisPost(5, 7);
    assert.equal(get(moving.records, 3 * RECORD + 0x94, 4), 0);
    assert.equal(get(moving.records, 4 * RECORD + 0x98, 4), 0xabcd);
    assert.equal(get(moving.records, 3 * RECORD + 0x98, 4), 0x1234);
    // 기록 범위: 읽기는 4 바이트가 남아야 하고 쓰기도 마찬가지이다. 경계 길이는 허용하고 1 바이트 부족하면 거부한다.
    const exactRead = make(3, false, false);
    const readOnly = new Uint8Array(3 * RECORD + 0x94 + 4);
    const shortRead = scene({ records: readOnly.subarray(0, readOnly.length - 1) });
    shortRead.world.pool.physical(5)[34] = 3;
    assert.throws(() => shortRead.deletion.daisPost(5, 7), (error) => error instanceof RangeError && /Player 물리 범위/.test(error.message));
    const okRead = scene({ records: readOnly });
    okRead.world.pool.physical(5)[34] = 3;
    okRead.deletion.daisPost(5, 7);
    assert.equal(okRead.log.at(-1), 'post:5,7');
    assert.ok(exactRead);
    const writeRecords = new Uint8Array(3 * RECORD + 0x98 + 4);
    put(writeRecords, 3 * RECORD + 0x94, 5, 4);
    const exactWrite = scene({ records: writeRecords });
    exactWrite.world.pool.physical(5)[34] = 3;
    exactWrite.deletion.daisPost(5, 7);
    const shortWrite = scene({ records: new Uint8Array(3 * RECORD + 0x98 + 3) });
    put(shortWrite.hooks.players().records, 3 * RECORD + 0x94, 5, 4);
    shortWrite.world.pool.physical(5)[34] = 3;
    assert.throws(() => shortWrite.deletion.daisPost(5, 7), RangeError);
    // 판본이 PATCH 가 아니면 거부하고 기록/후처리 장치가 없으면 도달한 위치에서 거부한다.
    const noPlayers = scene();
    noPlayers.world.pool.physical(5)[34] = 3;
    assert.throws(() => new RawDaisDeletion(noPlayers.world.pool, noPlayers.world.hash, noPlayers.world.types, noPlayers.world.footprint, 164, 166, { ...noPlayers.hooks, players: undefined }).daisPost(5, 7), /미연결/);
    const noPost = scene();
    assert.throws(() => new RawDaisDeletion(noPost.world.pool, noPost.world.hash, noPost.world.types, noPost.world.footprint, 164, 166, { ...noPost.hooks, post: undefined }).daisPost(5, 7), /미연결/);
});

test('RawDaisDeletion.findTypeAt/findTypeAtPoint: 좌표를 절삭한 칸에서 TYPE 의 genus 로 시작 단계를 정해 첫 일치를 돌려준다', () => {
    const s = scene();
    const find = (x, y, type) => RawDaisDeletion.findTypeAtPoint(s.world.pool, s.world.hash, s.world.types, x, y, type);
    // 점 조회: 다이스(5)·제단(7)·사제(6)가 같은 칸에 있고 사슬 순서는 7 → 6 → 5 이다.
    assert.equal(find(40.75, 41.25, 165), 5);
    assert.equal(find(40.75, 41.25, 164), 7);
    assert.equal(find(40.75, 41.25, 158), 6);
    assert.equal(find(40.75, 41.25, 99), 0);
    assert.equal(find(10.5, 10.5, 165), 0);
    // 기준 객체의 현재 좌표(+14 x, +18 y)로 조회한다: x 와 y 를 다른 값으로 둔 슬롯이다.
    s.place(8, 40.5, 41.5, { type: 99, extra: 0 });
    put(s.world.pool.physical(8), 15, 0x7f, 1);
    assert.equal(RawDaisDeletion.findTypeAt(s.world.pool, s.world.hash, s.world.types, 5, 165), 5);
    s.world.pool.physical(8)[14] = 0;
    s.world.pool.physical(8)[15] = 0;
    s.world.pool.physical(8)[16] = 0x22;
    s.world.pool.physical(8)[17] = 0x42;
    put(s.world.pool.physical(8), 18, floatToBits(41.25), 4);
    assert.equal(RawDaisDeletion.findTypeAt(s.world.pool, s.world.hash, s.world.types, 8, 165), 5);
    // 좌표 절삭 범위와 TYPE 범위(188 거부·187 허용)이다. 좌표를 먼저 검사한다.
    for (const bad of [NaN, Infinity, -Infinity, 2147483648, -2147483904]) {
        assert.throws(() => find(bad, 1, 165), (error) => error instanceof RangeError && /사각형 절삭 범위/.test(error.message), `x=${bad}`);
        assert.throws(() => find(1, bad, 165), RangeError, `y=${bad}`);
    }
    find(-2147483648, 1, 165);
    find(2147483520, 1, 165);
    assert.throws(() => find(40.75, 41.25, 188), (error) => error instanceof RangeError && /TYPE 범위/.test(error.message));
    assert.equal(find(40.75, 41.25, 187), 0);
    assert.throws(() => find(NaN, 41.25, 188), /사각형 절삭 범위/);
    // genus 의 표면 비트(6)가 있으면 표면 단계를 훑는다: level 0 에 놓은 순환 사슬을 만나면 finder 가 거부한다.
    for (const [flags2, scans] of [[0, false], [1, false], [2, true], [4, true], [6, true], [8, false]]) {
        const w = scene();
        w.world.types[165].flags2 = flags2;
        w.world.pool.physical(11)[10] = 1;
        put(w.world.pool.physical(11), 4, 11, 2);
        w.world.hash.setCell(0, 40, 41, 11);
        const call = () => RawDaisDeletion.findTypeAtPoint(w.world.pool, w.world.hash, w.world.types, 40.75, 41.25, 165);
        if (scans) assert.throws(call, /체인/, `flags2 ${flags2}`);
        else assert.equal(call(), 5);
    }
    // 첫 일치에서 멈춘다: 다이스 뒤에 순환 사슬이 있어도 다이스를 돌려주고, 다른 TYPE 이 앞서면 계속 가서 순환을 만난다.
    const first = scene();
    first.world.pool.physical(11)[10] = 1;
    put(first.world.pool.physical(11), 4, 11, 2);
    put(first.world.pool.physical(5), 4, 11, 2);
    assert.equal(RawDaisDeletion.findTypeAtPoint(first.world.pool, first.world.hash, first.world.types, 40.75, 41.25, 165), 5);
    const miss = scene();
    miss.world.pool.physical(11)[10] = 1;
    put(miss.world.pool.physical(11), 4, 11, 2);
    miss.world.pool.physical(5)[10] = 1;
    put(miss.world.pool.physical(5), 4, 11, 2);
    assert.throws(() => RawDaisDeletion.findTypeAtPoint(miss.world.pool, miss.world.hash, miss.world.types, 40.75, 41.25, 165), /체인/);
});

test('RawDaisDeletion.finishForms: 제단의 현재 Form 사슬을 다음 링크를 먼저 저장하며 돌고 Regular/Shared 24 프로세스마다 희생 완료를 부른다', () => {
    const regularType = 46;
    const sharedType = 61;
    // 제단 7 에 프로세스 Form 을 붙인다(최근 것이 사슬 머리). type 은 프로세스 Form TYPE 이다.
    const attach = (s, type, event, payload = 0.12) => {
        const process = s.world.host.attach(new RegularProcess(event, payload), type, 7, 0x10);
        assert.ok(process);
        return process;
    };
    const run = (s, hooks = {}) => {
        const finished = [];
        RawDaisDeletion.finishForms(s.world.pool, s.world.host, 5, 7, hooks.regular ?? regularType, hooks.shared ?? sharedType, hooks.finish ?? ((sid) => finished.push(sid)));
        return finished;
    };
    // 이벤트 24 인 Regular(46)·Shared(61) 프로세스마다 완료를 부르고 다른 이벤트·다른 Form TYPE 은 건너뛴다.
    const s = scene();
    attach(s, regularType, 24);
    attach(s, sharedType, 24);
    attach(s, regularType, 25);
    attach(s, sharedType, 23);
    attach(s, 20, 24);
    assert.deepEqual(run(s), [5, 5]);
    // 역할 번호는 후보마다 다시 읽는다(getter). 둘 다 숫자가 아니어도 같은 값을 본다.
    const live = scene();
    attach(live, regularType, 24);
    let current = regularType;
    assert.deepEqual(run(live, { regular: () => current, shared: () => 0 }), [5]);
    current = 47;
    assert.deepEqual(run(live, { regular: () => current, shared: () => 0 }), []);
    // 완료가 현재 후보를 지우거나 링크를 지워도 저장한 다음 링크로 계속 돈다(두 Form 모두 처리).
    const unlink = scene();
    const a = attach(unlink, regularType, 24);
    const b = attach(unlink, regularType, 24);
    const seen = [];
    RawDaisDeletion.finishForms(unlink.world.pool, unlink.world.host, 5, 7, regularType, sharedType, () => {
        seen.push(seen.length);
        // 첫 후보(최근 b)의 다음 링크를 지우면 저장한 값으로 a 를 계속 처리해야 한다.
        const head = get(unlink.world.pool.slot(7), 6, 2);
        put(unlink.world.pool.physical(head), 4, 0, 2);
    });
    assert.equal(seen.length, 2);
    assert.ok(a && b);
    // 제단 head 는 WORD(2 바이트)만 읽는다: 옆 바이트에 쓰레기가 있어도 영향이 없다.
    const noise = scene();
    attach(noise, regularType, 24);
    noise.world.pool.physical(7)[8] = 0xff;
    assert.deepEqual(run(noise), [5]);
    // 프로세스 Form TYPE 범위: 10 미만은 isProcess 진단 오류, 63(processTypeEnd) 이상 69 이하도 오류이며 70 이상은 프로세스가 아니라 건너뛴다.
    for (const [type, fails] of [[9, true], [10, false], [62, false], [63, true], [64, true], [69, true], [70, false], [100, false]]) {
        const w = scene();
        const process = type >= 10 && type <= 62 ? attach(w, type, 25) : null;
        const slot = process ? process.form : 12;
        if (!process) {
            w.world.pool.physical(slot).fill(0);
            w.world.pool.physical(slot)[10] = type;
            put(w.world.pool.physical(7), 6, slot, 2);
        }
        if (type === 10 || type === 62) {
            // Kernel 에 프로세스가 있는 정상 Form 이다(이벤트 25 라 완료는 부르지 않는다).
            assert.deepEqual(run(w), []);
        } else if (fails && type !== 70) {
            assert.throws(() => run(w), /isProcess assert/, `TYPE ${type}`);
        } else if (type >= 70) {
            assert.deepEqual(run(w), [], `TYPE ${type}`);
        }
    }
    // Kernel 에 프로세스가 없으면 거부하고, 매칭 TYPE 의 프로세스가 Regular 가 아니면 거부한다.
    const lost = scene();
    lost.world.pool.physical(12).fill(0);
    lost.world.pool.physical(12)[10] = 20;
    put(lost.world.pool.physical(7), 6, 12, 2);
    assert.throws(() => run(lost), /실제 Kernel 프로세스 누락/);
    const plain = scene();
    class Plain extends SquidProcess {
        // 실행할 일이 없는 일반 프로세스이다.
        runFrame() { return 0; }
    }
    const process = plain.world.host.attach(new Plain(), regularType, 7, 0x10);
    assert.ok(process);
    assert.throws(() => run(plain), /Regular 필드 미복원/);
    // 사슬 손상(순환·풀 밖)은 거부하고 완료 훅이 없으면 완료가 필요할 때 거부한다.
    const cycle = scene();
    const p = attach(cycle, regularType, 25);
    put(cycle.world.pool.physical(p.form), 4, p.form, 2);
    assert.throws(() => run(cycle), /체인 손상/);
    const out = scene();
    put(out.world.pool.physical(7), 6, 23999, 2);
    out.world.pool.physical(23999)[10] = 200;
    assert.deepEqual(run(out), []);
    // head 가 풀 크기와 같으면(23999 다음) 물리 범위 오류가 아니라 사슬 손상으로 거부한다.
    const edge = scene();
    put(edge.world.pool.physical(7), 6, 24000, 2);
    assert.throws(() => run(edge), /체인 손상/);
    // 다음 링크는 WORD(2 바이트)만 읽는다: Form 의 자식 head(+6)가 0 이 아니어도 사슬이 이어진다.
    const wideNext = scene();
    attach(wideNext, regularType, 24);
    attach(wideNext, regularType, 24);
    wideNext.world.pool.physical(get(wideNext.world.pool.slot(7), 6, 2))[6] = 1;
    assert.deepEqual(run(wideNext), [5, 5]);
    const noFinish = scene();
    attach(noFinish, regularType, 24);
    assert.throws(() => RawDaisDeletion.finishForms(noFinish.world.pool, noFinish.world.host, 5, 7, regularType, sharedType, undefined), /미연결/);
    // 빈 사슬(head 0)은 아무 것도 하지 않는다.
    const empty = scene();
    assert.deepEqual(run(empty), []);
});
