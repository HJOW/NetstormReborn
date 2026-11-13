// 원본 버튼 fixture의 전체0x170BYTE·공통 표·명시 응답을 같은 실제 처리기에 공급한다. 기대 동작은 계산하지 않는다. (2026-11-14 WB04-E416)
import assert from 'node:assert/strict';
import { decodeGumpNode, encodeGumpNode } from './gump-node-support.mjs';
import { put } from './raw-support.mjs';
import { GumpCreateState, GumpCreate } from '../www/ui/gump-create.js';
import { GumpLifetimeState, GumpLifetime } from '../www/ui/gump-lifetime.js';
import { GumpEvents, GumpEventState } from '../www/ui/gump-events.js';
import { GumpInput, GumpInputState } from '../www/ui/gump-input.js';
import { GumpFrame } from '../www/ui/gump-frame.js';
import { GumpCollection } from '../www/render/gump-collection.js';
import { ButtonGumpNode, ButtonGumpState, ButtonGump, ButtonGumpKind, bindButtonGump } from '../www/ui/button-gump.js';

// 원본 격리 장면의 여섯 주소다. 호스트 메모리 주소로 실행하지 않는다.
export const BUTTON_TOKENS = Array.from({ length: 6 }, (_, i) => 0x19000010 + i * 0x190);
// RLE는 동일 DWORD 연속 구간만 줄인다.
function tableText(table) { const result = []; for (let at = 0; at < table.length;) { let end = at + 1; while (end < table.length && table[end] === table[at]) end += 1; result.push(`${end - at}:${table[at]}`); at = end; } return result.join(';'); }
// 명시한 모든 표 칸을 입력 순서대로 읽는다.
function readTable(table, text) { let at = 0; for (const item of text.split(';')) { const [count, value] = item.split(':').map(Number); assert.ok(at + count <= table.length); table.fill(value, at, at + count); at += count; } assert.equal(at, table.length); }
// QWORD 숫자 입력을 IEEE double 보기로만 해석한다.
function doubleFromBits(bits) { const bytes = new ArrayBuffer(8); const view = new DataView(bytes); view.setBigUint64(0, BigInt(bits), true); return view.getFloat64(0, true); }

// 지원하지 않는 라벨/문맥 꼬리와 미변경 double 비트를 그대로 보존하는 안정된 노드다.
class ButtonRecord {
    // 기본/버튼 저장소·원문368BYTE·별칭 배경DWORD와 두 입력double비트다.
    node = new ButtonGumpNode(); original = new Uint8Array(0x170); backgroundFlags = 0; background = false; doubleBits = [];
    // raw 응답은 같은 객체 주소에 쓰며 호스트 가상 분류를 보존한다.
    read(raw) { assert.equal(raw.length, 0x170); this.original = Uint8Array.from(raw); const view = new DataView(this.original.buffer); Object.assign(this.node, decodeGumpNode(Buffer.from(raw.subarray(0, 160)).toString('hex'))); Object.assign(this.node, { value: view.getUint32(160, true), pressed: view.getUint32(164, true), blink: view.getUint32(168, true), buttonFlags: view.getUint32(172, true), firstDeadline: view.getFloat64(176, true), repeatDeadline: view.getFloat64(184, true), background: view.getUint32(256, true) }); this.backgroundFlags = view.getUint32(180, true); this.doubleBits = [view.getBigUint64(176, true), view.getBigUint64(184, true)]; }
    // 현재 필드만 원래 offset에 작성한다. 배경 +b4는 double과 의미가 겹치는 별도 파생 필드다.
    raw() { const raw = Uint8Array.from(this.original); raw.set(Buffer.from(encodeGumpNode(this.node), 'hex')); const view = new DataView(raw.buffer); put(raw, 160, this.node.value); put(raw, 164, this.node.pressed); put(raw, 168, this.node.blink); put(raw, 172, this.node.buttonFlags); put(raw, 256, this.node.background);
        // 변화 없는 NaN payload까지 원본 double 비트를 보존한다.
        for (const [i, field] of ['firstDeadline', 'repeatDeadline'].entries()) { const value = this.node[field]; if (Object.is(value, doubleFromBits(this.doubleBits[i]))) view.setBigUint64(176 + i * 8, this.doubleBits[i], true); else view.setFloat64(176 + i * 8, value, true); }
        if (this.background) put(raw, 180, this.backgroundFlags); return raw;
    }
}

