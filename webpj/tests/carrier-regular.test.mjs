// 골렘 Regular raw-carrier-regular.js(RawCarrierRegular)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: carrier-regular-x86.tsv 837행(두 x87 정밀도 동일) — 입력 16 DWORD(사건·개수·payload·내용물 존재 비트·새 TYPE flags2 둘·casttime·좌표 종류·타이머 할당·boss·회피/이동/경로·외부 변경 종류·Damageable/낙하 반환 비트)
//    → 반환 float 비트, 외부 효과 호출 순서(F 조회/C 생성/W 단어/O 소유자/P·X Pop/A 타이머 할당/T 타이머/R 갱신/E·M·H·V 회피/D Damageable/B 낙하), 여섯 슬롯(0·50·80·81·90·91)의 전체 raw.
//    carrier-regular-vtables.tsv 83행(보호 PE 의 실제 +5c 값) — 직접 분배 6·파생 fallback 77.
//  - 계약: CarrierRegular_LazyBoundariesAndEdition(내용물 없음/만료는 생성/표시 경계를 요구하지 않고 기본/낙하 경계 누락과 CD 풀을 거부).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E91: Carrier Regular 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawCarrierRegular } from '../www/core/raw-carrier-regular.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E91)
const fixtures = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const rows = readRows(path.join(fixtures, 'carrier-regular-x86.tsv'));
const vtableRows = readRows(path.join(fixtures, 'carrier-regular-vtables.tsv'));

// 원본 관찰의 명시 가상 효과 경계 표, 관찰 슬롯, 독립 입력 수이다. (2026-11-08 WB04-E91)
const PARENT_VT = 0x20000800;
const GHOST_VT = 0x20000900;
const OTHER_VT = 0x20000a00;
const WATCH = Object.freeze([0, 50, 80, 81, 90, 91]);
const ROWS = 837;
const VTABLE_ROWS = 83;

// 예외가 나는지 본다. (2026-11-08 WB04-E91)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E91)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// Native 와 같은 입력/큰 하위 효과만 공급하는 장면이다. 부모의 분기·좌표·프레임은 제품 코드가 실행한다. (2026-11-08 WB04-E91)
class CarrierRegularScene {
    constructor() {
        this.pool = new SidPool(24000);
        this.types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0 }));
        this.details = Array.from({ length: 188 }, () => ({ defaultFrame: 0, castTime: 0 }));
        this.row = [];
        this.boss = false;
        this.created = 0;
        this.trace = [];
    }

    // 원본 몸체가 허용하는 물리 슬롯을 추가 상태 필터 없이 연다(free/dead 독립 입력 포함).
    raw(sid) { return this.pool.physical(sid); }

    // unsigned 인자만 원문 형식으로 남긴다.
    event(name, values = []) { this.trace.push(values.reduce((text, value) => `${text}:${value >>> 0}`, name)); }

    // 매 입력마다 관측한 여섯 raw 와 현재 TYPE 를 독립 초기화한다.
    prepare(input) {
        assert.equal(input.length, 16);
        this.row = input;
        this.trace = [];
        this.created = 0;
        this.boss = input[9] !== 0;
        // 무관한 바이트는 a5 로 보존해 원문 필드 쓰기 폭을 검사한다.
        for (const sid of WATCH) this.raw(sid).fill(0xa5);
        put(this.raw(0), 18, 102, 2);
        const parent = this.raw(50);
        put(parent, 0, PARENT_VT);
        parent[10] = 158;
        parent[34] = 1;
        parent[35] = 0;
        // 좌표 상하한/NaN 의 명시 입력이다.
        const points = [[floatToBits(10.5), floatToBits(20.25)], [0x7fc01234, floatToBits(-10.0)], [0x7f800000, floatToBits(255.9)], [floatToBits(-5.0), 0x7fc01234]];
        put(parent, 14, points[input[7]][0]);
        put(parent, 18, points[input[7]][1]);
        // 두 내용물의 종류/단어 입력이며 실제 조회는 find 경계이다.
        for (let index = 0; index < 2; index += 1) {
            const raw = this.raw(80 + index);
            raw[10] = 3;
            put(raw, 18, 100 + index, 2);
            put(raw, 12, index === 0 ? 0xa123 : 0x8001, 2);
        }
        // create 가 바꿀 수 있는 새 TYPE 두 개도 별도로 초기화한다.
        for (let kind = 100; kind < 104; kind += 1) {
            this.details[kind] = { defaultFrame: kind - 93, castTime: input[6] | 0 };
            this.types[kind].flags2 = input[kind === 100 ? 4 : 5];
        }
    }

    // 원본이 호출한 큰 경계에서만 명시 현재 상태를 바꾼다.
    hooks() {
        const row = () => this.row;
        return {
            find: (parent, mask, kind, index) => {
                this.event('F', [parent, mask, kind, index]);
                const found = mask === 0x20 && (row()[3] & 1) ? 80 : mask === 1 && (row()[3] & 2) ? 81 : 0;
                if (row()[13] === 5) this.raw(50)[35] = 17;
                return found;
            },
            create: (kind, flags) => {
                const sid = 90 + this.created;
                this.created += 1;
                this.event('C', [kind, flags, sid]);
                const raw = this.raw(sid);
                put(raw, 0, GHOST_VT);
                raw[10] = kind + (row()[13] === 1 ? 2 : 0);
                raw[11] = 4;
                if (row()[13] === 1) this.raw(50)[35] = 5;
                return sid;
            },
            setWord: (sid, value) => { this.event('W', [sid, value]); put(this.raw(sid), 12, value, 2); if (row()[13] === 2) this.raw(50)[34] = 7; },
            setOwner: (sid, owner) => {
                this.event('O', [sid, owner]);
                this.raw(sid)[34] = owner;
                if (row()[13] === 3) {
                    put(this.raw(50), 14, floatToBits(30.5));
                    put(this.raw(50), 18, floatToBits(40.75));
                    put(this.raw(sid), 0, OTHER_VT);
                }
            },
            pop: (sid, x, y, flags) => {
                this.event(get(this.raw(sid), 0) === OTHER_VT ? 'X' : 'P', [sid, floatToBits(x), floatToBits(y), flags]);
                put(this.raw(sid), 14, floatToBits(x));
                put(this.raw(sid), 18, floatToBits(y));
                if (row()[13] === 4) this.types[101].flags2 = 0;
            },
            allocateTimer: () => { this.event('A', [24]); return row()[8] !== 0; },
            constructTimer: (sid, duration) => { this.event('T', [sid, floatToBits(duration)]); },
            refresh: (sid) => { this.event('R', [sid]); if (row()[13] === 6) this.raw(50)[40] = 0x12; },
            needsEscape: (x, y) => { this.event('E', [floatToBits(x), floatToBits(y)]); return row()[10] !== 0; },
            movementType: (type) => { this.event('M', [type, 0, 0]); return row()[11]; },
            hasPath: (sid, type) => { this.event('H', [sid, type]); return row()[12] !== 0; },
            escape: (sid, delay, event) => { this.event('V', [sid, floatToBits(delay), event]); },
            damageable: (sid, event, count, payload) => {
                this.event('D', [sid, event, count, floatToBits(payload)]);
                if (row()[13] === 7) this.raw(50)[40] = 0x33;
                return bitsToFloat(row()[14]);
            },
            falling: (sid, count) => {
                this.event('B', [sid, count]);
                if (row()[13] === 7) this.raw(50)[40] = 0x33;
                return bitsToFloat(row()[15]);
            }
        };
    }

    // 현재 장면으로 처리기를 만든다.
    logic(hooks = this.hooks()) { return new RawCarrierRegular(this.pool, this.types, this.details, () => this.boss, hooks); }

    // 여섯 물리 50바이트를 원문 순서로 합친다.
    image() { return WATCH.map((sid) => hex(this.raw(sid))).join(''); }
}

