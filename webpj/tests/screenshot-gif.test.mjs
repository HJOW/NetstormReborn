// Shift-F7 화면 저장의 GIF 인코더·팔레트·키 분배·파일 이름을 검사한다(근거: cpppj GifScreenshot.cpp). 인코더는 웹 GIF 해독기로 되돌려 픽셀을 비교한다. 게임을 실행하지 않는다. (2026-11-20 WB04-E1100)
import test from 'node:test';
import assert from 'node:assert/strict';
import { dispatchScreenshotKey, encodeScreenshotGif, screenshotFileName, screenshotPrimaryPalette } from '../www/core/screenshot-gif.js';
import { decodeGif } from '../www/assets/gif.js';
import { InputCode } from '../www/core/input-queue.js';

// 결정적 의사 난수 픽셀 화면이다(행 패딩 칸은 해독 때 무시되어야 하므로 별도 값으로 채운다).
function screen(width, height, pitch, seed, mode) {
    const pixels = new Uint8Array(pitch * height).fill(0xee); let state = seed >>> 0;
    for (let y = 0; y < height; y += 1) for (let x = 0; x < width; x += 1) {
        state = (Math.imul(state, 1664525) + 1013904223) >>> 0;
        pixels[y * pitch + x] = mode === 'flat' ? 7 : mode === 'stripes' ? (x + y) & 15 : state >>> 24;
    }
    return pixels;
}
const rgbTable = () => Uint8Array.from({ length: 768 }, (_, index) => (index * 7) & 0xff);

test('GIF 인코더: 해독기로 되돌리면 색 번호와 크기가 같고 행 패딩은 버려진다', () => {
    for (const [width, height, pitch, mode] of [[1, 1, 1, 'noise'], [5, 3, 8, 'noise'], [64, 48, 64, 'flat'], [200, 120, 256, 'stripes'], [320, 240, 320, 'noise'], [640, 480, 640, 'stripes']]) {
        const pixels = screen(width, height, pitch, width * 31 + height, mode), gif = encodeScreenshotGif(width, height, pitch, pixels, rgbTable());
        assert.equal(String.fromCharCode(...gif.subarray(0, 6)), 'GIF87a'); assert.equal(gif.at(-1), 0x3b);
        const decoded = decodeGif(gif); assert.equal(decoded.width, width); assert.equal(decoded.height, height);
        for (let y = 0; y < height; y += 1) for (let x = 0; x < width; x += 1) assert.equal(decoded.indices[y * width + x], pixels[y * pitch + x], `${width}x${height} (${x},${y})`);
    }
});

test('GIF 인코더: 전역 색표 256칸을 그대로 쓰고 사전이 가득 차 clear 코드가 나오는 큰 잡음 화면도 되돌려진다', () => {
    const rgb = rgbTable(), pixels = screen(800, 600, 800, 99, 'noise'), gif = encodeScreenshotGif(800, 600, 800, pixels, rgb);
    assert.equal(gif[10], 0xf7); assert.deepEqual([...gif.subarray(13, 13 + 768)], [...rgb]);
    const decoded = decodeGif(gif); assert.equal(decoded.indices.length, 800 * 600); assert.deepEqual([...decoded.indices.subarray(0, 4000)], [...pixels.subarray(0, 4000)]); assert.deepEqual([...decoded.indices.subarray(-4000)], [...pixels.subarray(-4000)]);
});

// GIF 한 장의 LZW 코드열을 해독기와 같은 폭 규칙(첫 코드 뒤부터 항목 하나씩 늦게 추가·폭 증가)으로 읽어 돌려준다. 해독 결과와 별개로 clear 위치를 직접 검사하기 위한 도우미이다.
function lzwCodes(gif) {
    const bytes = []; let offset = 13 + 768 + 10 + 1;
    // 서브 블록을 길이 0 이 나올 때까지 이어 붙인다.
    for (let count = gif[offset]; count > 0; count = gif[offset]) { for (let index = 1; index <= count; index += 1) bytes.push(gif[offset + index]); offset += count + 1; }
    const codes = []; let width = 9, avail = 258, bitPosition = 0, first = true;
    // 코드가 끝(257)을 만날 때까지 현재 폭만큼 LSB 부터 읽는다.
    for (;;) {
        let code = 0; for (let bit = 0; bit < width; bit += 1) code |= ((bytes[(bitPosition + bit) >> 3] >> ((bitPosition + bit) & 7)) & 1) << bit;
        bitPosition += width; codes.push(code);
        if (code === 257) break;
        if (code === 256) { width = 9; avail = 258; first = true; continue; }
        if (first) { first = false; continue; }
        avail += 1; if (avail === (1 << width) && width < 12) width += 1;
    }
    return codes;
}

