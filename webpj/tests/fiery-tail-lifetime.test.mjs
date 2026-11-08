// 불꽃 꼬리/건설 마나 생성·소멸 raw-fiery-tail-lifetime.js(RawFieryTailLifetime)를 실제 PE 관측 fixture 로 대조한다.
//  - fierytail-lifetime-x86.tsv 기본 027f 31입력(비교 037f 31행은 중복 가산하지 않고 건너뜀): 열 = 이름, 정밀도, 인수 8개(팔레트 파일명·warm 표 여부·표시 목록 용량·구멍·부모·깊이·부착 중 변경(1 깊이 변경/2 현재 부모 변경)·flags),
//    입력 RGBX 256개, 생성 직후 의미 88바이트·120입자 24바이트 zero·공유 색 표 11개(불꽃 표: 어두운 빨강 → 주황 8 + 주황 → 밝은색 3)·표시 슬롯·확보/부착 요청 순서, 반납 뒤 의미 상태·슬롯·반납 순서(입자 → 등록 제거 → flags bit 0 객체).
//  - streak-lifetime-x86.tsv 기본 027f 4입력(비교 037f 4행은 건너뜀): 건설 마나(process 12) 생성자의 의미 88바이트 — 수명 2·offset −8·깊이 −20, 빔과 같은 공유 색 표를 빌리며 자체 palette 는 건드리지 않음.
//  - cpppj 계약: 현재 TYPE 전역을 등록/확보 콜백 전에 읽어 부착하고 생성마다 다시 읽음, 두 TYPE 전역(GameAuxState)의 초기값과 일반 월드 준비 뒤 장기 보존, 경계 미연결/다른 풀 거부. 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E187: 불꽃 꼬리 수명 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import fs from 'node:fs';
import { REPO_ROOT } from './helpers.mjs';
import { put, get } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawRenderRegistry } from '../www/core/raw-construction-lifetime.js';
import { GameAuxState } from '../www/core/game-aux-state.js';
import { buildFieryTailPalette } from '../www/core/particle-gradient.js';
import { createFieryTailFrameFields } from '../www/core/raw-fiery-tail-frame.js';
import { FieryTailRelease, FieryTailVariant, RawFieryTailLifetime } from '../www/core/raw-fiery-tail-lifetime.js';

// 실제 PE 관측 fixture 이다(헤더 `#` 행 제외). (2026-11-09 WB04-E187)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
function rowsOf(name) {
    return fs.readFileSync(path.join(FIXTURES, name), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && line[0] !== '#').map((line) => line.split('\t'));
}

// fixture 의 hex 를 원문 바이트로 복원한다. (2026-11-09 WB04-E187)
const decode = (text) => Uint8Array.from(text.match(/../g) ?? [], (pair) => parseInt(pair, 16));

// 예외가 나는지 본다. (2026-11-09 WB04-E187)
function throwsOf(action) {
    try { action(); } catch { return true; }
    return false;
}

// float/double 의 원문 비트이다. (2026-11-09 WB04-E187)
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

// 지정 DWORD/QWORD 를 원문 little endian 으로 끝에 붙인다(지정 폭만 저장해 signed 상위 확장을 막는다). (2026-11-09 WB04-E187)
function append(bytes, value, width = 4) {
    let remaining = BigInt.asUintN(64, BigInt(value));
    for (let index = 0; index < width; index += 1) {
        bytes.push(Number(remaining & 0xffn));
        remaining >>= 8n;
    }
}

// 실제 88BYTE 의미 상태와 같은 순서로 현재 필드를 직렬화한다(반납 뒤 capacity 는 할당 시점 값). (2026-11-09 WB04-E187)
function fieldBytes(fields, allocated) {
    const bytes = [];
    const reg = fields.registration;
    append(bytes, reg.form >>> 0);
    append(bytes, reg.parent >>> 0);
    for (const value of [reg.left, reg.top, reg.right, reg.bottom]) append(bytes, floatBits(value));
    append(bytes, reg.slot >>> 0);
    append(bytes, reg.zOrder >>> 0);
    append(bytes, allocated);
    append(bytes, fields.total >>> 0);
    append(bytes, doubleBits(fields.lastTime), 8);
    append(bytes, doubleBits(fields.elapsed), 8);
    append(bytes, floatBits(fields.delta));
    append(bytes, floatBits(fields.scaledDelta));
    append(bytes, fields.first ? 1 : 0);
    append(bytes, fields.palette >>> 0);
    append(bytes, fields.maximumLife >>> 0);
    append(bytes, floatBits(fields.previousX));
    append(bytes, floatBits(fields.previousY));
    append(bytes, fields.offset >>> 0);
    return Uint8Array.from(bytes);
}

