// 원본 10.78 Uber 전체 생성·교체 160행의 현재 물리와 콜백 순서를 대조한다.
import './nan-exact.mjs';
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { decodeGumpNode, encodeGumpNode } from './gump-node-support.mjs';
import { GumpCreate, GumpCreateState } from '../www/ui/gump-create.js';
import { GumpTree, GumpTreeState } from '../www/ui/gump-tree.js';
import { GumpLifetime, GumpLifetimeState } from '../www/ui/gump-lifetime.js';
import { HudDividerNode } from '../www/ui/hud-dock.js';
import { UberRootGump, UberRootNode } from '../www/ui/uber-root-gump.js';

// 원본 격리 장면의 주소·고정 전역 순서이다.
const NODES = 0x1100000, STRIDE = 0x200, VTABLE = 0x1000000, TITLE = 0x565e70, FORMAT = 0x1003100, COUNT = 0x565dc8, SERIAL = 0x540418;
const GLOBALS = [0x540451, 0x5411c8, 0x542664, 0x5ca920, 0x5ca924, 0x5ca928, 0x5c78f0, 0x5c78f4, 0x531860, 0x531864, 0x540cec, 0x540cf8, 0x542a04];
const ROOT_FIELDS = ['vertical', 'minimapDivider', 'chatDivider', 'minimap', 'production', 'reservedB4', 'chat', 'reservedBC', 'knowledge', 'message'];
const DIVIDER_FIELDS = ['shape', 'tileWidth', 'tileHeight', 'type', 'frame', 'backgroundFlags', 'axis', 'minX', 'maxX', 'minY', 'maxY'];
// 명시 중단은 하위 장치 오류와 구별한다.
class ExplicitStop extends Error {}
// 정수 하위 DWORD와 부동소수점 원시 비트를 변환한다.
const number = (text) => Number(text) >>> 0;
const address = (index) => NODES + index * STRIDE;
const float = (bits) => { const data = new DataView(new ArrayBuffer(4)); data.setUint32(0, bits, true); return data.getFloat32(0, true); };
const bits = (value) => { const data = new DataView(new ArrayBuffer(4)); data.setFloat32(0, value, true); return data.getUint32(0, true); };
const hex = (text) => Buffer.from(text, 'latin1').toString('hex');

// 미작성 guard를 보존하며 기본·Uber·divider의 같은 물리 슬롯을 표현한다.
class Slot {
    // raw 물리와 현재 파생 종류·객체이다.
    raw; kind = 'b'; value; released = false;
    // 지정 바이트로 전체 슬롯을 채우고 기본 노드를 읽는다.
    constructor(fill) { this.raw = new Uint8Array(0x180).fill(fill); this.decode('b'); }
    // 현재 raw에서 해당 파생 필드를 같은 객체로 해석한다.
    decode(kind) {
        this.kind = kind; const base = decodeGumpNode(Buffer.from(this.raw.subarray(0, 160)).toString('hex'));
        this.value = kind === 'r' ? new UberRootNode() : kind === 'd' ? new HudDividerNode() : base;
        if (kind === 'r' || kind === 'd') Object.assign(this.value, base);
        const view = new DataView(this.raw.buffer);
        if (kind === 'r') {
            // Uber의 고정 자식·예약 DWORD를 원문 순서로 읽는다.
            for (let i = 0; i < ROOT_FIELDS.length; i += 1) this.value[ROOT_FIELDS[i]] = view.getUint32(0xa0 + i * 4, true);
        } else if (kind === 'd') {
            // divider의 배경·축·제한과 고정 slave 표를 읽는다.
            for (let i = 0; i < DIVIDER_FIELDS.length; i += 1) this.value[DIVIDER_FIELDS[i]] = i === 1 || i === 2 || i >= 7 ? view.getInt32(0xa0 + i * 4, true) : view.getUint32(0xa0 + i * 4, true);
            for (let i = 0; i < 10; i += 1) this.value.slaves[i] = [view.getUint32(0xcc + i * 8, true), view.getUint32(0xd0 + i * 8, true)];
        }
    }
    // 현재 필드만 원래 위치에 기록하고 나머지 guard를 보존한다.
    encode() {
        const out = this.raw.slice(); out.set(Buffer.from(encodeGumpNode(this.value), 'hex')); const view = new DataView(out.buffer);
        if (this.kind === 'r') {
            // Uber 자식·예약 DWORD의 실제 현재 값이다.
            for (let i = 0; i < ROOT_FIELDS.length; i += 1) view.setUint32(0xa0 + i * 4, this.value[ROOT_FIELDS[i]], true);
        } else if (this.kind === 'd') {
            // divider의 모든 확장 필드와 slave 표를 기록한다.
            for (let i = 0; i < DIVIDER_FIELDS.length; i += 1) view.setUint32(0xa0 + i * 4, this.value[DIVIDER_FIELDS[i]], true);
            for (let i = 0; i < 10; i += 1) { view.setUint32(0xcc + i * 8, this.value.slaves[i][0], true); view.setUint32(0xd0 + i * 8, this.value.slaves[i][1], true); }
        }
        return out;
    }
}