test('CarrierRegular: 원본 10.78 837입력의 전체 부모·좌표 제한·현재 raw 재조회·반환 비트·효과 순서가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new CarrierRegularScene();
    let count = 0;
    // 새 837 입력의 모든 효과/바이트를 비교한다.
    for (const fields of rows) {
        assert.equal(fields.length, 7);
        scene.prepare(fields[0].split(',').map(Number));
        const logic = scene.logic();
        const result = logic.handle(50, scene.row[0], scene.row[1], bitsToFloat(scene.row[2]));
        const label = `입력 ${fields[0]}`;
        assert.equal(floatToBits(result), Number(fields[1]), label);
        assert.equal(scene.trace.length === 0 ? '-' : scene.trace.join(';'), fields[2], label);
        assert.equal(scene.image(), fields[3], label);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('CarrierRegular: 보호 PE 의 실제 +5c 83개 가상 표에서 직접 분배 6·파생 fallback 77 이다', { skip: !vtableRows && '원본 fixture 없음' }, () => {
    const scene = new CarrierRegularScene();
    scene.prepare([0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 54, 0, 0, 0x3e4ccccd, 0]);
    const logic = scene.logic();
    let direct = 0;
    let count = 0;
    let fallbacks = 0;
    const handler = logic.handler((sid, event, invocation, payload) => {
        assert.ok(sid === 50 && event === 0 && invocation === 0 && payload === 1.0);
        fallbacks += 1;
        return 3.0;
    });
    // 각 줄은 보호 PE 의 실제 +5c DWORD 이며 제품의 분배표로 생성하지 않는다.
    for (const fields of vtableRows) {
        assert.equal(fields.length, 2);
        put(scene.raw(50), 0, Number(fields[0]));
        const expected = Number(fields[1]) === 0x42a750;
        assert.equal(logic.handles(50), expected);
        assert.equal(handler(50, 0, 0, 1.0), expected ? Math.fround(0.2) : 3.0);
        if (expected) direct += 1;
        count += 1;
    }
    assert.ok(count === VTABLE_ROWS && direct === 6 && fallbacks === 77);
});

test('CarrierRegular: 내용물 없음/만료는 생성/표시 경계를 요구하지 않고 기본/낙하 경계 누락과 CD 풀을 거부한다', () => {
    const scene = new CarrierRegularScene();
    scene.prepare([0x491, 0, 0x3e4ccccd, 0, 0, 0, 0, 0, 1, 1, 0, 54, 0, 0, 0, 0]);
    const hooks = scene.hooks();
    const logic = scene.logic({ find: hooks.find });
    assert.equal(logic.handle(50, 0x491, 0, Math.fround(0.2)), 0);
    assert.equal(logic.handle(50, 0x491, 100, Math.fround(0.2)), 0);
    assert.ok(throws(() => logic.handle(50, 0, 0, Math.fround(0.2))));
    assert.ok(throws(() => logic.handle(50, 0x5105, 0, Math.fround(0.2))));
    assert.ok(throws(() => new RawCarrierRegular({ edition: 'cd1072' }, scene.types, scene.details, () => false, {})));
    assert.ok(throws(() => new RawCarrierRegular(scene.pool, scene.types, scene.details, true, {})));
    assert.equal(logic.pool, scene.pool);
});
