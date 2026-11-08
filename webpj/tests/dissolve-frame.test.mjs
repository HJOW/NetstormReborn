// Dissolve(분해) 프레임/출력 raw-dissolve-frame.js(RawDissolveFrame)를 실제 PE 관측 fixture 로 대조한다(cpppj `tests/DissolveFrameTests.cpp` 의 Replay 를 그대로 옮김).
//  - dissolve-frame-x86.tsv 의 직접 준비(kind 0) 44행·캡처 뒤 초기화(kind 1) 44행·프레임/출력(kind 2) 164행: 열 = kind, 방향(가로 0 이 아니면 가로), 호출 전 84바이트 의미 상태, 캡처 그림, 용량 전체 입자,
//    부모 더미 50·두 폴 61/62 의 raw 150바이트, 전역 28바이트, 게임 난수, 호출 뒤 84바이트/입자/난수/요청 순서(N 확보·F 반납·D dirty·K 소진·P 픽셀)/68×48 행 padding 화면을 허용오차 없이 비트 단위로 대조한다.
//    직접 준비는 원점 (29, 31)·폴 61/62 로 부르고, 프레임 행은 이미 입자를 확보한 상태(particlesAllocated)로 복원한다. 화면은 시험 쪽 clip(1 ≤ x < 63, 2 ≤ y < 46)·pitch 68 이다.
//  - cpppj 계약: 경계 미연결·다른 판본·캡처/꼬리 손상·색 주소 토큰 오류 거부, 범위 밖 정수화는 원본 _ftol 경계처럼 0. 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E189: Dissolve 프레임 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import fs from 'node:fs';
import { REPO_ROOT } from './helpers.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { RawDissolveFrame, createDissolveFrameFields } from '../www/core/raw-dissolve-frame.js';

// 실제 PE 관측 fixture 이다(헤더 행은 `#` 로 시작해 걸러진다). (2026-11-09 WB04-E189)
const rows = fs.readFileSync(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'dissolve-frame-x86.tsv'), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && line[0] !== '#').map((line) => line.split('\t'));
// 원본 장치 입력의 폭/높이/padding 포함 행 폭이다(cpppj 시험 kWidth/kHeight/kPitch). (2026-11-09 WB04-E189)
const WIDTH = 64;
const HEIGHT = 48;
const PITCH = 68;

// fixture 의 hex 를 원문 바이트로 복원한다('-' 는 길이 0). (2026-11-09 WB04-E189)
const decode = (text) => (text === '-' ? new Uint8Array(0) : Uint8Array.from(text.match(/../g) ?? [], (pair) => parseInt(pair, 16)));

// 비정렬 리틀엔디언 DWORD 와 float/double 입력 비트를 계산 없이 읽는다. (2026-11-09 WB04-E189)
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

// 출력 비트를 원본 little endian 으로 끝에 붙인다. (2026-11-09 WB04-E189)
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

// 미사용 +0x64/+0x68 과 실제 주소를 뺀 원본 84바이트 의미 순서다(cpppj 시험 Fields). (2026-11-09 WB04-E189)
function fieldBytes(fields) {
    const value = fields.common;
    const rect = value.registration;
    const bytes = [];
    // 등록 영역의 네 저장 float 이다.
    for (const part of [rect.left, rect.top, rect.right, rect.bottom]) append(bytes, floatBits(part));
    append(bytes, rect.zOrder >>> 0);
    append(bytes, value.particles.length);
    append(bytes, value.total >>> 0);
    appendDouble(bytes, value.lastTime);
    appendDouble(bytes, value.elapsed);
    append(bytes, floatBits(value.delta));
    append(bytes, floatBits(value.scaledDelta));
    append(bytes, value.first ? 1 : 0);
    // 캡처의 signed DWORD 이다.
    for (const part of [value.image.width, value.image.height, value.image.hotspotX, value.image.hotspotY]) append(bytes, part >>> 0);
    append(bytes, 0);
    append(bytes, fields.opaqueCount >>> 0);
    append(bytes, fields.clockStarted ? 1 : 0);
    return Uint8Array.from(bytes);
}

// 색 주소만 offset+1 로 정규화한 용량 전체 24바이트 입자를 남긴다. (2026-11-09 WB04-E189)
function particleBytes(particles) {
    const bytes = [];
    // 수명이 끝난 입자와 미사용 꼬리도 보존한다.
    for (const particle of particles) {
        for (const part of [particle.x, particle.y, particle.velocityX, particle.velocityY, particle.life]) append(bytes, floatBits(part));
        append(bytes, particle.pixel >>> 0);
    }
    return Uint8Array.from(bytes);
}

