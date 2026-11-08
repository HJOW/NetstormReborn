// 번개 프레임/출력 raw-lightning-frame.js(RawLightningFrame)를 실제 PE 관측 fixture 로 대조한다.
//  - lightning-frame-x86.tsv 250행: 열 = 종류(0 프레임 + 출력 220행, 1 출력만 30행), 호출 전 64바이트 의미 상태, 경로 64바이트, 용량 전체 입자 24바이트, 부모 슬롯 50바이트, 전역 28바이트(시각 double·픽셀 배율 두 개·표시 속도 배율·카메라 두 개), 난수 상태,
//    호출 뒤 의미 상태/경로/용량 전체 입자/난수/출력 요청 순서(D dirty 사각형·L 선·S 소리·K 소진 삭제). 최초/연속 소리·시간 후퇴/긴 간격·전체 용량 수명·흔들림·소진과 굵기 경계·포인터 방향·카메라 float 저장/절삭·보조선 위치를 바이트 단위로 대조한다.
//  - cpppj 계약: 경계 미연결/다른 판본/입자 저장소 오류/부모 범위 거부. 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E186: 번개 프레임 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import fs from 'node:fs';
import { REPO_ROOT } from './helpers.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { RawLightningPath } from '../www/core/raw-lightning-path.js';
import { RawLightningFrame, createLightningFrameFields } from '../www/core/raw-lightning-frame.js';

// 실제 PE 관측 fixture 이다. (2026-11-09 WB04-E186)
const rows = fs.readFileSync(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'lightning-frame-x86.tsv'), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && line[0] !== '#').map((line) => line.split('\t'));

// fixture 의 빈 토큰/hex 를 원문 바이트로 복원한다. (2026-11-09 WB04-E186)
const decode = (text) => (text === '-' ? new Uint8Array(0) : Uint8Array.from(text.match(/../g) ?? [], (pair) => parseInt(pair, 16)));

// 비정렬 리틀엔디언 DWORD 와 float/double 입력 비트를 계산 없이 읽는다. (2026-11-09 WB04-E186)
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

// 실제 DWORD/double 을 원본 little endian 으로 끝에 붙인다. (2026-11-09 WB04-E186)
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

// 입자 전체를 실제 24BYTE 순서로 직렬화한다(미사용 입자와 음수 수명도 비교). (2026-11-09 WB04-E186)
function particleBytes(particles) {
    const bytes = [];
    for (const p of particles.values) {
        append(bytes, floatBits(p.x));
        append(bytes, floatBits(p.y));
        append(bytes, floatBits(p.strength));
        append(bytes, floatBits(p.jitterWeight));
        append(bytes, floatBits(p.life));
        append(bytes, p.previous >>> 0);
    }
    return Uint8Array.from(bytes);
}

// 원본 객체의 의미 64BYTE 와 같은 순서로 상태를 직렬화한다. (2026-11-09 WB04-E186)
function fieldBytes(fields) {
    const bytes = [];
    const rect = fields.registration;
    for (const value of [rect.left, rect.top, rect.right, rect.bottom]) append(bytes, floatBits(value));
    append(bytes, fields.particles.values.length);
    append(bytes, fields.particles.total >>> 0);
    appendDouble(bytes, fields.lastTime);
    appendDouble(bytes, fields.elapsed);
    append(bytes, floatBits(fields.delta));
    append(bytes, floatBits(fields.scaledDelta));
    append(bytes, fields.first ? 1 : 0);
    append(bytes, fields.firstLive >>> 0);
    append(bytes, floatBits(fields.jitter));
    append(bytes, fields.sound ? 1 : 0);
    return Uint8Array.from(bytes);
}

// 두 바이트 배열의 첫 불일치를 행/필드와 함께 보고한다. (2026-11-09 WB04-E186)
function compare(actual, expected, row, field) {
    assert.equal(actual.length, expected.length, `빔 프레임 행 ${row} ${field} 폭`);
    for (let index = 0; index < actual.length; index += 1) assert.equal(actual[index], expected[index], `빔 프레임 행 ${row} ${field} 바이트 ${index}`);
}