// Native 입력과 명시 장치만 구성하고 기대 결과 열은 계산하지 않는다.
class Scene {
    // 실제 슬롯·공통 등록표와 입력 전역·콜백 관측이다.
    slots; create = new GumpCreateState(); factory; globals = new Map(); scales; used = new Map(); title; test; stop; fail; effects = []; calls = new Map(); trace = []; allocations = 0; next = 6;
    // 원본 앞 11개 입력 열로 초기 물리와 기존 자식을 만든다.
    constructor(row) {
        this.slots = Array.from({ length: 32 }, () => new Slot(number(row[7]) & 255)); this.slots[0].decode('r');
        this.used.set(address(0), 'r'); this.used.set(address(1), 'b'); this.create.serial = 31;
        // 전역과 콜백 변경 입력을 원본 순서로 읽는다.
        for (const pair of row[2].split(',')) { const [key, value] = pair.split(':'); this.globals.set(number(key), number(value)); }
        this.scales = row[3].split(',').map(number); this.title = row[4] === '-' ? '' : Buffer.from(row[4], 'hex').toString('latin1'); this.test = number(row[5]); this.stop = row[9]; this.fail = number(row[10]);
        if (row[8]) for (const item of row[8].split(';')) { const [when, target, value] = item.split('/'); this.effects.push({ when, target: number(target), value: number(value) }); }
        this.factory = new GumpCreate(this.create, { node: (token) => this.base(token) }); this.baseDevice(address(1)); this.base(address(1)).flags = 0x801;
        const children = number(row[6]); this.base(address(1)).child = children ? address(2) : 0;
        // 기존 루트 직계 자식과 다음 링크를 현재 입력으로 구성한다.
        for (let i = 0; i < children; i += 1) {
            const token = address(i + 2); this.used.set(token, 'b'); this.baseDevice(token); const node = this.base(token);
            node.parent = address(1); node.next = i + 1 < children ? address(i + 3) : 0; node.x = Math.fround(13.5 + i); node.y = Math.fround(-7.25 - i);
        }
    }
    // 유효한 고정 주소의 슬롯만 해석한다.
    slot(token) { const index = (token - NODES) / STRIDE; if (!Number.isInteger(index) || index < 0 || index >= 32) throw new Error('Uber 검사 토큰 범위'); return this.slots[index]; }
    // 현재 기본 주소를 조회한다.
    base(token) { return token && !this.slot(token).released ? this.slot(token).value : null; }
    // 입력 효과를 현재 노드 또는 전역 DWORD에 기록한다.
    write(target, value) {
        if (target >= NODES && target < NODES + this.slots.length * STRIDE) {
            const index = Math.floor((target - NODES) / STRIDE), offset = (target - NODES) % STRIDE, slot = this.slots[index];
            slot.raw = slot.encode(); new DataView(slot.raw.buffer).setUint32(offset, value, true); slot.decode(slot.kind);
        } else if (target === COUNT) this.create.count = value; else if (target === SERIAL) this.create.serial = value; else this.globals.set(target, value);
    }
    // 제목 포인터의 첫 BYTE 또는 현재 전역 값을 읽는다.
    read(target) { if (target === TITLE) return this.title ? this.title.charCodeAt(0) : 0; if (!this.globals.has(target)) throw new Error('Uber 전역 누락'); return this.globals.get(target); }
    // 호출을 기록한 뒤 명시 변이와 중단을 순서대로 실행한다.
    record(name, text) {
        this.trace.push(text); const count = (this.calls.get(name) ?? 0) + 1; this.calls.set(name, count); const current = name + count;
        // 같은 호출에 지정된 현재 물리 변경을 순서대로 반영한다.
        for (const effect of this.effects) if (effect.when === current) this.write(effect.target, effect.value);
        if (this.stop === current) throw new ExplicitStop();
    }
    // 숫자 인자를 포함한 호출 관측이다.
    event(name, ...values) { this.record(name, [name, ...values.map((value) => value >>> 0)].join(':')); }
    // 새 실제 슬롯을 지정 파생 종류로 확보한다.
    fresh(kind) { const token = address(this.next++); this.slot(token).decode(kind); this.used.set(token, kind); return token; }
    // 실제 공통 생성기와 Native 기본 가상 표를 사용한다.
    baseDevice(token, command = 0, width = 0, height = 0, flags = 1) { this.factory.construct(token, command, width, height, flags); this.base(token).vtable = VTABLE; }
    // 명시된 네 하위 공장의 결과와 배율만 공급한다.
    child(name) {
        const token = this.fresh('b'); this.baseDevice(token, 0, 0, 0, name === 'knowledge' ? 0 : 1);
        if (name === 'minimap' || name === 'knowledge') this.base(token).scale = float(this.scales[name === 'minimap' ? 0 : 1]);
        this.event(name, token); return token;
    }
    // 실제 부모가 선택한 각 장치를 현재 슬롯과 입력 관측에 연결한다.
    hooks() {
        return {
            node: (token) => this.slot(token).kind === 'r' ? this.base(token) : null,
            divider: (token) => this.slot(token).kind === 'd' ? this.base(token) : null, base: (token) => this.base(token),
            read: (target) => this.read(target), write: (target, value) => this.write(target, value),
            allocate: (size) => { const token = ++this.allocations === this.fail ? 0 : this.fresh(size === 0xc8 ? 'r' : size === 0x11c ? 'd' : 'm'); this.event('alloc', size, token); return token; },
            constructBase: (token, kind, width, height, flags) => { this.baseDevice(token, kind, width, height, flags); this.event('base', token, kind, bits(width), bits(height), flags); },
            constructDivider: (token, axis, width, height, flags, type, frame) => {
                this.baseDevice(token, 55, width, height); const node = this.base(token); node.flags = (node.flags | 0x60200) >>> 0; node.axis = axis; node.slaves = Array.from({ length: 10 }, () => [0, 0]);
                this.event('divider', token, axis, width, height, flags, type, frame);
            },
            createMinimap: () => this.child('minimap'), createProduction: () => this.child('production'), createKnowledge: () => this.child('knowledge'), createChat: () => this.child('chat'),
            emptyObserver: () => this.event('map'), refreshTechnology: () => this.event('refresh'), cleanup: () => this.event('cleanup'),
            attach: (token, x, y, parent, mode) => { const node = this.base(token); node.flags = (node.flags & ~8) >>> 0; node.parent = parent; node.x = x; node.y = y; this.event('attach', token, bits(x), bits(y), parent, mode); },
            move: (token, x, y, parent) => this.event('move', token, bits(x), bits(y), parent), push: (token) => this.event('push', token),
            registerSlave: (master, slave, flags) => this.event('slave', master, slave, flags), type: (type) => { this.event('type', type); return 0; },
            translate: (text) => { this.record('translate', `translate:${hex(text)}`); return 0x12345678; }, help: (token, text) => this.event('help', token, text),
            helpWord: (token, text) => this.record('helpword', `helpword:${token}:${hex(text)}`), savedMessage: () => TITLE,
            equalText: (name, text) => { const equal = this.title.toLowerCase() === text.toLowerCase(); this.record('equal', `equal:${name}:${hex(text)}:${Number(equal)}`); return equal; },
            constructMessage: (token, name) => { this.baseDevice(token, 0, float(this.scales[2]), float(this.scales[3])); this.event('message', token, name); },
            formatMessage: (args) => { this.record('format', ['format', ...args.map((value) => value >>> 0)].join(':')); return FORMAT; },
            testMessage: (condition) => { this.event('test', condition); return this.test; }, visitMessage: (name) => this.event('visit', name),
            messageEvent: (token, event, first, second) => this.event('event', token, event, first, second), overlay: (token, enabled) => this.event('overlay', token, Number(enabled)),
            setDepth: (token, depth) => this.event('depth', token, depth), replaceRoot: (token) => this.event('replace', token),
        };
    }
    // 고정 전역 순서와 공통 count·serial을 기록한다.
    globalsText() { return [...GLOBALS.map((key) => `${key}:${this.globals.get(key)}`), `${COUNT}:${this.create.count}`, `${SERIAL}:${this.create.serial}`].join(','); }
    // 1000칸 등록표의 연속값만 압축한다.
    registryText() {
        const result = [];
        // 각 동일값 구간의 실제 길이를 기록한다.
        for (let i = 0; i < this.create.entries.length;) { let end = i + 1; while (end < this.create.entries.length && this.create.entries[end] === this.create.entries[i]) end += 1; result.push(`${end - i}:${this.create.entries[i]}`); i = end; }
        return result.join(';');
    }
    // 할당된 전체 guard를 주소 순서대로 기록한다.
    nodesText() { return [...this.used].sort((a, b) => a[0] - b[0]).map(([token, kind]) => `${token}:${kind}:${Buffer.from(this.slot(token).encode()).toString('hex')}`).join(';'); }
}

