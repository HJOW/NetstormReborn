// VFX 변환 커널 render/vfx-transform.js(drawScaledVfxImage·drawTranslatedVfxImage)를 원본 10.78 관측과 독립 BigInt 참조 모델로 대조한다.
//  - scaled-pixels-x86.tsv 240행: 실제 SHP 12종을 pane 원점/clip·RLE 투명/255·단위/분수 배율(0.5·0.99998·1.0·1.5·2.0)로 그린 전체 표면이 원본과 같다.
//  - translated-pixels-x86.tsv 296행(합성 288 + 실제 간헐천 8): 소스를 255 마스크로 쓰고 현재 대상 색을 색 변환표로 바꾼 전체 표면이 원본과 같다(배율 0.5·1.0·1.5).
//  - 원본 관측은 배율이 5종뿐이고 음수 배율·극단 위치가 없으므로, 같은 알고리즘을 BigInt 32/64비트 산술로 다시 쓴 참조 모델과 수천 개의 무작위 입력(음수 배율 반전, 감김 위치, 잘린 clip)을 대조한다.
//  - 계약: 표면/이미지/프레임 오류 거부, WORD 범위·간격 거부, 한 행 그림 생략, 단위 배율 직접 출력의 255 보존, scaleCoordinate/textureStep 직접 확인.
// 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E378: 5-A VFXTransform 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { ShapeDatabase } from '../www/assets/shapes.js';
import { drawScaledVfxImage, drawTranslatedVfxImage, scaleCoordinate, textureStep } from '../www/render/vfx-transform.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E378)
const scaledRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'scaled-pixels-x86.tsv'));
const translatedRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'translated-pixels-x86.tsv'));

// 16진수 문자열을 바이트 배열로 바꾼다. (2026-11-13 WB04-E378)
const unhex = (text) => Uint8Array.from(Buffer.from(text, 'hex'));
// 쉼표로 나뉜 정수 목록을 signed 32비트 정수로 읽는다(원문 DWORD 입력의 signed 해석). (2026-11-13 WB04-E378)
const ints = (text) => text.split(',').map((item) => Number(item) | 0);

// fixture 한 행의 SHP 에서 그림과 프레임을 읽는다. (2026-11-13 WB04-E378)
function openShape(hex) {
    const database = new ShapeDatabase(unhex(hex));
    return { image: database.decode(0, 0), frame: database.frame(0, 0) };
}

// 첫 다른 위치를 설명하는 비교이다. (2026-11-13 WB04-E378)
function firstDifference(actual, expected) {
    return actual.findIndex((value, index) => value !== expected[index]);
}

test('VfxTransform: scaled 원본 240행의 전체 표면이 pane 원점·clip·RLE 투명/255·단위/분수 배율까지 첫 실행에서 같다', { skip: !scaledRows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 입력만 읽고 제품 커널을 호출하여 기대 표면과 비교한다.
    for (const row of scaledRows) {
        assert.equal(row.length, 8);
        const [width, height] = ints(row[2]);
        const pane = ints(row[3]);
        const position = ints(row[4]);
        const scale = ints(row[5]);
        const { image, frame } = openShape(row[1]);
        const actual = new Uint8Array(width * height).fill(Number(row[6]));
        const clip = { left: pane[0], top: pane[1], right: pane[2] + 1, bottom: pane[3] + 1 };
        drawScaledVfxImage(actual, width, height, image, frame, { x: position[0], y: position[1] }, scale[0], scale[1], clip);
        const expected = unhex(row[7]);
        const mismatch = firstDifference(actual, expected);
        assert.equal(mismatch, -1, `행 ${row[0]} 픽셀 ${mismatch} 실제 ${actual[mismatch]} 기대 ${expected[mismatch]}`);
        count += 1;
    }
    assert.equal(count, 240);
});

test('VfxTransform: translated 원본 296행의 전체 표면이 마스크·대상 색 변환·결과 255 저장까지 첫 실행에서 같다', { skip: !translatedRows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 합성 288 + 실제 간헐천 8 을 순서대로 읽는다.
    for (const row of translatedRows) {
        assert.equal(row.length, 9);
        const [width, height] = ints(row[2]);
        const pane = ints(row[3]);
        const position = ints(row[4]);
        const scale = ints(row[5]);
        const { image, frame } = openShape(row[1]);
        const colors = unhex(row[6]);
        assert.equal(colors.length, 256);
        const actual = unhex(row[7]);
        const clip = { left: pane[0], top: pane[1], right: pane[2] + 1, bottom: pane[3] + 1 };
        drawTranslatedVfxImage(actual, width, height, image, frame, { x: position[0], y: position[1] }, scale[0], scale[1], clip, colors);
        const expected = unhex(row[8]);
        const mismatch = firstDifference(actual, expected);
        assert.equal(mismatch, -1, `행 ${row[0]} 픽셀 ${mismatch} 실제 ${actual[mismatch]} 기대 ${expected[mismatch]}`);
        count += 1;
    }
    assert.equal(count, 296);
});

// ─────────────────────────────────────────────────────────────────────────────
// 독립 참조 모델: 같은 C++ 알고리즘을 BigInt 32/64비트 산술로 다시 쓴 구현이다.
// ─────────────────────────────────────────────────────────────────────────────

