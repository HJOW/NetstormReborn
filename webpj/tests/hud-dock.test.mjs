// HUD divider 생성·슬레이브 등록·재배치와 HUD 표시 모드/채팅/지도 갱신(HudDock)을 원본 10.78 독립 관측 2,150행(전체 8개 0x11c BYTE 노드·22DWORD 전역·호출 순서·반환)에 대조한다. 원본게임을 실행하지 않는다.
// 입력은 앞 6열(진입 이름·노드 raw·전역·인자·callback 효과)만 읽고 기대 열은 비교에만 쓴다. (2026-11-15 WB04-E470)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { GumpCreateNode, GumpCreateState } from '../www/ui/gump-create.js';
import { GumpTree, GumpTreeState } from '../www/ui/gump-tree.js';
import { HudDock, HudDividerNode, HudDividerGump, HudDockPart } from '../www/ui/hud-dock.js';
import { decodeGumpNode, encodeGumpNode } from './gump-node-support.mjs';

// 원문 격리 포인터의 고정 주소/간격과 노드 raw 크기·개수, 입력 열 수이다.
const NODE_ADDRESS = 0x01000100; const NODE_STRIDE = 0x200; const RAW_BYTES = 0x11c; const NODE_COUNT = 8; const COLUMNS = 14; const EXPECTED_ROWS = 2150;
// callback 효과로 중단하는 예외이며 원문 정상 반환과 구별한다.
class ExplicitStop extends Error {}
// 음수도 하위 DWORD 비트로 읽는 입력 숫자와 float32 비트 변환이다.
const number = (text) => Number(BigInt.asUintN(32, BigInt(text)));
const signed = (value) => value | 0;
function floatBits(value) { const view = new DataView(new ArrayBuffer(4)); view.setFloat32(0, value, true); return view.getUint32(0, true); }
function floatValue(bits) { const view = new DataView(new ArrayBuffer(4)); view.setUint32(0, bits >>> 0, true); return view.getFloat32(0, true); }
// 원시 주소 0 또는 고정 노드 주소를 1기반 검사 토큰으로 바꾸고 되돌린다.
function tokenOf(address) {
    if (!address) return 0;
    if (address < NODE_ADDRESS || (address - NODE_ADDRESS) % NODE_STRIDE || (address - NODE_ADDRESS) / NODE_STRIDE >= NODE_COUNT) throw new Error('HUD raw 포인터 범위');
    return (address - NODE_ADDRESS) / NODE_STRIDE + 1;
}
const addressOf = (token) => (token ? NODE_ADDRESS + (token - 1) * NODE_STRIDE : 0);
// 바이트 배열의 DWORD 읽기/쓰기(little endian)이다.
const get = (raw, offset) => new DataView(raw.buffer, raw.byteOffset + offset, 4).getUint32(0, true);
const put = (raw, offset, value) => new DataView(raw.buffer, raw.byteOffset + offset, 4).setUint32(0, value >>> 0, true);
// effect 가 가리키는 노드 필드의 원본 offset 표이다.
const FIELDS = { flags: 4, parent: 0x14, type: 0x20, serial: 0x2c, scale: 0x38, baseWidth: 0x3c, baseHeight: 0x40, width: 0x44, height: 0x48, x: 0x4c, y: 0x50, axis: 0xb8, minX: 0xbc, maxX: 0xc0, minY: 0xc4, maxY: 0xc8,
    link0serial: 0xcc, link0flags: 0xd0, link1serial: 0xd4, link1flags: 0xd8, vertical: 0xa0, minimapDivider: 0xa4, chatDivider: 0xa8, minimap: 0xac, production: 0xb0, chat: 0xb8, knowledge: 0xc0 };
// 보고서와 같은 전역 순서이다.
const GLOBAL_NAMES = ['screenWidth', 'screenHeight', 'showHud', 'mapLeft', 'mapTop', 'mapWidth', 'mapHeight', 'floatLeft', 'floatTop', 'floatRight', 'floatBottom', 'rectLeft', 'rectTop', 'rectRight', 'rectBottom',
    'bottomLeft', 'bottomTop', 'bottomRight', 'bottomBottom', 'fullRefresh', 'savedChatY', 'presentMask'];

// 계산 기대값 없이 독립 입력의 실제 저장소와 명시 외부 장치만 공급한다.
class DockScene {
    // 실제 노드 8개(공통 기본+확장), HUD 현재 직계 토큰, 실제 전역(live 객체), 명시 serial 장치(presentMask), 색(보호 PE 00540452 의 signed BYTE)이다.
    storage = Array.from({ length: NODE_COUNT }, () => new HudDividerNode());
    children = { root: 0, vertical: 0, minimapDivider: 0, chatDivider: 0, minimap: 0, production: 0, chat: 0, knowledge: 0 };
    globals = { screenWidth: 0, screenHeight: 0, showHud: 0, mapLeft: 0, mapTop: 0, mapWidth: 0, mapHeight: 0, mapFloatRect: [0, 0, 0, 0], mapRect: { left: 0, top: 0, right: 0, bottom: 0 }, leftBottomRect: { left: 0, top: 0, right: 0, bottom: 0 }, fullRefresh: 0, savedChatY: 0 };
    presentMask = 0; color = 55; operation = ''; trace = ''; effects = []; calls = new Map(); create = new GumpCreateState(); treeState = new GumpTreeState();

