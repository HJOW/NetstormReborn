// 문자 편집 버퍼(TextEntryBuffer)의 원본 10.78 독립 관측 570행(285입력 × 두 정밀도) 대조: 13개 진입(construct/destroy/select/insert/reset/lines/lineCount/lineLength/copyLine/copyAll/copy/cut/paste)의
// 현재 10DWORD·할당 블록 전체(가드/반납 메모리 포함)·출력 2048바이트·선택 쌍·clipboard 출력·외부 호출 순서·반환·부분 중단을 cpppj `TextEntryBufferTests` 와 같은 입력 구성으로 비교한다.
// 앞 11열(입력)만 장치에 공급하고 뒤 11열(관측)은 비교에만 쓴다. 명시 할당 장치는 가상 주소 0x01010000 + n×0x10000 의 `size + 16` 바이트 블록을 주며, 호출 번호별 할당 실패/상태 변경/명시 중단을 입력대로 적용한다.
// 원본 게임을 실행하지 않는다. (2026-11-15 WB04-E481)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { TextEntryBuffer, TextEntryBufferState } from '../www/ui/text-entry-buffer.js';

// 명시 장치의 가상 블록 시작 주소/간격과 열 수, 상태 필드 이름 순서(원문 필드 순서)이다.
const BLOCK_BASE = 0x01010000; const BLOCK_STRIDE = 0x10000; const ROW_COLUMNS = 22;
const FIELD_NAMES = ['vtable', 'offsets', 'offsetCapacity', 'lineCount', 'dirty', 'selectionBegin', 'selectionEnd', 'length', 'capacity', 'data'];
// 부호 있는 필드(offsetCapacity·lineCount·selectionBegin/End·length·capacity)의 위치이다.
const SIGNED_FIELDS = new Set(['offsetCapacity', 'lineCount', 'selectionBegin', 'selectionEnd', 'length', 'capacity']);
// 명시 callback 중단을 원문 정상 반환과 구별하는 예외이다.
class EditExplicitStop extends Error {}
// 16진 문자열을 바이트 열로 바꾼다(빈 문자열은 빈 열).
const unhex = (text) => Uint8Array.from(Buffer.from(text, 'hex'));
// 바이트 열의 소문자 16진이다.
const hexOf = (bytes) => Buffer.from(bytes).toString('hex');
// 십진 입력의 하위 32비트이다(부호 있는 입력도 비트를 유지).
const editNumber = (text) => Number(BigInt.asUintN(32, BigInt(text)));

