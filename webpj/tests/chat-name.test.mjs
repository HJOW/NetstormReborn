// 전용 PE fixture가 없는 Chat 대상 이름 검사(ChatRecipients)와 이름 자동완성(ChatNameCompletion)을 기준선 소스 계약으로 검증한다. (2026-11-14 WB04-E436)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 순서/경계는 기준선 소스(ChatRecipients.cpp, ChatNameCompletion.cpp)에서 옮겼다. Identity 조회/번역/현재 Chat 기록은 호출 기록 대역이고 레코드 초기화는 실제 RawPlayerIdentity.initialize 다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { checkChatRecipients } from '../www/ui/chat-recipients.js';
import { ChatNameCompletionState, completeChatName } from '../www/ui/chat-name-completion.js';

const bytes = (text) => Uint8Array.from(Buffer.from(text, 'latin1'));
const nameOf = (record) => { let end = 8; while (end < 60 && record[end] !== 0) end += 1; return Buffer.from(record.subarray(8, end)).toString('latin1'); };
// 이름 표(known)에 있으면 true 를 돌려주는 Identity 대역과 번역 대역이다. 모든 호출을 기록한다.
function rig(known = ['Ann', 'Bob'], translations = { BATTLE: 'Battle', EVERYONE: 'Everyone', ALLIES: 'Allies', OBSERVER: 'Observer' }) {
    const log = []; const hooks = { resolve: (record) => { log.push(['resolve', nameOf(record)]); return known.includes(nameOf(record)); }, translate: (key) => { log.push(['translate', key]); return translations[key] ?? ''; } };
    return { log, hooks, scratch: new Uint8Array(60) };
}
const names = (log) => log.filter((entry) => entry[0] === 'resolve').map((entry) => entry[1]);

test('checkChatRecipients: manager 0 이거나 text 가 없으면 false, 공백뿐인 나머지는 하위 조회 없이 true 이며 레코드는 mode 0 으로 초기화한다', () => {
    const r = rig(); r.scratch.fill(0x55);
    assert.equal(checkChatRecipients(0, 'Ann', r.scratch, r.hooks), false); assert.equal(checkChatRecipients(5, null, r.scratch, r.hooks), false); assert.equal(checkChatRecipients(5, undefined, r.scratch, r.hooks), false); assert.deepEqual(r.log, []); assert.ok(r.scratch.every((byte) => byte === 0x55));
    assert.equal(checkChatRecipients(5, '', r.scratch, {}), true); assert.equal(checkChatRecipients(5, ' \t\r\n ', r.scratch, {}), true); assert.equal(r.scratch[8], 0); assert.deepEqual(Array.from(r.scratch.subarray(0, 8)), [0, 0, 0, 0, 0, 0, 0, 0]);
    // 첫 NUL 뒤는 입력이 아니다.
    assert.equal(checkChatRecipients(5, '\0Ann', r.scratch, {}), true); assert.equal(checkChatRecipients(5, bytes('  '), r.scratch, {}), true);
});

test('checkChatRecipients: CR/LF/TAB/공백으로 토큰을 나누고(VT/FF/NBSP 는 이름의 일부) 각 토큰을 이름 40바이트로 복사해 Identity 를 조회한다', () => {
    const r = rig(['Ann', 'Bob', 'a\x0bb', 'c\xa0d']); assert.equal(checkChatRecipients(5, '  Ann\tBob\r\nAnn  ', r.scratch, r.hooks), true); assert.deepEqual(names(r.log), ['Ann', 'Bob', 'Ann']); assert.deepEqual(r.log.filter((entry) => entry[0] === 'translate'), []);
    const odd = rig(['a\x0bb', 'c\xa0d']); assert.equal(checkChatRecipients(5, 'a\x0bb c\xa0d', odd.scratch, odd.hooks), true); assert.deepEqual(names(odd.log), ['a\x0bb', 'c\xa0d']);
    // 40바이트를 넘는 이름은 40바이트만 복사하고 다음 토큰은 절단 전 끝에서 시작한다.
    const long = rig(['A'.repeat(40), 'Bob']); assert.equal(checkChatRecipients(5, `${'A'.repeat(45)} Bob`, long.scratch, long.hooks), true); assert.deepEqual(names(long.log), ['A'.repeat(40), 'Bob']);
    const exact = rig(['B'.repeat(40)]); assert.equal(checkChatRecipients(5, 'B'.repeat(40), exact.scratch, exact.hooks), true); assert.equal(exact.scratch[48], 0);
    // 바이트 문자열/Uint8Array 입력과 확장 바이트는 그대로 비교한다.
    const raw = rig(['\xe9t\xe9']); assert.equal(checkChatRecipients(5, bytes('\xe9t\xe9'), raw.scratch, raw.hooks), true);
});

