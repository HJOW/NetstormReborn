// 원본 언어 번호/옵션 선택 규칙의 웹 대응 `core/language-options.js`(cpppj LanguageOptions, 원본 004de390/004de410/004de420/004dea40)를 검사한다(4-A client 자료 읽기 계열, WB04-E272).
//  - 고정 관측 `cpppj/tests/fixtures/language-options-x86.tsv` 4655 행(LANGID 4096 · 이름 13 · 옵션 546)을 그대로 재생해 결과·두 번의 용어표 읽기 순서·그 시점의 전역 값을 대조한다.
//  - cpppj LanguageOptionsTests 의 손 시험(첫 읽기 중 current 변경 · 두 번째 읽기 실패의 부분 효과 · 읽기 장치 누락)도 같은 입력으로 옮겼다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-10 WB04-E272)
import test from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';
import { dirname, resolve } from 'node:path';
import {
    KOREAN_LANGUAGE_NUMBER,
    applyOriginalLanguageOption,
    detectOriginalLanguage,
    findOriginalLanguage,
    originalLanguageName,
} from '../www/core/language-options.js';

// 저장소 안의 고정 관측 파일 경로이다. (2026-11-10 WB04-E272)
const FIXTURE = resolve(dirname(fileURLToPath(import.meta.url)), '../../cpppj/tests/fixtures/language-options-x86.tsv');

// 소문자 16진수(없으면 '-')를 바이트마다 한 글자인 문자열로 복원한다(ASCII 정규화를 하지 않는다). (2026-11-10 WB04-E272)
function decodeHex(hex) {
    if (hex === '-') return '';
    let text = '';
    for (let index = 0; index < hex.length; index += 2) text += String.fromCharCode(Number.parseInt(hex.slice(index, index + 2), 16));
    return text;
}

// 용어표 장치가 받은 이름을 같은 형식(소문자 16진수, 빈 이름은 '-')으로 관측한다. (2026-11-10 WB04-E272)
function encodeHex(text) {
    let hex = '';
    for (let index = 0; index < text.length; index += 1) hex += text.charCodeAt(index).toString(16).padStart(2, '0');
    return hex === '' ? '-' : hex;
}

test('고정 관측 4655 행: LANGID 4096 · 이름 13 · 옵션 546 이 모두 같다', () => {
    let count = 0;
    let options = 0;
    for (const line of readFileSync(FIXTURE, 'utf8').split(/\r?\n/)) {
        if (line === '' || line[0] === '#') continue;
        const cell = line.split('\t');
        if (cell[0] === 'D') {
            assert.equal(detectOriginalLanguage(Number(cell[1])), Number(cell[2]), line);
        } else if (cell[0] === 'F') {
            assert.equal(findOriginalLanguage(decodeHex(cell[1])), Number(cell[2]), line);
        } else {
            assert.equal(cell[0], 'O');
            // 전역 두 개는 부호 있는 32비트 상자이고, 기록은 부호 없는 십진수이다.
            const current = { value: Number(cell[2]) | 0 };
            const published = { value: Number(cell[3]) | 0 };
            const raw = decodeHex(cell[4]);
            const trace = [];
            const result = applyOriginalLanguageOption(Number(cell[1]), current, published, {
                read: () => raw,
                load: (name) => trace.push(`${encodeHex(name)},${current.value >>> 0},${published.value >>> 0}`),
            });
            assert.equal(result, cell[5] !== '0', line);
            assert.equal(current.value >>> 0, Number(cell[6]), line);
            assert.equal(published.value >>> 0, Number(cell[7]), line);
            assert.equal(trace.length === 0 ? '-' : trace.join(';'), cell[8], line);
            options += 1;
        }
        count += 1;
    }
    assert.equal(count, 4655);
    assert.equal(options, 546);
});

