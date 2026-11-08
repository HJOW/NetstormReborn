// EvilRay 생성/중단/소멸 raw-evil-ray-lifetime.js(RawEvilRayLifetime)와 단일 3색 띠 buildEvilRayPalette 를 실제 PE 관측 fixture 로 대조한다(cpppj `tests/EvilRayLifetimeTests.cpp` 를 그대로 옮김).
//  - evilray-lifetime-x86.tsv 생성 기본 027f 12행(비교 037f 12행은 중복 가산하지 않고 건너뜀): 열 = 종류, 팔레트 이름, 기존 공유 색(warm), 부모, 부착 중 변경, TYPE, source, 정밀도, 256색 RGBX, 생성 직후 80바이트,
//    공유 3색, 등록 목록, 생성 요청 순서, 반납 뒤 80바이트, 반납 뒤 목록, 반납 요청 순서. 부착 중 변경은 TYPE/source 전역과 부모 깊이를 바꾸며, source 는 등록 뒤 현재 값으로 읽힘을 확인한다.
//  - 같은 fixture 의 Stop 기본 5행: mode 3 만 중단하고 다른 mode/이미 중단(전체 DWORD 0xffffffff 포함)은 진단 뒤 복귀하는 순서와 진단 인자를 대조한다.
//  - cpppj 계약: 등록 뒤 source·등록 전 TYPE 의 읽기 위치, 도달한 장치 누락 때 앞선 효과(1200·8바이트 확보, 등록 유지, 공유 색 0 유지). 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E190: EvilRay 수명 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import fs from 'node:fs';
import { REPO_ROOT } from './helpers.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawRenderRegistry } from '../www/core/raw-construction-lifetime.js';
import { RawEvilRayLifetime, EvilRayRelease, buildEvilRayPalette, createEvilRayFields } from '../www/core/raw-evil-ray-lifetime.js';

// 실제 PE 관측 fixture 이다(헤더 행은 `#` 로 시작해 걸러진다). (2026-11-09 WB04-E190)
const rows = fs.readFileSync(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'evilray-lifetime-x86.tsv'), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && line[0] !== '#').map((line) => line.split('\t'));

// fixture 의 hex 를 원문 바이트로 복원한다. (2026-11-09 WB04-E190)
const decode = (text) => Uint8Array.from(text.match(/../g) ?? [], (pair) => parseInt(pair, 16));

// float 비트 변환과 리틀엔디언 DWORD/QWORD 기록이다. (2026-11-09 WB04-E190)
const floatBits = (value) => {
    const view = new DataView(new ArrayBuffer(4));
    view.setFloat32(0, value, true);
    return view.getUint32(0, true);
};
const floatFromBits = (bits) => {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
};
function append(bytes, value) {
    for (let index = 0; index < 4; index += 1) bytes.push((value >>> (index * 8)) & 0xff);
}
function appendDouble(bytes, value) {
    const view = new DataView(new ArrayBuffer(8));
    view.setFloat64(0, value, true);
    append(bytes, view.getUint32(0, true));
    append(bytes, view.getUint32(4, true));
}

// 포인터/미작성 패딩을 뺀 원문 80바이트 의미 필드이다(용량은 호출자가 준 값, cpppj 시험 Fields). (2026-11-09 WB04-E190)
function fieldBytes(fields, capacity) {
    const reg = fields.registration;
    const bytes = [];
    append(bytes, reg.form >>> 0);
    append(bytes, reg.parent >>> 0);
    // 네 float 의 전체 비트이다.
    for (const value of [reg.left, reg.top, reg.right, reg.bottom]) append(bytes, floatBits(value));
    append(bytes, reg.slot >>> 0);
    append(bytes, reg.zOrder >>> 0);
    append(bytes, capacity);
    append(bytes, fields.total >>> 0);
    appendDouble(bytes, fields.lastTime);
    appendDouble(bytes, fields.elapsed);
    append(bytes, floatBits(fields.delta));
    append(bytes, floatBits(fields.scaledDelta));
    append(bytes, fields.first ? 1 : 0);
    append(bytes, floatBits(fields.nextEmission));
    append(bytes, fields.source >>> 0);
    append(bytes, fields.stopping >>> 0);
    return Uint8Array.from(bytes);
}

// 빈 슬롯과 기존 항목의 원문 순서를 유지한다. (2026-11-09 WB04-E190)
const slotText = (registry) => (registry.slots.length ? registry.slots.join(',') : '-');