test('checkChatRecipients: Identity 실패 때만 여덟 채널 키를 순서대로 번역해 이름과(ASCII 대소문자 무시) 비교하며 일치하면 멈추고 모두 실패하면 false 다', () => {
    const r = rig(); assert.equal(checkChatRecipients(5, 'everyone', r.scratch, r.hooks), true);
    assert.deepEqual(r.log.filter((entry) => entry[0] === 'translate').map((entry) => entry[1]), ['', '', '', 'BATTLE', '', 'EVERYONE']);
    const none = rig(); assert.equal(checkChatRecipients(5, 'Ann Nobody', none.scratch, none.hooks), false); assert.deepEqual(none.log.filter((entry) => entry[0] === 'translate').map((entry) => entry[1]), ['', '', '', 'BATTLE', '', 'EVERYONE', 'ALLIES', 'OBSERVER']);
    // 확장 바이트는 접지 않는다. 길이가 다르면 다르다. 번역의 첫 NUL 뒤는 무시한다. 빈 번역은 일치하지 않는다.
    const ext = rig([], { BATTLE: '\xc9' }); assert.equal(checkChatRecipients(5, '\xe9', ext.scratch, ext.hooks), false);
    const prefix = rig([], { BATTLE: 'Battle\0junk' }); assert.equal(checkChatRecipients(5, 'BATTLE', prefix.scratch, prefix.hooks), true); const longer = rig([], { BATTLE: 'Battles' }); assert.equal(checkChatRecipients(5, 'battle', longer.scratch, longer.hooks), false);
    const empty = rig([], {}); assert.equal(checkChatRecipients(5, 'x', empty.scratch, empty.hooks), false);
    // 앞 토큰이 통과해도 뒤 토큰이 실패하면 false 이고 그 뒤 토큰은 조회하지 않는다.
    const stop = rig(['Ann']); assert.equal(checkChatRecipients(5, 'Ann Zed Bob', stop.scratch, stop.hooks), false); assert.deepEqual(names(stop.log), ['Ann', 'Zed']);
});

test('checkChatRecipients: 레코드는 토큰마다 선택 필드만 지우고(앞 8·이름 첫 바이트·+48..52) 나머지 꼬리를 보존하며 장치는 도달 시에만 요구한다', () => {
    const state = new Uint8Array(60); const seen = []; const hooks = { resolve: (record) => { seen.push(Array.from(record)); record.fill(0x77, 0, 8); record[20] = 0x66; return true; } };
    checkChatRecipients(5, 'Ann Bob', state, hooks); assert.deepEqual(seen[1].slice(0, 8), [0, 0, 0, 0, 0, 0, 0, 0]); assert.equal(seen[1][20], 0x66); assert.deepEqual(seen[1].slice(8, 12), [66, 111, 98, 0]);
    // 이전 토큰의 이름 꼬리는 새 NUL 뒤에 남는다(LongName 다음 Hi 는 Hi, NUL, gName 이 된다).
    const tail = new Uint8Array(60); const copies = []; checkChatRecipients(5, 'LongName Hi', tail, { resolve: (record) => { copies.push(Array.from(record.subarray(8, 17))); return true; } });
    assert.deepEqual(copies, [[...Buffer.from('LongName'), 0], [72, 105, 0, ...Buffer.from('gName'), 0]]);
    assert.throws(() => checkChatRecipients(5, 'Ann', new Uint8Array(60), {}), (error) => error.constructor === Error && /선택 장치 미연결/.test(error.message));
    const noTranslate = { resolve: () => false }; assert.throws(() => checkChatRecipients(5, 'Ann', new Uint8Array(60), noTranslate), /선택 장치 미연결/);
    // 조회 콜백이 이름 영역을 NUL 없이 채우면 물리 끝 거부다.
    assert.throws(() => checkChatRecipients(5, 'Ann', new Uint8Array(60), { resolve: (record) => { record.fill(0x41, 8, 60); return false; }, translate: () => 'x' }), (error) => error instanceof RangeError && /NUL 물리 끝/.test(error.message));
});

