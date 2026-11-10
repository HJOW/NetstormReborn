// client 자료 읽기 계열(cpppj PaletteFileRead·GifFileRead·FontCacheFile)의 웹 대응 `GamePalette.parse`·`decodeGif`·`BitmapFont.parse` 가 지키는 값 수준 계약을 고정한다(4-A client 자료 읽기 계열, WB04-E272).
//  - 고정 관측 `palette-file-read-x86.tsv`(92행)·`gif-file-read-x86.tsv`(56행)·`font-cache-file-x86.tsv`(reader 시나리오)를 재생하되, 원문 FILE 열기/위치/읽기/닫기 효과열·키 수명·부분 읽기(짧은 파일)·
//    후크 예외 시나리오는 브라우저에 FILE 이 없어 대조하지 않는다. 같은 입력 바이트에서 나온 결과 값(팔레트 RGB·GIF 치수와 색 번호·글꼴 서명/머리 판정)만 대조한다.
//  - 입력 바이트는 cpppj 시험이 만드는 방식(`(index*29+7)&255` 등)을 그대로 따라 만든다. 암호화(키 "abc") 행은 읽으면 같은 평문이 되므로 같은 입력이다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E272)
import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, resolve } from 'node:path';
import { GamePalette } from '../www/assets/palette.js';
import { decodeGif } from '../www/assets/gif.js';
import { BitmapFont } from '../www/assets/bitmap-font.js';

// 저장소 안의 고정 관측 폴더이다. (2026-11-10 WB04-E272)
const FIXTURES = resolve(dirname(fileURLToPath(import.meta.url)), '../../cpppj/tests/fixtures');

// 고정 관측 파일을 읽어 주석(#)과 빈 줄을 뺀 행의 칸 배열로 돌려준다. (2026-11-10 WB04-E272)
function readRows(name) {
    return readFileSync(`${FIXTURES}/${name}`, 'utf8').split(/\r?\n/).filter((line) => line !== '' && line[0] !== '#').map((line) => line.split('\t'));
}

// 소문자 16진수 문자열을 바이트 배열로 바꾼다. (2026-11-10 WB04-E272)
function hexBytes(hex) {
    return Uint8Array.from(hex.match(/../g) ?? [], (pair) => Number.parseInt(pair, 16));
}

test('팔레트 파일: 길이 0x308(.COL)·0x400(RGBQUAD)은 같은 RGB 로 읽고 그 밖은 거부한다(고정 관측 재생)', () => {
    let valueRows = 0;
    let rejectRows = 0;
    for (const row of readRows('palette-file-read-x86.tsv')) {
        // 칸: 길이,키,하위파일,부분읽기,?,?,?,시나리오,읽기실패번호,진단복귀
        const [length, , , partial, , , , scenario, , returnAssert] = row[0].split(',').map(Number);
        if (partial !== 0 || scenario !== 0) continue;
        const payload = Uint8Array.from({ length }, (_, index) => (index * 29 + 7) & 255);
        if (length === 0x308 || length === 0x400) {
            assert.equal(row[1], 'ok', row[0]);
            const saved = hexBytes(row[3]);
            const rgb = GamePalette.parse(payload).rgb;
            // 원문 저장 칸은 BGRX(4바이트)이고 웹 RGB 는 번호마다 R,G,B 세 바이트이다.
            for (let index = 0; index < 256; index += 1) {
                assert.deepEqual([rgb[index * 3], rgb[index * 3 + 1], rgb[index * 3 + 2]], [saved[index * 4 + 2], saved[index * 4 + 1], saved[index * 4]], `${row[0]} #${index}`);
            }
            valueRows += 1;
        } else if (returnAssert === 0) {
            // 원문은 진단(0x940)을 낸다. 웹은 진단 복귀 뒤 계속하지 않고 오류로 거부한다.
            assert.equal(row[1], 'assertion', row[0]);
            assert.throws(() => GamePalette.parse(payload), /팔레트 파일 크기/);
            rejectRows += 1;
        }
    }
    assert.equal(valueRows, 12);
    assert.equal(rejectRows, 8);
});

