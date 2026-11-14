// 점 명령 등록(RawDotCommandRegister)·전역 표(ClientDotCommandGlobals)·분배(RawDotCommandDispatch)를 기준선 소스 계약과 손 계산 바이트로 검증한다. (2026-11-14 WB04-E443)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 순서/경계는 기준선 소스(RawDotCommandRegister.cpp·ClientDotCommandGlobals.cpp·RawDotCommandDispatch.cpp)에서 옮겼다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { DOT_COMMAND_EMPTY_TOKEN, DOT_COMMAND_NAMES_TOKEN, DOT_COMMAND_STATE_BYTES, copyDotCommandName, registerDotCommand } from '../www/ui/dot-command-register.js';
import { ClientDotCommandGlobals } from '../www/ui/dot-command-globals.js';
import { DotCommandDispatchState, dispatchDotCommand } from '../www/ui/dot-command-dispatch.js';

const bytes = (text) => Uint8Array.from([...Buffer.from(text, 'latin1'), 0]);
const dword = (state, offset) => (state[offset] | (state[offset + 1] << 8) | (state[offset + 2] << 16) | (state[offset + 3] << 24)) >>> 0;
const setDword = (state, offset, value) => { for (let index = 0; index < 4; index += 1) state[offset + index] = (value >>> (index * 8)) & 0xff; };
const nameOf = (state, slot) => { let text = ''; for (let at = slot * 32; state[at]; at += 1) text += String.fromCharCode(state[at]); return text; };
const argText = (argument) => { let text = ''; for (const value of argument) { if (!value) break; text += String.fromCharCode(value); } return text; };

test('copyDotCommandName: count 바이트를 순방향으로 복사하다 NUL 을 만나면 0 으로 채우고, 목적/원본 물리 범위를 넘으면 RangeError 다', () => {
    const destination = new Uint8Array(8).fill(0x55); copyDotCommandName(destination, bytes('ab'), 6); assert.deepEqual(Array.from(destination), [0x61, 0x62, 0, 0, 0, 0, 0x55, 0x55]);
    const exact = new Uint8Array(3).fill(0x55); copyDotCommandName(exact, 'abcdef', 3); assert.deepEqual(Array.from(exact), [0x61, 0x62, 0x63]);
    assert.throws(() => copyDotCommandName(new Uint8Array(2), bytes('abc'), 3), { name: 'RangeError', message: /목적 물리/ }); assert.throws(() => copyDotCommandName(new Uint8Array(8), Uint8Array.from([0x61, 0x62]), 4), { name: 'RangeError', message: /원본 물리/ });
    // NUL 이 있으면 원본이 count 보다 짧아도 된다. count 0 은 아무것도 하지 않는다.
    copyDotCommandName(new Uint8Array(8), bytes('a'), 6); copyDotCommandName(new Uint8Array(0), Uint8Array.of(), 0);
});

test('globals: PE 정적 이미지(say·0x0042f1e0·이름표 주소·count 1)만 비영이고 같은 표/검색 상태를 복제 없이 빌려준다', () => {
    const globals = new ClientDotCommandGlobals(); const state = globals.bytes(); assert.equal(state.length, 0xa08); assert.equal(DOT_COMMAND_STATE_BYTES, 0xa08); assert.equal(globals.bytes(), state);
    assert.deepEqual(Array.from(state.subarray(0, 4)), [0x73, 0x61, 0x79, 0]); assert.deepEqual([dword(state, 0x800), dword(state, 0x900), dword(state, 0xa04)], [0x42f1e0, DOT_COMMAND_NAMES_TOKEN, 1]);
    assert.equal(globals.dispatchState(), globals.dispatchState()); assert.deepEqual([globals.dispatchState().fallbackIndex, globals.dispatchState().separatorOffset], [0, -1]);
    // 등록은 같은 표를 쓴다: 슬롯 1 에 'help', count 2.
    assert.equal(globals.register(bytes('help'), 0x1111, 1), 1); assert.equal(nameOf(state, 1), 'help'); assert.equal(dword(state, 0xa04), 2); assert.equal(new ClientDotCommandGlobals().bytes() !== state, true);
});

