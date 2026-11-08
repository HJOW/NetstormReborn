// 불꽃 꼬리 프레임/출력 raw-fiery-tail-frame.js(RawFieryTailFrame)를 실제 PE 관측 fixture 로 대조한다.
//  - fierytail-frame-x86.tsv 기본 027f 65입력(비교 037f 65행은 중복 가산하지 않고 건너뜀): 열 = 이름, 정밀도, 호출 전 68바이트 의미 상태, 용량 전체 입자 24바이트, 부모 슬롯 50바이트, 전역 40바이트(시각 double·픽셀 배율 두 개·표시 속도 배율·카메라 두 개·부모 화면 끝점 두 개·진단),
//    색 표 260바이트(색수 + 인접 DWORD 64개), 난수 상태, 호출 뒤 의미 상태/입자/색수/난수/dirty 요청(왼·위·오른·아래·flags 0)/선 요청(x1·y1·x2·y2·색)을 허용오차 없이 비트 단위로 대조한다.
//  - cpppj 계약: 색수 다음 인접 DWORD 가 공급되지 않으면 가짜 색을 내보내지 않고 거부한다. 경계 미연결/입자 수 오류/부모 범위 오류도 거부한다. 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E187: 불꽃 꼬리 프레임 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import fs from 'node:fs';
import { REPO_ROOT } from './helpers.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { RawFieryTailFrame, createFieryTailFrameFields, createFieryTailParticle } from '../www/core/raw-fiery-tail-frame.js';

// 실제 PE 관측 fixture 이다(헤더 행은 `#` 로 시작해 걸러진다). (2026-11-09 WB04-E187)
const rows = fs.readFileSync(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'fierytail-frame-x86.tsv'), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && line[0] !== '#').map((line) => line.split('\t'));

// fixture 의 hex 를 원문 바이트로 복원한다('-' 는 길이 0). (2026-11-09 WB04-E187)
const decode = (text) => (text === '-' ? new Uint8Array(0) : Uint8Array.from(text.match(/../g) ?? [], (pair) => parseInt(pair, 16)));

// 비정렬 리틀엔디언 DWORD 와 float/double 입력 비트를 계산 없이 읽는다. (2026-11-09 WB04-E187)
const get = (bytes, offset) => (bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24)) >>> 0;
function float(bytes, offset) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, get(bytes, offset), true);
    return view.getFloat32(0, true);
}
function double(bytes, offset) {
    const view = new DataView(new ArrayBuffer(8));
    view.setUint32(0, get(bytes, offset), true);
    view.setUint32(4, get(bytes, offset + 4), true);
    return view.getFloat64(0, true);
}

