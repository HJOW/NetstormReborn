// 생산 항목(InventoryItemGump)의 아이콘 자식 부착·Draw·클릭·비용 판정을 원본 10.78 독립 관측 132행(항목 노드 바이트·실제 생성한 그림/글자 자식 바이트·마지막 선택·반환·호출 순서)에 대조한다.
// 같은 풀/등록표와 실제 ImageGump/TextGump 생성자를 쓰는 독립 입력 장면이며 원본게임을 실행하지 않는다. 실제 자산 경로(PriestPlacementAssets·실제 TYPE 표)는 후속 단위이다. (2026-11-15 WB04-E473)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { GumpCreate, GumpCreateNode, GumpCreateState } from '../www/ui/gump-create.js';
import { GumpLifetime, GumpLifetimeState } from '../www/ui/gump-lifetime.js';
import { GumpCollection } from '../www/render/gump-collection.js';
import { GumpDrawState } from '../www/render/gump-draw.js';
import { StyleText, StyleTextState } from '../www/ui/style-text-fragments.js';
import { TextGump, TextGumpNode, TextGumpState } from '../www/ui/text-gump.js';
import { ImageGump, ImageGumpNode } from '../www/ui/image-gump.js';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { SidPool } from '../www/core/sid-pool.js';
import { InventoryItemNode, InventoryGridNode } from '../www/ui/inventory-gump.js';
import { InventoryItemGump } from '../www/ui/inventory-item-gump.js';
import { decodeGumpNode, encodeGumpNode } from './gump-node-support.mjs';

// 원본 관측과 같은 안정 토큰(항목·새로 만든 자식의 기준)이다.
const ITEM = 0x12401000; const BORN = 0x12404000;
// 숫자/비트/바이트 도우미이다.
const number = (text) => Number(BigInt.asUintN(32, BigInt(text)));
const inputs = (text) => text.split(',').map(number);
const floatBits = (value) => { const view = new DataView(new ArrayBuffer(4)); view.setFloat32(0, value, true); return view.getUint32(0, true); };
const floatValue = (bits) => { const view = new DataView(new ArrayBuffer(4)); view.setUint32(0, bits >>> 0, true); return view.getFloat32(0, true); };
const hex = (raw) => Buffer.from(raw).toString('hex');
const put = (raw, offset, value, size = 4) => { for (let i = 0; i < size; i += 1) raw[offset + i] = Math.floor(value / 2 ** (8 * i)) & 0xff; };
const double = (low, high) => { const view = new DataView(new ArrayBuffer(8)); view.setUint32(0, low, true); view.setUint32(4, high, true); return view.getFloat64(0, true); };
const doubleParts = (value) => { const view = new DataView(new ArrayBuffer(8)); view.setFloat64(0, value, true); return [view.getUint32(0, true), view.getUint32(4, true)]; };
// 원본이 쓰지 않는 필드도 임의 바이트(0xa5)로 남기는 입력이다.
const initialBase = () => decodeGumpNode('a5'.repeat(160));
// 실제 항목 0xc0 바이트를 원문 offset 에 맞춰 기록한다.
function encodeItem(n) {
    const raw = new Uint8Array(192); raw.set(Buffer.from(encodeGumpNode(n), 'hex')); put(raw, 160, n.content); put(raw, 164, n.reservedA4); const [low, high] = doubleParts(n.blinkUntil); put(raw, 168, low); put(raw, 172, high);
    raw[176] = n.blink; raw[177] = n.decoration; put(raw, 178, n.reservedB2, 2); put(raw, 180, n.requireImageHit); put(raw, 184, n.additional); put(raw, 188, floatBits(n.iconScale)); return hex(raw);
}
// 실제 생성한 이미지 0xb4 바이트만 기록한다.
function encodeImage(n) {
    const raw = new Uint8Array(180); raw.set(Buffer.from(encodeGumpNode(n), 'hex')); put(raw, 160, n.type); put(raw, 164, n.frame); put(raw, 168, n.mode); put(raw, 172, floatBits(n.hotspotX)); put(raw, 176, floatBits(n.hotspotY)); return hex(raw);
}
// 원본과 같은 16개 프레임 코드 입력이다. 품질 프레임의 기대 검색 결과를 계산하지 않는다(side='A', variant='P', flags 0 이 기본).
function makeFrames() {
    const codes = Array.from({ length: 16 }, () => ({ side: 0x41, variant: 0x50, number: 0, flags: 0 }));
    codes[8].side = 0x42; codes[9].flags = 0x20; codes[10] = { side: 0x42, variant: 0x50, number: 0, flags: 0x20 }; codes[11].flags = 0x40; codes[12] = { side: 0x42, variant: 0x50, number: 0, flags: 0x40 };
    return new FrameCodeTable(codes);
}