test('register: 슬롯 32바이트에 이름 31바이트+NUL, 함수/표시 표, count+1, 다음 표시 칸 0 종단을 쓰고 flag 가 0 이면 빈 문자열 주소를 쓴다', () => {
    const state = new ClientDotCommandGlobals().bytes(); state.fill(0x55, 0x20, 0x40); assert.equal(registerDotCommand(state, bytes('help'), 0x1234, 1), 1);
    assert.deepEqual(Array.from(state.subarray(0x20, 0x40)), [0x68, 0x65, 0x6c, 0x70, ...new Array(28).fill(0)]); assert.equal(dword(state, 0x804), 0x1234); assert.equal(dword(state, 0x904), DOT_COMMAND_NAMES_TOKEN + 32); assert.equal(dword(state, 0xa04), 2); assert.equal(dword(state, 0x908), 0);
    assert.equal(registerDotCommand(state, bytes('quiet'), 0x2222, 0), 1); assert.equal(dword(state, 0x808), 0x2222); assert.equal(dword(state, 0x908), DOT_COMMAND_EMPTY_TOKEN); assert.equal(dword(state, 0x90c), 0); assert.equal(dword(state, 0xa04), 3); assert.equal(nameOf(state, 2), 'quiet');
    // 긴 이름은 31바이트만 복사하고 32번째 바이트는 NUL 이다. 이름 바이트 문자열도 받는다.
    registerDotCommand(state, 'abcdefghijklmnopqrstuvwxyz0123456789', 0x3333, 1); assert.equal(nameOf(state, 3), 'abcdefghijklmnopqrstuvwxyz01234'); assert.equal(state[3 * 32 + 31], 0);
});

test('register: count 64 이상은 아무것도 쓰지 않고 0, 음수 count 는 RangeError, 63 번째 슬롯까지 등록하면 count 64 와 다음 표시 칸(0xa00)을 0 으로 쓴다', () => {
    const full = new ClientDotCommandGlobals().bytes(); setDword(full, 0xa04, 64); const before = Array.from(full); assert.equal(registerDotCommand(full, bytes('x'), 1, 1), 0); assert.deepEqual(Array.from(full), before);
    setDword(full, 0xa04, 0x7fffffff); assert.equal(registerDotCommand(full, bytes('x'), 1, 1), 0); setDword(full, 0xa04, 0x80000000); assert.throws(() => registerDotCommand(full, bytes('x'), 1, 1), { name: 'RangeError', message: /음수 초기count/ });
    const last = new ClientDotCommandGlobals().bytes(); setDword(last, 0xa04, 63); setDword(last, 0xa00, 0x77777777); assert.equal(registerDotCommand(last, bytes('end'), 9, 1), 1); assert.equal(dword(last, 0xa04), 64); assert.equal(dword(last, 0xa00), 0); assert.equal(dword(last, 0x900 + 63 * 4), DOT_COMMAND_NAMES_TOKEN + 63 * 32); assert.equal(registerDotCommand(last, bytes('more'), 9, 1), 0);
});

test('register: 복사 장치가 count 를 바꿔도 복사 뒤 현재 count 를 단 한 번 캡처해 쓰고(원문 ECX), 증가한 값은 캡처 값+1 이며 복사 장치는 31바이트 물리를 받는다', () => {
    const state = new ClientDotCommandGlobals().bytes(); let received = null;
    const hooks = { copy: (destination, name, count) => { received = [destination.length, Array.from(name), count]; setDword(state, 0xa04, 5); } };
    assert.equal(registerDotCommand(state, bytes('hi'), 0xabc, 1, hooks), 1); assert.deepEqual(received, [31, [0x68, 0x69, 0], 31]);
    assert.equal(dword(state, 0x800 + 5 * 4), 0xabc); assert.equal(dword(state, 0xa04), 6); assert.equal(state[5 * 32 + 31], 0); assert.equal(dword(state, 0x900 + 5 * 4), DOT_COMMAND_NAMES_TOKEN + 5 * 32); assert.equal(dword(state, 0x900 + 6 * 4), 0);
    // 복사 뒤 count 가 표 밖이면 RangeError(원문 인접 물리 밖 접근)이며 앞선 복사는 남는다. 장치를 명시 null 로 없애면 Error 다.
    const bad = new ClientDotCommandGlobals().bytes(); assert.throws(() => registerDotCommand(bad, bytes('x'), 1, 1, { copy: () => setDword(bad, 0xa04, 0x1000) }), { name: 'RangeError', message: /인접 표 범위/ });
    assert.throws(() => registerDotCommand(new ClientDotCommandGlobals().bytes(), bytes('x'), 1, 1, { copy: null }), { name: 'Error', message: /strncpy 장치 미연결/ });
    assert.throws(() => registerDotCommand(new Uint8Array(0xa04), bytes('x'), 1, 1), { name: 'RangeError', message: /인접 표 범위/ });
});