// 실제 PE의 두 정밀도별 160행을 빠짐없이 비교한다.
test('UberRoot 원본 10.78: 생성·교체의 전체 물리와 호출 160행', () => {
    const rows = fs.readFileSync(path.join(REPO_ROOT, 'cpppj/tests/fixtures/uber-root-x86.tsv'), 'utf8').split(/\r?\n/u).filter((line) => line && !line.startsWith('#')).map((line) => line.split('\t'));
    assert.equal(rows.length, 160);
    // 원본 입력과 실제 부모 출력을 각 행마다 독립 비교한다.
    for (const row of rows) {
        assert.equal(row.length, 18, row[0]); const scene = new Scene(row), root = new UberRootGump(scene.create, scene.hooks()); let completed = false, result = 0;
        try { if (row[1] === 'construct') result = root.construct(address(0)); else root.replace(); completed = true; }
        catch (error) { if (!(error instanceof ExplicitStop) && !number(row[10])) throw error; }
        assert.equal(completed, Boolean(number(row[11])), `${row[0]} 완료`);
        if (completed && row[1] === 'construct') assert.equal(result, number(row[12]), `${row[0]} 반환`);
        assert.equal(scene.globalsText(), row[13], `${row[0]} 전역`); assert.equal(scene.registryText(), row[14], `${row[0]} 등록표`);
        assert.equal(scene.nodesText(), row[15], `${row[0]} 노드`); assert.equal(scene.trace.join(';'), row[16], `${row[0]} 호출`);
    }
});

