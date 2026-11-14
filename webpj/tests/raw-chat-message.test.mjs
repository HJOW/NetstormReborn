// Chat 본문 조합 raw-chat-message(formatChatMessage·formatChatInput·sayChatMessage·writeChatNameFormat)를 기준선 소스 계약과 손 계산 바이트로 검증한다.
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 순서/경계는 기준선 소스(RawChatMessage.cpp)에서 옮겼다. 실제 게임·브라우저 실행이 아니다. (2026-11-14 WB04-E446)
import test from 'node:test';
import assert from 'node:assert/strict';
import { CHAT_MESSAGE_NAME_BYTES, CHAT_MESSAGE_OFFSET, formatChatInput, formatChatMessage, sayChatMessage, writeChatNameFormat } from '../www/ui/raw-chat-message.js';
import { stripChatFormatSlots } from '../www/ui/raw-chat-format.js';

// 바이트 문자열을 NUL 포함 바이트 배열로 바꾸고, 바이트 배열의 NUL 앞 문자열을 읽는 보조 함수다.
const bytes = (value) => Uint8Array.from([...Array.from(value, (char) => char.charCodeAt(0) & 0xff), 0]);
const textAt = (array, at = 0) => { let value = ''; for (let index = at; array[index]; index += 1) value += String.fromCharCode(array[index]); return value; };
const putDword = (array, at, value) => { for (let index = 0; index < 4; index += 1) array[at + index] = Math.floor(value / 2 ** (8 * index)) & 0xff; };
const putText = (array, at, value) => { for (let index = 0; index < value.length; index += 1) array[at + index] = value.charCodeAt(index); array[at + value.length] = 0; };

// 기록 틀이다. manager 1(2712바이트), 수신자 2(count/+4/+8 배열 토큰/+12 직접 표시), 배열 3(subscriber 4개 칸)이다. back 서식 "B:"(+0x14), front 서식 "F:"(+0x50), 현재 이름 Bob, subscriber 7 이다.
function rig({ count = 1, direct = 0, managerDirect = 0, flags = 0, array = [7], name = 'Bob' } = {}) {
    const log = []; const manager = new Uint8Array(2712); const recipients = new Uint8Array(16); const list = new Uint8Array(16); const memory = new Map([[1, manager], [2, recipients], [3, list]]);
    putDword(manager, 12, managerDirect); putDword(manager, 16, flags); putText(manager, 0x14, 'B:'); putText(manager, 0x50, 'F:'); putDword(recipients, 0, count); putDword(recipients, 8, 3); putDword(recipients, 12, direct);
    array.forEach((value, index) => putDword(list, index * 4, value));
    const state = { name, subscriber: 7, current: 1 };
    const hooks = {
        memory: (token) => { const found = memory.get(token); if (!found) throw new RangeError('토큰 없음'); return found; }, currentName: () => state.name,
        nameForSubscriber: (work, token) => { log.push(['nameFor', token]); putText(work, 0, 'Zed'); return token; },
        deliver: (target, source, delivered) => { log.push(['deliver', target, source, textAt(delivered), delivered.length]); return 9; }, inputVisible: (value) => log.push(['visible', value]),
    };
    return { hooks, manager, recipients, list, log, state, globals: { get current() { return state.current; }, get subscriber() { return state.subscriber; } }, work: new Uint8Array(CHAT_MESSAGE_NAME_BYTES).fill(0x55) };
}

test('formatChatMessage 기본: back 서식 → 이름("%s> ") → front 서식 → 본문 순으로 manager +0x298 에 쓰고 NUL 포함 길이로 전달하며 반환값을 돌려준다', () => {
    const r = rig(); assert.equal(CHAT_MESSAGE_OFFSET, 0x298);
    const result = formatChatMessage(1, 2, bytes('hello'), null, r.work, r.globals, r.hooks); assert.equal(result, 9);
    assert.deepEqual(r.log, [['deliver', 1, 2, 'B:Bob> F:hello', 15]]); assert.equal(textAt(r.manager, CHAT_MESSAGE_OFFSET), 'B:Bob> F:hello'); assert.equal(r.work[0], 0);
});

