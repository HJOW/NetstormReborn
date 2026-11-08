// 번개 생성/소멸 raw-lightning-lifetime.js(RawLightningLifetime·createLightning·createLightningAtScreen)를 실제 PE 관측 fixture 로 대조한다.
//  - lightning-lifetime-x86.tsv 의 기본 027f 55입력(비교 037f 55행은 중복 가산하지 않고 건너뜀): 열 = 이름, 정밀도, 인수 9개(warm 표 여부·표시 목록 용량·구멍·부모·깊이·부착 중 변경(1 깊이 변경/2 현재 부모 변경)·파일명 여부·flags), 입력 RGBX 256개,
//    생성 직후 의미 88바이트·경로 64바이트·120입자 24바이트 zero·공유 색 표 11개(번개 표: 파랑 → 흰색 8 + 흰색 → 회색 3)·표시 슬롯·확보/부착 요청 순서, 반납 뒤 의미 상태·슬롯·반납 순서(경로 → 입자 → 등록 제거 → flags bit 0 객체).
//  - cpppj 계약: wrapper 의 dead/void 표적 구별·화면 카메라 감김·경계 미연결/다른 풀/직접 좌표 경로의 수명(life 0 일 때만 기본값 보존) 거부. 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E186: 번개 수명 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import fs from 'node:fs';
import { REPO_ROOT } from './helpers.mjs';
import { put, get } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { RawRenderRegistry } from '../www/core/raw-construction-lifetime.js';
import { RawLightningPath } from '../www/core/raw-lightning-path.js';
import { buildLightningPalette } from '../www/core/particle-gradient.js';
import { createLightningFrameFields } from '../www/core/raw-lightning-frame.js';
import { LightningCreateStatus, LightningRelease, RawLightningLifetime, createLightning, createLightningAtScreen } from '../www/core/raw-lightning-lifetime.js';

// 실제 PE 관측 fixture 이다. (2026-11-09 WB04-E186)
const rows = fs.readFileSync(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'lightning-lifetime-x86.tsv'), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && line[0] !== '#').map((line) => line.split('\t'));

// 예외가 나는지 본다. (2026-11-09 WB04-E186)
function throwsOf(action) {
    try { action(); } catch { return true; }
    return false;
}

// fixture 의 실제 입력/출력 비트만 읽는다. (2026-11-09 WB04-E186)
const decode = (text) => Uint8Array.from(text.match(/../g) ?? [], (pair) => parseInt(pair, 16));

// 지정 DWORD/QWORD 를 원문 little endian 으로 끝에 붙인다(지정 폭만 저장해 signed 상위 확장을 막는다). (2026-11-09 WB04-E186)
function append(bytes, value, width = 4) {
    let remaining = BigInt.asUintN(64, BigInt(value));
    for (let index = 0; index < width; index += 1) {
        bytes.push(Number(remaining & 0xffn));
        remaining >>= 8n;
    }
}

// float/double 의 원문 비트이다. (2026-11-09 WB04-E186)
const floatBits = (value) => {
    const view = new DataView(new ArrayBuffer(4));
    view.setFloat32(0, value, true);
    return view.getUint32(0, true);
};
const doubleBits = (value) => {
    const view = new DataView(new ArrayBuffer(8));
    view.setFloat64(0, value, true);
    return view.getBigUint64(0, true);
};

// 실제 88BYTE 의미 상태와 같은 순서로 현재 필드를 직렬화한다. (2026-11-09 WB04-E186)
function fieldBytes(fields, allocated) {
    const bytes = [];
    const reg = fields.registration;
    append(bytes, reg.form >>> 0);
    append(bytes, reg.parent >>> 0);
    for (const value of [reg.left, reg.top, reg.right, reg.bottom]) append(bytes, floatBits(value));
    append(bytes, reg.slot >>> 0);
    append(bytes, reg.zOrder >>> 0);
    append(bytes, allocated);
    append(bytes, fields.particles.total >>> 0);
    append(bytes, doubleBits(fields.lastTime), 8);
    append(bytes, doubleBits(fields.elapsed), 8);
    append(bytes, floatBits(fields.delta));
    append(bytes, floatBits(fields.scaledDelta));
    append(bytes, fields.first ? 1 : 0);
    append(bytes, fields.palette);
    append(bytes, fields.firstLive >>> 0);
    append(bytes, fields.pathAllocated ? 2 : 0);
    append(bytes, floatBits(fields.jitter));
    append(bytes, fields.sound ? 3 : 0);
    return Uint8Array.from(bytes);
}