// 부호 없는 32비트로 감싼다. (2026-11-13 WB04-E378)
const u32 = (value) => BigInt.asUintN(32, BigInt(value));
// 부호 있는 32비트로 감싼다. (2026-11-13 WB04-E378)
const i32 = (value) => BigInt.asIntN(32, BigInt(value));

// 참조 구현의 꼭짓점 변환이다(C++ 표기: 0 방향 나눗셈 뒤 나머지가 음수면 1 을 뺀다). (2026-11-13 WB04-E378)
function referenceScale(coordinate, scale) {
    const shifted = i32(u32(coordinate) << 16n);
    const biased = shifted * BigInt(scale) + 0x8000n;
    const divisor = 1n << 32n;
    const quotient = biased / divisor - (biased % divisor < 0n ? 1n : 0n);
    return i32(quotient);
}

// 참조 구현의 그리기이다. 오류 검사는 모든 입력이 지원 범위 안임을 시험이 보장하므로 생략한다. 결과는 표면 배열을 직접 고친다. (2026-11-13 WB04-E378)
function referenceDraw(pixels, pitch, height, image, rect, position, scaleX, scaleY, clip, colors, translated) {
    const bigPosX = BigInt(position.x);
    const bigPosY = BigInt(position.y);
    const firstX = i32(bigPosX + referenceScale(rect.left, scaleX));
    const lastX = i32(bigPosX + referenceScale(rect.right, scaleX));
    const firstY = i32(bigPosY + referenceScale(rect.top, scaleY));
    const lastY = i32(bigPosY + referenceScale(rect.bottom, scaleY));
    if (firstY === lastY) return;
    const left = firstX < lastX ? firstX : lastX;
    const right = firstX < lastX ? lastX : firstX;
    const top = firstY < lastY ? firstY : lastY;
    const bottom = firstY < lastY ? lastY : firstY;
    const spanX = right - left;
    const spanY = bottom - top;
    // 보이는 사각형 = (변환 사각형) ∩ (clip ∩ 표면).
    const clamp = (value, low, high) => (value < low ? low : value > high ? high : value);
    const surfaceLeft = clamp(BigInt(clip.left), 0n, BigInt(pitch));
    const surfaceTop = clamp(BigInt(clip.top), 0n, BigInt(height));
    const surfaceRight = clamp(BigInt(clip.right), 0n, BigInt(pitch));
    const surfaceBottom = clamp(BigInt(clip.bottom), 0n, BigInt(height));
    const visibleLeft = clamp(left, surfaceLeft, surfaceRight);
    const visibleTop = clamp(top, surfaceTop, surfaceBottom);
    const visibleRight = clamp(right + 1n, surfaceLeft, surfaceRight);
    const visibleBottom = clamp(bottom + 1n, surfaceTop, surfaceBottom);
    if (visibleLeft >= visibleRight || visibleTop >= visibleBottom) return;
    const sourceX = BigInt(image.width - 1);
    const sourceY = BigInt(image.height - 1);
    const stepX = spanX === 0n ? 0n : ((lastX > firstX ? sourceX : -sourceX) * 65536n) / spanX;
    const stepY = ((lastY > firstY ? sourceY : -sourceY) * 65536n) / spanY;
    const startX = spanX !== 0n ? (lastX > firstX ? 0n : sourceX) : (lastY > firstY ? sourceX : 0n);
    const startY = lastY > firstY ? 0n : sourceY;
    // 행/열마다 시작 값에서 클리핑된 칸 수만큼 곱해 누산한다(감김 덧셈의 반복과 같다).
    for (let y = visibleTop; y < visibleBottom; y += 1n) {
        const v = u32((startY << 16n) + 0x8000n + stepY * (y - top));
        const sourceRow = Number(v >> 16n) * image.width;
        for (let x = visibleLeft; x < visibleRight; x += 1n) {
            const u = u32((startX << 16n) + 0x8000n + stepX * (x - left));
            const input = sourceRow + Number(u >> 16n);
            const target = Number(y) * pitch + Number(x);
            if (translated) {
                if (image.opacity[input] && image.indices[input] !== 255) pixels[target] = colors[pixels[target]];
            } else if (image.opacity[input]) {
                const ink = colors ? colors[image.indices[input]] : image.indices[input];
                if (ink !== 255) pixels[target] = ink;
            }
        }
    }
}

// 결정적 난수(LCG)이다. (2026-11-13 WB04-E378)
function makeRandom(seed) {
    let state = seed >>> 0;
    return (limit) => {
        state = (Math.imul(state, 1664525) + 1013904223) >>> 0;
        return Math.floor((state / 4294967296) * limit);
    };
}

// 무작위 그림(불투명/투명 섞임, 255 포함)과 프레임을 만든다. (2026-11-13 WB04-E378)
function randomShape(random, width, height, left, top) {
    const indices = new Uint8Array(width * height);
    const opacity = new Uint8Array(width * height);
    // 칸마다 색 번호와 불투명 여부를 정한다. 255 와 0 이 자주 나오게 치우친다.
    for (let index = 0; index < indices.length; index += 1) {
        const pick = random(8);
        indices[index] = pick === 0 ? 255 : pick === 1 ? 0 : random(256);
        opacity[index] = random(5) === 0 ? 0 : 255;
    }
    const image = { width, height, left, top, indices, opacity };
    const frame = { rect: { left, top, right: left + width - 1, bottom: top + height - 1 }, special: false };
    return { image, frame };
}

