// webpj Node 검사의 공용 도구 모듈이다. 저장소 경로, 데이터 파일 읽기, 메인 메뉴 자원 구성, 간단한 PNG 해독을 제공한다.
// 검사는 브라우저 없이 www 의 순수 모듈(DOM 미사용)만 불러 실행한다.
// (2026-10-23 WB03-M1: 메인 메뉴 검사를 위해 추가)
import fs from 'node:fs';
import path from 'node:path';
import zlib from 'node:zlib';
import { fileURLToPath } from 'node:url';

// webpj 폴더의 절대 경로이다. (2026-10-23 WB03-M1)
export const WEBPJ_ROOT = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');

// 저장소 루트의 절대 경로이다. (2026-10-23 WB03-M1)
export const REPO_ROOT = path.resolve(WEBPJ_ROOT, '..');

// 웹 공개 데이터 폴더(www/data)의 절대 경로이다. (2026-10-23 WB03-M1)
export const DATA_ROOT = path.join(WEBPJ_ROOT, 'www', 'data');

// 데이터 폴더 기준 상대 경로의 파일을 Uint8Array 로 읽는다. (2026-10-23 WB03-M1)
export function readData(relativePath) {
    return new Uint8Array(fs.readFileSync(path.join(DATA_ROOT, ...relativePath.split('/'))));
}

// 경로 목록의 데이터 파일을 읽어 경로 → Uint8Array Map 으로 돌려준다(브라우저 적재기 결과와 같은 모양). (2026-10-23 WB03-M1)
export function readDataMap(relativePaths) {
    return new Map(relativePaths.map((relativePath) => [relativePath, readData(relativePath)]));
}

// PNG 의 각 행 필터(0~4)를 되돌릴 때 쓰는 Paeth 예측 함수이다. (2026-10-23 WB03-M1)
function paeth(a, b, c) {
    const p = a + b - c;
    const pa = Math.abs(p - a);
    const pb = Math.abs(p - b);
    const pc = Math.abs(p - c);
    if (pa <= pb && pa <= pc) return a;
    return pb <= pc ? b : c;
}

// 8비트 RGB/RGBA, 비인터레이스 PNG 를 해독해 { width, height, rgb(Uint8Array) } 를 돌려준다.
// 원본 스크린샷(screenShots/*.png) 대조에만 쓰는 최소 구현이다. 다른 형식은 오류를 던진다. (2026-10-23 WB03-M1)
export function decodePng(filePath) {
    const bytes = fs.readFileSync(filePath);
    let offset = 8;
    let width = 0;
    let height = 0;
    let channels = 0;
    const idat = [];
    // 청크를 IEND 까지 차례로 읽는다.
    while (offset < bytes.length) {
        const length = bytes.readUInt32BE(offset);
        const type = bytes.toString('latin1', offset + 4, offset + 8);
        const data = bytes.subarray(offset + 8, offset + 8 + length);
        if (type === 'IHDR') {
            width = data.readUInt32BE(0);
            height = data.readUInt32BE(4);
            const bitDepth = data[8];
            const colorType = data[9];
            const interlace = data[12];
            if (bitDepth !== 8 || interlace !== 0 || (colorType !== 2 && colorType !== 6)) throw new Error('지원하지 않는 PNG 형식입니다.');
            channels = colorType === 6 ? 4 : 3;
        } else if (type === 'IDAT') {
            idat.push(data);
        } else if (type === 'IEND') {
            break;
        }
        offset += 12 + length;
    }
    const raw = zlib.inflateSync(Buffer.concat(idat));
    const stride = width * channels;
    const pixels = new Uint8Array(height * stride);
    // 각 행의 필터를 되돌린다.
    for (let y = 0; y < height; y += 1) {
        const filter = raw[y * (stride + 1)];
        const source = y * (stride + 1) + 1;
        const target = y * stride;
        // 행의 각 바이트에 필터 예측값을 더한다.
        for (let x = 0; x < stride; x += 1) {
            const left = x >= channels ? pixels[target + x - channels] : 0;
            const up = y > 0 ? pixels[target - stride + x] : 0;
            const upLeft = y > 0 && x >= channels ? pixels[target - stride + x - channels] : 0;
            const value = raw[source + x];
            const predictor = filter === 0 ? 0 : filter === 1 ? left : filter === 2 ? up : filter === 3 ? (left + up) >> 1 : paeth(left, up, upLeft);
            pixels[target + x] = (value + predictor) & 255;
        }
    }
    const rgb = new Uint8Array(width * height * 3);
    // 알파 채널을 버리고 RGB 만 남긴다.
    for (let i = 0; i < width * height; i += 1) {
        rgb[i * 3] = pixels[i * channels];
        rgb[i * 3 + 1] = pixels[i * channels + 1];
        rgb[i * 3 + 2] = pixels[i * channels + 2];
    }
    return { width, height, rgb };
}
