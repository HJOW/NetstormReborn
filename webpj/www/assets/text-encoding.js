// 원본 게임 글꼴(.chfnt)이 쓰는 Windows-1252 바이트와 JavaScript 문자열 사이의 변환 모듈이다.
// 원본 글꼴은 바이트 코드 0~255 로 글리프를 찾으므로, 화면에 그릴 문자열은 먼저 이 모듈로 바이트 배열로 바꾼다.
// 반대 방향(원본 텍스트 파일 바이트 → 문자열)의 해독도 이 모듈이 맡는다: decodeWindows1252, decodeOriginalText(UTF-8 이면 UTF-8, 아니면 Windows-1252).
// (2026-10-23 WB03-M1: 메인 메뉴 버튼 라벨을 원본 글리프로 그리기 위해 추가)
// (2026-10-28 WB02-D: 원본 텍스트 파일(tell·config·setup) 해독 추가)

// 유니코드 코드 포인트 → Windows-1252 의 0x80~0x9F 구간 바이트 대응표이다.
// 0x00~0x7F 와 0xA0~0xFF 는 코드 포인트와 바이트 값이 같으므로 이 표에 없다.
// 사용: encodeWindows1252 가 이 구간의 특수 문자(유로 기호, 따옴표 등)를 바꿀 때 조회한다. (2026-10-23 WB03-M1)
const CP1252_EXTRA = new Map([
    [0x20ac, 0x80], [0x201a, 0x82], [0x0192, 0x83], [0x201e, 0x84], [0x2026, 0x85], [0x2020, 0x86],
    [0x2021, 0x87], [0x02c6, 0x88], [0x2030, 0x89], [0x0160, 0x8a], [0x2039, 0x8b], [0x0152, 0x8c],
    [0x017d, 0x8e], [0x2018, 0x91], [0x2019, 0x92], [0x201c, 0x93], [0x201d, 0x94], [0x2022, 0x95],
    [0x2013, 0x96], [0x2014, 0x97], [0x02dc, 0x98], [0x2122, 0x99], [0x0161, 0x9a], [0x203a, 0x9b],
    [0x0153, 0x9c], [0x017e, 0x9e], [0x0178, 0x9f],
]);

// Windows-1252 로 나타낼 수 없는 문자를 대신할 바이트('?')이다. 원본 글꼴에 없는 문자를 빈칸 없이 표시하기 위해 쓴다. (2026-10-23 WB03-M1)
const REPLACEMENT_BYTE = 0x3f;

// Windows-1252 의 0x80~0x9F 구간 바이트 → 유니코드 코드 포인트 대응표이다(CP1252_EXTRA 의 역표).
// 정의되지 않은 바이트(0x81, 0x8D, 0x8F, 0x90, 0x9D)는 이 표에 없으며 같은 값의 C1 제어 문자로 읽는다(WHATWG windows-1252 와 같음).
// 사용: decodeWindows1252 가 이 구간의 바이트를 문자로 바꿀 때 조회한다. (2026-10-28 WB02-D: 원본 텍스트 파일 해독용)
const CP1252_DECODE = new Map(Array.from(CP1252_EXTRA, ([code, byte]) => [byte, code]));

// 한 번에 String.fromCharCode 에 넘길 바이트 수의 상한이다. 인자가 너무 많으면 호출 스택이 넘치므로 조각으로 나눠 변환한다. (2026-10-28 WB02-D)
const DECODE_CHUNK = 0x2000;

