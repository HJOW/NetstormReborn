// NSInfo(준비 창) 부모 생성(HudNsInfoCreate, 원본 0047ca50/0047c4b0/00479d40/0047afa0/004256d0)의 원본 10.78 독립 관측 170행 대조: 숨김 생성·NSInfo·PlayerInfo·BattleInfo·특수 버튼 생성의 전체 할당 블록(가드 바이트 포함)·
// 번역/서버 문자열 512바이트·13개 전역·호출 순서(번역/할당/형식화/부착/메타맵)와 부분 중단 50행을 cpppj `HudNsInfoCreateTests`(Native170ParentsChildrenAndPartialEffects)와 같은 입력 구성으로 비교한다.
// 앞 10열(입력)만 장치에 공급하고 뒤 8열(관측)은 비교에만 쓴다. 한 원문 할당의 raw 바이트와 형별 파생 저장소(기본·NSInfo·PlayerInfo·BattleInfo·버튼)는 경계마다 서로 내보내고 읽어 같은 물리처럼 다룬다.
// 원본 게임을 실행하지 않는다. (2026-11-15 WB04-E493)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { GumpCreateNode } from '../www/ui/gump-create.js';
import { HudNsInfoNode } from '../www/ui/hud-ns-info.js';
import { HudBattleInfoNode, HudNsInfoChild, HudNsInfoCreate, HudPlayerButtonNode, HudPlayerInfoNode } from '../www/ui/hud-ns-info-create.js';

// 명시 독립 장치의 가상 주소(노드/문자열/표/서버/형식화)와 원문 현재 전역 13개의 주소 순서이다.
const NODES = 0x01100000; const STRINGS = 0x01200000; const TABLE = 0x01000000; const SERVER = 0x00594970; const FORMAT = 0x01f00000;
const GLOBALS = [0x540421, 0x540440, 0x540457, 0x5ca920, 0x5ca924, 0x5467b8, 0x5411c8, 0x5bec3c, 0x54db4c, 0x540bac, 0x594fc8, 0x595344, 0x593150];
const ROWS = 170; const COLUMNS = 18;
// 명시 장치 중단을 정상 생성 복귀와 구별하는 예외이다.
class CreateStop extends Error {}
// 십진 입력의 하위 32비트와 float32 비트 보기, 16진 변환이다.
const number = (text) => Number(BigInt.asUintN(32, BigInt(text)));
const f32 = new Float32Array(1); const u32 = new Uint32Array(f32.buffer); const bitsOf = (value) => { f32[0] = value; return u32[0]; };
const unhex = (text) => Uint8Array.from(Buffer.from(text === '-' ? '' : text, 'hex')); const hexOf = (bytes) => Buffer.from(bytes).toString('hex'); const textHex = (value) => Buffer.from(value, 'latin1').toString('hex');
// 비정렬 DWORD 를 읽고 쓴다(범위 밖은 오류).
function load(bytes, offset) { if (offset + 4 > bytes.length) throw new RangeError('NSInfo 장치 읽기'); return (bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24)) >>> 0; }
function store(bytes, offset, value) { if (offset + 4 > bytes.length) throw new RangeError('NSInfo 장치 쓰기'); for (let i = 0; i < 4; i += 1) bytes[offset + i] = (value >>> (8 * i)) & 0xff; }

