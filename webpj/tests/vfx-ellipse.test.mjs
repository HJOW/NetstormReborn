// VFX 타원 윤곽 render/vfx-ellipse.js(drawVfxEllipse)를 원본 10.78 관측 270행과 계약으로 대조한다.
//  - vfx-ellipse-x86.tsv 의 270행(64×48 표면, 배경 199, 앞뒤 32바이트 sentinel 0x7d): 클립/중심/반경(음수 포함)/색 인자로 그린 전체 8비트 화면이 원본과 같다. EvilRay 의 음수 반경, 두 정수 구간, 퇴화(반경 0) 선,
//    화면 밖 중심, 한 픽셀 pane 이 포함되며 sentinel 은 변하지 않는다.
//  - 계약: 표면 용량/빈 클립 처리, 색의 하위 BYTE, 반복 상한 거부, 대칭 구조(축 대칭 점이 같은 색), 같은 인자 반복 시 같은 화면.
// 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E377: 5-A VFXEllipse 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { drawVfxEllipse, sar } from '../www/render/vfx-ellipse.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E377)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'vfx-ellipse-x86.tsv'));

test('VfxEllipse: 원본 10.78 270행의 64×48 전체 화면이 음수 반경·두 정수 구간·퇴화 선·화면 밖 중심·한 픽셀 pane 까지 같고 sentinel 은 변하지 않는다', { skip: !rows && '원본 fixture 없음' }, () => {
    let count = 0;
    // 입력만 읽고 제품 래스터를 호출하여 기대 화면과 비교한다.
    for (const row of rows) {
        assert.equal(row.length, 2);
        const args = row[0].split(',');
        assert.equal(args.length, 9);
        const expected = Uint8Array.from(Buffer.from(row[1], 'hex'));
        assert.equal(expected.length, 64 * 48);
        const clip = { left: Number(args[0]), top: Number(args[1]), right: Number(args[2]), bottom: Number(args[3]) };
        const center = { x: Number(args[4]), y: Number(args[5]) };
        // 앞뒤 32바이트는 원문과 같은 sentinel 이다.
        const protectedPixels = new Uint8Array(64 * 48 + 64).fill(0x7d);
        const pixels = protectedPixels.subarray(32, 32 + 64 * 48);
        pixels.fill(199);
        drawVfxEllipse(pixels, 64, 48, clip, center, Number(args[6]), Number(args[7]), Number(args[8]));
        const mismatch = pixels.findIndex((value, index) => value !== expected[index]);
        assert.equal(mismatch, -1, `입력 ${row[0]} 픽셀(${mismatch % 64},${Math.floor(mismatch / 64)}) 실제 ${pixels[mismatch]} 기대 ${expected[mismatch]}`);
        assert.ok(protectedPixels.subarray(0, 32).every((value) => value === 0x7d) && protectedPixels.subarray(protectedPixels.length - 32).every((value) => value === 0x7d), `sentinel ${row[0]}`);
        count += 1;
    }
    assert.equal(count, 270);
});

// 64×48 표면에 타원을 그려 돌려주는 도우미이다. (2026-11-13 WB04-E377)
function draw(args) {
    const pixels = new Uint8Array(64 * 48);
    drawVfxEllipse(pixels, 64, 48, args.clip ?? { left: 0, top: 0, right: 64, bottom: 48 }, args.center ?? { x: 32, y: 24 }, args.rx, args.ry, args.color ?? 200);
    return pixels;
}

