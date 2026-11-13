// 원본 VFX 일반 선 render/vfx-line.js(drawVfxLinePixels·drawVfxLine)를 원본 10.78 불꽃 꼬리 픽셀 관측과 독립 참조 모델·계약으로 대조한다.
//  - fierytail-pixels-x86.tsv 기본 027f 30행: 12 입자의 불꽃 꼬리를 실제 RawFieryTailFrame 으로 요청해 64×48 표면에 그린 전체 화면(배경 199, pane/클립/카메라/끝점/오프셋 변화)이 원본과 같다.
//    선 끝점이 표면 밖에 걸쳐 원문의 클립 교차(Cut)·분수 위상 재계산을 거친다. (비교 037f 30행은 중복 가산하지 않고 건너뜀.)
//  - 독립 참조 모델: 같은 C++ 알고리즘을 BigInt 로 다시 쓴 모델과 무작위 입력(표면 안팎 끝점, 급경사/완경사/45도, 부분 클립, 반전 방향)에서 표면이 같다.
//  - 계약: 표면 용량 오류, 빈 클립, 수평/수직/한 점, 색 하위 BYTE, 길이 한도, DIV 범위 초과, 표면 래퍼 위임.
// 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E379: 5-A VFXLine 클립 알고리즘 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { RawFieryTailFrame, createFieryTailFrameFields } from '../www/core/raw-fiery-tail-frame.js';
import { drawVfxLinePixels, drawVfxLine } from '../www/render/vfx-line.js';
import { IndexedSurface } from '../www/render/indexed-surface.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E379)
const pixelRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'fierytail-pixels-x86.tsv'));

// 16진수 문자열을 바이트 배열로 바꾼다. (2026-11-13 WB04-E379)
const unhex = (text) => Uint8Array.from(Buffer.from(text, 'hex'));
// 비정렬 리틀엔디언 DWORD 와 float 를 읽는다. (2026-11-13 WB04-E379)
const dword = (bytes, offset) => (bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24)) >>> 0;
function float(bytes, offset) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, dword(bytes, offset), true);
    return view.getFloat32(0, true);
}

test('VfxLine: 불꽃 꼬리 원본 027f 30행의 64×48 전체 화면이 클립 교차·분수 위상까지 같다', { skip: !pixelRows && '원본 fixture 없음' }, () => {
    const pool = new SidPool(24000, { server: false });
    let count = 0;
    let extended = 0;
    // 입력만 읽고 실제 RawFieryTailFrame.draw 가 요청한 선을 제품 선으로 그려 기대 화면과 비교한다.
    for (const row of pixelRows) {
        assert.equal(row.length, 6);
        if (Number(row[1]) === 0x037f) { extended += 1; continue; }
        assert.equal(Number(row[1]), 0x027f);
        const values = row[2].split(',');
        assert.equal(values.length, 9);
        const clip = { left: Number(values[0]), top: Number(values[1]), right: Number(values[2]), bottom: Number(values[3]) };
        const particles = unhex(row[3]);
        const colors = unhex(row[4]);
        const expected = unhex(row[5]);
        assert.ok(particles.length === 12 * 24 && colors.length === 12 * 4 && expected.length === 64 * 48);
        const fields = createFieryTailFrameFields();
        fields.total = 12;
        fields.offset = Number(values[8]) | 0;
        fields.particles = Array.from({ length: 12 }, (_, index) => {
            const offset = index * 24;
            return { x: float(particles, offset), y: float(particles, offset + 4), velocityX: float(particles, offset + 8), velocityY: float(particles, offset + 12), life: float(particles, offset + 16), spare: dword(particles, offset + 20) };
        });
        const readable = [];
        for (let index = 0; index < 12; index += 1) readable.push(dword(colors, index * 4));
        const state = { now: 0, pixelScaleX: 16, pixelScaleY: 11, rate: 1, cameraX: Math.fround(Number(values[6])), cameraY: Math.fround(Number(values[7])), diagnostic: 0 };
        const pixels = new Uint8Array(64 * 48).fill(199);
        const frame = new RawFieryTailFrame(pool, new GameRandom(1), state, { count: 11, readable }, {
            invalidate: () => { throw new Error('Draw 전용 원본 픽셀 검사에서 예상하지 않은 dirty 요청'); },
            line: (value) => drawVfxLinePixels(pixels, 64, 48, clip, { x: value.x1, y: value.y1 }, { x: value.x2, y: value.y2 }, value.color)
        });
        frame.draw(fields, Number(values[4]) | 0, Number(values[5]) | 0);
        const mismatch = pixels.findIndex((value, index) => value !== expected[index]);
        assert.equal(mismatch, -1, `${row[0]} 픽셀(${mismatch % 64},${Math.floor(mismatch / 64)}) 실제 ${pixels[mismatch]} 기대 ${expected[mismatch]}`);
        count += 1;
    }
    assert.equal(count, 30);
    assert.equal(extended, 30);
});

