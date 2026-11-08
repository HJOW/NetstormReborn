// Burning 출력·소멸·프레임 판정 raw-burning-lifetime-draw.js 를 실제 PE 관측 fixture 로 대조한다.
//  - burning-lifetime-draw-x86.tsv: D 579행(5픽셀 출력: 수명/좌표/clip 과 실제 쓰기 순서, 64×64 표면 Adler-32·합), X 64행(선택 버퍼/flags 별 반납·소리·등록/base 순서), C 12행(최초 색 표 생성자 의미 27 DWORD·입자 zero·부착/확보·첫 gradient 인수 6 double 비트).
//  - render-frame-gate-x86.tsv 128행: signed slot 과 현재 DWORD frame 의 487940 반환. burning-archer-capture-x86.tsv 88행: 71/128 의 +78/+a4 그림 제출(포구 (−7, 11), 그림 요청의 모양/프레임/좌표).
//  - 각 행은 입력만 설치하고 모듈이 계산한다. 기대 열은 비교에만 쓴다. 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E184: Burning 출력·소멸·프레임 판정 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat, floatToBits, adler32 } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawRenderRegistry } from '../www/core/raw-construction-lifetime.js';
import { RawBurningVisual, createBurningVisualFields } from '../www/core/raw-burning-visual.js';
import { BurningRelease, destroyBurningVisual, drawBurningArcherCapture, drawBurningVisual, isBurningArcherCapture, renderProcessFrameDue } from '../www/core/raw-burning-lifetime-draw.js';

// 실제 PE 관측 fixture 이다. (2026-11-09 WB04-E184)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const lifetimeRows = readRows(path.join(FIXTURES, 'burning-lifetime-draw-x86.tsv')) ?? [];
const gateRows = readRows(path.join(FIXTURES, 'render-frame-gate-x86.tsv')) ?? [];
const archerRows = readRows(path.join(FIXTURES, 'burning-archer-capture-x86.tsv')) ?? [];
const CAPACITY = 24000;

// 예외가 나는지 본다. (2026-11-09 WB04-E184)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 쉼표로 이은 입력 칸을 부호 없는 정수 목록으로 읽는다. (2026-11-09 WB04-E184)
const inputs = (text) => text.split(',').map((value) => Number(value));

// double 의 원문 비트를 부호 없는 64비트 십진 문자열로 나타낸다. (2026-11-09 WB04-E184)
function doubleBits(value) {
    const view = new DataView(new ArrayBuffer(8));
    view.setFloat64(0, value, true);
    return view.getBigUint64(0, true).toString();
}

// 실제 5픽셀 출력의 순서와 전체 64×64 표면을 독립 원문과 대조할 문자열로 만든다. (2026-11-09 WB04-E184)
function draw(v) {
    const fields = createBurningVisualFields();
    fields.capacity = v[0];
    fields.total = v[1];
    fields.particles = Array.from({ length: v[0] }, () => ({ x: 0, y: 0, velocityX: 0, velocityY: 0, life: 0, source: false }));
    if (fields.capacity) fields.particles[0] = { x: bitsToFloat(v[2]), y: bitsToFloat(v[3]), velocityX: 0, velocityY: 0, life: bitsToFloat(v[4]), source: v[5] !== 0 };
    // 별도 source/이동/만료 입자의 명시 입력이다.
    if (v[9]) for (let index = 1; index < 4 && index < fields.capacity; index += 1) fields.particles[index] = { x: 20.75 + index, y: 22.5 - index, velocityX: 0, velocityY: 0, life: index === 3 ? -1.0 : 4.0, source: index === 1 };
    const state = { cameraX: bitsToFloat(v[6]), cameraY: bitsToFloat(v[7]), viewport: v[8] ? { left: 10, top: 10, right: 30, bottom: 30 } : { left: 0, top: 0, right: 64, bottom: 64 } };
    // 실제 격리 색 표의 인접 8번 DWORD 도 명시 입력이다.
    const colors = [31, 32, 33, 34, 35, 36, 37, 38, 39];
    const pixels = new Uint8Array(4096).fill(0xa5);
    const writes = [];
    drawBurningVisual(fields, state, { count: 8, readable: colors }, (x, y, picked) => {
        const at = y * 64 + x;
        assert.ok(at >= 0 && at < pixels.length);
        pixels[at] = picked;
        writes.push(`${at}:${picked}`);
    });
    let sum = 0;
    for (const byte of pixels) sum += byte;
    return `${adler32(pixels)}:${sum}|${writes.join(';')}`;
}

