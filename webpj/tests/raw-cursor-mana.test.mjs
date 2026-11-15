// 커서 마나 표시 raw-cursor-mana.js(RawCursorMana 생성·프레임·통지·소멸과 CursorManaProcess)를 원본 10.78 관측 642행·마나/볼트 표시 432행씩과 cpppj 시험 계약으로 대조한다(WB04-E549).
//  - cursor-mana-x86.tsv 642행: 생성·프레임·통지·소멸·연속 호출의 효과 순서·224바이트 필드·전역·raw 슬롯 31개·단계별 필드 전부.
//  - cursor-mana-display-x86.tsv·mana-bolt-display-x86.tsv 각 432행: 실제 SquidDisplay 의 WORD 조건과 10픽셀 확대 변경 표(독립 입력의 부분 갱신).
//  - 계약: 미연결 경계 거부·판본/TYPE 표 크기 거부·theme 범위·목록 범위·실제 Host/Kernel 프로세스 수명.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E549)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, bitsToFloat, floatToBits, put, get } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { Kernel, BaseProcess } from '../www/core/kernel.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { SquidDisplay } from '../www/core/squid-display.js';
import { DirtyRegions } from '../www/render/dirty-regions.js';
import { CursorDropState } from '../www/core/raw-cursor-drop.js';
import {
    RawCursorMana, CursorManaGlobals, CursorManaProcess, CURSOR_MANA_FIELD_BYTES, createCursorManaFields, cursorManaThemeFrame,
    createCursorManaProcess, bindCursorManaLifecycle, bindCursorManaWorld
} from '../www/core/raw-cursor-mana.js';

// 원본 관측 fixture 이다. 없으면 해당 검사를 건너뛴다. (2026-11-15 WB04-E549)
const fixture = (name) => readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', name));
const manaRows = fixture('cursor-mana-x86.tsv');
const manaDisplayRows = fixture('cursor-mana-display-x86.tsv');
const boltDisplayRows = fixture('mana-bolt-display-x86.tsv');
// 원본 DWORD 목록만 읽으며 기대 출력을 호스트 수식으로 생성하지 않는다. (2026-11-15 WB04-E549)
const numbers = (value) => value.split(',').map((field) => Number(field) >>> 0);
// 바이트 배열을 소문자 16진 문자열로 만든다(원문 Hex). (2026-11-15 WB04-E549)
const hex = (bytes) => Buffer.from(bytes).toString('hex');
// 입력 두 DWORD 를 원본 double 시각으로 읽는다. (2026-11-15 WB04-E549)
function inputTime(input, index) {
    const view = new DataView(new ArrayBuffer(8));
    view.setUint32(0, input[index], true);
    view.setUint32(4, input[index + 1], true);
    return view.getFloat64(0, true);
}

// 외부 가상 경계만 독립 입력으로 공급하고 생성/이동/이벤트/삭제의 분기는 실제 모듈 몸체가 계산하는 장면이다(cpppj ManaScene). (2026-11-15 WB04-E549)
class ManaScene {
    constructor() {
        this.pool = new SidPool(24000, { server: false });
        this.types = Array.from({ length: 188 }, () => ({ theme: 0, footX: 0, footY: 0 }));
        this.clock = new SquidProcessState();
        this.globals = new CursorManaGlobals();
        this.cursor = new CursorDropState();
        this.fields = createCursorManaFields();
        this.input = [];
        this.trace = '';
        this.steps = '';
        this.created = 0;
        this.returned = 0;
        this.mutated = false;
        // raw 입력 슬롯을 실제 풀에서 확보한다. 시험은 게임/장치를 열지 않는다.
        for (let value = 5; value <= 150; value += 1) assert.equal(this.pool.allocate(2), value);
    }

    // free/dead 입력도 준비하는 검사 전용 접근이다. 제품은 이 접근으로 할당 검사를 우회하지 않는다.
    raw(sid) { return this.pool.physical(sid); }

    // 원문의 가상 경계 사건 형식을 유지한다(signed/float 인자도 전체 DWORD 로 기록).
    event(kind, values = []) { this.trace = `${this.trace}${this.trace ? ';' : ''}${kind}${values.map((value) => `:${value >>> 0}`).join('')}`; }

    // 최초 선택된 외부 효과만 현재 개수/시간을 바꾸는 명시 입력이다.
    mutate(kind) {
        if (this.mutated) return;
        const mode = this.input[19];
        if (!((mode === 1 && kind === 'D') || (mode === 2 && kind === 'V') || (mode === 3 && kind === 'X') || (mode === 4 && kind === 'D') || (mode === 5 && kind === 'O') || (mode === 6 && kind === 'A'))) return;
        this.mutated = true;
        if (mode >= 1 && mode <= 5) put(this.fields.bytes, 0x84, 1);
        if (mode === 4) put(this.fields.bytes, 0xd8, 0);
        this.clock.now += mode === 1 ? 0.125 : mode === 2 ? 0.25 : mode === 5 ? 0.5 : mode === 6 ? 1.25 : 0;
    }