// ─────────────────────────────────────────────────────────────────────────────
// 독립 참조 모델: 같은 C++ 알고리즘을 모든 값을 BigInt 로 두고 다시 쓴 구현이다(모듈은 Number + 필요한 곳만 BigInt).
// ─────────────────────────────────────────────────────────────────────────────

// 부호 없는/있는 32비트로 감싼다. (2026-11-13 WB04-E379)
const wrapU32 = (value) => BigInt.asUintN(32, value);
const wrapI32 = (value) => BigInt.asIntN(32, value);
const absBig = (value) => (value < 0n ? -value : value);

// 참조 구현의 선 그리기이다. 오류 검사 없이 지원 범위 입력만 받고 pixels 를 직접 고친다. 던질 오류는 { thrown } 으로 알린다. (2026-11-13 WB04-E379)
function referenceLine(pixels, pitch, height, clip, first, last, color) {
    const clamp = (value, low, high) => (value < low ? low : value > high ? high : value);
    const area = {
        left: clamp(BigInt(clip.left), 0n, BigInt(pitch)), top: clamp(BigInt(clip.top), 0n, BigInt(height)),
        right: clamp(BigInt(clip.right), 0n, BigInt(pitch)), bottom: clamp(BigInt(clip.bottom), 0n, BigInt(height))
    };
    if (area.left >= area.right || area.top >= area.bottom) return null;
    const ink = color & 0xff;
    const fx = BigInt(first.x);
    const fy = BigInt(first.y);
    const lx = BigInt(last.x);
    const ly = BigInt(last.y);
    const dx = lx - fx;
    const dy = ly - fy;
    if (dx === 0n || dy === 0n) {
        const left = area.left > (fx < lx ? fx : lx) ? area.left : (fx < lx ? fx : lx);
        const maxX = (fx > lx ? fx : lx) + 1n;
        const right = area.right < maxX ? area.right : maxX;
        const top = area.top > (fy < ly ? fy : ly) ? area.top : (fy < ly ? fy : ly);
        const maxY = (fy > ly ? fy : ly) + 1n;
        const bottom = area.bottom < maxY ? area.bottom : maxY;
        for (let y = top; y < bottom; y += 1n) for (let x = left; x < right; x += 1n) pixels[Number(y) * pitch + Number(x)] = ink;
        return null;
    }
    const ax = absBig(dx);
    const ay = absBig(dy);
    if (ax > 0x7fffffffn || ay > 0x7fffffffn) return { thrown: 'RangeError' };
    const small = ax < ay ? ax : ay;
    const large = ax < ay ? ay : ax;
    const slope = ax === ay ? 0xffffffffn : wrapU32((small << 32n) / large);
    const majorX = ax >= ay;
    const negativeSlope = (dx < 0n) !== (dy < 0n);
    const bounds = { left: area.left, top: area.top, right: area.right - 1n, bottom: area.bottom - 1n };
    let start = { x: fx, y: fy };
    let end = { x: lx, y: ly };
    const code = (point) => (point.x < bounds.left ? 8 : 0) | (point.x > bounds.right ? 4 : 0) | (point.y < bounds.top ? 2 : 0) | (point.y > bounds.bottom ? 1 : 0);
    // 32번 안에 수렴하지 않으면 { thrown: 'Error' }, DIV 몫이 32비트를 넘으면 { thrown: 'RangeError' }.
    let visible = null;
    for (let count = 0; count < 32; count += 1) {
        const a = code(start);
        const b = code(end);
        if ((a | b) === 0) { visible = true; break; }
        if ((a & b) !== 0) { visible = false; break; }
        const useStart = a !== 0;
        const c = useStart ? a : b;
        const onX = (c & 12) !== 0;
        const boundary = onX ? ((c & 8) !== 0 ? bounds.left : bounds.right) : ((c & 2) !== 0 ? bounds.top : bounds.bottom);
        const origin = onX ? fx : fy;
        const delta = boundary - origin;
        const distance = wrapU32(absBig(delta));
        let offset;
        if (onX === majorX) {
            const product = distance * slope;
            offset = wrapU32((product >> 32n) + ((product & 0xffffffffn) >= 0x80000000n ? 1n : 0n));
        } else {
            const numerator = ((useStart ? wrapU32(distance - 1n) : distance) << 32n) | 0x80000000n;
            const quotient = numerator / slope;
            const remainder = numerator % slope;
            if (quotient > 0xffffffffn) return { thrown: 'RangeError' };
            offset = wrapU32(quotient + (useStart ? 1n : 0n) - (remainder === 0n ? 1n : 0n));
        }
        const negative = (delta < 0n) !== negativeSlope;
        const other = wrapI32((onX ? fy : fx) + (negative ? wrapU32(-offset) : offset));
        const moved = onX ? { x: boundary, y: other } : { x: other, y: boundary };
        if (useStart) start = moved; else end = moved;
    }
    if (visible === null) return { thrown: 'Error' };
    if (!visible) return null;
    const startMajor = majorX ? start.x : start.y;
    const endMajor = majorX ? end.x : end.y;
    const originMajor = majorX ? fx : fy;
    let phase = wrapU32(wrapU32(absBig(startMajor - originMajor) * slope) + 0x80000000n);
    let point = { x: start.x, y: start.y };
    const sx = dx < 0n ? -1n : 1n;
    const sy = dy < 0n ? -1n : 1n;
    const total = absBig(endMajor - startMajor) + 1n;
    for (let index = 0n; index < total; index += 1n) {
        if (code(point) === 0) pixels[Number(point.y) * pitch + Number(point.x)] = ink;
        const next = phase + slope;
        const carry = next > 0xffffffffn;
        phase = wrapU32(next);
        if (majorX) {
            point = { x: wrapI32(point.x + sx), y: (ax === ay || carry) ? wrapI32(point.y + sy) : point.y };
        } else {
            point = { x: carry ? wrapI32(point.x + sx) : point.x, y: wrapI32(point.y + sy) };
        }
    }
    return null;
}