// 선택 버퍼와 scalar flags 에 따른 실제 반납·소리·등록/base 순서이다. (2026-11-09 WB04-E184)
function destroy(v) {
    const pool = new SidPool(CAPACITY, { server: true });
    const registry = new RawRenderRegistry(pool, { reserve: () => true, releaseArray: () => {}, attach: () => {} }, [111, 777, 222]);
    const fields = createBurningVisualFields();
    const r = fields.registration;
    r.token = 777;
    r.form = 83;
    r.parent = 50;
    r.slot = 1;
    r.zOrder = -12;
    fields.sound = v[4] ? 55 : 0;
    if (v[1]) fields.image.pixels = [0];
    if (v[2]) fields.rows = [0];
    if (v[3]) fields.particles = [{ x: 0, y: 0, velocityX: 0, velocityY: 0, life: 0, source: false }];
    const actions = [];
    const names = { [BurningRelease.Image]: 'image', [BurningRelease.Rows]: 'rows', [BurningRelease.Particles]: 'particles', [BurningRelease.Object]: 'object' };
    destroyBurningVisual(fields, registry, v[0], (kind) => { actions.push(`Free:${names[kind]}`); }, (sound) => { actions.push(`Stop:${sound}`); });
    const values = [r.form >>> 0, r.parent >>> 0, r.slot >>> 0, r.zOrder >>> 0];
    return `${values.join(',')}|${registry.slots.join(',')}|${actions.join(';')}`;
}

// 생성자의 의미 27 DWORD/입자 zero/부착·확보/첫 gradient 인수를 원문과 대조할 문자열로 만든다. (2026-11-09 WB04-E184)
function construct(v) {
    const pool = new SidPool(CAPACITY, { server: true });
    pool.physical(0)[35] = 248;
    pool.physical(50)[35] = 245;
    const actions = [];
    let extra = '';
    const registry = new RawRenderRegistry(pool, {
        reserve: (amount) => { actions.push(`N:${amount}`); return true; },
        releaseArray: () => {},
        attach: (fields, type, parent, flags) => { actions.push(`B:${type}:${parent}:0:${flags}`); fields.form = 83; fields.parent = parent; }
    }, [0]);
    const state = { processType: 14, palette: v[0] ? 0 : 1 };
    const visual = new RawBurningVisual(pool, registry, state, {
        reserve: (amount) => { actions.push(`N:${amount}`); return true; },
        createPalette: () => 1,
        gradient: (from, to, count, width) => { extra = `G:${[...from, ...to].map(doubleBits).join(',')}:${count}:${doubleBits(width)}`; },
        buildPalette: () => { extra += ';Palette'; }
    });
    const fields = createBurningVisualFields();
    visual.initialize(fields, 777, v[1] & 0xffff, v[2]);
    const r = fields.registration;
    const clock = new DataView(new ArrayBuffer(16));
    clock.setFloat64(0, fields.lastTime, true);
    clock.setFloat64(8, fields.elapsed, true);
    const values = [
        r.form >>> 0, r.parent >>> 0, floatToBits(r.left), floatToBits(r.top), floatToBits(r.right), floatToBits(r.bottom), r.slot >>> 0, r.zOrder >>> 0, fields.capacity, fields.total,
        clock.getUint32(0, true), clock.getUint32(4, true), clock.getUint32(8, true), clock.getUint32(12, true), fields.palette, fields.sound, fields.stage, fields.image.pixels.length ? 1 : 0,
        fields.image.width >>> 0, fields.image.height >>> 0, fields.image.hotspotX >>> 0, fields.image.hotspotY >>> 0, fields.victim, fields.mode, fields.rows.length ? 1 : 0, fields.selected >>> 0, fields.off ? 1 : 0
    ];
    const bytes = new Uint8Array(fields.particles.length * 24);
    // 호스트 padding 을 배제한 실제 24바이트 필드이다.
    fields.particles.forEach((particle, index) => {
        const at = index * 24;
        put(bytes, at, floatToBits(particle.x));
        put(bytes, at + 4, floatToBits(particle.y));
        put(bytes, at + 8, floatToBits(particle.velocityX));
        put(bytes, at + 12, floatToBits(particle.velocityY));
        put(bytes, at + 16, floatToBits(particle.life));
        put(bytes, at + 20, particle.source ? 1 : 0);
    });
    let sum = 0;
    for (const byte of bytes) sum += byte;
    return `${values.join(',')}|${registry.slots.join(',')}|${actions.join(';')}|${adler32(bytes)}|${sum}|${state.palette ? 'True' : 'False'}|${extra}`;
}

