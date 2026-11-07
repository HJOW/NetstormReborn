// raw 풀 위의 다리 붕괴 raw-bridge-decay.js(RawBridgeDecay 한 칸/수명, RawBridgeDecayScan 프레임 스캔)를 세 PE의 x86 관측과 대조한다.
//  - bridgedecay-x86.tsv: Cell 4241(한 칸 사건·결과)과 Life 3264(수명 7 초과 400행 포함: raw 4비트 쓰기 그대로)를 실제 슬롯의 float 이웃/WORD/프레임 쓰기로 재생,
//  - bridge-decay-checking-x86.tsv: 272입력의 진단(0x268) 접두·복귀 뒤 현재 WORD 재조회·중단,
//  - bridge-decay-scan-runtime-x86.tsv: 73행의 호출 순서·가변 범위·기한 비트(주기/끝 번호/커서/기한을 칸 효과가 바꾸는 6종 변경).
// 기대값은 계산하지 않고 입력만 구성한다. 표면 지도(hash0)는 원문 fixture처럼 발자국 전체를 번호로 채운 명시 입력이다. (2026-11-08 WB04-E45: RawBridgeDecay/Scan 이식과 함께 추가)
import assert from 'node:assert/strict';
import path from 'node:path';
import test from 'node:test';
import { bridgeDestroyProceeds, DEFAULT_BRIDGE_DECAY_MODE } from '../www/core/bridge-decay.js';
import { RawBridgeDecay, RawBridgeDecayScan } from '../www/core/raw-bridge-decay.js';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { REPO_ROOT } from './helpers.mjs';
import { floatToBits, get, put, readRows } from './raw-support.mjs';

// 표면/다리 타입 플래그와 공통 슬롯 vtable, 풀 크기이다. (2026-11-08 WB04-E45)
const SURFACE = 0x800;
const ISLAND = 2;
const BRIDGE = 4;
const BRIDGE_VTABLE = 0x5034c8;
const CAPACITY = 24000;

// fixture 파일 경로와 전체 행이다. (2026-11-08 WB04-E45)
const fixture = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name));
const decayRows = fixture('bridgedecay-x86.tsv');

// 기대값의 Frames 행(실제 bridge.type 프레임 코드)을 코드 배열로 읽는다. (2026-11-08 WB04-E45)
function framesOf(table) {
    const row = decayRows.find((item) => item[0] === 'Frames' && item[1] === table);
    const codes = [];
    // 네 바이트 코드마다 원본 순서를 유지한다.
    for (let index = 0; index + 8 <= row[2].length; index += 8) {
        const byte = (offset) => parseInt(row[2].slice(index + offset * 2, index + offset * 2 + 2), 16);
        codes.push({ side: byte(0), variant: byte(1), number: byte(2), flags: byte(3) });
    }
    return codes;
}

// "a,b,c;d,e,f" 형식의 정수 묶음, 입력 객체 13개 정수 필드이다. (2026-11-08 WB04-E45)
const tuples = (text) => (text === '-' || text === '' ? [] : text.split(';').map((entry) => entry.split(',').map(Number)));
const inputsOf = (text) => tuples(text).map((v) => ({ id: v[0], x: v[1], y: v[2], width: v[3], height: v[4], flags1: v[5] >>> 0, flags2: v[6] >>> 0, kind: v[7], frame: v[8], state: v[9], extra: v[10], graph: v[11], word: v[12] }));

// 진단 문자열 바이트를 16진수로 직렬화한다. (2026-11-08 WB04-E45)
const asciiHex = (text) => Array.from(text, (char) => char.charCodeAt(0).toString(16).padStart(2, '0')).join('');

// 풀은 장면마다 새로 만들지 않고 입력 슬롯만 지우고 쓴다(실제 소유자는 풀 하나이고 지운 슬롯은 새 풀과 같다). (2026-11-08 WB04-E45)
const sharedPool = new SidPool(CAPACITY, { server: true });

