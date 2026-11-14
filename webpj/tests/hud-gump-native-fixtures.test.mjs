// HUD 보조 Gump 계열의 원본 10.78 독립 관측 대조: 생산 실패 안내(ProductionHintGump, 126행)와 비용 표시(FloatingSpGump, 10.78 정밀도 열)를
// cpppj `ProductionHintGumpTests`·`FloatingSpGumpTests` 와 같은 입력 구성으로 비교한다. 노드 필드(전체 160바이트 일부·파생 필드)와 외부 호출 순서를 원본 관측에 맞춘다.
// 입력 열만 장치에 공급하고 관측 열은 비교에만 쓴다. 원본 게임을 실행하지 않는다. (2026-11-15 WB04-E486)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { GumpCreateState } from '../www/ui/gump-create.js';
import { ProductionHintGump, ProductionHintNode, ProductionHintState } from '../www/ui/production-hint-gump.js';
import { FloatingSpGump, FloatingSpGumpNode } from '../www/ui/floating-sp-gump.js';
import { decodeGumpNode, encodeGumpNode } from './gump-node-support.mjs';

// 탭 구분 fixture 의 머리글/주석 줄을 건너뛴 행 목록이다(빈 열 보존).
function rows(name) { return fs.readFileSync(new URL(`../../cpppj/tests/fixtures/${name}`, import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && !line.startsWith('#')).map((line) => line.split('\t')); }
// float32/float64 와 비트 사이를 오가는 공용 보기이다(NaN 비트를 BigInt/정수로 유지한다).
const f32 = new Float32Array(1); const u32 = new Uint32Array(f32.buffer); const f64 = new Float64Array(1); const u64 = new BigUint64Array(f64.buffer);
const bitsOf = (value) => { f32[0] = value; return u32[0]; }; const floatOf = (bits) => { u32[0] = bits >>> 0; return f32[0]; };
const doubleOf = (bits) => { u64[0] = BigInt.asUintN(64, BigInt(bits)); return f64[0]; };
// 바이트 문자열의 소문자 16진이다.
const textHex = (value) => Buffer.from(value, 'latin1').toString('hex');
// 부호 있는 32비트 해석과 부호 없는 십진 입력이다.
const signed32 = (value) => value | 0;

// 이벤트 문자열을 `;` 로 잇는 기록이다.
class Trace { constructor() { this.parts = []; } add(text) { this.parts.push(text); } text() { return this.parts.join(';'); } }

// 한 행의 생산 안내 장면이다(원본 격리 토큰·canary 입력). 노드의 +a8 만료 시각은 NaN 비트를 잃지 않게 64비트 정수로 보관한다.
class HintScene {
    constructor() {
        this.create = new GumpCreateState(); this.state = new ProductionHintState(); this.node = new ProductionHintNode(); this.trace = new Trace();
        this.now = 3.25; this.owned = ''; this.point = { x: 17, y: -9 }; this.box = { left: 10, top: 20, right: 40, bottom: 56 };
        Object.assign(this.node, decodeGumpNode('a5'.repeat(160)));
        this.node.head = this.node.reserved = this.node.owned = this.node.reference = 0xa5a5a5a5;
        // 만료 시각을 비트 그대로 두 방향으로 오가는 접근자다.
        this.untilBits = 0xa5a5a5a5a5a5a5a5n;
        Object.defineProperty(this.node, 'until', { configurable: true, enumerable: true, get: () => doubleOf(this.untilBits), set: (value) => { f64[0] = value; this.untilBits = u64[0]; } });
        this.create.collection.root = 0x12405000; this.state.lastSerial = 0x76543210;
    }
    // 원본 관측 snapshot 이 실제로 읽는 바이트만 같은 순서로 직렬화한다.
    snapshot() {
        const raw = Buffer.from(encodeGumpNode(this.node), 'hex'); const parts = [];
        for (const at of [0, 4, 0x20, 0x2c, 0x30, 0x3c, 0x40, 0x44, 0x48, 0x4c, 0x50]) parts.push(raw.subarray(at, at + 4));
        parts.push(raw.subarray(0x5c, 0x6c), raw.subarray(0x80, 0xa0));
        const dword = (value) => { const bytes = Buffer.alloc(4); bytes.writeUInt32LE(value >>> 0); return bytes; };
        const time = Buffer.alloc(8); time.writeBigUInt64LE(this.untilBits);
        parts.push(dword(this.node.head !== 0 ? 1 : 0), dword(this.node.reserved), time, dword(this.node.owned !== 0 ? 1 : 0), dword(this.node.reference));
        return Buffer.concat(parts).toString('hex');
    }
    hooks() {
        const kNode = 0x12401000; const kOwned = 0x12403000; const kReference = 0x12405000; const add = (text) => this.trace.add(text);
        return {
            node: (token) => { assert.equal(token, kNode); return this.node; }, now: () => { add('Now'); return this.now; },
            allocate: (size) => { add(`New,${size}`); assert.equal(size, 0xb8); return kNode; },
            constructText: () => { add('Text'); this.node.vtable = 0x5144d4; this.node.flags = 0x400001; this.node.serial = 41; this.node.head = 0; },
            duplicate: (text) => { this.owned = text; add(`Dup,${textHex(text)}`); return kOwned; }, string: (token) => { assert.equal(token, kOwned); return this.owned; },
            free: (token) => { assert.equal(token, kOwned); add('Free,1'); }, kill: () => add('Kill'),
            translate: (text) => { add(`Translate,${textHex(text)}`); return text; },
            style: (token, text) => { add(`Style,${textHex(text)}`); this.node.head = 0x12408000; },
            fit: (token, x, y) => { add(`Fit,${x},${y}`); this.node.baseWidth = this.node.width = 51.25; this.node.baseHeight = this.node.height = 13.5; },
            idle: (token, on) => { add(`Idle,${on ? 1 : 0}`); this.node.flags = (this.node.flags | 0x80) >>> 0; },
            bounds: (token) => { add(`Bounds,${token === kReference ? 1 : 0}`); return this.box; }, position: () => { add('Position'); return this.point; },
            draw: (head, point) => add(`Draw,${head !== 0 ? 1 : 0},${point.x >>> 0},${point.y >>> 0},0`),
            attach: (token, x, y, parent, mode) => { add(`Attach,${bitsOf(x)},${bitsOf(y)},${parent},${mode}`); this.node.x = x; this.node.y = y; },
            deleteText: (token, flags) => { add('TextDelete'); this.node.vtable = 0x50ac70; if (flags & 1) add('Free,2'); return token; },
        };
    }
}

test('ProductionHintGump 원본 10.78 126행: 구성/Idle/Draw/Delete/Show 의 노드 필드·마지막 일련번호·호출 순서가 일치한다', () => {
    const table = rows('production-hint-x86.tsv'); const failures = [];
    assert.equal(table.length, 126);
    const kNode = 0x12401000; const kReference = 0x12405000; const texts = ['', '~0~w~ENot enough Storm Power to build this', '~0~w~ENone in inventory'];
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 5, `행 ${index}`);
        const input = row[1].split(','); const scene = new HintScene(); const gump = new ProductionHintGump(scene.create, scene.state, scene.hooks());
        // 입력 열의 i 번째 값을 명시 double 비트/DWORD 로 읽는다.
        const d = (at) => doubleOf(input[at]); const u = (at) => Number(BigInt.asUintN(32, BigInt(input[at])));
        if (row[0] === 'C') { scene.now = d(0); scene.state.white = u(2); scene.state.black = u(3); scene.state.kind = (u(4) << 24) >> 24; gump.construct(kNode, texts[u(1)]); }
        else if (row[0] === 'I') { scene.node.until = d(0); scene.now = d(1); gump.idle(kNode); }
        else if (row[0] === 'D') { scene.node.head = u(0); scene.point = { x: signed32(u(1)), y: signed32(u(2)) }; gump.draw(kNode); }
        else if (row[0] === 'X') { scene.node.owned = u(0) ? 0x12403000 : 0; assert.equal(gump.deleteScalar(kNode, u(1)), kNode); }
        else { assert.equal(row[0], 'W'); scene.now = d(3); scene.box = u(4) ? { left: -17, top: 20, right: 2147483647, bottom: -2147483648 } : { left: 0, top: 0, right: 36, bottom: 36 }; gump.show(u(0) !== 0, signed32(u(1)), signed32(u(2)), kReference); }
        const problems = [];
        if (scene.snapshot() !== row[2]) problems.push('snapshot');
        if (scene.state.lastSerial !== Number(row[3])) problems.push('serial');
        if (scene.trace.text() !== row[4]) problems.push('trace');
        if (problems.length) failures.push(`${index}/${row[0]}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
});

// 한 행의 비용 표시 장면이다. 노드의 float32/double 필드는 입력 비트를 그대로 둔다.
class FloatingScene {
    constructor() {
        this.create = new GumpCreateState(); this.node = new FloatingSpGumpNode(); this.trace = new Trace(); this.now = 3.25; this.mutate = false;
        this.create.collection.root = 0x12345678; this.create.screenWidth = 640; this.create.screenHeight = 480; this.node.vtable = 0x50688c; this.node.depth = 0xa7a7a7a7;
        this.node.x = this.node.y = this.node.velocity = this.node.acceleration = floatOf(0xa7a7a7a7); this.node.padding = 0xa7a7a7a7; this.node.stamp = doubleOf(0xa7a7a7a7a7a7a7a7n);
    }
    // 원본과 같은 쉼표/DWORD/정확한 double 바이트로 현재 필드를 보고한다.
    snapshot() {
        const node = this.node; f64[0] = node.stamp; const stamp = Buffer.alloc(8); stamp.writeBigUInt64LE(u64[0]);
        return `${node.vtable >>> 0},${node.depth >>> 0},${bitsOf(node.x)},${bitsOf(node.y)},${bitsOf(node.velocity)},${bitsOf(node.acceleration)},${node.padding >>> 0},${stamp.toString('hex')}`;
    }
    hooks() {
        const kNode = 0x12201000; const add = (text) => this.trace.add(text);
        return {
            node: (token) => { assert.equal(token, kNode); return this.node; }, now: () => this.now, allocate: (bytes) => { assert.equal(bytes, 0xb8); add('N'); return kNode; },
            constructText: () => { this.node.vtable = 0x5144d4; add('T'); }, clearText: () => add('C'), fitText: () => add('F'), styledText: (token, text) => add(`S,${text}`), idle: (token, enabled) => add(`I,${enabled ? 1 : 0}`),
            redraw: (token, erase) => { assert.equal(erase, false); const first = this.trace.parts.length === 0; add(`R,${this.snapshot()}`); if (this.mutate && first) { this.node.y = 7.5; this.node.velocity = -1.25; this.node.acceleration = 3; } },
            kill: () => add('K'), attach: (token, x, y, parent, mode) => { add(`A,${bitsOf(x)},${bitsOf(y)},${parent},${mode}`); this.node.x = x; this.node.y = y; }, reference: (token, object) => add(`O,${object}`),
            deleteText: (token, flags) => { this.node.vtable = 0x5144d4; add('C'); add('C'); add('B'); if (flags & 1) add('D'); return token; },
        };
    }
}

test('FloatingSpGump 원본 10.78 행: 구성/표시/삭제/Idle 의 노드 필드와 호출 순서(Redraw 안의 필드 변경 포함)가 일치한다', () => {
    const all = rows('floating-sp-x86.tsv').filter((row) => row[0] !== 'section'); const failures = [];
    assert.equal(all.length, 426);
    const kNode = 0x12201000;
    for (const [index, row] of all.entries()) {
        assert.equal(row.length, 6, `행 ${index}`);
        const input = row[1].split(','); const scene = new FloatingScene(); const gump = new FloatingSpGump(scene.create, scene.hooks());
        const number = (text) => Number(BigInt.asUintN(32, BigInt(text)));
        if (row[0] === 'construct') gump.construct(kNode, Number(input[0]) | 0, floatOf(number(input[1])));
        else if (row[0] === 'show') gump.show(Number(input[0]) | 0, Number(input[1]) | 0, Number(input[2]) | 0, number(input[3]));
        else if (row[0] === 'delete') assert.equal(gump.deleteScalar(kNode, number(input[0])), kNode);
        else {
            assert.equal(row[0], 'idle'); assert.equal(input.length, 9);
            scene.node.x = floatOf(number(input[0])); scene.node.y = floatOf(number(input[1])); scene.node.velocity = floatOf(number(input[2])); scene.node.acceleration = floatOf(number(input[3])); scene.node.stamp = Number(input[4]);
            scene.now = Number(input[5]); scene.create.screenWidth = Number(input[6]) | 0; scene.create.screenHeight = Number(input[7]) | 0; scene.mutate = input[8] === '1'; gump.idle(kNode);
        }
        const problems = [];
        // 두 정밀도 열(x87 53비트/64비트)이 모두 같은 값이어야 한다(원본 관측이 두 정밀도에서 동일했다).
        if (scene.snapshot() !== row[2] || scene.snapshot() !== row[4]) problems.push('snapshot');
        if (scene.trace.text() !== row[3] || scene.trace.text() !== row[5]) problems.push('trace');
        if (problems.length) failures.push(`${index}/${row[0]}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
});
