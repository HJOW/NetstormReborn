// 커서 사정거리 표시 raw-cursor-range.js(RawCursorRange 생성·직선/원형 프레임·통지·소멸과 CursorRangeProcess)를 원본 10.78 관측 593행과 cpppj 시험 계약으로 대조한다(WB04-E550).
//  - cursor-range-x86.tsv 593행: 생성·프레임·통지·소멸·연속 호출의 효과 순서·512바이트 필드·전역·raw 슬롯 102개·단계별 필드 전부.
//    방향/발자국은 실제 RawGunTarget 을 같은 풀·해시에 연결하며 검사 전용 식으로 대신하지 않는다.
//  - 계약: 미연결 경계 거부·판본/TYPE 표 크기 거부·목록 범위·실제 Host/Kernel 프로세스 수명.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E550)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, bitsToFloat, floatToBits, put, get } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { Kernel } from '../www/core/kernel.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { RawSquidDestroy } from '../www/core/squid-destroy.js';
import { typeConstructorAddress } from '../www/core/squid-constructors.js';
import { SquidProcessHost, SquidProcessState } from '../www/core/squid-process.js';
import { RawGunTarget } from '../www/core/raw-gun-target.js';
import { CursorDropState } from '../www/core/raw-cursor-drop.js';
import { CursorManaGlobals } from '../www/core/raw-cursor-mana.js';
import {
    RawCursorRange, CursorRangeGlobals, CursorRangeProcess, CURSOR_RANGE_FIELD_BYTES, createCursorRangeFields, createCursorRangeProcess, bindCursorRangeLifecycle
} from '../www/core/raw-cursor-range.js';

// 원본 관측 fixture 이다. 없으면 해당 검사를 건너뛴다. (2026-11-15 WB04-E550)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'cursor-range-x86.tsv'));
// 원본 DWORD 목록만 읽으며 기대 출력을 호스트 수식으로 생성하지 않는다. (2026-11-15 WB04-E550)
const numbers = (value) => value.split(',').map((field) => Number(field) >>> 0);
// 바이트 배열을 소문자 16진 문자열로 만든다(원문 Hex). (2026-11-15 WB04-E550)
const hex = (bytes) => Buffer.from(bytes).toString('hex');
// 입력 두 DWORD 를 원본 double 시각으로 읽는다. (2026-11-15 WB04-E550)
function inputTime(input, index) {
    const view = new DataView(new ArrayBuffer(8));
    view.setUint32(0, input[index], true);
    view.setUint32(4, input[index + 1], true);
    return view.getFloat64(0, true);
}
// 단정도 필드를 읽는다. (2026-11-15 WB04-E550)
function F32(bytes, offset) { return new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength).getFloat32(offset, true); }
// signed 16비트로 해석한다. (2026-11-15 WB04-E550)
const int16 = (value) => (value << 16) >> 16;

// 외부 가상 경계와 TYPE/총 자료만 독립 입력으로 공급하고 생성/이동/이벤트/삭제의 분기는 실제 모듈 몸체가 계산하는 장면이다(cpppj RangeScene). (2026-11-15 WB04-E550)
class RangeScene {
    constructor() {
        this.pool = new SidPool(24000, { server: false });
        this.hash = new SquidHash();
        this.types = Array.from({ length: 188 }, () => ({ cost: 0, flags1: 0, flags2: 0, footX: 0, footY: 0, maxHitPoints: 0, group: 0, theme: 0, hotspotX: 0, hotspotY: 0 }));
        this.gunTypes = Array.from({ length: 188 }, () => ({ range: 0, theme: 0, directionFrames: [0, 1, 2, 3], heightExtension: 0 }));
        this.shapes = Array.from({ length: 188 }, () => ({ loaded: false, frameCount: 0, frames: [] }));
        this.target = {
            friendlyFire: false, checking: false, showRanges: false, sunCannonType: 128, sunFenceType: 136, windArcherType: 140, thunderArcherType: 148,
            windHalfAngle: 30, scaleX: 16, scaleY: 11, allyMasks: new Array(9).fill(0), playerFlags: new Array(9).fill(0), angles: {}
        };
        this.coordinates = new CursorManaGlobals();
        this.globals = new CursorRangeGlobals(this.coordinates, this.target);
        this.clock = new SquidProcessState();
        this.cursor = new CursorDropState();
        this.fields = createCursorRangeFields();
        this.input = [];
        this.trace = '';
        this.steps = '';
        this.created = 0;
        this.returned = 0;
        this.mutated = false;
        this.killed = false;
        // fixture 에서 쓰는 모든 슬롯을 실제 풀에서 확보한다.
        for (let value = 5; value <= 300; value += 1) assert.equal(this.pool.allocate(2), value);
        // 방향/발자국을 검사 전용 식으로 대신하지 않는 실제 모듈이다.
        this.gun = new RawGunTarget(this.pool, this.hash, this.types, this.gunTypes, this.shapes, this.target);
    }

    // free/dead 입력도 준비하는 검사 전용 접근이며 제품의 할당 검사를 바꾸지 않는다.
    raw(sid) { return this.pool.physical(sid); }

    // 모든 DWORD/float 인수와 호출 순서를 원문 형식으로 기록한다.
    event(kind, values = []) { this.trace = `${this.trace}${this.trace ? ';' : ''}${kind}${values.map((value) => `:${value >>> 0}`).join('')}`; }

    // 원본 실행기에 공급했던 명시 외부 효과 입력만 상태를 바꾼다.
    mutate(kind) {
        if (this.mutated) return;
        const mode = this.input[19];
        if (!((mode === 1 && kind === 'D') || (mode === 2 && kind === 'V') || (mode === 3 && kind === 'X') || (mode === 4 && kind === 'D') || (mode === 5 && kind === 'O') || (mode === 6 && kind === 'A'))) return;
        this.mutated = true;
        if (mode >= 1 && mode <= 5) { put(this.fields.bytes, 0x1a4, 1); put(this.fields.bytes, 0x1a8, 1); }
        if (mode === 4) put(this.fields.bytes, 0x1e8, 0);
        this.clock.now += mode === 1 ? 0.125 : mode === 2 ? 0.25 : mode === 5 ? 0.5 : mode === 6 ? 1.25 : 0;
    }