// 한 원문 할당의 raw 바이트와 형별 파생 저장소를 같이 소유한다(원문 new 요청 폭 포함).
class CreateRecord {
    constructor(size, fill) { this.size = size; this.kind = 'base'; this.raw = new Uint8Array(0x180).fill(fill & 0xff); this.base = new GumpCreateNode(); this.ns = new HudNsInfoNode(); this.player = new HudPlayerInfoNode(); this.battle = new HudBattleInfoNode(); this.button = new HudPlayerButtonNode(); this.textFlags = 0; this.buttonFlags = 0; }
    // 현재 종류의 기본 노드이다.
    node() { return this.kind === 'ns' ? this.ns : this.kind === 'player' ? this.player : this.kind === 'battle' ? this.battle : this.kind === 'button' ? this.button : this.base; }
    // 독립 raw 의 현재 필드만 실제 형별 저장소에 반영한다.
    import() {
        const r = this.raw; const b = this.node(); b.vtable = load(r, 0); b.flags = load(r, 4); b.command = load(r, 0x20); this.textFlags = load(r, 0xd0); this.buttonFlags = load(r, 0xac);
        if (this.kind === 'ns') { const n = this.ns; n.background = load(r, 0xa0); for (let i = 0; i < 8; i += 1) n.players[i] = load(r, 0xa4 + i * 4); n.roster = load(r, 0xc4); n.title = load(r, 0xc8); n.upperDivider = load(r, 0xcc); n.lowerDivider = load(r, 0xd0); n.battle = load(r, 0xd4); }
        if (this.kind === 'player') { const p = this.player; p.text = load(r, 0xa0); p.master = load(r, 0xa4); p.button = load(r, 0xa8); p.start = load(r, 0xac); p.player = load(r, 0xb0); p.buttonType = load(r, 0xb4); p.reserved = load(r, 0xb8); }
        if (this.kind === 'battle') { const t = this.battle; t.title = load(r, 0xa0); t.summary = load(r, 0xa4); t.body = load(r, 0xa8); t.battle = load(r, 0xac); t.changed = load(r, 0xb0); for (let i = 0; i < 22; i += 1) { t.settings[i][0] = load(r, 0xb4 + i * 8); t.settings[i][1] = load(r, 0xb8 + i * 8); } t.bytes.set(r.subarray(0x164, 0x168)); t.mode = load(r, 0x168); }
        if (this.kind === 'button') { const u = this.button; u.buttonFlags = load(r, 0xac); u.untouchedC0 = load(r, 0xc0); u.entry = load(r, 0xc4); }
    }
    // 제품이 쓴 필드만 raw 에 반영하고 다른 바이트는 보존한다.
    export() {
        const r = this.raw; const b = this.node(); store(r, 0, b.vtable); store(r, 4, b.flags); store(r, 0x20, b.command);
        if (this.kind === 'base') { store(r, 0xd0, this.textFlags); store(r, 0xac, this.buttonFlags); }
        if (this.kind === 'ns') { const n = this.ns; store(r, 0xa0, n.background); for (let i = 0; i < 8; i += 1) store(r, 0xa4 + i * 4, n.players[i]); store(r, 0xc4, n.roster); store(r, 0xc8, n.title); store(r, 0xcc, n.upperDivider); store(r, 0xd0, n.lowerDivider); store(r, 0xd4, n.battle); }
        if (this.kind === 'player') { const p = this.player; store(r, 0xa0, p.text); store(r, 0xa4, p.master); store(r, 0xa8, p.button); store(r, 0xac, p.start); store(r, 0xb0, p.player); store(r, 0xb4, p.buttonType); store(r, 0xb8, p.reserved); }
        if (this.kind === 'battle') { const t = this.battle; store(r, 0xa0, t.title); store(r, 0xa4, t.summary); store(r, 0xa8, t.body); store(r, 0xac, t.battle); store(r, 0xb0, t.changed); for (let i = 0; i < 22; i += 1) { store(r, 0xb4 + i * 8, t.settings[i][0]); store(r, 0xb8 + i * 8, t.settings[i][1]); } r.set(t.bytes, 0x164); store(r, 0x168, t.mode); }
        if (this.kind === 'button') { const u = this.button; store(r, 0xac, u.buttonFlags); store(r, 0xc0, u.untouchedC0); store(r, 0xc4, u.entry); }
    }
}