    // 원본 이름 목록과 입력 6열로 만든다: 노드 raw 8개·22 전역·callback 효과를 설치하고 실제 HudDock 을 같은 노드/전역 객체에 연결한다.
    constructor(row) {
        this.operation = row[1];
        const raw = Uint8Array.from(Buffer.from(row[2], 'hex')); if (raw.length !== RAW_BYTES * NODE_COUNT) throw new Error('HUD 입력 raw 폭');
        // 고정 8개 현재 노드를 설치한다.
        for (let index = 0; index < NODE_COUNT; index += 1) this.decodeNode(index, raw.subarray(index * RAW_BYTES, (index + 1) * RAW_BYTES));
        const globals = row[3].split(':'); if (globals.length !== 22) throw new Error('HUD 전역 입력 폭');
        this.setGlobals(globals.map(number));
        if (row[5] !== '-') for (const effect of row[5].split(';')) this.effects.push(effect.split('/'));
        this.tree = new GumpTree(this.create, this.treeState, this.treeHooks());
        this.dock = new HudDock((part) => this.children[Object.keys(this.children)[part]], this.globals, () => this.color, this.hooks());
    }

    // 유효한 토큰의 같은 상속 저장소이다.
    node(token) { if (!token || token > this.storage.length) throw new Error('HUD 검사 토큰'); return this.storage[token - 1]; }

    // 원시 필드의 입력을 같은 실제 노드로 읽는다(확장 필드와 slave 20DWORD 포함). HUD 루트의 +a0.. 는 직계 자식 포인터이다.
    decodeNode(index, raw) {
        const node = this.storage[index]; Object.assign(node, decodeGumpNode(Buffer.from(raw.subarray(0, 160)).toString('hex'))); node.parent = tokenOf(node.parent);
        node.shape = get(raw, 0xa0); node.tileWidth = signed(get(raw, 0xa4)); node.tileHeight = signed(get(raw, 0xa8)); node.type = get(raw, 0xac); node.frame = get(raw, 0xb0); node.backgroundFlags = get(raw, 0xb4);
        node.axis = get(raw, 0xb8); node.minX = signed(get(raw, 0xbc)); node.maxX = signed(get(raw, 0xc0)); node.minY = signed(get(raw, 0xc4)); node.maxY = signed(get(raw, 0xc8));
        // 모든 20DWORD 를 원래 슬롯 순서대로 읽는다.
        for (let i = 0; i < node.slaves.length; i += 1) node.slaves[i] = [get(raw, 0xcc + 8 * i), get(raw, 0xd0 + 8 * i)];
        if (this.operation === 'hud' && index === 0) {
            this.children = { root: 1, vertical: tokenOf(node.shape), minimapDivider: tokenOf(node.tileWidth >>> 0), chatDivider: tokenOf(node.tileHeight >>> 0), minimap: tokenOf(node.type), production: tokenOf(node.frame), chat: tokenOf(node.axis), knowledge: tokenOf(node.maxX >>> 0) };
        }
    }

    // 호스트 필드를 원문 offset 에만 쓴다. 기대 기하/분기를 계산하지 않는다.
    encodeNode(index) {
        const node = this.storage[index]; const base = Object.assign(new GumpCreateNode(), node); base.vtable = 0; base.parent = addressOf(base.parent);
        const raw = new Uint8Array(RAW_BYTES); raw.set(Buffer.from(encodeGumpNode(base), 'hex'));
        put(raw, 0xa0, node.shape); put(raw, 0xa4, node.tileWidth); put(raw, 0xa8, node.tileHeight); put(raw, 0xac, node.type); put(raw, 0xb0, node.frame); put(raw, 0xb4, node.backgroundFlags);
        put(raw, 0xb8, node.axis); put(raw, 0xbc, node.minX); put(raw, 0xc0, node.maxX); put(raw, 0xc4, node.minY); put(raw, 0xc8, node.maxY);
        // 원본의 전체 고정 슬롯을 비교한다.
        for (let i = 0; i < node.slaves.length; i += 1) { put(raw, 0xcc + 8 * i, node.slaves[i][0]); put(raw, 0xd0 + 8 * i, node.slaves[i][1]); }
        if (this.operation === 'hud' && index === 0) {
            const c = this.children; put(raw, 0xa0, addressOf(c.vertical)); put(raw, 0xa4, addressOf(c.minimapDivider)); put(raw, 0xa8, addressOf(c.chatDivider)); put(raw, 0xac, addressOf(c.minimap));
            put(raw, 0xb0, addressOf(c.production)); put(raw, 0xb8, addressOf(c.chat)); put(raw, 0xc0, addressOf(c.knowledge));
        }
        return raw;
    }