// 공통 슬롯의 빈 칸과 기존 토큰을 모두 원문 순서로 기록한다. (2026-11-09 WB04-E186)
const slotsOf = (registry) => (registry.slots.length ? registry.slots.join(',') : '-');

// 두 바이트 배열의 첫 불일치 위치를 입력 번호와 함께 보고한다. (2026-11-09 WB04-E186)
function compare(actual, expected, row, field) {
    assert.equal(actual.length, expected.length, `빔 수명 행 ${row} ${field} 폭`);
    for (let index = 0; index < actual.length; index += 1) assert.equal(actual[index], expected[index], `빔 수명 행 ${row} ${field} 바이트 ${index}`);
}

test('번개 수명: 기본 027f 55입력의 생성·공유 색 표·경로/입자·표시 등록/반납 순서를 실제 PE 와 대조한다', () => {
    let count = 0;
    let extended = 0;
    // 비교 64비트 행은 보존하지만 기본 고유 입력 수에 중복 가산하지 않는다.
    rows.forEach((row, current) => {
        assert.equal(row.length, 13);
        if (Number(row[1]) === 0x037f) { extended += 1; return; }
        assert.equal(Number(row[1]), 0x027f);
        const args = row[2].split(',');
        assert.equal(args.length, 9);
        const warm = Number(args[1]) !== 0;
        const sound = Number(args[7]) !== 0;
        const capacity = Number(args[2]);
        const hole = Number(args[3]);
        const parent = Number(args[4]);
        const depth = Number(args[5]);
        const mutate = Number(args[6]);
        const pool = new SidPool(32768, { server: false });
        // 부착 전/후 부모와 표적을 원문 실험과 같은 raw 입력으로 구성한다.
        for (const [id, type, deep] of [[0, 149, 255], [50, 148, depth], [51, 144, 127], [52, 149, 0]]) {
            const raw = pool.physical(id);
            raw.fill(0);
            raw[6] = 1;
            raw[10] = type;
            raw[35] = deep & 0xff;
            const x = id === 0 ? 1.5 : id === 50 ? 20.5 : id === 51 ? -7.75 : 40.125;
            const y = id === 0 ? 1.5 : id === 50 ? 21.25 : id === 51 ? 8.25 : 35.375;
            put(raw, 14, floatBits(x));
            put(raw, 18, floatBits(y));
        }
        const initial = [];
        // 목록 검색/확장 결과는 제품 몸체가 결정한다.
        for (let slot = 0; slot < capacity; slot += 1) initial.push(slot === hole ? 0 : 100 + slot);
        const events = [];
        const registry = new RawRenderRegistry(pool, {
            reserve: (bytes) => { events.push(`N:${bytes}`); return true; },
            releaseArray: () => { events.push('F:R'); },
            attach: (fields, type, target, flags) => {
                events.push(`B:${type}:${target}:0:${flags}:${fields.slot}:${fields.zOrder}`);
                fields.form = 7;
                fields.parent = mutate === 2 ? 51 : target;
                if (mutate === 1) pool.physical(target)[35] = 128;
            }
        }, initial);
        const random = new GameRandom(1);
        const lightningPath = new RawLightningPath(pool, random, { scaleX: 16, scaleY: 11, vanderType: 148, tallType: 149, manOWarType: 144 }, (sid) => {
            const raw = pool.slot(sid);
            const foot = raw[10] === 144 ? 2 : 3;
            const view = new DataView(new ArrayBuffer(8));
            view.setUint32(0, get(raw, 14), true);
            view.setUint32(4, get(raw, 18), true);
            return [Math.fround(view.getFloat32(0, true) - foot * 0.5), Math.fround(view.getFloat32(4, true) - foot * 0.5)];
        });
        const state = { palette: warm ? 1 : 0 };
        let colors = [];
        // warm 표는 원문과 같은 11개 외부 입력이다.
        if (warm) for (let color = 31; color < 42; color += 1) colors.push(color);
        const rgbx = decode(row[3]);
        assert.equal(rgbx.length, 1024);
        // 입력 RGBX 는 재계산 없이 DWORD 전체를 읽는다.
        const palette = Uint32Array.from({ length: 256 }, (_, color) => get(rgbx, color * 4));
        const lifetime = new RawLightningLifetime(pool, registry, lightningPath, state, {
            reserve: (bytes) => { events.push(`N:${bytes}`); return true; },
            createPalette: () => 1,
            buildPalette: (token) => { assert.equal(token, 1); events.push('N:44'); colors = buildLightningPalette(palette); }
        });
        const fields = createLightningFrameFields();
        const poisoned = new DataView(new ArrayBuffer(4));
        poisoned.setUint32(0, 0xa5a5a5a5, true);
        fields.delta = poisoned.getFloat32(0, true);
        fields.scaledDelta = fields.delta;
        fields.path.bytes.fill(0xa5);
        lifetime.initialize(fields, 1, parent, 52, sound ? 'shortZap.wav' : null);
        const allocated = fields.particles.values.length;
        compare(fieldBytes(fields, allocated), decode(row[4]), current, '생성 상태');
        compare(fields.path.bytes, decode(row[5]), current, '경로');
        // 생성 직후 모든 24BYTE 필드는 0 이어야 하며 용량 120 전체가 대상이다.
        for (const particle of fields.particles.values) assert.ok(particle.x === 0 && particle.y === 0 && particle.strength === 0 && particle.jitterWeight === 0 && particle.life === 0 && particle.previous === 0);
        assert.equal(decode(row[6]).length, allocated * 24);
        assert.ok(decode(row[6]).every((byte) => byte === 0));
        assert.equal(slotsOf(registry), row[8], `표시 슬롯 행 ${current}`);
        assert.equal(events.join(';'), row[9], `생성 요청 순서 행 ${current}`);
        assert.equal(state.palette, 1);
        const colorBytes = [];
        // 공유 색 표 11개 전체를 비교한다(인접 할당 DWORD 는 이 빔 Draw 에서 읽지 않음).
        for (const color of colors) append(colorBytes, color);
        compare(Uint8Array.from(colorBytes), decode(row[7]), current, '팔레트');
        events.length = 0;
        lifetime.destroy(fields, Number(args[8]), (released) => {
            const token = registry.slots[fields.registration.slot];
            assert.equal(token, released === LightningRelease.Object ? 0 : 1);
            events.push(released === LightningRelease.Path ? 'F:L' : released === LightningRelease.Particles ? 'F:P' : 'F:O');
        });
        assert.equal(fields.particles.values.length, 0);
        compare(fieldBytes(fields, allocated), decode(row[10]), current, '반납 상태');
        assert.equal(slotsOf(registry), row[11], `반납 슬롯 행 ${current}`);
        assert.equal(events.join(';'), row[12], `반납 순서 행 ${current}`);
        assert.equal(state.palette, 1);
        count += 1;
    });
    assert.equal(count, 55);
    assert.equal(extended, 55);
});