// 기존 실제 PE의 외부 효과 경계를 동일하게 유지한 raw 입력 장면이다: 입력 순서대로 70부터 타입을 설치한다. (2026-11-08 WB04-E45)
class RawDecayScene {
    constructor(inputs, codes, recordText, spotText, mode) {
        this.inputs = inputs;
        this.pool = sharedPool;
        this.types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 1, footY: 1 }));
        this.frames = Array.from({ length: 188 }, () => []);
        this.surface = new SquidHash();
        this.spots = new Uint8Array(65536);
        this.graphs = Array.from({ length: 255 }, () => ({ surfaces: 0, inUse: 0, reserved: 0 }));
        this.mode = mode;
        this.events = [];
        // 원문에 지정한 Graph 레코드만 쓴다.
        for (const record of tuples(recordText)) this.graphs[record[0]].surfaces = (record[1] << 16) >> 16;
        // 현재 bit 위치를 그대로 보존한다.
        for (const spot of tuples(spotText)) this.spots[spot[1] * 256 + spot[0]] = spot[2];
        // 객체마다 원문처럼 다른 TYPE 번호를 사용한다.
        inputs.forEach((entry, index) => {
            const type = 70 + index;
            this.types[type] = { flags1: entry.flags1, flags2: entry.flags2, footX: entry.width, footY: entry.height };
            this.frames[type] = entry.kind === 0 ? codes : [{ side: 0x41, variant: 0x41, number: 1, flags: 0 }];
            const raw = this.pool.physical(entry.id);
            raw.fill(0);
            put(raw, 0, BRIDGE_VTABLE, 4);
            raw[10] = type;
            raw[11] = entry.state;
            put(raw, 12, entry.word, 2);
            put(raw, 14, floatToBits(entry.x), 4);
            put(raw, 18, floatToBits(entry.y), 4);
            raw[30] = entry.graph;
            raw[40] = entry.extra;
            put(raw, 36, entry.frame >>> 0, 4);
            // 명시 원문 지도는 각 발자국 전체를 번호로 채운 입력이다.
            for (let y = entry.y - entry.height + 1; y <= entry.y; y += 1) for (let x = entry.x - entry.width + 1; x <= entry.x; x += 1) this.surface.setCell(0, x, y, entry.id);
        });
    }

    // 사건을 호출 순서대로 모은다. 수명/프레임 결과는 이 기록기로 만들지 않는다.
    event(value) { this.events.push(value); }

    // 원문도 외부 기본 삭제/이동체/소리/표시만 stub 했고 새 raw 실행기도 같은 경계를 공급한다.
    hooks() {
        const floatAt = (raw, offset) => new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getFloat32(offset, true);
        return {
            destroy: (sid, flags) => {
                const raw = this.pool.slot(sid);
                const graph = raw[30];
                if (graph === 254) throw new Error('원문 삭제의 무효 Graph 입력');
                const code = this.frames[raw[10]][get(raw, 36, 4)];
                if (!bridgeDestroyProceeds(this.mode, raw[40], this.graphs[graph].surfaces, code.flags)) return;
                this.event(`D:${sid}:${flags}`);
                this.pool.physical(sid)[11] |= 2;
            },
            notifySurface: () => {},
            refresh: (sid) => this.event(`R:${sid}`),
            carriers: (sid) => {
                const raw = this.pool.slot(sid);
                this.event(`C:${Math.trunc(floatAt(raw, 14))}:${Math.trunc(floatAt(raw, 18))}`);
            },
            crackSound: (sid) => {
                const raw = this.pool.slot(sid);
                this.event(`S:${get(raw, 14, 4)}:${get(raw, 18, 4)}`);
            },
        };
    }

    // 실제 변경된 네 필드를 입력 순서대로 직렬화하며 기대 열은 수정하지 않는다.
    after() {
        return this.inputs.map((entry) => {
            const raw = this.pool.slot(entry.id);
            return `${entry.id},${get(raw, 12, 2)},${get(raw, 36, 4)},${raw[11]}`;
        }).join(';');
    }

    // 기대 사건 표기('-'는 빈 문자열)와 비교할 현재 사건 문자열이다.
    eventText() { return this.events.join(' '); }

    // 실행기를 만든다. options.checking 은 현재 DWORD 를 돌려주는 함수이다.
    decay(hooks = this.hooks(), options = {}) {
        return new RawBridgeDecay(this.pool, this.types, this.frames, this.surface, this.spots, this.graphs, this.mode, hooks, options);
    }
}