// 기본 53비트 원문 입력을 같은 raw 부모·난수·출력 경계로 재현한다. (2026-11-09 WB04-E186)
function replay(kind, expectedCount) {
    const pool = new SidPool(24000, { server: false });
    // 원본 부모 50 을 포함하는 실제 저장소를 준비한다.
    for (let sid = 5; sid <= 50; sid += 1) assert.equal(pool.allocate(2), sid);
    let count = 0;
    // 기대값은 다시 만들지 않고 전체 원문 바이트를 읽는다.
    rows.forEach((row, current) => {
        assert.equal(row.length, 12);
        if (Number(row[0]) !== kind) return;
        const before = decode(row[1]);
        const pathBytes = decode(row[2]);
        const particles = decode(row[3]);
        const parent = decode(row[4]);
        const stateBytes = decode(row[5]);
        assert.ok(before.length === 64 && pathBytes.length === 64 && parent.length === 50 && stateBytes.length === 28 && particles.length === get(before, 16) * 24, '빔 프레임 입력 폭');
        pool.allocated(50).set(parent);
        const random = new GameRandom(Number(row[6]));
        const path = new RawLightningPath(pool, random, { scaleX: 16, scaleY: 11, vanderType: 148, tallType: 149, manOWarType: 144 }, () => { throw new Error('프레임/Draw에서 중심 조회가 호출됨'); });
        const state = { now: double(stateBytes, 0), pixelScaleX: float(stateBytes, 8), pixelScaleY: float(stateBytes, 12), rate: float(stateBytes, 16), cameraX: float(stateBytes, 20), cameraY: float(stateBytes, 24) };
        const fields = createLightningFrameFields();
        const rect = fields.registration;
        rect.token = 1;
        rect.form = 1234;
        rect.parent = 50;
        rect.left = float(before, 0);
        rect.top = float(before, 4);
        rect.right = float(before, 8);
        rect.bottom = float(before, 12);
        fields.particles.values = Array.from({ length: get(before, 16) }, () => ({ x: 0, y: 0, strength: 0, jitterWeight: 0, life: 0, previous: 0 }));
        fields.particles.total = get(before, 20) | 0;
        fields.particles.owner = 1;
        fields.lastTime = double(before, 24);
        fields.elapsed = double(before, 32);
        fields.delta = float(before, 40);
        fields.scaledDelta = float(before, 44);
        fields.first = get(before, 48) !== 0;
        fields.firstLive = get(before, 52) | 0;
        fields.jitter = float(before, 56);
        if (get(before, 60)) fields.sound = 'shortZap.wav';
        fields.path.bytes.set(pathBytes);
        // fixture 토큰은 그대로 같은 입자 저장소의 index+1 이다.
        fields.particles.values.forEach((particle, index) => {
            const offset = index * 24;
            Object.assign(particle, { x: float(particles, offset), y: float(particles, offset + 4), strength: float(particles, offset + 8), jitterWeight: float(particles, offset + 12), life: float(particles, offset + 16), previous: get(particles, offset + 20) });
        });
        const events = [];
        let killed = false;
        const frame = new RawLightningFrame(pool, random, path, state, {
            invalidate: (value) => { events.push(0x44); append(events, value.left >>> 0); append(events, value.top >>> 0); append(events, value.right >>> 0); append(events, value.bottom >>> 0); append(events, 0); },
            line: (value) => { events.push(0x4c); append(events, value.x1 >>> 0); append(events, value.y1 >>> 0); append(events, value.x2 >>> 0); append(events, value.y2 >>> 0); append(events, value.color >>> 0); },
            sound: (x, y, filename) => { assert.equal(filename, 'shortZap.wav'); events.push(0x53); append(events, floatBits(x)); append(events, floatBits(y)); append(events, 1); append(events, 0); },
            removeForm: () => { events.push(0x4b); append(events, 0); killed = true; }
        });
        if (kind === 0) frame.step(fields);
        if (!killed) frame.draw(fields);
        compare(fieldBytes(fields), decode(row[7]), current, '의미 상태');
        compare(fields.path.bytes, decode(row[8]), current, '경로');
        compare(particleBytes(fields.particles), decode(row[9]), current, '용량 전체 입자');
        assert.equal(random.state, Number(row[10]), `빔 프레임 행 ${current} 난수`);
        compare(Uint8Array.from(events), decode(row[11]), current, '출력 순서');
        count += 1;
    });
    assert.equal(count, expectedCount);
}

test('번개 프레임: 최초/연속 소리·시간 후퇴/긴 간격·전체 용량 수명·흔들림·소진을 실제 PE 220행과 대조한다', () => { replay(0, 220); });
test('번개 출력: 굵기 경계·포인터 방향·카메라 float 저장/절삭·보조선 위치를 실제 PE 30행과 대조한다', () => { replay(1, 30); });

test('번개 프레임 생성자/입력: 경계 미연결·다른 판본·입자 저장소/부모 범위 오류를 거부한다', () => {
    const throwsOf = (action) => { try { action(); } catch { return true; } return false; };
    const pool = new SidPool(24000, { server: false });
    for (let sid = 5; sid <= 50; sid += 1) pool.allocate(2);
    const random = new GameRandom(1);
    const path = new RawLightningPath(pool, random, { scaleX: 16, scaleY: 11, vanderType: 148, tallType: 149, manOWarType: 144 }, () => [0, 0]);
    const state = { now: 0, pixelScaleX: 1, pixelScaleY: 1, rate: 1, cameraX: 0, cameraY: 0 };
    const hooks = { invalidate: () => {}, line: () => {}, sound: () => {}, removeForm: () => {} };
    assert.ok(throwsOf(() => new RawLightningFrame({}, random, path, state, hooks)));
    for (const name of Object.keys(hooks)) assert.ok(throwsOf(() => new RawLightningFrame(pool, random, path, state, { ...hooks, [name]: undefined })));
    const frame = new RawLightningFrame(pool, random, path, state, hooks);
    const fields = createLightningFrameFields();
    fields.registration.parent = 70000;
    assert.ok(throwsOf(() => frame.step(fields)));
    fields.registration.parent = 50;
    fields.particles.total = 5;
    assert.ok(throwsOf(() => frame.step(fields)));
    assert.ok(throwsOf(() => frame.draw(fields)));
    pool.allocated(50).fill(0);
    fields.particles.total = 0;
    assert.ok(throwsOf(() => frame.step(fields)));
});