// 무작위 색 변환표이다(일부 값은 255). (2026-11-13 WB04-E378)
function randomColors(random) {
    const colors = new Uint8Array(256);
    // 칸마다 변환 값을 정한다.
    for (let index = 0; index < 256; index += 1) colors[index] = random(6) === 0 ? 255 : random(256);
    return colors;
}

// 사용하는 배율 목록이다: 음수 반전, 분수, 단위 근처, 확대 등. (2026-11-13 WB04-E378)
const SCALES = [0x10000, 0x10000, -0x10000, 0x8000, -0x8000, 0xffff, 0x10001, 0x18000, -0x18000, 0x20000, 0x4000, -0x2aaa, 0x30000, 0x8000, 0x1, -0x1, 0x18000, 0x20000];

test('VfxTransform: 독립 BigInt 참조 모델과 무작위 입력(음수 배율 반전·감김 위치·잘린 clip)에서 일반/translated 표면이 같다', () => {
    const random = makeRandom(0x4e53);
    let drawn = 0;
    let rejected = 0;
    // 그림 크기·위치·배율·clip 조합을 많이 섞는다.
    for (let round = 0; round < 4000; round += 1) {
        const width = 1 + random(12);
        const height = random(10) === 0 ? 1 : 2 + random(11);
        const left = random(13) - 6;
        const top = random(13) - 6;
        const { image, frame } = randomShape(random, width, height, left, top);
        const pitch = 8 + random(30);
        const rows = 8 + random(30);
        const scaleX = SCALES[random(SCALES.length)];
        const scaleY = SCALES[random(SCALES.length)];
        // 위치는 대부분 표면 근처이고 가끔 32비트 감김 직전/직후의 큰 값이다.
        const position = { x: random((pitch >> 1) + 8) - 4, y: random((rows >> 1) + 8) - 4 };
        if (random(20) === 0) position.x = 2147483647 - random(40);
        if (random(20) === 0) position.y = -2147483648 + random(40);
        const clipLeft = random(pitch) - 2;
        const clipTop = random(rows) - 2;
        // 절반은 표면 전체, 나머지는 대개 비어 있지 않은 부분 clip 이고 가끔 뒤집힌(빈) clip 이다.
        const clip = random(2) === 0
            ? { left: 0, top: 0, right: pitch, bottom: rows }
            : { left: clipLeft, top: clipTop, right: clipLeft + (random(12) === 0 ? -random(4) : 1 + random(pitch)), bottom: clipTop + (random(12) === 0 ? -random(4) : 1 + random(rows)) };
        const colors = randomColors(random);
        const background = random(256);
        const translated = random(2) === 0;
        const normalColors = !translated && random(2) === 0 ? colors : null;
        const actual = new Uint8Array(pitch * rows).fill(background);
        const expected = new Uint8Array(pitch * rows).fill(background);
        // 위치 감김 때문에 WORD 범위를 벗어나는 입력은 제품이 거부하므로 같은 거부를 기대한다.
        let threw = null;
        try {
            if (translated) drawTranslatedVfxImage(actual, pitch, rows, image, frame, position, scaleX, scaleY, clip, colors);
            else drawScaledVfxImage(actual, pitch, rows, image, frame, position, scaleX, scaleY, clip, normalColors);
        } catch (error) {
            threw = error;
        }
        const unit = !translated && scaleX === 0x10000 && scaleY === 0x10000;
        const wordRange = (value) => value >= -32768 && value <= 32767;
        const fx = Number(i32(BigInt(position.x) + referenceScale(frame.rect.left, scaleX)));
        const lx = Number(i32(BigInt(position.x) + referenceScale(frame.rect.right, scaleX)));
        const fy = Number(i32(BigInt(position.y) + referenceScale(frame.rect.top, scaleY)));
        const ly = Number(i32(BigInt(position.y) + referenceScale(frame.rect.bottom, scaleY)));
        const supported = unit || (wordRange(fx) && wordRange(lx) && wordRange(fy) && wordRange(ly) && Math.abs(fx - lx) <= 32767 && Math.abs(fy - ly) <= 32767);
        if (!supported) {
            assert.ok(threw instanceof RangeError, `범위 밖 입력은 RangeError 여야 합니다: 라운드 ${round}`);
            assert.deepEqual(Array.from(actual), Array.from(expected), '거부 시 표면은 변하지 않는다');
            rejected += 1;
            continue;
        }
        assert.equal(threw, null, `라운드 ${round}: ${threw}`);
        // 단위 배율 일반 출력은 별도 직접 출력 경로이므로 255 를 그대로 칠하는 간단한 기대값을 쓴다.
        if (unit) {
            const px = position.x + frame.rect.left;
            const py = position.y + frame.rect.top;
            const surfaceClip = {
                left: Math.max(0, Math.min(clip.left, pitch)), top: Math.max(0, Math.min(clip.top, rows)),
                right: Math.max(0, Math.min(clip.right, pitch)), bottom: Math.max(0, Math.min(clip.bottom, rows)),
            };
            // 그림 칸마다 clip 안이고 불투명이면 색(변환표 적용)을 칠한다.
            for (let iy = 0; iy < image.height; iy += 1) {
                for (let ix = 0; ix < image.width; ix += 1) {
                    const x = px + ix;
                    const y = py + iy;
                    if (x < surfaceClip.left || x >= surfaceClip.right || y < surfaceClip.top || y >= surfaceClip.bottom) continue;
                    if (!image.opacity[iy * image.width + ix]) continue;
                    const color = image.indices[iy * image.width + ix];
                    expected[y * pitch + x] = normalColors ? normalColors[color] : color;
                }
            }
        } else {
            referenceDraw(expected, pitch, rows, image, frame.rect, position, scaleX, scaleY, clip, translated ? colors : normalColors, translated);
        }
        const mismatch = firstDifference(actual, expected);
        assert.equal(mismatch, -1, `라운드 ${round} translated=${translated} scale=${scaleX},${scaleY} 픽셀 ${mismatch} 실제 ${actual[mismatch]} 기대 ${expected[mismatch]}`);
        if (actual.some((value) => value !== background)) drawn += 1;
    }
    // 대부분의 라운드에서 실제로 무언가 그려졌는지 확인한다(빈 비교로 통과하지 않도록).
    assert.ok(drawn > 1200, `그려진 라운드 ${drawn} 거부 ${rejected}`);
});