// 결정적 난수(LCG)이다. (2026-11-13 WB04-E379)
function makeRandom(seed) {
    let state = seed >>> 0;
    return (limit) => {
        state = (Math.imul(state, 1664525) + 1013904223) >>> 0;
        return Math.floor((state / 4294967296) * limit);
    };
}

// 제품 선을 불러 { thrown } 또는 null 을 돌려준다. (2026-11-13 WB04-E379)
function attempt(pixels, pitch, height, clip, first, last, color) {
    try {
        drawVfxLinePixels(pixels, pitch, height, clip, first, last, color);
        return null;
    } catch (error) {
        return { thrown: error instanceof RangeError ? 'RangeError' : 'Error' };
    }
}

test('VfxLine: 독립 BigInt 참조 모델과 무작위 입력(표면 안팎 끝점·급경사/완경사/45도·부분 clip·반전)에서 표면과 오류가 같다', () => {
    const random = makeRandom(0x4c49);
    let drawn = 0;
    let rejected = 0;
    // 끝점·클립·색 조합을 많이 섞는다.
    for (let round = 0; round < 30000; round += 1) {
        const width = 8 + random(40);
        const height = 8 + random(30);
        const range = [24, 24, 48, 48, 96, 240][random(6)];
        const point = () => ({ x: random(range) - ((range / 3) | 0), y: random(range) - ((range / 3) | 0) });
        const first = point();
        const last = point();
        // 가끔 45도/축 정렬/한 점이 되게 만든다.
        const shape = random(12);
        if (shape === 0) { last.x = first.x + (last.y - first.y); } else if (shape === 1) { last.x = first.x + (first.y - last.y); } else if (shape === 2) { last.x = first.x; } else if (shape === 3) { last.y = first.y; }
        const left = random(2) === 0 ? 0 : random(8);
        const top = random(2) === 0 ? 0 : random(8);
        const clip = random(10) === 0 ? { left: -random(5), top: -random(5), right: width + random(5), bottom: height + random(5) } : { left, top, right: width - (random(2) === 0 ? 0 : random(8)), bottom: height - (random(2) === 0 ? 0 : random(8)) };
        const color = random(300);
        const actual = new Uint8Array(width * height).fill(1);
        const expected = new Uint8Array(width * height).fill(1);
        const actualError = attempt(actual, width, height, clip, first, last, color);
        const expectedError = referenceLine(expected, width, height, clip, first, last, color);
        assert.deepEqual(actualError, expectedError, `라운드 ${round} 오류 ${JSON.stringify([first, last, clip])}`);
        if (actualError) { rejected += 1; continue; }
        const mismatch = actual.findIndex((value, index) => value !== expected[index]);
        assert.equal(mismatch, -1, `라운드 ${round} 선 ${JSON.stringify([first, last, clip])} 픽셀 ${mismatch} 실제 ${actual[mismatch]} 기대 ${expected[mismatch]}`);
        if (actual.some((value) => value !== 1)) drawn += 1;
    }
    // 대부분의 라운드에서 실제로 무언가 그려졌는지 확인한다(빈 비교로 통과하지 않도록).
    assert.ok(drawn > 11000, `그려진 라운드 ${drawn} 거부 ${rejected}`);
});