// 용량 전체 입자를 실제 24바이트 순서로 직렬화한다. (2026-11-09 WB04-E187)
function particleBytes(fields) {
    const bytes = [];
    for (const p of fields.particles) {
        append(bytes, floatBits(p.x));
        append(bytes, floatBits(p.y));
        append(bytes, floatBits(p.velocityX));
        append(bytes, floatBits(p.velocityY));
        append(bytes, floatBits(p.life));
        append(bytes, p.spare >>> 0);
    }
    return Uint8Array.from(bytes);
}

// 현재 표시 슬롯의 빈 칸과 기존 토큰도 원문 순서로 기록한다. (2026-11-09 WB04-E187)
const slotsOf = (registry) => (registry.slots.length ? registry.slots.join(',') : '-');

// 두 바이트 배열의 첫 불일치 위치를 입력 번호와 함께 보고한다. (2026-11-09 WB04-E187)
function compare(actual, expected, row, field) {
    assert.equal(actual.length, expected.length, `불꽃 꼬리 수명 행 ${row} ${field} 폭`);
    for (let index = 0; index < actual.length; index += 1) assert.equal(actual[index], expected[index], `불꽃 꼬리 수명 행 ${row} ${field} 바이트 ${index}`);
}

// 단정도 float 로 0xa5a5a5a5 비트의 독극물 값이다(원본이 쓰지 않는 delta/scaledDelta 보존 확인용). (2026-11-09 WB04-E187)
const POISON = (() => {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, 0xa5a5a5a5, true);
    return view.getFloat32(0, true);
})();

test('불꽃 꼬리 수명: 기본 027f 31입력의 생성·120입자·공유 색 표·표시 등록/반납 순서를 실제 PE 와 대조한다', () => {
    let count = 0;
    let extended = 0;
    // 비교 64비트 행은 보존하지만 기본 고유 입력 수에 중복 가산하지 않는다.
    rowsOf('fierytail-lifetime-x86.tsv').forEach((row, current) => {
        assert.equal(row.length, 12);
        if (Number(row[1]) === 0x037f) { extended += 1; return; }
        assert.equal(Number(row[1]), 0x027f);
        const args = row[2].split(',');
        assert.equal(args.length, 8);
        const warm = Number(args[1]) !== 0;
        const capacity = Number(args[2]);
        const hole = Number(args[3]);
        const parent = Number(args[4]);
        const depth = Number(args[5]);
        const mutate = Number(args[6]);
        const flags = Number(args[7]) >>> 0;
        const pool = new SidPool(32768, { server: false });
        // 슬롯 0/50/51 의 깊이만 원문 실험과 같은 raw 입력으로 구성한다.
        pool.physical(0)[35] = 255;
        pool.physical(50)[35] = depth & 0xff;
        pool.physical(51)[35] = 127;
        const initial = [];
        // 초기 목록만 입력으로 구성한다. 검색/확장은 제품 몸체가 수행한다.
        for (let slot = 0; slot < capacity; slot += 1) initial.push(slot === hole ? 0 : 100 + slot);
        const events = [];
        const registry = new RawRenderRegistry(pool, {
            reserve: (bytes) => { events.push(`N:${bytes}`); return true; },
            releaseArray: () => { events.push('F:R'); },
            attach: (fields, type, target, attachFlags) => {
                events.push(`B:${type}:${target}:0:${attachFlags}:${fields.slot}:${fields.zOrder}`);
                fields.form = 7;
                fields.parent = mutate === 2 ? 51 : target;
                if (mutate === 1) pool.physical(50)[35] = 128;
            }
        }, initial);
        const state = { palette: warm ? 1 : 0 };
        let colors = [];
        // 이미 존재하는 11색은 원본 warm 실험과 같은 외부 입력이다.
        if (warm) for (let color = 31; color < 42; color += 1) colors.push(color);
        const rgbx = decode(row[3]);
        assert.equal(rgbx.length, 1024);
        // 입력 RGBX 는 재계산 없이 DWORD 전체를 읽는다.
        const palette = Uint32Array.from({ length: 256 }, (_, color) => get(rgbx, color * 4));
        const lifetime = new RawFieryTailLifetime(pool, registry, state, {
            reserve: (bytes) => { events.push(`N:${bytes}`); return true; },
            createPalette: () => 1,
            buildPalette: (token) => { assert.equal(token, 1); events.push('N:44'); colors = buildFieryTailPalette(palette); }
        });
        const fields = createFieryTailFrameFields();
        fields.delta = POISON;
        fields.scaledDelta = POISON;
        lifetime.initialize(fields, 1, parent);
        const allocated = fields.particles.length;
        compare(fieldBytes(fields, allocated), decode(row[4]), current, '생성 상태');
        compare(particleBytes(fields), decode(row[5]), current, '120입자');
        assert.equal(slotsOf(registry), row[7], `표시 슬롯 행 ${current}`);
        assert.equal(events.join(';'), row[8], `생성 요청 순서 행 ${current}`);
        assert.equal(state.palette, 1);
        const colorBytes = [];
        // 공유 색 표 11개 전체를 비교한다(인접 DWORD 를 추가하지 않음).
        for (const color of colors) append(colorBytes, color);
        compare(Uint8Array.from(colorBytes), decode(row[6]), current, '공유 색 표');
        events.length = 0;
        lifetime.destroy(fields, flags, (released) => {
            const token = registry.slots[fields.registration.slot];
            assert.equal(token, released === FieryTailRelease.Object ? 0 : 1);
            events.push(released === FieryTailRelease.Particles ? 'F:P' : 'F:O');
        });
        assert.equal(fields.particles.length, 0);
        compare(fieldBytes(fields, allocated), decode(row[9]), current, '반납 상태');
        assert.equal(slotsOf(registry), row[10], `반납 슬롯 행 ${current}`);
        assert.equal(events.join(';'), row[11], `반납 순서 행 ${current}`);
        assert.equal(state.palette, 1);
        count += 1;
    });
    assert.equal(count, 31);
    assert.equal(extended, 31);
});