// 같은 풀/등록표·실제 이미지와 TextGump 생성자를 사용하는 독립 입력 장면이다.
class ItemScene {
    // 실제 내용물과 단일 Gump 등록표, 항목/격자/그림/글자/기본 노드 저장소, 실제 할당 순서와 크기이다.
    pool = new SidPool(24000, { server: false }); create = new GumpCreateState(); item = new InventoryItemNode(); grid = new InventoryGridNode();
    images = new Map(); texts = new Map(); bases = new Map(); born = [];
    // 원문 입력과 명시 조회 응답이다.
    frames = makeFrames(); gumpFrame = -1; cost = 800; owner = 1; stock = 0; disabled = 0; last = 0; bridge = 82; background = 96; backgroundFrame = 2; editor = false;
    cellX = 16; cellY = 11; money = 800; bounds = 0; cells = 0; opened = 0; secondOpen = 0; knowledgeCalls = 0; children = 0; denied = 0; forward = 0; robust = 1;
    point = { x: 17, y: -9 }; shadow = { x: 5, y: 5 }; ownerMaps = new Array(188).fill(0); trace = '';

    // 원본과 같은 실제 생성자를 준비하고 명시 외부 경계만 관측한다.
    constructor() {
        this.factory = new GumpCreate(this.create, { node: (token) => this.base(token) }); this.create.serial = 23;
        Object.assign(this.item, initialBase()); const item = this.item; item.vtable = 0x5068f4; item.flags = 9; item.child = 0; item.serial = 41; item.content = 128;
        item.reservedA4 = item.requireImageHit = item.additional = 0xa5a5a5a5; item.blinkUntil = double(0xa5a5a5a5, 0xa5a5a5a5); item.blink = item.decoration = 0xa5; item.reservedB2 = 0xa5a5; item.iconScale = floatValue(0xa5a5a5a5);
        put(this.pool.physical(128), 18, 128, 2);
        this.collection = new GumpCollection(this.create.collection, this.factory.collectionHooks());
        this.life = new GumpLifetime(this.create, new GumpLifetimeState(), { node: (token) => this.base(token) });
        const scene = this; const event = (name, values) => this.event(name, values);
        this.image = new ImageGump(this.factory, { get localOwner() { return scene.owner; }, ownerMaps: this.ownerMaps, shadowOffset: this.shadow }, {
            node: (token) => this.images.get(token) ?? null, type: () => ({ shape: 0x16000000, hotspotX: 2, hotspotY: 3 }),
            header: (kind, frame) => { event('Header', [kind, 0, frame]); return { width: 40, height: 60, hotspotX: 12, hotspotY: -7 }; },
        });
        this.text = new TextGump(this.create, new TextGumpState(), this.factory, this.life, this.collection, new GumpDrawState(), new StyleText(new StyleTextState(), {}), {
            node: (token) => this.texts.get(token) ?? null, baseNode: (token) => this.base(token),
        });
        const encoder = (value) => hex(Buffer.from(value, 'latin1'));
        this.hooks = {
            node: (token) => (token === ITEM ? this.item : null), base: (token) => this.base(token), type: () => ({ gumpFrame: this.gumpFrame, frames: this.frames }),
            baseAttach: (token, x, y, parent, mode) => { event('Attach', [token, floatBits(x), floatBits(y), parent, mode]); }, clearChildren: () => { event('Clear'); }, cleanup: () => { event('Cleanup'); },
            robust: (sid, allow) => { event('Robust', [sid, allow ? 1 : 0]); return this.robust !== 0; }, assertion: (message) => { event(`Assert,${encoder(message)}`); },
            measure: (kind, argument, explicitFrame) => {
                event('Measure', [kind, argument, 0, explicitFrame ? 1 : 0]); const rects = [[0, 0, 64, 96], [-10, -7, 10, 30], [1000, 1000, 0, 0]]; const r = rects[this.bounds];
                return { bounds: { left: r[0], top: r[1], right: r[2], bottom: r[3] }, cellX: 16, cellY: 11 };
            },
            walk: (query, visit) => {
                event('Walk', [query.type, query.argument >>> 0, query.direction >>> 0, floatBits(query.x), floatBits(query.y), query.explicitFrame ? 1 : 0]);
                const data = [{ frame: 7, x: this.cells === 2 ? -0.5 : 0, y: this.cells === 2 ? 2.25 : 0 }, { frame: 8, x: 1.5, y: -1.0 }];
                // 명시 decoder 경계 입력만 순서대로 전달한다.
                for (let i = 0; i < this.cells; i += 1) { visit(data[i]); event('Advance'); }
            },
            allocate: (size) => {
                const token = BORN + this.born.length * 256; this.born.push([token, size]); event('New', [size, token]);
                if (size === 180) this.images.set(token, Object.assign(new ImageGumpNode(), initialBase()));
                else { assert.equal(size, 164); const node = Object.assign(new TextGumpNode(), initialBase()); node.head = 0xa5a5a5a5; this.texts.set(token, node); }
                return token;
            },
            image: (token, kind, frame, mode) => { this.image.construct(token, kind, frame >>> 0, mode); }, text: (token) => { this.text.construct(token, 0, 0, 0, 0); },
            styledText: (token, value) => { event(`Text,${token},${encoder(value)}`); }, fitText: (token) => { event('Fit', [token, 0, 0]); },
            costUnits: () => this.cost, money: (owner) => { event('Money', [owner]); return this.money; }, position: () => { event('Position'); return this.point; },
            background: (rect, flags, type, frame) => { event('Background', [rect.left, rect.top, rect.right, rect.bottom, flags, type, frame]); },
            local: (token, ev, first, second) => { event('Local', [ev, first, second]); return [7.5, -3.25]; },
            knowledgeOpen: () => { const result = this.knowledgeCalls ? this.secondOpen : this.opened; this.knowledgeCalls += 1; event('Knowledge', [result]); return result; },
            imageHit: (token, x, y) => { event('ImageHit', [token, floatBits(x), floatBits(y)]); return token === 0x12403200 || this.children !== 3 ? 1 : 0; },
            denied: (first, flags) => { event('Denied', [first, flags]); return this.denied; }, forward: (token, ev, first, second) => { event('Forward', [ev, first, second]); return this.forward; },
            tell: (value) => { event(`Tell,${encoder(value)}`); }, about: (token, first, second) => { event('About', [first, second]); return 0x80000009; },
        };
        this.hooks.attach = this.hooks.baseAttach;
    }