// 재생성은 이전 루트 삭제·정리 뒤 실제 전체 생성 부모와 루트 공개를 실행한다.
test('UberRoot 재생성: 이전 루트 정리 뒤 새 실제 루트를 공개한다', () => {
    const row = fs.readFileSync(path.join(REPO_ROOT, 'cpppj/tests/fixtures/uber-root-x86.tsv'), 'utf8').split(/\r?\n/u).find((line) => line && !line.startsWith('#')).split('\t');
    const scene = new Scene(row), hooks = scene.hooks(), oldRoot = scene.create.entries[0]; let firstCleanup = true;
    hooks.kill = (token) => scene.event('kill', token);
    hooks.cleanup = () => { scene.event('cleanup'); if (firstCleanup) { firstCleanup = false; scene.create.entries[0] = 0; scene.create.collection.root = 0; scene.create.count -= 1; } };
    hooks.replaceRoot = (token) => { scene.event('replace', token); scene.create.entries[0] = token; scene.create.collection.root = token; };
    new UberRootGump(scene.create, hooks).recreate();
    const current = scene.create.entries[0]; assert.notEqual(current, oldRoot); assert.equal(scene.create.collection.root, current); assert.equal(scene.base(current).id, 0);
    assert.deepEqual(scene.trace.slice(0, 3), [`kill:${oldRoot}`, 'cleanup', `alloc:200:${current}`]); assert.equal(scene.trace.at(-1), `replace:${current}`);
    // 이전 루트가 없더라도 정리를 먼저 하며 선택하지 않은 Kill 장치는 요구하지 않는다.
    const blank = new Scene(row), emptyHooks = blank.hooks(); blank.create.entries.fill(0); blank.create.collection.root = 0; blank.create.count = 0;
    emptyHooks.replaceRoot = (token) => { blank.event('replace', token); blank.create.entries[0] = token; blank.create.collection.root = token; };
    new UberRootGump(blank.create, emptyHooks).recreate(); assert.equal(blank.trace[0], 'cleanup'); assert.ok(blank.create.entries[0]);
});

