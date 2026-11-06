// cpppj 오라클 비교에 쓰는 그림 파일 도구이다. 웹 런타임(www)에서는 쓰지 않는다.
//  - BMP: cpppj `Client::Capture`(platform::WriteBitmap)가 쓰는 122바이트 머리의 32비트 BGRA BMP 를 읽고 같은 형식으로 쓴다.
//    읽기는 24/32비트, 위→아래(높이 음수)·아래→위(높이 양수) 무압축/BITFIELDS 를 받는다.
//  - PNG: 색이 256개 이하인 RGB 그림을 무손실 팔레트 PNG(색 형식 3, 8비트)로 쓰고, 색 형식 2·3·6(8비트, 비월 없음) PNG 를 읽는다.
//    cpppj 캡처는 8비트 팔레트 화면을 RGB 로 바꾼 것이라 색이 256개 이하이므로, 저장소에는 3MB BMP 대신 이 PNG 를 둔다.
// 그림 값은 { width, height, rgb: Uint8Array(width*height*3) } 이다(알파는 버린다).
// (2026-11-07 WB02-H: LEFT_JOBS.webpj.md 3.2절 순서2의 cpppj 오라클 수집·비교를 위해 추가)
import zlib from 'node:zlib';

// PNG 서명 8바이트이다. (2026-11-07 WB02-H)
const PNG_SIGNATURE = Buffer.from([0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a]);

// cpppj platform::WriteBitmap 의 머리 크기(BITMAPFILEHEADER 14 + BITMAPV4HEADER 108)이다. (2026-11-07 WB02-H)
export const CPP_BITMAP_HEADER_SIZE = 122;

// CRC-32(IEEE) 표이다. PNG 조각 검사값에 쓴다. (2026-11-07 WB02-H)
const CRC_TABLE = (() => {
    const table = new Uint32Array(256);
    // 바이트 값마다 다항식 나눗셈 나머지를 미리 계산한다.
    for (let value = 0; value < 256; value += 1) {
        let crc = value;
        // 8비트를 한 비트씩 민다.
        for (let bit = 0; bit < 8; bit += 1) crc = crc & 1 ? 0xedb88320 ^ (crc >>> 1) : crc >>> 1;
        table[value] = crc >>> 0;
    }
    return table;
})();

// 바이트 배열의 CRC-32 를 계산한다. (2026-11-07 WB02-H)
function crc32(bytes) {
    let crc = 0xffffffff;
    // 바이트마다 표를 따라 갱신한다.
    for (const byte of bytes) crc = CRC_TABLE[(crc ^ byte) & 0xff] ^ (crc >>> 8);
    return (crc ^ 0xffffffff) >>> 0;
}

// BMP 파일 바이트를 RGB 그림으로 읽는다. 지원하지 않는 형식이면 Error 를 던진다. (2026-11-07 WB02-H)
export function readBmp(bytes) {
    const data = Buffer.from(bytes.buffer, bytes.byteOffset, bytes.byteLength);
    if (data.length < 54 || data.toString('latin1', 0, 2) !== 'BM') throw new Error('BMP 서명이 아님');
    const offset = data.readUInt32LE(10);
    const width = data.readInt32LE(18);
    const rawHeight = data.readInt32LE(22);
    const bits = data.readUInt16LE(28);
    const compression = data.readUInt32LE(30);
    if (width <= 0 || rawHeight === 0 || (bits !== 24 && bits !== 32) || (compression !== 0 && compression !== 3)) throw new Error(`지원하지 않는 BMP: ${width}x${rawHeight} ${bits}bit 압축${compression}`);
    const height = Math.abs(rawHeight);
    const topDown = rawHeight < 0;
    const stride = Math.ceil((width * bits) / 32) * 4;
    if (offset + stride * height > data.length) throw new Error('BMP 픽셀 자료가 잘림');
    const rgb = new Uint8Array(width * height * 3);
    const step = bits / 8;
    // 행마다 BGR(A) 를 RGB 로 옮긴다.
    for (let y = 0; y < height; y += 1) {
        const row = offset + (topDown ? y : height - 1 - y) * stride;
        // 한 행의 픽셀을 옮긴다.
        for (let x = 0; x < width; x += 1) {
            const source = row + x * step;
            const target = (y * width + x) * 3;
            rgb[target] = data[source + 2];
            rgb[target + 1] = data[source + 1];
            rgb[target + 2] = data[source];
        }
    }
    return { width, height, rgb };
}