// 앞 10열의 자료만으로 하위 장치와 현재 자료를 공급하는 장면이다.
class CreateScene {
    constructor(row) {
        this.records = new Map(); this.strings = new Map(); this.trace = ''; this.calls = new Map(); this.allocationCalls = 0; this.formatted = '';
        this.fill = number(row[3]); this.typeResult = number(row[4]);
        const values = row[2].split(':'); assert.equal(values.length, GLOBALS.length); this.globals = values.map(number);
        this.formatInput = Buffer.from(unhex(row[5])).toString('latin1'); this.serverInput = Buffer.from(unhex(row[6])).toString('latin1'); this.server = new Uint8Array(0x100).fill(this.fill & 0xff);
        this.effects = row[7] === '-' ? [] : row[7].split(';').map((item) => { const v = item.split('/'); assert.equal(v.length, 3); return v; });
        this.stops = new Set(row[8] === '-' ? [] : row[8].split(';')); this.failures = new Set(row[9] === '-' ? [] : row[9].split(':').map(number));
        if (row[1] !== 'hidden') this.add(row[1] === 'ns' ? 0xd8 : row[1] === 'player' ? 0xbc : row[1] === 'battle' ? 0x16c : 0xc8);
    }
    global(address) { const index = GLOBALS.indexOf(address); if (index < 0) throw new RangeError('NSInfo 전역 주소'); return index; }
    exportAll() { for (const record of this.records.values()) record.export(); }
    importAll() { for (const record of this.records.values()) record.import(); }
    // 새 원문 할당 블록을 초기 물리 패턴으로 추가한다.
    add(size) { const token = NODES + this.records.size * 0x200; const record = new CreateRecord(size, this.fill); this.records.set(token, record); record.import(); return token; }
    // 원문 고정 주소를 선택한 같은 파생 종류로 조회한다.
    kind(token, kind) { const record = this.records.get(token); if (!record) throw new RangeError('NSInfo 기록 없음'); if (record.kind !== kind) { record.export(); record.kind = kind; record.import(); } return record; }
    // 현재 번역 버퍼/원문 문자열/형식화 결과를 NUL 앞만 해석한다.
    string(token) {
        if (token === 0x50c894) return '~w~E%s\nPlayers in Zone %d\n'; if (token === 0x50cd3c) return '~r~EBATTLEMASTER'; if (token === 0x50cd2c) return 'Start Battle'; if (token === 0x50d068) return 'LAN Server'; if (token === FORMAT) return this.formatted;
        const bytes = token === SERVER ? this.server : this.strings.get(token); if (!bytes) throw new RangeError('NSInfo 문자열 없음'); const end = bytes.indexOf(0); if (end < 0) throw new Error('NSInfo 문자열 NUL');
        return Buffer.from(bytes.subarray(0, end)).toString('latin1');
    }
    // 명시 주소의 같은 물리 메모리만 변경한다.
    effect(address, value) {
        if (address >= NODES && address < NODES + 0x10000) { const token = NODES + Math.floor((address - NODES) / 0x200) * 0x200; store(this.records.get(token).raw, address - token, value); return; }
        if (address >= STRINGS && address < STRINGS + 0x10000) { const token = STRINGS + Math.floor((address - STRINGS) / 0x200) * 0x200; store(this.strings.get(token), address - token, value); return; }
        const index = this.global(address); this.globals[index] = index < 3 ? value & 255 : value;
    }
    // 기록 뒤 입력 순서대로 변경하고 중단한다.
    event(name, values = []) {
        this.exportAll(); this.trace += `${this.trace ? ';' : ''}${name}${values.map((value) => `:${value}`).join('')}`;
        const count = (this.calls.get(name) ?? 0) + 1; this.calls.set(name, count); const trigger = `${name}${count}`;
        for (const e of this.effects) if (e[0] === trigger) this.effect(number(e[1]), number(e[2]));
        this.importAll(); if (this.stops.has(trigger)) throw new CreateStop();
    }
    // 하위 장치의 기본 필드 초기화를 명시한다.
    device(token, flags = 1) { this.exportAll(); const raw = this.records.get(token).raw; store(raw, 0, TABLE); store(raw, 4, flags); store(raw, 0x20, 0); this.importAll(); }
    // 실제 번역 토큰과 전체 512바이트 장치를 만든다.
    translate(source) {
        const key = this.string(source); const value = key === '~w~E%s\nPlayers in Zone %d\n' ? this.formatInput : `T:${key}`;
        const token = STRINGS + this.strings.size * 0x200; const bytes = new Uint8Array(0x200).fill(this.fill & 0xff); for (let i = 0; i < value.length; i += 1) bytes[i] = value.charCodeAt(i) & 0xff; bytes[value.length] = 0; this.strings.set(token, bytes);
        this.event('translate', [textHex(key), token]); return token;
    }
    // 제품 본문에 같은 공통/텍스트/자산 장치만 연결한다.
    hooks(getBody) {
        const e = (name, values) => this.event(name, values); const box = (record, field, button) => ({ get value() { return button && record.kind === 'button' ? record.button.buttonFlags : record[field]; }, set value(v) { if (button && record.kind === 'button') record.button.buttonFlags = v >>> 0; else record[field] = v >>> 0; } });
        return {
            node: (token) => this.kind(token, 'ns').ns, player: (token) => this.kind(token, 'player').player, battle: (token) => this.kind(token, 'battle').battle, button: (token) => this.kind(token, 'button').button,
            base: (token) => { const record = this.records.get(token); if (!record) throw new RangeError('NSInfo 기본 노드 없음'); return record.node(); },
            textFlags: (token) => box(this.records.get(token), 'textFlags', false), buttonFlags: (token) => box(this.records.get(token), 'buttonFlags', true),
            read: (address) => this.globals[this.global(address)],
            allocate: (size) => { this.allocationCalls += 1; const token = this.failures.has(this.allocationCalls) ? 0 : this.add(size); e('alloc', [size, token]); return token; },
            baseConstruct: (token, command, width, height, flags) => { this.device(token, flags); e('base', [token, command, bitsOf(width), bitsOf(height), flags]); },
            background: (token, width, height, bg, flags, type, frame) => { this.device(token); e('background', [token, bitsOf(width), bitsOf(height), bg, flags, type, frame]); return token; },
            scroll: (token, a, b, c, d) => { this.device(token); this.records.get(token).textFlags = 0x100; e('scroll', [token, a >>> 0, b >>> 0, c, d]); return token; },
            text: (token, a, b, c, d) => { this.device(token); e('text', [token, a >>> 0, b >>> 0, c, d]); return token; },
            buttonBase: (token, a, b, flags) => { this.device(token); this.records.get(token).button.buttonFlags = 0x100; e('buttonBase', [token, a >>> 0, b >>> 0, flags]); },
            stone: (token, a, b, label, color, context) => { this.device(token); this.records.get(token).buttonFlags = 0x100; e('stone', [token, a >>> 0, b >>> 0, label, color, context]); return token; },
            scrollable: (token, value) => { const r = this.records.get(token); r.textFlags = ((r.textFlags & ~4) | ((value & 1) * 4)) >>> 0; e('scrollable', [token, value]); },
            slider: (token, value) => { const r = this.records.get(token); r.textFlags = ((r.textFlags & ~2) | ((value & 1) * 2)) >>> 0; e('slider', [token, value]); },
            wrap: (token, value) => e('wrap', [token, value]), parseScroll: (token, text) => e('parseScroll', [token, textHex(this.string(text))]), parseText: (token, text) => e('parseText', [token, textHex(this.string(text))]),
            fitText: (token, a, b) => e('fitText', [token, a, b]), fitStone: (token) => e('fitStone', [token]),
            attach: (token, x, y, parent, mode) => {
                const kind = this.records.get(token).kind; const body = getBody();
                if (kind === 'player') body.attachPlayer(token, x, y, parent, mode); else if (kind === 'battle') body.attachBattle(token, x, y, parent, mode);
                else e(kind === 'button' ? 'baseAttach' : 'attach', [token, bitsOf(x), bitsOf(y), parent, mode]);
            },
            baseAttach: (token, x, y, parent, mode) => e('baseAttach', [token, bitsOf(x), bitsOf(y), parent, mode]),
            translate: (source) => this.translate(source), type: (source) => { e('type', [source, this.typeResult]); return this.typeResult; },
            serverName: () => {
                if (this.globals[this.global(0x54db4c)] === 0xffffffff) { const text = this.string(this.translate(0x50d068)); for (let i = 0; i < text.length; i += 1) this.server[i] = text.charCodeAt(i); this.server[text.length] = 0; }
                else { for (let i = 0; i < this.serverInput.length; i += 1) this.server[i] = this.serverInput.charCodeAt(i); this.server[this.serverInput.length] = 0; e('remote', [this.globals[this.global(0x54db4c)], 0, 0, SERVER, 80, 0]); }
                return SERVER;
            },
            formatZone: (format, name, zone) => {
                const fmt = this.string(format); const serverName = this.string(name); let used = 0;
                // snprintf 의 %s/%d 만 해석한다(관측 형식은 이 둘뿐이다).
                const text = fmt.replace(/%([sd%])/g, (match, code) => { if (code === '%') return '%'; used += 1; return code === 's' ? serverName : String(zone | 0); });
                if (text.length >= 512) throw new Error('NSInfo 명시 형식화 폭'); void used; this.formatted = text; e('format', [textHex(fmt), textHex(serverName), zone >>> 0]); return FORMAT;
            },
            assertion: (line) => e('assert', [line]), metamapLayout: (kind, token) => e('meta', [kind === HudNsInfoChild.Player ? 'player' : 'battle', token]),
        };
    }
    // 전체 현재 노드/가드 바이트를 원문 할당 순서로 관측한다.
    nodes() { this.exportAll(); const parts = [...this.records.entries()].map(([token, r]) => `${token}:${r.size}:${hexOf(r.raw)}`); return parts.length === 0 ? '-' : parts.join(';'); }
    // 번역/서버 전체 바이트와 형식화 내용을 모두 관측한다.
    stringsText() { return `${[...this.strings.entries()].map(([token, bytes]) => `${token}:${hexOf(bytes)}`).join(';')}|${hexOf(this.server)}|${textHex(this.formatted)}`; }
    // 현재 전역만 원문 순서로 관측한다.
    globalsText() { return this.globals.map((value) => value >>> 0).join(':'); }
}