// Windows-1252 바이트 배열을 JavaScript 문자열로 바꾼다. 바이트 하나가 문자 하나가 되며 NUL 도 그대로 둔다.
// 사용: 원본 텍스트 파일(.english·tell·config·setup 의 복호화 결과)을 읽을 때 호출한다. (2026-10-28 WB02-D)
export function decodeWindows1252(bytes) {
    const chunks = [];
    // 큰 파일(수십 KB)도 안전하게 바꾸도록 조각마다 코드 배열을 만들어 문자열로 이어 붙인다.
    for (let start = 0; start < bytes.length; start += DECODE_CHUNK) {
        const end = Math.min(bytes.length, start + DECODE_CHUNK);
        const codes = new Array(end - start);
        // 조각 안의 각 바이트를 코드 포인트로 바꾼다. 0x80~0x9F 만 표를 거친다.
        for (let i = start; i < end; i += 1) {
            const byte = bytes[i];
            codes[i - start] = byte >= 0x80 && byte <= 0x9f ? (CP1252_DECODE.get(byte) ?? byte) : byte;
        }
        chunks.push(String.fromCharCode(...codes));
    }
    return chunks.join('');
}

// 원본·클론 텍스트 파일 바이트를 문자열로 읽는다. UTF-8 BOM 이 있거나 전체가 올바른 UTF-8 이면 UTF-8 로, 그 밖에는 Windows-1252 로 읽는다.
// 원본 파일은 Windows-1252 이고 클론에서 새로 만드는 한국어 파일은 UTF-8 이라는 규칙이다(docs/formats/vfs.md 4절). 7비트 ASCII 만 있으면 어느 쪽이든 같다.
// 사용: decodeOriginalText(bytes). (2026-10-28 WB02-D)
export function decodeOriginalText(bytes) {
    const hasBom = bytes.length >= 3 && bytes[0] === 0xef && bytes[1] === 0xbb && bytes[2] === 0xbf;
    try {
        return new TextDecoder('utf-8', { fatal: true, ignoreBOM: false }).decode(hasBom ? bytes.subarray(3) : bytes);
    } catch {
        return decodeWindows1252(bytes);
    }
}

// 기본 다국어 평면(BMP) 밖의 문자를 대신할 코드(U+FFFD 대체 문자)이다. 한글·라틴 문자는 모두 BMP 안이라 UI 문구에서는 쓰이지 않는다. (2026-10-31 WB03-N)
const REPLACEMENT_UNIT = 0xfffd;

// 문자열을 UTF-16 코드 단위 배열(Uint16Array)로 바꾼다. 한국어 D2Coding 글꼴로 그릴 UI 문구의 부호화이다.
// Windows-1252 바이트 배열(Uint8Array)과 배열 종류로 구별되어, 글자 측정·그리기가 어느 글꼴을 쓸지 정한다(assets/font-set.js).
// 서로게이트 쌍(BMP 밖 문자)은 글꼴에 없는 문자로 보고 대체 문자 하나로 바꾼다.
// 사용: encodeUtf16Units('캠페인') → Uint16Array [0xCEA0, ...]. (2026-10-31 WB03-N)
export function encodeUtf16Units(text) {
    const output = [];
    // 코드 포인트 단위로 훑어 BMP 문자는 그대로, 그 밖은 대체 문자로 넣는다.
    for (const character of String(text ?? '')) {
        const code = character.codePointAt(0);
        output.push(code > 0xffff ? REPLACEMENT_UNIT : code);
    }
    return Uint16Array.from(output);
}

// 문자열을 Windows-1252 바이트 배열(Uint8Array)로 바꾼다. 나타낼 수 없는 문자는 '?' 로 바꾼다.
// 사용: 원본 글꼴로 그리거나 폭을 잴 라벨을 넘기기 전에 호출한다. 예) encodeWindows1252('Campaign')
// (2026-10-23 WB03-M1)
export function encodeWindows1252(text) {
    const source = String(text ?? '');
    const output = [];
    // 서로게이트 쌍을 하나의 코드 포인트로 읽도록 for...of 로 문자 단위 순회한다.
    for (const character of source) {
        const code = character.codePointAt(0);
        if (code < 0x80 || (code >= 0xa0 && code <= 0xff)) output.push(code);
        else output.push(CP1252_EXTRA.get(code) ?? REPLACEMENT_BYTE);
    }
    return Uint8Array.from(output);
}