// 두 바이트 배열이 같은지 행/필드와 함께 확인한다. (2026-11-09 WB04-E190)
function compare(actual, expected, row, field) {
    assert.equal(actual.length, expected.length, `EvilRay 수명 행 ${row} ${field} 폭`);
    for (let index = 0; index < actual.length; index += 1) {
        if (actual[index] !== expected[index]) assert.fail(`EvilRay 수명 행 ${row} ${field} 바이트 ${index} 실제 ${actual[index]} 원본 ${expected[index]}`);
    }
}

test('EvilRay 수명: 기본 027f 12행의 생성 필드·공유 3색·등록·반납 순서를 실제 PE 와 비트 단위로 대조한다', () => {
    let count = 0;
    let extended = 0;
    // 생성 행만 원문 입력 그대로 재생한다.
    rows.forEach((row, current) => {
        if (row[0] !== 'create') return;
        assert.equal(row.length, 16);
        if (Number(row[7]) === 0x037f) { extended += 1; return; }
        const parent = Number(row[3]) & 0xffff;
        const warm = Number(row[2]) !== 0;
        const mutate = Number(row[4]) !== 0;
        const pool = new SidPool(32768, { server: false });
        // 기대값이 아닌 독립 원문 입력이다.
        const state = { palette: warm ? 1 : 0, processType: Number(row[5]) >>> 0, source: Number(row[6]) >>> 0 };
        pool.physical(0)[35] = 255;
        pool.physical(50)[35] = 127;
        pool.physical(51)[35] = 64;
        const events = [];
        const event = (value) => events.push(value);
        // 원문 입력의 유일한 빈칸 20 을 준비한다.
        const initial = Array.from({ length: 40 }, (_, index) => (index === 20 ? 0 : 100 + index));
        const registry = new RawRenderRegistry(pool, {
            reserve: (bytes) => { event(`N:${bytes}`); return true; },
            releaseArray: () => event('F:R'),
            attach: (fields, type, target, flags) => {
                event(`B:${type}:${target}:0:${flags}:${fields.slot}:${fields.zOrder}`);
                fields.form = 7;
                fields.parent = mutate ? 51 : parent;
                if (mutate) {
                    state.processType = 88;
                    state.source = 0xffffffff;
                    pool.physical(parent)[35] = 128;
                }
            }
        }, initial);
        // warm 행은 외부 기존 3색 입력을 그대로 둔다.
        let colors = [31, 32, 33];
        const rgbx = decode(row[8]);
        assert.equal(rgbx.length, 1024);
        // 원문 COL 바이트를 RGBX DWORD 순서로 재구성한다.
        const palette = Array.from({ length: 256 }, (_, index) => (rgbx[index * 4] | (rgbx[index * 4 + 1] << 8) | (rgbx[index * 4 + 2] << 16) | (rgbx[index * 4 + 3] << 24)) >>> 0);
        const body = new RawEvilRayLifetime(pool, registry, state, {
            reserve: (bytes) => { event(`N:${bytes}`); return true; },
            createPalette: () => 1,
            buildPalette: (token) => { assert.equal(token, 1); event('N:12'); colors = buildEvilRayPalette(palette); }
        });
        const fields = createEvilRayFields();
        fields.delta = floatFromBits(0xa5a5a5a5);
        fields.scaledDelta = fields.delta;
        body.initialize(fields, 1, parent);
        const capacity = fields.particles.length;
        assert.equal(capacity, 50);
        compare(fieldBytes(fields, capacity), decode(row[9]), current, '생성 필드');
        assert.equal(slotText(registry), row[11], `EvilRay 수명 행 ${current} 등록 목록`);
        assert.equal(events.join(';'), row[12], `EvilRay 수명 행 ${current} 생성 요청`);
        // 미사용 용량 전체의 실제 초기값이다.
        for (const particle of fields.particles) assert.ok(particle.x === 0 && particle.y === 0 && particle.velocityX === 0 && particle.velocityY === 0 && particle.life === 0 && particle.spare === 0);
        // 인접 DWORD 를 추가하지 않고 실제 3색만 비교한다.
        const colorBytes = [];
        for (const color of colors) append(colorBytes, color);
        compare(Uint8Array.from(colorBytes), decode(row[10]), current, '공유 3색');
        events.length = 0;
        body.destroy(fields, 1, (kind) => {
            if (kind === EvilRayRelease.Particles) {
                assert.equal(registry.slots[20], 1);
                event('F:P');
            } else {
                assert.equal(registry.slots[20], 0);
                event('F:O');
            }
        });
        assert.equal(fields.particles.length, 0);
        compare(fieldBytes(fields, capacity), decode(row[13]), current, '반납 필드');
        assert.equal(slotText(registry), row[14], `EvilRay 수명 행 ${current} 반납 목록`);
        assert.equal(events.join(';'), row[15], `EvilRay 수명 행 ${current} 반납 요청`);
        assert.equal(state.palette, 1);
        count += 1;
    });
    assert.equal(count, 12);
    assert.equal(extended, 12);
});