    // 실행 전 입력만 준비하며 이전 행의 결과는 공유하지 않는다.
    prepare(values) {
        assert.equal(values.length, 36);
        this.input = values;
        this.trace = '';
        this.steps = '';
        this.created = 0;
        this.returned = 0;
        this.mutated = false;
        this.fields.bytes.fill(0xa5);
        this.cursor = new CursorDropState();
        this.cursor.cursorGump = values[17];
        this.globals.mapLimit = values[18] | 0;
        this.globals.radiusLimit = bitsToFloat(values[34]);
        this.globals.invalidPoint = bitsToFloat(values[35]);
        this.clock.now = inputTime(values, 22);
        this.types[100].theme = values[1] | 0;
        this.types[100].footX = values[2] | 0;
        this.types[100].footY = values[3] | 0;
        // raw 슬롯의 무관한 바이트도 표식으로 채워 실제 쓰기 범위를 검사한다.
        for (let sid = 50; sid <= 150; sid += 1) {
            if (sid >= 80 && sid !== 150) continue;
            const bytes = this.raw(sid);
            bytes.fill(0xa5);
            put(bytes, 0, 1);
            bytes[10] = sid === 150 ? 100 : 103;
            bytes[11] = sid === 150 ? 0 : (values[13] | (((values[15] >>> (sid - 50)) & 1) ? 4 : 0) | (((values[16] >>> (sid - 50)) & 1) ? 2 : 0)) & 0xff;
            put(bytes, 6, sid === 150 ? 90 : 0, 2);
            put(bytes, 14, values[sid === 150 ? 28 : 30]);
            put(bytes, 18, values[sid === 150 ? 29 : 31]);
        }
        const fieldBytes = this.fields.bytes;
        put(fieldBytes, 0, 0x50c468);
        put(fieldBytes, 4, 90);
        put(fieldBytes, 8, 150);
        // SID0은 명시 입력이며 생성/판정의 기대값을 만들지 않는다.
        for (let index = 0; index < 30; index += 1) put(fieldBytes, 12 + 4 * index, ((values[14] >>> index) & 1) ? 0 : 50 + index);
        put(fieldBytes, 0x84, values[4]);
        put(fieldBytes, 0x88, values[5]);
        put(fieldBytes, 0x98, values[6]);
        put(fieldBytes, 0x90, values[7]);
        put(fieldBytes, 0x8c, values[8]);
        put(fieldBytes, 0x94, values[9]);
        put(fieldBytes, 0xd8, values[10]);
        // 사각 위상은 fixture 생성기의 명시 입력이며 출력 보간을 대체하지 않는다.
        for (let index = 0; index < 7; index += 1) put(fieldBytes, 0x9c + 4 * index, floatToBits(Math.fround(0.25 * (index + 1))));
        put(fieldBytes, 0xbc, floatToBits(Math.fround((this.types[100].footX - 1) | 0)));
        put(fieldBytes, 0xb8, floatToBits(Math.fround((this.types[100].footY - 1) | 0)));
        const view = new DataView(fieldBytes.buffer, fieldBytes.byteOffset, fieldBytes.byteLength);
        view.setFloat64(0xc0, inputTime(values, 24), true);
        view.setFloat64(0xc8, 3.25, true);
        view.setFloat64(0xd0, inputTime(values, 26), true);
    }

    // 실제 분기/산술 대신 외부 효과의 입력/출력과 현재 상태 변경만 제공한다.
    hooks() {
        return {
            create: (type, flags) => { const sid = 50 + this.created; this.created += 1; this.event('C', [type, flags, sid]); return sid; },
            setWord: (sid, value) => put(this.raw(sid), 12, value, 2),
            themeFrame: cursorManaThemeFrame,
            detach: (sid, flags) => { this.event('D', [sid, flags]); this.raw(sid)[11] |= 4; this.mutate('D'); },
            unpop: (sid, flags) => { this.event('U', [sid, flags]); this.raw(sid)[11] &= ~4; this.mutate('U'); },
            destroy: (sid, flags) => { this.event('X', [sid, flags]); this.raw(sid)[11] |= 2; this.mutate('X'); },
            position: (sid, x, y, flags) => {
                this.event('V', [sid, floatToBits(x), floatToBits(y), flags]);
                put(this.raw(sid), 14, floatToBits(x));
                put(this.raw(sid), 18, floatToBits(y));
                this.raw(sid)[11] &= ~4;
                this.mutate('V');
            },
            center: (sid) => { this.event('O', [sid]); this.mutate('O'); return [bitsToFloat(this.input[32]), bitsToFloat(this.input[33])]; }
        };
    }

    // 실제 Attach 의 Form/부모 쓰기와 외부 현재 시각 변경을 제공한다.
    attach() {
        return (actual, parent, form, flags) => {
            assert.equal(actual, this.fields);
            this.event('A', [50, parent, form, flags]);
            put(this.fields.bytes, 4, form || 90);
            put(this.fields.bytes, 8, parent);
            this.mutate('A');
        };
    }

    // 한 실제 함수 호출 뒤 224바이트 전부를 보존한다.
    snapshot() { this.steps = `${this.steps}${this.steps ? '|' : ''}${hex(this.fields.bytes)}`; }

    // 같은 실제 몸체에 저장 입력의 단일/연속 호출을 재생한다.
    run(logic) {
        const input = this.input;
        const frame = () => {
            logic.frame(this.fields, () => { this.event('K', [0]); logic.destroy(this.fields, 1); });
            this.snapshot();
        };
        if (input[0] === 0 || input[0] === 5) {
            logic.construct(this.fields, 150, 0, this.attach());
            this.snapshot();
            if (input[0] === 5) {
                // 생성 결과를 그대로 이어 실제 여러 프레임과 종료/소멸을 실행한다.
                for (let index = 0; index < input[21]; index += 1) { this.clock.now += 0.125; frame(); }
                logic.notify(this.fields, 3, 0, 0);
                this.snapshot();
                logic.destroy(this.fields, 1);
                this.snapshot();
            }
        } else if (input[0] === 1) frame();
        else if (input[0] === 2) { logic.notify(this.fields, input[11], input[12], 0x87654321); this.snapshot(); }
        else if (input[0] === 3) { logic.destroy(this.fields, 1); this.snapshot(); }
        else if (input[0] === 4) {
            this.event('N', [0xe0]);
            if (input[20]) { logic.construct(this.fields, 150, 0, this.attach()); this.returned = 1; }
            this.snapshot();
        }
    }

    // 전체 현재 raw 31슬롯을 원문과 같은 순서로 읽는다.
    rawSnapshot() {
        let result = '';
        for (let sid = 50; sid <= 150; sid += 1) if (sid < 80 || sid === 150) result += hex(this.pool.slot(sid));
        return result;
    }

    // 현재 커서 토큰·double 시각·new 반환 식별값이다.
    globalsText() {
        const view = new DataView(new ArrayBuffer(8));
        view.setFloat64(0, this.clock.now, true);
        return `${this.cursor.cursorGump >>> 0},${view.getUint32(0, true)},${view.getUint32(4, true)},${this.returned}`;
    }