test('GIF 인코더: 사전이 4096 항목에 닿는 바로 그 코드 뒤에 clear 를 내고 사전을 다시 시작한다', () => {
    const codes = lzwCodes(encodeScreenshotGif(800, 600, 800, screen(800, 600, 800, 99, 'noise'), rgbTable()));
    assert.equal(codes[0], 256); assert.equal(codes.at(-1), 257);
    const clears = codes.flatMap((code, index) => (code === 256 ? [index] : []));
    assert.ok(clears.length >= 3, `clear ${clears.length}`);
    // 항목 258..4095 를 모두 채운 뒤(3838번 추가) 그 다음 코드 하나를 더 쓰고 clear 가 나온다: 처음 clear 사이 데이터 코드는 3839 개다.
    assert.equal(clears[1] - clears[0] - 1, 3839); assert.equal(clears[2] - clears[1] - 1, 3839);
});

test('GIF 인코더: 잘못된 크기·행 간격·색표는 거부한다', () => {
    const ok = encodeScreenshotGif(4, 2, 4, new Uint8Array(8), rgbTable()); assert.ok(ok.length > 20);
    for (const args of [[0, 2, 4], [4, 0, 4], [65536, 2, 65536], [4, 2, 3], [4, 2, 4, new Uint8Array(7)], [4, 2, 4, new Uint8Array(8), new Uint8Array(767)]]) {
        assert.throws(() => encodeScreenshotGif(args[0], args[1], args[2], args[3] ?? new Uint8Array(8), args[4] ?? rgbTable()), RangeError, JSON.stringify(args.slice(0, 3)));
    }
});

test('primary 팔레트: 색마다 +24 의 R·G·B double 에 255 를 곱해 절삭하고 범위 밖은 거부한다', () => {
    const primary = new Uint8Array(256 * 56), view = new DataView(primary.buffer);
    for (let color = 0; color < 256; color += 1) { view.setFloat64(color * 56 + 24, color / 255, true); view.setFloat64(color * 56 + 32, 0.5, true); view.setFloat64(color * 56 + 40, 1, true); }
    const rgb = screenshotPrimaryPalette(primary); assert.equal(rgb.length, 768);
    assert.deepEqual([...rgb.subarray(0, 3)], [0, 127, 255]); assert.deepEqual([...rgb.subarray(3 * 200, 3 * 200 + 3)], [200, 127, 255]);
    view.setFloat64(5 * 56 + 24, 1.5, true); assert.throws(() => screenshotPrimaryPalette(primary), RangeError);
    assert.throws(() => screenshotPrimaryPalette(new Uint8Array(100)), /primary/u);
});

test('키 분배: Shift-F7 만 저장하고 Tell 을 부르며 Control·뗌·Shift 없음·다른 키는 소비하지 않는다', () => {
    const calls = []; const save = () => calls.push('save'), tell = (name) => calls.push(`tell:${name}`);
    assert.equal(dispatchScreenshotKey({ code: 0x760000 | InputCode.shift }, save, tell), true); assert.deepEqual(calls, ['save', 'tell:TellGifWritten']);
    calls.length = 0;
    for (const code of [0x760000, 0x760000 | InputCode.shift | InputCode.control, 0x760000 | InputCode.shift | InputCode.release, 0x750000 | InputCode.shift]) assert.equal(dispatchScreenshotKey({ code }, save, tell), false, code.toString(16));
    assert.deepEqual(calls, []);
    // 도달한 장치가 없으면 거부한다(저장 장치 누락은 Tell 전에 실패한다).
    assert.throws(() => dispatchScreenshotKey({ code: 0x760000 | InputCode.shift }, null, tell), /스크린샷/u);
});

test('파일 이름: pic<signed 번호>.gif', () => {
    assert.equal(screenshotFileName(0), 'pic0.gif'); assert.equal(screenshotFileName(12), 'pic12.gif'); assert.equal(screenshotFileName(0xffffffff), 'pic-1.gif');
});