test('VfxLine: 완전히 클립 안에 있는 선은 양 끝점을 항상 칠하고 참조 모델과 같다', () => {
    const random = makeRandom(99);
    // 끝점이 모두 표면 안인 선만 만든다.
    for (let round = 0; round < 3000; round += 1) {
        const first = { x: 2 + random(30), y: 2 + random(20) };
        const last = { x: 2 + random(30), y: 2 + random(20) };
        const actual = new Uint8Array(36 * 26);
        drawVfxLinePixels(actual, 36, 26, { left: 0, top: 0, right: 36, bottom: 26 }, first, last, 5);
        const expected = new Uint8Array(36 * 26);
        referenceLine(expected, 36, 26, { left: 0, top: 0, right: 36, bottom: 26 }, first, last, 5);
        assert.deepEqual(Array.from(actual), Array.from(expected));
        assert.equal(actual[first.y * 36 + first.x], 5);
        assert.equal(actual[last.y * 36 + last.x], 5);
    }
});

// ─────────────────────────────────────────────────────────────────────────────
// 계약
// ─────────────────────────────────────────────────────────────────────────────

// 표면 전체 clip 이다. (2026-11-13 WB04-E379)
const fullClip = (pitch, height) => ({ left: 0, top: 0, right: pitch, bottom: height });

// 켜진 픽셀의 "x,y" 목록을 읽기 쉬운 문자열로 돌려준다. (2026-11-13 WB04-E379)
function lit(pixels, pitch) {
    const result = [];
    pixels.forEach((value, index) => { if (value) result.push(`${index % pitch},${Math.floor(index / pitch)}`); });
    return result.join(' ');
}

test('VfxLine: 표면 용량 오류와 정확히 맞는 길이를 구분한다', () => {
    const draw = (pixels, pitch, height) => drawVfxLinePixels(pixels, pitch, height, fullClip(4, 4), { x: 0, y: 0 }, { x: 2, y: 1 }, 1);
    assert.throws(() => draw(new Uint8Array(16), 0, 4), /표면 용량/);
    assert.throws(() => draw(new Uint8Array(16), 4, 0), /표면 용량/);
    assert.throws(() => draw(new Uint8Array(15), 4, 4), /표면 용량/);
    assert.throws(() => draw(new Uint8Array(16), -4, 4), /표면 용량/);
    assert.throws(() => draw(new Uint8Array(16), 4, -4), /표면 용량/);
    // 정확히 맞는 길이와 더 긴 버퍼는 허용한다. 1×N·N×1 표면도 그린다.
    draw(new Uint8Array(16), 4, 4);
    draw(new Uint8Array(17), 4, 4);
    const column = new Uint8Array(5);
    drawVfxLinePixels(column, 1, 5, fullClip(1, 5), { x: 0, y: 1 }, { x: 0, y: 3 }, 7);
    assert.deepEqual(Array.from(column), [0, 7, 7, 7, 0]);
    const row = new Uint8Array(5);
    drawVfxLinePixels(row, 5, 1, fullClip(5, 1), { x: 1, y: 0 }, { x: 3, y: 0 }, 7);
    assert.deepEqual(Array.from(row), [0, 7, 7, 7, 0]);
});