// RGB 그림을 cpppj platform::WriteBitmap 과 같은 바이트(122바이트 머리, 32비트 BGRA, 위→아래, 알파 255)로 만든다. (2026-11-07 WB02-H)
export function encodeCppBitmap({ width, height, rgb }) {
    const pixelSize = width * height * 4;
    const bytes = Buffer.alloc(CPP_BITMAP_HEADER_SIZE + pixelSize);
    bytes.write('BM', 0, 'latin1');
    bytes.writeUInt32LE(bytes.length, 2);
    bytes.writeUInt32LE(CPP_BITMAP_HEADER_SIZE, 10);
    bytes.writeUInt32LE(108, 14);
    bytes.writeUInt32LE(width, 18);
    bytes.writeInt32LE(-height, 22);
    bytes.writeUInt16LE(1, 26);
    bytes.writeUInt16LE(32, 28);
    bytes.writeUInt32LE(3, 30);
    bytes.writeUInt32LE(pixelSize, 34);
    bytes.writeUInt32LE(0x00ff0000, 54);
    bytes.writeUInt32LE(0x0000ff00, 58);
    bytes.writeUInt32LE(0x000000ff, 62);
    bytes.writeUInt32LE(0xff000000, 66);
    bytes.writeUInt32LE(0x73524742, 70);
    // 픽셀마다 BGRA 로 쓴다.
    for (let index = 0; index < width * height; index += 1) {
        const target = CPP_BITMAP_HEADER_SIZE + index * 4;
        bytes[target] = rgb[index * 3 + 2];
        bytes[target + 1] = rgb[index * 3 + 1];
        bytes[target + 2] = rgb[index * 3];
        bytes[target + 3] = 255;
    }
    return bytes;
}

// PNG 조각 하나(길이·종류·자료·CRC)를 만든다. (2026-11-07 WB02-H)
function chunk(type, payload) {
    const head = Buffer.alloc(8);
    head.writeUInt32BE(payload.length, 0);
    head.write(type, 4, 'latin1');
    const crc = Buffer.alloc(4);
    crc.writeUInt32BE(crc32(Buffer.concat([head.subarray(4), payload])), 0);
    return Buffer.concat([head, payload, crc]);
}

// RGB 그림을 팔레트 PNG(색 형식 3)로 만든다. 팔레트는 색 값(0xRRGGBB) 오름차순이라 같은 그림은 언제나 같은 바이트가 된다.
// 색이 256개를 넘으면 Error 를 던진다. zlib 압축 수준 9 를 쓴다. (2026-11-07 WB02-H)
export function encodeIndexedPng({ width, height, rgb }) {
    const colors = new Set();
    // 쓰인 색을 모은다.
    for (let index = 0; index < width * height; index += 1) colors.add((rgb[index * 3] << 16) | (rgb[index * 3 + 1] << 8) | rgb[index * 3 + 2]);
    if (colors.size > 256) throw new Error(`색 ${colors.size}개라 팔레트 PNG 로 쓸 수 없음`);
    const palette = [...colors].sort((a, b) => a - b);
    const lookup = new Map(palette.map((color, index) => [color, index]));
    const raw = Buffer.alloc((width + 1) * height);
    // 행마다 필터 0 바이트와 팔레트 번호를 쓴다.
    for (let y = 0; y < height; y += 1) {
        raw[y * (width + 1)] = 0;
        // 한 행의 픽셀 번호를 쓴다.
        for (let x = 0; x < width; x += 1) {
            const source = (y * width + x) * 3;
            raw[y * (width + 1) + 1 + x] = lookup.get((rgb[source] << 16) | (rgb[source + 1] << 8) | rgb[source + 2]);
        }
    }
    const header = Buffer.alloc(13);
    header.writeUInt32BE(width, 0);
    header.writeUInt32BE(height, 4);
    header[8] = 8;
    header[9] = 3;
    const plte = Buffer.alloc(palette.length * 3);
    palette.forEach((color, index) => { plte[index * 3] = color >>> 16; plte[index * 3 + 1] = (color >>> 8) & 0xff; plte[index * 3 + 2] = color & 0xff; });
    return Buffer.concat([PNG_SIGNATURE, chunk('IHDR', header), chunk('PLTE', plte), chunk('IDAT', zlib.deflateSync(raw, { level: 9 })), chunk('IEND', Buffer.alloc(0))]);
}