// 실제 생성/이벤트/입력/프레임/수명에 공유할 현재 저장소와 원문 관측 장치다.
export class ButtonScene {
    // 원문 일곱 스칼라와 다섯 전체 표를 소유하는 실제 상태다.
    create = new GumpCreateState(); life = new GumpLifetimeState(); event = new GumpEventState(); inputState = new GumpInputState(); state = Object.assign(new ButtonGumpState(), { valueEvent: 0x12345, clickEvent: 0x12346, rightEvent: 0x12347, repeatEvent: 0x12348, baseColor: 56 });
    // 안정된 노드·명시 가상 응답/투영/벽시계와 폴링·부모 반환 입력이다.
    records = new Map(); responses = new Map(); anchors = new Map(); times = []; timeIndex = 0; poll = { code: 0x10000, x: 999, y: 888 }; parentReturns = [1, 1, 1, 1, 1, 1]; baseReturn = 0x81234567;
    // 콜백 당시 전체 상태 기록과 생성 뒤 연결할 실제 분배기/처리기다.
    calls = []; dispatch = null; eventHooks = {}; inputHooks = {}; frameHooks = {}; factory; collection; lifetime; events; input; button; frame;
    // 입력/응답의 전역index는 원문과 같다.
    scalar(index, value) { const targets = [[this.create, 'count'], [this.create, 'serial'], [this.life, 'focus'], [this.life, 'killCount'], [this.life, 'dialogCount'], [this.life, 'currentDialog'], [this.create.collection, 'overrideHead']]; assert.ok(targets[index]); if (value !== undefined) targets[index][0][targets[index][1]] = value; return targets[index][0][targets[index][1]]; }
    // 표 사본을 만들지 않고 실제 배열을 반환한다.
    tables() { return [this.create.entries, this.life.idle, this.life.overlay, this.life.kill, this.life.dialogs]; }
    // 현재 전역/다섯 표와 모든 raw 노드를 원문 순서로 직렬화한다.
    snapshot() { return Array.from({ length: 7 }, (_, i) => this.scalar(i)).join(',') + '#' + this.tables().map(tableText).join('#'); }
    nodesSnapshot() { return [...this.records].sort(([a], [b]) => a - b).map(([token, record]) => `${token}=${Buffer.from(record.raw()).toString('hex')}`).join(';'); }
    // 콜백 당시 전체 상태를 먼저 기록한 뒤 입력이 지정한 쓰기만 같은 저장소에 적용한다.
    respond(kind, token, args = []) { const key = `${kind},${token}`; this.calls.push(`${[kind, token, ...args].join(',')}@${this.snapshot()}@${this.nodesSnapshot()}`);
        // 응답 대상0은 스칼라,1은 실제 idle 표,나머지는 지정 노드DWORD다.
        for (const [target, at, value] of this.responses.get(key) ?? []) { if (!target) this.scalar(at, value); else if (target === 1) this.life.idle[at] = value; else { const record = this.records.get(target); const raw = record.raw(); put(raw, at, value); record.read(raw); } }
        this.create.collection.root = this.create.entries[0];
    }
    // 이벤트 전파는 현재 부모의 명시 반환 DWORD만 공급한다.
    parent(token, event, first, second) { this.respond('Parent', token, [event, first, second]); const index = BUTTON_TOKENS.indexOf(token); assert.ok(index >= 0); return this.parentReturns[index]; }
    // 입력/통지의 서로 다른 fallback과 순환 분배를 초기화한다.
    prepare() { const node = (token) => this.records.get(token)?.node; this.eventHooks = { node, handle: (...args) => this.parent(...args) }; this.inputHooks = { node, handle: this.eventHooks.handle }; this.frameHooks = { node, idle: (token) => this.respond('OtherIdle', token), broadcast: (token, ...args) => this.respond('OtherBroadcast', token, args), redraw: (token, flag) => this.respond('Redraw', token, [Number(flag)]) }; bindButtonGump(this.eventHooks, this.inputHooks, this.frameHooks, (token) => token === BUTTON_TOKENS[1] ? this.dispatch : null); }
    // 가상/시간/폴링/배경 별칭은 기대 동작을 계산하지 않는 관측 장치다.
    hooks() { return { node: (token) => this.records.get(token)?.node, baseNode: (token) => this.records.get(token)?.node, backgroundFlags: (token) => { const record = this.records.get(token); if (!record) return null; return { get value() { return record.backgroundFlags; }, set value(value) { record.backgroundFlags = value; } }; }, now: () => { assert.ok(this.timeIndex < this.times.length); const value = this.times[this.timeIndex]; this.respond('Now', this.timeIndex++, [value]); return doubleFromBits(value); }, poll: (code) => { this.respond('Poll', 0, [code]); return this.poll; }, redraw: (token, value) => this.respond('Redraw', token, [Number(value)]), sound: () => this.respond('Sound', 0), baseEvent: (token, ...args) => { this.respond('Base', token, args); return this.baseReturn; } }; }
    // 모든 실제 공통 모듈이 같은 노드·표를 빌린 뒤 현재 버튼 포인터를 연결한다.
    rig() { this.prepare(); const lookup = this.eventHooks.node; this.factory = new GumpCreate(this.create, { node: lookup }); this.lifetime = new GumpLifetime(this.create, this.life, { node: lookup, push: (token) => this.respond('Push', token) }); this.collection = new GumpCollection(this.create.collection, this.factory.collectionHooks((sid) => { this.respond('Project', sid); return this.anchors.get(sid); })); this.events = new GumpEvents(this.create, this.life, this.event, this.collection, this.eventHooks); this.input = new GumpInput(this.create, this.life, this.inputState, this.collection, this.events, this.inputHooks); this.button = new ButtonGump(this.create, this.state, this.factory, this.collection, this.events, this.input, this.lifetime, this.hooks()); this.frame = new GumpFrame(this.create, this.life, this.frameHooks); this.dispatch = this.button; return this; }
    // 완성 입력만 읽고 결과·콜백 기대값은 해석하지 않는다.
    load(row) {
        // 각 raw 노드는 자체 원문 꼬리를 보존한다.
        for (const item of row[3].split(';')) { const [token, text] = item.split('='); const record = new ButtonRecord(); record.read(Uint8Array.from(Buffer.from(text, 'hex'))); record.background = Number(token) >= BUTTON_TOKENS[4]; this.records.set(Number(token), record); }
        const parts = row[4].split('#'); parts[0].split(',').map(Number).forEach((value, i) => this.scalar(i, value)); this.tables().forEach((table, i) => readTable(table, parts[i + 1])); this.create.collection.root = this.create.entries[0]; const extras = row[5].split('#'); extras[0].split(',').map(Number).forEach((kind, i) => { this.records.get(BUTTON_TOKENS[i]).node.kind = kind ? ButtonGumpKind.STONE : ButtonGumpKind.BASE; }); this.times = extras[1].split(',').map(BigInt); const [code, x, y] = extras[2].split(',').map(Number); this.poll = { code, x: x | 0, y: y | 0 }; this.parentReturns = extras[3].split(',').map(Number); this.baseReturn = Number(extras[4]); this.state.baseColor = Number(extras[5]) << 24 >> 24;
        // 명시 투영점/응답만 같은 현재 장치에 공급한다.
        for (const item of extras[6].split(';')) { const [sid, x, y] = item.split(',').map(Number); this.anchors.set(sid, { x, y }); }
        if (row[6] !== '-') for (const item of row[6].split(';')) { const [key, text] = item.split(':'); this.responses.set(key, text.split('/').map((v) => v.split(',').map(Number))); }
    }
    // 직접 연결 검사용 여섯 실제 노드/트리이며 결과 상태를 미리 작성하지 않는다.
    basic() { this.create.count = 6; this.create.serial = 91;
        // 원문 CPP Basic의 같은 작은 독립 입력이다.
        for (const [i, token] of BUTTON_TOKENS.entries()) { const record = new ButtonRecord(); record.read(new Uint8Array(0x170)); record.background = i >= 4; Object.assign(record.node, { vtable: 0x19008000 + i * 0x100, flags: 1, id: i, serial: 75 + i, parent: i ? BUTTON_TOKENS[0] : 0, baseWidth: 75, width: 75, baseHeight: 19, height: 19, scale: 1 }); this.records.set(token, record); this.create.entries[i] = token; this.anchors.set(i, { x: 17, y: -19 }); }
        this.create.collection.root = BUTTON_TOKENS[0]; this.records.get(BUTTON_TOKENS[0]).node.child = BUTTON_TOKENS[1]; const n = this.records.get(BUTTON_TOKENS[1]).node; Object.assign(n, { flags: 0xa01, background: BUTTON_TOKENS[5], kind: ButtonGumpKind.STONE, child: BUTTON_TOKENS[5] }); this.records.get(BUTTON_TOKENS[5]).node.parent = BUTTON_TOKENS[1]; this.times = new Array(32).fill(0x3ff0000000000000n); return this.rig();
    }
}