test('formatChatMessage 아이콘: manager +16 의 8/0x40/0x80 은 D36/D37/D39 를 이어 붙이고 0x20 은 D41 을 처음부터 덮어쓰며, +12 비영이면 수신자 1명은 D38 아니면 D40 을 붙인다', () => {
    const cases = [[8, 1, 0, '~[Iicon.D36]B:Bob> F:x'], [0x40, 1, 0, '~[Iicon.D37]B:Bob> F:x'], [0x80, 1, 0, '~[Iicon.D39]B:Bob> F:x'], [0xc8, 1, 0, '~[Iicon.D36]~[Iicon.D37]~[Iicon.D39]B:Bob> F:x'],
        [0x20, 1, 0, '~[Iicon.D41]B:Bob> F:x'], [0x28, 1, 0, '~[Iicon.D41]B:Bob> F:x'], [0, 1, 1, '~[Iicon.D38]B:Bob> F:x'], [0, 2, 1, '~[Iicon.D40]B:Bob> F:x'], [0, 0, 1, '~[Iicon.D40]B:Bob> F:x'], [0x10, 1, 0, 'B:Bob> F:x']];
    for (const [flags, count, managerDirect, expected] of cases) { const r = rig({ flags, count, managerDirect }); formatChatMessage(1, 2, bytes('x'), null, r.work, r.globals, r.hooks); assert.equal(textAt(r.manager, CHAT_MESSAGE_OFFSET), expected, `${flags}/${count}/${managerDirect}`); }
});

test('formatChatMessage 직접 대상: 수신자 +12 비영이면 첫 비자기 수신자 이름을 대상 이름으로 조회하고 front 서식은 붙이지 않는다', () => {
    const r = rig({ count: 3, direct: 1, array: [7, 7, 11] }); formatChatMessage(1, 2, bytes('hi'), '%s (to %s)> ', r.work, r.globals, r.hooks);
    assert.deepEqual(r.log, [['nameFor', 11], ['deliver', 1, 2, 'B:Bob (to Zed)> hi', 19]]); assert.equal(textAt(r.work), 'Zed');
    // 자기만 있거나 count 가 0 이하이면 대상은 없다(이름 작업 버퍼는 지워진 채).
    const self = rig({ count: 2, direct: 1, array: [7, 7] }); formatChatMessage(1, 2, bytes('hi'), '%s (to %s)> ', self.work, self.globals, self.hooks); assert.equal(textAt(self.manager, CHAT_MESSAGE_OFFSET), 'B:Bob (to )> hi'); assert.equal(self.log.some((entry) => entry[0] === 'nameFor'), false);
    for (const count of [0, -1 >>> 0]) { const none = rig({ count, direct: 1 }); formatChatMessage(1, 2, bytes('hi'), '%s> ', none.work, none.globals, none.hooks); assert.equal(none.log.some((entry) => entry[0] === 'nameFor'), false, String(count)); }
    // 조회가 0 을 돌려주면 이름 버퍼를 지운다.
    const zero = rig({ count: 2, direct: 1, array: [7, 5] }); zero.hooks.nameForSubscriber = (work) => { putText(work, 0, 'junk'); return 0; }; formatChatMessage(1, 2, bytes('hi'), '%s (to %s)> ', zero.work, zero.globals, zero.hooks); assert.equal(textAt(zero.manager, CHAT_MESSAGE_OFFSET), 'B:Bob (to )> hi');
    // 첫 비자기가 2번째일 때 비교 기준은 배열 토큰을 먼저 고정하지만 반환은 현재 배열을 다시 읽는다.
    const swapped = rig({ count: 2, direct: 1, array: [7, 5] }); const original = swapped.hooks.memory; swapped.hooks.memory = (token) => { if (token === 3 && !swapped.once) { swapped.once = true; return original(3); } return original(token); }; formatChatMessage(1, 2, bytes('hi'), '%s (to %s)> ', swapped.work, swapped.globals, swapped.hooks);
    assert.deepEqual(swapped.log[0], ['nameFor', 5]);
});