// 출력 비트를 원본 little endian 으로 끝에 붙인다(지정 폭만). (2026-11-09 WB04-E187)
const floatBits = (value) => {
    const view = new DataView(new ArrayBuffer(4));
    view.setFloat32(0, value, true);
    return view.getUint32(0, true);
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

// 의미 68바이트를 원본 객체 순서로 직렬화한다. (2026-11-09 WB04-E187)
function fieldBytes(fields) {
    const bytes = [];
    const rect = fields.registration;
    for (const value of [rect.left, rect.top, rect.right, rect.bottom]) append(bytes, floatBits(value));
    append(bytes, fields.particles.length);
    append(bytes, fields.total >>> 0);
    appendDouble(bytes, fields.lastTime);
    appendDouble(bytes, fields.elapsed);
    append(bytes, floatBits(fields.delta));
    append(bytes, floatBits(fields.scaledDelta));
    append(bytes, fields.first ? 1 : 0);
    append(bytes, fields.maximumLife >>> 0);
    append(bytes, floatBits(fields.previousX));
    append(bytes, floatBits(fields.previousY));
    append(bytes, fields.offset >>> 0);
    return Uint8Array.from(bytes);
}

// 용량 전체 입자(사용 꼬리 밖의 active 입자와 쓰지 않는 필드 포함)를 실제 24바이트 순서로 직렬화한다. (2026-11-09 WB04-E187)
function particleBytes(particles) {
    const bytes = [];
    for (const p of particles) {
        append(bytes, floatBits(p.x));
        append(bytes, floatBits(p.y));
        append(bytes, floatBits(p.velocityX));
        append(bytes, floatBits(p.velocityY));
        append(bytes, floatBits(p.life));
        append(bytes, p.spare >>> 0);
    }
    return Uint8Array.from(bytes);
}

// 두 바이트 배열의 첫 불일치를 행/필드와 함께 보고한다. (2026-11-09 WB04-E187)
function compare(actual, expected, row, field) {
    assert.equal(actual.length, expected.length, `불꽃 꼬리 프레임 행 ${row} ${field} 폭`);
    for (let index = 0; index < actual.length; index += 1) assert.equal(actual[index], expected[index], `불꽃 꼬리 프레임 행 ${row} ${field} 바이트 ${index}`);
}

// 예외가 나는지 본다. (2026-11-09 WB04-E187)
function throwsOf(action) {
    try { action(); } catch { return true; }
    return false;
}

test('불꽃 꼬리 프레임/출력: 기본 027f 65입력의 최초 10입자·시간 조각·전체 용량 수명·진단 2·선 색을 실제 PE 와 비트 단위로 대조한다', () => {
    const pool = new SidPool(24000, { server: false });
    // 원본 부모 50 을 포함하는 실제 저장소를 준비한다.
    for (let sid = 5; sid <= 50; sid += 1) assert.equal(pool.allocate(2), sid);
    let count = 0;
    let extended = 0;
    // 기대값은 다시 만들지 않고 전체 원문 바이트를 읽는다. 비교 037f 행은 보존만 한다.
    rows.forEach((row, current) => {
        assert.equal(row.length, 14);
        if (Number(row[1]) === 0x037f) { extended += 1; return; }
        assert.equal(Number(row[1]), 0x027f);
        const before = decode(row[2]);
        const particles = decode(row[3]);
        const parent = decode(row[4]);
        const globals = decode(row[5]);
        const colors = decode(row[6]);
        assert.ok(before.length === 68 && parent.length === 50 && globals.length === 40 && colors.length === 260, '불꽃 꼬리 입력 폭');
        pool.allocated(50).set(parent);
        const fields = createFieryTailFrameFields();
        const rect = fields.registration;
        rect.parent = 50;
        rect.left = float(before, 0);
        rect.top = float(before, 4);
        rect.right = float(before, 8);
        rect.bottom = float(before, 12);
        const capacity = get(before, 16);
        assert.equal(particles.length, capacity * 24, '불꽃 꼬리 입자 폭');
        fields.total = get(before, 20) | 0;
        fields.lastTime = double(before, 24);
        fields.elapsed = double(before, 32);
        fields.delta = float(before, 40);
        fields.scaledDelta = float(before, 44);
        fields.first = get(before, 48) !== 0;
        fields.maximumLife = get(before, 52) | 0;
        fields.previousX = float(before, 56);
        fields.previousY = float(before, 60);
        fields.offset = get(before, 64) | 0;
        // 현재 입력 입자만 복원하며 기대 출력에는 제품 계산을 사용하지 않는다.
        fields.particles = Array.from({ length: capacity }, (_, index) => {
            const offset = index * 24;
            return { x: float(particles, offset), y: float(particles, offset + 4), velocityX: float(particles, offset + 8), velocityY: float(particles, offset + 12), life: float(particles, offset + 16), spare: get(particles, offset + 20) };
        });
        const state = { now: double(globals, 0), pixelScaleX: float(globals, 8), pixelScaleY: float(globals, 12), rate: float(globals, 16), cameraX: float(globals, 20), cameraY: float(globals, 24), diagnostic: get(globals, 36) | 0 };
        // 조회 가능한 인접 DWORD 도 원본 입력에서만 공급한다.
        const readable = [];
        for (let offset = 4; offset < colors.length; offset += 4) readable.push(get(colors, offset));
        const palette = { count: get(colors, 0) | 0, readable };
        const random = new GameRandom(Number(row[7]));
        const dirty = [];
        const lines = [];
        const frame = new RawFieryTailFrame(pool, random, state, palette, {
            invalidate: (value) => { append(dirty, value.left >>> 0); append(dirty, value.top >>> 0); append(dirty, value.right >>> 0); append(dirty, value.bottom >>> 0); append(dirty, 0); },
            line: (value) => { append(lines, value.x1 >>> 0); append(lines, value.y1 >>> 0); append(lines, value.x2 >>> 0); append(lines, value.y2 >>> 0); append(lines, value.color >>> 0); }
        });
        frame.step(fields);
        frame.draw(fields, get(globals, 28) | 0, get(globals, 32) | 0);
        compare(fieldBytes(fields), decode(row[8]), current, '의미 상태');
        compare(particleBytes(fields.particles), decode(row[9]), current, '용량 전체 입자');
        // 진단 분기가 바꿀 수 있는 색수만 현재 값으로 직렬화한다.
        const paletteAfter = Uint8Array.from(colors);
        for (let index = 0; index < 4; index += 1) paletteAfter[index] = (palette.count >>> (index * 8)) & 0xff;
        compare(paletteAfter, decode(row[10]), current, '색 표');
        assert.equal(random.state, Number(row[11]), `불꽃 꼬리 프레임 행 ${current} 난수`);
        compare(Uint8Array.from(dirty), decode(row[12]), current, 'dirty 요청');
        compare(Uint8Array.from(lines), decode(row[13]), current, '선 요청');
        compare(pool.slot(50), parent, current, '부모 불변');
        count += 1;
    });
    assert.equal(count, 65);
    assert.equal(extended, 65);
});

test('불꽃 꼬리 선 색: 초기 수명이 색수 다음 DWORD 를 읽으며 그 입력이 없으면 가짜 색 없이 거부한다', () => {
    const pool = new SidPool(24000, { server: false });
    const random = new GameRandom(1);
    const state = { now: 0, pixelScaleX: 16, pixelScaleY: 11, rate: 1, cameraX: 0, cameraY: 0, diagnostic: 0 };
    const palette = { count: 11, readable: new Array(11).fill(0) };
    const fields = createFieryTailFrameFields();
    fields.particles = [{ ...createFieryTailParticle(), life: 4 }];
    fields.total = 1;
    let output = 0;
    const frame = new RawFieryTailFrame(pool, random, state, palette, { invalidate: () => {}, line: () => { output += 1; } });
    assert.throws(() => frame.draw(fields, 0, 0), RangeError);
    assert.equal(output, 0);
    // 인접 DWORD 를 외부 입력으로 공급하면 같은 호출이 그 DWORD 를 선 색으로 낸다.
    palette.readable = [...new Array(11).fill(0), 0x00123456];
    frame.draw(fields, 3, 4);
    assert.equal(output, 1);
});

test('불꽃 꼬리 프레임 생성자/입력: 경계 미연결·다른 판본·입자 수/부모 SID/head 오류를 상태 변경 전에 거부한다', () => {
    const pool = new SidPool(24000, { server: false });
    for (let sid = 5; sid <= 50; sid += 1) pool.allocate(2);
    const random = new GameRandom(1);
    const state = { now: 0, pixelScaleX: 16, pixelScaleY: 11, rate: 1, cameraX: 0, cameraY: 0, diagnostic: 0 };
    const palette = { count: 0, readable: [] };
    const hooks = { invalidate: () => {}, line: () => {} };
    assert.ok(throwsOf(() => new RawFieryTailFrame({}, random, state, palette, hooks)));
    assert.ok(throwsOf(() => new RawFieryTailFrame(pool, {}, state, palette, hooks)));
    for (const name of Object.keys(hooks)) assert.ok(throwsOf(() => new RawFieryTailFrame(pool, random, state, palette, { ...hooks, [name]: undefined })));
    const frame = new RawFieryTailFrame(pool, random, state, palette, hooks);
    const fields = createFieryTailFrameFields();
    fields.registration.parent = 70000;
    assert.ok(throwsOf(() => frame.step(fields)));
    fields.registration.parent = 50;
    fields.total = 5;
    assert.ok(throwsOf(() => frame.step(fields)));
    assert.ok(throwsOf(() => frame.draw(fields, 0, 0)));
    fields.total = 0;
    // head(raw[6..7])가 0 인 부모는 원본 assert 처럼 거부하며 난수를 소비하지 않는다.
    pool.allocated(50).fill(0);
    assert.ok(throwsOf(() => frame.step(fields)));
    assert.equal(random.state, 1);
});