    // 이전 행의 결과를 넘기지 않고 입력 바이트만 준비한다.
    prepare(values) {
        assert.equal(values.length, 58);
        this.input = values;
        this.trace = '';
        this.steps = '';
        this.created = 0;
        this.returned = 0;
        this.mutated = false;
        this.killed = false;
        this.fields.bytes.fill(0xa5);
        this.cursor = new CursorDropState();
        this.cursor.cursorGump = values[16];
        this.coordinates.mapLimit = values[18] | 0;
        this.coordinates.invalidPoint = bitsToFloat(values[35]);
        this.target.scaleX = bitsToFloat(values[47]);
        this.target.scaleY = bitsToFloat(values[48]);
        this.target.windHalfAngle = bitsToFloat(values[53]);
        this.clock.now = inputTime(values, 22);
        // TYPE/총 값은 입력이며 getter/판정은 실제 모듈이 실행한다.
        for (const number of [100, 101, 128, 140]) {
            const type = this.types[number];
            type.group = values[1] | 0;
            type.theme = values[2] | 0;
            type.flags2 = values[3];
            type.footX = values[51] | 0;
            type.footY = values[52] | 0;
            type.hotspotX = values[45] | 0;
            type.hotspotY = values[46] | 0;
            this.gunTypes[number].range = bitsToFloat(number === values[54] ? values[57] : values[5]);
            this.gunTypes[number].theme = type.theme;
            this.gunTypes[number].directionFrames = [0, 1, 2, 3];
        }
        // 원문의 raw 102개 전체 표식을 유지하며 TYPE/상태/좌표의 입력만 바꾼다.
        for (let sid = 50; sid <= 251; sid += 1) {
            if (sid >= 150 && sid < 250) continue;
            const bytes = this.raw(sid);
            bytes.fill(0xa5);
            put(bytes, 0, 1);
            bytes[10] = (sid === 250 ? values[4] : sid === 251 ? 6 : 104) & 0xff;
            bytes[11] = sid >= 250 ? 0 : values[13] & 0xff;
            bytes[34] = 1;
            bytes[40] = sid === 250 ? values[17] & 0xff : 0;
            put(bytes, 6, sid === 250 ? 300 : 0, 2);
            put(bytes, 14, values[sid >= 250 ? 28 : 30]);
            put(bytes, 18, values[sid >= 250 ? 29 : 31]);
            put(bytes, 36, 2);
        }
        put(this.raw(251), 18, values[54], 2);
        const bytes = this.fields.bytes;
        put(bytes, 0, 0x50c498);
        put(bytes, 4, 300);
        put(bytes, 8, 250);
        put(bytes, 12, values[36]);
        put(bytes, 16, values[1] === 6 ? 1 : 0);
        // SID0 의 첫 칸/전체 칸 입력도 원문과 같은 100칸 배열에 준비한다.
        for (let index = 0; index < 100; index += 1) put(bytes, 20 + 4 * index, (values[14] === 2 || (values[14] === 1 && index === 0)) ? 0 : 50 + index);
        // 명시 필드 오프셋과 입력의 대응이다.
        const initial = [[0x1a4, values[37]], [0x1a8, values[8]], [0x1ac, values[37]], [0x1b0, values[34]], [0x1b4, values[7]], [0x1b8, values[9]], [0x1bc, floatToBits(5)], [0x1c0, values[39]],
            [0x1c4, values[40]], [0x1c8, values[41]], [0x1cc, values[42]], [0x1e8, values[10]], [0x1ec, values[38]]];
        // 입력 필드만 바꾸며 다른 padding 은 표식 그대로 둔다.
        for (const [offset, value] of initial) put(bytes, offset, value);
        const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
        view.setFloat64(0x1d0, inputTime(values, 24), true);
        view.setFloat64(0x1d8, inputTime(values, 55), true);
        view.setFloat64(0x1e0, inputTime(values, 26), true);
        // 네 방향의 허용은 frame 단독 실행의 명시 입력이다. 생성자는 실제 판정을 실행한다.
        for (let side = 0; side < 4; side += 1) put(bytes, 0x1f0 + 4 * side, (values[15] & (1 << side)) ? 1 : 0);
    }

    // 계산 대신 실제 경계 인수/효과와 TYPE 자료만 공급한다.
    hooks() {
        return {
            world: {
                create: (type, flags) => { const sid = 50 + this.created; this.created += 1; this.event('C', [type, flags, sid]); return sid; },
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
            },
            range: (type) => {
                if (!this.gunTypes[type]) throw new RangeError('사정거리 시험 TYPE 범위');
                return this.gunTypes[type].range;
            },
            firstContent: (sid) => { this.event('T', [sid, 1, 0, 0]); return 251; },
            mode: (sid) => { this.event('M', [sid]); return this.input[6]; },
            footprint: (sid) => { this.event('P', [sid]); return this.gun.footprint(sid, false); },
            cannonDirection: (query, direction) => {
                this.event('B', [floatToBits(query.x), floatToBits(query.y), floatToBits(query.centerX), floatToBits(query.centerY), query.type, query.direction, query.owner, query.frame, direction]);
                return this.gun.cannonDirection(query, direction);
            },
            directionFrame: (sid, direction) => { this.event('F', [sid, direction]); return this.gun.directionFrame(this.raw(sid)[10], direction); },
            firingOffset: (type, frame) => { this.event('H', [type, 0, frame]); return [int16(this.input[49]), int16(this.input[50])]; },
            frameRun: (type, variant) => {
                assert.ok(type === 104 && variant < 2);
                // C++ LetterRun 은 { first, count } 순서의 집합 초기화이다.
                return { first: this.input[43] + variant * 10, count: this.input[44] + variant };
            },
            diagnostic: (color, text) => { assert.equal(text, 'Dead circle range!'); this.event('Q', [color]); }
        };
    }

    // 실제 Host 부착의 필드 쓰기와 명시 현재 시각 변경을 공급한다.
    attach() {
        return (actual, parent, form, flags) => {
            assert.equal(actual, this.fields);
            this.event('A', [51, parent, form, flags]);
            put(this.fields.bytes, 4, form || 300);
            put(this.fields.bytes, 8, parent);
            this.mutate('A');
        };
    }

    // 한 실제 호출 후 필드 512바이트를 모두 기록한다.
    snapshot() { this.steps = `${this.steps}${this.steps ? '|' : ''}${hex(this.fields.bytes)}`; }