// 분배 시험 틀: 슬롯 0 say(0x42f1e0), 1 help(0x1111), 2 Quit(0x2222). invoke/error 호출을 기록한다.
function dispatchRig(results = {}) {
    const globals = new ClientDotCommandGlobals(); globals.register(bytes('help'), 0x1111, 1); globals.register(bytes('Quit'), 0x2222, 1); const log = [];
    const hooks = { invoke: (callback, argument) => { log.push(['invoke', callback.toString(16), argText(argument), argument.length]); return results[callback] ?? 0; }, error: (name, code) => log.push(['error', name, code]) };
    return { globals, log, hooks, table: globals.bytes(), state: globals.dispatchState() };
}
const run = (r, text) => dispatchCall(r, bytes(text));
const dispatchCall = (r, input) => dispatchDotCommand(r.table, r.state, input, r.hooks);

test('dispatch: 빈 입력은 호출 없이 1 이고 세미콜론으로 이어진 명령을 차례로(앞 공백 포함 인자·NUL 포함 길이로) 분배하며 이름은 ASCII 대소문자를 무시한다', () => {
    const empty = dispatchRig(); assert.equal(run(empty, ''), 1); assert.deepEqual(empty.log, []);
    const r = dispatchRig(); assert.equal(run(r, '.help me;.QUIT'), 1); assert.deepEqual(r.log, [['invoke', '1111', ' me', 4], ['invoke', '2222', '', 1]]); assert.equal(r.state.separatorOffset, 5);
    const spaced = dispatchRig(); assert.equal(run(spaced, ' \t\r\n.hElP x'), 1); assert.deepEqual(spaced.log, [['invoke', '1111', ' x', 3]]);
    // 끝의 세미콜론 뒤가 비면 거기서 끝난다. 세미콜론 사이의 인자도 다음 명령에 섞이지 않는다.
    const trailing = dispatchRig(); assert.equal(run(trailing, '.help a;'), 1); assert.deepEqual(trailing.log, [['invoke', '1111', ' a', 3]]);
    const chain = dispatchRig(); run(chain, '.help;.quit;.help z'); assert.deepEqual(chain.log.map((entry) => entry.slice(1, 3)), [['1111', ''], ['2222', ''], ['1111', ' z']]);
});

test('dispatch: 일치하지 않거나 패턴이 아니면 기본 슬롯에 입력 전체(NUL 포함)를 넘기고 1 이며 오류 코드가 있으면 호출 뒤 현재 이름으로 오류를 낸다', () => {
    const r = dispatchRig({ 0x42f1e0: 7 }); assert.equal(run(r, '.nope arg'), 1); assert.deepEqual(r.log, [['invoke', '42f1e0', '.nope arg', 10], ['error', 'say', 7]]);
    for (const input of ['hello', '.', ' .', '.help$x', '.help a$b', '..x']) { const x = dispatchRig(); assert.equal(run(x, input), 1, input); assert.equal(x.log[0][1], '42f1e0', input); assert.equal(x.log[0][2], input, input); }
    // 마지막 일치 시도가 실패면 공유 separatorOffset 은 -1 이다. 선행 입력이 있어도 현재 위치부터 넘긴다.
    const ok = dispatchRig(); run(ok, '.help a;nope'); assert.deepEqual(ok.log.map((entry) => entry[2]), [' a', 'nope']);
    // 호출 코드가 0 이면 오류를 읽지 않는다(error 장치가 없어도 통과).
    const quiet = dispatchRig(); quiet.hooks.error = undefined; assert.equal(run(quiet, '.help'), 1);
    const loud = dispatchRig({ 0x1111: 3 }); loud.hooks.error = undefined; assert.throws(() => run(loud, '.help'), { name: 'Error', message: /오류 출력 장치 미연결/ });
});

