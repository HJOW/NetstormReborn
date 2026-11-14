// 미니맵 Draw→평면 복사→카메라 외곽선(원본 00480660 와 8BIT 출력)의 원본 10.78 독립 관측 120행 대조: 순수 기하 계획(블릿 원점/소스 평면 주소/크기/카메라 외곽)과 GumpPixelTarget 의 rawBitmap/linesClipped 출력이
// 전체 192×160 표면 바이트와 일치하는지, 할당 밖 보이는 소스 BYTE(가드) 읽기 주소 집합이 실제 CPU 의 읽기 구간과 일치하는지를 cpppj `MinimapBlitTests`(IndependentNativeFullSurfaceAndGuardReads)와 같은 입력 구성으로 비교한다.
// 입력 열(bitmap/가드 입력 패턴·클립·카메라/뷰·배경/잉크)만 공급하고 관측 열은 비교에만 쓴다. 원본 게임을 실행하지 않는다. (2026-11-15 WB04-E494)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { GumpDrawState, GumpPixelTarget, setGumpDrawClip } from '../www/render/gump-draw.js';
import { MinimapGeometryState, planMinimapDraw } from '../www/ui/minimap-geometry.js';

// 독립 native 의 격리 대상 표면 크기와 열 수, bitmap 한 변의 바이트 수이다.
const WIDTH = 192; const HEIGHT = 160; const FIELDS = 33; const BITMAP_BYTES = 0x4000;
// 십진 입력과 float32 비트 보기이다.
const number = (text) => Number(text); const f32 = new Float32Array(1); const u32 = new Uint32Array(f32.buffer); const floatOf = (bits) => { u32[0] = bits >>> 0; return f32[0]; };
// Python 의 signed 우측 이동과 같은 바닥 나눗셈으로 원시 입력 BYTE 만 복원한다(음수 guard 입력에서도 같은 입력 메모리 패턴).
function pattern(index, seed) { const row = Math.floor(index / 128); return (index * 73 + row * 19 + seed) & 0xff; }
// 독립 원문 출력의 변경 구간(`offset:hex;...`)을 전체 배경 위에 무손실 복원한다.
function nativePixels(runs, background) {
    const result = new Uint8Array(WIDTH * HEIGHT).fill(background);
    if (runs === '-') return result;
    for (const run of runs.split(';')) { const pair = run.split(':'); assert.equal(pair.length, 2); assert.equal(pair[1].length % 2, 0); const first = Number(pair[0]); const bytes = Buffer.from(pair[1], 'hex'); assert.ok(first + bytes.length <= result.length); result.set(bytes, first); }
    return result;
}
// native 읽기 구간에서 bitmap 할당 밖 실제 BYTE 주소 집합만 추출한다(앞뒤 guard 만 별도 장치에 해당).
function nativeGuardReads(ranges) {
    const result = new Set(); if (ranges === '-') return result;
    for (const range of ranges.split(';')) { const pair = range.split(':'); assert.equal(pair.length, 2); const first = Number(pair[0]); const length = Number(pair[1]); for (let byte = first; byte < first + length; byte += 1) if (byte < 0 || byte >= BITMAP_BYTES) result.add(byte); }
    return result;
}