    // 현재 장면의 실제 몸체이다.
    logic(hooks = this.hooks()) { return new RawCursorMana(this.pool, this.types, this.clock, this.globals, this.cursor, hooks); }
}

test('RawCursorMana: 원본 10.78 생성·프레임·통지·소멸 642행의 효과·필드·전역·raw 슬롯이 같다', { skip: !manaRows && '원본 fixture 없음' }, () => {
    const scene = new ManaScene();
    let count = 0;
    // 실제 PE 결과를 읽으며 기대값을 다시 계산하지 않는다.
    for (const row of manaRows.filter((fields) => fields[0] !== 'input36')) {
        assert.equal(row.length, 11);
        scene.prepare(numbers(row[0]));
        scene.run(scene.logic());
        const label = `행 ${count} 입력 ${row[0]}`;
        assert.equal(scene.trace || '-', row[1], `${label} 효과`);
        assert.equal(hex(scene.fields.bytes), row[2], `${label} 필드`);
        assert.equal(scene.globalsText(), row[3], `${label} 전역`);
        assert.equal(scene.rawSnapshot(), row[4], `${label} raw`);
        assert.equal(scene.steps, row[5], `${label} 단계별 필드`);
        // 기본 53비트와 64비트 원문이 모두 같았던 행이다.
        assert.deepEqual([row[1], row[2], row[3], row[4], row[5]], [row[6], row[7], row[8], row[9], row[10]]);
        count += 1;
    }
    assert.equal(count, 642);
});

// 전달한 마나 103 또는 볼트 105 fixture 의 432입력을 실제 bounds/clip/dirty 전체 변경표와 대조한다. (2026-11-15 WB04-E549)
function replayManaDisplay(rows) {
    const types = Array.from({ length: 188 }, () => ({ footX: 0, footY: 0, flags1: 0, flags2: 0, maxHitPoints: 0, defaultFrame: 0, name: '' }));
    const shapes = types.map(() => ({ frameCount: 0, loaded: false, frames: [] }));
    // 원문의 합성 TYPE/SHP 입력이다.
    shapes[74] = {
        frameCount: 2,
        loaded: true,
        frames: [[100, 120, -3, 5], [130, 140, 7, -9], [50, 70, 2, 3], [80, 90, 4, 6]].map(([width, height, hotspotX, hotspotY]) => ({ width, height, hotspotX, hotspotY, cellWidth: 1, cellHeight: 1 }))
    };
    types[74].flags2 = 0x10000;
    types[74].footX = 1;
    types[74].footY = 1;
    const dirty = new DirtyRegions(640, 480);
    const sink = { suppressedInput: false, suppressed: () => sink.suppressedInput || dirty.fullRedraw, invalidate: (rect, flags) => dirty.add(rect, flags) };
    const display = new SquidDisplay(types, shapes, sink, { cameraX: 7, cameraY: 9, zoom: 65536, viewport: { left: 12, top: 9, right: 620, bottom: 450 } });
    let count = 0;
    // 입력 raw 전체를 그대로 사용하고 출력 사각형을 시험에서 계산하지 않는다.
    for (const row of rows.filter((fields) => fields[0] !== 'input7')) {
        assert.equal(row.length, 4);
        const input = numbers(row[0]);
        assert.equal(input.length, 7);
        dirty.clear();
        sink.suppressedInput = input[5] !== 0;
        types[74].flags1 = input[2];
        display.setView({ cameraX: 7, cameraY: 9, zoom: input[3] | 0, viewport: { left: 12, top: 9, right: 620, bottom: 450 } });
        const bytes = new Uint8Array(50);
        // WORD 옆 바이트/플래그/좌표도 원문에 기록된 바이트를 그대로 읽는다.
        for (let index = 0; index < bytes.length; index += 1) bytes[index] = Number.parseInt(row[1].slice(index * 2, index * 2 + 2), 16);
        const before = bytes.slice();
        display.update(bytes, input[6] ? 0x2000 : 0);
        assert.deepEqual(bytes, before);
        // 실제 dirty 변경표의 순서와 각 flags 를 모두 보존한다.
        const output = dirty.entries.map((entry) => `${entry.rect.left},${entry.rect.top},${entry.rect.right},${entry.rect.bottom},${entry.flags}`).join(';') || '-';
        assert.equal(row[2], sink.suppressedInput ? '1' : '0');
        assert.equal(output, row[3], `행 ${count}`);
        count += 1;
    }
    assert.equal(count, 432);
}

test('SquidDisplay: 원본 10.78 커서 마나 473770/8c 표시 432행의 WORD 조건과 10픽셀 확대 변경 표가 같다', { skip: !manaDisplayRows && '원본 fixture 없음' }, () => replayManaDisplay(manaDisplayRows));

test('SquidDisplay: 원본 10.78 마나 볼트 45ac90/8c 표시 432행의 WORD 무관/10픽셀 확대 변경 표가 같다', { skip: !boltDisplayRows && '원본 fixture 없음' }, () => replayManaDisplay(boltDisplayRows));

// 미복원 경계를 생략하지 않으며 잘못된 판본/타입과 목록/테마 범위를 거부한다. (2026-11-15 WB04-E549)
test('RawCursorMana: 미연결 경계와 판본/TYPE/theme/목록 범위를 거부한다', { skip: !manaRows && '원본 fixture 없음' }, () => {
    const scene = new ManaScene();
    scene.prepare(numbers(manaRows.find((fields) => fields[0] !== 'input36')[0]));
    const logic = scene.logic();
    assert.equal(logic.pool, scene.pool);
    assert.equal(logic.processType, 50);
    assert.throws(() => cursorManaThemeFrame(-1), RangeError);
    assert.throws(() => cursorManaThemeFrame(11), RangeError);
    assert.deepEqual([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10].map(cursorManaThemeFrame), [4, 5, 6, 7, 4, 5, 6, 7, 8, 9, 10]);
    // 10.72 판본 풀과 187칸 TYPE 표는 연결하지 않는다.
    assert.throws(() => new RawCursorMana({ edition: 'cd1072', capacity: 24000 }, scene.types, scene.clock, scene.globals, scene.cursor, scene.hooks()), TypeError);
    assert.throws(() => new RawCursorMana(scene.pool, scene.types.slice(0, 187), scene.clock, scene.globals, scene.cursor, scene.hooks()), TypeError);
    // 효과가 없으면 도달한 첫 가상 경계에서 거부한다(빈 효과로 생략하지 않음).
    const missing = scene.logic({});
    assert.throws(() => missing.construct(scene.fields, 150, 0, scene.attach()), /미연결/);
    // 현재 개수 31 은 30칸 목록 밖이다.
    put(scene.fields.bytes, 8, 150);
    put(scene.fields.bytes, 0x98, 0);
    put(scene.fields.bytes, 0xd8, 1);
    put(scene.fields.bytes, 0x94, 0);
    put(scene.fields.bytes, 0x84, 31);
    assert.throws(() => logic.destroy(scene.fields, 1), RangeError);
});