test('dispatch: 이름은 80바이트, 인자는 127바이트까지만 쓰고(그 뒤는 잘림), 이름표의 첫 중복이 선택되며 signed count 밖의 슬롯은 보지 않는다', () => {
    const r = dispatchRig(); r.globals.register(bytes('help'), 0x9999, 1); run(r, '.help'); assert.equal(r.log[0][1], '1111');
    const long = dispatchRig(); run(long, `.help ${'x'.repeat(200)}`); assert.equal(long.log[0][2].length, 127); assert.equal(long.log[0][3], 128); assert.equal(long.log[0][2], ` ${'x'.repeat(126)}`);
    // 이름이 80바이트를 넘으면 앞 80바이트로 비교한다(표의 슬롯 이름은 31바이트라 일치하지 않고 기본 슬롯).
    const name = dispatchRig(); run(name, `.${'q'.repeat(90)} a`); assert.equal(name.log[0][1], '42f1e0');
    const negative = dispatchRig(); setDword(negative.table, 0xa04, 0xffffffff); run(negative, '.help'); assert.equal(negative.log[0][1], '42f1e0');
    const hidden = dispatchRig(); setDword(hidden.table, 0xa04, 2); run(hidden, '.quit'); assert.equal(hidden.log[0][1], '42f1e0');
});

test('dispatch: callback 이 같은 표/검색 상태를 바꿔도 현재 값을 다시 읽는다(기본 슬롯·이름·중첩 분배의 separatorOffset)', () => {
    const r = dispatchRig({ 0x42f1e0: 5 }); r.hooks.invoke = (callback, argument) => { r.log.push(['invoke', callback.toString(16), argText(argument)]); if (callback === 0x42f1e0) { r.state.fallbackIndex = 1; } return 5; };
    run(r, '.zzz'); assert.deepEqual(r.log, [['invoke', '42f1e0', '.zzz'], ['error', 'help', 5]]);
    // 중첩 분배가 separatorOffset 을 바꾸면 바깥 분배는 현재 값을 쓴다: 안쪽 호출이 마지막 검색 상태를 0 으로 남긴다.
    const nested = dispatchRig(); let depth = 0; nested.hooks.invoke = (callback, argument) => { nested.log.push(['invoke', callback.toString(16), argText(argument)]); if (callback === 0x1111 && depth === 0) { depth += 1; dispatchDotCommand(nested.table, nested.state, bytes('.quit'), nested.hooks); } return 0; };
    run(nested, '.help a;.quit');
    // 안쪽 분배가 공유 separatorOffset 을 5 로 남기므로 바깥은 현재 값(5)으로 다음 위치(' a;.quit')를 잡아 일치하지 않는 입력으로 기본 슬롯에 넘긴다.
    assert.deepEqual(nested.log.map((entry) => [entry[1], entry[2]]), [['1111', ' a'], ['2222', ''], ['42f1e0', ' a;.quit']]);
    // 호출 뒤 이름이 바뀌어도 오류 이름은 현재 물리에서 읽는다.
    const renamed = dispatchRig({ 0x1111: 9 }); renamed.hooks.invoke = () => { copyDotCommandName(renamed.table.subarray(32, 63), bytes('renamed'), 31); return 9; }; run(renamed, '.help'); assert.deepEqual(renamed.log, [['error', 'renamed', 9]]);
});

test('dispatch 오류: 다음 구분자 위치가 입력 밖이면 RangeError, 입력/표에 NUL 이 없으면 RangeError, 함수 장치 누락은 Error 다', () => {
    const r = dispatchRig(); r.hooks.invoke = (callback) => { r.state.separatorOffset = 1000; return 0; }; assert.throws(() => run(r, '.help'), { name: 'RangeError', message: /다음 구분자/ });
    const negative = dispatchRig(); negative.hooks.invoke = () => { negative.state.separatorOffset = -5; return 0; }; assert.throws(() => run(negative, '.help'), RangeError);
    assert.throws(() => dispatchCall(dispatchRig(), Uint8Array.from([0x2e, 0x68])), { name: 'RangeError', message: /NUL 없음/ });
    assert.throws(() => dispatchDotCommand(new Uint8Array(0x10), new DotCommandDispatchState(), bytes('.x'), { invoke() {} }), RangeError);
    const noInvoke = dispatchRig(); noInvoke.hooks.invoke = undefined; assert.throws(() => run(noInvoke, '.help'), { name: 'Error', message: /실제 함수 장치 미연결/ });
    assert.deepEqual([new DotCommandDispatchState().fallbackIndex, new DotCommandDispatchState().separatorOffset], [0, -1]);
});