// 입력 종류별 전체 독립 관찰을 빠짐없이 소비한다. (2026-11-09 WB04-E184)
function replay(kind, expectedCount, run) {
    let count = 0;
    // 기대 열을 현재 값과만 비교한다.
    for (const row of lifetimeRows) {
        if (row[0] !== kind) continue;
        assert.equal(row.length, 3);
        assert.equal(run(inputs(row[1])), row[2], `Burning ${kind} 행 ${count}`);
        count += 1;
    }
    assert.equal(count, expectedCount);
}

test('Burning 출력: 5픽셀 순서·clip·색 조회·수명을 실제 PE 579행 전체로 대조한다', () => { replay('D', 579, draw); });
test('Burning 소멸: 선택 버퍼/flags 별 반납·소리·등록/base 순서를 실제 PE 64행으로 대조한다', () => { replay('X', 64, destroy); });
test('Burning 생성자: 최초 색 표와 실제 시작/끝 RGB gradient 를 실제 PE 12행으로 대조한다', () => { replay('C', 12, construct); });

test('RenderProcess 프레임 진입: signed slot 과 현재 DWORD frame 의 487940 반환을 실제 PE 128행으로 대조한다', () => {
    assert.equal(gateRows.length, 128);
    // signed slot 과 현재 DWORD frame 의 원본 반환이다.
    gateRows.forEach((row, index) => {
        assert.equal(row.length, 3);
        assert.equal(renderProcessFrameDue(Number(row[0]), Number(row[1])), row[2] === '1', `프레임 판정 행 ${index}`);
    });
});

test('Burning 궁수 캡처: 71/128 의 현재 프레임/출력 WORD/extra 로 그림 요청을 실제 PE 88행과 대조한다', () => {
    assert.equal(archerRows.length, 88);
    // 원본 그림 요청을 현재 요청과 직접 비교한다.
    archerRows.forEach((row, index) => {
        const fields = row[0].split(',');
        const raw = new Uint8Array(50);
        const type = Number(fields[0]);
        put(raw, 0, type === 71 ? 0x50b888 : 0x50b9b8);
        raw[10] = type;
        put(raw, 36, Number(fields[2]));
        put(raw, 12, Number(fields[3]), 2);
        raw[40] = Number(fields[4]) & 0xff;
        const requests = [];
        drawBurningArcherCapture(raw, Number(fields[1]) | 0, -7, 11, (shape, frame, x, y) => { requests.push(`${shape},${frame},${x},${y}`); }, 311, 233);
        assert.equal(`${(Number(fields[1]) | 0) >>> 0}|${requests.join(';')}`, row[1], `궁수 캡처 행 ${index}`);
    });
});

test('Burning 출력/소멸/궁수: 입자·출력 연결 오류와 다른 발사기·미연결 반납 경계를 거부한다', () => {
    const fields = createBurningVisualFields();
    fields.capacity = 2;
    fields.particles = [{ x: 1, y: 1, velocityX: 0, velocityY: 0, life: 1, source: false }];
    assert.ok(throws(() => drawBurningVisual(fields, { cameraX: 0, cameraY: 0, viewport: { left: 0, top: 0, right: 64, bottom: 64 } }, { count: 8, readable: [] }, () => {})));
    fields.capacity = 0;
    drawBurningVisual(fields, { cameraX: 0, cameraY: 0, viewport: { left: 0, top: 0, right: 64, bottom: 64 } }, { count: 8, readable: [] }, undefined);
    const raw = new Uint8Array(50);
    assert.ok(!isBurningArcherCapture(raw) && throws(() => drawBurningArcherCapture(raw, 0, 0, 0, () => {}, 0, 0)));
    const pool = new SidPool(CAPACITY, { server: true });
    const registry = new RawRenderRegistry(pool, { reserve: () => true, releaseArray: () => {}, attach: () => {} });
    assert.ok(throws(() => destroyBurningVisual(createBurningVisualFields(), registry, 0, undefined, () => {})));
    assert.ok(throws(() => destroyBurningVisual(createBurningVisualFields(), registry, 0, () => {}, undefined)));
});