test('번개 wrapper: dead/void 표적은 raw 주소 SID 로 구별하고 생존 표적만 생성하며 화면 카메라는 32비트로 감긴다', () => {
    const pool = new SidPool(24000, { server: false });
    for (let sid = 5; sid <= 52; sid += 1) pool.allocate(2);
    pool.allocated(51)[11] = 2;
    pool.allocated(52)[11] = 0;
    const calls = [];
    const create = (parent, target, sound) => { calls.push([parent, target, sound]); return { token: 1 }; };
    assert.deepEqual(createLightning(pool, 50, 51, 'shortZap.wav', create), { status: LightningCreateStatus.TargetUnavailable, skippedTarget: 51, process: null });
    assert.equal(calls.length, 0);
    const created = createLightning(pool, 50, 52, null, create);
    assert.ok(created.status === LightningCreateStatus.Created && created.process.token === 1 && calls.length === 1 && calls[0][2] === null);
    assert.equal(createLightning(pool, 50, 52, null, () => null).status, LightningCreateStatus.AllocationFailed);
    assert.ok(throwsOf(() => createLightning(pool, 50, 52, null, undefined)) && throwsOf(() => createLightning(pool, 50, 30000, null, create)) && throwsOf(() => createLightning({}, 50, 52, null, create)));
    const seen = [];
    createLightningAtScreen(50, [2147483647, -5], [10, 20], 1.5, 2, 7, (parent, from, to, life, sound) => { seen.push([parent, from, to, life, sound]); return null; });
    assert.deepEqual(seen[0], [50, [-2147483647, 2], [12, 27], 1.5, 'shortZap.wav']);
    assert.ok(throwsOf(() => createLightningAtScreen(50, [0, 0], [0, 0], 1, 0, 0, undefined)));
});