test('VfxEllipse: 표면 용량 오류·빈 클립·색의 하위 BYTE·표면 밖 중심·반복 상한을 처리한다', () => {
    assert.throws(() => drawVfxEllipse(new Uint8Array(10), 64, 48, { left: 0, top: 0, right: 64, bottom: 48 }, { x: 0, y: 0 }, 3, 3, 1), RangeError);
    assert.throws(() => drawVfxEllipse(new Uint8Array(64 * 48), 0, 48, { left: 0, top: 0, right: 64, bottom: 48 }, { x: 0, y: 0 }, 3, 3, 1), RangeError);
    assert.throws(() => drawVfxEllipse(new Uint8Array(64 * 48), 64, 0, { left: 0, top: 0, right: 64, bottom: 48 }, { x: 0, y: 0 }, 3, 3, 1), RangeError);
    // 클립이 비거나 표면 밖이면 아무 것도 그리지 않는다.
    assert.ok(draw({ rx: 5, ry: 3, clip: { left: 10, top: 10, right: 10, bottom: 20 } }).every((value) => value === 0));
    assert.ok(draw({ rx: 5, ry: 3, clip: { left: 100, top: 100, right: 200, bottom: 200 } }).every((value) => value === 0));
    // 색은 하위 BYTE 만 쓴다(0x1c8 → 0xc8).
    const pixels = draw({ rx: 5, ry: 3, color: 0x1c8 });
    assert.ok(pixels.some((value) => value === 0xc8) && pixels.every((value) => value === 0 || value === 0xc8));
    // 표면 밖 중심의 큰 타원도 클립 안만 그린다(화면을 벗어나는 쓰기 없음).
    const far = draw({ rx: 80, ry: 60, center: { x: -30, y: -20 } });
    assert.ok(far.some((value) => value === 200));
    // 비수렴/과도한 반경은 반복 상한에서 거부한다(작은 클립이라 그리지 않으면서도 반복은 센다).
    assert.throws(() => drawVfxEllipse(new Uint8Array(64 * 48), 64, 48, { left: 0, top: 0, right: 64, bottom: 48 }, { x: 32, y: 24 }, 3000000, 3000000, 1), RangeError);
});

test('VfxEllipse: 같은 인자는 같은 화면이고 네 대칭점이 같은 색이며 퇴화 반경은 선으로 그린다', () => {
    const first = draw({ rx: 9, ry: 5 });
    assert.deepEqual(draw({ rx: 9, ry: 5 }), first);
    // 중심(32,24) 기준 좌우·상하 대칭이다.
    for (let y = 0; y < 48; y += 1) for (let x = 0; x < 64; x += 1) {
        const mirrorX = 64 - x;
        const mirrorY = 48 - y;
        if (mirrorX < 64 && mirrorY < 48) assert.equal(first[y * 64 + x], first[y * 64 + mirrorX], `좌우 ${x},${y}`);
        if (mirrorY < 48) assert.equal(first[y * 64 + x], first[mirrorY * 64 + x], `상하 ${x},${y}`);
    }
    // 반경 한 쪽이 0 이면 (중심 − 반경)에서 (중심 + 반경)까지의 선이다: 가로 선(ry 0).
    const horizontal = draw({ rx: 6, ry: 0 });
    for (let x = 0; x < 64; x += 1) assert.equal(horizontal[24 * 64 + x], x >= 26 && x <= 38 ? 200 : 0, `가로 ${x}`);
    assert.equal(horizontal.reduce((sum, value) => sum + (value ? 1 : 0), 0), 13);
    const vertical = draw({ rx: 0, ry: 4 });
    assert.equal(vertical.reduce((sum, value) => sum + (value ? 1 : 0), 0), 9);
    for (let y = 20; y <= 28; y += 1) assert.equal(vertical[y * 64 + 32], 200);
    // 두 반경이 모두 0 이면 한 점이다.
    const point = draw({ rx: 0, ry: 0 });
    assert.equal(point.reduce((sum, value) => sum + (value ? 1 : 0), 0), 1);
    assert.equal(point[24 * 64 + 32], 200);
});

// 독립 참조 구현(BigInt, 32비트 감김을 매 연산마다 명시): 원문 C++ 의 같은 알고리즘을 다른 표기로 따로 옮겨, 원본 관측이 닿지 못하는 큰 반경에서 두 번째 구간의 결정 변수 갱신까지 대조한다.
// 반환: 그려지는 중심 기준 오프셋 점들의 원문 순서(dx, dy 부호 포함이 아니라 (x, y) 와 네 대칭 부호 조합은 호출자가 펼친다). (2026-11-13 WB04-E377)
function referencePlot(radiusX, radiusY) {
    const M = 1n << 32n;
    const u = (value) => ((value % M) + M) % M;
    const s = (value) => { const v = u(value); return v >= (1n << 31n) ? v - M : v; };
    const sar = (value) => (value >> 1n) | (value & (1n << 31n));
    const rx = u(BigInt(radiusX));
    const ry = u(BigInt(radiusY));
    const a = u(rx * rx);
    const b = u(ry * ry);
    let x = 0n;
    let y = ry;
    let dx = 0n;
    let dy = u(u(a * 2n) * ry);
    let decision = u((a >> 2n) + b - u(a * ry));
    const points = [];
    // 첫 구간.
    while (s(dx - dy) < 0n) {
        points.push([x, y]);
        if (s(decision) >= 0n) { y = u(y - 1n); dy = u(dy - u(a * 2n)); decision = u(decision - dy); }
        x = u(x + 1n);
        dx = u(dx + u(b * 2n));
        decision = u(decision + dx + b);
    }
    const difference = u(a - b);
    decision = u(decision + sar(u(sar(difference) + difference - dx - dy)));
    // 두 번째 구간(do/while).
    do {
        points.push([x, y]);
        if (s(decision) < 0n) { x = u(x + 1n); dx = u(dx + u(b * 2n)); decision = u(decision + dx); }
        y = u(y - 1n);
        dy = u(dy - u(a * 2n));
        decision = u(decision - u(dy - a));
    } while (s(y) >= 0n);
    return points.map(([px, py]) => [Number(s(px)), Number(s(py))]);
}

