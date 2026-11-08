// 건설 효과 raw-construction-effects.js(RawConstructionEffects·ConstructionEffectsState·요청 어댑터)와 선택 어댑터 raw-construction-selection.js를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: constructioneffects-x86.tsv 의 `originals` 1178행 — 밀어내기(Displace)와 섬 정리(DropIsland)의 입력(타입·좌표·발자국·플레이어·flags·억제·서버·spot 윤곽·관계 표·변동)과
//    후보 목록(번호·타입·extra·좌표·소유자·프레임·해시 단계·genus)에서 반환값과 효과 순서(A 탐색 범위/R 경로 타입/C 경로 삭제/U Unpop/P Pop/F 추락/S 소리/K 삭제/N 알림/I·B 방송/E assert)가 같다.
//    `originalCD`·`original1037` 행은 다른 판본이라 사용하지 않는다.
//  - 계약: 필수 경계·지도 크기 누락 거부, 요청 어댑터의 다른 풀 거부, 타입 표 밖/비유한 좌표 거부, Dude 의 foot 0 도 한 칸 사각형으로 펼침, 추락/삭제/방송 뒤 현재 좌표·genus·섬 타입 다시 읽기,
//    공유 server 역할로 방송 결정, 선택 어댑터(기준점·소유 사제 조회)의 풀 불일치 거부와 WORD 전달.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E135: 건설 효과 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { ConstructionEffectsState, RawConstructionEffects, makeConstructionEffectsRequestHooks } from '../www/core/raw-construction-effects.js';
import { makeConstructionAnchorHooks, makeConstructionPriestLookupHooks } from '../www/core/raw-construction-selection.js';
import { makeConstructionProcessHooks } from '../www/core/raw-construction-process.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E135)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'constructioneffects-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[0] === 'originals');

// 판본별 관측 행 수, 경로 타입 경계의 토큰, 섬 타입, 가상 표 기록값, 풀 크기이다. (2026-11-08 WB04-E135)
const ROWS = 1178;
const PATH_TYPE = 0x12345678;
const ISLAND = 94;
const VTABLE = 0x11010000;
const CAPACITY = 24000;

