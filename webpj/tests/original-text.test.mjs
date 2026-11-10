// 원문 인코딩 계층(cpppj OriginalText.cpp)의 웹 대응 `assets/text-encoding.js` 검사(4-A OriginalText, WB04-E269).
//  - Windows-1252 해독은 256바이트 전부 WHATWG windows-1252(Node TextDecoder)와 같다. cpppj 표(0x80~0x9f)도 같은 표준 대응이며 정의되지 않은 5바이트는 같은 값의 C1 제어 문자로 읽는다.
//  - 엄격한 인코딩(encodeOriginalText = cpppj EncodeOriginalText): 256바이트 왕복, 나타낼 수 없는 문자·짝 없는 서로게이트 거부, BOM 없음. 글꼴 라벨용 encodeWindows1252 는 '?' 로 대체한다.
//  - decodeOriginalText 규칙(BOM·유효한 UTF-8·Windows-1252 대체)과 cpppj 와의 의도된 차이.
// 6차 한국어의 4바이트 확장 시퀀스(DecodeDisplayText/EncodeDisplayText, 한글 음절·호환 자모·CJK 기호·전각 ASCII)는 화면용 비트맵 글꼴 경계이고 웹은 UTF-16 + D2Coding 으로 그리므로 옮기지 않는다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E269)
import test from 'node:test';
import assert from 'node:assert/strict';
import { decodeOriginalText, decodeWindows1252, encodeOriginalText, encodeWindows1252 } from '../www/assets/text-encoding.js';

// 0..255 모든 바이트이다. (2026-11-10 WB04-E269)
const ALL_BYTES = Uint8Array.from({ length: 256 }, (_, byte) => byte);

test('OriginalText: Windows-1252 해독은 256바이트 전부 표준 windows-1252 와 같다', () => {
    const expected = new TextDecoder('windows-1252').decode(ALL_BYTES);
    assert.equal(decodeWindows1252(ALL_BYTES), expected);
    // 정의되지 않은 다섯 바이트는 같은 값의 C1 제어 문자로 읽는다(cpppj 표의 0x81·0x8d·0x8f·0x90·0x9d).
    for (const byte of [0x81, 0x8d, 0x8f, 0x90, 0x9d]) assert.equal(decodeWindows1252(Uint8Array.of(byte)).charCodeAt(0), byte);
    // 정의된 0x80~0x9f 는 표준 대응이다(예: 0x80 유로, 0x99 상표, 0x9f Ÿ).
    assert.deepEqual([0x80, 0x99, 0x9f].map((byte) => decodeWindows1252(Uint8Array.of(byte)).charCodeAt(0)), [0x20ac, 0x2122, 0x178]);
});

test('OriginalText: 엄격한 인코딩은 256바이트를 왕복하고 나타낼 수 없는 문자를 거부한다', () => {
    // 모든 바이트가 해독 → 인코딩으로 같은 바이트로 돌아온다(정의되지 않은 C1 제어 문자 5개도 원본 표에 같은 값으로 있어 그대로 돌아온다).
    for (let byte = 0; byte < 256; byte += 1) assert.deepEqual([...encodeOriginalText(decodeWindows1252(Uint8Array.of(byte)))], [byte], `바이트 ${byte}`);
    assert.deepEqual([...encodeOriginalText(decodeWindows1252(ALL_BYTES))], [...ALL_BYTES]);
    // 한글·표 밖 기호·짝 없는 서로게이트는 '?' 로 바꾸지 않고 거부한다.
    assert.throws(() => encodeOriginalText('한글'), RangeError);
    assert.throws(() => encodeOriginalText('a b'), RangeError);
    assert.throws(() => encodeOriginalText('\ud800'), RangeError);
    // BMP 밖 문자(서로게이트 쌍)도 표에 없다.
    assert.throws(() => encodeOriginalText('😀'), RangeError);
    // BOM 은 만들지 않고 빈 입력은 빈 바이트이다.
    assert.deepEqual([...encodeOriginalText('€é')], [0x80, 0xe9]);
    assert.equal(encodeOriginalText('').length, 0);
    assert.equal(encodeOriginalText(undefined).length, 0);
    // 글꼴 라벨용 encodeWindows1252 는 같은 입력을 '?' 로 대체한다.
    assert.deepEqual([...encodeWindows1252('한a')], [0x3f, 0x61]);
});

test('OriginalText: 파일 바이트 해독은 BOM·유효한 UTF-8·Windows-1252 순서이다', () => {
    // UTF-8 BOM 은 떼고 UTF-8 로 읽는다.
    assert.equal(decodeOriginalText(Uint8Array.from([0xef, 0xbb, 0xbf, 0xea, 0xb0, 0x80])), '가');
    // 올바른 UTF-8 은 BOM 이 없어도 UTF-8 이다(웹의 의도된 규칙: 클론의 새 한국어 파일은 UTF-8).
    assert.equal(decodeOriginalText(Uint8Array.from([0xea, 0xb0, 0x80])), '가');
    // UTF-8 이 아니면 Windows-1252 이다(원본 파일).
    assert.equal(decodeOriginalText(Uint8Array.of(0xe9)), 'é');
    assert.equal(decodeOriginalText(Uint8Array.of(0x80, 0x99)), '€™');
    // 7비트 ASCII 와 빈 입력은 그대로이다.
    assert.equal(decodeOriginalText(new TextEncoder().encode('plain')), 'plain');
    assert.equal(decodeOriginalText(new Uint8Array(0)), '');
    // 큰 입력도 조각으로 안전하게 바꾼다.
    assert.equal(decodeWindows1252(new Uint8Array(100000).fill(0x41)).length, 100000);
});