    // 원문과 같은 단일/연속 호출을 실행한다.
    run(logic) {
        const input = this.input;
        const frame = () => {
            logic.frame(this.fields, () => { this.event('K', [0]); this.killed = true; logic.destroy(this.fields, 1); });
            this.snapshot();
        };
        if (input[0] === 0 || input[0] === 5) {
            logic.construct(this.fields, 250, 0, this.attach());
            this.snapshot();
            if (input[0] === 5) {
                // 생성 필드를 그대로 이어 정상 프레임을 실행한다.
                for (let index = 0; index < input[21]; index += 1) { this.clock.now += 0.125; frame(); }
                logic.notify(this.fields, 3, 0, 0);
                this.snapshot();
                // 실제 종료의 즉시/반경 축소/다음 프레임 자기 삭제를 이어 관찰한다.
                for (let index = 0; index < 3 && !this.killed; index += 1) { this.clock.now += 0.25; frame(); }
                if (!this.killed) { logic.destroy(this.fields, 1); this.snapshot(); }
            }
        } else if (input[0] === 1) frame();
        else if (input[0] === 2) { logic.notify(this.fields, input[11], input[12], 0x87654321); this.snapshot(); }
        else if (input[0] === 3) { logic.destroy(this.fields, 1); this.snapshot(); }
        else if (input[0] === 4) {
            this.event('N', [0x200]);
            if (input[20]) { logic.construct(this.fields, 250, 0, this.attach()); this.returned = 1; }
            this.snapshot();
        }
    }

    // 가상 표 토큰을 포함한 전체 102개 raw 슬롯의 결과다.
    rawSnapshot() {
        let result = '';
        for (let sid = 50; sid <= 251; sid += 1) if (sid < 150 || sid >= 250) result += hex(this.pool.slot(sid));
        return result;
    }

    // 현재 커서 토큰·double 시각·할당 반환값이다.
    globalsText() {
        const view = new DataView(new ArrayBuffer(8));
        view.setFloat64(0, this.clock.now, true);
        return `${this.cursor.cursorGump >>> 0},${view.getUint32(0, true)},${view.getUint32(4, true)},${this.returned}`;
    }

    // 현재 장면의 실제 몸체이다.
    logic(hooks = this.hooks()) { return new RawCursorRange(this.pool, this.types, this.clock, this.globals, this.cursor, hooks); }
}

test('RawCursorRange: 원본 10.78 생성·직선/원형 프레임·통지·소멸 593행의 효과·필드·전역·raw 슬롯이 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new RangeScene();
    let count = 0;
    // 기대값을 계산하지 않고 독립 원문을 재생한다.
    for (const row of rows.filter((fields) => fields[0] !== 'input58')) {
        assert.equal(row.length, 11);
        scene.prepare(numbers(row[0]));
        scene.run(scene.logic());
        const label = `행 ${count} 입력 ${row[0]}`;
        assert.equal(scene.trace || '-', row[1], `${label} 효과`);
        assert.equal(hex(scene.fields.bytes), row[2], `${label} 필드`);
        assert.equal(scene.globalsText(), row[3], `${label} 전역`);
        assert.equal(scene.rawSnapshot(), row[4], `${label} raw`);
        assert.equal(scene.steps, row[5], `${label} 단계별 필드`);
        // 기본 53비트와 64비트 원문이 모두 같았던 행은 뒤 다섯 열이 비어('-') 있다.
        assert.equal(row[6], '-', `${label} 두 관측 차이`);
        count += 1;
    }
    assert.equal(count, 593);
});

test('RawCursorRange: 공유 전역·판본/TYPE 표 크기·미연결 경계·목록 범위 계약', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new RangeScene();
    scene.prepare(numbers(rows.find((fields) => fields[0] !== 'input58')[0]));
    const logic = scene.logic();
    assert.equal(logic.pool, scene.pool);
    assert.equal(logic.processType, 51);
    assert.equal(scene.globals.coordinates, scene.coordinates);
    assert.equal(scene.globals.targeting, scene.target);
    assert.throws(() => new RawCursorRange({ edition: 'cd1072', capacity: 24000 }, scene.types, scene.clock, scene.globals, scene.cursor, scene.hooks()), TypeError);
    assert.throws(() => new RawCursorRange(scene.pool, scene.types.slice(0, 187), scene.clock, scene.globals, scene.cursor, scene.hooks()), TypeError);
    // 효과가 없으면 도달한 첫 가상 경계에서 거부한다(빈 효과로 생략하지 않음).
    const missing = scene.logic({});
    assert.throws(() => missing.construct(scene.fields, 250, 0, scene.attach()), /미연결/);
    // 100칸 밖의 목록 SID 는 풀 크기 미만이어야 한다.
    put(scene.fields.bytes, 20, 24000);
    assert.throws(() => logic.destroy(scene.fields, 1), RangeError);
    assert.equal(CURSOR_RANGE_FIELD_BYTES, 0x200);
    assert.equal(createCursorRangeFields().bytes.length, 0x200);
});

// ---- 변형 확인에서 드러난 원본 관측이 닿지 않는 경계의 계약 시험(WB04-E550) ----

// fixture 첫 입력(생성, 원형 모드, 재진입 변경 끔)을 바탕으로 일부 입력 칸만 바꾼 장면을 준비한다. (2026-11-15 WB04-E550)
function sceneWith(overrides = {}) {
    const scene = new RangeScene();
    const values = numbers(rows.find((fields) => fields[0] !== 'input58')[0]).slice();
    values[19] = 0;
    for (const [index, value] of Object.entries(overrides)) values[Number(index)] = value >>> 0;
    scene.prepare(values);
    return scene;
}

// 시각 필드(이전/생성/기한)와 현재 게임 시각을 직접 쓴다. (2026-11-15 WB04-E550)
function setTimes(scene, { now, previous, created, deadline }) {
    const view = new DataView(scene.fields.bytes.buffer, scene.fields.bytes.byteOffset, scene.fields.bytes.byteLength);
    scene.clock.now = now;
    if (previous !== undefined) view.setFloat64(0x1d0, previous, true);
    if (created !== undefined) view.setFloat64(0x1d8, created, true);
    if (deadline !== undefined) view.setFloat64(0x1e0, deadline, true);
}