// 번역/현재 Chat 100행(이름 +0, Subscriber +44)을 공급하는 자동완성 장치다. rows = [[이름, subscriber], ...].
function chat(rows = [['Ann', 1], ['alice', 2], ['Bob', 0], ['Anna', 3]], options = {}) {
    const log = []; const records = new Uint8Array(4800); rows.forEach(([name, subscriber], index) => { records.set(bytes(name), index * 48); records[index * 48 + 44] = subscriber; });
    const hooks = { translate: (key) => { log.push(['translate', key]); return { BATTLE: 'Battle', EVERYONE: 'Everyone', ALLIES: 'Allies', OBSERVER: 'Observer' }[key]; }, records: () => { log.push(['records']); return options.records ?? records; }, assertion: (code) => log.push(['assertion', code]) };
    for (const key of options.omit ?? []) delete hooks[key]; return { log, hooks, state: new ChatNameCompletionState(), records };
}
const read = (state, offset) => (state.bytes[offset] | (state.bytes[offset + 1] << 8) | (state.bytes[offset + 2] << 16) | (state.bytes[offset + 3] << 24)) >>> 0;
const candidate = (state, index) => { let end = 0x90 + index * 128; const start = end; while (state.bytes[end] !== 0) end += 1; return Buffer.from(state.bytes.subarray(start, end)).toString('latin1'); };

test('ChatNameCompletionState: 0x4090 바이트 0 으로 시작한다', () => { const state = new ChatNameCompletionState(); assert.equal(state.bytes.length, 0x4090); assert.ok(state.bytes.every((byte) => byte === 0)); });

test('새 질의: 비어 있지 않은 네 채널 번역과 Subscriber 가 있는 Chat 행 이름을 후보로 모으고 접두/길이/인덱스/개수를 쓴 뒤 첫 일치 후보를 반환한다', () => {
    const c = chat(); assert.equal(completeChatName(c.state, 'a', 1, false, c.hooks), 'Allies');
    assert.deepEqual(c.log, [['translate', 'BATTLE'], ['translate', 'EVERYONE'], ['translate', 'ALLIES'], ['translate', 'OBSERVER'], ['records']]);
    assert.deepEqual([read(c.state, 0x88), read(c.state, 4), read(c.state, 0), c.state.bytes[8]], [7, 1, 2, 'a'.charCodeAt(0)]); assert.deepEqual([0, 1, 2, 3, 4, 5, 6, 7].map((index) => candidate(c.state, index)), ['Battle', 'Everyone', 'Allies', 'Observer', 'Ann', 'alice', 'Anna', '']);
    // 이후 NULL 질의는 같은 후보에서 다음/이전 일치로 순환하며 번역/기록을 다시 읽지 않는다.
    c.log.length = 0; assert.equal(completeChatName(c.state, null, 0, false, c.hooks), 'Ann'); assert.equal(completeChatName(c.state, undefined, 0, false, c.hooks), 'alice'); assert.equal(completeChatName(c.state, null, 0, false, c.hooks), 'Anna'); assert.deepEqual(c.log, []);
    // 마지막 일치 뒤에는 한 번 감겨 처음부터 다시 찾는다. 역방향은 거꾸로 순환한다.
    assert.equal(completeChatName(c.state, null, 0, false, c.hooks), 'Allies'); assert.equal(completeChatName(c.state, null, 0, true, c.hooks), 'Anna'); assert.equal(completeChatName(c.state, null, 0, true, c.hooks), 'alice');
});

