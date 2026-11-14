// TextEntryGump/ChatWindow 원본 관측 시험이 함께 쓰는 지원 틀이다: cpppj `TextEntryGumpTests`/`ChatWindowTestSupport.h` 의 `EntryScene`(노드 48필드·Slider·편집 버퍼·할당 블록·명시 장치)을 옮겼다. (2026-11-15 WB04-E483, E485 에서 공용 모듈로 분리)
import assert from 'node:assert/strict';
import { TextEntryGump, ConsoleGumpNode, TextEntryGumpState } from '../www/ui/text-entry-gump.js';
import { ScrollGumpNode } from '../www/ui/scroll-gump.js';
import { TextEntryBuffer, TextEntryBufferState } from '../www/ui/text-entry-buffer.js';

// 원문 격리 주소 토큰, 블록 주소/간격, 제외문자 외부 주소, 열 수이다.
export const ENTRY_NODE = 0x01009000; export const ENTRY_BUFFER = 0x01001000; export const ENTRY_VERTICAL = 0x0100a000; export const ENTRY_HORIZONTAL = 0x0100b000; export const ENTRY_VTABLE = 0x0100c000;
export const BLOCK_BASE = 0x01010000; export const BLOCK_STRIDE = 0x10000; export const EXCLUDED_ADDRESS = 0x01006000; export const ROW_COLUMNS = 23;
// 부호 있는 편집 버퍼 필드 이름과 순서(원문 필드 순서)이다.
const BUFFER_NAMES = ['vtable', 'offsets', 'offsetCapacity', 'lineCount', 'dirty', 'selectionBegin', 'selectionEnd', 'length', 'capacity', 'data'];
const BUFFER_SIGNED = new Set(['offsetCapacity', 'lineCount', 'selectionBegin', 'selectionEnd', 'length', 'capacity']);
// 노드 48필드 중 부호 있는 위치(shift 기준/끝·스크롤 줄/열·최대 길이)이다.
const SIGNED_NODE = new Set([28, 29, 30, 31, 38]);
// float32/float64 와 비트 사이를 오가는 공용 보기이다.
export const f32 = new Float32Array(1); const u32 = new Uint32Array(f32.buffer); const f64 = new Float64Array(1); const u64 = new Uint32Array(f64.buffer);
export const bitsOf = (value) => { f32[0] = value; return u32[0]; }; export const floatOf = (bits) => { u32[0] = bits >>> 0; return f32[0]; };
// 십진 입력의 하위 32비트(부호 있는 입력도 비트 유지)와 부호 있는 해석이다.
export const entryNumber = (text) => Number(BigInt.asUintN(32, BigInt(text))); export const signed = (value) => value | 0;
export const unhex = (text) => Uint8Array.from(Buffer.from(text, 'hex')); export const hexOf = (bytes) => Buffer.from(bytes).toString('hex');
// 명시 callback 이름을 trace 문자열로 만든다: 이름 뒤에 `:값` 을 붙인다.
export const packed = (values) => values.join(':');