test('경계(등록): 원문 주소 식별자 값, 원본이 count 보다 정확히 1 바이트 짧을 때의 오류, count 0(첫 슬롯)의 등록, 슬롯 끝 바이트 쓰기', () => {
    assert.equal(DOT_COMMAND_NAMES_TOKEN, 0x531b30); assert.equal(DOT_COMMAND_EMPTY_TOKEN, 0x503c8b);
    assert.throws(() => copyDotCommandName(new Uint8Array(8), Uint8Array.from([0x61, 0x62]), 3), { name: 'RangeError', message: /원본 물리/ });
    const state = new Uint8Array(0xa08).fill(0x55); setDword(state, 0xa04, 0); assert.equal(registerDotCommand(state, bytes('zero'), 0x77, 1), 1);
    assert.equal(nameOf(state, 0), 'zero'); assert.equal(state[31], 0); assert.equal(state[32], 0x55); assert.equal(dword(state, 0x800), 0x77); assert.equal(dword(state, 0x900), DOT_COMMAND_NAMES_TOKEN); assert.equal(dword(state, 0xa04), 1);
});

test('경계(전역): 비영 바이트는 PE 정적 이미지의 열 바이트뿐이다', () => {
    const state = new ClientDotCommandGlobals().bytes(); const nonzero = []; state.forEach((value, index) => { if (value) nonzero.push(index); });
    assert.deepEqual(nonzero, [0, 1, 2, 0x800, 0x801, 0x802, 0x900, 0x901, 0x902, 0xa04]);
});

test('경계(분배): 이름 비교는 A/Z 를 접고 @/[ 는 접지 않으며 모든 글자를 비교하고, 검색 실패는 separatorOffset 을 -1 로 남기며 선행 공백 1/3칸도 건너뛴다', () => {
    const r = dispatchRig(); r.globals.register(bytes('Alpha'), 0x3331, 1); r.globals.register(bytes('Zeta'), 0x3332, 1); r.globals.register(bytes('[x'), 0x3333, 1); r.globals.register(bytes('@y'), 0x3334, 1);
    for (const [input, callback] of [['.alpha', '3331'], ['.ALPHA', '3331'], ['.zeta', '3332'], ['.ZETA', '3332'], ['.[x', '3333'], ['.@y', '3334']]) { r.log.length = 0; run(r, input); assert.equal(r.log[0][1], callback, input); }
    for (const input of ['.{x', '.`y', '.xelp', '.hxlp', '.helq', '.hel']) { r.log.length = 0; run(r, input); assert.equal(r.log[0][1], '42f1e0', input); }
    run(r, '.help$x'); assert.equal(r.state.separatorOffset, -1); run(r, 'nothing'); assert.equal(r.state.separatorOffset, -1);
    for (const input of [' .help', '   .help', '\t.help']) { r.log.length = 0; run(r, input); assert.equal(r.log[0][1], '1111', JSON.stringify(input)); }
});

test('경계(분배): 이름이 구분자 앞에서 끝나는 홀수 길이 이름, 첫 슬롯(say), 입력 뒤에 NUL 이 아닌 바이트가 더 있는 기본 슬롯 호출, 다음 위치 0 허용', () => {
    const r = dispatchRig(); r.globals.register(bytes('abc'), 0x4444, 1); run(r, '.abc;.help'); assert.deepEqual(r.log.map((entry) => entry[1]), ['4444', '1111']);
    const say = dispatchRig(); run(say, '.say hi'); assert.deepEqual(say.log, [['invoke', '42f1e0', ' hi', 4]]);
    const trailing = dispatchRig(); dispatchCall(trailing, Uint8Array.from([0x2e, 0x6e, 0x6f, 0x70, 0x65, 0, 0x41, 0])); assert.deepEqual(trailing.log, [['invoke', '42f1e0', '.nope', 6]]);
    // 호출이 separatorOffset 을 0 으로 남기면 다음 위치는 같은 위치(0)이며 오류가 아니다. 두 번째 호출부터는 원래 값이다.
    const zero = dispatchRig(); let times = 0; zero.hooks.invoke = (callback, argument) => { times += 1; zero.log.push(['invoke', callback.toString(16)]); if (times === 1) zero.state.separatorOffset = 0; return 0; };
    assert.equal(run(zero, '.help'), 1); assert.deepEqual(zero.log, [['invoke', '1111'], ['invoke', '1111']]);
    // 표가 DWORD 를 읽을 만큼 길지 않으면 RangeError 다(count 위치에서 2바이트 부족).
    assert.throws(() => dispatchDotCommand(new Uint8Array(0xa06), new DotCommandDispatchState(), bytes('.x'), { invoke() {} }), { name: 'RangeError', message: /DWORD의 실제 물리 범위/ });
});