    // 콜백과 실행 뒤 같은 22DWORD 전역/장치 상태를 직렬화한다.
    globalValues() {
        const g = this.globals;
        return [g.screenWidth, g.screenHeight, g.showHud, g.mapLeft, g.mapTop, g.mapWidth, g.mapHeight, ...g.mapFloatRect.map(floatBits), g.mapRect.left, g.mapRect.top, g.mapRect.right, g.mapRect.bottom,
            g.leftBottomRect.left, g.leftBottomRect.top, g.leftBottomRect.right, g.leftBottomRect.bottom, g.fullRefresh, floatBits(g.savedChatY), this.presentMask].map((value) => value >>> 0);
    }

    // 입력된 실제 전역을 같은 객체에 쓴다.
    setGlobals(values) {
        const g = this.globals;
        g.screenWidth = signed(values[0]); g.screenHeight = signed(values[1]); g.showHud = values[2]; g.mapLeft = signed(values[3]); g.mapTop = signed(values[4]); g.mapWidth = signed(values[5]); g.mapHeight = signed(values[6]);
        for (let i = 0; i < 4; i += 1) g.mapFloatRect[i] = floatValue(values[7 + i]);
        Object.assign(g.mapRect, { left: signed(values[11]), top: signed(values[12]), right: signed(values[13]), bottom: signed(values[14]) });
        Object.assign(g.leftBottomRect, { left: signed(values[15]), top: signed(values[16]), right: signed(values[17]), bottom: signed(values[18]) });
        g.fullRefresh = values[19]; g.savedChatY = floatValue(values[20]); this.presentMask = values[21];
    }

    // 현재 경계의 raw 장치만 변경한다. 기대 출력 자료는 여기서 읽지 않는다.
    applyEffect(effect) {
        const value = number(effect[3]);
        if (effect[1].startsWith('node')) {
            const index = Number(effect[1].slice(4)); const raw = this.encodeNode(index); put(raw, FIELDS[effect[2]], value); this.decodeNode(index, raw);
        } else if (effect[1] === 'global') {
            const values = this.globalValues(); const at = GLOBAL_NAMES.indexOf(effect[2]); if (at < 0) throw new Error('HUD 전역효과 이름'); values[at] = value; this.setGlobals(values);
        } else if (effect[1] !== 'control') throw new Error('HUD 효과 대상');
    }

    // 원문과 같은 장치 기본 효과 뒤 같은 호출 번호의 mutation 을 적용한다.
    event(name, values = []) {
        if (this.trace) this.trace += ';';
        this.trace += name;
        // 실제 인자만 순서대로 관측한다.
        for (const value of values) this.trace += `:${value >>> 0}`;
        const count = (this.calls.get(name) ?? 0) + 1; this.calls.set(name, count); const trigger = `${name}${count}`; let stop = false;
        // 경계 안의 효과 순서와 부분 중단을 보존한다.
        for (const effect of this.effects) {
            if (effect[0] !== trigger) continue;
            this.applyEffect(effect); if (effect[1] === 'control' && effect[2] === 'stop' && effect[3] === '1') stop = true;
        }
        if (stop) throw new ExplicitStop();
    }

    // Native 명시 기본 Attach 장치와 같은 상태/인자를 제공한다.
    baseAttach(name, token, x, y, parent, mode) {
        const node = this.node(token); node.flags = (node.flags & ~8) >>> 0; node.x = x; node.y = y; if (parent) node.parent = parent;
        this.event(name, [token, floatBits(x), floatBits(y), parent, mode]);
    }

    // Native 가상 Push 경계이며 공통 트리 분리 자체를 검증 범위로 확대하지 않는다.
    push(token) { const node = this.node(token); node.flags = (node.flags | 8) >>> 0; this.event('push', [token]); }

    // 실제 nested divider 본체와 일반 UI 의 명시 Attach 경계를 구별한다.
    attach(token, x, y, parent, mode) {
        if (this.operation === 'hud' && token >= 2 && token <= 4) this.dock.attach(token, x, y, parent, mode); else this.baseAttach('attach', token, x, y, parent, mode);
    }

    // 공통 Move 에 필요한 실제 노드와 명시 최종 가상 경계를 공급한다.
    treeHooks() { return { node: (token) => this.node(token), redraw: () => {}, push: (token) => this.push(token), attach: (token, x, y, parent, mode) => this.attach(token, x, y, parent, mode), kill: () => {} }; }

    // 원문의 나머지 명시 외부 경계를 같은 공유 상태로 연결한다.
    hooks() {
        return {
            node: (token) => this.node(token), divider: (token) => this.node(token),
            constructBackground: (token, w, h, flags, type, frame) => {
                const node = this.node(token); node.flags = 9; node.scale = 1; node.baseWidth = node.width = w; node.baseHeight = node.height = h; node.type = type; node.frame = frame; node.backgroundFlags = flags;
                this.event('background', [token, floatBits(w), floatBits(h), flags, 1, type, frame]);
            },
            activate: (token, active) => { this.event('activate', [token, active]); },
            baseAttach: (token, x, y, parent, mode) => { this.baseAttach('baseAttach', token, x, y, parent, mode); },
            push: (token) => { this.push(token); }, attach: (token, x, y, parent, mode) => { this.attach(token, x, y, parent, mode); },
            move: (token, x, y, parent) => { this.tree.move(token, x, y, parent); },
            // 첫 현재 일치만 선택한다.
            resolveSerial: (serial) => {
                let token = 0;
                for (let i = 0; i < this.storage.length; i += 1) if ((this.presentMask & (1 << i)) && this.storage[i].serial === serial) { token = i + 1; break; }
                this.event('resolve', [serial, token]); return token;
            },
            bubble: (token, event, first, second) => { this.event('bubble', [token, event, first, second]); if (this.operation === 'hud') this.dock.handle(event, first, second); },
            closePopups: () => { this.event('closePopups'); }, invalidate: (x, y, w, h, flag) => { this.event('invalidate', [x, y, w, h, flag]); }, finishResize: () => { this.event('finish'); },
            assertion: (line) => { this.event('assert', [line]); }, baseHandle: (event, first, second) => { this.event('baseHandle', [event, first, second]); return 73; },
            chatVisible: (value) => { this.event('chatVisible', [value ? 1 : 0]); }, chatInput: (value) => { this.event('chatInput', [value ? 1 : 0]); },
        };
    }

