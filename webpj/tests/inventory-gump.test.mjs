// 생산 격자/항목(InventoryGump)의 생성·배치·통지·점멸을 원본 10.78 독립 관측 격자 170행·점멸 110행의 같은 raw 입력과 전체 노드 바이트·외부 호출·순서·반환에 대조하고, 공통 트리/수명에 격자와 항목을 두 번 부착하는 통합을 검사한다.
// 원본게임을 실행하지 않는다. 앞 열의 입력만 읽고 뒤 열의 관측은 비교에만 쓴다(cpppj `InventoryGumpTests`/`InventoryBlinkTests` 와 같은 방식). (2026-11-15 WB04-E472)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { GumpCreate, GumpCreateNode, GumpCreateState } from '../www/ui/gump-create.js';
import { GumpLifetime, GumpLifetimeState } from '../www/ui/gump-lifetime.js';
import { GumpTree, GumpTreeState } from '../www/ui/gump-tree.js';
import { GumpCollection } from '../www/render/gump-collection.js';
import { GumpRedraw } from '../www/render/gump-redraw.js';
import { SidPool } from '../www/core/sid-pool.js';
import { ContainedFinderState } from '../www/core/raw-contained-finder.js';
import { InventoryGump, InventoryGridNode, InventoryItemNode } from '../www/ui/inventory-gump.js';
import { decodeGumpNode, encodeGumpNode } from './gump-node-support.mjs';

// 원본 격리 관측과 같은 안정 토큰이다.
const NODE = 0x12401000; const FIRST_ITEM = 0x12403000; const ROOT = 0x15000000; const BLINK_ITEM = ROOT + 128;
// 숫자/비트 변환과 바이트 읽기·쓰기 도우미이다.
const number = (text) => Number(BigInt.asUintN(32, BigInt(text)));
const floatBits = (value) => { const view = new DataView(new ArrayBuffer(4)); view.setFloat32(0, value, true); return view.getUint32(0, true); };
const floatValue = (bits) => { const view = new DataView(new ArrayBuffer(4)); view.setUint32(0, bits >>> 0, true); return view.getFloat32(0, true); };
const get = (raw, offset, size = 4) => { let value = 0; for (let i = size - 1; i >= 0; i -= 1) value = value * 256 + raw[offset + i]; return value; };
const put = (raw, offset, value, size = 4) => { for (let i = 0; i < size; i += 1) raw[offset + i] = Math.floor(value / 2 ** (8 * i)) & 0xff; };
const hex = (raw) => Buffer.from(raw).toString('hex');
const bytes = (text) => Uint8Array.from(Buffer.from(text, 'hex'));
const inputs = (text) => text.split(',').map(number);
// 원본이 쓰지 않는 필드도 임의 바이트(0xa5)로 남기는 입력이다.
const initialBase = () => decodeGumpNode('a5'.repeat(160));
const double = (low, high) => { const view = new DataView(new ArrayBuffer(8)); view.setUint32(0, low, true); view.setUint32(4, high, true); return view.getFloat64(0, true); };
const doubleParts = (value) => { const view = new DataView(new ArrayBuffer(8)); view.setFloat64(0, value, true); return [view.getUint32(0, true), view.getUint32(4, true)]; };