test('formatChatMessage 이름/본문: 빈 현재 이름은 ++Nameless++, 이름은 첫 NUL 앞까지, 본문의 ~0~6 슬롯은 제거되고 빈 본문은 아무것도 쓰지 않는다', () => {
    const empty = rig({ name: '' }); formatChatMessage(1, 2, bytes('a'), null, empty.work, empty.globals, empty.hooks); assert.equal(textAt(empty.manager, CHAT_MESSAGE_OFFSET), 'B:++Nameless++> F:a');
    const cut = rig({ name: 'Al\0ice' }); formatChatMessage(1, 2, bytes('a'), null, cut.work, cut.globals, cut.hooks); assert.equal(textAt(cut.manager, CHAT_MESSAGE_OFFSET), 'B:Al> F:a');
    const slots = rig(); formatChatMessage(1, 2, bytes('a~1b~6c~7d~~0e'), null, slots.work, slots.globals, slots.hooks); assert.equal(textAt(slots.manager, CHAT_MESSAGE_OFFSET), 'B:Bob> F:abc~7d~e');
    const blank = rig(); blank.manager[CHAT_MESSAGE_OFFSET] = 0x41; assert.equal(formatChatMessage(1, 2, bytes(''), null, blank.work, blank.globals, blank.hooks), 0); assert.deepEqual(blank.log, []); assert.equal(blank.manager[CHAT_MESSAGE_OFFSET], 0x41);
    assert.throws(() => formatChatMessage(1, 2, Uint8Array.of(0x41), null, blank.work, blank.globals, blank.hooks), { name: 'RangeError', message: /NUL 물리/ });
});

test('formatChatMessage 훅 대체와 오류: 이름/제거 훅은 대체할 수 있고(명시 null 은 Error), 작업 버퍼/물리가 부족하거나 필수 장치가 없으면 도달한 위치에서 거부한다', () => {
    const custom = rig(); const seen = []; custom.hooks.formatName = (destination, pattern, name, other) => { seen.push([pattern, name, other]); writeChatNameFormat(destination, '%s ', name, other); }; custom.hooks.stripSlots = (output) => { seen.push(['strip', textAt(output)]); };
    formatChatMessage(1, 2, bytes('a~1'), '%s> ', custom.work, custom.globals, custom.hooks); assert.deepEqual(seen, [['%s> ', 'Bob', ''], ['strip', 'B:Bob F:a~1']]); assert.equal(textAt(custom.manager, CHAT_MESSAGE_OFFSET), 'B:Bob F:a~1');
    for (const name of ['formatName', 'stripSlots']) { const r = rig(); r.hooks[name] = null; assert.throws(() => formatChatMessage(1, 2, bytes('a'), null, r.work, r.globals, r.hooks), { name: 'Error', message: /실제 장치 미연결/ }, name); }
    for (const name of ['memory', 'currentName', 'deliver']) { const r = rig(); delete r.hooks[name]; assert.throws(() => formatChatMessage(1, 2, bytes('a'), null, r.work, r.globals, r.hooks), /실제 장치 미연결/, name); }
    const small = rig(); assert.throws(() => formatChatMessage(1, 2, bytes('a'), null, new Uint8Array(255), small.globals, small.hooks), { name: 'RangeError', message: /256BYTE/ });
    const thin = rig(); thin.hooks.memory = (token) => (token === 1 ? new Uint8Array(0x298) : new Uint8Array(16)); assert.throws(() => formatChatMessage(1, 2, bytes('a'), null, thin.work, thin.globals, thin.hooks), { name: 'RangeError', message: /manager 물리 부족/ });
    const lacking = rig({ count: 2, direct: 1, array: [7, 5] }); delete lacking.hooks.nameForSubscriber; assert.throws(() => formatChatMessage(1, 2, bytes('a'), '%s> ', lacking.work, lacking.globals, lacking.hooks), /실제 장치 미연결/);
    // 대상 조회는 직접 대상일 때만 필요하다.
    const noDirect = rig(); delete noDirect.hooks.nameForSubscriber; formatChatMessage(1, 2, bytes('a'), null, noDirect.work, noDirect.globals, noDirect.hooks); assert.equal(noDirect.log.length, 1);
    // 출력 한계를 넘는 본문은 출력 물리 부족으로 거부한다(2048-1 이 한도).
    const huge = rig(); assert.throws(() => formatChatMessage(1, 2, bytes('x'.repeat(2048)), null, huge.work, huge.globals, huge.hooks), { name: 'RangeError', message: /복사의 실제 물리 부족/ });
});