// 프레임을 한 번 실행하고 종료 요청은 'K:0' 으로만 기록한다(실제 삭제 없이 후속 쓰기를 관찰). (2026-11-15 WB04-E550)
function frameOnce(scene, hooks = scene.hooks()) {
    scene.logic(hooks).frame(scene.fields, () => scene.event('K', [0]));
}

// 시험 장면의 월드 효과 일부를 바꾼 훅 묶음을 만든다. (2026-11-15 WB04-E550)
function withWorld(scene, world) {
    const base = scene.hooks();
    return { ...base, world: { ...base.world, ...world } };
}

test('RawCursorRange: 각도 정규화·포함·거리의 직접 값(경계 포함)', () => {
    const n = (value) => RawCursorRange.normalize(value);
    // 0 은 그대로이고 음수는 한 바퀴 이상 더하며 360 은 허용하고 초과분은 나머지만 남긴다.
    assert.deepEqual([0, -90, -360, 360, 361, 450, 720, -720, -0.5].map(n), [0, 270, 360, 360, 1, 90, 0, 360, 359.5]);
    const c = (value, first, last) => RawCursorRange.contains(value, first, last);
    // 180도 이하 구간은 안쪽이며 양 끝을 포함한다. 순서가 뒤바뀐 입력도 같다.
    assert.deepEqual([c(15, 10, 20), c(10, 10, 20), c(20, 10, 20), c(9, 10, 20), c(21, 10, 20), c(15, 20, 10)], [true, true, true, false, false, true]);
    assert.deepEqual([c(100, 0, 180), c(190, 0, 180)], [true, false]);
    // 180도를 넘는 구간은 바깥쪽을 선택하며 양 끝을 포함한다.
    assert.deepEqual([c(0, 0, 270), c(270, 0, 270), c(100, 0, 270), c(1, 0, 270)], [true, true, false, false]);
    const d = (value, endpoint) => RawCursorRange.angleDistance(value, endpoint);
    assert.deepEqual([d(10, 350), d(350, 10), d(100, 90), d(0, 180)], [20, 20, 10, 180]);
});

test('RawCursorRange: 직접 접근 범위·좌표 제한·목록과 부모 진단·TYPE 표 크기', () => {
    const scene = sceneWith();
    const logic = scene.logic();
    const bytes = scene.fields.bytes;
    assert.throws(() => logic.raw(4), RangeError);
    assert.equal(logic.raw(5).length, 50);
    // 목록 인덱스는 0..99 이며 목록의 SID 는 풀 크기 미만이다(범위 밖 인덱스가 읽을 칸에 작은 값을 둬 접근 거부를 확인).
    put(bytes, 0x1a4, 50);
    assert.throws(() => logic.marker(scene.fields, 100), RangeError);
    put(bytes, 16, 1);
    assert.throws(() => logic.marker(scene.fields, -1), RangeError);
    put(bytes, 20, 24000);
    assert.throws(() => logic.marker(scene.fields, 0), RangeError);
    put(bytes, 20, 50);
    assert.equal(logic.marker(scene.fields, 0), 50);
    // 제한은 엄격한 부등식이며 맵 한계는 32비트 정수로 감기고 대체점은 단정도로 저장한다.
    const invalid = [1, 1];
    assert.deepEqual([[0, 5], [256, 5], [5, 0], [5, 256], [Number.NaN, 5]].map((point) => logic.clamp(point)), [invalid, invalid, invalid, invalid, invalid]);
    assert.deepEqual(logic.clamp([255.5, 0.5]), [255.5, 0.5]);
    scene.coordinates.mapLimit = 2 ** 32 + 256;
    assert.deepEqual(logic.clamp([300, 5]), invalid);
    scene.coordinates.mapLimit = 256;
    scene.coordinates.invalidPoint = 0.1;
    assert.deepEqual(logic.clamp([Number.NaN, 5]), [Math.fround(0.1), Math.fround(0.1)]);
    // 부모 슬롯 번호 범위와 종속 머리 WORD 의 낮은/높은 바이트 진단이다.
    put(bytes, 8, 4);
    assert.throws(() => frameOnce(scene), RangeError);
    put(bytes, 8, 24000);
    assert.throws(() => frameOnce(scene), RangeError);
    put(bytes, 8, 250);
    put(scene.raw(250), 6, 0, 2);
    assert.throws(() => frameOnce(scene), /getHead/);
    put(scene.raw(250), 6, 0x100, 2);
    scene.coordinates.invalidPoint = 1;
    setTimes(scene, { now: 77.5, previous: 70 });
    frameOnce(scene);
    assert.equal(new DataView(bytes.buffer).getFloat64(0x1d0, true), 77.5);
    // 189칸 TYPE 표도 거부한다.
    assert.throws(() => new RawCursorRange(scene.pool, [...scene.types, scene.types[0]], scene.clock, scene.globals, scene.cursor, scene.hooks()), TypeError);
});