// 실제 공통 트리와 지연 수명으로 이전 자식을 새 Uber에 옮기고 전체를 정리한다.
test('UberRoot 교체: 실제 트리와 삭제 큐가 기존 자식 주소를 보존한다', () => {
    const line = fs.readFileSync(path.join(REPO_ROOT, 'cpppj/tests/fixtures/uber-root-x86.tsv'), 'utf8').split(/\r?\n/u).find((value) => value.startsWith('replace-1024-768-empty-') && value.split('\t')[0].endsWith('27f'));
    const row = line.split('\t'); row[6] = '3'; const scene = new Scene(row), before = scene.create.count, hooks = scene.hooks();
    // 기존 세 자식을 실제 공통 트리의 양방향 부착 상태로 둔다.
    for (let i = 2; i < 5; i += 1) { const node = scene.base(address(i)); node.flags = (node.flags & ~8) >>> 0; node.previous = i === 2 ? 0 : address(i - 1); }
    let tree, lifetime;
    tree = new GumpTree(scene.create, new GumpTreeState(), { node: hooks.base, redraw: () => {}, push: (token) => tree.push(token),
        attach: (...args) => tree.attach(...args), kill: (token) => lifetime.kill(token) });
    lifetime = new GumpLifetime(scene.create, new GumpLifetimeState(), { node: hooks.base, push: (token) => tree.push(token),
        deletingDestructor: (token, flags) => lifetime.deleteScalar(token, flags), release: (token) => { scene.slot(token).released = true; } });
    hooks.attach = (...args) => tree.attach(...args); hooks.push = (token) => tree.push(token); hooks.cleanup = () => lifetime.cleanup(); hooks.replaceRoot = (token) => lifetime.replaceRoot(token);
    new UberRootGump(scene.create, hooks).replace(); const current = scene.create.entries[0];
    assert.ok(current && current !== address(1)); assert.equal(scene.create.collection.root, current); assert.equal(scene.base(address(1)), null); assert.equal(scene.create.count, before + 7);
    let children = 0;
    // 새 루트에서 모든 이전·신규 직계 자식의 실제 부모를 확인한다.
    for (let child = scene.base(current).child; child; child = scene.base(child).next) { assert.equal(scene.base(child).parent, current); children += 1; }
    assert.equal(children, 10); for (let i = 2; i < 5; i += 1) assert.equal(scene.base(address(i)).parent, current);
    assert.equal([...scene.create.entries].filter((value) => value === current).length, 1);
    lifetime.kill(current); lifetime.cleanup(); assert.equal(scene.create.count, 0); assert.equal(scene.create.entries[0], 0);
});

// 실제 하위 공장이 빠지면 앞선 자식 효과를 남기고 그 위치에서 멈춘다.
test('UberRoot 생성: 생산 공장 누락은 앞선 실제 효과를 보존한다', () => {
    const row = fs.readFileSync(path.join(REPO_ROOT, 'cpppj/tests/fixtures/uber-root-x86.tsv'), 'utf8').split(/\r?\n/u).find((line) => line && !line.startsWith('#')).split('\t');
    const scene = new Scene(row), hooks = scene.hooks(); delete hooks.createProduction;
    assert.throws(() => new UberRootGump(scene.create, hooks).construct(address(0)), /하위 몸체 미연결/u);
    const root = scene.base(address(0)); assert.ok(root.vertical && root.minimapDivider && root.chatDivider && root.minimap);
    assert.equal(scene.calls.get('minimap'), 1); assert.equal(scene.calls.has('production'), false);
});
