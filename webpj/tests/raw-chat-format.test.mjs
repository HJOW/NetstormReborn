// Chat 서식 명령(RawChatFormat)의 고정 스타일 검색·캡처 쓰기·슬롯 제거·manager/Config 쓰기를 기준선 소스 계약과 손 계산 바이트로 검증한다. (2026-11-14 WB04-E442)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 순서/경계는 기준선 소스(RawChatFormat.cpp)에서 옮겼다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { applyChatFormat, stripChatFormatSlots, CHAT_FORMAT_CAPTURE_BYTES } from '../www/ui/raw-chat-format.js';

const bytes = (text, size = text.length) => { const result = new Uint8Array(size); for (let index = 0; index < text.length; index += 1) result[index] = text.charCodeAt(index) & 0xff; return result; };
const cstr = (data) => { let result = ''; for (const byte of data) { if (!byte) break; result += String.fromCharCode(byte); } return result; };
// 장치 기록 틀이다. memory 는 manager 토큰마다 600바이트 물리를 준다.
function rig({ current = 7, subscriber = 12, captureSize = 100, fill = 0x55, mutateStrip = null } = {}) {
    const log = []; const memories = new Map(); const memory = (token) => { if (!memories.has(token)) memories.set(token, new Uint8Array(600).fill(0x41)); return memories.get(token); };
    const capture = new Uint8Array(captureSize).fill(fill); const globals = { current, subscriber };
    const hooks = { memory, assertion: (line) => log.push(['assertion', line]), copyFormat: (manager, offset, value, limit) => log.push(['copy', manager, offset, value, limit]), writeConfig: (key, value) => log.push(['config', key, value]), illegalFormat: (value) => log.push(['illegal', value]) };
    if (mutateStrip) hooks.stripSlots = (data) => { log.push(['strip', cstr(data)]); mutateStrip(data); };
    return { log, capture, globals, hooks, memory, memories };
}

test('stripChatFormatSlots: ~0~6 두 바이트를 제자리 제거하고 제거한 위치를 다시 검사하며 새 NUL 뒤의 원래 꼬리를 보존한다', () => {
    const a = bytes('a~0b~6c~7d~', 16); stripChatFormatSlots(a); assert.equal(cstr(a), 'abc~7d~');
    const chained = bytes('~0~1~2x', 12); stripChatFormatSlots(chained); assert.equal(cstr(chained), 'x');
    // 꼬리 보존: 'ab~1cd\0XYZ\0' → 앞 5바이트가 'abcd\0', 그 뒤는 옛 바이트 그대로('d' 가 인덱스 5 에 남는다).
    const tail = Uint8Array.from([0x61, 0x62, 0x7e, 0x31, 0x63, 0x64, 0, 0x58, 0x59, 0x5a, 0]); stripChatFormatSlots(tail); assert.deepEqual(Array.from(tail), [0x61, 0x62, 0x63, 0x64, 0, 0x64, 0, 0x58, 0x59, 0x5a, 0]);
    // 범위 밖 숫자('~7','~/')와 끝의 ~ 는 그대로이고 NUL 로 시작하면 아무것도 하지 않는다.
    const keep = bytes('~7~/~', 8); stripChatFormatSlots(keep); assert.equal(cstr(keep), '~7~/~'); const empty = bytes('', 4); stripChatFormatSlots(empty);
    assert.throws(() => stripChatFormatSlots(Uint8Array.from([0x61, 0x62])), { name: 'RangeError', message: /물리 범위/ }); assert.throws(() => stripChatFormatSlots(Uint8Array.from([0x7e, 0x31, 0x61])), { name: 'RangeError', message: /NUL 없음/ });
    assert.equal(CHAT_FORMAT_CAPTURE_BYTES, 81);
});