// 실제 Host/Kernel 에 붙는 프로세스의 수명 장면이다: 부모 하나와 마나 객체를 만드는 가상 효과만 독립 입력이다. (2026-11-15 WB04-E549)
function processScene() {
    const pool = new SidPool(32768, { server: true });
    const types = Array.from({ length: 188 }, (_, number) => ({
        cost: 0, flags1: 0, flags2: 0, footX: 3, footY: 4, theme: 2, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {})
    }));
    const destroy = new RawSquidDestroy(pool, new SquidUnpop(pool, new SquidHash(), new Uint8Array(65536)), types);
    const kernel = new Kernel();
    const state = new SquidProcessState();
    state.now = 10;
    const host = new SquidProcessHost(pool, types, kernel, destroy, state);
    const parent = pool.allocate();
    const raw = pool.allocated(parent);
    raw[10] = 100;
    put(raw, 14, floatToBits(40));
    put(raw, 18, floatToBits(50));
    const events = [];
    const hooks = {
        create: (type, flags) => { const sid = pool.allocate(2); events.push(['C', type, flags]); return sid; },
        setWord: (sid, word) => put(pool.allocated(sid), 12, word, 2),
        themeFrame: cursorManaThemeFrame,
        detach: (sid) => { events.push(['D', sid]); pool.allocated(sid)[11] |= 4; },
        unpop: (sid) => { events.push(['U', sid]); pool.allocated(sid)[11] &= ~4; },
        destroy: (sid) => { events.push(['X', sid]); pool.allocated(sid)[11] |= 2; },
        position: (sid, x, y, flags) => { events.push(['V', sid, x, y, flags]); put(pool.allocated(sid), 14, floatToBits(x)); put(pool.allocated(sid), 18, floatToBits(y)); },
        center: () => [40, 50]
    };
    const cursor = new CursorDropState();
    const globals = new CursorManaGlobals();
    const logic = new RawCursorMana(pool, types, state, globals, cursor, hooks);
    return { pool, types, kernel, state, host, parent, events, hooks, cursor, globals, logic };
}

test('CursorManaProcess: 실제 Host/Kernel 부착·프레임·통지·소멸과 수명 연결', () => {
    const s = processScene();
    const lifecycle = {};
    bindCursorManaLifecycle(lifecycle, s.host, s.kernel, s.logic);
    const token = lifecycle.createMana(s.parent, 0);
    const process = s.kernel.get(token);
    assert.ok(process instanceof CursorManaProcess);
    assert.equal(process.token, token);
    // 생성자가 8개 표시 객체를 만들고(마나 타입 103·flags2) 부착 뒤 필드에 Form/부모 번호를 남긴다.
    assert.equal(s.events.filter((event) => event[0] === 'C').length, 8);
    assert.deepEqual(s.events.find((event) => event[0] === 'C'), ['C', 103, 2]);
    assert.equal(get(process.fields.bytes, 8), s.parent);
    assert.equal(get(process.fields.bytes, 4), process.form);
    assert.equal(get(process.fields.bytes, 0x84), 8);
    assert.equal(s.host.isRegistered(token, process), true);
    // 첫 프레임은 반경을 키우고 8개 객체를 부모 둘레에 놓는다(가상 해제 뒤 위치).
    s.state.now = 10.1;
    s.events.length = 0;
    process.runFrame();
    assert.equal(s.events.filter((event) => event[0] === 'V').length, 8);
    assert.ok(F32(process.fields.bytes, 0x90) > 0);
    // 이벤트 1 은 숨김 값 전체 DWORD 를 보존하고 void 가 아닌 객체를 숨긴다. 이벤트 3 은 종료 표식이다.
    lifecycle.processEvent(token, 1, 0xfffffff0, 0x87654321);
    assert.equal(get(process.fields.bytes, 0xd8), 0xfffffff0);
    lifecycle.processEvent(token, 3, 0, 0);
    assert.equal(get(process.fields.bytes, 0x94), 1);
    // 소멸 때 현재 커서 토큰이 같으면 먼저 지우고 표시 객체를 삭제한다.
    s.cursor.cursorGump = token;
    lifecycle.removeProcess(token, 0);
    assert.equal(s.cursor.cursorGump, 0);
    assert.equal(s.events.filter((event) => event[0] === 'X').length, 8);
    assert.equal(s.kernel.get(token), null);
});

test('CursorManaProcess: Host/풀 불일치·Form 폭·낡은 토큰을 거부한다', () => {
    const s = processScene();
    const other = new SidPool(32768, { server: true });
    const otherHost = new SquidProcessHost(other, s.types, new Kernel(), new RawSquidDestroy(other, new SquidUnpop(other, new SquidHash(), new Uint8Array(65536)), s.types), new SquidProcessState());
    assert.throws(() => createCursorManaProcess(otherHost, s.logic, s.parent, 0), TypeError);
    assert.throws(() => bindCursorManaLifecycle({}, otherHost, s.kernel, s.logic), TypeError);
    const hooks = {};
    bindCursorManaLifecycle(hooks, s.host, s.kernel, s.logic);
    assert.throws(() => hooks.createMana(s.parent, 65536), RangeError);
    // 빈 Kernel 번호에는 제거/이벤트를 보내지 않는다.
    assert.throws(() => hooks.removeProcess(0, 0), /Host 불일치/);
    assert.throws(() => hooks.processEvent(0, 1, 0, 0), /Host 불일치/);
});