test('writeChatNameFormat: 세 고정 서식만 NUL 포함으로 쓰고 이름 안의 %s 는 해석하지 않으며 이름/상대/서식은 첫 NUL 앞만 쓰고 그 밖의 서식은 Error 다', () => {
    const out = new Uint8Array(64).fill(0x55);
    writeChatNameFormat(out, '%s> ', 'A%sB', 'ignored'); assert.equal(textAt(out), 'A%sB> '); assert.equal(out[6], 0); assert.equal(out[7], 0x55);
    writeChatNameFormat(out, '%s ', 'N', ''); assert.equal(textAt(out), 'N '); writeChatNameFormat(out, '%s (to %s)> ', 'N', 'T%s'); assert.equal(textAt(out), 'N (to T%s)> ');
    writeChatNameFormat(out, '%s> \0junk', 'N\0x', 'o'); assert.equal(textAt(out), 'N> '); writeChatNameFormat(out, '%s (to %s)> ', 'N', 'Zed\0x'); assert.equal(textAt(out), 'N (to Zed)> ');
    for (const pattern of ['%s>', '%s', '', '%d> ', '%s (to %s)>']) assert.throws(() => writeChatNameFormat(out, pattern, 'n', 'o'), { name: 'Error', message: /미지원 원문 서식/ }, pattern);
    assert.throws(() => writeChatNameFormat(new Uint8Array(3), '%s> ', 'Bob', ''), { name: 'RangeError', message: /복사의 실제 물리 부족/ }); assert.doesNotThrow(() => writeChatNameFormat(new Uint8Array(5), '%s ', 'Bob', '')); assert.throws(() => writeChatNameFormat(new Uint8Array(4), '%s ', 'Bob', ''), RangeError);
});

test('formatChatInput: 선행 ! 는 이름+공백 서식과 ! 뒤 본문, 직접 대상(manager +12)은 to 서식, 그 밖은 일반 서식이며 같은 manager 를 대상/수신자로 쓴다', () => {
    const plain = rig({ count: 1 }); formatChatInput(1, bytes('hi'), plain.work, plain.globals, plain.hooks); assert.equal(textAt(plain.manager, CHAT_MESSAGE_OFFSET), 'B:Bob> F:hi');
    const bang = rig(); formatChatInput(1, bytes('!hi'), bang.work, bang.globals, bang.hooks); assert.equal(textAt(bang.manager, CHAT_MESSAGE_OFFSET), 'B:Bob F:hi');
    // manager 가 수신자 객체도 겸하므로 count=+0, 배열 토큰=+8, 직접 표시=+12 가 같은 물리다.
    const direct = rig({ managerDirect: 1 }); putDword(direct.manager, 0, 1); putDword(direct.manager, 8, 3); formatChatInput(1, bytes('hi'), direct.work, direct.globals, direct.hooks);
    assert.equal(textAt(direct.manager, CHAT_MESSAGE_OFFSET), '~[Iicon.D38]B:Bob (to )> hi'); assert.deepEqual(direct.log.at(-1).slice(0, 3), ['deliver', 1, 1]);
    assert.throws(() => formatChatInput(1, new Uint8Array(0), plain.work, plain.globals, plain.hooks), { name: 'RangeError', message: /첫 입력 BYTE/ });
    const emptyBang = rig(); formatChatInput(1, bytes('!'), emptyBang.work, emptyBang.globals, emptyBang.hooks); assert.deepEqual(emptyBang.log, []);
});