test('applyChatFormat 검색: 고정 패턴 (~[^~ CR TAB LF space]+)+ 의 첫 부분 검색/연속 그룹을 캡처하고 일치가 없으면 illegalFormat 뒤 0 이다', () => {
    const cases = [['x ~12 ab', '~12'], ['~a~b c', '~a~b'], ['~~a', '~a'], ['k~1~2x ', '~1~2x'], ['~\x0bx', '~\x0bx'], ['~\xe9é~z', '~\xe9é~z'], ['~a ~b', '~a'], ['~a\t~b', '~a'], ['~1~~2', '~1']];
    for (const [input, expected] of cases) { const r = rig({ mutateStrip: () => {} }); applyChatFormat(false, r.globals, input, r.capture, r.hooks); assert.equal(r.log[1][3], expected.slice(0, 80), JSON.stringify(input)); }
    // 일치 없음: 입력(첫 NUL 앞)을 진단으로 넘기고 캡처/manager/Config 는 건드리지 않는다.
    for (const input of ['no tilde', '~', '~ a', '~\r', '']) { const r = rig(); assert.equal(applyChatFormat(false, r.globals, `${input}\0hidden`, r.capture, r.hooks), 0); assert.deepEqual(r.log, [['illegal', input]]); assert.equal(r.capture[0], 0x55); }
});

test('applyChatFormat 순서: 캡처 쓰기(최대 80바이트+NUL) → 슬롯 제거 → manager 복사(back 0x14/60, front 0x50/500) → 강제 NUL(back 0x4f, front 0x243) → ID 키 Config 쓰기이고 항상 0', () => {
    const back = rig(); assert.equal(applyChatFormat(false, back.globals, '~1~2x', back.capture, back.hooks), 0);
    assert.deepEqual(back.log, [['copy', 7, 0x14, 'x', 60], ['config', '12chatFormat', 'x']]); assert.equal(back.memory(7)[0x4f], 0); assert.equal(back.memory(7)[0x4e], 0x41); assert.equal(back.memory(7)[0x50], 0x41);
    // 슬롯 제거는 앞으로 옮기기만 하므로 캡처에는 'x,NUL' 뒤로 옛 바이트가 남는다.
    assert.equal(cstr(back.capture), 'x'); assert.deepEqual(Array.from(back.capture.subarray(0, 8)), [0x78, 0, 0x78, 0, 0x78, 0, 0x55, 0x55]);
    const front = rig({ subscriber: -5 }); applyChatFormat(true, front.globals, 'a~Hi', front.capture, front.hooks); assert.deepEqual(front.log, [['copy', 7, 0x50, '~Hi', 500], ['config', '-5chatFormat2', '~Hi']]); assert.equal(front.memory(7)[0x243], 0); assert.equal(front.memory(7)[0x242], 0x41);
});

test('applyChatFormat: 그룹이 80바이트를 넘으면 앞 80바이트만 쓰고 [80]=NUL 이며 캡처의 그 뒤는 보존한다', () => {
    const long = `~${'z'.repeat(100)}`; const r = rig(); applyChatFormat(false, r.globals, long, r.capture, r.hooks);
    assert.equal(r.log[0][3], `~${'z'.repeat(79)}`); assert.equal(r.capture[79], 0x7a); assert.equal(r.capture[80], 0); assert.equal(r.capture[81], 0x55);
    // 슬롯 제거는 실제 NUL 까지 앞으로 옮기므로 뒤의 옛 바이트를 지우지 않는다.
    const slots = rig(); applyChatFormat(false, slots.globals, '~0~1ab', slots.capture, slots.hooks); assert.deepEqual(Array.from(slots.capture.subarray(0, 8)), [0x61, 0x62, 0, 0x62, 0, 0x62, 0, 0x55]);
});

test('applyChatFormat: 현재 manager 가 0 이면 assertion(back 157/front 184)을 부르고 현재 전역으로 계속하며, manager 는 복사 직전 값을 쓴다', () => {
    const zeroBack = rig({ current: 0 }); applyChatFormat(false, zeroBack.globals, '~a', zeroBack.capture, zeroBack.hooks); assert.deepEqual(zeroBack.log[0], ['assertion', 157]); assert.deepEqual(zeroBack.log[1], ['copy', 0, 0x14, '~a', 60]);
    const zeroFront = rig({ current: 0 }); applyChatFormat(true, zeroFront.globals, '~a', zeroFront.capture, zeroFront.hooks); assert.deepEqual(zeroFront.log[0], ['assertion', 184]);
    // 슬롯 제거 중 현재 manager 가 바뀌어도 복사 대상은 제거 뒤(복사 직전)에 읽은 값이다.
    const moving = rig({ mutateStrip: () => {} }); moving.hooks.stripSlots = () => { moving.globals.current = 9; }; applyChatFormat(false, moving.globals, '~a', moving.capture, moving.hooks); assert.equal(moving.log[0][1], 9); assert.equal(moving.memories.has(9), true);
});

