// 문자 편집 Gump(TextEntry/Console)의 원본 10.78 독립 관측 2,292행(1,146입력 × 두 정밀도) 대조: 13개 진입(construct/consoleConstruct/attach/scroll/relative/handle/console/draw/idle/setText/append/excluded/horizontal/destroy)의
// 48필드 노드·두 Slider 11필드·편집 버퍼 10DWORD·할당 블록 전체·clipboard 출력·외부 호출 순서·반환·스택 정리량을 cpppj `TextEntryGumpTests` 와 같은 입력 구성으로 비교한다.
// 앞 13열(입력)만 장치에 공급하고 뒤 10열(관측)은 비교에만 쓴다. 호출 장치는 원문 격리 주소 토큰(노드 0x01009000·버퍼 0x01001000·Slider 0x0100a000/0x0100b000)과 `size + 16` 바이트 블록 할당기를 쓴다.
// 원본 게임을 실행하지 않는다. (2026-11-15 WB04-E483)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { TextEntryGump, ConsoleGumpNode, TextEntryGumpState } from '../www/ui/text-entry-gump.js';
import { ScrollGumpNode } from '../www/ui/scroll-gump.js';
import { TextEntryBuffer, TextEntryBufferState } from '../www/ui/text-entry-buffer.js';

// 원문 격리 주소 토큰, 블록 주소/간격, 제외문자 외부 주소, 열 수이다.
const ENTRY_NODE = 0x01009000; const ENTRY_BUFFER = 0x01001000; const ENTRY_VERTICAL = 0x0100a000; const ENTRY_HORIZONTAL = 0x0100b000; const ENTRY_VTABLE = 0x0100c000;
const BLOCK_BASE = 0x01010000; const BLOCK_STRIDE = 0x10000; const EXCLUDED_ADDRESS = 0x01006000; const ROW_COLUMNS = 23;
// 부호 있는 편집 버퍼 필드 이름과 순서(원문 필드 순서)이다.
const BUFFER_NAMES = ['vtable', 'offsets', 'offsetCapacity', 'lineCount', 'dirty', 'selectionBegin', 'selectionEnd', 'length', 'capacity', 'data'];
const BUFFER_SIGNED = new Set(['offsetCapacity', 'lineCount', 'selectionBegin', 'selectionEnd', 'length', 'capacity']);
// 노드 48필드 중 부호 있는 위치(shift 기준/끝·스크롤 줄/열·최대 길이)이다.
const SIGNED_NODE = new Set([28, 29, 30, 31, 38]);
// float32/float64 와 비트 사이를 오가는 공용 보기이다.
const f32 = new Float32Array(1); const u32 = new Uint32Array(f32.buffer); const f64 = new Float64Array(1); const u64 = new Uint32Array(f64.buffer);
const bitsOf = (value) => { f32[0] = value; return u32[0]; }; const floatOf = (bits) => { u32[0] = bits >>> 0; return f32[0]; };
// 십진 입력의 하위 32비트(부호 있는 입력도 비트 유지)와 부호 있는 해석이다.
const entryNumber = (text) => Number(BigInt.asUintN(32, BigInt(text))); const signed = (value) => value | 0;
const unhex = (text) => Uint8Array.from(Buffer.from(text, 'hex')); const hexOf = (bytes) => Buffer.from(bytes).toString('hex');
// 명시 callback 이름을 trace 문자열로 만든다: 이름 뒤에 `:값` 을 붙인다.
const packed = (values) => values.join(':');