test('VfxLine: 수평·수직·한 점은 양 끝을 포함해 클립과 교차한 부분만 채우고 색은 하위 BYTE 다', () => {
    const run = (first, last, clip = fullClip(8, 6), color = 9) => {
        const pixels = new Uint8Array(48);
        drawVfxLinePixels(pixels, 8, 6, clip, first, last, color);
        return lit(pixels, 8);
    };
    assert.equal(run({ x: 2, y: 3 }, { x: 5, y: 3 }), '2,3 3,3 4,3 5,3');
    assert.equal(run({ x: 5, y: 3 }, { x: 2, y: 3 }), '2,3 3,3 4,3 5,3');
    assert.equal(run({ x: 4, y: 5 }, { x: 4, y: 2 }), '4,2 4,3 4,4 4,5');
    assert.equal(run({ x: 6, y: 1 }, { x: 6, y: 1 }), '6,1');
    // 표면 밖으로 이어지는 수평/수직선.
    assert.equal(run({ x: -5, y: 2 }, { x: 2, y: 2 }), '0,2 1,2 2,2');
    assert.equal(run({ x: 5, y: 2 }, { x: 40, y: 2 }), '5,2 6,2 7,2');
    assert.equal(run({ x: 3, y: -4 }, { x: 3, y: 1 }), '3,0 3,1');
    assert.equal(run({ x: 3, y: 4 }, { x: 3, y: 99 }), '3,4 3,5');
    // 부분 클립과 클립 밖 선(전혀 안 그림).
    assert.equal(run({ x: 0, y: 2 }, { x: 7, y: 2 }, { left: 2, top: 0, right: 5, bottom: 6 }), '2,2 3,2 4,2');
    assert.equal(run({ x: 1, y: 0 }, { x: 1, y: 5 }, { left: 2, top: 0, right: 5, bottom: 6 }), '');
    assert.equal(run({ x: 0, y: 8 }, { x: 5, y: 8 }), '');
    assert.equal(run({ x: 0, y: -1 }, { x: 5, y: -1 }), '');
    assert.equal(run({ x: 8, y: 1 }, { x: 12, y: 1 }), '');
    // 색은 하위 BYTE: 0x1ff → 255, 256 → 0(안 보이지만 쓰기는 한다).
    const pixels = new Uint8Array(48).fill(1);
    drawVfxLinePixels(pixels, 8, 6, fullClip(8, 6), { x: 0, y: 0 }, { x: 1, y: 0 }, 0x1ff);
    assert.deepEqual(Array.from(pixels.subarray(0, 3)), [255, 255, 1]);
    drawVfxLinePixels(pixels, 8, 6, fullClip(8, 6), { x: 0, y: 0 }, { x: 0, y: 0 }, 256);
    assert.equal(pixels[0], 0);
});

test('VfxLine: 빈 clip(표면 밖·뒤집힘·너비/높이 0)은 아무것도 쓰지 않는다', () => {
    const empties = [
        { left: 4, top: 0, right: 4, bottom: 6 }, { left: 0, top: 3, right: 8, bottom: 3 }, { left: 5, top: 0, right: 2, bottom: 6 }, { left: 0, top: 5, right: 8, bottom: 1 },
        { left: 9, top: 0, right: 20, bottom: 6 }, { left: -20, top: 0, right: -3, bottom: 6 }, { left: 0, top: 7, right: 8, bottom: 20 }, { left: 0, top: -9, right: 8, bottom: -2 }
    ];
    for (const clip of empties) {
        const pixels = new Uint8Array(48);
        drawVfxLinePixels(pixels, 8, 6, clip, { x: 0, y: 0 }, { x: 7, y: 5 }, 3);
        drawVfxLinePixels(pixels, 8, 6, clip, { x: 0, y: 2 }, { x: 7, y: 2 }, 3);
        assert.equal(lit(pixels, 8), '', JSON.stringify(clip));
    }
    // 한 칸 clip 은 그 칸만 허용한다(대각선이 지나가는 칸).
    const pixels = new Uint8Array(48);
    drawVfxLinePixels(pixels, 8, 6, { left: 3, top: 3, right: 4, bottom: 4 }, { x: 0, y: 0 }, { x: 6, y: 6 }, 3);
    assert.equal(lit(pixels, 8), '3,3');
});