// 예외가 나는지 본다. (2026-11-08 WB04-E135)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 장면: 행마다 공유 타입/지도/풀과 효과 기록을 다시 준비한다. (2026-11-08 WB04-E135)
class Scene {
    constructor() {
        this.pool = new SidPool(CAPACITY, { server: true });
        this.hash = new SquidHash();
        this.types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0, group: 0 }));
        this.spots = new Uint8Array(65536);
        this.state = new ConstructionEffectsState();
        this.input = [];
        this.events = [];
    }

    // 실행기의 합성 입력을 공급한다. 기대 효과를 계산하지 않는다. (2026-11-08 WB04-E135)
    prepare(v) {
        this.input = v;
        for (const type of this.types) Object.assign(type, { flags1: 0, flags2: 0, footX: 0, footY: 0, group: 0 });
        this.hash.reset();
        this.events = [];
        this.state = new ConstructionEffectsState();
        this.types[v[0]].footX = v[3] | 0;
        this.types[v[0]].footY = v[4] | 0;
        this.types[ISLAND].footX = v[3] | 0;
        this.types[ISLAND].footY = v[4] | 0;
        this.state.blocked = v[7] !== 0;
        this.state.server = v[8] !== 0;
        this.state.alliances.fill(v[12]);
        const profile = v[9];
        const open = profile >= 7 && profile <= 13;
        this.spots.fill(open ? 0 : profile);
        // 한 칸 spot 입력으로 검색 순서와 탐색 거리를 구분한다.
        if (open) {
            const positions = [[20, 22], [19, 22], [20, 24], [21, 21], [23, 21], [19, 23], [1, 2]];
            const [px, py] = positions[profile - 7];
            this.spots[py * 256 + px] = 2;
        }
    }

    // 후보를 fixture 에 지정된 해시 단계의 머리에 등록한다. (2026-11-08 WB04-E135)
    node(n) {
        const raw = this.pool.physical(n[0]);
        raw.fill(0);
        put(raw, 0, VTABLE);
        raw[10] = n[1];
        raw[40] = n[2];
        put(raw, 14, n[3]);
        put(raw, 18, n[4]);
        raw[34] = n[5];
        put(raw, 36, n[6], 4);
        this.types[n[1]].flags2 = n[8];
        if (n[1] !== ISLAND) this.types[n[1]].footX = this.types[n[1]].footY = 1;
        const x = bitsToFloat(n[3]);
        const y = bitsToFloat(n[4]);
        put(raw, 4, this.hash.bucket(n[7], x, y), 2);
        this.hash.setBucket(n[7], x, y, n[0]);
    }

    // 경계 인자의 DWORD 비트를 순서대로 기록한다. 인자가 없으면 글자만 남긴다. (2026-11-08 WB04-E135)
    append(letter, values = []) {
        this.events.push(`${letter}${values.map((value) => `:${value >>> 0}`).join('')}`);
    }

    // 원본 실행기에서 대체한 효과만 재생한다. finder begin 은 인자만 관찰한다. (2026-11-08 WB04-E135)
    hooks() {
        const bits = floatToBits;
        return {
            pathType: (type) => { this.append('R', [type]); return PATH_TYPE; },
            clearPath: (sid, type) => this.append('C', [sid, type, 0, 1]),
            unpop: (sid, flags) => this.append('U', [sid, flags]),
            pop: (sid, x, y, flags) => this.append('P', [sid, bits(x), bits(y), flags]),
            fall: (sid) => {
                this.append('F', [sid]);
                if (this.input[14] === 1) {
                    const raw = this.pool.physical(sid);
                    put(raw, 14, bits(1.25));
                    put(raw, 18, bits(2.5));
                }
            },
            sound: (x, y) => this.append('S', [bits(x), bits(y), 0]),
            destroy: (sid, flags) => {
                this.append('K', [sid, flags]);
                if (this.input[14] === 2) this.types[this.pool.slot(sid)[10]].flags2 = 0x10000;
            },
            notify: (sid) => this.append('N', [sid]),
            broadcast: (x, y, flags, player) => { this.append('I'); this.append('B', [x, y, flags, player]); },
            assertion: (expression, line) => {
                assert.equal(expression, line === 0x7b ? 'noIslandTypeCount == 9' : line === 0x7c ? 'islandTypeCount == 1' : 'islandStalagTypeCount == 1');
                this.append('E', [line]);
            },
            scan: (a) => this.append('A', [a.left, a.top, a.right, a.bottom]),
        };
    }

    // 현재 장면으로 효과 몸체를 만든다. (2026-11-08 WB04-E135)
    effects(hooks = this.hooks()) {
        return new RawConstructionEffects(this.pool, this.hash, this.types, this.spots, this.state, hooks);
    }
}

test('효과: 원본 10.78 관측 1178행(밀어내기·섬 정리)의 반환값과 효과 순서가 같다', { skip: rows.length === 0 }, () => {
    assert.equal(rows.length, ROWS);
    const scene = new Scene();
    // 관찰 행의 모든 입력과 효과 순서를 비교한다.
    for (const row of rows) {
        assert.equal(row.length, 6);
        const v = row[2].split(',').map(Number);
        assert.equal(v.length, 15);
        scene.prepare(v);
        if (row[3] !== '-') {
            // 입력 후보를 동일 순서로 등록한다.
            for (const entry of row[3].split(';')) {
                const n = entry.split(',').map(Number);
                assert.equal(n.length, 9);
                scene.node(n);
            }
        }
        const effects = scene.effects();
        let result = true;
        if (row[1] === 'Displace') result = effects.displace(bitsToFloat(v[1]), bitsToFloat(v[2]), v[0], v[5]);
        else effects.dropIsland(bitsToFloat(v[1]), bitsToFloat(v[2]), v[6], v[5]);
        if (row[4] !== '-') assert.equal(result, Number(row[4]) !== 0, row.join(' | '));
        assert.equal(scene.events.join(';'), row[5] === '-' ? '' : row[5], row.join(' | '));
    }
});