// 처음 다른 바이트를 원본 행 번호와 함께 표시한다. (2026-11-09 WB04-E189)
function compare(actual, expected, row, field) {
    assert.equal(actual.length, expected.length, `분해 행 ${row} ${field} 폭`);
    for (let index = 0; index < actual.length; index += 1) {
        if (actual[index] !== expected[index]) assert.fail(`분해 행 ${row} ${field} 바이트 ${index} 실제 ${actual[index]} 원본 ${expected[index]}`);
    }
}

// 예외가 나는지 본다. (2026-11-09 WB04-E189)
function throwsOf(action) {
    try { action(); } catch { return true; }
    return false;
}

// 직접 준비·캡처 뒤 초기화·실제 프레임/출력을 각각 같은 고정 입력으로 실행하고 처리한 행 수를 돌려준다(cpppj Replay). (2026-11-09 WB04-E189)
function replay(kind) {
    const pool = new SidPool(24000, { server: false });
    // 부모와 두 폴의 실제 raw 슬롯을 확보한다.
    for (let sid = 5; sid <= 62; sid += 1) assert.equal(pool.allocate(2), sid);
    let count = 0;
    // 기대 상태/픽셀은 최초 원문에서만 읽는다.
    rows.forEach((row, current) => {
        assert.equal(row.length, 13, '분해 fixture 열 수');
        if (Number(row[0]) !== kind) return;
        const before = decode(row[2]);
        const image = decode(row[3]);
        const particles = decode(row[4]);
        const parents = decode(row[5]);
        const globals = decode(row[6]);
        assert.ok(before.length === 84 && parents.length === 150 && globals.length === 28 && particles.length === get(before, 20) * 24, '분해 입력 폭');
        // 현재 더미/두 폴을 별도로 공급한다.
        for (let part = 0; part < 3; part += 1) pool.allocated(part === 0 ? 50 : 60 + part).set(parents.subarray(part * 50, part * 50 + 50));
        const game = new GameRandom(Number(row[7]));
        const state = { now: double(globals, 0), pixelScaleX: float(globals, 8), pixelScaleY: float(globals, 12), rate: float(globals, 16), cameraX: float(globals, 20), cameraY: float(globals, 24), diagnostic: 0 };
        const fields = createDissolveFrameFields();
        const value = fields.common;
        const rect = value.registration;
        rect.parent = 50;
        rect.left = float(before, 0);
        rect.top = float(before, 4);
        rect.right = float(before, 8);
        rect.bottom = float(before, 12);
        rect.zOrder = get(before, 16) | 0;
        value.total = get(before, 24) | 0;
        value.lastTime = double(before, 28);
        value.elapsed = double(before, 36);
        value.delta = float(before, 44);
        value.scaledDelta = float(before, 48);
        value.first = get(before, 52) !== 0;
        value.image = { width: get(before, 56) | 0, height: get(before, 60) | 0, hotspotX: get(before, 64) | 0, hotspotY: get(before, 68) | 0, pixels: Uint8Array.from(image) };
        fields.opaqueCount = get(before, 76) | 0;
        fields.clockStarted = get(before, 80) !== 0;
        fields.particlesAllocated = kind === 2;
        // 원문 색 주소 토큰까지 복원한다.
        value.particles = Array.from({ length: get(before, 20) }, (_, index) => {
            const offset = index * 24;
            return { x: float(particles, offset), y: float(particles, offset + 4), velocityX: float(particles, offset + 8), velocityY: float(particles, offset + 12), life: float(particles, offset + 16), pixel: get(particles, offset + 20) };
        });
        const events = [];
        const canvas = new Uint8Array(PITCH * HEIGHT).fill(199);
        let killed = false;
        const logic = new RawDissolveFrame(pool, game, state, {
            reserve: (amount) => { events.push(0x4e); append(events, amount); return true; },
            release: (scratch) => { events.push(0x46); append(events, scratch ? 1 : 2); },
            invalidate: (area) => { events.push(0x44); append(events, area.left >>> 0); append(events, area.top >>> 0); append(events, area.right >>> 0); append(events, area.bottom >>> 0); append(events, 0); },
            removeForm: () => { events.push(0x4b); append(events, 0); killed = true; },
            pixel: (pixel) => {
                events.push(0x50);
                append(events, pixel.x >>> 0);
                append(events, pixel.y >>> 0);
                append(events, pixel.color >>> 0);
                // 실제 장치 clip 범위 안만 pitch 68 화면에 쓴다.
                if (pixel.x >= 1 && pixel.x < WIDTH - 1 && pixel.y >= 2 && pixel.y < HEIGHT - 2) canvas[pixel.y * PITCH + pixel.x] = pixel.color & 0xff;
            }
        });
        const orientation = Number(row[1]) | 0;
        if (kind === 0) logic.prepare(fields, 29, 31, 61, 62, orientation);
        else if (kind === 1) logic.initialize(fields, 61, 62, orientation);
        else {
            logic.step(fields);
            if (!killed) logic.draw(fields);
        }
        compare(fieldBytes(fields), decode(row[8]), current, '의미 상태');
        compare(particleBytes(value.particles), decode(row[9]), current, '전체 입자');
        assert.equal(game.state, Number(row[10]), `분해 행 ${current} 게임 난수`);
        compare(Uint8Array.from(events), decode(row[11]), current, '요청 순서');
        compare(canvas, decode(row[12]), current, '픽셀/padding');
        compare(value.image.pixels, image, current, '캡처 원본 불변');
        count += 1;
    });
    return count;
}