// Paeth 예측값이다(PNG 필터 4). (2026-11-07 WB02-H)
function paeth(a, b, c) {
    const p = a + b - c;
    const pa = Math.abs(p - a);
    const pb = Math.abs(p - b);
    const pc = Math.abs(p - c);
    return pa <= pb && pa <= pc ? a : pb <= pc ? b : c;
}

// PNG 바이트를 RGB 그림으로 읽는다. 8비트 색 형식 2(RGB)·3(팔레트)·6(RGBA, 알파 버림)과 비월 없음만 받는다. (2026-11-07 WB02-H)
export function readPng(bytes) {
    const data = Buffer.from(bytes.buffer, bytes.byteOffset, bytes.byteLength);
    if (!data.subarray(0, 8).equals(PNG_SIGNATURE)) throw new Error('PNG 서명이 아님');
    let offset = 8;
    let width = 0;
    let height = 0;
    let colorType = -1;
    let palette = null;
    const idat = [];
    // 조각을 차례로 읽는다.
    while (offset + 8 <= data.length) {
        const length = data.readUInt32BE(offset);
        const type = data.toString('latin1', offset + 4, offset + 8);
        const payload = data.subarray(offset + 8, offset + 8 + length);
        if (type === 'IHDR') {
            width = payload.readUInt32BE(0);
            height = payload.readUInt32BE(4);
            colorType = payload[9];
            if (payload[8] !== 8 || payload[12] !== 0 || ![2, 3, 6].includes(colorType)) throw new Error('지원하지 않는 PNG 형식');
        } else if (type === 'PLTE') palette = payload;
        else if (type === 'IDAT') idat.push(payload);
        else if (type === 'IEND') break;
        offset += 12 + length;
    }
    const channels = colorType === 2 ? 3 : colorType === 6 ? 4 : 1;
    const stride = width * channels;
    const raw = zlib.inflateSync(Buffer.concat(idat));
    const pixels = Buffer.alloc(stride * height);
    // 행마다 필터를 되돌린다.
    for (let y = 0; y < height; y += 1) {
        const filter = raw[y * (stride + 1)];
        const source = raw.subarray(y * (stride + 1) + 1, (y + 1) * (stride + 1));
        // 행의 바이트마다 필터 종류에 맞게 복원한다.
        for (let x = 0; x < stride; x += 1) {
            const left = x >= channels ? pixels[y * stride + x - channels] : 0;
            const up = y > 0 ? pixels[(y - 1) * stride + x] : 0;
            const corner = x >= channels && y > 0 ? pixels[(y - 1) * stride + x - channels] : 0;
            const predict = filter === 0 ? 0 : filter === 1 ? left : filter === 2 ? up : filter === 3 ? (left + up) >> 1 : paeth(left, up, corner);
            pixels[y * stride + x] = (source[x] + predict) & 0xff;
        }
    }
    const rgb = new Uint8Array(width * height * 3);
    // 픽셀마다 RGB 로 바꾼다.
    for (let index = 0; index < width * height; index += 1) {
        if (channels === 1) {
            const entry = pixels[index] * 3;
            rgb.set([palette[entry], palette[entry + 1], palette[entry + 2]], index * 3);
        } else {
            rgb.set(pixels.subarray(index * channels, index * channels + 3), index * 3);
        }
    }
    return { width, height, rgb };
}

// 8비트 색 번호 화면과 팔레트(256×3 RGB)를 RGB 그림으로 바꾼다. 웹 IndexedSurface 결과를 cpppj 캡처와 같은 색 공간으로 비교할 때 쓴다. (2026-11-07 WB02-H)
export function indexedToRgb({ width, height, pixels }, paletteRgb) {
    const rgb = new Uint8Array(width * height * 3);
    // 픽셀마다 팔레트 색을 옮긴다.
    for (let index = 0; index < width * height; index += 1) {
        const entry = pixels[index] * 3;
        rgb[index * 3] = paletteRgb[entry];
        rgb[index * 3 + 1] = paletteRgb[entry + 1];
        rgb[index * 3 + 2] = paletteRgb[entry + 2];
    }
    return { width, height, rgb };
}