// ─────────────────────────────────────────────────────────────────────────────
// 계약
// ─────────────────────────────────────────────────────────────────────────────

// 단순 그림과 프레임을 만든다(모두 불투명, 색 번호 = 위치 기반). (2026-11-13 WB04-E378)
function plainShape(width, height, left = 0, top = 0) {
    const indices = new Uint8Array(width * height);
    const opacity = new Uint8Array(width * height).fill(255);
    indices.forEach((_, index) => { indices[index] = (index % 200) + 1; });
    return {
        image: { width, height, indices, opacity },
        frame: { rect: { left, top, right: left + width - 1, bottom: top + height - 1 }, special: false },
    };
}

// 전체 표면 clip 이다. (2026-11-13 WB04-E378)
const fullClip = (pitch, height) => ({ left: 0, top: 0, right: pitch, bottom: height });

test('VfxTransform: 표면 용량 오류와 정확히 맞는 길이를 구분한다', () => {
    const { image, frame } = plainShape(2, 2);
    const clip = fullClip(4, 4);
    const draw = (pixels, pitch, height) => drawScaledVfxImage(pixels, pitch, height, image, frame, { x: 0, y: 0 }, 0x8000, 0x8000, clip);
    assert.throws(() => draw(new Uint8Array(16), 0, 4), RangeError);
    assert.throws(() => draw(new Uint8Array(16), 4, 0), RangeError);
    assert.throws(() => draw(new Uint8Array(15), 4, 4), RangeError);
    assert.throws(() => draw(new Uint8Array(16), -4, 4), RangeError);
    assert.throws(() => draw(new Uint8Array(16), 4, -4), RangeError);
    // 정확히 맞는 길이와 더 긴 버퍼는 허용한다.
    draw(new Uint8Array(16), 4, 4);
    draw(new Uint8Array(17), 4, 4);
});

test('VfxTransform: 이미지/프레임 오류를 거부하고 거부 시 표면은 변하지 않는다', () => {
    const pixels = new Uint8Array(64).fill(9);
    const clip = fullClip(8, 8);
    const place = { x: 2, y: 2 };
    const call = (image, frame, scale = 0x8000) => drawScaledVfxImage(pixels, 8, 8, image, frame, place, scale, scale, clip);
    const base = plainShape(3, 3);
    // 너비/높이 0·32769.
    assert.throws(() => call({ ...base.image, width: 0 }, base.frame), RangeError);
    assert.throws(() => call({ ...base.image, height: 0 }, base.frame), RangeError);
    assert.throws(() => call({ ...base.image, width: 32769, height: 1, indices: new Uint8Array(32769), opacity: new Uint8Array(32769) }, { rect: { left: 0, top: 0, right: 32768, bottom: 0 }, special: false }), RangeError);
    assert.throws(() => call({ ...base.image, width: 1, height: 32769, indices: new Uint8Array(32769), opacity: new Uint8Array(32769) }, { rect: { left: 0, top: 0, right: 0, bottom: 32768 }, special: false }), RangeError);
    // 칸 수/마스크 길이 불일치.
    assert.throws(() => call({ ...base.image, indices: new Uint8Array(8) }, base.frame), RangeError);
    assert.throws(() => call({ ...base.image, opacity: new Uint8Array(8) }, base.frame), RangeError);
    // 특수 프레임·프레임 영역 불일치.
    assert.throws(() => call(base.image, { ...base.frame, special: true }), RangeError);
    assert.throws(() => call(base.image, { rect: { left: 0, top: 0, right: 3, bottom: 2 }, special: false }), RangeError);
    assert.throws(() => call(base.image, { rect: { left: 0, top: 0, right: 2, bottom: 3 }, special: false }), RangeError);
    assert.ok(pixels.every((value) => value === 9));
    // 정확히 32768 너비(허용 상한)는 오류가 아니다.
    const wide = plainShape(32768, 1);
    drawScaledVfxImage(pixels, 8, 8, wide.image, wide.frame, place, 0x10000, 0x10000, clip);
});