test('질의 길이와 대소문자: 접두 비교는 strnicmp 처럼 ASCII 대소문자를 무시하고 길이/NUL 에서 멈추며 접두 길이 0 은 모든 후보와 일치한다', () => {
    const c = chat(); assert.equal(completeChatName(c.state, 'AL', 2, false, c.hooks), 'Allies'); assert.equal(completeChatName(c.state, null, 0, false, c.hooks), 'alice'); assert.equal(completeChatName(c.state, null, 0, false, c.hooks), 'Allies');
    // 길이가 질의보다 길면 NUL 패딩과 비교되어 후보가 더 짧거나 같을 때만 일치한다(Anna 는 네 번째 글자가 패딩 NUL 과 달라 건너뛴다).
    assert.equal(completeChatName(c.state, 'Ann', 5, false, c.hooks), 'Ann'); assert.equal(completeChatName(c.state, null, 0, false, c.hooks), 'Ann'); assert.equal(completeChatName(c.state, 'Ann', 3, false, c.hooks), 'Ann'); assert.equal(completeChatName(c.state, null, 0, false, c.hooks), 'Anna');
    // 길이 0 은 첫 후보와 일치한다(질의 문자열이 비어 있어도).
    assert.equal(completeChatName(c.state, '', 0, false, c.hooks), 'Battle'); assert.equal(completeChatName(c.state, 'zzz', 0, false, c.hooks), 'Battle');
    // 질의의 첫 NUL 뒤는 접두가 아니다. 이전 접두의 꼬리는 새 길이 밖에서 남는다.
    const t = chat(); completeChatName(t.state, 'abcdef', 6, false, t.hooks); completeChatName(t.state, 'xy\0zz', 2, false, t.hooks); assert.deepEqual(Array.from(t.state.bytes.subarray(8, 14)), [120, 121, 99, 100, 101, 102]);
});

test('후보 수집 조건: Subscriber 가 0 인 행은 건너뛰고 128 이상의 이름은 제외하며 이전 후보의 뒤쪽 바이트는 지우지 않는다', () => {
    const rows = [['LongerNameThanBefore', 1], ['x', 0], ['y', 4]]; const c = chat(rows); completeChatName(c.state, 'l', 1, false, c.hooks); assert.equal(read(c.state, 0x88), 6); assert.deepEqual([4, 5].map((index) => candidate(c.state, index)), ['LongerNameThanBefore', 'y']);
    const shorter = chat([['Z', 1]]); shorter.state.bytes.set(c.state.bytes); completeChatName(shorter.state, 'z', 1, false, shorter.hooks); assert.equal(candidate(shorter.state, 4), 'Z'); assert.equal(shorter.state.bytes[0x90 + 4 * 128 + 2], 'n'.charCodeAt(0));
    // 이름 칸이 이어져 128 이상이 되는 행은 후보에서 제외한다.
    const wide = chat([], {}); wide.records.fill(0x78, 0, 200); wide.records[44] = 1; completeChatName(wide.state, 'q', 1, false, wide.hooks); assert.equal(read(wide.state, 0x88), 6); assert.deepEqual([candidate(wide.state, 4).length, candidate(wide.state, 5).length], [104, 56]);
    // 후보가 없으면 undefined 를 돌려준다(한 번 감긴 뒤).
    const none = chat([]); assert.equal(completeChatName(none.state, 'qq', 2, false, none.hooks), undefined);
});

test('순환 경계: 저장 후보가 없을 때의 NULL 질의·역방향 감김·signed 인덱스', () => {
    const empty = new ChatNameCompletionState(); const hooks = {}; assert.equal(completeChatName(empty, null, 0, false, hooks), undefined); assert.equal(completeChatName(empty, null, 0, true, hooks), undefined);
    // 역방향은 인덱스가 음수가 되면 후보 수로 감겨 마지막 후보에서 다시 찾는다.
    const c = chat(); completeChatName(c.state, 'o', 1, false, c.hooks); assert.equal(read(c.state, 0), 3); assert.equal(completeChatName(c.state, null, 0, true, c.hooks), 'Observer'); assert.equal(read(c.state, 0), 3);
    const d = chat(); completeChatName(d.state, '', 0, false, d.hooks); assert.equal(read(d.state, 0), 0); assert.equal(completeChatName(d.state, null, 0, true, d.hooks), 'Anna'); assert.equal(read(d.state, 0), 6);
});