test('분해 준비: 투명/1픽셀/가로·세로와 픽셀마다 게임 난수 두 번의 순서 44입력을 실제 PE 와 비트 단위로 대조한다', () => {
    assert.equal(replay(0), 44);
});

test('분해 초기화: 현재 더미 좌표/깊이와 캡처 기준점으로 준비하는 44입력을 실제 PE 와 비트 단위로 대조한다', () => {
    assert.equal(replay(1), 44);
});

test('분해 프레임/출력: 최초 delta 0·퇴행 시계·소진·dirty·실제 clip 과 padding 164입력을 실제 PE 와 비트 단위로 대조한다', () => {
    assert.equal(replay(2), 164);
});

test('분해 프레임 생성자/입력: 경계 미연결·다른 판본·캡처/꼬리·색 토큰 오류를 거부하고 범위 밖 dirty 정수화는 0 으로 낸다', () => {
    const pool = new SidPool(24000, { server: false });
    for (let sid = 5; sid <= 62; sid += 1) pool.allocate(2);
    const game = new GameRandom(7);
    const state = { now: 1, pixelScaleX: 16, pixelScaleY: 11, rate: 1, cameraX: 0, cameraY: 0, diagnostic: 0 };
    const hooks = { reserve: () => true, release: () => {}, invalidate: () => {}, pixel: () => {}, removeForm: () => {} };
    assert.ok(throwsOf(() => new RawDissolveFrame({}, game, state, hooks)));
    assert.ok(throwsOf(() => new RawDissolveFrame(pool, {}, state, hooks)));
    // 출력 경계 하나라도 없으면 빈 성공으로 바꾸지 않고 거부한다.
    for (const name of Object.keys(hooks)) assert.ok(throwsOf(() => new RawDissolveFrame(pool, game, state, { ...hooks, [name]: undefined })));
    const dirty = [];
    let output = 0;
    const frame = new RawDissolveFrame(pool, game, state, { ...hooks, invalidate: (area) => dirty.push(area), pixel: () => { output += 1; } });
    const fields = createDissolveFrameFields();
    const value = fields.common;
    value.registration.parent = 50;
    // 빈 캡처(폭 0)는 모든 단계에서 거부한다.
    assert.ok(throwsOf(() => frame.prepare(fields, 0, 0, 61, 62, 1)));
    assert.ok(throwsOf(() => frame.step(fields)));
    value.image = { width: 2, height: 2, hotspotX: 0, hotspotY: 0, pixels: Uint8Array.of(1, 2, 3, 4) };
    value.total = 1;
    assert.ok(throwsOf(() => frame.draw(fields)), '꼬리가 용량보다 큼');
    // 색 주소 토큰이 0 이거나 그림 밖이면 픽셀을 내지 않고 거부한다.
    value.particles = [{ x: 3, y: 4, velocityX: 0, velocityY: 0, life: 1, pixel: 0 }];
    assert.ok(throwsOf(() => frame.draw(fields)));
    value.particles[0].pixel = 5;
    assert.ok(throwsOf(() => frame.draw(fields)));
    value.particles[0].pixel = 4;
    frame.draw(fields);
    assert.equal(output, 1);
    assert.equal(game.state, 7);
    // 카메라가 무한대이면 원본 _ftol 불확정 경계처럼 dirty 정수가 0 이 된다(예외 없음).
    state.cameraX = Infinity;
    fields.clockStarted = true;
    value.lastTime = 1;
    frame.step(fields);
    assert.equal(dirty.length, 2);
    assert.equal(dirty[1].left, 0);
    assert.equal(dirty[1].right, 0);
});