// 파생 격자의 전체 원문 입력을 준비한다(+a0 최소 열/행, +a8/+ac 칸 크기, +b0..c0 목록·커서·클릭·고정·항목 배율).
function initialGrid() {
    const node = Object.assign(new InventoryGridNode(), initialBase());
    node.minimumColumns = node.minimumRows = 0xa5a5a5a5 | 0; node.cellWidth = node.cellHeight = node.itemScale = floatValue(0xa5a5a5a5);
    node.contentMask = node.acceptedCursor = node.requireImageHit = node.fixed = 0xa5a5a5a5; return node;
}
// 미작성 +a4/+b2 를 포함한 항목 입력이다.
function initialItem() {
    const node = Object.assign(new InventoryItemNode(), initialBase());
    node.content = node.reservedA4 = node.requireImageHit = node.additional = 0xa5a5a5a5; node.blinkUntil = double(0xa5a5a5a5, 0xa5a5a5a5); node.blink = node.decoration = 0xa5; node.reservedB2 = 0xa5a5; node.iconScale = floatValue(0xa5a5a5a5); return node;
}
// 호스트 배치 대신 실제 원본 offset 으로 현재 격자를 기록한다(0xc4 바이트).
function encodeGrid(n) {
    const raw = new Uint8Array(0xc4); raw.set(Buffer.from(encodeGumpNode(n), 'hex')); put(raw, 160, n.minimumColumns >>> 0); put(raw, 164, n.minimumRows >>> 0); put(raw, 168, floatBits(n.cellWidth)); put(raw, 172, floatBits(n.cellHeight));
    put(raw, 176, n.contentMask); put(raw, 180, n.acceptedCursor); put(raw, 184, n.requireImageHit); put(raw, 188, n.fixed); put(raw, 192, floatBits(n.itemScale)); return hex(raw);
}
// 항목의 double/두 BYTE/미작성 WORD 도 원본 폭으로 기록한다(0xc0 바이트).
function encodeItem(n) {
    const raw = new Uint8Array(0xc0); raw.set(Buffer.from(encodeGumpNode(n), 'hex')); put(raw, 160, n.content); put(raw, 164, n.reservedA4); const [low, high] = doubleParts(n.blinkUntil); put(raw, 168, low); put(raw, 172, high);
    raw[176] = n.blink; raw[177] = n.decoration; put(raw, 178, n.reservedB2, 2); put(raw, 180, n.requireImageHit); put(raw, 184, n.additional); put(raw, 188, floatBits(n.iconScale)); return hex(raw);
}
// 원문 0xc0 필드를 호스트 파생 노드로 읽으며 패딩을 추론하지 않는다.
function loadItem(raw) {
    const node = Object.assign(new InventoryItemNode(), decodeGumpNode(hex(raw.subarray(0, 160))));
    node.content = get(raw, 160); node.reservedA4 = get(raw, 164); node.blinkUntil = double(get(raw, 168), get(raw, 172)); node.blink = raw[176]; node.decoration = raw[177]; node.reservedB2 = get(raw, 178, 2);
    node.requireImageHit = get(raw, 180); node.additional = get(raw, 184); node.iconScale = floatValue(get(raw, 188)); return node;
}

// 완성 입력과 효과 관측을 보관한다. 기대 분기/배치값은 계산하지 않는다.
class InventoryScene {
    // 원본 내용물의 같은 물리 범위, 같은 검색 정책, 단일 등록표, 격자/항목/기본 노드 저장소와 독립 외부 입력이다.
    pool = new SidPool(24000, { server: false }); finder = new ContainedFinderState(); create = new GumpCreateState(); grid = initialGrid(); items = new Map(); bases = new Map();
    point = { x: 7, y: -3 }; cursor = 0; parent = 50; forward = 0; baseReturn = 0; inside = 0; now = 0; trace = '';

    // 원본과 같은 기본/파생 주소만 공급하는 공장과 관측 경계를 설치한다.
    constructor() {
        this.factory = new GumpCreate(this.create, { node: (token) => this.node(token), allocate: (size) => { assert.equal(size, 160); this.bases.set(42, new GumpCreateNode()); return 42; } });
        this.create.serial = 23;
        const event = (text, args) => this.event(text, args);
        this.hooks = {
            grid: (token) => (token === NODE ? this.grid : null), item: (token) => this.items.get(token) ?? null,
            assertion: (text) => { event(`Assert,${hex(Buffer.from(text, 'latin1'))}`); },
            clearChildren: (token) => { assert.equal(token, NODE); event('Clear'); },
            baseAttach: (token, x, y, parent, mode) => { assert.equal(token, NODE); event('BaseAttach', [floatBits(x), floatBits(y), parent, mode]); },
            allocateItem: (size) => { assert.equal(size, 0xc0); const token = FIRST_ITEM + this.items.size * 0x100; this.items.set(token, initialItem()); event('New', [size, token]); return token; },
            attach: (token, x, y, parent, mode) => { if (token === NODE) event('V', [floatBits(x), floatBits(y), parent, mode]); else event('ItemAttach', [token, floatBits(x), floatBits(y), parent, mode]); },
            push: (token) => { assert.equal(token, NODE); event('Push'); }, bind: (token, sid) => { assert.equal(token, NODE); event('Bind', [sid]); this.grid.object = sid; },
            idle: (token, enabled) => { assert.ok(enabled); event('Idle', [token, 1]); this.node(token).flags = (this.node(token).flags | 0x80) >>> 0; },
            now: () => { event('Now'); return this.now; }, contentParent: (sid) => { event('Parent', [sid]); return this.parent; },
            forward: (token, ev, first, second) => { assert.equal(token, NODE); event('Forward', [ev, first, second]); return this.forward; },
            baseHandle: (token, ev, first, second) => { assert.equal(token, NODE); event('BaseHandle', [ev, first, second]); return this.baseReturn; },
            local: (token, ev, point) => { assert.equal(token, NODE); event('Local', [ev, point.x >>> 0, point.y >>> 0]); return [7.5, -3.25]; },
            inside: (token, point) => { assert.equal(token, NODE); event('Inside', [floatBits(point[0]), floatBits(point[1])]); return this.inside !== 0; }, cancelCursor: () => { event('Cancel'); },
        };
    }