test('bindCursorManaWorld: 실제 월드의 생성/WORD/해제/재등록/배치/삭제 연결', () => {
    const calls = [];
    const pool = new SidPool(32768, { server: true });
    const sid = pool.allocate();
    pool.allocated(sid)[10] = 82;
    const world = {
        core: () => ({ pool, bridgeState: { bridgeType: 82 } }),
        create: (type, flags) => { calls.push(['create', type, flags]); return sid; },
        unpop: (target, flags) => calls.push(['unpop', target, flags]),
        repop: (target, flags) => calls.push(['repop', target, flags]),
        pop: (target, x, y, flags) => calls.push(['pop', target, x, y, flags]),
        destroy: (target, flags) => calls.push(['destroy', target, flags])
    };
    const hooks = {};
    const notified = [];
    bindCursorManaWorld(hooks, world, (target) => [target, 1], (target) => notified.push(target));
    assert.equal(hooks.create(103, 2), sid);
    hooks.setWord(sid, 0x12345);
    assert.equal(get(pool.allocated(sid), 12, 2), 0x2345);
    // 현재 다리 TYPE 일 때만 표면 통지를 호출한다.
    assert.deepEqual(notified, [sid]);
    pool.allocated(sid)[10] = 5;
    hooks.setWord(sid, 1);
    assert.deepEqual(notified, [sid]);
    hooks.detach(sid, 0);
    hooks.unpop(sid, 0);
    hooks.position(sid, 1.5, 2.5, 0x10);
    hooks.destroy(sid, 0);
    assert.deepEqual(calls, [['create', 103, 2], ['unpop', sid, 0], ['repop', sid, 0], ['pop', sid, 1.5, 2.5, 0x10], ['destroy', sid, 0]]);
    assert.deepEqual(hooks.center(7), [7, 1]);
    assert.equal(hooks.themeFrame, cursorManaThemeFrame);
    // 다리 TYPE 인데 통지 장치가 없으면 거부한다.
    const bare = {};
    bindCursorManaWorld(bare, world, () => [0, 0]);
    pool.allocated(sid)[10] = 82;
    assert.throws(() => bare.setWord(sid, 1), /미연결/);
});

test('RawCursorMana: 시간 절삭·삼각 함수 직접 값과 부모/목록/SID 범위 진단', { skip: !manaRows && '원본 fixture 없음' }, () => {
    // _ftol 하위 DWORD: 음수는 0 쪽으로 절삭하고 2^32 배수는 0, 비유한/2^63 이상은 0 이다.
    assert.equal(RawCursorMana.timeInteger(-1.5), -1);
    assert.equal(RawCursorMana.timeInteger(2.9), 2);
    assert.equal(RawCursorMana.timeInteger(4294967296.5), 0);
    assert.equal(RawCursorMana.timeInteger(4294967297.9), 1);
    assert.equal(RawCursorMana.timeInteger(2147483648), -2147483648);
    assert.equal(RawCursorMana.timeInteger(2 ** 62 + 2 ** 20), 2 ** 20);
    assert.equal(RawCursorMana.timeInteger(2 ** 63), 0);
    assert.equal(RawCursorMana.timeInteger(-(2 ** 63)), 0);
    assert.equal(RawCursorMana.timeInteger(Number.NaN), 0);
    assert.equal(RawCursorMana.timeInteger(Number.POSITIVE_INFINITY), 0);
    // x87 fcos/fsin: 범위 밖 유한 입력은 그대로 남고 범위 안은 실제 삼각 값이다.
    assert.equal(RawCursorMana.trig(2 ** 63, true), 2 ** 63);
    assert.equal(RawCursorMana.trig(-(2 ** 63), false), -(2 ** 63));
    assert.equal(RawCursorMana.trig(1, true), Math.cos(1));
    assert.equal(RawCursorMana.trig(1, false), Math.sin(1));
    assert.ok(Number.isNaN(RawCursorMana.trig(Number.NaN, true)));
    // 프레임: 부모 SID 는 5 이상 풀 크기 미만이며, 부모의 종속 머리 WORD 는 낮은/높은 바이트 중 하나라도 있어야 한다.
    const scene = new ManaScene();
    scene.prepare(numbers(manaRows.find((fields) => fields[0] !== 'input36')[0]));
    const logic = scene.logic();
    const kill = () => assert.fail('종료하지 않아야 합니다');
    put(scene.fields.bytes, 8, 4);
    assert.throws(() => logic.frame(scene.fields, kill), RangeError);
    put(scene.fields.bytes, 8, 24000);
    assert.throws(() => logic.frame(scene.fields, kill), RangeError);
    put(scene.fields.bytes, 8, 150);
    put(scene.raw(150), 6, 0, 2);
    assert.throws(() => logic.frame(scene.fields, kill), /getHead/);
    // 높은 바이트만 있어도 진단은 통과해 프레임이 계속된다(시각 필드가 갱신되므로 진행 여부를 확인).
    put(scene.raw(150), 6, 0x100, 2);
    put(scene.fields.bytes, 0x98, 0);
    put(scene.fields.bytes, 0x94, 0);
    put(scene.fields.bytes, 0xd8, 1);
    put(scene.fields.bytes, 0x84, 0);
    scene.clock.now = 77.5;
    logic.frame(scene.fields, kill);
    assert.equal(new DataView(scene.fields.bytes.buffer).getFloat64(0xc0, true), 77.5);
    // 목록 SID 는 풀 크기 미만이며 30칸 밖 인덱스를 거부한다. 풀 크기와 같은 SID 도 거부한다.
    put(scene.fields.bytes, 0x84, 1);
    put(scene.fields.bytes, 0xd8, 0);
    put(scene.fields.bytes, 12, 24000);
    assert.throws(() => logic.frame(scene.fields, kill), RangeError);
    put(scene.fields.bytes, 12, 3);
    assert.throws(() => logic.frame(scene.fields, kill), RangeError);
    put(scene.fields.bytes, 0x84, 31);
    put(scene.fields.bytes, 12, 50);
    assert.throws(() => logic.notify(scene.fields, 1, 0, 0), RangeError);
});