test('효과: 누락된 경계·지도 크기·다른 풀·타입 표 밖·비유한 좌표를 첫 효과 전에 거부한다', () => {
    const scene = new Scene();
    scene.prepare([100, floatToBits(20), floatToBits(21), 3, 3, 3, 0x1000, 0, 1, 2, 0x10000, 3, 0, 0, 0]);
    for (const name of ['pathType', 'clearPath', 'unpop', 'pop', 'fall', 'sound', 'destroy', 'notify', 'broadcast']) {
        assert.ok(throws(() => scene.effects({ ...scene.hooks(), [name]: undefined })), name);
    }
    assert.ok(throws(() => new RawConstructionEffects(scene.pool, scene.hash, scene.types, new Uint8Array(0), scene.state, scene.hooks())));
    assert.ok(throws(() => new RawConstructionEffects(scene.pool, scene.hash, scene.types.slice(0, 171), scene.spots, scene.state, scene.hooks())));
    assert.ok(throws(() => new RawConstructionEffects({ edition: 'cd1072' }, scene.hash, scene.types, scene.spots, scene.state, scene.hooks())));
    const effects = scene.effects();
    assert.equal(effects.pool, scene.pool);
    assert.ok(throws(() => makeConstructionEffectsRequestHooks(new SidPool(CAPACITY, { server: true }), effects, {})));
    assert.ok(throws(() => effects.displace(20, 21, 300, 3)));
    assert.ok(throws(() => effects.displace(bitsToFloat(0x7fc01234), 21, 100, 3)));
    assert.deepEqual(scene.events, []);
    // 억제 중이면 비유한 좌표라도 아무 것도 하지 않는다.
    scene.state.blocked = true;
    effects.dropIsland(bitsToFloat(0x7fc01234), 21, 0, 3);
    assert.deepEqual(scene.events, []);
});

test('효과: 요청 어댑터는 displace/displaceCd/cancel 세 효과를 몸체에 잇고 나머지 요청 경계는 보존한다', () => {
    const scene = new Scene();
    scene.prepare([100, floatToBits(20), floatToBits(21), 3, 3, 3, 0x1000, 0, 1, 2, 0x10000, 3, 0, 0, 0]);
    const effects = scene.effects();
    const keep = () => true;
    const connected = makeConstructionEffectsRequestHooks(scene.pool, effects, { mayPlace: keep });
    assert.equal(connected.mayPlace, keep);
    assert.equal(connected.displace(20, 21, 100, 3), true);
    assert.deepEqual(scene.events, ['A:18:19:20:21']);
    scene.events = [];
    assert.equal(connected.displaceCd(20, 21, 100), undefined);
    assert.deepEqual(scene.events, ['A:18:19:20:21']);
    scene.events = [];
    scene.state.blocked = true;
    connected.cancel(20, 21, 0x1000, 3);
    assert.deepEqual(scene.events, []);
});

test('효과: Dude 의 foot 0 도 한 칸 사각형으로 펼치고 음수 발자국은 효과 전에 거부한다', () => {
    const scene = new Scene();
    scene.prepare([70, floatToBits(20), floatToBits(21), 0, 0, 3, 0, 0, 1, 2, 0, 3, 0, 0, 0]);
    const effects = scene.effects();
    assert.equal(effects.displace(20, 21, 70, 3), true);
    assert.deepEqual(scene.events, ['A:20:21:21:22']);
    scene.events = [];
    scene.types[70].footX = -1;
    assert.ok(throws(() => effects.displace(20, 21, 70, 3)));
    assert.deepEqual(scene.events, []);
});

test('효과: 추락 뒤 현재 좌표·삭제 뒤 현재 genus·방송 뒤 현재 섬 발자국을 다시 읽는다', () => {
    const scene = new Scene();
    scene.prepare([100, floatToBits(20), floatToBits(21), 3, 3, 3, 0, 0, 1, 2, 0, 3, 0, 0, 0]);
    scene.node([50, 157, 0, floatToBits(20), floatToBits(21), 3, 0, 0, 0]);
    const hooks = scene.hooks();
    let called = 0;
    // 방송 경계가 섬 발자국을 바꾸는 상황과 추락 경계가 조각 좌표를 바꾸는 상황을 공급한다.
    hooks.broadcast = () => { scene.types[94].footX = 4; };
    hooks.fall = (sid) => {
        const raw = scene.pool.physical(sid);
        put(raw, 14, floatToBits(1.25));
        put(raw, 18, floatToBits(2.5));
    };
    hooks.sound = (x, y) => { assert.equal(x, 1.25); assert.equal(y, 2.5); called += 1; };
    hooks.destroy = (sid, flags) => { assert.equal(flags, 0x48); scene.types[scene.pool.slot(sid)[10]].flags2 = 0x10000; };
    hooks.notify = (sid) => { assert.equal(sid, 50); called += 1; };
    const effects = scene.effects(hooks);
    effects.dropIsland(20, 21, 0, 3);
    assert.equal(called, 2);
    assert.ok(scene.events[0].startsWith('A:17:19:20:21'), scene.events[0]);
});

