// 픽셀 파편 프레임/출력 raw-shrapnel-frame.js(RawShrapnelFrame)를 실제 PE 관측 fixture 로 대조한다(cpppj `tests/ShrapnelFrameTests.cpp` 의 Replay 를 그대로 옮김).
//  - shrapnel-frame-x86.tsv 의 준비(kind 0) 198행과 프레임/출력(kind 1) 782행: 열 = kind, 가상 생성기, 호출 전 76바이트 의미 상태, 캡처 그림, 용량 전체 입자, 부모 슬롯 50바이트, 전역 28바이트(시각 double·픽셀 배율 두 개·표시 배율·카메라 두 개),
//    게임 난수 상태, CRT 상태, 호출 뒤 의미 상태/용량 전체 입자/두 난수/요청 순서(N 확보·F 반납·D dirty·K 소진·P 픽셀)/68×48 행 padding 포함 픽셀 화면을 허용오차 없이 비트 단위로 대조한다.
//    화면은 시험 쪽 clip(1 ≤ x < 63, 2 ≤ y < 46)과 pitch 68 로 cpppj 시험과 같게 그린다. 캡처 원본 그림이 바뀌지 않았는지도 확인한다.
//  - cpppj 계약: 경계 미연결·다른 판본·이미지/용량/꼬리 손상·부모 번호 범위·색 주소 토큰 오류를 상태 변경 전에 거부한다. 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E188: 파편 프레임 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import fs from 'node:fs';
import { REPO_ROOT } from './helpers.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { CrtRandom, GameRandom } from '../www/core/random.js';
import { RawShrapnelFrame, ShrapnelKind, createShrapnelFrameFields, createShrapnelParticle } from '../www/core/raw-shrapnel-frame.js';

// 실제 PE 관측 fixture 이다(헤더 행은 `#` 로 시작해 걸러진다). (2026-11-09 WB04-E188)
const rows = fs.readFileSync(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'shrapnel-frame-x86.tsv'), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && line[0] !== '#').map((line) => line.split('\t'));
// 원본 장치 입력의 폭/높이/padding 포함 행 폭이다(cpppj 시험 kWidth/kHeight/kPitch). (2026-11-09 WB04-E188)
const WIDTH = 64;
const HEIGHT = 48;
const PITCH = 68;

// fixture 의 hex 를 원문 바이트로 복원한다('-' 는 길이 0). (2026-11-09 WB04-E188)
const decode = (text) => (text === '-' ? new Uint8Array(0) : Uint8Array.from(text.match(/../g) ?? [], (pair) => parseInt(pair, 16)));

// 비정렬 리틀엔디언 DWORD 와 float/double 입력 비트를 계산 없이 읽는다. (2026-11-09 WB04-E188)
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

// 출력 비트를 원본 little endian 으로 끝에 붙인다. (2026-11-09 WB04-E188)
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

// 실제 76바이트 의미 상태를 원문 순서로 직렬화한다(cpppj 시험 Fields). (2026-11-09 WB04-E188)
function fieldBytes(fields) {
    const bytes = [];
    const rect = fields.registration;
    // 네 표시 영역 float 를 먼저 보존한다.
    for (const value of [rect.left, rect.top, rect.right, rect.bottom]) append(bytes, floatBits(value));
    append(bytes, rect.zOrder >>> 0);
    append(bytes, fields.particles.length);
    append(bytes, fields.total >>> 0);
    appendDouble(bytes, fields.lastTime);
    appendDouble(bytes, fields.elapsed);
    append(bytes, floatBits(fields.delta));
    append(bytes, floatBits(fields.scaledDelta));
    append(bytes, fields.first ? 1 : 0);
    // 캡처 치수/기준점은 signed DWORD 전체를 보존한다.
    for (const value of [fields.image.width, fields.image.height, fields.image.hotspotX, fields.image.hotspotY]) append(bytes, value >>> 0);
    append(bytes, floatBits(fields.gravity));
    return Uint8Array.from(bytes);
}

// 용량 전체의 24바이트 입자(미사용 0 포함)를 원문 순서로 직렬화한다. (2026-11-09 WB04-E188)
function particleBytes(particles) {
    const bytes = [];
    // 다섯 float 와 색 offset+1 토큰을 차례로 쓴다.
    for (const particle of particles) {
        for (const value of [particle.x, particle.y, particle.velocityX, particle.velocityY, particle.life]) append(bytes, floatBits(value));
        append(bytes, particle.pixel >>> 0);
    }
    return Uint8Array.from(bytes);
}