// fixture 첫 입력으로 장면을 준비하고 재진입 변경 입력을 끈다(독립 검사의 기초 상태). (2026-11-15 WB04-E549)
function baseScene() {
    const scene = new ManaScene();
    scene.prepare(numbers(manaRows.find((fields) => fields[0] !== 'input36')[0]));
    scene.input[19] = 0;
    scene.trace = '';
    scene.globals.invalidPoint = 1;
    scene.globals.mapLimit = 256;
    return scene;
}

// 원형/사각 프레임 한 번에 쓰는 필드 입력을 직접 적는다(공급하지 않은 칸은 기초 상태 그대로). (2026-11-15 WB04-E549)
function frameInput(scene, { rect = 0, count = 0, side = 1, radius = 0, limit = 22, closing = 0, hidden = 1, now = 100, last = 100 } = {}) {
    const bytes = scene.fields.bytes;
    const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
    put(bytes, 0x98, rect);
    put(bytes, 0x84, count);
    put(bytes, 0x88, side);
    put(bytes, 0x94, closing);
    put(bytes, 0xd8, hidden);
    put(bytes, 0x90, floatToBits(radius));
    put(bytes, 0x8c, floatToBits(limit));
    view.setFloat64(0xc0, last, true);
    view.setFloat64(0xd0, 0, true);
    for (let index = 0; index < 30; index += 1) put(bytes, 12 + 4 * index, 50 + index);
    scene.clock.now = now;
}

// 프레임을 한 번 실행하고 종료 요청은 'K:0' 로만 기록한다(실제 삭제 없이 후속 쓰기를 관찰). (2026-11-15 WB04-E549)
function runFrameOnce(scene, hooks = scene.hooks()) {
    scene.logic(hooks).frame(scene.fields, () => scene.event('K', [0]));
}

// 단정도 필드를 읽는다. (2026-11-15 WB04-E549)
function F32(bytes, offset) { return new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength).getFloat32(offset, true); }

// 배치 한 칸의 현재 슬롯에 월드 좌표를 쓴다. (2026-11-15 WB04-E549)
function placeAt(scene, sid, x, y) {
    put(scene.raw(sid), 14, floatToBits(x));
    put(scene.raw(sid), 18, floatToBits(y));
}

test('RawCursorMana: 원형 프레임의 float 델타 저장·시간 역행·종료 경계·void/dead/free 종료', { skip: !manaRows && '원본 fixture 없음' }, () => {
    const bytes = (scene) => scene.fields.bytes;
    // 성장: 델타를 float 로 좁힌 값(0.0959000051…)으로 곱한다. 좁히지 않으면 마지막 저장값이 1.9385000467… 로 달라진다.
    let scene = baseScene();
    frameInput(scene, { now: 100.0959, last: 100, radius: 0.5 });
    runFrameOnce(scene);
    assert.equal(F32(bytes(scene), 0x90), 1.938499927520752);
    // 축소(닫는 중)도 같은 float 델타를 쓴다.
    scene = baseScene();
    frameInput(scene, { now: 100.0411, last: 100, radius: 1.4299999475479126, closing: 1 });
    runFrameOnce(scene);
    assert.equal(F32(bytes(scene), 0x90), 0.19699996709823608);
    // 시간이 거꾸로 가도 반경이 한계와 같으면 성장 분기에 들어가지 않는다.
    scene = baseScene();
    frameInput(scene, { now: 99, last: 100, radius: 22, limit: 22 });
    runFrameOnce(scene);
    assert.equal(F32(bytes(scene), 0x90), 22);
    // 닫는 중 반경이 단정도 0.1 과 같으면(<=) 종료하며 이 경우 시각 필드를 쓰지 않는다. 그보다 한 단위 크면 종료하지 않고 시각을 쓴다.
    scene = baseScene();
    frameInput(scene, { now: 105, last: 100, radius: Math.fround(0.1), closing: 1 });
    runFrameOnce(scene);
    assert.equal(scene.trace, 'K:0');
    assert.equal(new DataView(bytes(scene).buffer).getFloat64(0xc0, true), 100);
    scene = baseScene();
    frameInput(scene, { now: 100, last: 100, radius: bitsToFloat(floatToBits(Math.fround(0.1)) + 1), closing: 1 });
    runFrameOnce(scene);
    assert.equal(scene.trace, '');
    assert.equal(new DataView(bytes(scene).buffer).getFloat64(0xc0, true), 100);
    // 첫 표시 객체가 free(1) 또는 dead(2)이면 종료 요청 후 바로 돌아가며 시각 필드는 이전 값이다. void(4)만이면 계속한다.
    for (const state of [1, 2, 3]) {
        scene = baseScene();
        frameInput(scene, { count: 2, hidden: 0, now: 105, last: 100, radius: 3 });
        scene.raw(50)[11] = state;
        runFrameOnce(scene);
        assert.equal(scene.trace, 'K:0', `상태 ${state}`);
        assert.equal(new DataView(bytes(scene).buffer).getFloat64(0xc0, true), 100, `상태 ${state}`);
    }
    scene = baseScene();
    frameInput(scene, { count: 1, hidden: 0, now: 105, last: 100, radius: 3 });
    scene.raw(50)[11] = 4;
    runFrameOnce(scene);
    assert.ok(scene.trace.startsWith('D:50:0;O:150;V:50:'), scene.trace);
    assert.equal(new DataView(bytes(scene).buffer).getFloat64(0xc0, true), 105);
});