test('건설 마나(Streak) 생성자: 기본 027f 4입력의 의미 필드를 실제 PE 와 대조하고 빔 공유 색 표만 빌린다', () => {
    let count = 0;
    let extended = 0;
    // 격리 원문이 기록한 8개 중 기본 정밀도만 완료로 센다(비교 정밀도 4행은 같은 입력의 중복 관측).
    for (const row of rowsOf('streak-lifetime-x86.tsv')) {
        assert.equal(row.length, 4);
        if (Number(row[2]) === 0x37f) { extended += 1; continue; }
        assert.equal(Number(row[2]), 0x27f);
        const parent = Number(row[0]);
        const mutate = Number(row[1]) !== 0;
        const pool = new SidPool(32768, { server: false });
        pool.physical(50)[35] = 255;
        pool.physical(51)[35] = 127;
        const initial = [];
        // 원본과 같은 빈 칸 20 만 입력한다.
        for (let index = 0; index < 40; index += 1) initial.push(index === 20 ? 0 : 100 + index);
        const registry = new RawRenderRegistry(pool, {
            reserve: () => true,
            releaseArray: () => {},
            attach: (fields, type, target, flags) => {
                assert.ok(type === 12 && target === parent && flags === 16);
                fields.form = 7;
                fields.parent = mutate ? 51 : parent;
            }
        }, initial);
        const beamPalette = { value: 1 };
        // 불꽃 자체 소유 999 는 건드리지 않아야 한다.
        const state = { palette: 999, sharedPalette: beamPalette };
        const lifetime = new RawFieryTailLifetime(pool, registry, state, { reserve: (bytes) => { assert.equal(bytes, 2880); return true; } }, FieryTailVariant.Streak);
        const fields = createFieryTailFrameFields();
        fields.delta = POISON;
        fields.scaledDelta = POISON;
        lifetime.initialize(fields, 1, parent);
        compare(fieldBytes(fields, fields.particles.length), decode(row[3]), count, '건설 마나 생성 상태');
        assert.ok(beamPalette.value === 1 && state.palette === 999 && fields.particles.length === 120);
        count += 1;
    }
    assert.equal(count, 4);
    assert.equal(extended, 4);
});