test('raw-bridge-decay: 원문 4241행의 한 칸 결과를 실제 raw float 이웃/WORD/프레임 쓰기로 재생한다', () => {
    const codes = framesOf('0');
    let count = 0;
    // 원본 기대값의 모든 Cell 행을 사용하고 새 golden 은 만들지 않는다.
    for (const row of decayRows.filter((item) => item[0] === 'Cell')) {
        const inputs = inputsOf(row[1]);
        const scene = new RawDecayScene(inputs, codes, row[2], row[3], { server: row[4] === '1', authority: true, editor: false, debugRedraw: row[5] === '1', debugKeep: row[6] === '1' });
        scene.decay().cell(Number(row[7]));
        assert.equal(scene.eventText(), row[8] === '-' ? '' : row[8], `Cell ${count} 사건`);
        assert.equal(scene.after(), row[9], `Cell ${count} 상태`);
        count += 1;
    }
    assert.equal(count, 4241);
});

test('raw-bridge-decay: 정상 7 초과 400행을 포함한 Life 3264행을 원문의 실제 4비트 쓰기 그대로 대조한다', () => {
    const codes = framesOf('0');
    let count = 0;
    // 모든 Life 관측을 수명 값의 크기로 생략하지 않는다.
    for (const row of decayRows.filter((item) => item[0] === 'Life')) {
        const inputs = inputsOf(row[1]);
        const scene = new RawDecayScene(inputs, codes, row[2], '-', { server: row[3] === '1', authority: true, editor: false, debugRedraw: row[4] === '1', debugKeep: row[5] === '1' });
        scene.decay().applyLife(inputs[0].id, Number(row[6]));
        assert.equal(scene.eventText(), row[7] === '-' ? '' : row[7], `Life ${count} 사건`);
        assert.equal(scene.after(), row[8], `Life ${count} 상태`);
        count += 1;
    }
    assert.equal(count, 3264);
});

test('raw-bridge-decay: checking 관측 272입력의 진단 접두·복귀 뒤 현재 WORD 재조회·중단을 대조한다', () => {
    const codes = framesOf('0');
    let count = 0;
    // 원본 결과 행을 수정하지 않고 모든 272 입력을 재생한다.
    for (const row of fixture('bridge-decay-checking-x86.tsv')) {
        assert.equal(row.length, 8);
        const p = row[0].split(',');
        assert.equal(p.length, 5);
        const inputs = inputsOf(row[1]);
        const scene = new RawDecayScene(inputs, codes, row[2], '-', { ...DEFAULT_BRIDGE_DECAY_MODE });
        let checking = Number(p[2]) >>> 0;
        const root = Number(row[3]);
        const hooks = scene.hooks();
        let outcome = 'ok';
        // callback 의 현재 WORD 변경도 복귀 뒤 같은 슬롯에서 다시 읽는다.
        hooks.assertion = (condition, source, line) => {
            scene.event(`D:${asciiHex(condition)}:${asciiHex(source)}:${line}`);
            if (p[4] === '1') {
                checking = 0;
                const raw = scene.pool.physical(root);
                put(raw, 12, (get(raw, 12, 2) & ~0x78) | 8, 2);
            }
            if (p[3] === '0') throw new Error('assertion');
        };
        try {
            scene.decay(hooks, { checking: () => checking }).cell(root);
        } catch (failure) {
            outcome = failure.message;
        }
        assert.equal(outcome, row[4], `checking ${count} 상태`);
        assert.equal(String(checking), row[5], `checking ${count} 최종 DWORD`);
        assert.equal(scene.eventText(), row[6] === '-' ? '' : row[6], `checking ${count} 사건`);
        assert.equal(scene.after(), row[7], `checking ${count} 결과`);
        count += 1;
    }
    assert.equal(count, 272);
});