// 한 입력 행의 현재 노드/Slider/버퍼/메모리와 명시 장치이다. 기대 관측으로 장치를 만들지 않는다.
class EntryScene {
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

test('TextEntryGump/Console 원본 10.78 2,292행: 13개 진입의 노드 48필드·Slider·편집 버퍼·할당 전체·clipboard·호출 순서·반환·스택 정리량이 일치한다', () => {
    const table = fs.readFileSync(new URL('../../cpppj/tests/fixtures/text-entry-gump-x86.tsv', import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && !line.startsWith('name\t')).map((line) => line.split('\t'));
    assert.equal(table.length, 2292);
    const failures = [];
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, ROW_COLUMNS, `행 ${index}`);
        const scene = new EntryScene(row); const args = row[5] === '-' ? [] : row[5].split(':').map(entryNumber); const operation = row[1]; let result = 0;
        // 본문(NUL 제외)을 바이트 문자열로 만든 초기 입력과 NUL 포함 입력이다.
        const textString = Buffer.from(scene.text.subarray(0, scene.text.length - 1)).toString('latin1');
        if (operation === 'construct' || operation === 'consoleConstruct') {
            const initial = args[2] ? textString : null;
            if (operation === 'construct') scene.gump.construct(ENTRY_NODE, signed(args[0]), signed(args[1]), initial, args[3], args[4], args[5]);
            else scene.gump.constructConsole(ENTRY_NODE, signed(args[0]), signed(args[1]), initial, args[3], args[4], args[5]);
        } else if (operation === 'attach') scene.gump.attach(ENTRY_NODE, floatOf(args[0]), floatOf(args[1]), args[2], args[3]);
        else if (operation === 'scroll') scene.gump.setScroll(ENTRY_NODE, signed(args[0]), signed(args[1]));
        else if (operation === 'relative') scene.gump.relativeScroll(ENTRY_NODE, signed(args[0]), signed(args[1]));
        else if (operation === 'handle') result = scene.gump.handle(ENTRY_NODE, args[0], args[1], args[2]) >>> 0;
        else if (operation === 'console') result = scene.gump.handleConsole(ENTRY_NODE, args[0], args[1], args[2]) >>> 0;
        else if (operation === 'draw') scene.gump.draw(ENTRY_NODE);
        else if (operation === 'idle') scene.gump.idle(ENTRY_NODE);
        else if (operation === 'setText') scene.gump.setText(ENTRY_NODE, args[0] ? scene.text : null);
        else if (operation === 'append') scene.gump.appendText(ENTRY_NODE, args[0] ? scene.text : null, args[1] !== 0);
        else if (operation === 'excluded') scene.gump.setExcluded(ENTRY_NODE, args[0] ? textString : null);
        else if (operation === 'horizontal') scene.gump.setHorizontal(ENTRY_NODE, args[0]);
        else if (operation === 'destroy') scene.gump.destruct(ENTRY_NODE);
        else throw new Error(`TextEntry 관측 진입 이름 ${operation}`);
        const problems = [];
        if (packed(scene.nodeFields()) !== row[13]) problems.push('node');
        if (packed(scene.childFields(scene.vertical)) !== row[14]) problems.push('vertical');
        if (packed(scene.childFields(scene.horizontal)) !== row[15]) problems.push('horizontal');
        if (packed(scene.bufferFields()) !== row[16]) problems.push('buffer');
        if (scene.blocksOutput() !== row[17]) problems.push('blocks');
        if ((scene.clipboardOutput.length === 0 ? '-' : hexOf(scene.clipboardOutput)) !== row[18]) problems.push('clipboard');
        if ((scene.trace === '' ? '-' : scene.trace) !== row[19]) problems.push('trace');
        if (result !== entryNumber(row[20])) problems.push('result');
        if (entryNumber(row[21]) !== 4 + args.length * 4) problems.push('stack');
        if (!(entryNumber(row[22]) > 0)) problems.push('instructions');
        if (problems.length) failures.push(`${index}/${row[0]}/${operation}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
});

// 원본 관측 fixture 에는 Tab/ESC/위/아래 키가 없어 기준선 소스(TextEntryGump.cpp Handle)의 계약으로 독립 확인한다: 이 네 키는 기본 입력기가 처리하지 않아 반환 0, 변경 통지(changeEvent) 없음, 편집 버퍼/노드 변경 없음이다.
// 입력 전달(forwardInput) 통지는 처리 여부와 무관하게 공통 Redraw 뒤에 나가므로 이 계약에 넣지 않는다. 같은 계약이 기본 분기(코드 < 0x20 또는 >= 0x100)에도 적용되어 네 `case` 문은 동등 변형 대상이다. (2026-11-15 WB04-E483)
test('TextEntryGump handle: Tab/ESC/위/아래 키는 처리하지 않고 반환 0·통지 없음·본문 불변이다(독립 확인)', () => {
    const table = fs.readFileSync(new URL('../../cpppj/tests/fixtures/text-entry-gump-x86.tsv', import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && !line.startsWith('name\t')).map((line) => line.split('\t'));
    // 본문이 있고(length>0) 읽기 전용이 아닌 handle 행의 입력 열을 틀로 쓴다(관측 열은 쓰지 않는다).
    const template = table.find((row) => row[1] === 'handle' && Number(row[3].split(':')[7]) > 0 && Number(row[2].split(':')[27]) === 0);
    assert.ok(template);
    for (const code of [9, 27, 0x260000, 0x280000]) {
        const row = [...template]; row[5] = `${code}:0:0`;
        const scene = new EntryScene(row); const before = scene.bufferFields().join(':'); const nodeBefore = scene.nodeFields().join(':');
        assert.equal(scene.gump.handle(ENTRY_NODE, code, 0, 0) >>> 0, 0, `코드 ${code}`);
        assert.equal(scene.bufferFields().join(':'), before, `버퍼 ${code}`); assert.equal(scene.nodeFields().join(':'), nodeBefore, `노드 ${code}`);
        assert.ok(!scene.trace.includes(`bubble:${ENTRY_NODE}:${scene.globals.changeEvent}:0:0`), `변경 통지 ${code}`);
    }
});

// 초기/추가/교체 본문이 null 이면 빈 문자열과 같은 효과다(원문 null 문자열 포인터). fixture 의 null 입력 행이 드물어 독립 확인한다. (2026-11-15 WB04-E483)
test('TextEntryGump 구성/교체/추가: null 본문은 빈 본문과 같은 효과이다(독립 확인)', () => {
    const table = fs.readFileSync(new URL('../../cpppj/tests/fixtures/text-entry-gump-x86.tsv', import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && !line.startsWith('name\t')).map((line) => line.split('\t'));
    const template = table.find((row) => row[1] === 'setText'); assert.ok(template);
    const run = (arg) => { const row = [...template]; row[5] = '0'; const scene = new EntryScene(row); scene.gump.setText(ENTRY_NODE, arg); return [scene.bufferFields().join(':'), scene.blocksOutput(), scene.nodeFields().join(':')]; };
    assert.deepEqual(run(null), run(Uint8Array.of(0)));
    assert.deepEqual(run(undefined), run(Uint8Array.of(0)));
});