test('효과: 섬 정리 방송은 공유 server 역할의 현재 값으로 결정하고 flags 0x50 이면 방송하지 않는다', () => {
    const scene = new Scene();
    const input = new Array(15).fill(0);
    input[0] = ISLAND;
    input[3] = input[4] = 1;
    input[8] = 1;
    scene.prepare(input);
    let server = false;
    scene.state.sharedServer = () => server;
    const hooks = scene.hooks();
    let broadcasts = 0;
    let diagnostics = 0;
    hooks.broadcast = (x, y, flags, player) => {
        assert.equal(x, 20 * 65536);
        assert.equal(y, 21 * 65536);
        assert.equal(player, 1);
        broadcasts += 1;
    };
    hooks.assertion = () => { diagnostics += 1; };
    const effects = scene.effects(hooks);
    effects.dropIsland(20, 21, 0, 1);
    assert.equal(broadcasts, 0);
    assert.equal(diagnostics, 0);
    // 독립 필드가 꺼져 있어도 공유 역할이 켜지면 방송한다.
    server = true;
    scene.state.server = false;
    effects.dropIsland(20, 21, 0, 1);
    assert.equal(broadcasts, 1);
    assert.equal(diagnostics, 0);
    // 방송 억제 비트(0x50)가 있으면 방송하지 않는다.
    effects.dropIsland(20, 21, 0x10, 1);
    assert.equal(broadcasts, 1);
});

test('선택 어댑터: 기준점·소유 사제 조회를 같은 풀에서만 잇고 SID 를 WORD 로 전달한다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const other = new SidPool(CAPACITY, { server: true });
    const calls = [];
    const anchor = { pool, locate: (player, type, x, y) => { calls.push(['locate', player, type, x, y]); return 0x12345; } };
    const lookup = { pool, find: (player) => { calls.push(['find', player]); return 0x10032; } };
    assert.ok(throws(() => makeConstructionAnchorHooks(other, anchor, {})));
    assert.ok(throws(() => makeConstructionPriestLookupHooks(other, lookup, {})));
    assert.ok(throws(() => makeConstructionAnchorHooks(pool, null, {})));
    const keep = () => 1;
    const confirm = makeConstructionAnchorHooks(pool, anchor, { start: keep });
    assert.equal(confirm.start, keep);
    assert.equal(confirm.findBuilder(3, 100, 20.5, 21.5), 0x2345);
    const notice = makeConstructionPriestLookupHooks(pool, lookup, { refresh: keep });
    assert.equal(notice.refresh, keep);
    assert.equal(notice.findPriest(3), 0x32);
    assert.deepEqual(calls, [['locate', 3, 100, 20.5, 21.5], ['find', 3]]);
});

test('효과: 건설 프로세스 어댑터의 유닛 밀어내기 경계가 실제 displace 로 이어진다(적대 거부 반환은 버린다)', () => {
    const scene = new Scene();
    scene.prepare([100, floatToBits(20), floatToBits(21), 3, 3, 3, 0, 0, 1, 2, 0, 3, 0, 0, 0]);
    // 후보 유닛(genus 0x200000)이 소유자 4, 관계 표 0 이라 요청 플레이어 3 에게 적대이다.
    scene.node([50, 102, 0, floatToBits(20), floatToBits(21), 4, 0, 0, 0x200000]);
    const effects = scene.effects();
    const reward = { pool: scene.pool, maxHitPoints: (sid) => sid + 1 };
    const connected = makeConstructionProcessHooks(scene.pool, reward, effects, { keep: 1 });
    assert.equal(connected.keep, 1);
    assert.equal(connected.maxHitPoints(9), 10);
    // 적대 거부(false)는 호출한 쪽으로 돌려주지 않는다.
    assert.equal(connected.push(20, 21, 100, 3), undefined);
    assert.ok(scene.events.length > 0 && scene.events[0].startsWith('A:'));
    assert.ok(!scene.events.some((event) => event.startsWith('U:')));
    // 다른 풀의 효과 모듈은 거부한다.
    const other = new Scene();
    other.prepare([100, floatToBits(20), floatToBits(21), 3, 3, 3, 0, 0, 1, 2, 0, 3, 0, 0, 0]);
    assert.ok(throws(() => makeConstructionProcessHooks(scene.pool, reward, other.effects(), {})));
});