test('RawCursorRange: 생성 계약(대체점·목록 넘침·Attach 쓰기·TYPE 범위·내용물 사거리·Wind 모드 폭)', () => {
    const inv = floatToBits(2.5);
    // 원형: 대체점이 단정도로 저장된 공급 값이다(8개 객체 모두).
    let scene = sceneWith();
    scene.coordinates.invalidPoint = 2.5;
    scene.logic().construct(scene.fields, 250, 0, scene.attach());
    for (let sid = 50; sid < 58; sid += 1) assert.ok(scene.trace.includes(`V:${sid}:${inv}:${inv}:0`), `원형 ${sid}`);
    // 직선(group 1): 네 방향 모두 허용되어 방향당 2개(사거리 10 × 0.2)씩 만든다.
    scene = sceneWith({ 1: 1 });
    scene.coordinates.invalidPoint = 2.5;
    scene.logic().construct(scene.fields, 250, 0, scene.attach());
    for (let sid = 50; sid < 58; sid += 1) assert.ok(scene.trace.includes(`V:${sid}:${inv}:${inv}:0`), `직선 ${sid}`);
    assert.equal(get(scene.fields.bytes, 0x1ac), 8);
    // 가상 위치 효과가 개수를 200 으로 올려도 100칸을 넘기 전에 거부하며 표시 객체는 정확히 100개만 만든다.
    for (const overrides of [{ 1: 1 }, {}]) {
        scene = sceneWith(overrides);
        const raising = withWorld(scene, { position: () => { put(scene.fields.bytes, 0x1a8, 200); put(scene.fields.bytes, 0x1a4, 200); } });
        assert.throws(() => scene.logic({ ...scene.hooks(), world: raising.world }).construct(scene.fields, 250, 0, scene.attach()), /생성 목록 범위/);
        assert.equal(scene.trace.split(';').filter((event) => event.startsWith('C:')).length, 100);
    }
    // Attach 가 쓰지 않으면 필드의 Form/부모 칸은 생성자가 둔 0 그대로다.
    scene = sceneWith();
    scene.logic().construct(scene.fields, 250, 0, () => {});
    assert.deepEqual([get(scene.fields.bytes, 4), get(scene.fields.bytes, 8)], [0, 0]);
    // 부모 TYPE 이 표 크기(188)와 같으면 범위 오류이다. Wind 아처가 가상 모드 조회에서 TYPE 을 바꿔도 현재 TYPE 을 다시 확인한다.
    scene = sceneWith();
    scene.raw(250)[10] = 188;
    assert.throws(() => scene.logic().construct(scene.fields, 250, 0, scene.attach()), /부모 TYPE/);
    scene = sceneWith({ 4: 140 });
    const changing = { ...scene.hooks(), mode: () => { scene.raw(250)[10] = 188; return 0; } };
    assert.throws(() => scene.logic(changing).construct(scene.fields, 250, 0, scene.attach()), /현재 TYPE/);
    // 내용물 사거리: 현재 TYPE 의 flags2 0x10000/0x20000 어느 쪽이든 첫 내용물의 WORD(높은 바이트 포함)로 사거리를 조회한다.
    for (const [flags2, expected] of [[0x10000, true], [0x20000, true], [0x30000, true], [0x40000, false], [0, false]]) {
        scene = sceneWith({ 3: flags2 });
        put(scene.raw(251), 18, 0x0165, 2);
        const seen = [];
        const recording = { ...scene.hooks(), range: (type) => { seen.push(type); return 10; } };
        scene.logic(recording).construct(scene.fields, 250, 0, scene.attach());
        assert.deepEqual(seen, [expected ? 0x165 : 100], `flags2 ${flags2.toString(16)}`);
        assert.equal(scene.trace.includes('T:250:1:0:0'), expected);
    }
    // Wind 아처의 모드는 부호 없는 32비트로 곱한다(0xfffffffe 는 −2 가 아니라 4294967294 이다).
    scene = sceneWith({ 4: 140, 36: 1 });
    const unsignedMode = { ...scene.hooks(), mode: () => 0xfffffffe };
    scene.logic(unsignedMode).construct(scene.fields, 250, 0, scene.attach());
    const half = Math.fround(30);
    const angle = RawCursorRange.normalize(Math.fround(4294967294 * 45.0));
    const first = RawCursorRange.normalize(RawCursorRange.normalize(Math.fround(angle - half)));
    const last = RawCursorRange.normalize(RawCursorRange.normalize(Math.fround(half + angle)));
    assert.equal(F32(scene.fields.bytes, 0x1c4), first);
    assert.equal(F32(scene.fields.bytes, 0x1c8), last);
    assert.notEqual(first, 240);
});

test('RawCursorRange: 원형 프레임의 종료(Stop)·델타 float 저장·기한/닫힘 경계·void 건너뜀·Wind 상한·중심 합', () => {
    const f32 = (scene, offset) => F32(scene.fields.bytes, offset);
    const double = (scene, offset) => new DataView(scene.fields.bytes.buffer).getFloat64(offset, true);
    // 닫는 중 반경이 0.1 이하이면 종료한다: 부모 extra 의 8 비트가 켜져 있을 때만 현재 커서 토큰을 0 으로 지운다.
    for (const [extra, token] of [[8, 0], [4, 7]]) {
        const scene = sceneWith({ 7: floatToBits(0.05), 9: 1, 16: 7, 17: extra, 10: 1 });
        setTimes(scene, { now: 105, previous: 100 });
        frameOnce(scene);
        assert.equal(scene.trace, 'K:0');
        assert.equal(scene.cursor.cursorGump, token, `extra ${extra}`);
    }
    // 닫는 중 반경이 단정도 0.1 이면 종료(시각 필드 미갱신), 한 단위 크면 종료하지 않고 시각을 쓴다.
    let scene = sceneWith({ 7: floatToBits(Math.fround(0.1)), 9: 1, 10: 1 });
    setTimes(scene, { now: 105, previous: 100 });
    frameOnce(scene);
    assert.equal(scene.trace, 'K:0');
    assert.equal(double(scene, 0x1d0), 100);
    scene = sceneWith({ 7: floatToBits(Math.fround(0.1)) + 1, 9: 1, 10: 1 });
    setTimes(scene, { now: 105, previous: 105 });
    frameOnce(scene);
    assert.equal(scene.trace, '');
    // 성장·축소는 델타를 단정도로 좁힌 값으로 곱한다(좁히지 않으면 마지막 저장값이 달라진다).
    scene = sceneWith({ 7: floatToBits(0.5), 10: 1 });
    setTimes(scene, { now: 100.0959, previous: 100 });
    frameOnce(scene);
    assert.equal(f32(scene, 0x1b4), 1.938499927520752);
    scene = sceneWith({ 7: floatToBits(1.4299999475479126), 9: 1, 10: 1 });
    setTimes(scene, { now: 100.0411, previous: 100 });
    frameOnce(scene);
    assert.equal(f32(scene, 0x1b4), 0.19699996709823608);
    // 기한이 현재 시각과 같으면 닫지 않고 앞서 지났으면 닫힘 값 1 을 쓴다. 0 은 기한 없음이다.
    scene = sceneWith({ 10: 1 });
    setTimes(scene, { now: 105, previous: 105, deadline: 105 });
    frameOnce(scene);
    assert.equal(get(scene.fields.bytes, 0x1b8), 0);
    scene = sceneWith({ 10: 1 });
    setTimes(scene, { now: 105, previous: 105, deadline: 104 });
    frameOnce(scene);
    assert.equal(get(scene.fields.bytes, 0x1b8), 1);
    // void(4) 표시 객체는 다시 해제하지 않고, dead(2)는 진단만 하고 건너뛴다.
    scene = sceneWith({ 13: 4, 10: 0 });
    setTimes(scene, { now: 105, previous: 105 });
    frameOnce(scene);
    assert.ok(!scene.trace.includes('D:'));
    assert.ok(scene.trace.includes('V:50:'));
    scene = sceneWith({ 13: 2, 10: 0 });
    setTimes(scene, { now: 105, previous: 105 });
    frameOnce(scene);
    assert.equal(scene.trace, new Array(8).fill('Q:29').join(';'));
    // Wind 관측각의 시작이 360 이면 비활성(<360)이다: 시작 −1(비활성)과 같은 위치가 나온다.
    const traceOf = (start) => {
        const probe = sceneWith({ 40: floatToBits(start), 41: floatToBits(90), 42: floatToBits(120), 10: 0 });
        setTimes(probe, { now: 105.25, previous: 105.25 });
        frameOnce(probe);
        return probe.trace;
    };
    assert.equal(traceOf(360), traceOf(-1));
    assert.notEqual(traceOf(0), traceOf(-1));
    // 중심(255.99998474…)에 x=7.7e-6 을 더한 값은 단정도로 저장하면 256 이 되어 범위 밖(대체점)이다. 저장 전 합으로 판단하지 않는다.
    scene = sceneWith({ 37: 1, 7: floatToBits(Math.fround(7.7e-6)), 34: floatToBits(22), 10: 0 });
    setTimes(scene, { now: 10, previous: 10 });
    const edge = withWorld(scene, { center: (sid) => { scene.event('O', [sid]); return [255.99998474121094, 100]; } });
    frameOnce(scene, { ...scene.hooks(), world: edge.world });
    assert.equal(scene.trace, `D:50:0;O:250;V:50:${floatToBits(1)}:${floatToBits(1)}:0`);
    // 소멸: 현재 커서 토큰은 부호 없는 32비트 전체로 비교한다.
    scene = sceneWith({ 16: 0x80000001 });
    scene.logic().destroy(scene.fields, 0x80000001);
    assert.equal(scene.cursor.cursorGump, 0);
});