    // 생존 노드는 실제 기본 생성자가 사용하는 같은 주소이다.
    node(token) { if (token === NODE) return this.grid; return this.items.get(token) ?? this.bases.get(token) ?? null; }

    // 호출 인자만 원문 구분자로 기록한다(전체 DWORD 를 부호 해석 없이 보존).
    event(text, args = []) { this.trace += (this.trace ? ';' : '') + text; for (const value of args) this.trace += `,${value >>> 0}`; }

    // 현재 커서 전역의 live 값이다.
    state() { const scene = this; return { get cursorPoint() { return scene.point; }, get cursorFlags() { return scene.cursor; } }; }

    // 원본에 공급한 flags/slot 을 실제 별도 ContentForm 에 쓴다. 입력 목록은 "flags:slot;…" 이고 "-" 는 빈 목록이다.
    contents(text) {
        const parts = text === '-' ? [] : text.split(';'); let raw = this.pool.physical(50); put(raw, 6, parts.length ? 100 : 0, 2);
        // 목록과 실제 next WORD 를 입력 순서대로 작성한다.
        for (let i = 0; i < parts.length; i += 1) {
            const fields = parts[i].split(':'); raw = this.pool.physical(100 + i); raw[10] = 6; put(raw, 4, i + 1 < parts.length ? 101 + i : 0, 2); put(raw, 20, number(fields[0]), 2); raw[23] = number(fields[1]);
        }
    }
}