test('HudNsInfoCreate 원본 10.78 170행: 다섯 생성의 할당 블록 전체·문자열·전역·호출 순서와 부분 중단 50행이 일치한다', () => {
    const table = fs.readFileSync(new URL('../../cpppj/tests/fixtures/hud-nsinfo-create-x86.tsv', import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && !line.startsWith('#')).map((line) => line.split('\t'));
    assert.equal(table.length, ROWS);
    const failures = []; let partial = 0; let full = 0;
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, COLUMNS, `행 ${index}`);
        const scene = new CreateScene(row); const currentIndex = scene.global(0x593150); let body = null;
        body = new HudNsInfoCreate({ get value() { return scene.globals[currentIndex]; }, set value(v) { scene.globals[currentIndex] = v >>> 0; } }, scene.hooks(() => body));
        let completed = true; let result = 0;
        try {
            if (row[1] === 'hidden') result = body.createHidden(); else if (row[1] === 'ns') result = body.construct(NODES); else if (row[1] === 'player') result = body.constructPlayer(NODES);
            else if (row[1] === 'battle') result = body.constructBattle(NODES); else result = body.constructButton(NODES, 16, 15);
        } catch (error) { if (!(error instanceof CreateStop)) throw error; completed = false; }
        if (!completed) partial += 1; const problems = [];
        if (completed && (row[1] === 'hidden' || row[1] === 'ns') && result) { full += 1; if (scene.records.size !== 50) problems.push('records50'); }
        if (completed !== (row[10] === '1')) problems.push('completed');
        if (completed && (result >>> 0) !== number(row[11])) problems.push('result');
        if (scene.globalsText() !== row[12]) problems.push('globals');
        if (scene.nodes() !== row[13]) problems.push('nodes');
        if (scene.stringsText() !== row[14]) problems.push('strings');
        if ((scene.trace === '' ? '-' : scene.trace) !== row[15]) problems.push('trace');
        if (!(number(row[17]) > 0)) problems.push('instructions');
        if (completed && Number(row[16]) !== (row[1] === 'button' ? 12 : 4)) problems.push('stack');
        if (problems.length) failures.push(`${index}/${row[0]}/${row[1]}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
    assert.equal(partial, 50); assert.ok(full > 0);
});