// 두 바이트 배열의 첫 불일치를 행/필드와 함께 보고한다. (2026-11-09 WB04-E188)
function compare(actual, expected, row, field) {
    assert.equal(actual.length, expected.length, `파편 행 ${row} ${field} 폭`);
    for (let index = 0; index < actual.length; index += 1) {
        if (actual[index] !== expected[index]) assert.fail(`파편 행 ${row} ${field} 바이트 ${index} 실제 ${actual[index]} 원본 ${expected[index]}`);
    }
}

// 예외가 나는지 본다. (2026-11-09 WB04-E188)
function throwsOf(action) {
    try { action(); } catch { return true; }
    return false;
}

// 실제 원문 입력의 준비(kind 0) 또는 프레임/출력(kind 1)을 재현하고 처리한 행 수를 돌려준다(cpppj Replay). (2026-11-09 WB04-E188)
function replay(kind) {
    const pool = new SidPool(24000, { server: false });
    // 원문 부모 50 까지 실제 슬롯을 확보한다.
    for (let sid = 5; sid <= 50; sid += 1) assert.equal(pool.allocate(2), sid);
    let count = 0;
    // 기대 상태/픽셀은 다시 계산하지 않고 원문을 읽는다.
    rows.forEach((row, current) => {
        assert.equal(row.length, 15, '파편 fixture 열 수');
        if (Number(row[0]) !== kind) return;
        const before = decode(row[2]);
        const image = decode(row[3]);
        const particles = decode(row[4]);
        const parent = decode(row[5]);
        const globals = decode(row[6]);
        assert.ok(before.length === 76 && parent.length === 50 && globals.length === 28 && particles.length === get(before, 20) * 24, '파편 입력 폭');
        pool.allocated(50).set(parent);
        const game = new GameRandom(Number(row[7]));
        const crt = new CrtRandom(Number(row[8]));
        const state = { now: double(globals, 0), pixelScaleX: float(globals, 8), pixelScaleY: float(globals, 12), rate: float(globals, 16), cameraX: float(globals, 20), cameraY: float(globals, 24), diagnostic: 0 };
        const fields = createShrapnelFrameFields();
        const rect = fields.registration;
        rect.parent = 50;
        rect.left = float(before, 0);
        rect.top = float(before, 4);
        rect.right = float(before, 8);
        rect.bottom = float(before, 12);
        rect.zOrder = get(before, 16) | 0;
        fields.total = get(before, 24) | 0;
        fields.lastTime = double(before, 28);
        fields.elapsed = double(before, 36);
        fields.delta = float(before, 44);
        fields.scaledDelta = float(before, 48);
        fields.first = get(before, 52) !== 0;
        fields.image = { width: get(before, 56) | 0, height: get(before, 60) | 0, hotspotX: get(before, 64) | 0, hotspotY: get(before, 68) | 0, pixels: Uint8Array.from(image) };
        fields.gravity = float(before, 72);
        // 색 주소만 같은 bitmap 의 offset+1 토큰으로 입력한다.
        fields.particles = Array.from({ length: get(before, 20) }, (_, index) => {
            const offset = index * 24;
            return { x: float(particles, offset), y: float(particles, offset + 4), velocityX: float(particles, offset + 8), velocityY: float(particles, offset + 12), life: float(particles, offset + 16), pixel: get(particles, offset + 20) };
        });
        const events = [];
        const canvas = new Uint8Array(PITCH * HEIGHT).fill(199);
        let killed = false;
        const logic = new RawShrapnelFrame(pool, game, crt, state, {
            reserve: (amount) => { events.push(0x4e); append(events, amount); return true; },
            release: (scratch) => { events.push(0x46); append(events, scratch ? 1 : 2); },
            invalidate: (value) => { events.push(0x44); append(events, value.left >>> 0); append(events, value.top >>> 0); append(events, value.right >>> 0); append(events, value.bottom >>> 0); append(events, 0); },
            removeForm: () => { events.push(0x4b); append(events, 0); killed = true; },
            pixel: (value) => {
                events.push(0x50);
                append(events, value.x >>> 0);
                append(events, value.y >>> 0);
                append(events, value.color >>> 0);
                // 실제 장치 clip 범위 안만 pitch 68 화면에 쓴다.
                if (value.x >= 1 && value.x < WIDTH - 1 && value.y >= 2 && value.y < HEIGHT - 2) canvas[value.y * PITCH + value.x] = value.color & 0xff;
            }
        });
        if (kind === 0) logic.prepare(fields, Number(row[1]));
        else {
            logic.step(fields);
            if (!killed) logic.draw(fields);
        }
        compare(fieldBytes(fields), decode(row[9]), current, '의미 상태');
        compare(particleBytes(fields.particles), decode(row[10]), current, '용량 전체');
        assert.equal(game.state, Number(row[11]), `파편 행 ${current} 게임 난수`);
        assert.equal(crt.state, Number(row[12]), `파편 행 ${current} CRT 난수`);
        compare(Uint8Array.from(events), decode(row[13]), current, '요청 순서');
        compare(canvas, decode(row[14]), current, '픽셀/padding');
        compare(fields.image.pixels, image, current, '캡처 원본 불변');
        count += 1;
    });
    return count;
}