test('VfxTransform: 프레임·화면 WORD 범위와 축 간격 32767 초과를 거부한다', () => {
    const pixels = new Uint8Array(64).fill(9);
    const clip = fullClip(8, 8);
    // 프레임 WORD 범위: left 32768 은 범위 밖, 32767 은 안(비단위 배율 경로).
    const outLeft = plainShape(1, 2, 32768, 0);
    assert.throws(() => drawScaledVfxImage(pixels, 8, 8, outLeft.image, outLeft.frame, { x: 0, y: 0 }, 0x8000, 0x8000, clip), RangeError);
    const outTop = plainShape(2, 1, 0, -32769);
    assert.throws(() => drawScaledVfxImage(pixels, 8, 8, outTop.image, outTop.frame, { x: 0, y: 0 }, 0x8000, 0x8000, clip), RangeError);
    const outRight = plainShape(2, 1, 32767, 0);
    assert.throws(() => drawScaledVfxImage(pixels, 8, 8, outRight.image, outRight.frame, { x: 0, y: 0 }, 0x8000, 0x8000, clip), RangeError);
    const outBottom = plainShape(1, 2, 0, 32767);
    assert.throws(() => drawScaledVfxImage(pixels, 8, 8, outBottom.image, outBottom.frame, { x: 0, y: 0 }, 0x8000, 0x8000, clip), RangeError);
    // 변환된 화면 끝점이 WORD 밖: 위치로 밀어낸다.
    const small = plainShape(2, 2);
    assert.throws(() => drawScaledVfxImage(pixels, 8, 8, small.image, small.frame, { x: 40000, y: 0 }, 0x20000, 0x20000, clip), RangeError);
    assert.throws(() => drawScaledVfxImage(pixels, 8, 8, small.image, small.frame, { x: 0, y: -40000 }, 0x20000, 0x20000, clip), RangeError);
    assert.throws(() => drawScaledVfxImage(pixels, 8, 8, small.image, small.frame, { x: 0, y: 40000 }, 0x20000, 0x20000, clip), RangeError);
    assert.throws(() => drawScaledVfxImage(pixels, 8, 8, small.image, small.frame, { x: -40000, y: 0 }, 0x20000, 0x20000, clip), RangeError);
    // 축 간격 32767 초과: 폭 20001 그림을 2배 확대하면 간격 40000.
    const huge = plainShape(20001, 2, -10000, 0);
    assert.throws(() => drawScaledVfxImage(pixels, 8, 8, huge.image, huge.frame, { x: 0, y: 0 }, 0x20000, 0x20000, clip), RangeError);
    const tall = plainShape(1, 20001, 0, -10000);
    assert.throws(() => drawScaledVfxImage(pixels, 8, 8, tall.image, tall.frame, { x: 0, y: 0 }, 0x20000, 0x20000, clip), RangeError);
    // 간격 32767 정확히는 허용된다: 폭 16384 를 2배(끝점 −16384~16382 → 간격 32766)로 확대.
    const edge = plainShape(16384, 2, -8192, 0);
    drawScaledVfxImage(pixels, 8, 8, edge.image, edge.frame, { x: 0, y: 0 }, 0x20000, 0x20000, clip);
    assert.ok(pixels.every((value) => value === 9 || value > 0));
    // 단위 배율 일반 출력은 WORD 범위 검사 없이 직접 출력으로 간다.
    drawScaledVfxImage(pixels, 8, 8, outLeft.image, outLeft.frame, { x: 0, y: 0 }, 0x10000, 0x10000, clip);
    // translated 는 단위 배율도 WORD 범위 검사를 거친다.
    assert.throws(() => drawTranslatedVfxImage(pixels, 8, 8, outLeft.image, outLeft.frame, { x: 0, y: 0 }, 0x10000, 0x10000, clip, new Uint8Array(256)), RangeError);
});

test('VfxTransform: 한 행 그림의 비단위 변환은 아무것도 그리지 않고 단위 배율 일반 출력은 그린다', () => {
    const line = plainShape(4, 1, 0, 0);
    const clip = fullClip(8, 4);
    const half = new Uint8Array(32).fill(7);
    drawScaledVfxImage(half, 8, 4, line.image, line.frame, { x: 1, y: 1 }, 0x8000, 0x8000, clip);
    assert.ok(half.every((value) => value === 7));
    const identity = new Uint8Array(32).fill(7);
    drawScaledVfxImage(identity, 8, 4, line.image, line.frame, { x: 1, y: 1 }, 0x10000, 0x10000, clip);
    assert.deepEqual(Array.from(identity.subarray(9, 13)), [1, 2, 3, 4]);
    // translated 는 단위 배율에서도 한 행 그림을 생략한다.
    const translated = new Uint8Array(32).fill(7);
    drawTranslatedVfxImage(translated, 8, 4, line.image, line.frame, { x: 1, y: 1 }, 0x10000, 0x10000, clip, new Uint8Array(256).fill(1));
    assert.ok(translated.every((value) => value === 7));
});