test('GIF 파일: 치수와 색 번호를 같은 입력에서 읽고 크기 0·LZW 코드 크기 1 은 거부한다(고정 관측 재생)', () => {
    let decoded = 0;
    let rejected = 0;
    for (const row of readRows('gif-file-read-x86.tsv')) {
        // 칸: 폭,높이,색,키,하위파일,부분읽기,시나리오,실제행
        const [width, height, color, , , partial, scenario] = row[0].split(',').map(Number);
        // 입력 바이트 칸이 '-' 이면 해독 전에 실패한 행이고, 부분 읽기 행은 파일이 잘려 FILE 경계 시험이다.
        if (row[5] === '-' || partial !== 0) continue;
        const bytes = hexBytes(row[5]);
        // 서술자 WORD(406ef7 이전 407171)의 폭·높이가 관측한 치수 칸과 같다.
        assert.equal(`${bytes[6] | (bytes[7] << 8)},${bytes[8] | (bytes[9] << 8)}`, row[2], row[0]);
        if (scenario === 6 || width === 0 || height === 0) {
            // 코드 크기 1(시나리오 6) 또는 크기 0 은 원문 해독기가 거부한다. 웹은 치수를 먼저 내놓지 않고 전체 해독을 거부한다.
            assert.throws(() => decodeGif(bytes), /GIF/);
            rejected += 1;
        } else {
            const image = decodeGif(bytes);
            assert.deepEqual([image.width, image.height], [width, height], row[0]);
            assert.deepEqual([...image.indices], new Array(width * height).fill(color), row[0]);
            decoded += 1;
        }
    }
    assert.equal(decoded, 24);
    assert.equal(rejected, 8);
});

// 글꼴 캐시 파일을 cpppj 시험과 같은 머리·표로 만든다(글리프 블록은 모두 비어 있다). 서명 16바이트 + 머리 정수 4개 + 256항목 표 네 개이다. (2026-11-10 WB04-E272)
function fontFile({ signature = 'BitmapFontData\x1a\0', version = 1, charset = 256 } = {}) {
    const bytes = new Uint8Array(4132);
    const view = new DataView(bytes.buffer);
    for (let index = 0; index < 16; index += 1) bytes[index] = signature.charCodeAt(index);
    view.setUint32(16, version, true);
    view.setInt32(20, 14, true);
    view.setInt32(24, 11, true);
    view.setInt32(28, 3, true);
    view.setUint32(32, charset, true);
    for (let code = 0; code < 256; code += 1) {
        view.setInt32(36 + code * 4, 5, true);
        view.setInt32(36 + 1024 + code * 4, -2, true);
        view.setInt32(36 + 2048 + code * 4, 6, true);
        // 글리프 블록 크기 표(넷째)는 0 으로 둔다.
    }
    return bytes;
}

test('글꼴 캐시 파일: 서명·버전·글자 수 판정이 원문 reader 시나리오와 같다', () => {
    // 고정 관측의 reader(쓰기 아님) 행에서 시나리오별 반환값(1 읽음/0 안 읽음)을 모은다. 시나리오: 7 글자 수 255, 8 버전 2, 9 첫 글자 X, 10 마지막 서명 바이트 255.
    const returned = new Map();
    for (const row of readRows('font-cache-file-x86.tsv')) {
        const [writing, , , scenario] = row[0].split(',').map(Number);
        if (writing !== 0 || scenario < 7 || scenario > 10 || row[1] !== 'ok') continue;
        returned.set(scenario, new Set([...(returned.get(scenario) ?? []), row[2]]));
    }
    // 8·9 는 읽지 않고(0), 10 은 읽으며(1), 7 은 진단 복귀 시에만 읽는다(1).
    assert.deepEqual([...returned.get(8)], ['0']);
    assert.deepEqual([...returned.get(9)], ['0']);
    assert.deepEqual([...returned.get(10)], ['1']);
    assert.deepEqual([...returned.get(7)], ['1']);
    // 웹: 정상 머리는 읽고 글자 수·버전·서명이 다르면 거부한다. 마지막 서명 바이트는 원문처럼 무시한다(진단 복귀 뒤 계속은 웹에 없다).
    const font = BitmapFont.parse(fontFile());
    assert.deepEqual([font.height, font.ascent, font.descent], [14, 11, 3]);
    assert.deepEqual([font.glyph(65).advance, font.glyph(65).bearing, font.glyph(65).drawAdvance, font.glyph(65).present], [5, -2, 6, false]);
    assert.equal(font.measure(Uint8Array.of(65, 66)), 10);
    assert.throws(() => BitmapFont.parse(fontFile({ charset: 255 })), /머리/);
    assert.throws(() => BitmapFont.parse(fontFile({ version: 2 })), /머리/);
    assert.throws(() => BitmapFont.parse(fontFile({ signature: 'XitmapFontData\x1a\0' })), /머리/);
    assert.equal(BitmapFont.parse(fontFile({ signature: 'BitmapFontData\x1a\xff' })).height, 14);
    // 14번째 바이트(0x1a)가 다르면 서명이 다르다.
    assert.throws(() => BitmapFont.parse(fontFile({ signature: 'BitmapFontData\x1b\0' })), /머리/);
});