// 탭 구분 fixture 의 주석 줄을 건너뛴 행 목록이다. 빈 열도 보존한다.
function rows(name) { return fs.readFileSync(new URL(`../../cpppj/tests/fixtures/${name}`, import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line && !line.startsWith('#')).map((line) => line.split('\t')); }

test('InventoryGump 원본 10.78 격자 170행: 격자·항목 생성/부착/통지/삭제 통지의 전체 노드 바이트·생성 항목·호출 순서·반환이 모두 일치한다', () => {
    const table = rows('inventory-grid-x86.tsv'); const failures = [];
    assert.equal(table.length, 170);
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 6, `행 ${index}`);
        const s = new InventoryScene(); const inventory = new InventoryGump(s.create, s.factory, s.pool, s.finder, s.state(), s.hooks); let result = '-'; let node = '';
        if (row[0] === 'Grid') { const a = inputs(row[1]); result = String(inventory.constructGrid(NODE, a[0] | 0, a[1] | 0, a[2])); }
        else if (row[0] === 'Item') {
            const a = row[1].split(','); s.now = Number(a[4]); const raw = s.pool.physical(number(a[2])); put(raw, 20, number(a[3]), 2); s.items.set(NODE, initialItem());
            // 독립 단일 항목은 격자와 같은 주소를 쓰므로 기본 조회를 항목에 연결한 생성기를 사용한다.
            const factory = new GumpCreate(s.create, { node: (token) => s.items.get(token) });
            const hooks = { ...s.hooks, idle: (token, enabled) => { assert.ok(enabled); s.event('Idle', [token, 1]); s.items.get(token).flags = (s.items.get(token).flags | 0x80) >>> 0; } };
            const single = new InventoryGump(s.create, factory, s.pool, s.finder, s.state(), hooks);
            result = String(single.constructItem(NODE, floatValue(number(a[0])), floatValue(number(a[1])), number(a[2]))); node = encodeItem(s.items.get(NODE)); s.items.clear();
        } else {
            inventory.constructGrid(NODE, 2, 6, 8); s.grid.object = 50; s.grid.x = 1.25; s.grid.y = -2.5; s.trace = '';
            if (row[0] === 'Attach') {
                const parts = row[1].split('|'); const a = inputs(parts[0]);
                s.grid.minimumColumns = a[0] | 0; s.grid.minimumRows = a[1] | 0; s.grid.contentMask = a[2]; s.grid.fixed = a[3]; s.grid.itemScale = floatValue(a[4]); s.grid.requireImageHit = a[5];
                s.grid.baseWidth = floatValue(a[6]); s.grid.baseHeight = floatValue(a[7]); s.grid.width = floatValue(a[8]); s.grid.height = floatValue(a[9]); s.contents(parts[1]); inventory.attachGrid(NODE, 5, 6, 0, a[10]);
            } else if (row[0] === 'Handle') {
                const a = inputs(row[1]); s.parent = a[1]; put(s.pool.physical(100), 20, a[2], 2); s.inside = a[3]; s.cursor = a[4]; s.grid.acceptedCursor = a[5]; s.forward = a[6]; s.baseReturn = a[7]; result = String(inventory.handleGrid(NODE, a[0], 100, 9));
            } else { assert.equal(row[0], 'Gone'); inventory.parentGone(NODE, number(row[1])); }
        }
        if (!node) node = encodeGrid(s.grid);
        let children = ''; for (const [token, item] of s.items) children += `${children ? ';' : ''}${token}=${encodeItem(item)}`;
        // 순회 자체는 원본/기존 검색기 각각의 책임이므로 원본의 순회 경계 로그만 비교에서 제외한다.
        const nativeTrace = row[5].split(';').filter((event) => event !== 'Next' && event !== 'Begin,50').join(';'); const actualTrace = s.trace || '-'; const itemText = children || '-';
        const problems = []; if (result !== row[2]) problems.push('result'); if (node !== row[3]) problems.push('node'); if (itemText !== row[4]) problems.push('items'); if (actualTrace !== nativeTrace) problems.push('trace');
        if (problems.length) failures.push(`${index}/${row[0]}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
});

// 점멸 행 하나를 같은 실제 기본 노드/수명/수집기/다시 그리기에 재생한다. 호출 순서·표시 비트·시각 조회·요청 사각형을 기록해 돌려준다.
function runBlink(row) {
    const rootBytes = new Uint8Array(160); rootBytes.set(bytes(row[1]).subarray(0, 160)); const root = decodeGumpNode(hex(rootBytes)); const item = loadItem(bytes(row[2]));
    const base = (token) => (token === ROOT ? root : token === BLINK_ITEM ? item : null); const create = new GumpCreateState(); create.collection.root = ROOT;
    const factory = new GumpCreate(create, { node: base }); const lifeState = new GumpLifetimeState(); if (item.flags & 0x80) lifeState.idle[0] = BLINK_ITEM; const life = new GumpLifetime(create, lifeState, { node: base });
    const collection = new GumpCollection(create.collection, { node: base }); const requests = []; let trace = ''; let timeCalls = 0; const times = row[3].split(',');
    const event = (name, value) => { trace += `${trace ? ';' : ''}${name},${item.flags >>> 0},${value}`; };
    const redraw = new GumpRedraw(collection, { node: base, map: () => ({ left: 0, top: 0, right: 0, bottom: 0 }), invalidate: (rect, flags) => { requests.push(`${rect.left >>> 0},${rect.top >>> 0},${rect.right >>> 0},${rect.bottom >>> 0},${flags}`); } });
    const pool = new SidPool(24000, { server: false }); const content = pool.physical(1); content.fill(0); content[21] = 255; const initial = Uint8Array.from(content);
    const timeBits = (value) => { const [low, high] = doubleParts(value); return `${high.toString(16).padStart(8, '0')}${low.toString(16).padStart(8, '0')}`; };
    const timeValue = (text) => double(Number.parseInt(text.slice(8), 16), Number.parseInt(text.slice(0, 8), 16));
    const hooks = {
        item: (token) => (token === BLINK_ITEM ? item : null), idle: (token, enabled) => { event('idle', enabled ? '1' : '0'); life.setIdle(token, enabled); },
        now: () => { if (!timeCalls && row[10] !== '-') item.blinkUntil = timeValue(row[10].padStart(16, '0')); const value = timeValue(times[timeCalls++].padStart(16, '0')); event('clock', timeBits(value)); return value; },
        redraw: (token, erase) => { event('redraw', erase ? '1' : '0'); redraw.whole(token, erase ? 1 : 0); }, clearBlink: (sid) => { assert.equal(sid, 1); pool.physical(sid)[21] &= 0xfd; },
    };
    const handler = new InventoryGump(create, factory, pool, new ContainedFinderState(), { cursorPoint: { x: 0, y: 0 }, cursorFlags: 0 }, hooks); handler.idleItem(BLINK_ITEM);
    return { trace: trace || '-', requests: requests.join(';') || '-', item, lifeState, content, initial, timeCalls, root, rootBytes, factory };
}

test('InventoryGump 원본 10.78 점멸 110행: 점멸 단계·Idle 표·표시 비트 전환·시각 조회 횟수·내용물 비트·다시 그리기 사각형이 모두 일치한다', () => {
    const table = rows('inventory-blink-x86.tsv'); const failures = [];
    assert.equal(table.length, 110);
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 11, `행 ${index}`);
        const run = runBlink(row); const problems = [];
        if (encodeItem(run.item) !== row[4]) problems.push('node');
        if (run.trace !== row[5]) problems.push('trace');
        // 끝 500칸까지 원본 Idle 표와 비교한다.
        const expectedIdle = inputs(row[6]); if (expectedIdle.length !== run.lifeState.idle.length || expectedIdle.some((value, i) => value !== run.lifeState.idle[i])) problems.push('idle');
        if (run.content[21] !== number(row[7])) problems.push('pool'); if (run.requests !== row[8]) problems.push('requests'); if (run.timeCalls !== number(row[9])) problems.push('time');
        if (problems.length) failures.push(`${row[0]}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
});

test('InventoryGump 공통 트리/수명 통합: 같은 등록표에 격자와 항목을 두 번 부착하면 이전 자식은 지연 삭제 뒤 정리되고 새 항목은 실제 내용물 SID 를 참조한다', () => {
    const s = new InventoryScene(); const treeState = new GumpTreeState(); const lifeState = new GumpLifetimeState(); let tree; let life;
    const treeHooks = { node: (token) => s.node(token), redraw: () => {}, push: (token) => tree.push(token), kill: (token) => life.kill(token), attach: (token, x, y, parent, mode) => tree.attach(token, x, y, parent, mode) };
    tree = new GumpTree(s.create, treeState, treeHooks);
    life = new GumpLifetime(s.create, lifeState, { node: treeHooks.node, push: treeHooks.push, deletingDestructor: (token, flags) => life.deleteScalar(token, flags), release: (token) => { s.items.delete(token); } });
    s.create.screenWidth = 640; s.create.screenHeight = 480; const root = s.factory.initializeRoot();
    s.hooks.clearChildren = (token) => { while (s.node(token).child) life.kill(s.node(token).child); }; s.hooks.baseAttach = (token, x, y, parent, mode) => { tree.attach(token, x, y, parent, mode); };
    s.hooks.attach = s.hooks.baseAttach; s.hooks.idle = (token, enable) => { life.setIdle(token, enable); };
    const inventory = new InventoryGump(s.create, s.factory, s.pool, s.finder, s.state(), s.hooks);
    inventory.constructGrid(NODE, 1, 6, 8); s.grid.form = 0; s.grid.object = 50; s.grid.baseWidth = s.grid.width = 40; s.grid.baseHeight = s.grid.height = 250;
    s.contents('8:0;8:1;64:255'); inventory.attachGrid(NODE, 0, 26, root, 0);
    assert.equal(s.create.count, 4); assert.equal(s.items.size, 2); assert.equal(s.grid.parent, root);
    // 두 항목은 Deck 사본이 아닌 실제 내용물 100/101 을 참조하고 같은 부모에 존재한다.
    for (const [token, item] of s.items) { assert.ok(item.content === 100 || item.content === 101); assert.equal(item.parent, NODE); assert.equal(s.create.entries[item.id], token); s.node(token).form = 0; }
    const oldItems = s.items.size; s.contents('8:3'); inventory.attachGrid(NODE, 0, 26, root, 0);
    assert.equal(lifeState.killCount, oldItems); assert.equal(s.items.size, 3); life.cleanup(); assert.equal(s.items.size, 1); assert.equal(s.create.count, 3);
    const [token, item] = [...s.items][0]; assert.ok(item.content === 100 && item.parent === NODE); assert.equal(item.y, 125); assert.equal(s.create.entries[item.id], token);
});

// 항목 하나를 격자와 같은 주소 없이 만들 때 쓰는 처리기이다: 기본 조회를 항목 저장소에 연결하고 Idle 장치는 같은 항목에 0x80 을 쓴다.
function itemRig(s, pool = s.pool) {
    const factory = new GumpCreate(s.create, { node: (token) => s.items.get(token) });
    return new InventoryGump(s.create, factory, pool, s.finder, s.state(), { ...s.hooks, idle: (token) => { s.event('Idle', [token, 1]); const node = s.items.get(token); node.flags = (node.flags | 0x80) >>> 0; } });
}

test('InventoryGump 노드 기본값과 생성 검사: 파생 필드는 모두 0 이고 10.78 풀/등록표가 아니면 TypeError 이며 필요한 효과가 없으면 Error 이다', () => {
    const grid = new InventoryGridNode(); const item = new InventoryItemNode();
    assert.deepEqual([grid.minimumColumns, grid.minimumRows, grid.cellWidth, grid.cellHeight, grid.contentMask, grid.acceptedCursor, grid.requireImageHit, grid.fixed, grid.itemScale], new Array(9).fill(0));
    assert.deepEqual([item.content, item.reservedA4, item.blinkUntil, item.blink, item.decoration, item.reservedB2, item.requireImageHit, item.additional, item.iconScale], new Array(9).fill(0));
    const s = new InventoryScene();
    assert.throws(() => new InventoryGump(s.create, s.factory, { edition: 'cd1072', capacity: 10 }, s.finder, s.state(), s.hooks), TypeError);
    assert.throws(() => new InventoryGump({ collection: { edition: 'cd1072' } }, s.factory, s.pool, s.finder, s.state(), s.hooks), TypeError);
    // 파생 저장소가 없거나 필요한 효과가 없으면 도달한 위치에서 거부한다.
    const inventory = new InventoryGump(s.create, s.factory, s.pool, s.finder, s.state(), { ...s.hooks, grid: undefined });
    assert.throws(() => inventory.constructGrid(NODE, 1, 1, 1), /생산 격자 실제 하위 미연결/);
    const none = new InventoryGump(s.create, s.factory, s.pool, s.finder, s.state(), { ...s.hooks, grid: () => null, item: () => null });
    assert.throws(() => none.constructGrid(NODE, 1, 1, 1), /생산 격자 저장소 없음/); assert.throws(() => none.constructItem(NODE, 1, 1, 100), /생산 항목 저장소 없음/); assert.throws(() => none.constructGrid(0, 1, 1, 1), /생산 격자 저장소 없음/);
});

test('InventoryGump 격자 생성: 0열은 진단 뒤 계속하고 항목 생성은 내용물 SID 범위·점멸 비트(0x200 만)·원문 초기값을 지킨다', () => {
    const s = new InventoryScene(); const inventory = new InventoryGump(s.create, s.factory, s.pool, s.finder, s.state(), s.hooks);
    assert.equal(inventory.constructGrid(NODE, 0, 6, 8), NODE); assert.equal(s.trace, `Assert,${hex(Buffer.from('_numSlotsX != 0', 'latin1'))}`); assert.equal(s.grid.minimumColumns, 0); assert.equal(s.grid.minimumRows, 6); assert.equal(s.grid.vtable, 0x506800);
    // 내용물 SID 는 0 이 아니고 풀 범위 안이며 65535 이하여야 한다.
    const big = { edition: 'patch1078', capacity: 100000, slot: () => new Uint8Array(32) }; const wide = itemRig(s, big); s.items.set(NODE, initialItem());
    assert.throws(() => wide.constructItem(NODE, 1, 1, 0), /SID 범위 오류/); assert.throws(() => wide.constructItem(NODE, 1, 1, 100000), /SID 범위 오류/); assert.throws(() => wide.constructItem(NODE, 1, 1, 65536), /SID 범위 오류/);
    assert.equal(wide.constructItem(NODE, 1, 1, 65535), NODE);
    // 내용물 flags 에 0x200 이 없으면(비트 0 만 있어도) 점멸하지 않고, 있으면 단계 4 로 시작한다.
    put(s.pool.physical(100), 20, 0x1, 2); s.items.set(NODE, initialItem()); s.trace = ''; const single = itemRig(s); single.constructItem(NODE, 1, 1, 100); assert.equal(s.items.get(NODE).blink, 0); assert.equal(s.trace, '');
    put(s.pool.physical(100), 20, 0x200, 2); s.items.set(NODE, initialItem()); s.now = 5; single.constructItem(NODE, 1, 1, 100); assert.equal(s.items.get(NODE).blink, 4); assert.equal(s.items.get(NODE).blinkUntil, 5.1); assert.equal(s.trace, `Idle,${NODE},1;Now`);
});

test('InventoryGump 격자 부착: 부모가 없으면 빈 목록, 고정 배치의 0열/0행은 거부, 할당 실패는 거부하며 항목 배율은 비0 일 때만 전달한다', () => {
    const build = () => { const s = new InventoryScene(); const inventory = new InventoryGump(s.create, s.factory, s.pool, s.finder, s.state(), s.hooks); inventory.constructGrid(NODE, 2, 6, 8); s.grid.baseWidth = s.grid.width = 40; s.grid.baseHeight = s.grid.height = 250; s.trace = ''; return { s, inventory }; };
    // object 0 이면 SID 0 에서 순회하므로 SID 1 의 내용물 목록과 무관하게 빈 목록이다.
    const empty = build(); put(empty.s.pool.physical(1), 6, 100, 2); put(empty.s.pool.physical(100), 4, 0, 2); empty.s.pool.physical(100)[10] = 6; put(empty.s.pool.physical(100), 20, 8, 2); empty.s.grid.object = 0; empty.inventory.attachGrid(NODE, 0, 0, 0, 0);
    assert.equal(empty.s.items.size, 0);
    // 고정 배치에서 열 또는 행이 0 이면 원본 0 나눗셈 경로를 거부한다(둘 중 하나만 0 이어도).
    for (const [columns, rows] of [[0, 2], [2, 0], [0, 0]]) { const { s, inventory } = build(); s.grid.object = 50; s.grid.fixed = 1; s.grid.minimumColumns = columns; s.grid.minimumRows = rows; s.contents('8:0'); assert.throws(() => inventory.attachGrid(NODE, 0, 0, 0, 0), /0열\/행/, `${columns}x${rows}`); }
    // 항목 할당이 0 을 돌려주면 성공으로 바꾸지 않는다.
    const failed = build(); failed.s.grid.object = 50; failed.s.contents('8:0'); failed.s.hooks.allocateItem = () => 0; assert.throws(() => failed.inventory.attachGrid(NODE, 0, 0, 0, 0), /할당 실패/);
    // 격자 항목 배율이 0 이면 항목 기본(0)이고 1 이어도 그대로 전달하며, 클릭 요구는 격자 값을 따른다.
    for (const [scale, expected] of [[0, 0], [1, 1], [0.5, 0.5]]) {
        const { s, inventory } = build(); s.grid.object = 50; s.grid.itemScale = scale; s.grid.requireImageHit = 0; s.contents('8:0'); inventory.attachGrid(NODE, 0, 0, 0, 0);
        const item = [...s.items.values()][0]; assert.equal(item.iconScale, expected); assert.equal(item.requireImageHit, 0);
    }
});

test('InventoryGump 격자 배치 보정: 풀 크기와 같은 SID 는 거부하고 부모가 없으면 내용물이 있어도 빈 목록이며 행/열이 1 일 때만 최소값으로 올린다', () => {
    const s = new InventoryScene(); const inventory = new InventoryGump(s.create, s.factory, s.pool, s.finder, s.state(), s.hooks);
    // 풀 크기(24000)와 같은 SID 는 풀 범위 오류이다.
    s.items.set(NODE, initialItem()); assert.throws(() => itemRig(s).constructItem(NODE, 1, 1, 24000), /SID 범위 오류/);
    inventory.constructGrid(NODE, 2, 6, 8); s.grid.baseWidth = s.grid.width = 40; s.grid.baseHeight = s.grid.height = 250;
    // object 0 이면 SID 1 의 목록(slot 8 → 최댓값 9)을 읽지 않으므로 열/행 1 의 기본 칸 크기이다.
    put(s.pool.physical(1), 6, 100, 2); const raw = s.pool.physical(100); raw[10] = 6; put(raw, 4, 0, 2); put(raw, 20, 8, 2); raw[23] = 8; s.grid.object = 0; s.grid.minimumRows = 1; s.grid.minimumColumns = 1; inventory.attachGrid(NODE, 0, 0, 0, 0);
    assert.equal(s.grid.cellWidth, 40); assert.equal(s.grid.cellHeight, 250);
    // 한 칸짜리 목록: 열 1·행 1 에서 최소 행(1)은 그대로이고 최소 열이 더 크면 열을 최소 열로 올린다(두 번째 보정).
    s.grid.object = 50; s.contents('8:0'); s.grid.minimumRows = 1; s.grid.minimumColumns = 3; inventory.attachGrid(NODE, 0, 0, 0, 0);
    assert.equal(s.grid.cellWidth, Math.fround(40 / 3)); assert.equal(s.grid.cellHeight, 250);
    // 열이 1 이고 최소 행이 더 크면 행을 최소 행으로 올린다(첫 번째 보정).
    s.items.clear(); s.grid.minimumRows = 4; s.grid.minimumColumns = 1; inventory.attachGrid(NODE, 0, 0, 0, 0); assert.equal(s.grid.cellWidth, 40); assert.equal(s.grid.cellHeight, Math.fround(250 / 4));
});

test('InventoryGump 격자 배치 정밀도: 열 수는 sqrt 를 float32 로 줄인 값과 배정도 비율의 곱을 올림하고 음수 열의 항목 위치는 0 쪽으로 절삭한다', () => {
    const s = new InventoryScene(); const inventory = new InventoryGump(s.create, s.factory, s.pool, s.finder, s.state(), s.hooks); inventory.constructGrid(NODE, 2, 1, 8);
    // 개수 2(slot 1 + 1): sqrt(2) 를 float 로 줄이면 열 13, 배정도로 두면 14 가 되는 폭/높이 비율이다.
    s.grid.object = 50; s.grid.baseWidth = 919.2388305664062; s.grid.baseHeight = 100; s.grid.width = 1000; s.grid.height = 100; s.contents('8:1');
    assert.notEqual(Math.ceil((919.2388305664062 / 100) * Math.fround(Math.sqrt(2))), Math.ceil((919.2388305664062 / 100) * Math.sqrt(2)));
    inventory.attachGrid(NODE, 0, 0, 0, 0); assert.equal(s.grid.cellWidth, Math.fround(1000 / 13)); assert.equal(s.grid.cellHeight, 100);
    // 고정 배치의 음수 열: 나머지/몫은 0 쪽으로 절삭한다(slot 3, 열 -2 → 나머지 1·몫 -1).
    const fixed = new InventoryScene(); const tool = new InventoryGump(fixed.create, fixed.factory, fixed.pool, fixed.finder, fixed.state(), fixed.hooks); tool.constructGrid(NODE, -2, 2, 8);
    fixed.grid.object = 50; fixed.grid.fixed = 1; fixed.grid.width = 40; fixed.grid.height = 100; fixed.contents('8:3'); fixed.trace = ''; tool.attachGrid(NODE, 0, 0, 0, 0);
    assert.equal(fixed.grid.cellWidth, -20); assert.equal(fixed.grid.cellHeight, 50);
    assert.ok(fixed.trace.includes(`ItemAttach,${FIRST_ITEM},${floatBits(-20)},${floatBits(-50)},${NODE},0`), fixed.trace);
});

test('InventoryGump 점멸 종료: 다시 그리기가 끝난 뒤에 내용물의 점멸 비트를 지운다', () => {
    // 단계 0 항목: 다시 그리는 순간에는 내용물 비트(bit1)가 남아 있고 종료 뒤에 지워진다.
    const create = new GumpCreateState(); const node = Object.assign(new InventoryItemNode(), initialBase()); node.blink = 0; node.flags = 0; const pool = new SidPool(24000, { server: false }); const content = pool.physical(1); content[21] = 0xff;
    const seen = []; const handler = new InventoryGump(create, new GumpCreate(create, { node: () => node }), pool, new ContainedFinderState(), { cursorPoint: { x: 0, y: 0 }, cursorFlags: 0 },
        { item: () => node, idle: () => {}, redraw: () => { seen.push(content[21]); }, clearBlink: (sid) => { content[21] &= 0xfd; seen.push(`clear${sid}`); } });
    node.content = 1; handler.idleItem(1); assert.deepEqual(seen, [0xff, 'clear1']); assert.equal(content[21], 0xfd); assert.equal(node.flags & 1, 1);
});