test('VfxTransform: 단위 배율 일반 출력은 255 를 불투명 색으로 쓰고 비단위는 투명으로 건너뛴다', () => {
    const image = { width: 2, height: 2, indices: Uint8Array.from([255, 5, 6, 255]), opacity: Uint8Array.from([255, 255, 255, 255]) };
    const frame = { rect: { left: 0, top: 0, right: 1, bottom: 1 }, special: false };
    const clip = fullClip(4, 4);
    const unit = new Uint8Array(16).fill(1);
    drawScaledVfxImage(unit, 4, 4, image, frame, { x: 1, y: 1 }, 0x10000, 0x10000, clip);
    assert.deepEqual(Array.from(unit.subarray(5, 7)), [255, 5]);
    assert.deepEqual(Array.from(unit.subarray(9, 11)), [6, 255]);
    // 2배 확대(비단위)는 255 칸을 건너뛴다.
    const scaled = new Uint8Array(16).fill(1);
    drawScaledVfxImage(scaled, 4, 4, image, frame, { x: 0, y: 0 }, 0x20000, 0x20000, clip);
    assert.ok(scaled.includes(5) && scaled.includes(6));
    assert.ok(!scaled.includes(255));
    // 색 변환표 결과가 255 여도 비단위 일반 출력은 건너뛴다.
    const mapped = new Uint8Array(16).fill(1);
    const toTransparent = new Uint8Array(256).fill(255);
    drawScaledVfxImage(mapped, 4, 4, image, frame, { x: 0, y: 0 }, 0x20000, 0x20000, clip, toTransparent);
    assert.ok(mapped.every((value) => value === 1));
    // 같은 표로 단위 배율은 255 를 그대로 칠한다.
    drawScaledVfxImage(mapped, 4, 4, image, frame, { x: 0, y: 0 }, 0x10000, 0x10000, clip, toTransparent);
    assert.equal(mapped[0], 255);
});

test('VfxTransform: translated 는 대상 색을 표로 바꾸되 마스크의 255·투명 칸은 건드리지 않고 결과 255 는 저장한다', () => {
    const image = { width: 2, height: 2, indices: Uint8Array.from([255, 5, 6, 7]), opacity: Uint8Array.from([255, 255, 0, 255]) };
    const frame = { rect: { left: 0, top: 0, right: 1, bottom: 1 }, special: false };
    const clip = fullClip(4, 4);
    const colors = new Uint8Array(256);
    colors.forEach((_, index) => { colors[index] = (index + 100) & 0xff; });
    colors[3] = 255;
    const target = new Uint8Array(16).fill(3);
    drawTranslatedVfxImage(target, 4, 4, image, frame, { x: 0, y: 0 }, 0x10000, 0x10000, clip, colors);
    // (0,0): 마스크 255 → 그대로 3, (1,0): 불투명 → 대상 3 → 255 저장, (0,1): 투명 → 3, (1,1): 불투명 → 255 저장.
    assert.deepEqual(Array.from(target.subarray(0, 2)), [3, 255]);
    assert.deepEqual(Array.from(target.subarray(4, 6)), [3, 255]);
    assert.throws(() => drawTranslatedVfxImage(target, 4, 4, image, frame, { x: 0, y: 0 }, 0x10000, 0x10000, clip, null), TypeError);
});

test('VfxTransform: 음수 배율은 축을 반전하고 배율 0 은 세로가 접히면 생략·가로가 접히면 한 열만 그린다', () => {
    const image = { width: 4, height: 1, indices: Uint8Array.from([10, 20, 30, 40]), opacity: Uint8Array.from([255, 255, 255, 255]) };
    const tall = { width: 4, height: 2, indices: Uint8Array.from([10, 20, 30, 40, 50, 60, 70, 80]), opacity: new Uint8Array(8).fill(255) };
    const frame = { rect: { left: 0, top: 0, right: 3, bottom: 1 }, special: false };
    const clip = fullClip(10, 6);
    // 가로 반전(−1.0 배): 좌표가 −3~0 으로 가고 왼쪽 끝이 오른쪽 열로 간다.
    const flipped = new Uint8Array(60);
    drawScaledVfxImage(flipped, 10, 6, tall, frame, { x: 4, y: 1 }, -0x10000, 0x10000, clip);
    assert.deepEqual(Array.from(flipped.subarray(10 + 1, 10 + 5)), [40, 30, 20, 10]);
    assert.deepEqual(Array.from(flipped.subarray(20 + 1, 20 + 5)), [80, 70, 60, 50]);
    // 한 행 그림은 비단위 변환을 생략한다.
    const skipped = new Uint8Array(60).fill(2);
    drawScaledVfxImage(skipped, 10, 6, image, { rect: { left: 0, top: 0, right: 3, bottom: 0 }, special: false }, { x: 4, y: 1 }, -0x10000, 0x10000, clip);
    assert.ok(skipped.every((value) => value === 2));
    // 세로 반전(−1.0 배): 위 행이 아래로 간다.
    const vertical = new Uint8Array(60);
    drawScaledVfxImage(vertical, 10, 6, tall, frame, { x: 1, y: 3 }, 0x10000, -0x10000, clip);
    assert.deepEqual(Array.from(vertical.subarray(20 + 1, 20 + 5)), [50, 60, 70, 80]);
    assert.deepEqual(Array.from(vertical.subarray(30 + 1, 30 + 5)), [10, 20, 30, 40]);
    // 두 축 배율 0: 세로 끝점이 같아 아무것도 그리지 않는다.
    const zero = new Uint8Array(60).fill(2);
    drawScaledVfxImage(zero, 10, 6, tall, frame, { x: 4, y: 1 }, 0, 0, clip);
    assert.ok(zero.every((value) => value === 2));
    // 가로 배율 0 + 세로 1.0: 가로 간격 0 이라 오른쪽 변(마지막 원본 열) 한 열만 위에서 아래로 그린다.
    const column = new Uint8Array(60);
    drawScaledVfxImage(column, 10, 6, tall, frame, { x: 4, y: 1 }, 0, 0x10000, clip);
    assert.equal(column[10 + 4], 40);
    assert.equal(column[20 + 4], 80);
    assert.equal(column.reduce((sum, value) => sum + (value ? 1 : 0), 0), 2);
    // 가로 배율 0 + 세로 −1.0: 세로가 뒤집혀 왼쪽 변(첫 원본 열)이 아래에서 위로 놓인다.
    const flippedColumn = new Uint8Array(60);
    drawScaledVfxImage(flippedColumn, 10, 6, tall, frame, { x: 4, y: 3 }, 0, -0x10000, clip);
    assert.equal(flippedColumn[20 + 4], 50);
    assert.equal(flippedColumn[30 + 4], 10);
    assert.equal(flippedColumn.reduce((sum, value) => sum + (value ? 1 : 0), 0), 2);
});