test('sayChatMessage: 현재 manager 0 이면 무시, 빈 입력은 RangeError, 앞 공백은 무시, 빈 본문은 입력줄 숨김(0), 그 밖은 wrapper 로 전달하며 항상 0 을 돌려준다', () => {
    const off = rig(); off.state.current = 0; assert.equal(sayChatMessage(bytes('hi'), off.work, off.globals, off.hooks), 0); assert.deepEqual(off.log, []); assert.equal(sayChatMessage(new Uint8Array(0), off.work, off.globals, off.hooks), 0);
    const r = rig(); assert.throws(() => sayChatMessage(new Uint8Array(0), r.work, r.globals, r.hooks), { name: 'RangeError', message: /첫 입력 BYTE/ });
    assert.equal(sayChatMessage(bytes(' hi'), r.work, r.globals, r.hooks), 0); assert.deepEqual(r.log, []);
    assert.equal(sayChatMessage(bytes(''), r.work, r.globals, r.hooks), 0); assert.deepEqual(r.log, [['visible', 0]]); r.log.length = 0;
    assert.equal(sayChatMessage(bytes('hi'), r.work, r.globals, r.hooks), 0); assert.deepEqual(r.log, [['deliver', 1, 1, 'B:Bob> F:hi', 12]]);
    const noVisible = rig(); delete noVisible.hooks.inputVisible; assert.throws(() => sayChatMessage(bytes(''), noVisible.work, noVisible.globals, noVisible.hooks), /실제 장치 미연결/);
});

test('stripChatFormatSlots 기본 훅: 본문 슬롯 제거는 기존 raw-chat-format 과 같은 구현이다', () => {
    const a = bytes('x~3y~0~1z'); const b = Uint8Array.from(a); stripChatFormatSlots(a); const r = rig(); formatChatMessage(1, 2, b, null, r.work, r.globals, r.hooks); assert.equal(textAt(r.manager, CHAT_MESSAGE_OFFSET), `B:Bob> F:${textAt(a)}`);
});