// 한 입력 행의 현재 상태·메모리·장치·관측 기록이다. 기대 관측으로 장치를 만들지 않는다.
class EditScene {
    constructor(row) {
        this.state = new TextEntryBufferState(); this.blocks = new Map(); this.calls = new Map(); this.trace = ''; this.clipboardOutput = new Uint8Array(0);
        this.text = Uint8Array.from([...unhex(row[5]), 0]); this.excluded = Uint8Array.from([...unhex(row[6]), 0]); this.clipboardInput = unhex(row[7]);
        this.fill = editNumber(row[10]) & 0xff; this.output = new Uint8Array(2048).fill(0x6d); this.selection = Int32Array.of(0x11223344, 0x55667788);
        const fields = row[2].split(':'); assert.equal(fields.length, 10);
        this.setFields(fields.map(editNumber));
        if (row[3] !== '-') for (const item of row[3].split(';')) { const parts = item.split(':'); assert.equal(parts.length, 4); this.blocks.set(editNumber(parts[0]), { nominal: editNumber(parts[1]), live: editNumber(parts[2]), bytes: unhex(parts[3]) }); }
        this.nextBlock = this.blocks.size;
        this.failures = new Set(row[8] === '-' ? [] : row[8].split(','));
        this.effects = row[9] === '-' ? [] : row[9].split(';').map((effect) => effect.split('/'));
        this.buffer = new TextEntryBuffer(this.state, this.hooks());
    }
    // 현재 10DWORD 를 원문 필드 순서의 부호 없는 값으로 읽는다.
    fields() { return FIELD_NAMES.map((name) => this.state[name] >>> 0); }
    // 원시 10 필드를 같은 상태에 쓴다(부호 있는 필드는 signed 32 로 해석).
    setFields(values) { FIELD_NAMES.forEach((name, index) => { this.state[name] = SIGNED_FIELDS.has(name) ? values[index] | 0 : values[index] >>> 0; }); }
    // 주소가 속한 블록의 그 위치부터 남은 살아 있는 보기이다. 블록 밖/끝 이후는 오류이다.
    memory(address) {
        let found = null;
        // 주소 이하 중 가장 큰 블록 시작 주소를 고른다(upper_bound 의 직전).
        for (const key of this.blocks.keys()) if (key <= address && (found === null || key > found)) found = key;
        if (found === null) throw new Error('편집버퍼 장치 주소');
        const block = this.blocks.get(found); const offset = address - found;
        if (offset >= block.bytes.length) throw new Error('편집버퍼 장치 물리범위');
        return block.bytes.subarray(offset);
    }
    // 앞 11열의 상태 변경만 적용한다(control 은 중단 판정에서 따로 본다).
    applyEffect(effect) {
        if (effect[1] === 'control') return;
        if (effect[1] !== 'state') throw new Error('편집버퍼 효과 대상');
        const at = FIELD_NAMES.indexOf(effect[2]); if (at < 0) throw new Error('편집버퍼 효과 필드');
        const values = this.fields(); values[at] = editNumber(effect[3]); this.setFields(values);
    }
    // 장치 기본 효과 뒤 같은 호출 번호의 변경과 명시 중단을 실행한다. 실제 장치 인자를 trace 에 기록한다.
    event(name, values) {
        this.trace += `${this.trace ? ';' : ''}${[name, ...values].join(':')}`;
        const count = (this.calls.get(name) ?? 0) + 1; this.calls.set(name, count);
        const trigger = `${name}${count}`; let stopped = false;
        // 같은 경계의 여러 변경을 원문 입력 순서대로 적용한다.
        for (const effect of this.effects) { if (effect[0] !== trigger) continue; this.applyEffect(effect); if (effect[1] === 'control' && effect[2] === 'stop' && effect[3] === '1') stopped = true; }
        if (stopped) throw new EditExplicitStop();
    }
    // 새 블록(size + 16 바이트, fill 바이트)을 주거나 입력이 명시한 실패면 0 이다.
    allocate(size, failed) {
        if (failed) return 0;
        if (size > 16384) throw new Error('편집버퍼 명시 할당 한도');
        const token = BLOCK_BASE + this.nextBlock * BLOCK_STRIDE; this.nextBlock += 1;
        this.blocks.set(token, { nominal: size, live: 1, bytes: new Uint8Array(size + 16).fill(this.fill) }); return token;
    }
    // 실제 소유 메모리/clipboard 효과를 가진 훅 묶음이다.
    hooks() {
        return {
            bytes: (address) => this.memory(address),
            allocate: (size) => { const token = this.allocate(size, this.failures.has(`allocate${(this.calls.get('allocate') ?? 0) + 1}`)); this.event('allocate', [size, token]); return token; },
            release: (token) => { if (token) { const block = this.blocks.get(token); if (!block) throw new Error('편집버퍼 해제 주소'); block.live = 0; } this.event('release', [token]); },
            resize: (old, size) => {
                const token = this.allocate(size, this.failures.has(`resize${(this.calls.get('resize') ?? 0) + 1}`));
                if (token && old) { const previous = this.blocks.get(old); if (!previous) throw new Error('편집버퍼 재할당 주소'); const current = this.blocks.get(token); current.bytes.set(previous.bytes.subarray(0, Math.min(previous.nominal, size))); previous.live = 0; }
                this.event('resize', [old, size, token]); return token;
            },
            clipboardRead: (target) => { target.fill(0); target.set(this.clipboardInput.subarray(0, Math.min(this.clipboardInput.length, target.length))); this.event('clipboardRead', [target.length]); },
            clipboardWrite: (source) => { this.clipboardOutput = Uint8Array.from(source); this.event('clipboardWrite', [this.clipboardOutput.length === 0 ? '-' : hexOf(this.clipboardOutput)]); },
        };
    }
    // 반납 메모리와 NUL 바깥 가드까지 전부 주소 순서로 관측한다.
    blocksOutput() {
        const parts = [...this.blocks.keys()].sort((a, b) => a - b).map((address) => { const block = this.blocks.get(address); return `${address}:${block.nominal}:${block.live}:${hexOf(block.bytes)}`; });
        return parts.length === 0 ? '-' : parts.join(';');
    }
}