test('VfxTransform: scaleCoordinate 는 독립 BigInt 몫과 같고 음수는 아래 정수로 내린다', () => {
    const random = makeRandom(7);
    const check = (coordinate, scale) => {
        assert.equal(scaleCoordinate(coordinate, scale), Number(referenceScale(coordinate, scale)), `c=${coordinate} s=${scale}`);
    };
    // 가장자리 값.
    const coordinates = [-32768, -32767, -1, 0, 1, 2, 31, 32767];
    const scales = [-2147483648, -0x20000, -0x10001, -0x10000, -0xffff, -1, 0, 1, 0x7fff, 0x8000, 0x8001, 0xffff, 0x10000, 0x10001, 0x18000, 0x20000, 2147483647];
    for (const coordinate of coordinates) for (const scale of scales) check(coordinate, scale);
    // 무작위 값.
    for (let round = 0; round < 20000; round += 1) check(random(65536) - 32768, (random(4294967296) | 0));
    // 알려진 값: 10 × 0.5 = 5, 음수는 아래로 내림: −3 × 0.5 = −1.5 → −2 (곱의 하위 16비트가 0 이라 편향 0x8000 은 올림을 만들지 않는다).
    assert.equal(scaleCoordinate(10, 0x8000), 5);
    assert.equal(scaleCoordinate(-3, 0x8000), -2);
    assert.equal(scaleCoordinate(1, 0xffff), 0);
    assert.equal(scaleCoordinate(2, 0xffff), 1);
});

test('VfxTransform: textureStep 은 0 방향 절삭 몫이다', () => {
    const random = makeRandom(11);
    // 독립 BigInt 몫(0 방향)과 대조한다.
    const check = (delta, span) => assert.equal(textureStep(delta, span), Number(BigInt(delta) * 65536n / BigInt(span)), `d=${delta} s=${span}`);
    for (const delta of [-32767, -31, -1, 1, 7, 31, 32767]) for (const span of [1, 2, 3, 5, 31, 32, 255, 32766, 32767]) check(delta, span);
    for (let round = 0; round < 50000; round += 1) {
        const delta = random(65535) - 32767;
        if (delta !== 0) check(delta, 1 + random(32767));
    }
    assert.equal(textureStep(-1, 3), -21845);
    assert.equal(textureStep(1, 3), 21845);
});

test('VfxTransform: 가장 작은 표면·그림과 경계 크기는 처리되고 한 항목만 틀린 입력도 메시지로 구분해 거부한다', () => {
    // 폭 1 표면(pitch 1)에 폭 1·높이 2 그림을 세로 2배로 확대한다: 세 행에 [1, 2, 2].
    const narrow = plainShape(1, 2);
    const column = new Uint8Array(4).fill(9);
    drawScaledVfxImage(column, 1, 4, narrow.image, narrow.frame, { x: 0, y: 0 }, 0x20000, 0x20000, fullClip(1, 4));
    assert.deepEqual(Array.from(column), [1, 2, 2, 9]);
    // 높이 1 표면에 폭 2·높이 1 그림을 단위 배율로 그린다.
    const flat = plainShape(2, 1);
    const row = new Uint8Array(4).fill(9);
    drawScaledVfxImage(row, 4, 1, flat.image, flat.frame, { x: 1, y: 0 }, 0x10000, 0x10000, fullClip(4, 1));
    assert.deepEqual(Array.from(row), [9, 1, 2, 9]);
    // 1×1 표면.
    const dot = plainShape(1, 1);
    const single = new Uint8Array(1).fill(9);
    drawScaledVfxImage(single, 1, 1, dot.image, dot.frame, { x: 0, y: 0 }, 0x10000, 0x10000, fullClip(1, 1));
    assert.equal(single[0], 1);
    // 높이 32768 그림(허용 상한)은 처리한다.
    const tallest = plainShape(1, 32768);
    drawScaledVfxImage(new Uint8Array(64), 8, 8, tallest.image, tallest.frame, { x: 0, y: 0 }, 0x10000, 0x10000, fullClip(8, 8));

    const surface = () => new Uint8Array(64);
    const attempt = (image, frame) => drawScaledVfxImage(surface(), 8, 8, image, frame, { x: 2, y: 2 }, 0x8000, 0x8000, fullClip(8, 8));
    const rect = (width, height) => ({ rect: { left: 0, top: 0, right: width - 1, bottom: height - 1 }, special: false });
    const blank = (length) => new Uint8Array(length);
    // 이미지 오류: 한 조건씩만 틀리게 만들고 프레임 영역은 이미지와 일치시킨다.
    assert.throws(() => attempt({ width: 0, height: 5, indices: blank(0), opacity: blank(0) }, rect(0, 5)), /이미지 용량/);
    assert.throws(() => attempt({ width: 5, height: 0, indices: blank(0), opacity: blank(0) }, rect(5, 0)), /이미지 용량/);
    assert.throws(() => attempt({ width: 32769, height: 1, indices: blank(32769), opacity: blank(32769) }, rect(32769, 1)), /이미지 용량/);
    assert.throws(() => attempt({ width: 1, height: 32769, indices: blank(32769), opacity: blank(32769) }, rect(1, 32769)), /이미지 용량/);
    assert.throws(() => attempt({ width: 3, height: 3, indices: blank(8), opacity: blank(8) }, rect(3, 3)), /이미지 용량/);
    assert.throws(() => attempt({ width: 3, height: 3, indices: blank(9), opacity: blank(8) }, rect(3, 3)), /이미지 용량/);
    // 프레임 오류: 특수 레코드와 영역 불일치(가로/세로).
    assert.throws(() => attempt(plainShape(3, 3).image, { rect: rect(3, 3).rect, special: true }), /프레임 영역/);
    assert.throws(() => attempt(plainShape(3, 3).image, rect(4, 3)), /프레임 영역/);
    assert.throws(() => attempt(plainShape(3, 3).image, rect(3, 4)), /프레임 영역/);
    // 표면 오류 메시지.
    assert.throws(() => drawScaledVfxImage(new Uint8Array(8), 4, 4, dot.image, dot.frame, { x: 0, y: 0 }, 0x8000, 0x8000, fullClip(4, 4)), /표면 용량/);
});