test('VfxLine: 기울어진 선의 방향·분수 위상·표면 밖 끝점 자르기를 손으로 확인한 값과 대조한다', () => {
    const run = (first, last, clip = fullClip(8, 6)) => {
        const pixels = new Uint8Array(48);
        drawVfxLinePixels(pixels, 8, 6, clip, first, last, 9);
        return lit(pixels, 8);
    };
    // dx=4, dy=2: 기울기 0.5, 위상 0.5 에서 시작해 두 걸음째마다 carry.
    assert.equal(run({ x: 0, y: 0 }, { x: 4, y: 2 }), '0,0 1,1 2,1 3,2 4,2');
    // 방향이 뒤집히면 같은 위상 규칙으로 시작점에서 다시 걷는다.
    assert.equal(run({ x: 6, y: 5 }, { x: 2, y: 3 }), '2,3 3,3 4,4 5,4 6,5');
    // 45도 선은 매 걸음 두 축이 움직인다.
    assert.equal(run({ x: 1, y: 1 }, { x: 4, y: 4 }), '1,1 2,2 3,3 4,4');
    assert.equal(run({ x: 4, y: 1 }, { x: 1, y: 4 }), '4,1 3,2 2,3 1,4');
    // 급경사: 주축이 y.
    assert.equal(run({ x: 0, y: 0 }, { x: 2, y: 4 }), '0,0 1,1 1,2 2,3 2,4');
    // 완전히 밖으로 지나가는 선과 같은 반 평면 선.
    assert.equal(run({ x: -9, y: -9 }, { x: -2, y: -3 }), '');
    assert.equal(run({ x: 9, y: 0 }, { x: 20, y: 5 }), '');
    assert.equal(run({ x: -3, y: -1 }, { x: 20, y: -4 }), '');
    // 왼쪽 위 밖에서 시작해 안쪽을 지나 오른쪽 아래로 나가는 선은 안쪽 구간만 그린다.
    assert.equal(run({ x: -2, y: -2 }, { x: 10, y: 8 }), '0,0 1,0 2,1 3,2 4,3 5,4 6,5 7,5');
});

test('VfxLine: 선 길이 한도(signed 31비트)를 넘으면 거부하고 한도 안의 긴 선은 처리한다', () => {
    const pixels = new Uint8Array(40 * 30);
    const clip = fullClip(40, 30);
    const message = (action) => {
        try { action(); } catch (error) { return `${error.constructor.name}: ${error.message}`; }
        return null;
    };
    // 가로 길이가 2^32−1 이면 한도 초과, 2^31−1 이하는 한도 검사를 통과한다(다른 이유로 던져도 한도 메시지는 아니다).
    assert.match(message(() => drawVfxLinePixels(pixels, 40, 30, clip, { x: -2147483648, y: 3 }, { x: 2147483647, y: 9 }, 1)), /RangeError: .*거리 한도/);
    assert.match(message(() => drawVfxLinePixels(pixels, 40, 30, clip, { x: 3, y: -2147483648 }, { x: 9, y: 2147483647 }, 1)), /RangeError: .*거리 한도/);
    assert.doesNotMatch(message(() => drawVfxLinePixels(pixels, 40, 30, clip, { x: -1073741824, y: 3 }, { x: 1073741823, y: 9 }, 1)) ?? '', /거리 한도/);
    assert.doesNotMatch(message(() => drawVfxLinePixels(pixels, 40, 30, clip, { x: 3, y: -1073741824 }, { x: 9, y: 1073741823 }, 1)) ?? '', /거리 한도/);
    // 하한 전용: 길이 2^31 이면 한도 초과(절댓값이 INT32_MAX 를 넘는다).
    assert.match(message(() => drawVfxLinePixels(pixels, 40, 30, clip, { x: -1073741824, y: 3 }, { x: 1073741824, y: 9 }, 1)), /거리 한도/);
    // 한도 안의 긴 선이 표면을 지나가면 표면 안쪽 구간이 그려진다.
    assert.ok(lit(pixels, 40).includes('20,'));
});

test('VfxLine: drawVfxLine 래퍼는 표면의 pixels·width·height·bounds 를 그대로 위임하고 clip 기본값은 표면 전체다', () => {
    const surface = new IndexedSurface(12, 8);
    drawVfxLine(surface, { x: -3, y: 2 }, { x: 14, y: 2 }, 6);
    assert.equal(lit(surface.pixels, 12), '0,2 1,2 2,2 3,2 4,2 5,2 6,2 7,2 8,2 9,2 10,2 11,2');
    const clipped = new IndexedSurface(12, 8);
    drawVfxLine(clipped, { x: 0, y: 0 }, { x: 11, y: 7 }, 4, { left: 2, top: 2, right: 6, bottom: 6 });
    const direct = new Uint8Array(12 * 8);
    drawVfxLinePixels(direct, 12, 8, { left: 2, top: 2, right: 6, bottom: 6 }, { x: 0, y: 0 }, { x: 11, y: 7 }, 4);
    assert.deepEqual(Array.from(clipped.pixels), Array.from(direct));
    assert.notEqual(lit(direct, 12), '');
});