test('VfxEllipse: 큰 반경(1~24)과 여러 중심·클립에서 독립 BigInt 참조 구현과 화면이 같다(두 번째 구간의 결정 변수 갱신 포함)', () => {
    const clips = [{ left: 0, top: 0, right: 64, bottom: 48 }, { left: 10, top: 8, right: 50, bottom: 40 }];
    const centers = [{ x: 32, y: 24 }, { x: 5, y: 40 }, { x: 60, y: 3 }];
    let compared = 0;
    for (let rx = 1; rx <= 24; rx += 1) {
        for (let ry = 1; ry <= 24; ry += 1) {
            const offsets = referencePlot(rx, ry);
            for (const clip of clips) {
                for (const center of centers) {
                    const expected = new Uint8Array(64 * 48);
                    // 원문 네 대칭점의 순서(+x+y, +x−y, −x+y, −x−y)로 표면과 클립 안에만 찍는다.
                    for (const [ox, oy] of offsets) {
                        for (const [px, py] of [[center.x + ox, center.y + oy], [center.x + ox, center.y - oy], [center.x - ox, center.y + oy], [center.x - ox, center.y - oy]]) {
                            if (px >= Math.max(0, clip.left) && px < Math.min(64, clip.right) && py >= Math.max(0, clip.top) && py < Math.min(48, clip.bottom)) expected[py * 64 + px] = 77;
                        }
                    }
                    const actual = new Uint8Array(64 * 48);
                    drawVfxEllipse(actual, 64, 48, clip, center, rx, ry, 77);
                    assert.deepEqual(actual, expected, `rx ${rx} ry ${ry} 중심 ${center.x},${center.y} 클립 ${clip.left},${clip.top}`);
                    compared += 1;
                }
            }
        }
    }
    assert.equal(compared, 24 * 24 * 6);
    // 음수 반경은 부호 있는 DWORD 비트로 같은 알고리즘을 탄다(원본 EvilRay): 반경 −3,−5 는 2^32−3, 2^32−5 와 같은 경로.
    for (const [rx, ry] of [[-1, -2], [-3, -5], [-2, 4], [6, -1]]) {
        const offsets = referencePlot(rx, ry);
        const expected = new Uint8Array(64 * 48);
        for (const [ox, oy] of offsets) for (const [px, py] of [[32 + ox, 24 + oy], [32 + ox, 24 - oy], [32 - ox, 24 + oy], [32 - ox, 24 - oy]]) if (px >= 0 && px < 64 && py >= 0 && py < 48) expected[py * 64 + px] = 77;
        const actual = new Uint8Array(64 * 48);
        drawVfxEllipse(actual, 64, 48, { left: 0, top: 0, right: 64, bottom: 48 }, { x: 32, y: 24 }, rx, ry, 77);
        assert.deepEqual(actual, expected, `음수 반경 ${rx},${ry}`);
    }
});