test('원문 진단/범위: 길이 127 이상은 진단 복귀 뒤 복사를 계속하고 장치 누락/잘못된 기록/물리 범위 밖은 명시 거부한다', () => {
    const c = chat(); assert.equal(completeChatName(c.state, 'a'.repeat(130), 126, false, c.hooks), undefined); assert.equal(c.log.some((entry) => entry[0] === 'assertion'), false);
    c.log.length = 0; completeChatName(c.state, 'a'.repeat(130), 127, false, c.hooks); assert.deepEqual(c.log.filter((entry) => entry[0] === 'assertion'), [['assertion', 0x354]]); assert.equal(read(c.state, 4), 127); assert.equal(c.state.bytes[8 + 126], 'a'.charCodeAt(0)); assert.equal(c.state.bytes[8 + 127], 0);
    assert.throws(() => completeChatName(chat().state, 'a', 127, false, chat([], { omit: ['assertion'] }).hooks), (error) => error.constructor === Error && /원문 길이 진단 필요/.test(error.message));
    for (const [omit, pattern] of [['translate', /실제 번역 필요/], ['records', /현재 Chat 기록 필요/]]) { const g = chat([], { omit: [omit] }); assert.throws(() => completeChatName(g.state, 'a', 1, false, g.hooks), pattern, omit); }
    const bad = chat([], { records: new Uint8Array(4799) }); assert.throws(() => completeChatName(bad.state, 'a', 1, false, bad.hooks), (error) => error instanceof RangeError && /100행 필요/.test(error.message));
    const huge = chat(); assert.throws(() => completeChatName(huge.state, 'a', 0x4090, false, huge.hooks), /strncpy 물리 범위/); assert.throws(() => completeChatName(chat().state, 'a', -1, false, chat().hooks), /strncpy 물리 범위/); assert.doesNotThrow(() => completeChatName(chat().state, 'a', 0x4090 - 8, false, chat().hooks));
    // NUL 이 없는 기록 이름은 물리 범위 거부다.
    const noNul = chat([]); noNul.records.fill(0x78, 0, 4800); assert.throws(() => completeChatName(noNul.state, 'a', 1, false, noNul.hooks), /NUL 범위/);
});

test('checkChatRecipients(경계): 초기화 필드·토큰별 지움 범위(주소 48..51 지움, 모드 52 보존)·이름 NUL 이 레코드 끝 바로 앞일 때·ASCII 접기 경계·긴 이름의 글자 단위 비교', () => {
    // 초기화는 mode 0 을 포함해 레코드의 선택 필드만 바꾼다(앞 8·이름 첫 바이트·주소 48..51·모드 52..55). 나머지는 그대로다.
    const scratch = new Uint8Array(60).fill(0x55); checkChatRecipients(5, '', scratch, {});
    assert.deepEqual(Array.from(scratch), Array.from({ length: 60 }, (_, index) => (index < 9 || (index >= 48 && index < 56) ? 0 : 0x55)));
    // 토큰마다 주소 48..51 은 지우고 모드(52..55)와 +56.. 는 지우지 않는다. 앞 8 바이트(바이트 0 포함)도 지운다.
    const seen = []; const state = new Uint8Array(60); checkChatRecipients(5, 'Ann Bob', state, { resolve: (record) => { seen.push(Array.from(record)); record.fill(0x66, 0, 8); record.fill(0x77, 48, 60); return true; } });
    assert.deepEqual(seen[1].slice(0, 8), [0, 0, 0, 0, 0, 0, 0, 0]); assert.deepEqual(seen[1].slice(48, 60), [0, 0, 0, 0, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77, 0x77]);
    // 이름 NUL 이 레코드의 마지막 바이트(59)에만 있어도 채널 비교는 정상이다(홀수 위치 NUL 을 건너뛰지 않는다).
    const edge = rig([], { BATTLE: 'x' }); const lastNul = { resolve: (record) => { record.fill(0x41, 8, 59); record[59] = 0; return false; }, translate: () => 'x' }; assert.equal(checkChatRecipients(5, 'Ann', edge.scratch, lastNul), false);
    // ASCII 접기는 A..Z 만이다('Z'/'z' 는 같고 '['/'{' 와 '@'/'`' 는 다르다). 길이가 같아도 마지막 글자까지 모두 비교한다.
    const fold = (key, name) => { const f = rig([], { BATTLE: key }); return checkChatRecipients(5, name, f.scratch, f.hooks); };
    assert.equal(fold('z', 'Z'), true); assert.equal(fold('Z', 'z'), true); assert.equal(fold('{', '['), false); assert.equal(fold('[', '{'), false); assert.equal(fold('`', '@'), false); assert.equal(fold('Battlx', 'BATTLE'), false); assert.equal(fold('battle', 'BATTLX'), false); assert.equal(fold('battle', 'BATTLE'), true);
});