test('applyChatFormat 오류: 캡처가 81바이트 미만이면 RangeError(쓰기 전), manager 물리가 강제 NUL 위치보다 작으면 RangeError(복사 뒤), 장치 누락은 도달한 위치에서만 Error 다', () => {
    const small = rig({ captureSize: 80 }); assert.throws(() => applyChatFormat(false, small.globals, '~a', small.capture, small.hooks), { name: 'RangeError', message: /81BYTE/ }); assert.equal(small.capture[0], 0x55); assert.deepEqual(small.log, []);
    const shortMemory = rig(); shortMemory.hooks.memory = () => new Uint8Array(0x4f); assert.throws(() => applyChatFormat(false, shortMemory.globals, '~a', shortMemory.capture, shortMemory.hooks), { name: 'RangeError', message: /manager 범위/ }); assert.equal(shortMemory.log.length, 1);
    const exact = rig(); exact.hooks.memory = () => new Uint8Array(0x50); assert.equal(applyChatFormat(false, exact.globals, '~a', exact.capture, exact.hooks), 0); const frontShort = rig(); frontShort.hooks.memory = () => new Uint8Array(0x243); assert.throws(() => applyChatFormat(true, frontShort.globals, '~a', frontShort.capture, frontShort.hooks), RangeError);
    for (const name of ['copyFormat', 'memory', 'writeConfig']) { const r = rig(); assert.throws(() => applyChatFormat(false, r.globals, '~a', r.capture, { ...r.hooks, [name]: undefined }), { name: 'Error', message: /실제 장치 미연결/ }, name); }
    const noIllegal = rig(); assert.throws(() => applyChatFormat(false, noIllegal.globals, 'zz', noIllegal.capture, { ...noIllegal.hooks, illegalFormat: undefined }), /실제 장치 미연결/);
    const noAssertion = rig({ current: 0 }); assert.throws(() => applyChatFormat(false, noAssertion.globals, '~a', noAssertion.capture, { ...noAssertion.hooks, assertion: undefined }), /실제 장치 미연결/);
    // 명시적으로 없앤 슬롯 제거 장치는 거부하고, 지정하지 않으면 기본 구현을 쓴다.
    const noStrip = rig(); assert.throws(() => applyChatFormat(false, noStrip.globals, '~a', noStrip.capture, { ...noStrip.hooks, stripSlots: null }), /실제 장치 미연결/); const defaulted = rig(); applyChatFormat(false, defaulted.globals, '~0q', defaulted.capture, defaulted.hooks); assert.equal(defaulted.log[0][3], 'q');
});

test('경계: 슬롯 제거로 비는 캡처도 오류가 아니고(빈 문자열 복사), 입력 선두 NUL 은 빈 입력이며, NUL 뒤는 검색하지 않고 정확히 81바이트 캡처도 허용한다', () => {
    const empty = rig({ mutateStrip: undefined }); applyChatFormat(false, empty.globals, '~0', empty.capture, empty.hooks); assert.deepEqual(empty.log, [['copy', 7, 0x14, '', 60], ['config', '12chatFormat', '']]);
    const leading = rig(); applyChatFormat(false, leading.globals, '\0~a', leading.capture, leading.hooks); assert.deepEqual(leading.log, [['illegal', '']]);
    const cut = rig(); applyChatFormat(false, cut.globals, '~a\0~b', cut.capture, cut.hooks); assert.equal(cut.log[0][3], '~a');
    const exact = rig({ captureSize: 81 }); assert.equal(applyChatFormat(false, exact.globals, '~a', exact.capture, exact.hooks), 0); assert.equal(exact.capture[2], 0);
    // 길이 2 배열의 '~1' 은 슬롯이지만 복사할 NUL 이 없어 NUL 범위 오류다(물리 범위 오류가 아니다).
    assert.throws(() => stripChatFormatSlots(Uint8Array.from([0x7e, 0x31])), { name: 'RangeError', message: /NUL 없음/ });
});