test('RawCursorRange: 직선 프레임의 float 중간 저장 순서·가시 경계·변형/dead/Run 개수·TYPE 범위', () => {
    const f = Math.fround;
    const raysInputs = { 36: 0, 8: 3, 13: 0, 10: 0 };
    // 사거리 20 을 3등분(단정도 6.6666665)한 간격과 성장 15 로 세 칸이 모두 보이는 장면의 전체 효과열을 단정도 저장 위치대로 다시 계산해 대조한다.
    let scene = sceneWith(raysInputs);
    setTimes(scene, { now: 100, created: 99, previous: 100 });
    frameOnce(scene);
    const type = scene.types[100];
    const [x, y] = [100, 110];
    const step = f(20 / 3);
    const grown = (100 - 99) * 15;
    const visible = f(grown);
    const phase = f(grown % step);
    const directions = [[0, -1], [1, 0], [0, 1], [-1, 0]];
    let expected = 'P:250';
    const animations = [];
    for (let side = 0; side < 4; side += 1) {
        const originX = f(x + -3 / 16);
        const originY = f(y + 5 / 11);
        const beginX = f(originX - type.hotspotX / 16);
        const beginY = f(originY - type.hotspotY / 11);
        expected += `;F:250:${side * 2};H:100:0:${side}`;
        for (let item = 0; item < 3; item += 1) {
            const product = item * step;
            if (!(product < visible)) continue;
            const sid = 50 + 3 * side + item;
            const distance = f(f(product) + phase);
            const point = [f(directions[side][0] * distance + beginX), f(directions[side][1] * distance + beginY)];
            expected += `;D:${sid}:0;V:${sid}:${floatToBits(point[0])}:${floatToBits(point[1])}:16`;
            animations.push([sid, (Math.trunc(distance * 2.0) % 4) + 2]);
        }
    }
    assert.equal(scene.trace, expected);
    for (const [sid, animation] of animations) assert.equal(get(scene.raw(sid), 36, 4), animation);
    // 곱이 가시 거리와 정확히 같은 칸은 그려지지 않는다(간격 5, 성장 10 × 0.5초 = 5).
    scene = sceneWith({ 36: 0, 8: 2, 34: floatToBits(10), 39: floatToBits(10) });
    setTimes(scene, { now: 100, created: 99.5, previous: 100 });
    frameOnce(scene);
    assert.equal(scene.trace.split(';').filter((event) => event.startsWith('V:')).length, 4);
    // 프레임 Run 조회는 현재 변형(0x1ec)을 그대로 전달한다.
    scene = sceneWith({ 36: 0, 8: 1, 38: 1 });
    setTimes(scene, { now: 100, created: 99, previous: 100 });
    const variants = [];
    frameOnce(scene, { ...scene.hooks(), frameRun: (typeNumber, variant) => { variants.push(variant); return { first: 0, count: 3 }; } });
    assert.ok(variants.length > 0 && variants.every((variant) => variant === 1));
    // Run 개수 0 은 보이는 칸에서 거부한다. 부모 TYPE 이 표 크기이면 범위 오류이다.
    scene = sceneWith({ 36: 0, 8: 1 });
    setTimes(scene, { now: 100, created: 99, previous: 100 });
    assert.throws(() => frameOnce(scene, { ...scene.hooks(), frameRun: () => ({ first: 0, count: 0 }) }), /Run 개수/);
    scene = sceneWith({ 36: 0, 8: 1 });
    scene.raw(250)[10] = 188;
    assert.throws(() => frameOnce(scene), /직선 TYPE/);
    // dead(2)만 켜진 표시 객체는 직선에서 다시 해제한다(void 4 만 건너뜀).
    scene = sceneWith({ 36: 0, 8: 1, 13: 2 });
    setTimes(scene, { now: 100, created: 99, previous: 100 });
    frameOnce(scene);
    assert.ok(scene.trace.includes('D:50:0'));
    scene = sceneWith({ 36: 0, 8: 1, 13: 4 });
    setTimes(scene, { now: 100, created: 99, previous: 100 });
    frameOnce(scene);
    assert.ok(!scene.trace.includes('D:'));
    // 직선 종료 요청(닫는 중)도 Stop 이며 부모 extra 8 일 때만 토큰을 지운다.
    scene = sceneWith({ 36: 0, 9: 1, 16: 7, 17: 8 });
    frameOnce(scene);
    assert.equal(scene.trace, 'K:0');
    assert.equal(scene.cursor.cursorGump, 0);
});