    // 단일 생성/트리 처리기에 모든 파생의 같은 기본 주소를 공급한다.
    base(token) {
        if (token === ITEM) return this.item; if (token === 43) return this.grid;
        return this.images.get(token) ?? this.texts.get(token) ?? this.bases.get(token) ?? null;
    }

    // 실제 인자를 원문 구분자로만 기록한다(전체 DWORD 비트).
    event(value, values = []) { this.trace += (this.trace ? ';' : '') + value; for (const entry of values) this.trace += `,${entry >>> 0}`; }

    // 같은 실제 원본 현재 상태의 live 묶음이다.
    state() {
        const scene = this;
        return { get localOwner() { return scene.owner; }, get editor() { return scene.editor; }, get stockEnabled() { return scene.stock; }, get inputDisabled() { return scene.disabled; },
            get lastType() { return scene.last; }, set lastType(value) { scene.last = value >>> 0; }, get bridgeType() { return scene.bridge; }, get pixelsX() { return scene.cellX; }, get pixelsY() { return scene.cellY; },
            get backgroundType() { return scene.background; }, get backgroundFrame() { return scene.backgroundFrame; } };
    }
}

// 탭 구분 fixture 의 주석 줄을 건너뛴 행 목록이다.
function rows(name) { return fs.readFileSync(new URL(`../../cpppj/tests/fixtures/${name}`, import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line && !line.startsWith('#')).map((line) => line.split('\t')); }

test('InventoryItemGump 원본 10.78 132행: 항목·실제 그림/글자 자식 생성·클릭·비용·장식의 노드 바이트와 호출 순서·반환·마지막 선택이 모두 일치한다', () => {
    const table = rows('inventory-item-x86.tsv'); const failures = [];
    assert.equal(table.length, 132);
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 7, `행 ${index}`);
        const s = new ItemScene(); const a = inputs(row[1]); const item = new InventoryItemGump(s.pool, s.state(), s.hooks); let result = '-'; const raw = s.pool.physical(128);
        if (row[0] === 'Attach') {
            put(raw, 18, a[0], 2); put(raw, 20, a[1], 2); put(raw, 12, a[2], 2); raw[30] = a[3]; s.gumpFrame = a[4] | 0; s.item.width = floatValue(a[5]); s.item.height = floatValue(a[6]); s.item.iconScale = floatValue(a[7]);
            s.stock = a[8]; s.cells = a[9]; s.bounds = a[10]; s.money = floatValue(a[11]); s.editor = a[12] !== 0; s.item.flags = (9 | (a[13] ? 0x10000 : 0)) >>> 0; s.robust = a[14];
            item.attach(ITEM, 5, 6, 0, 7);
        } else if (row[0] === 'Handle') {
            s.opened = a[1]; s.disabled = a[2]; s.item.requireImageHit = a[3]; s.children = a[4]; s.denied = a[5]; s.forward = a[6]; s.last = a[7]; s.secondOpen = a[8];
            if (s.children) {
                const tokens = [0x12403000, 0x12403100, 0x12403200]; const n = s.children === 3 ? 3 : 1; s.item.child = tokens[0];
                // 나머지 자식 필드는 원본과 같은 임의 기본 입력이다.
                for (let i = 0; i < n; i += 1) { const node = initialBase(); s.bases.set(tokens[i], node); node.next = i + 1 < n ? tokens[i + 1] : 0; node.command = ((s.children !== 2 && i !== 0) || s.children === 1) ? 3 : 45; }
            }
            result = String(item.handle(ITEM, a[0], 17, 0xfffffff7) >>> 0);
        } else if (row[0] === 'Draw') { s.item.decoration = a[0]; s.item.baseWidth = floatValue(a[1]); s.item.baseHeight = floatValue(a[2]); item.draw(ITEM); }
        else { assert.equal(row[0], 'Afford'); s.cost = a[0] | 0; s.money = floatValue(a[1]); s.editor = a[2] !== 0; result = String(item.affordable(128) >>> 0); }
        let nodes = '';
        // 모든 실제 생성 노드를 원본 할당 순서대로 기록한다.
        for (const [token, size] of s.born) { nodes += `${nodes ? ';' : ''}${token}=`; if (size === 180) nodes += encodeImage(s.images.get(token)); else { const data = new Uint8Array(164); data.set(Buffer.from(encodeGumpNode(s.texts.get(token)), 'hex')); put(data, 160, s.texts.get(token).head); nodes += hex(data); } }
        const problems = []; const encoded = encodeItem(s.item); const trace = s.trace || '-';
        if (result !== row[2]) problems.push('result'); if (encoded !== row[3]) problems.push('item'); if (String(s.last) !== row[4]) problems.push('last'); if ((nodes || '-') !== row[5]) problems.push('nodes'); if (trace !== row[6]) problems.push('trace');
        if (problems.length) failures.push(`${index}/${row[0]}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
});

// 한 번의 부착을 실행하는 도우미이다: 내용물(SID 128)의 종류/인자/flags/품질과 장면 입력을 정하고 항목을 부착한 뒤 장면을 돌려준다.
function attachScene(edit = {}) {
    const s = new ItemScene(); const raw = s.pool.physical(128); put(raw, 18, edit.kind ?? 128, 2); put(raw, 20, edit.flags ?? 0, 2); put(raw, 12, edit.argument ?? 3, 2); raw[30] = edit.quality ?? 3;
    s.cells = edit.cells ?? 1; s.bounds = edit.bounds ?? 0; s.gumpFrame = edit.gumpFrame ?? -1; s.stock = edit.stock ?? 0; s.item.width = edit.width ?? 40; s.item.height = edit.height ?? 40; s.item.iconScale = edit.iconScale ?? 0;
    if (edit.bridge !== undefined) s.bridge = edit.bridge; if (edit.frames) s.frames = edit.frames; if (edit.cost !== undefined) s.cost = edit.cost; if (edit.money !== undefined) s.money = edit.money; if (edit.walk) s.hooks.walk = edit.walk;
    s.item.flags = 9; const item = new InventoryItemGump(s.pool, s.state(), s.hooks); item.attach(ITEM, 5, 6, 0, 7); return { s, item };
}

test('InventoryItemGump 생성/저장소/내용물 검사: 판본·필요한 효과·노드·SID 범위가 어긋나면 도달한 위치에서 거부하고 비용 판정은 내용물 종류 WORD 로 조회한다', () => {
    const s = new ItemScene();
    assert.throws(() => new InventoryItemGump({ edition: 'cd1072', capacity: 10 }, s.state(), s.hooks), TypeError);
    assert.throws(() => new InventoryItemGump(s.pool, s.state(), {}).draw(ITEM), /실제 하위 미연결/);
    assert.throws(() => new InventoryItemGump(s.pool, s.state(), { ...s.hooks, node: () => null }).draw(ITEM), /항목 저장소 없음/); assert.throws(() => new InventoryItemGump(s.pool, s.state(), s.hooks).draw(0), /항목 저장소 없음/);
    // 자식 저장소가 없으면 클릭 처리의 자식 순회에서 거부한다.
    s.item.child = 0x12403000; s.item.requireImageHit = 0; s.opened = 0; assert.throws(() => new InventoryItemGump(s.pool, s.state(), s.hooks).handle(ITEM, 0x10000, 1, 2), /자식 저장소 없음/);
    // 내용물 SID: 0·풀 크기·65536 이상은 거부하고 65535 는 허용한다(큰 풀 대역).
    const big = { edition: 'patch1078', capacity: 100000, physical: () => new Uint8Array(32), slot: () => new Uint8Array(32) };
    for (const [content, ok] of [[0, false], [24000, false], [65535, true], [65536, false], [100000, false]]) {
        const scene = new ItemScene(); scene.item.content = content; scene.item.flags = 9; const pool = content === 24000 || content === 0 ? scene.pool : big;
        const item = new InventoryItemGump(pool, scene.state(), scene.hooks); if (ok) item.attach(ITEM, 0, 0, 0, 0); else assert.throws(() => item.attach(ITEM, 0, 0, 0, 0), /내용물SID 오류/, String(content));
    }
    // 비용 판정은 내용물 flags 와 무관하게 종류 WORD(offset 18)를 비용표 조회에 넘긴다.
    const kinds = []; const affordScene = new ItemScene(); const raw = affordScene.pool.physical(128); put(raw, 18, 0x0102, 2); put(raw, 20, 0x1234, 2);
    const probe = new InventoryItemGump(affordScene.pool, affordScene.state(), { ...affordScene.hooks, costUnits: (kind) => { kinds.push(kind); return 5; } }); probe.affordable(128); assert.deepEqual(kinds, [0x0102]);
});

test('InventoryItemGump 부착 인자: 내용물 인자가 짝수여도 패턴 질의와 측정에 그대로 전달하고 명시 프레임은 인자를 대체한다', () => {
    for (const argument of [4, 6, 12]) { const { s } = attachScene({ argument }); assert.ok(s.trace.includes(`Measure,128,${argument},0,0`), s.trace); assert.ok(s.trace.includes(`Walk,128,${argument},0,0,0,0`), s.trace); }
    for (const gumpFrame of [4, 6]) { const { s } = attachScene({ argument: 12, gumpFrame }); assert.ok(s.trace.includes(`Measure,128,${gumpFrame},0,1`), s.trace); assert.ok(s.trace.includes(`Walk,128,${gumpFrame},0,0,0,1`), s.trace); }
});

test('InventoryItemGump 다리 프레임: 품질 1/3 은 플래그 0x20/0x40 으로 부호 확장한 프레임 코드를 다시 찾고 범위 밖 프레임/표 없음은 거부한다', () => {
    // 품질별 재검색: 코드 7({A,P,0}) → 품질 1: 0x20 → 9, 품질 3: 0x40 → 11, 그 밖: 0 → 0. 코드 8({B,P,0}) → 10/12.
    for (const [quality, expected] of [[1, 9], [3, 11], [0, 0], [2, 0]]) { const { s } = attachScene({ kind: 82, bridge: 82, quality, cells: 1 }); assert.ok(s.trace.includes(`Header,82,0,${expected}`), `${quality}: ${s.trace}`); }
    const second = attachScene({ kind: 82, bridge: 82, quality: 1, cells: 2 }); assert.ok(second.s.trace.includes('Header,82,0,9;') && second.s.trace.includes('Header,82,0,10'), second.s.trace);
    // 부호 확장: 코드 플래그 0x80(−128) + 품질 1(0x20) = −96 → 플래그 0xa0(−96)인 첫 코드를 찾는다.
    const codes = Array.from({ length: 16 }, () => ({ side: 0x41, variant: 0x50, number: 0, flags: 0 })); codes[7].flags = 0x80; codes[3].flags = 0xa0; const signed = attachScene({ kind: 82, bridge: 82, quality: 1, cells: 1, frames: new FrameCodeTable(codes) });
    assert.ok(signed.s.trace.includes('Header,82,0,3'), signed.s.trace);
    // 프레임 코드 범위: −1 과 코드 수(16)는 거부하고 0 은 허용하며 표가 없으면 거부한다.
    for (const frame of [-1, 16]) assert.throws(() => attachScene({ kind: 82, bridge: 82, walk: (query, visit) => visit({ frame, x: 0, y: 0 }) }), /다리 프레임 코드 오류/, String(frame));
    attachScene({ kind: 82, bridge: 82, walk: (query, visit) => visit({ frame: 0, x: 0, y: 0 }) });
    const noTable = new ItemScene(); noTable.hooks.type = () => ({ gumpFrame: -1, frames: null }); put(noTable.pool.physical(128), 18, 82, 2); noTable.bridge = 82; noTable.cells = 1; noTable.item.flags = 9;
    assert.throws(() => new InventoryItemGump(noTable.pool, noTable.state(), noTable.hooks).attach(ITEM, 0, 0, 0, 0), /다리 프레임 코드 오류/);
});

test('InventoryItemGump 아이콘 모드와 수량 글자: flags 0x100 은 모드 1, 0x418 이 있고 살 수 없으면 모드 2 이며 낮은 비트만 있으면 모드 0 이고 글자는 재고 표시와 flags 8 에서만 만든다', () => {
    const mode = (flags, edit = {}) => { const { s } = attachScene({ flags, stock: 1, ...edit }); return { mode: [...s.images.values()][0].mode, text: s.texts.size, trace: s.trace }; };
    assert.equal(mode(0x1).mode, 0); assert.equal(mode(0x1).text, 0); assert.equal(mode(0x100).mode, 1); assert.equal(mode(0x1, { cost: 5, money: 1 }).mode, 0);
    assert.equal(mode(0x8, { cost: 5, money: 1 }).mode, 2); assert.equal(mode(0x8, { cost: 1, money: 5 }).mode, 0); assert.equal(mode(0x10, { cost: 5, money: 1 }).mode, 2); assert.equal(mode(0x400, { cost: 5, money: 1 }).mode, 2);
    // 수량 글자: 품질 3 의 `~w~E3` 를 만들어 맞춘 뒤 그림 오른쪽 아래에 부착한다. flags 8 이 없거나 재고 표시가 꺼져 있으면 만들지 않는다.
    const stocked = mode(0x8, { cost: 1, money: 5 }); assert.equal(stocked.text, 1); assert.ok(stocked.trace.includes(`Text,${BORN + 256},${hex(Buffer.from('~w~E3', 'latin1'))}`), stocked.trace);
    assert.equal(mode(0x8, { stock: 0, cost: 1, money: 5 }).text, 0); assert.equal(mode(0x1, { cost: 1, money: 5 }).text, 0); assert.equal(mode(0x2, { cost: 1, money: 5 }).text, 0);
});

test('InventoryItemGump 정밀도: 배율 비율·아이콘 위치·그림 크기·수량 글자 위치는 원문처럼 float32 저장 위치에서만 줄이고 비용 판정은 현재 SP 를 float32 로 줄여 비교한다', () => {
    const f = Math.fround;
    // 아이콘 경계가 노드보다 커서 배율을 정할 때(rx > 1): 비율을 float32 로 저장한 뒤 그 역수를 float32 로 저장한다. 비율을 줄이지 않은 값의 역수와는 다른 입력이다.
    const exactRatio = (168.4899444580078 + 2) / 81.11344146728516; const rx = f(exactRatio); assert.notEqual(f(1 / rx), f(1 / exactRatio));
    const wide = new ItemScene(); put(wide.pool.physical(128), 18, 128, 2); wide.hooks.measure = () => ({ bounds: { left: 0, top: 0, right: 168.4899444580078, bottom: 10 } }); wide.hooks.walk = (query, visit) => visit({ frame: 7, x: 0, y: 0 });
    wide.item.width = 81.11344146728516; wide.item.height = 40; wide.item.iconScale = 0; wide.item.flags = 9; new InventoryItemGump(wide.pool, wide.state(), wide.hooks).attach(ITEM, 0, 0, 0, 0);
    assert.equal([...wide.images.values()][0].scale, f(1 / rx));
    // 위치: 중심 + 칸 × 16 × 배율 의 배정도 합을 먼저 float32 로 저장(px)한 뒤 그림 폭과 더해 float32 로 저장한 것이 수량 글자 x 이다.
    const placement = (width, scale, x) => {
        const s = new ItemScene(); put(s.pool.physical(128), 18, 128, 2); put(s.pool.physical(128), 20, 8, 2); s.pool.physical(128)[30] = 3; s.stock = 1; s.item.width = width; s.item.height = 40; s.item.iconScale = scale; s.item.flags = 9;
        s.hooks.walk = (query, visit) => visit({ frame: 7, x, y: 0 }); const original = s.hooks.attach; const raw = []; s.hooks.attach = (token, ax, ay, parent, mode) => { raw.push([ax, ay]); original(token, ax, ay, parent, mode); };
        s.hooks.baseAttach = original; new InventoryItemGump(s.pool, s.state(), s.hooks).attach(ITEM, 0, 0, 0, 0);
        placement.raw = raw; return s.trace.split(';').filter((entry) => entry.startsWith('Attach,'));
    };
    for (const [width, scale, x] of [[50.376487731933594, 0.42222005128860474, 1.5494251251220703], [41.31998825073242, 0.49306783080101013, -0.48595738410949707]]) {
        const centerX = f((width - 64 * scale) * 0.5 + 2.0); const centerY = f((40 - 96 * scale) * 0.5 + 2.0); const px = f(centerX + x * 16 * scale); const imageWidth = f(scale * 40); const tx = f(px + imageWidth);
        const events = placement(width, scale, x);
        // events[0] 은 항목 자신의 기본 부착이고 그림 자식, 수량 글자 자식 순서이다.
        assert.equal(events[1], `Attach,${BORN},${floatBits(px)},${floatBits(centerY)},${ITEM},0`);
        assert.ok(events[2].startsWith(`Attach,${BORN + 256},${floatBits(tx)},`), `${events[2]} != ${floatBits(tx)}`);
        // 자식 부착 장치가 받은 숫자도 float32 로 줄인 값이다(그림 위치와 수량 글자 위치).
        assert.equal(placement.raw[0][0], px); assert.equal(placement.raw[1][0], tx); assert.ok(placement.raw.flat().every((value) => Object.is(f(value), value)), JSON.stringify(placement.raw));
        // 줄이지 않은 합/폭을 쓰면 값이 달라지는 입력인지 확인한다.
        assert.ok(f(centerX + x * 16 * scale + imageWidth) !== tx || f(px + scale * 40) !== tx);
    }
    // 비용 판정: 현재 SP 도 float32 로 줄인 뒤 비교한다(799.99999999 → 800).
    const money = new ItemScene(); const probe = new InventoryItemGump(money.pool, money.state(), { ...money.hooks, money: () => 799.99999999, costUnits: () => 800 }); assert.equal(probe.affordable(128), 1);
    const poor = new InventoryItemGump(money.pool, money.state(), { ...money.hooks, money: () => 799.9, costUnits: () => 800 }); assert.equal(poor.affordable(128), 0);
});