test('completeChatName(경계): 큰 질의 길이는 DWORD 전체로 쓰고 선두 NUL 질의는 빈 접두이며 후보 복사/기록 이름 길이 경계와 래핑 시작 위치', () => {
    // 길이 256(1 바이트를 넘는 값)은 DWORD 로 저장하고 첫 NUL 에서 비교를 멈춘다.
    const c = chat(); completeChatName(c.state, 'al', 256, false, c.hooks); assert.equal(read(c.state, 4), 256); assert.deepEqual(c.log.filter((entry) => entry[0] === 'assertion'), [['assertion', 0x354]]);
    // 선두 NUL 질의는 접두가 모두 0 이다(질의의 뒤쪽 글자를 복사하지 않는다).
    const d = chat(); completeChatName(d.state, '\0abc', 4, false, d.hooks); assert.deepEqual(Array.from(d.state.bytes.subarray(8, 12)), [0, 0, 0, 0]);
    // 번역이 NUL 로 시작하면 빈 후보이고 상태 크기를 넘는 뒤쪽 글자는 복사하지 않는다.
    const e = chat(); e.hooks.translate = (key) => (key === 'BATTLE' ? `\0${'x'.repeat(20000)}` : 'ok'); completeChatName(e.state, 'o', 1, false, e.hooks); assert.equal(candidate(e.state, 0), ''); assert.equal(candidate(e.state, 1), 'ok');
    // 번역의 첫 NUL 뒤는 후보에 복사하지 않는다(앞쪽 글자는 모두 복사한다).
    const nulCut = chat(); nulCut.hooks.translate = (key) => (key === 'BATTLE' ? 'ok junk' : 'x'); completeChatName(nulCut.state, 'o', 1, false, nulCut.hooks); assert.equal(candidate(nulCut.state, 0), 'ok'); assert.equal(nulCut.state.bytes[0x90 + 3], 0);
    // 번역 길이가 후보 위치 뒤 남은 상태 폭 이상이면 거부한다(첫 후보 위치 0x90 에서 0x4000 바이트 남아 16383 은 통과, 16384 는 거부).
    const f = chat(); f.hooks.translate = (key) => (key === 'BATTLE' ? 'a'.repeat(16383) : 'ok'); assert.doesNotThrow(() => completeChatName(f.state, 'a', 1, false, f.hooks)); const g = chat(); g.hooks.translate = (key) => (key === 'BATTLE' ? 'a'.repeat(16384) : 'ok'); assert.throws(() => completeChatName(g.state, 'a', 1, false, g.hooks), (error) => error instanceof RangeError && /후보 복사 물리 범위/.test(error.message));
    // 기록 이름 길이가 정확히 128 이면 후보에서 제외하고(127 은 포함) 영역 첫 행과 행 사이를 이어 읽는다.
    const h = chat([]); h.records.fill(0x78, 0, 128); completeChatName(h.state, 'q', 1, false, h.hooks); assert.equal(read(h.state, 0x88), 5); assert.equal(candidate(h.state, 4).length, 80);
    const i = chat([]); i.records.fill(0x78, 0, 127); completeChatName(i.state, 'q', 1, false, i.hooks); assert.equal(read(i.state, 0x88), 6); assert.equal(candidate(i.state, 4).length, 127);
    // 순방향 감김은 인덱스 0 부터 다시 찾는다(후보 0 만 일치할 때 한 번 더 반환한다).
    const j = chat(); assert.equal(completeChatName(j.state, 'b', 1, false, j.hooks), 'Battle'); assert.equal(completeChatName(j.state, null, 0, false, j.hooks), 'Battle');
    // ASCII 접기: 'Z'/'z' 와 '['/'{' 를 구별하고 확장 바이트는 접지 않는다.
    const k = chat([['Zed', 1], ['[x', 2], ['\xc9a', 3]]); assert.equal(completeChatName(k.state, 'z', 1, false, k.hooks), 'Zed'); assert.equal(completeChatName(k.state, '{', 1, false, k.hooks), undefined); assert.equal(completeChatName(k.state, '\xe9', 1, false, k.hooks), undefined); assert.equal(completeChatName(k.state, '\xc9', 1, false, k.hooks), '\xc9a');
});
