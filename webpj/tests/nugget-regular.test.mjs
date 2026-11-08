// 너겟 Regular raw-nugget-regular.js(RawNuggetRegular)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: nugget-regular-x86.tsv(10.78 전용; x87 정밀도 53/64 두 관측 중 기본 CRT 53비트 열) 9019행 — 모드(0 Handle / 1 CanFall)·사건·개수·payload·프레임·A 구간 프레임 수·지도 xor/패턴·좌표·권한·상태·예약 성공·난수 시드·변이·지도 모양 →
//    반환 DWORD(float 비트), 호출 순서(F 프레임 / R·X 갱신 / A 예약 / I 난수 / C 생성 / D 삭제 / U·P·Y 공간 / B Damageable), 슬롯 50 raw, 난수 상태.
//  - 계약: NuggetRegular_LazyBoundariesAndExactVtable·NuggetMenuCurrent_RegularUsesActualVtableAndCurrentTypeFrames(초기 count 가 하위 장치를 요구하지 않음, 정확한 가상 표만 분배, 현재 TYPE 의 A 구간).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E72: 너겟 Regular 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, floatToBits, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { RawNuggetRegular } from '../www/core/raw-nugget-regular.js';

// 원본 관측 행이다(단일 판본 파일). 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E72)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'nugget-regular-x86.tsv'));

// 원문 관측의 현재 가상 표, 콜백이 바꾸는 명시 가상 표, 원본 독립 입력 수, 관찰 SID 이다. (2026-11-08 WB04-E72)
const NATIVE_VT = 0x50dc30;
const CHANGED_VT = 0x20000800;
const ROWS = 9019;
const SOURCE = 50;

// 예외가 나는지 본다. (2026-11-08 WB04-E72)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E72)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 현재 물리 raw/점유 지도/난수와 명시 하위 효과만 공급하는 장면이다. (2026-11-08 WB04-E72)
class NuggetScene {
    constructor() {
        this.pool = new SidPool(24000, { server: true });
        this.counts = new Array(188).fill(0);
        this.spots = new Uint8Array(65536);
        this.row = [];
        this.random = new GameRandom();
        this.boss = false;
        this.trace = [];
        this.spotKey = -1;
    }

    // 원본이 직접 읽는 물리 슬롯을 상태 필터 없이 빌린다.
    raw() {
        return this.pool.physical(SOURCE);
    }

    // 원문 경계의 전체 DWORD 인자를 보존한다.
    event(name, values) {
        this.trace.push([name, ...values].join(':'));
    }

    // 지도를 입력으로 채울 뿐 조회 좌표의 기대 색인을 계산하지 않는다.
    prepare(input) {
        this.row = input;
        assert.equal(input.length, 15);
        this.boss = input[9] !== 0;
        this.random.setState(input[12]);
        this.trace = [];
        const raw = this.raw();
        raw.fill(0xa5);
        put(raw, 0, NATIVE_VT);
        raw[10] = 154;
        raw[11] = input[10];
        put(raw, 14, input[7]);
        put(raw, 18, input[8]);
        put(raw, 36, input[4]);
        this.counts[154] = input[5] | 0;
        // 원문과 같은 고정 패턴/xor 또는 균일 BYTE 입력이다. 같은 입력(패턴 여부, xor)이면 다시 채우지 않는다.
        const key = input[14] * 0x100000000 + input[6];
        if (this.spotKey !== key) {
            for (let index = 0; index < this.spots.length; index += 1) this.spots[index] = (((input[14] ? ((index * 17) ^ ((index >> 8) * 93)) : 0) ^ input[6]) & 0xff);
            this.spotKey = key;
        }
    }

    // 프레임/공간/삭제의 큰 경계만 명시 구현하고 부모 계산은 제품 몸체를 사용한다.
    hooks() {
        return {
            setFrame: (sid, frame, flags) => {
                this.event('F', [sid, frame >>> 0, flags]);
                put(this.raw(), 36, frame >>> 0);
                if (this.row[13] === 1) { put(this.raw(), 0, CHANGED_VT); this.raw()[10] = 155; }
            },
            refresh: (sid) => { this.event(get(this.raw(), 0) === CHANGED_VT ? 'X' : 'R', [sid]); },
            allocate: () => { this.event('A', [40]); return this.row[11] !== 0; },
            randomInteger: (low, high) => { this.event('I', [low >>> 0, high >>> 0]); return low + this.random.next(high - low); },
            construct: (sid, event, payload) => { this.event('C', [event, sid, floatToBits(payload)]); },
            destroy: (sid, flags) => { this.event('D', [sid, flags]); this.raw()[11] |= 2; },
            unpop: (sid, flags) => {
                this.event('U', [sid, flags]);
                this.raw()[11] |= 4;
                if (this.row[13] === 2) { put(this.raw(), 0, CHANGED_VT); put(this.raw(), 14, floatToBits(42.5)); put(this.raw(), 18, floatToBits(43.5)); }
            },
            pop: (sid, x, y, flags) => {
                this.event(get(this.raw(), 0) === CHANGED_VT ? 'Y' : 'P', [sid, floatToBits(x), floatToBits(y), flags]);
                put(this.raw(), 14, floatToBits(x));
                put(this.raw(), 18, floatToBits(y));
                this.raw()[11] &= ~4 & 0xff;
            },
            damageable: (sid, event, count, payload) => { this.event('B', [sid, event, count, floatToBits(payload)]); return bitsToFloat(this.row[3]); }
        };
    }
}

