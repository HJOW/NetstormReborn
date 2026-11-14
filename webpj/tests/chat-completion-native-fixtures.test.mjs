// 채팅 이름 자동완성(ChatNameCompletion, 원본 0042e540)의 원본 10.78 독립 관측 189행 대조: 번역 중복/빈값/127·128바이트 넘침·100행 기록·현재 기록 변경·접두 잔류·정/역방향 순환을
// cpppj `ChatNameCompletionTests` 와 같은 입력 구성으로 비교한다. 행 사이에 원문 BSS(0x4090바이트 상태)를 유지하며 `reset` 열이 1 인 행에서만 새 상태로 시작한다.
// 입력 열(상태 초기화·기록 4800바이트·번역 모드·질의·길이·방향)만 장치에 공급하고 관측 열(결과·상태 전체·호출 순서)은 비교에만 쓴다. 원본 게임을 실행하지 않는다. (2026-11-15 WB04-E484)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { ChatNameCompletionState, completeChatName } from '../www/ui/chat-name-completion.js';

// 16진 문자열을 바이트 문자열(한 글자 한 바이트)로 복원한다(`-` 는 빈 값).
const decode = (text) => (text === '-' ? '' : Buffer.from(text, 'hex').toString('latin1'));
// 바이트 열(문자열 또는 배열)을 소문자 16진으로 만든다. trim 이면 끝의 0 바이트를 줄이고, 비면 `-` 이다.
function encode(bytes, trim = false) {
    let end = bytes.length;
    if (trim) while (end > 0 && bytes[end - 1] === 0) end -= 1;
    const text = typeof bytes === 'string' ? Buffer.from(bytes.slice(0, end), 'latin1').toString('hex') : Buffer.from(bytes.subarray(0, end)).toString('hex');
    return text === '' ? '-' : text;
}