test('MinimapBlit 원본 10.78 120행: 블릿 계획과 전체 192×160 표면·가드 읽기 주소 집합·클립 전역 불변이 일치한다', () => {
    const table = fs.readFileSync(new URL('../../cpppj/tests/fixtures/minimap-blit-x86.tsv', import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && !line.startsWith('#')).map((line) => line.split('\t'));
    assert.equal(table.length, 120); const failures = []; let guardRows = 0;
    for (const [index, f] of table.entries()) {
        assert.equal(f.length, FIELDS, `행 ${index}`);
        const width = floatOf(number(f[2]) >>> 0); const height = floatOf(number(f[3]) >>> 0);
        const geometry = new MinimapGeometryState(); geometry.width = width; geometry.height = height; geometry.cropX = number(f[4]) | 0; geometry.cropY = number(f[5]) | 0; geometry.scrolling = 0;
        const view = { camera: [number(f[12]) >>> 0, number(f[13]) >>> 0], viewport: { left: number(f[14]), top: number(f[15]), right: number(f[14]) + number(f[16]), bottom: number(f[15]) + number(f[17]) }, pixelsX: number(f[18]), pixelsY: number(f[19]) };
        const destination = { x: number(f[6]), y: number(f[7]) };
        const plan = planMinimapDraw(geometry, view, destination); const nativeBlit = f[28].split(':'); const nativeFrame = f[29].split(':'); const problems = [];
        if (nativeBlit.length !== 8 || nativeFrame.length !== 5) { failures.push(`${index}: 원문 열 폭`); continue; }
        const sourceBase = plan.source.y * 128 + plan.source.x;
        if (plan.destination.x !== number(nativeBlit[0]) || plan.destination.y !== number(nativeBlit[1]) || sourceBase !== number(nativeBlit[2])) problems.push('blit-origin');
        if (number(nativeBlit[3]) !== 128 || nativeBlit[4] !== '0' || nativeBlit[5] !== '0' || plan.size.x !== number(nativeBlit[6]) || plan.size.y !== number(nativeBlit[7])) problems.push('blit-size');
        if (plan.camera.left !== number(nativeFrame[0]) || plan.camera.top !== number(nativeFrame[1]) || plan.camera.right !== number(nativeFrame[2]) || plan.camera.bottom !== number(nativeFrame[3])) problems.push('frame');
        const background = number(f[22]) & 0xff; const ink = number(f[23]) >>> 0; if (ink !== number(nativeFrame[4])) problems.push('ink');
        const bitmap = new Uint8Array(BITMAP_BYTES); for (let byte = 0; byte < bitmap.length; byte += 1) bitmap[byte] = pattern(byte, number(f[20]));
        const pixels = new Uint8Array(WIDTH * HEIGHT).fill(background); const state = new GumpDrawState(); const clip = { left: number(f[8]), top: number(f[9]), right: number(f[10]), bottom: number(f[11]) }; setGumpDrawClip(state, clip);
        const target = new GumpPixelTarget(pixels, WIDTH, HEIGHT, state); const bounds = { left: destination.x, top: destination.y, right: destination.x + Math.trunc(width), bottom: destination.y + Math.trunc(height) }; const guardReads = new Set();
        target.rawBitmap(bitmap, 128, BigInt(sourceBase), plan.size, plan.destination, bounds, (offset) => { guardReads.add(Number(offset)); return pattern(Number(offset), number(f[21])); });
        const left = plan.camera.left; const top = plan.camera.top; const right = plan.camera.right - 1; const bottom = plan.camera.bottom - 1;
        const line = (x1, y1, x2, y2) => ({ first: { x: x1, y: y1 }, last: { x: x2, y: y2 }, color: ink });
        target.linesClipped([line(left, top, right, top), line(left, bottom, right, bottom), line(left, top, left, bottom), line(right, top, right, bottom)], bounds);
        if (Buffer.compare(Buffer.from(pixels), Buffer.from(nativePixels(f[25], background))) !== 0) problems.push('pixels');
        const expectedGuards = nativeGuardReads(f[26]); if (guardReads.size !== expectedGuards.size || ![...guardReads].every((value) => expectedGuards.has(value))) problems.push('guard');
        if ((guardReads.size === 0) !== (number(f[27]) === 0)) problems.push('guard-flag'); if (guardReads.size > 0) guardRows += 1;
        if (state.clip.left !== clip.left || state.clip.top !== clip.top || state.clip.right !== clip.right || state.clip.bottom !== clip.bottom) problems.push('clip');
        if (f[24] !== '1' || !(number(f[31]) > 0) || f[32] !== '0') problems.push('completion');
        if (problems.length) failures.push(`${index}/${f[0]}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
    assert.equal(guardRows, 40);
});