test('RawCursorMana: 원형 프레임이 중심 조회마다 현재 부모를 다시 읽고 float 합 경계를 지킨다', { skip: !manaRows && '원본 fixture 없음' }, () => {
    // 가상 해제가 부모 필드를 149 로 바꾸면 중심 조회는 그 현재 부모를 받는다(부모 머리 진단도 다시 한다).
    let scene = baseScene();
    frameInput(scene, { count: 1, hidden: 0, radius: 3 });
    put(scene.raw(149), 6, 5, 2);
    const moved = { ...scene.hooks(), detach: (sid, flags) => { scene.event('D', [sid, flags]); put(scene.fields.bytes, 8, 149); } };
    runFrameOnce(scene, moved);
    assert.ok(scene.trace.startsWith('D:50:0;O:149;V:50:'), scene.trace);
    scene = baseScene();
    frameInput(scene, { count: 1, hidden: 0, radius: 3 });
    const unheaded = { ...scene.hooks(), detach: () => put(scene.fields.bytes, 8, 148) };
    assert.throws(() => runFrameOnce(scene, unheaded), /getHead/);
    // 중심(255.99998474…)에 x=7.7e-6 을 더한 값은 단정도로 저장하면 256 이 되어 범위 밖(대체점)이다. 저장 전 합으로 판단하지 않는다.
    scene = baseScene();
    frameInput(scene, { count: 1, hidden: 0, radius: Math.fround(7.7e-6), now: 10, last: 10 });
    const edge = { ...scene.hooks(), center: (sid) => { scene.event('O', [sid]); return [255.99998474121094, 100]; } };
    runFrameOnce(scene, edge);
    assert.equal(scene.trace, `D:50:0;O:150;V:50:${floatToBits(1)}:${floatToBits(1)}:0`);
});

test('RawCursorMana: 사각 프레임의 float 델타 저장·삭제 경계·위상 범위·음수 절반 개수', { skip: !manaRows && '원본 fixture 없음' }, () => {
    const setSlots = (scene, positions) => {
        for (const [sid, x, y] of positions) placeAt(scene, sid, x, y);
        placeAt(scene, 150, positions.parentX, positions.parentY);
    };
    // 부모(2^24, 2^24)와 현재 위치(0.5, 0.5): 델타를 단정도로 좁히면 16777216 이 되어 네 변 모두 원점(0)=범위 밖 대체점이 되고 삭제된다.
    let scene = baseScene();
    frameInput(scene, { rect: 1, count: 4, side: 1, now: 5, last: 5 });
    put(scene.fields.bytes, 0xbc, floatToBits(0));
    put(scene.fields.bytes, 0xb8, floatToBits(0));
    placeAt(scene, 150, 16777216, 16777216);
    for (const sid of [50, 51, 52, 53]) placeAt(scene, sid, 0.5, 0.5);
    runFrameOnce(scene);
    const invalid = `${floatToBits(1)}:${floatToBits(1)}`;
    assert.equal(scene.trace, [50, 51, 52, 53].map((sid) => `D:${sid}:0;V:${sid}:${invalid}:16;X:${sid}:0`).join(';'));
    // 삭제 경계: 변마다 한 쪽 거리가 한계(22)와 같으면 남고(>), 한계를 넘는 한 쪽만으로도 삭제된다(||).
    scene = baseScene();
    frameInput(scene, { rect: 1, count: 4, side: 1, limit: 22, now: 5, last: 5 });
    put(scene.fields.bytes, 0xbc, floatToBits(0));
    put(scene.fields.bytes, 0xb8, floatToBits(0));
    setSlots(scene, Object.assign([[50, 30, 8], [51, 8, 30], [52, 7, 30], [53, 30, 7]], { parentX: 30, parentY: 30 }));
    runFrameOnce(scene);
    assert.deepEqual(scene.trace.split(';').filter((event) => event[0] === 'X'), ['X:52:0', 'X:53:0']);
    assert.deepEqual([50, 51, 52, 53].map((index) => get(scene.fields.bytes, 12 + 4 * (index - 50))), [50, 51, 0, 0]);
    // 개수 8 은 둘째 위상 복사가 일곱째 칸(위상 7칸 밖)에 이르러 가상 해제 전에 거부한다.
    scene = baseScene();
    frameInput(scene, { rect: 1, count: 8, side: 8, now: 5, last: 5 });
    assert.throws(() => runFrameOnce(scene), RangeError);
    assert.equal(scene.trace, '');
    // 개수 -1 은 C 정수 나눗셈(0 쪽 절삭)으로 절반이 0 이어서 첫 위상 하나를 증가시키고 처리할 표시 객체가 없어 종료를 요청한다.
    scene = baseScene();
    frameInput(scene, { rect: 1, count: 0xffffffff, side: 0xffffffff, now: 5, last: 5 });
    runFrameOnce(scene);
    assert.equal(scene.trace, 'K:0');
    assert.equal(F32(scene.fields.bytes, 0x9c), Math.fround(0.55));
    assert.equal(F32(scene.fields.bytes, 0xa0), 0.5);
});

test('RawCursorMana: 생성 계약(범위 밖 대체점·공급 반경·발자국 감김·부모 TYPE·목록 넘침·Attach 쓰기)', { skip: !manaRows && '원본 fixture 없음' }, () => {
    let scene = baseScene();
    scene.globals.invalidPoint = 2.5;
    scene.globals.radiusLimit = 10.5;
    scene.types[100].footX = 0;
    scene.types[100].footY = -5;
    scene.logic().construct(scene.fields, 150, 0, scene.attach());
    assert.equal(scene.trace.split(';').filter((event) => event.startsWith(`V:`)).length, 8);
    assert.ok(scene.trace.includes(`V:50:${floatToBits(2.5)}:${floatToBits(2.5)}:0`));
    assert.equal(F32(scene.fields.bytes, 0x8c), 10.5);
    assert.equal(F32(scene.fields.bytes, 0xbc), -1);
    assert.equal(F32(scene.fields.bytes, 0xb8), -6);
    // Attach 가 쓰지 않으면 필드의 Form/부모 칸은 생성자가 둔 0 그대로다.
    scene = baseScene();
    scene.logic().construct(scene.fields, 150, 0, () => {});
    assert.deepEqual([get(scene.fields.bytes, 4), get(scene.fields.bytes, 8)], [0, 0]);
    // 부모 TYPE 이 표 크기(188)와 같으면 범위 오류이다.
    scene = baseScene();
    scene.raw(150)[10] = 188;
    assert.throws(() => scene.logic().construct(scene.fields, 150, 0, scene.attach()), RangeError);
    // 가상 위치 효과가 개수를 99 로 올려도 30칸을 넘기 전에 거부하며 표시 객체는 정확히 30개만 만든다.
    scene = baseScene();
    const raising = { ...scene.hooks(), position: () => put(scene.fields.bytes, 0x84, 99) };
    assert.throws(() => scene.logic(raising).construct(scene.fields, 150, 0, scene.attach()), /생성 목록 범위/);
    assert.equal(scene.trace.split(';').filter((event) => event.startsWith('C:')).length, 30);
});