// 한 입력 행의 현재 노드/Slider/버퍼/메모리와 명시 장치이다. 기대 관측으로 장치를 만들지 않는다.
export class EntryScene {
    constructor(row) {
        this.node = new ConsoleGumpNode(); this.vertical = new ScrollGumpNode(); this.horizontal = new ScrollGumpNode(); this.state = new TextEntryBufferState(); this.globals = new TextEntryGumpState();
        this.blocks = new Map(); this.trace = ''; this.clipboardOutput = new Uint8Array(0); this.sliderAllocated = 0; this.timeIndex = 0;
        this.setNodeFields(row[2].split(':'));
        const words = row[3].split(':').map(entryNumber); assert.equal(words.length, 10); this.setBufferFields(words);
        if (row[4] !== '-') for (const item of row[4].split(';')) { const parts = item.split(':'); this.blocks.set(entryNumber(parts[0]), { nominal: entryNumber(parts[1]), live: entryNumber(parts[2]), bytes: unhex(parts[3]) }); }
        this.nextBlock = this.blocks.size; this.fill = entryNumber(row[12]) & 0xff;
        const setText = (hex, terminate) => Uint8Array.from([...unhex(hex), ...(terminate ? [0] : [])]);
        this.text = setText(row[6], true); this.excluded = setText(row[7], true); this.clipboardInput = setText(row[8], false); this.times = row[9].split(':').map(Number);
        this.completion = row[10] === '~' ? null : Buffer.from(setText(row[10], false)).toString('latin1'); this.recipientFlag = row[11] === '1';
        // 두 Slider 의 독립 초기 상태이다.
        for (const [child, token] of [[this.vertical, ENTRY_VERTICAL], [this.horizontal, ENTRY_HORIZONTAL]]) {
            child.vtable = ENTRY_VTABLE; child.flags = 1; child.serial = token === ENTRY_VERTICAL ? 31 : 32; child.scale = 1;
            child.baseWidth = child.width = token === ENTRY_VERTICAL ? 10 : 100; child.baseHeight = child.height = token === ENTRY_VERTICAL ? 100 : 10; child.position = 3;
        }
        this.globals.brown = 9; this.globals.black = 0; this.globals.yellow = 5;
        this.buffer = new TextEntryBuffer(this.state, this.bufferHooks()); this.gump = new TextEntryGump(this.globals, this.hooks());
    }
    // 입력 48필드를 같은 노드에 원시 비트로 설치한다(부호 있는 위치는 signed 해석).
    setNodeFields(fields) {
        assert.equal(fields.length, 48); const value = fields.map(entryNumber); const node = this.node;
        node.vtable = value[0]; node.flags = value[1]; node.command = value[2]; node.serial = value[3]; node.scale = floatOf(value[4]); node.baseWidth = floatOf(value[5]); node.baseHeight = floatOf(value[6]); node.width = floatOf(value[7]); node.height = floatOf(value[8]); node.x = floatOf(value[9]); node.y = floatOf(value[10]);
        for (let i = 0; i < 8; i += 1) node.textContext[i] = value[11 + i];
        node.backgroundFlags = value[19]; node.verticalSlider = value[20]; node.horizontalSlider = value[21]; node.ownsBuffer = value[22]; node.reservedC4 = value[23];
        u64[0] = value[24]; u64[1] = value[25]; node.blinkAt = f64[0];
        node.blinkPhase = value[26]; node.readOnly = value[27]; node.shiftAnchor = signed(value[28]); node.shiftEnd = signed(value[29]); node.scrollLine = signed(value[30]); node.scrollColumn = signed(value[31]);
        node.showVertical = value[32]; node.showHorizontal = value[33]; node.forwardInput = value[34]; node.multiline = value[35]; node.changeNotice = value[36]; node.recipientHighlight = value[37]; node.maxLength = signed(value[38]);
        node.excludedChars = value[39]; node.buffer = value[40]; node.reserved10C = value[41]; node.autocomplete = value[42]; node.submit = value[43]; node.resetWord = value[44]; node.wordStart = value[45]; node.wordLength = value[46]; node.reserved124 = value[47];
        void SIGNED_NODE;
    }
    // 현재 노드 48필드를 원문 순서의 부호 없는 값으로 읽는다.
    nodeFields() {
        const node = this.node; f64[0] = node.blinkAt; const low = u64[0]; const high = u64[1];
        return [node.vtable, node.flags, node.command, node.serial, bitsOf(node.scale), bitsOf(node.baseWidth), bitsOf(node.baseHeight), bitsOf(node.width), bitsOf(node.height), bitsOf(node.x), bitsOf(node.y),
            ...Array.from(node.textContext), node.backgroundFlags, node.verticalSlider, node.horizontalSlider, node.ownsBuffer, node.reservedC4, low, high, node.blinkPhase, node.readOnly, node.shiftAnchor, node.shiftEnd, node.scrollLine, node.scrollColumn,
            node.showVertical, node.showHorizontal, node.forwardInput, node.multiline, node.changeNotice, node.recipientHighlight, node.maxLength, node.excludedChars, node.buffer, node.reserved10C, node.autocomplete, node.submit, node.resetWord, node.wordStart, node.wordLength, node.reserved124].map((value) => value >>> 0);
    }
    // Slider 의 11필드(vtable/flags/serial/scale/기본 크기/크기/위치/position)를 읽는다.
    childFields(node) { return [node.vtable, node.flags, node.serial, bitsOf(node.scale), bitsOf(node.baseWidth), bitsOf(node.baseHeight), bitsOf(node.width), bitsOf(node.height), bitsOf(node.x), bitsOf(node.y), node.position].map((value) => value >>> 0); }
    // 편집 버퍼 10DWORD 를 읽고 쓴다.
    bufferFields() { return BUFFER_NAMES.map((name) => this.state[name] >>> 0); }
    setBufferFields(values) { BUFFER_NAMES.forEach((name, index) => { this.state[name] = BUFFER_SIGNED.has(name) ? values[index] | 0 : values[index] >>> 0; }); }
    // 주소가 속한 블록의 그 위치부터 남은 보기이며 제외문자 외부 주소는 입력 문자열이다.
    memory(address) {
        if (address === EXCLUDED_ADDRESS) return this.excluded;
        let found = null;
        // 주소 이하 중 가장 큰 블록 시작 주소를 고른다.
        for (const key of this.blocks.keys()) if (key <= address && (found === null || key > found)) found = key;
        if (found === null) throw new Error('TextEntry 메모리 주소');
        const block = this.blocks.get(found); const offset = address - found; if (offset >= block.bytes.length) throw new Error('TextEntry 물리 범위');
        return block.bytes.subarray(offset);
    }
    base(token) { return token === ENTRY_NODE ? this.node : token === ENTRY_VERTICAL ? this.vertical : token === ENTRY_HORIZONTAL ? this.horizontal : null; }
    slider(token) { if (token === ENTRY_VERTICAL) return this.vertical; if (token === ENTRY_HORIZONTAL) return this.horizontal; throw new Error('TextEntry Slider 주소'); }
    // 명시 장치의 호출 인자만 순서대로 기록한다(값은 부호 없는 십진수 문자열).
    event(name, values = []) { this.trace += `${this.trace ? ';' : ''}${name}${values.map((value) => `:${value}`).join('')}`; }
    // nominal 과 +16 바이트 가드를 따로 유지한다.
    allocate(size) { const address = BLOCK_BASE + this.nextBlock * BLOCK_STRIDE; this.nextBlock += 1; this.blocks.set(address, { nominal: size, live: 1, bytes: new Uint8Array(size + 16).fill(this.fill) }); return address; }
    // 편집 버퍼 본체용 소유 메모리/clipboard 장치이다.
    bufferHooks() {
        return {
            bytes: (address) => this.memory(address),
            allocate: (size) => { const address = this.allocate(size); this.event('allocate', [size, address]); return address; },
            release: (address) => { if (address) { const block = this.blocks.get(address); if (!block) throw new Error('TextEntry 해제 주소'); block.live = 0; } this.event('release', [address]); },
            resize: (previous, size) => {
                const address = this.allocate(size);
                if (previous) { const old = this.blocks.get(previous); if (!old) throw new Error('TextEntry 재할당 주소'); this.blocks.get(address).bytes.set(old.bytes.subarray(0, Math.min(old.nominal, size))); old.live = 0; }
                this.event('resize', [previous, size, address]); return address;
            },
            clipboardRead: (target) => { target.fill(0); target.set(this.clipboardInput.subarray(0, Math.min(target.length, this.clipboardInput.length))); this.event('clipboardRead', [target.length]); },
            clipboardWrite: (source) => { this.clipboardOutput = Uint8Array.from(source); this.event('clipboardWrite', [source.length === 0 ? '-' : hexOf(source)]); },
        };
    }
    // 일반 트리/글꼴/Slider 는 독립 관측과 같은 명시 장치 계약이다.
    hooks() {
        const attach = (name) => (token, x, y, parent, mode) => { const target = this.base(token); target.x = x; target.y = y; this.event(name, [token, bitsOf(x), bitsOf(y), parent, mode]); };
        const cString = (bytes) => { const end = bytes.indexOf(0); if (end < 0) throw new Error('TextEntry 장치 NUL 없음'); return bytes.subarray(0, end); };
        return {
            node: (token) => (token === ENTRY_NODE ? this.node : null), console: (token) => (token === ENTRY_NODE ? this.node : null), base: (token) => this.base(token),
            buffer: (token) => (token === ENTRY_BUFFER ? this.buffer : null), slider: (token) => this.slider(token),
            allocate: (size) => { const token = size === 40 ? ENTRY_BUFFER : (this.sliderAllocated++ ? ENTRY_HORIZONTAL : ENTRY_VERTICAL); this.event('new', [size, token]); return token; },
            releaseBuffer: (token) => this.event('delete', [token]),
            constructBackground: (token, flags) => { const node = this.node; node.flags = 1; node.scale = 1; node.baseWidth = 0; node.baseHeight = 0; node.backgroundFlags = flags >>> 0; this.event('constructBackground', [token, flags >>> 0]); },
            constructSlider: (token, width, height, brown, black) => { const child = this.slider(token); child.vtable = ENTRY_VTABLE; child.flags = 1; child.scale = 1; child.baseWidth = child.width = width; child.baseHeight = child.height = height; child.position = 0; this.event('constructSlider', [token, width >>> 0, height >>> 0, brown >>> 0, black >>> 0]); },
            baseAttach: attach('baseAttach'), attach: attach('attach'),
            push: (token) => this.event('push', [token]), redraw: (token, value) => this.event('redraw', [token, value ? 1 : 0]), idle: (token, value) => this.event('idle', [token, value ? 1 : 0]),
            bubble: (token, event, first, second) => this.event('bubble', [token, event >>> 0, first >>> 0, second >>> 0]),
            sliderRange: (token, first, last) => this.event('range', [token, first >>> 0, last >>> 0]), sliderPage: (token, value) => this.event('page', [token, value >>> 0]),
            sliderPosition: (token, value) => { this.event('position', [token, value >>> 0]); this.slider(token).position = value; },
            now: () => { const value = this.times[Math.min(this.timeIndex, this.times.length - 1)]; this.timeIndex += 1; f64[0] = value; this.event('now', [((BigInt(u64[1]) << 32n) | BigInt(u64[0])).toString()]); return value; },
            measure: (token, value, length = -1) => {
                const visible = length < 0 ? cString(value) : value.subarray(0, length); let width = 0;
                for (const byte of visible) width += 4 + (byte % 5);
                this.event('measure', [visible.length === 0 ? '-' : hexOf(visible), length >>> 0, width, '14']); return [width, 14];
            },
            position: (token) => { this.event('screen', [token]); return { x: 17, y: 23 }; },
            drawBackground: (token) => this.event('background', [token]),
            drawText: (token, value, point, context) => { const visible = cString(value); this.event('text', [visible.length === 0 ? '-' : hexOf(visible), point.x >>> 0, point.y >>> 0, context ? '1' : '0', context ? context[2] >>> 0 : this.node.textContext[2]]); },
            fill: (rect, color) => this.event('fill', [rect.left >>> 0, rect.top >>> 0, rect.right >>> 0, rect.bottom >>> 0, color >>> 0]),
            recipient: (value) => { this.event('recipient', [value.length === 0 ? '-' : Buffer.from(value, 'latin1').toString('hex')]); return this.recipientFlag; },
            allocateBytes: (size) => { const token = this.allocate(size); this.event('new', [size, token]); return token; },
            releaseBytes: (token) => { if (this.blocks.has(token)) this.blocks.get(token).live = 0; this.event('delete', [token]); }, bytes: (token) => this.memory(token),
            complete: (autocomplete, word, length, shift) => { this.event('complete', [word === null || word === undefined ? '~' : Buffer.from(word, 'latin1').toString('hex'), length >>> 0, shift ? '1' : '0']); return this.completion; },
            submit: (token, value) => this.event('submit', [value.length === 0 ? '-' : Buffer.from(value, 'latin1').toString('hex')]),
            assertion: (line) => this.event('assert', [line]), destructBase: (token) => this.event('destructBase', [token]),
        };
    }
    // 반납 메모리와 NUL 바깥 가드까지 전부 주소 순서로 관측한다.
    blocksOutput() {
        const parts = [...this.blocks.keys()].sort((a, b) => a - b).map((address) => { const block = this.blocks.get(address); return `${address}:${block.nominal}:${block.live}:${hexOf(block.bytes)}`; });
        return parts.length === 0 ? '-' : parts.join(';');
    }
}