// 직선 프레임 한 번의 입력 묶음으로 장면을 만들고 실행한 뒤 단정도 저장 위치를 원문 소스대로 다시 계산한 효과열과 대조한다. (2026-11-15 WB04-E550)
function raysCase(input) {
    const f = Math.fround;
    const p = { offX: -3, offY: 5, hotX: 3, hotY: -5, now: 100, ...input };
    const scene = sceneWith({
        36: 0, 8: p.count, 13: 0, 10: 0, 34: floatToBits(p.range), 39: floatToBits(p.growth), 28: floatToBits(p.x), 29: floatToBits(p.y),
        47: floatToBits(p.scaleX), 48: floatToBits(p.scaleY), 49: p.offX >>> 0, 50: p.offY >>> 0, 45: p.hotX >>> 0, 46: p.hotY >>> 0
    });
    setTimes(scene, { now: p.now, created: p.created, previous: p.now });
    frameOnce(scene);
    const step = f(p.range / p.count);
    const grown = (p.now - p.created) * p.growth;
    const visible = f(grown);
    const phase = f(grown % step);
    const directions = [[0, -1], [1, 0], [0, 1], [-1, 0]];
    let expected = 'P:250';
    for (let side = 0; side < 4; side += 1) {
        const originX = f(p.x + p.offX / p.scaleX);
        const originY = f(p.y + p.offY / p.scaleY);
        const beginX = f(originX - p.hotX / p.scaleX);
        const beginY = f(originY - p.hotY / p.scaleY);
        expected += `;F:250:${side * 2};H:100:0:${side}`;
        for (let item = 0; item < p.count; item += 1) {
            const product = item * step;
            if (!(product < visible)) continue;
            const sid = 50 + p.count * side + item;
            const distance = f(f(product) + phase);
            let point = [f(directions[side][0] * distance + beginX), f(directions[side][1] * distance + beginY)];
            // 엄격한 좌표 제한이며 범위 밖이면 대체점(1, 1)이다.
            if (!(point[0] > 0 && point[0] < 256 && point[1] > 0 && point[1] < 256)) point = [1, 1];
            expected += `;D:${sid}:0;V:${sid}:${floatToBits(point[0])}:${floatToBits(point[1])}:16`;
        }
    }
    return { scene, expected };
}

test('RawCursorRange: 직선 프레임의 단계별 단정도 저장(간격·위상·원점·시작·거리·가시·좌표)을 구분하는 입력', () => {
    const base = { now: 100, x: 60, y: 70 };
    const cases = [
        // 시작 Y(beginY)·원점 Y/시작 X·간격·거리·위상·원점 X 를 각각 구분하는 입력들이다(탐색으로 고른 값).
        { ...base, range: 10, count: 3, growth: 13, created: 99.5, scaleX: 11, scaleY: 11 },
        { range: 11, count: 4, growth: Math.fround(13.7), now: 100, created: 99.363, x: Math.fround(60.3711), y: Math.fround(70.2917), scaleX: 13, scaleY: 13 },
        { range: 12, count: 5, growth: Math.fround(14.4), now: 100, created: 99.226, x: Math.fround(60.7422), y: Math.fround(70.5834), scaleX: 15, scaleY: 15 },
        { range: 13, count: 6, growth: Math.fround(15.1), now: 100, created: 99.089, x: Math.fround(61.1133), y: Math.fround(70.8751), scaleX: 11, scaleY: 17 },
        { range: 17, count: 5, growth: 13, now: 100, created: 98.541, x: Math.fround(62.5977), y: Math.fround(72.0419), scaleX: 13, scaleY: 17 },
        { range: 10, count: 6, growth: Math.fround(17.2), now: 100, created: 99.226, x: Math.fround(64.8243), y: Math.fround(73.7921), scaleX: 13, scaleY: 13 }
    ];
    for (const input of cases) {
        const { scene, expected } = raysCase(input);
        assert.equal(scene.trace, expected, JSON.stringify(input));
    }
    // 단정도로 저장한 좌표가 256 이 되는 경계(저장 전 값은 256 미만)는 범위 밖 대체점이다: 오른쪽·아래 방향이 걸린다.
    const edge = Math.fround(255.99998474121094);
    const clampCase = raysCase({ range: 10, count: 1, growth: 10, now: 100, created: 100 - 1.00000077, x: edge, y: edge, offX: 0, offY: 0, hotX: 0, hotY: 0, scaleX: 16, scaleY: 11 });
    assert.equal(clampCase.scene.trace, clampCase.expected);
    const inv = floatToBits(1);
    assert.ok(clampCase.scene.trace.includes(`V:51:${inv}:${inv}:16`));
    assert.ok(clampCase.scene.trace.includes(`V:52:${inv}:${inv}:16`));
    // 가시 거리는 단정도로 저장한 값과 비교한다: 곱 5 는 성장 5.0000001 의 단정도 값 5 와 같아 그려지지 않는다.
    const scene = sceneWith({ 36: 0, 8: 2, 34: floatToBits(10), 39: floatToBits(10) });
    setTimes(scene, { now: 100, created: 100 - 0.50000001, previous: 100 });
    frameOnce(scene);
    assert.equal(scene.trace.split(';').filter((event) => event.startsWith('V:')).length, 4);
});