test('RawCursorMana: 좌표 제한·목록/부모 직접 범위·소멸 토큰 폭과 free/dead 건너뜀', { skip: !manaRows && '원본 fixture 없음' }, () => {
    // 제한은 엄격한 부등식이며 맵 한계는 32비트 정수로 감기고 대체점은 단정도로 저장한다.
    const scene = baseScene();
    const logic = scene.logic();
    const invalid = [1, 1];
    assert.deepEqual(logic.clamp([0, 5]), invalid);
    assert.deepEqual(logic.clamp([256, 5]), invalid);
    assert.deepEqual(logic.clamp([5, 0]), invalid);
    assert.deepEqual(logic.clamp([5, 256]), invalid);
    assert.deepEqual(logic.clamp([Number.NaN, 5]), invalid);
    assert.deepEqual(logic.clamp([255.5, 0.5]), [255.5, 0.5]);
    scene.globals.mapLimit = 2 ** 32 + 256;
    assert.deepEqual(logic.clamp([300, 5]), invalid);
    scene.globals.mapLimit = 256;
    scene.globals.invalidPoint = 0.1;
    assert.deepEqual(logic.clamp([Number.NaN, 5]), [Math.fround(0.1), Math.fround(0.1)]);
    // 목록 인덱스는 0..29 이고 목록의 SID 는 풀 크기 미만이다.
    assert.throws(() => logic.marker(scene.fields, -1), RangeError);
    assert.throws(() => logic.marker(scene.fields, 30), RangeError);
    put(scene.fields.bytes, 12, 24000);
    assert.throws(() => logic.marker(scene.fields, 0), RangeError);
    put(scene.fields.bytes, 12, 50);
    assert.equal(logic.marker(scene.fields, 0), 50);
    // 소멸: 현재 커서 토큰은 부호 없는 32비트 전체로 비교하고, free/dead 표시 객체는 삭제하지 않는다.
    const wide = baseScene();
    frameInput(wide, { count: 0 });
    wide.cursor.cursorGump = 0x80000001;
    wide.logic().destroy(wide.fields, 0x80000001);
    assert.equal(wide.cursor.cursorGump, 0);
    const skipping = baseScene();
    frameInput(skipping, { count: 4 });
    skipping.raw(50)[11] = 1;
    skipping.raw(51)[11] = 2;
    skipping.raw(52)[11] = 4;
    put(skipping.fields.bytes, 12 + 8, 0);
    skipping.logic().destroy(skipping.fields, 1);
    assert.equal(skipping.trace, 'X:53:0');
    // 시간 절삭/삼각 함수의 입력 범위 경계(2^62/2^63 직전과 이상).
    assert.equal(RawCursorMana.trig(2 ** 62, true), Math.cos(2 ** 62));
    assert.equal(RawCursorMana.trig(2 ** 63 - 1024, false), Math.sin(2 ** 63 - 1024));
});

test('CursorManaProcess: 종료/제거 flags 전달·낡은 Kernel 객체 거부·월드 삭제 flags·TYPE 표 크기', () => {
    const s = processScene();
    const kills = [];
    const original = s.host.kill.bind(s.host);
    s.host.kill = (process, flags) => { kills.push(flags >>> 0); return original(process, flags); };
    const lifecycle = {};
    bindCursorManaLifecycle(lifecycle, s.host, s.kernel, s.logic);
    // 닫는 중 반경 0 으로 한 프레임을 돌리면 실제 Kill(flags 0)이 호출된다.
    let token = lifecycle.createMana(s.parent, 0);
    lifecycle.processEvent(token, 3, 0, 0);
    s.state.now = 10.5;
    s.kernel.get(token).runFrame();
    assert.deepEqual(kills, [0]);
    assert.equal(s.kernel.get(token), null);
    // 제거 요청의 flags 는 그대로 Host Kill 에 전달한다.
    token = lifecycle.createMana(s.parent, 0);
    lifecycle.removeProcess(token, 0x10);
    assert.deepEqual(kills, [0, 0x10]);
    // SquidProcess 가 아닌 Kernel 객체의 번호는 거부한다.
    const plain = s.kernel.add(new BaseProcess());
    assert.throws(() => lifecycle.removeProcess(plain, 0), /Host 불일치/);
    assert.throws(() => lifecycle.processEvent(plain, 1, 0, 0), /Host 불일치/);
    // 월드 삭제는 호출자의 flags 를 그대로 보낸다.
    const calls = [];
    const world = { core: () => ({ pool: s.pool, bridgeState: { bridgeType: 82 } }), create() {}, unpop() {}, repop() {}, pop() {}, destroy: (sid, flags) => calls.push([sid, flags]) };
    const hooks = {};
    bindCursorManaWorld(hooks, world, () => [0, 0]);
    hooks.destroy(7, 3);
    assert.deepEqual(calls, [[7, 3]]);
    // 189칸 TYPE 표도 거부한다.
    assert.throws(() => new RawCursorMana(s.pool, [...s.types, s.types[0]], s.state, s.globals, s.cursor, s.hooks), TypeError);
});

test('createCursorManaFields: 224바이트 필드를 0 으로 만든다', () => {
    const fields = createCursorManaFields();
    assert.equal(fields.bytes.length, CURSOR_MANA_FIELD_BYTES);
    assert.equal(CURSOR_MANA_FIELD_BYTES, 0xe0);
    assert.ok(fields.bytes.every((value) => value === 0));
});