test('raw-bridge-decay: 실행기를 만든 뒤 바뀐 전체 checking DWORD 와 누락된 진단 장치는 쓰기 전에 실패한다', () => {
    const codes = framesOf('0');
    const frame = codes.findIndex((code) => code.side === 0x4b && code.number === 1);
    assert.ok(frame >= 0);
    const inputs = [
        { id: 6100, x: 99, y: 100, width: 1, height: 1, flags1: SURFACE, flags2: ISLAND, kind: 1, frame: 0, state: 0, extra: 0, graph: 1, word: 0 },
        { id: 6101, x: 100, y: 100, width: 1, height: 1, flags1: SURFACE, flags2: BRIDGE, kind: 0, frame, state: 0, extra: 0, graph: 1, word: 9 << 3 },
    ];
    const scene = new RawDecayScene(inputs, codes, '1,9,1', '-', { ...DEFAULT_BRIDGE_DECAY_MODE });
    let checking = 0;
    const decay = scene.decay(scene.hooks(), { checking: () => checking });
    decay.cell(6101);
    assert.equal(get(scene.pool.slot(6101), 12, 2) & 0x78, 6 << 3);
    put(scene.pool.physical(6101), 12, 9 << 3, 2);
    checking = 0x80000000;
    scene.events.length = 0;
    // 선택 진단이 미연결이면 수명 쓰기 전 실패한다.
    assert.throws(() => decay.cell(6101), /미연결/);
    assert.equal(get(scene.pool.slot(6101), 12, 2), 9 << 3);
    assert.equal(scene.events.length, 0);
    checking = 0;
    decay.cell(6101);
    assert.equal(get(scene.pool.slot(6101), 12, 2) & 0x78, 6 << 3);
    // 구성 오류는 거부한다.
    assert.throws(() => new RawBridgeDecay(scene.pool, scene.types.slice(1), scene.frames, scene.surface, scene.spots, scene.graphs, scene.mode, scene.hooks()), TypeError);
    assert.throws(() => new RawBridgeDecay(scene.pool, scene.types, scene.frames, scene.surface, scene.spots, scene.graphs.slice(1), scene.mode, scene.hooks()), TypeError);
});

// 스캔 장면: 15000 번대 일곱 슬롯의 상태(free·dead·void)·extra(abstract·buried)와 마지막 타입의 bridge genus 부재를 입력으로 넣는다. (2026-11-08 WB04-E45)
function scanScene(pool, types) {
    const states = [0, 1, 2, 4, 0, 0, 0];
    const extras = [0, 0, 0, 0, 1, 8, 0];
    // 일곱째는 상태는 정상이어도 bridge genus 가 없는 타입이다.
    for (let index = 0; index < 7; index += 1) {
        const raw = pool.physical(15000 + index);
        raw.fill(0);
        raw[10] = 70 + index;
        raw[11] = states[index];
        raw[40] = extras[index];
        types[70 + index].flags2 = index === 6 ? 0 : BRIDGE;
    }
}

const bitsView = new DataView(new ArrayBuffer(8));
const doubleOf = (text) => {
    bitsView.setBigUint64(0, BigInt(text));
    return bitsView.getFloat64(0);
};
const bitsOf = (value) => {
    bitsView.setFloat64(0, value);
    return bitsView.getBigUint64(0);
};