test('VfxEllipse: 빈 클립(너비 0 또는 높이 0)은 큰 반경이어도 반복 없이 돌아오고 1×1·1×N 표면과 정확히 맞는 용량은 허용한다', () => {
    const big = (clip) => drawVfxEllipse(new Uint8Array(64 * 48), 64, 48, clip, { x: 32, y: 24 }, 3000000, 3000000, 1);
    // 너비 0 / 높이 0 인 클립은 조기 반환이라 반복 상한 오류를 내지 않는다.
    assert.doesNotThrow(() => big({ left: 10, top: 0, right: 10, bottom: 48 }));
    assert.doesNotThrow(() => big({ left: 0, top: 20, right: 64, bottom: 20 }));
    assert.doesNotThrow(() => big({ left: 30, top: 0, right: 5, bottom: 48 }));
    assert.doesNotThrow(() => big({ left: 0, top: 30, right: 64, bottom: 5 }));
    // 너비/높이가 1 인 유효 클립은 반복 상한에 걸린다(조기 반환이 아님).
    assert.throws(() => big({ left: 10, top: 0, right: 11, bottom: 48 }), RangeError);
    assert.throws(() => big({ left: 0, top: 20, right: 64, bottom: 21 }), RangeError);
    // 표면 용량: 정확히 맞는 길이는 허용하고 모자라면 거부하며 pitch/height 1 은 허용한다.
    const one = new Uint8Array(1);
    drawVfxEllipse(one, 1, 1, { left: 0, top: 0, right: 1, bottom: 1 }, { x: 0, y: 0 }, 0, 0, 9);
    assert.equal(one[0], 9);
    const column = new Uint8Array(5);
    drawVfxEllipse(column, 1, 5, { left: 0, top: 0, right: 1, bottom: 5 }, { x: 0, y: 2 }, 0, 2, 7);
    assert.deepEqual([...column], [7, 7, 7, 7, 7]);
    const row = new Uint8Array(5);
    drawVfxEllipse(row, 5, 1, { left: 0, top: 0, right: 5, bottom: 1 }, { x: 2, y: 0 }, 2, 0, 6);
    assert.deepEqual([...row], [6, 6, 6, 6, 6]);
    assert.throws(() => drawVfxEllipse(new Uint8Array(5), 5, 2, { left: 0, top: 0, right: 5, bottom: 2 }, { x: 0, y: 0 }, 1, 1, 1), RangeError);
    assert.throws(() => drawVfxEllipse(new Uint8Array(5), 6, 1, { left: 0, top: 0, right: 6, bottom: 1 }, { x: 0, y: 0 }, 1, 1, 1), RangeError);
});

test('VfxEllipse: 반경 1~64 전 조합의 독립 참조 구현 대조(산술 시프트 부호 비트와 홀수 차분 포함)', () => {
    let compared = 0;
    for (let rx = 1; rx <= 64; rx += 1) {
        for (let ry = 1; ry <= 64; ry += 1) {
            const offsets = referencePlot(rx, ry);
            const expected = new Uint8Array(200 * 200);
            for (const [ox, oy] of offsets) for (const [px, py] of [[100 + ox, 100 + oy], [100 + ox, 100 - oy], [100 - ox, 100 + oy], [100 - ox, 100 - oy]]) if (px >= 0 && px < 200 && py >= 0 && py < 200) expected[py * 200 + px] = 5;
            const actual = new Uint8Array(200 * 200);
            drawVfxEllipse(actual, 200, 200, { left: 0, top: 0, right: 200, bottom: 200 }, { x: 100, y: 100 }, rx, ry, 5);
            assert.deepEqual(actual, expected, `rx ${rx} ry ${ry}`);
            compared += 1;
        }
    }
    assert.equal(compared, 64 * 64);
});

test('VfxEllipse: 산술 시프트 SAR 1 은 부호 비트만 보존하고 최하위 비트를 섞지 않는다', () => {
    assert.equal(sar(0), 0);
    assert.equal(sar(1), 0);
    assert.equal(sar(2), 1);
    assert.equal(sar(7), 3);
    assert.equal(sar(0x7fffffff), 0x3fffffff);
    // 음수(부호 비트 켜짐)는 위에 1 을 채운다: 0x80000000 → 0xc0000000, 0x80000001 → 0xc0000000, 0xfffffffe → 0xffffffff, 0xffffffff → 0xffffffff.
    assert.equal(sar(0x80000000), 0xc0000000);
    assert.equal(sar(0x80000001), 0xc0000000);
    assert.equal(sar(0xfffffffe), 0xffffffff);
    assert.equal(sar(0xffffffff), 0xffffffff);
    assert.equal(sar(0xfffffff0), 0xfffffff8);
});
