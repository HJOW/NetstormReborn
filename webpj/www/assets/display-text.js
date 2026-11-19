// Session의 고정 바이트 이름과 웹 문자열 사이에 cpppj 표시 부호화를 제공한다.
// 사용: 같은 프로세스의 codec.encode(text), codec.decode(bytes). 파일·경로의 원문 부호화와 구별한다.
import { decodeWindows1252, encodeOriginalText } from './text-encoding.js';

// NUL·줄바꿈·ESC와 겹치지 않는 25진 숫자와 확장 문자 시작 바이트다.
const DIGITS = Object.freeze([1, 2, 3, 4, 5, 6, 7, 8, 11, 12, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 28, 29, 30]);
// 확장 4바이트 시퀀스의 선두 값이다.
const LEAD = 31;
// 순서는 cpppj OriginalText.cpp의 한국어·일본어 정적 인덱스와 같다.
const RANGES = Object.freeze([[0xac00, 0xd7a3], [0x3131, 0x318e], [0x3000, 0x303f], [0xff01, 0xff5e], [0x3040, 0x30ff], [0xff61, 0xff9f]]);
// 정적 구간의 끝이자 동적 한자의 첫 인덱스다.
const STATIC_COUNT = 11679;
// 25진 숫자 세 개로 표현하는 전체 인덱스 수다.
const CAPACITY = 25 ** 3;

// 인덱스를 원본 4바이트 표현으로 바꾼다.
function sequence(index) { return [LEAD, DIGITS[Math.floor(index / 625)], DIGITS[Math.floor(index / 25) % 25], DIGITS[index % 25]]; }

// 한 프로세스의 확장 한자 배정표와 표시 인코더를 소유한다.
export class DisplayTextCodec {
    // 정적 구간 뒤에 처음 사용한 순서대로 배정한 한자와 역방향 표다.
    #characters = []; #indices = new Map();

    // 현재 한자 배정까지 포함해 코드 포인트의 확장 인덱스를 얻는다. 해당하지 않으면 null이다.
    #index(code) {
        let base = 0;
        // 앞 구간의 길이를 더해 cpppj와 같은 연속 인덱스를 만든다.
        for (const [first, last] of RANGES) {
            if (code >= first && code <= last) return base + code - first;
            base += last - first + 1;
        }
        if (code < 0x4e00 || code > 0x9fff) return null;
        let index = this.#indices.get(code);
        if (index === undefined) {
            if (STATIC_COUNT + this.#characters.length >= CAPACITY) return null;
            index = STATIC_COUNT + this.#characters.length; this.#characters.push(code); this.#indices.set(code, index);
        }
        return index;
    }

    // 현재 배정표의 인덱스를 코드 포인트로 되돌린다. 미배정 값은 null이다.
    #code(index) {
        // 정적 구간을 앞에서부터 소비하고 남은 값만 동적 한자 표에 쓴다.
        for (const [first, last] of RANGES) {
            const length = last - first + 1;
            if (index < length) return first + index;
            index -= length;
        }
        return this.#characters[index] ?? null;
    }

    // 원본 1252 문자와 확장 문자를 바이트로 만든다. 범위 밖 문자·잘못된 UTF-16은 거부한다.
    encode(text) {
        const output = [];
        // 입력 문자 순서가 동적 한자의 최초 배정 순서다.
        for (const character of String(text ?? '')) {
            const code = character.codePointAt(0);
            if (code > 0xffff || (code >= 0xd800 && code <= 0xdfff)) throw new RangeError('표시 부호화의 유효하지 않은 BMP 문자');
            if (code < 0x80 || (code >= 0xa0 && code <= 0xff)) output.push(code);
            else {
                const index = this.#index(code);
                if (index === null) output.push(...encodeOriginalText(character));
                else output.push(...sequence(index));
            }
        }
        return Uint8Array.from(output);
    }

    // 정확한 확장 시퀀스만 해독하고 불완전하거나 미배정된 바이트는 원문 1252로 보존한다.
    decode(bytes) {
        if (!(bytes instanceof Uint8Array)) throw new TypeError('표시 해독의 실제 바이트 배열 누락');
        if (bytes.length >= 3 && bytes[0] === 0xef && bytes[1] === 0xbb && bytes[2] === 0xbf) return new TextDecoder('utf-8').decode(bytes.subarray(3));
        let output = '';
        // 유효한 4바이트는 한 번에 소비하고 나머지는 한 바이트씩 읽는다.
        for (let at = 0; at < bytes.length; at += 1) {
            if (bytes[at] === LEAD && at + 3 < bytes.length) {
                const high = DIGITS.indexOf(bytes[at + 1]), middle = DIGITS.indexOf(bytes[at + 2]), low = DIGITS.indexOf(bytes[at + 3]);
                if (high >= 0 && middle >= 0 && low >= 0) {
                    const code = this.#code(high * 625 + middle * 25 + low);
                    if (code !== null) { output += String.fromCodePoint(code); at += 3; continue; }
                }
            }
            output += decodeWindows1252(bytes.subarray(at, at + 1));
        }
        return output;
    }
}

// cpppj의 프로세스 전역 한자 배정과 같이 모든 앱 인스턴스가 같은 표를 사용한다.
export const displayTextCodec = new DisplayTextCodec();