test('raw-bridge-decay-scan: 73행의 호출 순서·가변 범위·기한 비트를 대조한다(기본 53비트와 64비트가 같았던 모든 행)', () => {
    let count = 0;
    // 주석 이외의 모든 원문 관측을 빠짐없이 소비한다.
    for (const v of fixture('bridge-decay-scan-runtime-x86.tsv')) {
        assert.equal(v.length, 17);
        const pool = new SidPool(CAPACITY, { server: true });
        const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 1, footY: 1 }));
        scanScene(pool, types);
        const integer = (column) => Number(v[column]) | 0;
        const periodView = new DataView(new ArrayBuffer(4));
        periodView.setUint32(0, Number(v[1]) >>> 0);
        let period = periodView.getFloat32(0);
        const state = { first: integer(2), last: integer(3), cursor: integer(4), next: doubleOf(v[5]) };
        const mode = { editor: integer(9) !== 0, authority: integer(10) !== 0 };
        const time = { game: doubleOf(v[6]), delta: doubleOf(v[7]) };
        const visits = [];
        const scan = new RawBridgeDecayScan(pool, types, state, () => period, mode, (sid) => {
            visits.push(sid);
            if (visits.length !== 1) return;
            // 원문 관측과 같은 명시 외부 효과만 설치한다.
            switch (integer(11)) {
            case 1: period = 0.25; break;
            case 2: state.last = 15002; break;
            case 3: state.cursor = 15003; break;
            case 4: state.first = 15002; state.next = -2; break;
            default: break;
            }
        });
        scan.frame(time, integer(8) !== 0);
        // 기대 SID 는 원문의 호출 기록에서만 읽는다.
        const wanted = v[12] === '-' ? [] : v[12].split(',').map(Number);
        assert.deepEqual(visits, wanted, `scan ${count} 방문`);
        assert.equal(state.first, integer(13));
        assert.equal(state.last, integer(14));
        assert.equal(state.cursor, integer(15));
        assert.equal(bitsOf(state.next), BigInt(v[16]), `scan ${count} 다음 시각`);
        count += 1;
    }
    assert.equal(count, 73);
});

test('raw-bridge-decay-scan: 구성 오류를 거부하고 칸 예외는 삼키며 정지·편집기·권한 없음은 무변경이다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 1, footY: 1 }));
    scanScene(pool, types);
    const state = { first: 15000, last: 15006, cursor: 15000, next: 10 };
    assert.throws(() => new RawBridgeDecayScan(pool, types.slice(1), state, () => 10, {}, () => {}), TypeError);
    assert.throws(() => new RawBridgeDecayScan(pool, types, state, () => 10, {}, null), TypeError);
    const visits = [];
    const scan = new RawBridgeDecayScan(pool, types, state, () => 10, { editor: false, authority: true }, (sid) => {
        visits.push(sid);
        throw new Error('칸 처리 실패');
    });
    scan.frame({ game: 20, delta: 0.014 }, false);
    // 기한이 지난 프레임은 범위 전체를 훑고 칸 예외는 다음 번호 진행을 막지 않는다. free/dead/void/abstract(1)/비다리는 칸 처리에 닿지 않고 buried(8) 슬롯은 방문 대상이다.
    assert.deepEqual(visits, [15000, 15005]);
    assert.equal(state.cursor, 15000);
    assert.equal(state.next, 30);
    const before = { ...state };
    new RawBridgeDecayScan(pool, types, state, () => 10, { editor: true, authority: true }, () => visits.push(-1)).frame({ game: 99, delta: 1 }, false);
    new RawBridgeDecayScan(pool, types, state, () => 10, { editor: false, authority: false }, () => visits.push(-1)).frame({ game: 99, delta: 1 }, false);
    scan.frame({ game: 99, delta: 1 }, true);
    assert.deepEqual(state, before);
    assert.ok(!visits.includes(-1));
});