test('원문 일곱 이름은 별칭 없이 순서대로이며 한국어 7 은 클론 추가이고 표 밖은 거부한다', () => {
    assert.deepEqual([0, 1, 2, 3, 4, 5, 6, 7].map(originalLanguageName), ['english', 'english', 'french', 'german', 'spanish', 'japanese', 'portuguese', 'korean']);
    assert.equal(KOREAN_LANGUAGE_NUMBER, 7);
    assert.equal(findOriginalLanguage('korean'), 7);
    // 영어는 이름이 둘이어도 항상 0 이며, 대소문자가 다르거나 앞뒤 공백이 있으면 모르는 이름(0)이다.
    assert.equal(findOriginalLanguage('english'), 0);
    assert.equal(findOriginalLanguage('French'), 0);
    for (const bad of [-1, 8, 1.5, Number.NaN]) assert.throws(() => originalLanguageName(bad), RangeError);
});

// LANGID 는 WORD 이므로 높은 16비트는 버린다(고정 관측은 WORD 범위뿐이라 변형 확인으로 추가).
test('detectOriginalLanguage: 높은 16비트는 버리고 낮은 WORD 만 해석한다', () => {
    // 0x10000 | 0x0800(하위 언어 2) → 영국식 영어 1, 0x10000 | 7 → 독일어 3, 높은 비트만 있으면 0.
    assert.equal(detectOriginalLanguage(0x10800), 1);
    assert.equal(detectOriginalLanguage(0x10007), 3);
    assert.equal(detectOriginalLanguage(0x20000), 0);
    assert.equal(detectOriginalLanguage(-65536 + 2048), 1);
});

test('첫 읽기에서 callback 이 current 를 바꿔도 두 번째 읽기는 바뀐 값을 보고 published 는 선택값이다', () => {
    const current = { value: -1 };
    const published = { value: -1 };
    let loads = 0;
    const changed = applyOriginalLanguageOption(0, current, published, {
        read: () => 'german',
        load: (name) => {
            assert.equal(name, 'german');
            // 두 읽기가 끝나기 전에는 published 를 쓰지 않는다.
            assert.equal(published.value, -1);
            loads += 1;
            if (loads === 1) {
                assert.equal(current.value, 3);
                current.value = 6;
            } else {
                assert.equal(current.value, 6);
            }
        },
    });
    assert.equal(changed, true);
    assert.deepEqual([current.value, published.value, loads], [6, 3, 2]);
});

test('두 번째 읽기가 실패하면 current 는 이미 쓴 채로 남고 published 는 쓰지 않으며 재시도하지 않는다', () => {
    const current = { value: -1 };
    const published = { value: 7 };
    let loads = 0;
    assert.throws(() => applyOriginalLanguageOption(0, current, published, {
        read: () => 'french',
        load: () => {
            loads += 1;
            if (loads === 2) throw new Error('load');
        },
    }), /load/);
    assert.deepEqual([current.value, published.value, loads], [2, 7, 2]);
    // current 가 이미 같은 번호이면 읽기 장치가 없어도 읽지 않고 거짓이다.
    assert.equal(applyOriginalLanguageOption(0, current, published, { read: () => 'french' }), false);
    assert.equal(published.value, 7);
});

test('장치 누락과 128 바이트 지역 범위 초과를 거부한다', () => {
    // 읽기 장치가 없으면 거부하고, 선택이 바뀌는데 용어표 장치가 없으면 current 를 쓴 뒤 거부한다.
    assert.throws(() => applyOriginalLanguageOption(0, { value: 0 }, { value: 0 }, {}), /읽기 미연결/);
    const current = { value: 0 };
    assert.throws(() => applyOriginalLanguageOption(0, current, { value: 0 }, { read: () => 'german' }), /용어표 읽기 미연결/);
    assert.equal(current.value, 3);
    // NUL 앞이 127 글자이면 통과, 128 글자이면 지역 범위 초과이다. NUL 뒤의 긴 글자는 보지 않는다.
    assert.equal(applyOriginalLanguageOption(0, { value: 0 }, { value: 0 }, { read: () => 'x'.repeat(127), load: () => {} }), false);
    assert.throws(() => applyOriginalLanguageOption(0, { value: 0 }, { value: 0 }, { read: () => 'x'.repeat(128), load: () => {} }), RangeError);
    assert.equal(applyOriginalLanguageOption(0, { value: 0 }, { value: 0 }, { read: () => `${'x'.repeat(10)}\0${'y'.repeat(500)}`, load: () => {} }), false);
});
