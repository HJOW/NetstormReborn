// 원본 raw 슬롯·관측 fixture 대조 검사가 함께 쓰는 보조 함수 모음이다(탭 행 읽기·float 비트 변환·리틀엔디언 읽기/쓰기·Adler-32·FNV-32).
// 사용: import { readRows, adler32, bitsToFloat } from './raw-support.mjs'. (2026-11-05 WB04-D2: Unpop 처리기·삭제기·미니맵 대조 검사와 함께 추가)
import fs from 'node:fs';

// 탭 구분 행을 읽는다(주석·빈 줄 제외, CRLF 허용). 파일이 없으면 null 이다. (2026-11-05 WB04-D2)
export function readRows(file) {
    if (!fs.existsSync(file)) return null;
    return fs.readFileSync(file, 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && line[0] !== '#').map((line) => line.split('\t'));
}

// 단정도 float 비트(uint32)를 수로 바꾼다. (2026-11-05 WB04-D2)
export function bitsToFloat(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, Number(bits) >>> 0, true);
    return view.getFloat32(0, true);
}

// 수를 단정도 float 비트(uint32)로 바꾼다. (2026-11-05 WB04-D2)
export function floatToBits(value) {
    const view = new DataView(new ArrayBuffer(4));
    view.setFloat32(0, value, true);
    return view.getUint32(0, true);
}

// 바이트 배열에 리틀엔디언 값을 쓴다(지정 폭만, cpppj Put). (2026-11-05 WB04-D2)
export function put(raw, offset, value, width = 4) {
    // 낮은 바이트부터 지정 폭만 쓴다.
    for (let index = 0; index < width; index += 1) raw[offset + index] = Math.floor(value / 2 ** (8 * index)) & 0xff;
}

// 바이트 배열의 리틀엔디언 값을 읽는다(부호 없는 수). (2026-11-05 WB04-D2)
export function get(raw, offset, width = 4) {
    let value = 0;
    // 낮은 바이트부터 조합한다.
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value;
}

// zlib 과 같은 Adler-32 이다(5552바이트마다 나머지, 결과는 매 바이트 나머지와 같음). (2026-11-05 WB04-D2)
export function adler32(bytes) {
    let a = 1;
    let b = 0;
    // 묶음마다 두 합의 나머지를 구한다.
    for (let start = 0; start < bytes.length; start += 5552) {
        const end = Math.min(start + 5552, bytes.length);
        // 묶음 안에서는 나머지 없이 더한다.
        for (let index = start; index < end; index += 1) {
            a += bytes[index];
            b += a;
        }
        a %= 65521;
        b %= 65521;
    }
    return ((b << 16) | a) >>> 0;
}

// 순서 보존 FNV-1a 32비트 해시이다(cpppj Fnv). (2026-11-05 WB04-D2)
export function fnv32(bytes) {
    let value = 2166136261;
    // 모든 바이트를 순서대로 섞는다.
    for (let index = 0; index < bytes.length; index += 1) value = Math.imul(value ^ bytes[index], 16777619) >>> 0;
    return value >>> 0;
}

// 32비트 값 목록을 리틀엔디언 바이트 배열로 만든다. (2026-11-05 WB04-D2)
export function dwordBytes(values) {
    const bytes = new Uint8Array(values.length * 4);
    const view = new DataView(bytes.buffer);
    // 각 값을 uint32 로 낮은 바이트부터 넣는다.
    values.forEach((value, index) => view.setUint32(index * 4, value >>> 0, true));
    return bytes;
}