test('파편 준비: 세 가상 생성·매 반복 검사 CRT 난수·게임 난수·가장자리/투명 픽셀 198입력을 실제 PE 와 비트 단위로 대조한다', () => {
    assert.equal(replay(0), 198);
});

test('파편 프레임/출력: 저장된 가속도/넓은 y·진입 수명/소진·정수화/clip·행 padding 782입력을 실제 PE 와 비트 단위로 대조한다', () => {
    assert.equal(replay(1), 782);
});

test('파편 프레임 생성자/입력: 경계 미연결·다른 판본·이미지/꼬리/부모/색 토큰 오류를 상태 변경 전에 거부한다', () => {
    const pool = new SidPool(24000, { server: false });
    for (let sid = 5; sid <= 50; sid += 1) pool.allocate(2);
    const game = new GameRandom(7);
    const crt = new CrtRandom(9);
    const state = { now: 1, pixelScaleX: 16, pixelScaleY: 11, rate: 1, cameraX: 0, cameraY: 0, diagnostic: 0 };
    const hooks = { reserve: () => true, release: () => {}, invalidate: () => {}, pixel: () => {}, removeForm: () => {} };
    assert.ok(throwsOf(() => new RawShrapnelFrame({}, game, crt, state, hooks)));
    assert.ok(throwsOf(() => new RawShrapnelFrame(pool, {}, crt, state, hooks)));
    assert.ok(throwsOf(() => new RawShrapnelFrame(pool, game, {}, state, hooks)));
    // 출력 경계 하나라도 없으면 빈 성공으로 바꾸지 않고 거부한다.
    for (const name of Object.keys(hooks)) assert.ok(throwsOf(() => new RawShrapnelFrame(pool, game, crt, state, { ...hooks, [name]: undefined })));
    const frame = new RawShrapnelFrame(pool, game, crt, state, hooks);
    const fields = createShrapnelFrameFields();
    fields.registration.parent = 50;
    // 빈 캡처(폭 0)는 준비/프레임/출력 모두 거부한다.
    assert.ok(throwsOf(() => frame.prepare(fields, ShrapnelKind.Explosion)));
    assert.ok(throwsOf(() => frame.step(fields)));
    fields.image = { width: 2, height: 2, hotspotX: 0, hotspotY: 0, pixels: Uint8Array.of(1, 2, 3) };
    assert.ok(throwsOf(() => frame.draw(fields)), '치수와 픽셀 수 불일치');
    fields.image.pixels = Uint8Array.of(1, 2, 3, 4);
    fields.total = 1;
    assert.ok(throwsOf(() => frame.draw(fields)), '꼬리가 용량보다 큼');
    fields.total = 0;
    fields.registration.parent = 0;
    assert.ok(throwsOf(() => frame.step(fields)), '부모 0');
    fields.registration.parent = 24000;
    assert.ok(throwsOf(() => frame.step(fields)), '부모 용량 밖');
    fields.registration.parent = 50;
    assert.ok(throwsOf(() => frame.prepare(fields, 3)), '가상 생성기 범위 밖');
    // 색 주소 토큰이 0 이거나 그림 밖이면 픽셀을 내지 않고 거부한다.
    let output = 0;
    const counted = new RawShrapnelFrame(pool, game, crt, state, { ...hooks, pixel: () => { output += 1; } });
    fields.particles = [{ ...createShrapnelParticle(), life: 1, pixel: 0 }];
    fields.total = 1;
    assert.ok(throwsOf(() => counted.draw(fields)));
    fields.particles[0].pixel = 5;
    assert.ok(throwsOf(() => counted.draw(fields)));
    fields.particles[0].pixel = 4;
    counted.draw(fields);
    assert.equal(output, 1);
    // 거부된 호출은 두 난수를 소비하지 않았다.
    assert.equal(game.state, 7);
    assert.equal(crt.state, 9);
});