test('TextEntryBuffer 원본 10.78 570행: 13개 진입의 현재 10DWORD·할당 전체·출력·선택·clipboard·호출 순서·반환·부분 중단이 일치한다', () => {
    const table = fs.readFileSync(new URL('../../cpppj/tests/fixtures/text-entry-buffer-x86.tsv', import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && !line.startsWith('name\t')).map((line) => line.split('\t'));
    assert.equal(table.length, 570);
    const failures = []; let stopped = 0;
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, ROW_COLUMNS, `행 ${index}`);
        const scene = new EditScene(row); const args = row[4] === '-' ? [] : row[4].split(':').map(editNumber);
        const signed = (value) => value | 0; let result = 0; let completed = true; let meaningful = false; let parameters = args.length;
        try {
            const operation = row[1];
            if (operation === 'construct') { scene.buffer.construct(args[0]); result = 1; meaningful = true; }
            else if (operation === 'destroy') scene.buffer.destroy();
            else if (operation === 'select') scene.buffer.select(signed(args[0]), signed(args[1]));
            else if (operation === 'insert') scene.buffer.insert(args[0] ? scene.text : null);
            else if (operation === 'reset') scene.buffer.reset(args[0] ? scene.text : null);
            else if (operation === 'lines') { result = scene.buffer.ensureLines() ? 1 : 0; meaningful = true; }
            else if (operation === 'lineCount') { result = scene.buffer.lineCount() >>> 0; meaningful = true; }
            else if (operation === 'lineLength') { result = scene.buffer.lineLength(signed(args[0])) >>> 0; meaningful = true; }
            else if (operation === 'copyLine') { result = scene.buffer.copyLine(signed(args[0]), scene.output.subarray(0, args[1]), scene.selection) >>> 0; meaningful = true; parameters = 4; }
            else if (operation === 'copyAll') { result = scene.buffer.copyAll(scene.output.subarray(0, args[0])) >>> 0; meaningful = true; parameters = 2; }
            else if (operation === 'copy') scene.buffer.copy();
            else if (operation === 'cut') scene.buffer.cut();
            else if (operation === 'paste') scene.buffer.paste(args[0] ? scene.excluded : null);
            else throw new Error('편집버퍼 원문 진입 이름');
        } catch (error) { if (!(error instanceof EditExplicitStop)) throw error; completed = false; stopped += 1; }
        const problems = [];
        if (completed !== (row[11] === '1')) problems.push('completed');
        if (scene.fields().join(':') !== row[12]) problems.push('state');
        if (scene.blocksOutput() !== row[13]) problems.push('blocks');
        if (hexOf(scene.output) !== row[14]) problems.push('output');
        if (`${scene.selection[0] >>> 0}:${scene.selection[1] >>> 0}` !== row[15]) problems.push('selection');
        if ((scene.clipboardOutput.length === 0 ? '-' : hexOf(scene.clipboardOutput)) !== row[16]) problems.push('clipboard');
        if ((scene.trace === '' ? '-' : scene.trace) !== row[17]) problems.push('trace');
        if (completed && meaningful && result !== editNumber(row[18])) problems.push('result');
        // 원문 진입의 native 인자 수(스택 정리량 4 + 인자×4)와 명령 수 관측도 입력 계약과 맞아야 한다.
        if (completed && editNumber(row[19]) !== 4 + parameters * 4) problems.push('stack');
        if (!(editNumber(row[20]) > 0) || row[21] !== '0') problems.push('instructions');
        if (problems.length) failures.push(`${index}/${row[0]}/${row[1]}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
    assert.equal(stopped, 10);
});

// 원본 관측 fixture 는 construct 의 명시 중단을 두 번째 할당에서만 담으므로(mutation0283), 첫 번째 할당 호출에서 중단했을 때의 부분 상태를 기준선 소스(TextEntryBuffer.cpp Construct)의 순서로 독립 확인한다:
// 가상표·선택(0,0)·capacity 0 은 할당 호출 전에 쓰이고, data/length/offsets/lineCount/offsetCapacity/dirty 는 도달 전이라 이전 값을 유지한다. (2026-11-15 WB04-E481)
test('TextEntryBuffer construct 첫 할당 중단: 할당 전에 쓴 가상표·선택·capacity 만 남고 나머지는 이전 값이다(독립 확인)', () => {
    // 입력 11열만 가진 합성 행이다: 이름/연산/상태10필드/블록/인자/본문/제외/clipboard/실패/변경/fill.
    const row = ['independent0001', 'construct', '0:4369:5:3:0:7:9:11:77:8738', '-', '0', '', '', '', '-', 'allocate1/control/stop/1', '165'];
    const scene = new EditScene(row);
    assert.throws(() => scene.buffer.construct(0), EditExplicitStop);
    // vtable=0x508da8, offsets/offsetCapacity/lineCount/dirty/length/data 는 이전 값, 선택 0, capacity 0.
    assert.equal(scene.fields().join(':'), '5279144:4369:5:3:0:0:0:11:0:8738');
    assert.equal(scene.trace, 'allocate:2048:16842752'); assert.equal(scene.blocks.size, 1);
});