test('불꽃 꼬리 TYPE: 등록/확보 콜백보다 먼저 읽은 값으로 부착하고 생성마다 현재 전역을 다시 읽는다', () => {
    // 생성자 부류(Fiery/Streak)와 현재 TYPE DWORD 가 서로 독립임을 두 변종 모두에서 확인한다.
    for (const variant of [FieryTailVariant.Fiery, FieryTailVariant.Streak]) {
        const pool = new SidPool(32768, { server: false });
        let type = 56;
        let attached = 0;
        const registry = new RawRenderRegistry(pool, {
            reserve: () => { type = 57; return true; },
            releaseArray: () => {},
            attach: (fields, current, parent, flags) => { assert.ok(parent === 50 && flags === 16 && type === 57); attached = current; fields.parent = 50; fields.form = 7; }
        });
        const state = { palette: 1, sharedProcessType: () => type };
        const body = new RawFieryTailLifetime(pool, registry, state, { reserve: (bytes) => { assert.equal(bytes, 2880); type = 58; return true; } }, variant);
        const fields = createFieryTailFrameFields();
        body.initialize(fields, 1, 50);
        assert.ok(attached === 56 && type === 58 && fields.particles.length === 120 && fields.maximumLife === (variant === FieryTailVariant.Streak ? 2 : 4));
        body.destroy(fields, 1, () => {});
    }
    // 같은 시스템의 다음 생성은 처음의 TYPE 사본 대신 그때의 현재 값을 읽는다.
    const pool = new SidPool(32768, { server: false });
    let type = 56;
    const attached = [];
    const registry = new RawRenderRegistry(pool, { reserve: () => true, releaseArray: () => {}, attach: (fields, value) => { attached.push(value); fields.parent = 50; fields.form = 7; } });
    const body = new RawFieryTailLifetime(pool, registry, { palette: 1, sharedProcessType: () => type }, { reserve: () => true }, FieryTailVariant.Streak);
    const first = createFieryTailFrameFields();
    const second = createFieryTailFrameFields();
    body.initialize(first, 1, 50);
    type = 57;
    body.initialize(second, 2, 50);
    assert.ok(attached.length === 2 && attached[0] === 56 && attached[1] === 57 && first.maximumLife === 2 && second.maximumLife === 2);
    body.destroy(first, 1, () => {});
    body.destroy(second, 1, () => {});
});

test('불꽃 꼬리/건설 마나 TYPE 전역: 초기값 11/12 와 값이 일반 월드 준비 뒤에도 장기 보존된다', () => {
    const owner = new GameAuxState();
    assert.ok(owner.fieryTailProcessType === 11 && owner.streakProcessType === 12);
    owner.fieryTailProcessType = 56;
    owner.streakProcessType = 57;
    owner.resetForGeneralWorld();
    assert.ok(owner.fieryTailProcessType === 56 && owner.streakProcessType === 57);
});

test('불꽃 꼬리 수명 생성자/입력: 경계 미연결·다른 풀·첫 공유 색 경계 누락·반납 경계 누락을 거부하고 공유 색 owner 에 직접 쓴다', () => {
    const pool = new SidPool(32768, { server: false });
    const makeRegistry = (target) => new RawRenderRegistry(target, { reserve: () => true, releaseArray: () => {}, attach: (fields, type, parent) => { fields.form = 7; fields.parent = parent; } });
    const registry = makeRegistry(pool);
    const hooks = { reserve: () => true };
    assert.ok(throwsOf(() => new RawFieryTailLifetime({}, registry, { palette: 1 }, hooks)));
    assert.ok(throwsOf(() => new RawFieryTailLifetime(pool, makeRegistry(new SidPool(32768, { server: false })), { palette: 1 }, hooks)));
    assert.ok(throwsOf(() => new RawFieryTailLifetime(pool, registry, { palette: 1 }, {})));
    assert.ok(throwsOf(() => new RawFieryTailLifetime(pool, registry, undefined, hooks)));
    assert.ok(throwsOf(() => new RawFieryTailLifetime(pool, registry, { palette: 1 }, hooks, 'Other')));
    // 첫 공유 색이 필요한데 구성 경계가 없으면 앞선 등록·입자 확보 뒤 거부한다.
    assert.ok(throwsOf(() => new RawFieryTailLifetime(pool, registry, { palette: 0 }, hooks).initialize(createFieryTailFrameFields(), 9, 50)));
    // 색 표 생성이 0 을 돌려주면 거부한다.
    assert.ok(throwsOf(() => new RawFieryTailLifetime(pool, makeRegistry(pool), { palette: 0 }, { reserve: () => true, createPalette: () => 0, buildPalette: () => {} }).initialize(createFieryTailFrameFields(), 3, 50)));
    // 건설 마나는 빔이 소유한 공유 상자가 비어 있으면 그 상자에 직접 만들고 자체 palette 는 건드리지 않는다.
    const box = { value: 0 };
    const built = [];
    const state = { palette: 999, sharedPalette: box };
    const fields = createFieryTailFrameFields();
    new RawFieryTailLifetime(pool, makeRegistry(pool), state, { reserve: () => true, createPalette: () => 7, buildPalette: (token) => built.push(token) }, FieryTailVariant.Streak).initialize(fields, 4, 50);
    assert.ok(box.value === 7 && state.palette === 999 && fields.palette === 7 && built.length === 1 && built[0] === 7);
    assert.ok(throwsOf(() => new RawFieryTailLifetime(pool, makeRegistry(pool), { palette: 1 }, hooks).destroy(createFieryTailFrameFields(), 0, undefined)));
});