test('NuggetRegular: 원본 10.78 9019행의 반환·왕복 프레임/낙하/지면/난수 효과 순서·슬롯 raw·난수 상태가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new NuggetScene();
    let count = 0;
    // 기본 CRT 의 53비트 원문을 모두 재생한다.
    for (const fields of rows) {
        assert.equal(fields.length, 9);
        scene.prepare(fields[0].split(',').map(Number));
        const logic = new RawNuggetRegular(scene.pool, scene.counts, scene.spots, () => scene.boss, scene.hooks());
        const result = scene.row[0] === 0 ? floatToBits(logic.handle(SOURCE, scene.row[1], scene.row[2], bitsToFloat(scene.row[3]))) : Number(logic.canFall(SOURCE));
        assert.equal(result, Number(fields[1]), `행 ${count}: 입력 ${fields[0]}`);
        assert.equal(scene.trace.length === 0 ? '-' : scene.trace.join(';'), fields[2], `행 ${count}: 입력 ${fields[0]}`);
        assert.equal(hex(scene.raw()), fields[3], `행 ${count}: 입력 ${fields[0]}`);
        assert.equal(scene.random.state, Number(fields[4]), `행 ${count}: 입력 ${fields[0]}`);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('NuggetRegular: 초기 count 와 정상 분배, 명시 늦은 효과 누락은 서로 구별한다', () => {
    const scene = new NuggetScene();
    scene.prepare([0, 0, 1, 0x3e99999a, 1, 4, 0, 0x41a40000, 0x41aa0000, 1, 0, 1, 0, 0, 0]);
    const logic = new RawNuggetRegular(scene.pool, scene.counts, scene.spots, () => scene.boss, {});
    assert.ok(logic.handles(SOURCE));
    assert.equal(logic.handle(SOURCE, 2, 0, Math.fround(0.3)), Math.fround(0.3));
    put(scene.raw(), 0, 0xdeadbeef);
    assert.ok(!logic.handles(SOURCE));
    put(scene.raw(), 0, NATIVE_VT);
    scene.raw()[10] = 155;
    assert.ok(logic.handles(SOURCE));
    assert.ok(throws(() => logic.handle(SOURCE, 2, 1, Math.fround(0.3))));
});

test('NuggetRegular: 실제 가상 표 진입 뒤 현재 TYPE 의 A 구간을 사용하며 초기 count 는 하위 장치를 요구하지 않는다', () => {
    const scene = new NuggetScene();
    scene.prepare([0, 1, 1, 0x3e800000, 1, 99, 6, 0x41a40000, 0x41aa0000, 1, 0, 0, 0, 0, 0]);
    scene.raw()[10] = 70;
    scene.counts[70] = 3;
    let frames = 0;
    let refreshes = 0;
    let reservations = 0;
    const hooks = {
        setFrame: (sid, frame, flags) => { assert.ok(sid === SOURCE && frame === 2 && flags === 0); frames += 1; put(scene.raw(), 36, frame >>> 0); },
        refresh: (sid) => { assert.ok(sid === SOURCE && scene.raw()[10] === 70); refreshes += 1; },
        allocate: () => { reservations += 1; return false; }
    };
    const regular = new RawNuggetRegular(scene.pool, scene.counts, scene.spots, () => scene.boss, hooks);
    assert.ok(regular.handles(SOURCE) && regular.handle(SOURCE, 1, 1, 0) === Math.fround(0.1));
    assert.ok(frames === 1 && refreshes === 1 && get(scene.raw(), 36) === 2);
    assert.ok(regular.handle(SOURCE, 1, 1, 0) === 0 && reservations === 1);
    assert.equal(regular.handle(SOURCE, 2, 0, 0.25), 0.25);
    put(scene.raw(), 0, 0x501dd0);
    scene.raw()[10] = 154;
    // TYPE 154 여도 다른 가상 표는 너겟 분배 대상이 아니다.
    assert.ok(!regular.handles(SOURCE));
    assert.ok(throws(() => new RawNuggetRegular(scene.pool, scene.counts.slice(1), scene.spots, () => scene.boss, hooks)));
    assert.ok(throws(() => new RawNuggetRegular(scene.pool, scene.counts, new Uint8Array(5), () => scene.boss, hooks)));
});