test('VfxTransform: WORD 경계값은 처리하고 네 끝점 중 하나만 벗어나도 메시지로 구분해 거부한다', () => {
    const clip = fullClip(8, 8);
    const draw = (image, frame, position, scale = 0x8000) => drawScaledVfxImage(new Uint8Array(64), 8, 8, image, frame, position, scale, scale, clip);
    // 프레임 WORD 경계값: 가로/세로 각각 −32768 과 32767 이 허용된다(폭·높이 1 그림이라 한 행 생략으로 끝난다).
    const corner = (left, top) => plainShape(1, 1, left, top);
    for (const [left, top] of [[32767, 0], [-32768, 0], [0, 32767], [0, -32768]]) {
        const shape = corner(left, top);
        draw(shape.image, shape.frame, { x: 0, y: 0 });
    }
    // 프레임 WORD 위반: 각 항목을 하나씩.
    for (const [left, top] of [[32768, 0], [-32769, 0], [0, 32768], [0, -32769]]) {
        const shape = corner(left, top);
        assert.throws(() => draw(shape.image, shape.frame, { x: 0, y: 0 }), /프레임 WORD/);
    }
    // 화면 끝점 경계값: 두 끝점이 같은 값 −32768 / 32767.
    const box = plainShape(2, 2);
    draw(box.image, box.frame, { x: 32767, y: 0 });
    draw(box.image, box.frame, { x: -32768, y: 0 });
    draw(box.image, box.frame, { x: 0, y: 32767 });
    draw(box.image, box.frame, { x: 0, y: -32768 });
    // 네 끝점(firstX·lastX·firstY·lastY) 중 정확히 하나만 벗어난다(2배 확대에서 끝점 차이 2).
    assert.throws(() => draw(box.image, box.frame, { x: 32766, y: 0 }, 0x20000), /화면 WORD/);
    assert.throws(() => draw(box.image, box.frame, { x: -32769, y: 0 }, 0x20000), /화면 WORD/);
    assert.throws(() => draw(box.image, box.frame, { x: 0, y: 32766 }, 0x20000), /화면 WORD/);
    assert.throws(() => draw(box.image, box.frame, { x: 0, y: -32769 }, 0x20000), /화면 WORD/);
    // 끝점 간격이 정확히 32767 이면 허용되고(translated 단위 배율로 가로/세로 각각) 32768 이면 거부한다.
    const colors = new Uint8Array(256);
    const wide = plainShape(32768, 2);
    drawTranslatedVfxImage(new Uint8Array(64), 8, 8, wide.image, wide.frame, { x: -16000, y: 0 }, 0x10000, 0x10000, clip, colors);
    const high = plainShape(2, 32768);
    drawTranslatedVfxImage(new Uint8Array(64), 8, 8, high.image, high.frame, { x: 0, y: -16000 }, 0x10000, 0x10000, clip, colors);
    assert.throws(() => drawScaledVfxImage(new Uint8Array(64), 8, 8, plainShape(16385, 2, 0, 0).image, plainShape(16385, 2, 0, 0).frame, { x: -16384, y: 0 }, 0x20000, 0x20000, clip), /간격/);
    assert.throws(() => drawScaledVfxImage(new Uint8Array(64), 8, 8, plainShape(2, 16385, 0, 0).image, plainShape(2, 16385, 0, 0).frame, { x: 0, y: -16384 }, 0x20000, 0x20000, clip), /간격/);
});