test('EvilRay Stop: 기본 027f 5행의 mode 3/다른 mode·이미 중단된 전체 DWORD 의 진단과 결과를 실제 PE 와 대조한다', () => {
    const pool = new SidPool(32768, { server: false });
    // Stop 은 등록/확보를 호출하지 않는다.
    const registry = new RawRenderRegistry(pool, { reserve: () => true, releaseArray: () => {}, attach: () => {} });
    const state = { palette: 0, processType: 10, source: 0 };
    let count = 0;
    let extended = 0;
    // 원문 Stop 10행 중 기본 53비트 5행만 같은 제품 정밀도로 비교한다.
    for (const row of rows) {
        if (row[0] !== 'stop') continue;
        assert.equal(row.length, 6);
        if (Number(row[3]) === 0x037f) { extended += 1; continue; }
        let diagnostics = '[]';
        const body = new RawEvilRayLifetime(pool, registry, state, {
            assertion: (expression, file, number) => {
                assert.equal(file, '.\\Evilray.cpp');
                diagnostics = JSON.stringify([[expression, file, number]]);
            }
        });
        const fields = createEvilRayFields();
        fields.stopping = Number(row[2]) >>> 0;
        body.stop(fields, Number(row[1]) >>> 0);
        assert.equal(fields.stopping, Number(row[4]));
        assert.equal(diagnostics, row[5]);
        count += 1;
    }
    assert.equal(count, 5);
    assert.equal(extended, 5);
    assert.equal(registry.slots.length, 0);
});

test('EvilRay 수명 읽기 순서: TYPE 는 등록 전·source 는 등록/부착 뒤 현재 값이며 도달한 장치 누락 때 앞선 효과만 남긴다', () => {
    const pool = new SidPool(32768, { server: false });
    let type = 57;
    let source = 77;
    let attached = 0;
    const registry = new RawRenderRegistry(pool, {
        reserve: () => true,
        releaseArray: () => {},
        attach: (fields, value) => { attached = value; type = 88; source = 99; fields.form = 7; fields.parent = 50; }
    });
    const state = { palette: 1, processType: 10, source: 0, sharedProcessType: () => type, sharedSource: () => source };
    const body = new RawEvilRayLifetime(pool, registry, state, { reserve: (bytes) => { assert.equal(bytes, 1200); source = 101; return true; } });
    const fields = createEvilRayFields();
    body.initialize(fields, 1, 50);
    assert.ok(attached === 57 && fields.source === 99 && source === 101 && fields.particles.length === 50);
    body.stop(fields, 3);
    assert.equal(fields.stopping, 1);
    body.destroy(fields, 0, () => {});
    // 공유 색이 없고 생성 장치가 미연결이면 1200·8 확보 뒤 멈추고 등록은 남는다.
    state.palette = 0;
    const requested = [];
    const partial = new RawEvilRayLifetime(pool, registry, state, { reserve: (bytes) => { requested.push(bytes); return true; } });
    const unfinished = createEvilRayFields();
    assert.throws(() => partial.initialize(unfinished, 2, 50));
    assert.deepEqual(requested, [1200, 8]);
    assert.ok(unfinished.particles.length === 50 && unfinished.registration.slot >= 0 && state.palette === 0);
    partial.destroy(unfinished, 1, () => {});
    // 다른 판본/월드와 반납 장치 누락은 거부한다.
    assert.throws(() => new RawEvilRayLifetime({}, registry, state, {}));
    assert.throws(() => new RawEvilRayLifetime(new SidPool(32768, { server: false }), registry, state, {}));
    assert.throws(() => body.destroy(createEvilRayFields(), 0, null));
});