// 실제 Host/Kernel 에 붙는 프로세스의 수명 장면이다: 부모 하나와 사정거리 객체를 만드는 가상 효과만 독립 입력이다. (2026-11-15 WB04-E550)
function processScene() {
    const pool = new SidPool(32768, { server: true });
    const types = Array.from({ length: 188 }, (_, number) => ({
        cost: 0, flags1: 0, flags2: 0, footX: 3, footY: 4, group: 0, hotspotX: 0, hotspotY: 0, ...([1, 2, 3, 6].includes(number) ? { constructorAddress: typeConstructorAddress(number) } : {})
    }));
    const destroy = new RawSquidDestroy(pool, new SquidUnpop(pool, new SquidHash(), new Uint8Array(65536)), types);
    const kernel = new Kernel();
    const state = new SquidProcessState();
    state.now = 10;
    const host = new SquidProcessHost(pool, types, kernel, destroy, state);
    const parent = pool.allocate();
    const raw = pool.allocated(parent);
    raw[10] = 100;
    raw[40] = 8;
    put(raw, 14, floatToBits(40));
    put(raw, 18, floatToBits(50));
    const events = [];
    const hooks = {
        world: {
            create: (type, flags) => { const sid = pool.allocate(2); events.push(['C', type, flags]); return sid; },
            detach: (sid) => { events.push(['D', sid]); pool.allocated(sid)[11] |= 4; },
            unpop: (sid) => { events.push(['U', sid]); pool.allocated(sid)[11] &= ~4; },
            destroy: (sid) => { events.push(['X', sid]); pool.allocated(sid)[11] |= 2; },
            position: (sid, x, y, flags) => { events.push(['V', sid, x, y, flags]); put(pool.allocated(sid), 14, floatToBits(x)); put(pool.allocated(sid), 18, floatToBits(y)); },
            center: () => [40, 50]
        },
        range: () => 20,
        firstContent: () => parent,
        mode: () => 0,
        cannonDirection: () => true,
        footprint: () => ({}),
        directionFrame: () => 0,
        firingOffset: () => [0, 0],
        frameRun: () => ({ first: 0, count: 2 }),
        diagnostic: () => {}
    };
    const cursor = new CursorDropState();
    const coordinates = new CursorManaGlobals();
    const target = { scaleX: 16, scaleY: 11, windArcherType: 140, windHalfAngle: 30 };
    const globals = new CursorRangeGlobals(coordinates, target);
    const logic = new RawCursorRange(pool, types, state, globals, cursor, hooks);
    return { pool, types, kernel, state, host, parent, events, hooks, cursor, globals, logic };
}

test('CursorRangeProcess: 실제 Host/Kernel 부착·프레임·통지·종료·소멸과 수명 연결', () => {
    const s = processScene();
    const kills = [];
    const original = s.host.kill.bind(s.host);
    s.host.kill = (process, flags) => { kills.push(flags >>> 0); return original(process, flags); };
    const lifecycle = {};
    bindCursorRangeLifecycle(lifecycle, s.host, s.logic);
    const token = lifecycle.createRange(s.parent, 0);
    const process = s.kernel.get(token);
    assert.ok(process instanceof CursorRangeProcess);
    assert.equal(process.token, token);
    assert.equal(process.pid, token);
    // 원형 모드로 8개 표시 객체(타입 104·flags2)를 만들고 부착 뒤 필드에 Form/부모 번호를 남긴다.
    assert.deepEqual(s.events.filter((event) => event[0] === 'C').map((event) => event.join(':')), new Array(8).fill('C:104:2'));
    assert.equal(get(process.fields.bytes, 12), 1);
    assert.equal(get(process.fields.bytes, 0x1a4), 8);
    assert.equal(get(process.fields.bytes, 8), s.parent);
    assert.equal(get(process.fields.bytes, 4), process.form);
    assert.equal(s.host.isRegistered(token, process), true);
    assert.equal(s.pool.slot(process.form)[10], 51);
    // 첫 프레임은 반경을 키우고 8개를 부모 둘레에 놓는다.
    s.state.now = 10.1;
    s.events.length = 0;
    process.runFrame();
    assert.equal(s.events.filter((event) => event[0] === 'V').length, 8);
    assert.ok(F32(process.fields.bytes, 0x1b4) > 0);
    // 이벤트 3 은 종료 표식이며 다음 프레임에서 반경이 0.1 이하로 줄면 실제 Kill 을 호출한다(커서 토큰은 부모 extra 8 이어서 지운다).
    s.cursor.cursorGump = token;
    process.notify(3, 0, 0);
    assert.equal(get(process.fields.bytes, 0x1b8), 1);
    for (let step = 0; step < 6 && s.kernel.get(token); step += 1) { s.state.now += 0.05; process.runFrame(); }
    assert.deepEqual(kills, [0]);
    assert.equal(s.kernel.get(token), null);
    assert.equal(s.cursor.cursorGump, 0);
    assert.equal(s.events.filter((event) => event[0] === 'X').length, 8);
});

test('CursorRangeProcess: Host/풀 불일치·Form 폭·부착 실패를 거부한다', () => {
    const s = processScene();
    const other = new SidPool(32768, { server: true });
    const otherHost = new SquidProcessHost(other, s.types, new Kernel(), new RawSquidDestroy(other, new SquidUnpop(other, new SquidHash(), new Uint8Array(65536)), s.types), new SquidProcessState());
    assert.throws(() => createCursorRangeProcess(otherHost, s.logic, s.parent, 0), TypeError);
    assert.throws(() => bindCursorRangeLifecycle({}, otherHost, s.logic), TypeError);
    const hooks = {};
    bindCursorRangeLifecycle(hooks, s.host, s.logic);
    assert.throws(() => hooks.createRange(s.parent, 65536), RangeError);
    // 부모가 dead 이면 부착이 거부되어(null) 생성이 실패한다.
    s.pool.allocated(s.parent)[11] |= 2;
    assert.throws(() => hooks.createRange(s.parent, 0), /Attach 실패/);
});

test('CursorRangeProcess: 부모 extra 가 8 이 아니면 종료가 토큰을 지우지 않고 소멸이 현재 토큰만 지운다', () => {
    const s = processScene();
    s.pool.allocated(s.parent)[40] = 4;
    const lifecycle = {};
    bindCursorRangeLifecycle(lifecycle, s.host, s.logic);
    const token = lifecycle.createRange(s.parent, 0);
    const process = s.kernel.get(token);
    s.cursor.cursorGump = token;
    process.notify(3, 0, 0);
    s.state.now = 10.05;
    process.runFrame();
    assert.equal(s.kernel.get(token), null);
    // 종료(Stop)는 extra 8 이 아니어서 토큰을 남기지만 소멸자가 같은 번호이면 지운다.
    assert.equal(s.cursor.cursorGump, 0);
    // 다른 토큰을 가진 커서는 소멸이 건드리지 않는다.
    const other = lifecycle.createRange(s.parent, 0);
    s.cursor.cursorGump = other + 1;
    s.kernel.get(other).notify(3, 0, 0);
    s.state.now = 10.1;
    s.kernel.get(other).runFrame();
    assert.equal(s.kernel.get(other), null);
    assert.equal(s.cursor.cursorGump, other + 1);
});