test('ChatNameCompletion 원본 10.78 189행: 번역/빈값/넘침/기록/접두 잔류/정·역방향 순환의 결과·상태 전체·호출 순서가 일치한다', () => {
    const table = fs.readFileSync(new URL('../../cpppj/tests/fixtures/chat-completion-x86.tsv', import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && !line.startsWith('name\t')).map((line) => line.split('\t'));
    assert.equal(table.length, 189);
    // 행 사이에 원문 BSS(자동완성 상태)를 유지한다.
    let state = new ChatNameCompletionState(); const failures = [];
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 11, `행 ${index}`);
        if (row[1] === '1') state = new ChatNameCompletionState();
        // 현재 Chat 100행×48바이트 기록(입력 앞부분만 채우고 나머지는 0)이다.
        const records = new Uint8Array(4800); const input = Buffer.from(decode(row[2]), 'latin1'); records.set(input.subarray(0, 4800));
        const mode = Number(row[3]); const trace = [];
        const hooks = {
            records: () => records, assertion: (line) => trace.push(`A:${line}`),
            // 번역 모드: 1 프랑스어형, 2 모두 `dup`, 3 ALLIES 만 빈 값, 4/5 BATTLE 을 127/128 개의 Z 로, 그 밖은 키 그대로이다.
            translate: (key) => {
                trace.push(`T:${encode(key)}`);
                if (mode === 1) { if (key === 'BATTLE') return 'Area'; if (key === 'EVERYONE') return 'Tous'; if (key === 'ALLIES') return 'Friends'; return 'Eyes'; }
                if (mode === 2) return 'dup';
                if (mode === 3 && key === 'ALLIES') return '';
                if ((mode === 4 || mode === 5) && key === 'BATTLE') return 'Z'.repeat(mode === 4 ? 127 : 128);
                return key;
            },
        };
        const query = row[4] === '~' ? null : decode(row[4]);
        const result = completeChatName(state, query, Number(row[5]) | 0, row[6] === '1', hooks);
        const problems = [];
        if ((result === undefined || result === null ? '~' : encode(result)) !== row[7]) problems.push('result');
        if (encode(state.bytes, true) !== row[8]) problems.push('state');
        if ((trace.join(';') || '-') !== row[9]) problems.push('trace');
        if (!(Number(row[10]) > 0)) problems.push('instructions');
        if (problems.length) failures.push(`${index}/${row[0]}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
});

// 관측 fixture 가 담지 않는 입력(번역/질의 안의 NUL·대소문자 경계 'Z'·'['·길이 126/127 진단 경계·strncpy 물리 한계·접두 꼬리 영 채움)을 기준선 소스(ChatNameCompletion.cpp)의 계약으로 독립 확인한다.
// 기대값은 모듈이 아니라 C++ 소스의 `substr(0, find('\0'))`·`length > 126`·`width > size - 8`·`index < text.size() ? 글자 : 0` 에서 읽었다. (2026-11-15 WB04-E484)
test('ChatNameCompletion 독립 확인: 번역/질의의 첫 NUL 절단·대소문자 경계·길이 진단 경계·strncpy 물리 한계·접두 영 채움', () => {
    const records = new Uint8Array(4800);
    // 번역 함수와 진단 기록을 받는 호출 도우미이다. 첫 후보는 항상 BATTLE 의 번역이다.
    const call = (state, query, length, translate, trace = []) => completeChatName(state, query, length, false, { records: () => records, assertion: (line) => trace.push(line), translate });
    const CANDIDATE0 = 0x90; const PREFIX = 8;
    // (가) 번역 안의 첫 NUL 앞만 후보로 저장한다: 'ab\0cd' → 'ab', '\0xyz' → 빈 후보(NUL 뒤 글자를 쓰지 않는다).
    for (const [translated, expected] of [['ab\0cd', 'ab'], ['\0xyz', '']]) {
        const state = new ChatNameCompletionState(); assert.equal(call(state, '', 0, (key) => (key === 'BATTLE' ? translated : key)), expected);
        assert.deepEqual(Array.from(state.bytes.subarray(CANDIDATE0, CANDIDATE0 + 6)), [...Buffer.from(expected, 'latin1'), ...new Array(6 - expected.length).fill(0)], translated);
    }
    // (나) 대소문자 비교는 ASCII 문자만 접는다: 'z' 는 'Z' 와 같지만 '[' 와 '{' 는 다르다.
    assert.equal(call(new ChatNameCompletionState(), 'z', 1, (key) => (key === 'BATTLE' ? 'ZEBRA' : key)), 'ZEBRA');
    assert.equal(call(new ChatNameCompletionState(), '[', 1, (key) => (key === 'BATTLE' ? '{abc' : key)), undefined);
    // (다) 길이 126 은 진단 없이 통과하고 127 부터 진단 0x354 가 나간다.
    const quiet = []; call(new ChatNameCompletionState(), '', 126, (key) => key, quiet); assert.deepEqual(quiet, []);
    const loud = []; call(new ChatNameCompletionState(), '', 127, (key) => key, loud); assert.deepEqual(loud, [0x354]);
    // (라) 질의의 첫 NUL 앞만 접두가 되고 길이 폭까지 영으로 채우며 폭 뒤의 이전 접두 꼬리는 남긴다.
    const state = new ChatNameCompletionState(); call(state, 'abcde', 5, (key) => key);
    assert.deepEqual(Array.from(state.bytes.subarray(PREFIX, PREFIX + 5)), [0x61, 0x62, 0x63, 0x64, 0x65]);
    call(state, 'ab', 3, (key) => key); assert.deepEqual(Array.from(state.bytes.subarray(PREFIX, PREFIX + 5)), [0x61, 0x62, 0, 0x64, 0x65]);
    call(state, 'ab\0cd', 5, (key) => key); assert.deepEqual(Array.from(state.bytes.subarray(PREFIX, PREFIX + 5)), [0x61, 0x62, 0, 0, 0]);
    call(state, '\0ab', 3, (key) => key); assert.deepEqual(Array.from(state.bytes.subarray(PREFIX, PREFIX + 3)), [0, 0, 0]);
    // (마) 접두 길이가 상태 끝(0x4090 − 8 = 0x4088)까지는 허용되고 그 다음부터는 strncpy 물리 범위 오류이다.
    assert.throws(() => call(new ChatNameCompletionState(), '', 0x4089, (key) => key), { name: 'RangeError', message: /strncpy/ });
    try { call(new ChatNameCompletionState(), '', 0x4088, (key) => key); } catch (error) { assert.doesNotMatch(String(error.message), /strncpy/); }
});