    // 현재 전체 노드의 원시 관측을 붙인다(원문 주소 순서).
    nodesOutput() { let result = ''; for (let i = 0; i < this.storage.length; i += 1) result += Buffer.from(this.encodeNode(i)).toString('hex'); return result; }

    // 실제 전체 전역을 관측한다(원문 전역 순서).
    globalsOutput() { return this.globalValues().join(':'); }
}

// 탭 구분 fixture 의 머리글을 건너뛴 행 목록이다. 빈 열도 보존한다.
function rows(name) { return fs.readFileSync(new URL(`../../cpppj/tests/fixtures/${name}`, import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line && !line.startsWith('name\t')).map((line) => line.split('\t')); }

test('HudDock 원본 10.78 2,150행: 생성/등록/모든 관계 BYTE/중첩 모드·채팅·viewport 와 callback 부분 쓰기가 전체 노드·전역·호출 순서·반환까지 일치한다', () => {
    const table = rows('hud-dock-geometry-x86.tsv'); let stopped = 0; const failures = [];
    assert.equal(table.length, EXPECTED_ROWS);
    for (const row of table) {
        assert.equal(row.length, COLUMNS, row[0]);
        const scene = new DockScene(row); const args = row[4].split(':').map(number); let result = 0; let completed = true;
        try {
            if (row[1] === 'construct') result = scene.dock.construct(1, args[0], signed(args[1]), signed(args[2]), args[3], args[4], args[5]);
            else if (row[1] === 'register') scene.dock.registerSlave(1, args[0], args[1]);
            else if (row[1] === 'reflow') scene.dock.attach(1, floatValue(args[0]), floatValue(args[1]), args[2], args[3]);
            else if (row[1] === 'hud') result = scene.dock.handle(args[0], args[1], args[2]);
            else throw new Error('HUD 진입 이름');
        } catch (error) { if (!(error instanceof ExplicitStop)) throw error; completed = false; stopped += 1; }
        const problems = [];
        if (completed !== (row[6] === '1')) problems.push('completed');
        if (scene.nodesOutput() !== row[7]) problems.push('nodes');
        if (scene.globalsOutput() !== row[8]) problems.push('globals');
        if ((scene.trace || '-') !== row[9]) problems.push('trace');
        // void/미완료 EAX 를 의미 반환값으로 비교하지 않는다.
        if (completed && (row[1] === 'construct' || row[1] === 'hud') && result >>> 0 !== number(row[10])) problems.push('result');
        if (completed && number(row[11]) !== 4 + args.length * 4) problems.push('stackdelta');
        if (!(number(row[12]) > 0 && number(row[12]) >= number(row[13]))) problems.push('instructions');
        if (problems.length) failures.push(`${row[0]}/${row[1]}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []); assert.equal(stopped, 6);
});

// 원본 관측이 닿지 않는 독립 시험용 장면이다. 노드 8개를 0 으로 채운 HUD 입력에서 시작해 직계 토큰(루트 1, 세로 2, 지도 구분선 3, 채팅 구분선 4, 지도 5, 생산 6, 채팅 7, 지식 8)과 serial(101+번호)을 둔다.
function plainScene(operation = 'hud') {
    const row = ['plain', operation, '00'.repeat(RAW_BYTES * NODE_COUNT), new Array(22).fill('0').join(':'), '0', '-'];
    const scene = new DockScene(row);
    scene.children = { root: 1, vertical: 2, minimapDivider: 3, chatDivider: 4, minimap: 5, production: 6, chat: 7, knowledge: 8 };
    // 노드마다 서로 다른 serial 을 준다.
    for (let i = 0; i < scene.storage.length; i += 1) scene.storage[i].serial = 101 + i;
    scene.globals.screenWidth = 1024; scene.globals.screenHeight = 768;
    return scene;
}

// 모든 노드의 슬레이브 칸을 비운다.
function clearSlaves(scene) { for (const node of scene.storage) for (const slave of node.slaves) { slave[0] = 0; slave[1] = 0; } }

test('HudDividerNode 기본값과 HudDockPart: 확장 필드는 0 이고 슬레이브 10칸은 [0, 0] 이며 직계 종류 번호는 서로 다르다', () => {
    const node = new HudDividerNode();
    assert.deepEqual([node.axis, node.minX, node.maxX, node.minY, node.maxY], [0, 0, 0, 0, 0]); assert.equal(node.slaves.length, 10); assert.ok(node.slaves.every((slave) => slave.length === 2 && slave[0] === 0 && slave[1] === 0));
    assert.deepEqual(Object.values(HudDockPart), [0, 1, 2, 3, 4, 5, 6, 7]);
});

test('HudDock 생성 검사: 노드/트리 처리기 아홉 개 중 하나라도 없으면 TypeError 이고 직계 토큰은 객체와 함수 두 형태가 같다', () => {
    const scene = plainScene(); const base = scene.hooks();
    for (const name of ['node', 'divider', 'baseAttach', 'push', 'attach', 'move', 'resolveSerial', 'bubble', 'closePopups']) assert.throws(() => new HudDock(scene.children, scene.globals, () => 55, { ...base, [name]: undefined }), TypeError, name);
    // 객체 형태와 함수 형태는 같은 현재 토큰을 읽고 값을 캐시하지 않는다.
    const viaObject = new HudDock(scene.children, scene.globals, () => 55, base); const viaFunction = new HudDock((part) => scene.children[Object.keys(scene.children)[part]], scene.globals, () => 55, base);
    for (const dock of [viaObject, viaFunction]) { clearSlaves(scene); dock.registerNormalLayout(); assert.deepEqual(scene.storage[1].slaves[0], [103, 1]); }
    scene.children.minimapDivider = 5; clearSlaves(scene); viaObject.registerNormalLayout(); assert.deepEqual(scene.storage[1].slaves[0], [105, 1]);
});

test('HudDock 표준 배치 등록: 원문 순서의 열 번 등록과 세 divider 의 제한·축 비트(기존 비트 보존)', () => {
    const scene = plainScene(); scene.storage[1].axis = 0x30; scene.storage[2].axis = 0x02; scene.storage[3].axis = 0x02;
    scene.dock.registerNormalLayout();
    // 세로 divider(2): 지도 구분선·채팅 구분선·지도·생산·지식·채팅, 지도 구분선(3): 지도·생산·지식, 채팅 구분선(4): 채팅.
    assert.deepEqual(scene.storage[1].slaves.slice(0, 7), [[103, 1], [104, 4], [105, 1], [106, 1], [108, 1], [107, 4], [0, 0]]);
    assert.deepEqual(scene.storage[2].slaves.slice(0, 4), [[105, 8], [106, 2], [108, 2], [0, 0]]); assert.deepEqual(scene.storage[3].slaves.slice(0, 2), [[107, 8], [0, 0]]);
    // 등록된 자식은 40000 비트를 갖는다.
    for (const index of [2, 3, 4, 5, 6, 7]) assert.equal(scene.storage[index].flags, 0x40000, String(index));
    assert.deepEqual([scene.storage[1].minX, scene.storage[1].maxX, scene.storage[1].axis], [60, 130, 0x34]);
    assert.deepEqual([scene.storage[2].minY, scene.storage[2].maxY, scene.storage[2].axis], [768 - 123, 768, 0x0a]); assert.deepEqual([scene.storage[3].minY, scene.storage[3].maxY, scene.storage[3].axis], [25, 768 - 25, 0x0a]);
    // 호출 효과는 없다(호출 순서 trace 비어 있음).
    assert.equal(scene.trace, '');
});

test('HudDividerGump 생성: 배경 인자는 signed→float32 이고 가상 표/플래그/색/축을 쓰며 슬레이브만 지우고 제한은 보존한다', () => {
    const scene = plainScene('construct'); const node = scene.storage[0]; node.minX = -7; node.maxX = 9; node.minY = 11; node.maxY = -13; node.slaves[3] = [55, 66]; node.slaves[9] = [77, 88];
    const hooks = scene.hooks(); hooks.constructBackground = (token, w, h, flags, type, frame) => { scene.storage[token - 1].flags = 0; scene.event('background', [token, floatBits(w), floatBits(h), flags, type, frame]); };
    const gump = new HudDividerGump(scene.globals, () => -55, hooks);
    // 짝수 폭/높이도 하위 비트 변화 없이 float 로 전달한다.
    assert.equal(gump.construct(1, 0x12, 100, -20, 0xa0b0c0d0, 5, 6), 1);
    assert.equal(scene.trace, `background:1:${floatBits(100)}:${floatBits(-20)}:${0xa0b0c0d0}:5:6;activate:1:1`);
    assert.equal(node.vtable, 0x514fd8); assert.equal(node.flags, 0x60200); assert.equal(node.command, 0xffffffc9); assert.equal(node.axis, 0x12);
    assert.deepEqual([node.minX, node.maxX, node.minY, node.maxY], [-7, 9, 11, -13]); assert.ok(node.slaves.every((slave) => slave[0] === 0 && slave[1] === 0));
    // 경계가 없으면 거부한다.
    assert.throws(() => new HudDividerGump(scene.globals, () => 0, { ...hooks, constructBackground: undefined }).construct(1, 0, 1, 1, 0, 0, 0), /생성 경계 없음/);
    assert.throws(() => new HudDividerGump(scene.globals, () => 0, { ...hooks, activate: undefined }).construct(1, 0, 1, 1, 0, 0, 0), /생성 경계 없음/);
    assert.throws(() => new HudDividerGump(scene.globals, () => 0, { ...hooks, constructBackground: () => {} }).construct(0, 0, 1, 1, 0, 0, 0), /파생 저장소 불일치/);
});

test('HudDividerGump 슬레이브 등록: 첫 빈 칸·중복 허용·가득 차면 진단 83 뒤에도 자식 비트를 남기고 진단 경계가 없으면 거부하며 저장소 불일치는 거부한다', () => {
    const scene = plainScene('register'); const master = scene.storage[0]; const gump = new HudDividerGump(scene.globals, () => 55, scene.hooks());
    gump.registerSlave(1, 0, 4); assert.equal(master.slaves[0][0], 0); assert.equal(scene.storage[1].flags, 0);
    gump.registerSlave(1, 2, 4); gump.registerSlave(1, 2, 5); assert.deepEqual(master.slaves.slice(0, 3), [[102, 4], [102, 5], [0, 0]]); assert.equal(scene.storage[1].flags, 0x40000);
    // 앞쪽에 구멍이 생기면 첫 구멍에 쓴다.
    master.slaves[0] = [0, 0]; gump.registerSlave(1, 3, 6); assert.deepEqual(master.slaves[0], [103, 6]);
    for (let i = 0; i < 10; i += 1) master.slaves[i] = [200 + i, 0];
    gump.registerSlave(1, 4, 7); assert.equal(scene.trace, 'assert:83'); assert.ok(master.slaves.every((slave, i) => slave[0] === 200 + i && slave[1] === 0)); assert.equal(scene.storage[3].flags, 0x40000);
    // 진단 경계가 없으면 자식 비트를 쓰기 전에 거부한다.
    scene.storage[4].flags = 0; assert.throws(() => new HudDividerGump(scene.globals, () => 55, { ...scene.hooks(), assertion: undefined }).registerSlave(1, 5, 1), /진단 경계 없음/); assert.equal(scene.storage[4].flags, 0);
    // 확장 저장소가 기본 저장소와 다른 객체이면 거부한다.
    assert.throws(() => new HudDividerGump(scene.globals, () => 55, { ...scene.hooks(), divider: () => new HudDividerNode() }).registerSlave(1, 2, 1), /파생 저장소 불일치/);
    assert.throws(() => new HudDividerGump(scene.globals, () => 55, { ...scene.hooks(), divider: () => null }).registerSlave(1, 2, 1), /파생 저장소 불일치/);
    assert.throws(() => new HudDividerGump(scene.globals, () => 55, { ...scene.hooks(), divider: undefined }).registerSlave(1, 2, 1), /파생 저장소 불일치/);
});

test('HudDividerGump 재배치: 폭 연동 비트(0x10)가 없으면 폭을 건드리지 않고 있으면 화면 폭에서 x 를 뺀다', () => {
    for (const [axis, expected] of [[0x01, 50], [0x10, 1024 - 40]]) {
        const scene = plainScene('reflow'); const master = scene.storage[0]; master.axis = axis; master.flags = 1; master.scale = 1; master.baseWidth = 50; master.width = 50;
        scene.dock.attach(1, 40, 7, 0, 0); assert.equal(master.baseWidth, expected, String(axis)); if (axis === 1) assert.equal(master.width, 50);
    }
});

test('HudDock HUD 명령: 표시 모드는 지정 비트만 켜고 끄며 다른 플래그 비트를 보존하고, 지도 구분선이 없으면 좌하단 영역을 0 으로 지운다', () => {
    const scene = plainScene(); scene.storage[1].baseWidth = 130; for (const index of [1, 2, 4, 5, 7]) scene.storage[index].flags = 0x1a;
    // 세로/지도 divider 는 이어서 Move 의 기본 Attach 로 이동 비트(8)가 지워지므로 0x13, 지도/생산은 0x1b 이다.
    scene.dock.handle(0x0200003d, 1, 0); assert.deepEqual([1, 2, 4, 5].map((index) => scene.storage[index].flags), [0x13, 0x13, 0x1b, 0x1b]); assert.equal(scene.storage[7].flags, 0x1a);
    for (const index of [1, 2, 4, 5, 7]) scene.storage[index].flags = 0x1b;
    scene.dock.handle(0x0200003d, 3, 0); assert.equal(scene.storage[7].flags, 0x1a);
    scene.dock.handle(0x0200003d, 4, 0); assert.equal(scene.storage[5].flags, 0x1a); assert.equal(scene.storage[7].flags, 0x1b); assert.equal(scene.storage[4].flags, 0x1a);
    // release 비트(0x80000000)가 붙은 이동 사건도 같은 명령으로 처리하고 지도 구분선이 없으면 좌하단 영역을 지운다.
    scene.children.minimapDivider = 0; Object.assign(scene.globals.leftBottomRect, { left: 1, top: 2, right: 3, bottom: 4 });
    assert.equal(scene.dock.handle((0x0200003a | 0x80000000) >>> 0, 999, 0), 1); assert.deepEqual(scene.globals.leftBottomRect, { left: 0, top: 0, right: 0, bottom: 0 });
    // 무한/범위 밖 좌표는 ftol 하위 DWORD 가 0 이다.
    scene.storage[1].serial = 999; scene.storage[1].x = 3e38; scene.storage[1].baseWidth = 3e38; scene.dock.handle(0x0200003a, 999, 0); assert.equal(scene.globals.mapLeft, 0);
});

test('HudDock 경계 오류: 채팅 설정·dirty·기본 입력·화면 후처리 경계가 없으면 각각 도달한 위치에서 거부하고 기본 입력 결과는 그대로 돌려준다', () => {
    const build = (hooks) => { const scene = plainScene(); const dock = new HudDock(scene.children, scene.globals, () => 55, { ...scene.hooks(), ...hooks }); return { scene, dock }; };
    assert.throws(() => build({ chatVisible: undefined }).dock.handle(0x02000088, 0, 1), /채팅 설정 경계/); assert.throws(() => build({ chatInput: undefined }).dock.handle(0x02000088, 0, 1), /채팅 설정 경계/);
    assert.throws(() => build({ invalidate: undefined }).dock.handle(0x0200003a, 1, 0), /dirty 경계/); assert.throws(() => build({ baseHandle: undefined }).dock.handle(0x123, 1, 2), /기본 입력 경계/);
    assert.throws(() => build({ finishResize: undefined }).dock.handle(0x0200003d, 0, 0), /화면 후처리 경계/);
    const { scene, dock } = build({}); assert.equal(dock.handle(0x123, 1, 2), 73); assert.equal(scene.trace, 'baseHandle:291:1:2');
    // 채팅 토글은 실행 뒤에도 기본 입력 분배로 이어진다.
    const toggle = plainScene(); toggle.dock.handle(0x02000088, 0, 1); assert.ok(toggle.trace.endsWith('baseHandle:33554568:0:1'));
    // 토큰 0 이나 저장소 없는 토큰은 거부한다. 파생 저장소가 기본 저장소와 다르거나 없으면 거부한다.
    assert.throws(() => build({ node: () => null }).dock.handle(0x0200003a, 1, 0), /HUD Gump 저장소 없음/);
    const mismatch = plainScene(); const different = new HudDock(mismatch.children, mismatch.globals, () => 55, { ...mismatch.hooks(), divider: () => new HudDividerNode() });
    assert.throws(() => different.registerNormalLayout(), /파생 저장소 불일치/);
    // 표준 배치의 제한 단계에서 파생 저장소가 기본 저장소와 달라지면(열한 번째 조회) 동일 저장소가 필요하다고 거부한다.
    const late = plainScene(); let lookups = 0; const lateDock = new HudDock(late.children, late.globals, () => 55, { ...late.hooks(), divider: (token) => (++lookups > 10 ? new HudDividerNode() : late.node(token)) });
    assert.throws(() => lateDock.registerNormalLayout(), /동일 저장소 필요/);
    // 기본 저장소 조회 장치가 없으면 파생 저장소 확인 단계에서 같은 오류이다(함수 호출 오류가 아님).
    const bare = plainScene(); assert.throws(() => new HudDividerGump(bare.globals, () => 55, { ...bare.hooks(), node: undefined }).registerSlave(1, 2, 1), /기본 저장소 없음/);
    const missing = plainScene(); const noDivider = new HudDock(missing.children, missing.globals, () => 55, { ...missing.hooks(), divider: () => null });
    assert.throws(() => noDivider.registerNormalLayout(), /파생 저장소 불일치/);
});

// 슬레이브 재배치용 장면이다: 마스터(토큰 1)가 자식(토큰 2, serial 102)을 relation 으로 등록하고 재부착을 실행한다. 자식에게 가는 attach 인자(원시 숫자)를 그대로 기록한다.
function relationScene(relation, master, child, attach) {
    const scene = plainScene('reflow'); const node = scene.storage[0]; const other = scene.storage[1]; const captured = [];
    node.flags = 1; node.slaves[0] = [102, relation]; scene.presentMask = 2; Object.assign(node, master); Object.assign(other, child);
    const hooks = scene.hooks(); hooks.attach = (token, x, y, parent, mode) => { captured.push([token, x, y, parent, mode]); };
    const dock = new HudDock(scene.children, scene.globals, () => 55, hooks); dock.attach(1, attach.x, attach.y, 0, 0);
    return { scene, node, other, captured };
}

test('HudDividerGump 재배치 정밀도: 폭/높이는 확장 값에 현재 scale 을 한 번 곱한 뒤 float32 로 저장하고 attach 인자도 float32 이다', () => {
    const f = Math.fround;
    // 관계 1: 자식 폭 = 마스터 x − 자식 x. 단일 곱과 먼저 줄인 곱이 다른 입력이다.
    // 관측값과 경계값의 double 반올림을 문자열→Number 변환으로 명시한다.
    let [mx, cx, scale] = [-916.906494140625, Number('520.8770141601562'), 0.7855936884880066];
    assert.notEqual(f((mx - cx) * scale), f(f(mx - cx) * scale));
    let got = relationScene(1, { x: 0, y: 0, baseWidth: 0, baseHeight: 0 }, { x: cx, y: 3, scale, baseWidth: 0, width: 0, baseHeight: 0, height: 0 }, { x: mx, y: 0 });
    assert.equal(got.other.baseWidth, f(mx - cx)); assert.equal(got.other.width, f((mx - cx) * scale)); assert.deepEqual(got.captured, [[2, cx, 3, 0, 1]]);
    // 관계 4: 오른쪽 = 마스터 baseWidth + x(배정도 합), 폭 = 화면 폭 − 오른쪽. attach 의 x 는 그 합의 float32 이다.
    let bw; [bw, mx, scale] = [-154.4854278564453, 1.8625634908676147, 0.9063773155212402]; const right = bw + mx;
    assert.ok(f((1024 - right) * scale) !== f((1024 - f(right)) * scale) || f(1024 - right) !== f(1024 - f(right)));
    got = relationScene(4, { baseWidth: bw, baseHeight: 0, y: 0 }, { x: 5, y: 6, scale, baseWidth: 0, width: 0, baseHeight: 0, height: 0 }, { x: mx, y: 0 });
    assert.equal(got.other.baseWidth, f(1024 - right)); assert.equal(got.other.width, f((1024 - right) * scale)); assert.deepEqual(got.captured, [[2, f(right), 6, 0, 1]]); assert.ok(Object.is(got.captured[0][1], f(got.captured[0][1])));
    // 관계 2: 자식 높이 = 마스터 y − 자식 y(0x10/0x20 없음).
    // 관측값과 경계값의 double 반올림을 문자열→Number 변환으로 명시한다.
    let my; let cy; [my, cy, scale] = [-916.906494140625, Number('520.8770141601562'), 0.7855936884880066]; assert.notEqual(f((my - cy) * scale), f(f(my - cy) * scale));
    got = relationScene(2, { x: 0, y: 0 }, { x: 4, y: cy, scale, baseHeight: 0, height: 0 }, { x: 0, y: my });
    assert.equal(got.other.baseHeight, f(my - cy)); assert.equal(got.other.height, f((my - cy) * scale)); assert.deepEqual(got.captured, [[2, 4, cy, 0, 1]]);
    // 관계 8: 아래 = 마스터 baseHeight + y(배정도 합), 높이 = 화면 높이 − 아래. attach 의 y 는 그 합의 float32 이다.
    let bh; [bh, my, scale] = [-166.18467712402344, -10.066699028015137, 1.4021706581115723]; const bottom = bh + my;
    assert.ok(f((768 - bottom) * scale) !== f((768 - f(bottom)) * scale) || f(768 - bottom) !== f(768 - f(bottom)));
    got = relationScene(8, { baseHeight: bh, baseWidth: 0, x: 0 }, { x: 7, y: 8, scale, baseWidth: 0, width: 0, baseHeight: 0, height: 0 }, { x: 0, y: my });
    assert.equal(got.other.baseHeight, f(768 - bottom)); assert.equal(got.other.height, f((768 - bottom) * scale)); assert.deepEqual(got.captured, [[2, 7, f(bottom), 0, 1]]); assert.ok(Object.is(got.captured[0][2], f(got.captured[0][2])));
    // 관계 8+0x40: 높이 = (이전 마스터 y − 새 y) + 자식 baseHeight. 차이를 먼저 float 로 줄이지 않는다.
    let [oy, cbh] = [-187.3014373779297, 136.03646850585938]; my = 25.045419692993164; scale = 1.1661425828933716;
    assert.ok(f(((oy - my) + cbh) * scale) !== f((f(oy - my) + cbh) * scale) || f((oy - my) + cbh) !== f(f(oy - my) + cbh));
    got = relationScene(0x48, { y: oy, baseHeight: 0, x: 0 }, { x: 7, y: 8, scale, baseWidth: 0, width: 0, baseHeight: cbh, height: 0 }, { x: 0, y: my });
    assert.equal(got.other.baseHeight, f((oy - my) + cbh)); assert.equal(got.other.height, f(((oy - my) + cbh) * scale));
    // 관계 2+0x20: 자식 y 를 새 마스터 y 에 이전 간격을 유지해 옮긴다(preservedY 의 단일 float32 저장).
    let [py, poy, pcy] = [121.05547332763672, 129.78761291503906, -205.00482177734375];
    assert.notEqual(f(py - (poy - pcy)), f(py - f(poy - pcy)));
    got = relationScene(0x22, { y: poy, x: 0 }, { x: 9, y: pcy, scale: 1 }, { x: 0, y: py });
    assert.deepEqual(got.captured, [[2, 9, f(py - (poy - pcy)), 0, 1]]);
});

test('HudDividerGump 재부착 순서: popup 정리는 마스터 저장소 확인보다 먼저이므로 파생 저장소가 없어도 정리 호출은 남는다', () => {
    const scene = plainScene('reflow'); const dock = new HudDock(scene.children, scene.globals, () => 55, { ...scene.hooks(), divider: () => null });
    assert.throws(() => dock.attach(1, 1, 2, 0, 0), /파생 저장소 불일치/); assert.equal(scene.trace, 'closePopups');
});