test('경계: 대상 이름 버퍼 상수 256, DWORD 읽기의 끝 경계와 상위 바이트(4바이트 모두), 직접 대상의 첫 비자기 탐색의 count 경계/시작 위치', () => {
    assert.equal(CHAT_MESSAGE_NAME_BYTES, 256); const nul = String.fromCharCode(0);
    // 수신자 물리가 DWORD(+12)를 읽기에 정확히 16바이트면 되고 15바이트면 RangeError 다.
    const exact = rig(); const original = exact.hooks.memory; exact.hooks.memory = (token) => (token === 2 ? exact.recipients.subarray(0, 16) : original(token)); formatChatMessage(1, 2, bytes('a'), null, exact.work, exact.globals, exact.hooks);
    const thin = rig(); const raw = thin.hooks.memory; thin.hooks.memory = (token) => (token === 2 ? thin.recipients.subarray(0, 15) : raw(token)); assert.throws(() => formatChatMessage(1, 2, bytes('a'), null, thin.work, thin.globals, thin.hooks), { name: 'RangeError', message: /DWORD 물리 범위/ });
    // 큰 토큰/플래그의 네 바이트가 모두 쓰인다(배열 토큰 0x04030201, manager flags 0x108 은 8 비트만 켜짐).
    const big = rig({ count: 2, direct: 1, array: [7, 5], flags: 0x108 }); const bigToken = 0x04030201; putDword(big.recipients, 8, bigToken); const base = big.hooks.memory; big.hooks.memory = (token) => (token === bigToken ? big.list : base(token));
    formatChatMessage(1, 2, bytes('a'), '%s (to %s)> ', big.work, big.globals, big.hooks); assert.equal(textAt(big.manager, CHAT_MESSAGE_OFFSET), '~[Iicon.D36]B:Bob (to Zed)> a'); assert.deepEqual(big.log[0], ['nameFor', 5]);
    // count 가 정확히 0 이면 대상이 없고(array[0] 이 자기가 아니어도), count 1 이면 array[0] 을 검사한다.
    const zero = rig({ count: 0, direct: 1, array: [5] }); formatChatMessage(1, 2, bytes('a'), '%s> ', zero.work, zero.globals, zero.hooks); assert.equal(zero.log.some((entry) => entry[0] === 'nameFor'), false);
    const one = rig({ count: 1, direct: 1, array: [5] }); formatChatMessage(1, 2, bytes('a'), '%s> ', one.work, one.globals, one.hooks); assert.deepEqual(one.log[0], ['nameFor', 5]);
    // 첫 비자기가 첫 칸이면 시작 위치 0 에서 바로 찾고, 마지막 칸 다음(count 와 같은 위치)은 보지 않는다.
    const first = rig({ count: 2, direct: 1, array: [5, 7] }); formatChatMessage(1, 2, bytes('a'), '%s> ', first.work, first.globals, first.hooks); assert.deepEqual(first.log[0], ['nameFor', 5]);
    const edge = rig({ count: 2, direct: 1, array: [7, 7, 11] }); formatChatMessage(1, 2, bytes('a'), '%s> ', edge.work, edge.globals, edge.hooks); assert.equal(edge.log.some((entry) => entry[0] === 'nameFor'), false);
    const third = rig({ count: 3, direct: 1, array: [7, 7, 11] }); formatChatMessage(1, 2, bytes('a'), '%s> ', third.work, third.globals, third.hooks); assert.deepEqual(third.log[0], ['nameFor', 11]);
    // 이름 서식 경계: 이름이 NUL 로 시작하면 빈 이름이다.
    const out = new Uint8Array(32); writeChatNameFormat(out, '%s> ', nul + 'abc', ''); assert.equal(textAt(out), '> ');
});

test('경계: 출력 지우기(첫 BYTE)·아이콘 비트 이웃값·이름 첫 NUL·한 글자 이름·say 의 첫 BYTE 판정', () => {
    const nul = String.fromCharCode(0);
    const stale = rig(); putText(stale.manager, CHAT_MESSAGE_OFFSET, 'STALE-OUTPUT'); formatChatMessage(1, 2, bytes('a'), null, stale.work, stale.globals, stale.hooks); assert.equal(textAt(stale.manager, CHAT_MESSAGE_OFFSET), 'B:Bob> F:a');
    // 각 아이콘 비트의 이웃값(최하위 비트만 켜진 flags)은 아이콘을 붙이지 않는다.
    for (const flags of [1, 0x100]) { const r = rig({ flags }); formatChatMessage(1, 2, bytes('a'), null, r.work, r.globals, r.hooks); assert.equal(textAt(r.manager, CHAT_MESSAGE_OFFSET), 'B:Bob> F:a', String(flags)); }
    const lead = rig({ name: nul + 'Bob' }); formatChatMessage(1, 2, bytes('a'), null, lead.work, lead.globals, lead.hooks); assert.equal(textAt(lead.manager, CHAT_MESSAGE_OFFSET), 'B:++Nameless++> F:a');
    const one = rig({ name: 'A' }); formatChatMessage(1, 2, bytes('a'), null, one.work, one.globals, one.hooks); assert.equal(textAt(one.manager, CHAT_MESSAGE_OFFSET), 'B:A> F:a');
    // say: 첫 BYTE 가 0 이고 둘째가 0 이 아니어도 입력줄 숨김이다(둘째 BYTE 를 보지 않는다).
    const say = rig(); assert.equal(sayChatMessage(Uint8Array.of(0, 0x58, 0), say.work, say.globals, say.hooks), 0); assert.deepEqual(say.log, [['visible', 0]]);
});