test('번개 직접 좌표 생성: life 가 0 일 때만 경로 기본 수명을 보존하고 생성자는 경계/다른 풀을 거부한다', () => {
    const pool = new SidPool(24000, { server: false });
    for (let sid = 5; sid <= 50; sid += 1) pool.allocate(2);
    pool.allocated(50).fill(0);
    pool.allocated(50)[6] = 1;
    const registry = new RawRenderRegistry(pool, { reserve: () => true, releaseArray: () => {}, attach: (fields, type, parent) => { fields.form = 7; fields.parent = parent; } });
    const random = new GameRandom(1);
    const lightningPath = new RawLightningPath(pool, random, { scaleX: 16, scaleY: 11, vanderType: 148, tallType: 149, manOWarType: 144 }, () => [0, 0]);
    const lifetime = new RawLightningLifetime(pool, registry, lightningPath, { palette: 1 }, { reserve: () => true });
    const keep = createLightningFrameFields();
    lifetime.initializePoints(keep, 1, 50, [10, 20], [40, 60], 0, null);
    const defaultLife = Array.from(keep.path.bytes.subarray(32, 36));
    const custom = createLightningFrameFields();
    const other = new RawRenderRegistry(pool, { reserve: () => true, releaseArray: () => {}, attach: (fields, type, parent) => { fields.form = 7; fields.parent = parent; } });
    new RawLightningLifetime(pool, other, lightningPath, { palette: 1 }, { reserve: () => true }).initializePoints(custom, 1, 50, [10, 20], [40, 60], 2.5, 'shortZap.wav');
    const view = new DataView(new ArrayBuffer(4));
    view.setFloat32(0, 2.5, true);
    assert.deepEqual(Array.from(custom.path.bytes.subarray(32, 36)), [view.getUint8(0), view.getUint8(1), view.getUint8(2), view.getUint8(3)]);
    assert.ok(custom.sound === 'shortZap.wav' && keep.sound === null && defaultLife.length === 4);
    assert.ok(throwsOf(() => new RawLightningLifetime({}, registry, lightningPath, { palette: 1 }, { reserve: () => true })));
    assert.ok(throwsOf(() => new RawLightningLifetime(pool, { pool: new SidPool(24000, { server: false }) }, lightningPath, { palette: 1 }, { reserve: () => true })));
    assert.ok(throwsOf(() => new RawLightningLifetime(pool, registry, lightningPath, { palette: 1 }, {})));
    assert.ok(throwsOf(() => lifetime.destroy(createLightningFrameFields(), 0, undefined)));
    // 첫 공유 색이 필요한데 구성 경계가 없으면 앞선 등록 뒤 거부한다.
    assert.ok(throwsOf(() => new RawLightningLifetime(pool, registry, lightningPath, { palette: 0 }, { reserve: () => true }).initializePoints(createLightningFrameFields(), 9, 50, [0, 0], [1, 1], 0, null)));
});
