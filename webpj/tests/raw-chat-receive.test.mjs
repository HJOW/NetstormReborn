// Chat 수신 raw-chat-receive(receiveChatPacket·dispatchChatPacket)의 무시/뉴스/필터/관계/소리/표시 순서를 기준선 소스 계약과 손 계산 값으로 검증한다.
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 실제 게임·브라우저 실행이 아니다. (2026-11-14 WB04-E446)
import test from 'node:test';
import assert from 'node:assert/strict';
import { dispatchChatPacket, receiveChatPacket } from '../www/ui/raw-chat-receive.js';

// 패킷을 만든다: [0]=버전, [5]=형식 비트, [6..9]=발신자, [10..11]=이름 길이(NUL 포함), [12..13]=본문 길이(NUL 자리 포함), [14]=수신자 수, 이름, 본문+NUL(길이 칸)이다.
function packet({ version = 3, flags = 1, sender = 5, name = 'Al', body = 'hi', tail = 0, nameLength = null, bodyLength = null } = {}) {
    const nameBytes = [...Array.from(name, (char) => char.charCodeAt(0)), 0]; const bodyBytes = [...Array.from(body, (char) => char.charCodeAt(0)), ...Array(tail).fill(0x7a)]; const length = bodyLength ?? bodyBytes.length + 1;
    const data = new Uint8Array(15 + nameBytes.length + bodyBytes.length + 1); data[0] = version; data[1] = 0x12; data[5] = flags; data[14] = 1;
    for (let index = 0; index < 4; index += 1) data[6 + index] = Math.floor(sender / 2 ** (8 * index)) & 0xff;
    const size = nameLength ?? nameBytes.length; data[10] = size & 0xff; data[11] = (size >> 8) & 0xff; data[12] = length & 0xff; data[13] = (length >> 8) & 0xff;
    data.set(nameBytes, 15); data.set(bodyBytes, 15 + nameBytes.length); data[data.length - 1] = 0x21; return data; // 마지막 칸은 NUL 이 아닌 값으로 두어 수신이 NUL 을 쓰는지 본다.
}

// 기록 틀이다. 전역 기본값은 전투 0, 자기 subscriber 7, 메타맵 0, 글자색 0x33, 알림 토큰 0x77, Chat 0x99 다.
function rig({ globals = {}, options = {}, ignored = false, owner = 0 } = {}) {
    const log = []; const table = { 0x548740: 0x99, 0x594fbc: 0, 0x540c74: 7, 0x594fc8: 0, 0x5b5dd8: 0x33, 0x5ca92c: 0x77, 0x548748: 100, 0x548710: 0, 0x540c70: 1, ...globals };
    const hooks = {
        read: (address) => { log.push(['read', address]); const value = table[address]; return typeof value === 'function' ? value() : (value ?? 0); }, ignored: (subscriber) => { log.push(['ignored', subscriber]); return ignored; },
        config: (key) => { log.push(['config', key]); return options[key] ?? 0; }, owner: (subscriber) => { log.push(['owner', subscriber]); return owner; },
        wake: (token) => log.push(['wake', token]), sound: (name) => log.push(['sound', name]), append: (chat, text, context) => log.push(['append', chat, text, Array.from(context)]), show: () => log.push(['show']),
        log: (level, value) => log.push(['log', level, value]), setNewsTime: (stamp) => log.push(['news-time', stamp]), news: (value, a, b) => log.push(['news', value, a, b]),
    };
    return { hooks, log, table };
}
const effects = (log) => log.filter((entry) => !['read', 'config', 'owner', 'ignored'].includes(entry[0]));
const CONTEXT = [0, 5, 0x33, 0, 0, 0, 1, 1];

test('경고 로그: 버전이 3 을 넘으면 새 형식, 형식 하위 4비트가 1 이 아니면 비ASCII 로 알리고 즉시 끝낸다', () => {
    const a = rig(); receiveChatPacket(0x99, packet({ version: 4 }), a.hooks); assert.deepEqual(effects(a.log), [['log', 29, 'Received new format chat packet.\n']]);
    const b = rig(); receiveChatPacket(0x99, packet({ flags: 2 }), b.hooks); assert.deepEqual(effects(b.log), [['log', 29, 'Received non-ASCII chat packet.\n']]);
    const c = rig(); receiveChatPacket(0x99, packet({ version: 3, flags: 0x81 }), c.hooks); assert.equal(effects(c.log).some((entry) => entry[0] === 'log'), false);
    const d = rig(); receiveChatPacket(0x99, packet({ flags: 0x12 }), d.hooks); assert.deepEqual(effects(d.log), [['log', 29, 'Received non-ASCII chat packet.\n']]);
});

test('본문 NUL: 먼저 본문 마지막 BYTE 에 NUL 을 쓰고(무시되는 메시지에도 남는다), 범위가 맞지 않으면 RangeError 다', () => {
    const data = packet({ body: 'hix', bodyLength: 3 }); const r = rig({ ignored: true }); receiveChatPacket(0x99, data, r.hooks); assert.equal(data[data.length - 2], 0); assert.equal(data[data.length - 1], 0x21); assert.deepEqual(effects(r.log), []);
    assert.throws(() => receiveChatPacket(0x99, packet({ bodyLength: 0xff }), rig().hooks), { name: 'RangeError', message: /마지막 본문 BYTE/ });
    assert.throws(() => receiveChatPacket(0x99, packet({ nameLength: 0xfff1, bodyLength: 0 }), rig().hooks), RangeError); assert.throws(() => receiveChatPacket(0x99, packet({ nameLength: 0xfff0, bodyLength: 0 }), rig().hooks), { name: 'RangeError', message: /signed 주소 범위/ });
    assert.throws(() => receiveChatPacket(0x99, packet({ nameLength: 0xffff - 14, bodyLength: 0 }), rig().hooks), /마지막 본문 BYTE/);
});

test('비전투(594fbc=0): 알림을 깨우고, 자기가 아닌 발신자이고 메타맵 0 이면 소리를 내고, 현재 글자색 문맥으로 본문을 추가한 뒤 Chat 을 표시한다', () => {
    const r = rig(); receiveChatPacket(0x99, packet({ sender: 5, body: 'hello' }), r.hooks);
    assert.deepEqual(effects(r.log), [['wake', 0x77], ['sound', 'newChatMessage.wav'], ['append', 0x99, 'hello', CONTEXT], ['show']]);
    const self = rig(); receiveChatPacket(0x99, packet({ sender: 7 }), self.hooks); assert.equal(effects(self.log).some((entry) => entry[0] === 'sound'), false);
    const meta = rig({ globals: { 0x594fc8: 1 } }); receiveChatPacket(0x99, packet({ sender: 5 }), meta.hooks); assert.equal(effects(meta.log).some((entry) => entry[0] === 'sound'), false);
    const ignoring = rig({ ignored: true }); receiveChatPacket(0x99, packet({ sender: 5 }), ignoring.hooks); assert.deepEqual(effects(ignoring.log), []); assert.deepEqual(ignoring.log.find((entry) => entry[0] === 'ignored'), ['ignored', 5]);
});

test('시스템 뉴스: 상위 비트와 발신자 0 이면 뉴스(400,30)로 보내고, 이름이 있고 첫 BYTE 가 1 이면 시각이 현재보다 클 때만 갱신 후 보낸다', () => {
    const plain = rig(); receiveChatPacket(0x99, packet({ flags: 0x81, sender: 0, name: '', body: 'hello' }), plain.hooks); assert.deepEqual(effects(plain.log), [['news', 'hello', 400, 30]]);
    const stamped = (stamp) => { const data = packet({ flags: 0x81, sender: 0, name: 'Z\x01\x00\x00\x00\x00\x00', body: 'news' }); data[15] = 0x5a; data[16] = 1; data[17] = 0; for (let index = 0; index < 4; index += 1) data[19 + index] = Math.floor(stamp / 2 ** (8 * index)) & 0xff; return data; };
    // 이름 길이가 1 을 넘고 첫 BYTE 가 1 이어야 시각을 본다(여기서는 첫 BYTE 가 'Z' 라서 시각 검사가 없다).
    const noStamp = rig(); receiveChatPacket(0x99, stamped(50), noStamp.hooks); assert.deepEqual(effects(noStamp.log), [['news', 'news', 400, 30]]);
    const first = (stamp) => { const data = stamped(stamp); data[15] = 1; return data; };
    const older = rig(); receiveChatPacket(0x99, first(100), older.hooks); assert.deepEqual(effects(older.log), []); const newer = rig(); receiveChatPacket(0x99, first(101), newer.hooks); assert.deepEqual(effects(newer.log), [['news-time', 101], ['news', 'news', 400, 30]]);
    const small = rig(); receiveChatPacket(0x99, first(5), small.hooks); assert.deepEqual(effects(small.log), []);
    // 발신자가 0 이 아니면 뉴스가 아니다.
    const other = rig(); receiveChatPacket(0x99, packet({ flags: 0x81, sender: 3, body: 'x' }), other.hooks); assert.equal(effects(other.log).some((entry) => entry[0] === 'news'), false);
});

test('전투(594fbc≠0) 필터: 열 개 옵션을 원문 순서로 모두 읽고 채널 마스크와 옵션이 맞아야 표시하며 본문은 앞 세 글자를 지우고 개행을 붙인다', () => {
    const keys = ['ShowBattle', 'ShowEveryone', 'ShowAllies', 'ShowObserver', 'ShowWisper', 'ShowMyAllies', 'ShowMyEmenys', 'ShowMyNeutrals', 'ShowChatWindow', 'PlaySoundMessage'];
    const hidden = rig({ globals: { 0x594fbc: 1, 0x548710: 0 } }); receiveChatPacket(0x99, packet({ body: '123text' }), hidden.hooks); assert.deepEqual(hidden.log.filter((entry) => entry[0] === 'config').map((entry) => entry[1]), keys); assert.deepEqual(effects(hidden.log), []);
    const masks = [[8, 'ShowBattle'], [32, 'ShowEveryone'], [64, 'ShowAllies'], [128, 'ShowObserver'], [256, 'ShowWisper']];
    for (const [mask, key] of masks) {
        const on = rig({ globals: { 0x594fbc: 1, 0x548710: mask }, options: { [key]: 1, ShowMyEmenys: 1, ShowChatWindow: 1, PlaySoundMessage: 1 } }); receiveChatPacket(0x99, packet({ body: '123text' }), on.hooks);
        assert.deepEqual(effects(on.log), [['wake', 0x77], ['sound', 'newChatMessage.wav'], ['append', 0x99, 'text\n', CONTEXT], ['show']], key);
        const off = rig({ globals: { 0x594fbc: 1, 0x548710: mask } }); receiveChatPacket(0x99, packet({ body: '123text' }), off.hooks); assert.deepEqual(effects(off.log), [], key);
    }
    const wrong = rig({ globals: { 0x594fbc: 1, 0x548710: 8 }, options: { ShowEveryone: 1 } }); receiveChatPacket(0x99, packet({ body: '123text' }), wrong.hooks); assert.deepEqual(effects(wrong.log), []);
    assert.throws(() => receiveChatPacket(0x99, packet({ body: 'ab' }), rig({ globals: { 0x594fbc: 1 } }).hooks), { name: 'RangeError', message: /세자리 접두/ });
    assert.doesNotThrow(() => receiveChatPacket(0x99, packet({ body: 'abc' }), rig({ globals: { 0x594fbc: 1 } }).hooks));
});

test('전투 관계 필터: 소유자 관계표(0x595200+(owner+현재*9)*4)가 0 이면 ShowMyEmenys, 비영이면 ShowMyAllies, 소유자 중립표(0x52f81e+owner)가 비영이면 ShowMyNeutrals 가 필요하다', () => {
    const base = { 0x594fbc: 1, 0x548710: 8 }; const relation = 0x595200 + (2 + 1 * 9) * 4; const neutral = 0x52f81e + 2;
    const run = (globals, options) => { const r = rig({ globals: { ...base, ...globals }, options: { ShowBattle: 1, ShowChatWindow: 1, ...options }, owner: 2 }); receiveChatPacket(0x99, packet({ body: '123x' }), r.hooks); return r; };
    assert.equal(effects(run({}, { ShowMyEmenys: 1 }).log).some((entry) => entry[0] === 'append'), true); assert.equal(effects(run({}, { ShowMyEmenys: 0 }).log).some((entry) => entry[0] === 'append'), false);
    assert.equal(effects(run({ [relation]: 1 }, { ShowMyAllies: 1 }).log).some((entry) => entry[0] === 'append'), true); assert.equal(effects(run({ [relation]: 1 }, { ShowMyAllies: 0, ShowMyEmenys: 1 }).log).some((entry) => entry[0] === 'append'), false);
    assert.equal(effects(run({ [neutral]: 1 }, { ShowMyEmenys: 1, ShowMyNeutrals: 0 }).log).some((entry) => entry[0] === 'append'), false); assert.equal(effects(run({ [neutral]: 1 }, { ShowMyEmenys: 1, ShowMyNeutrals: 1 }).log).some((entry) => entry[0] === 'append'), true);
    // 소유자 조회는 채널 필터를 통과한 경우에만 부른다.
    const skipped = rig({ globals: { ...base, 0x548710: 0 }, options: {} }); receiveChatPacket(0x99, packet({ body: '123x' }), skipped.hooks); assert.equal(skipped.log.some((entry) => entry[0] === 'owner'), false);
    assert.deepEqual(run({}, { ShowMyEmenys: 1 }).log.find((entry) => entry[0] === 'owner'), ['owner', 5]);
});

test('전투 표시 효과: 소리는 PlaySoundMessage 가 켜진 상대 메시지(메타맵 0)에서만, Chat 표시는 ShowChatWindow 가 켜졌을 때만이며 알림은 항상 깨운다', () => {
    const run = (options, sender = 5, meta = 0) => { const r = rig({ globals: { 0x594fbc: 1, 0x548710: 8, 0x594fc8: meta }, options: { ShowBattle: 1, ShowMyEmenys: 1, ...options } }); receiveChatPacket(0x99, packet({ sender, body: '123x' }), r.hooks); return effects(r.log).map((entry) => entry[0]); };
    assert.deepEqual(run({}), ['wake', 'append']); assert.deepEqual(run({ PlaySoundMessage: 1 }), ['wake', 'sound', 'append']); assert.deepEqual(run({ PlaySoundMessage: 1, ShowChatWindow: 1 }), ['wake', 'sound', 'append', 'show']);
    assert.deepEqual(run({ PlaySoundMessage: 1 }, 7), ['wake', 'append']); assert.deepEqual(run({ PlaySoundMessage: 1 }, 5, 1), ['wake', 'append']);
});

test('dispatchChatPacket: 현재 Chat(548740)이 0 이면 다른 장치를 건드리지 않고, 비영이면 같은 수신 부모를 현재 Chat 으로 실행한다', () => {
    const off = rig({ globals: { 0x548740: 0 } }); dispatchChatPacket(packet({}), off.hooks); assert.deepEqual(off.log, [['read', 0x548740]]);
    const on = rig({ globals: { 0x548740: 0x1234 } }); dispatchChatPacket(packet({ body: 'z' }), on.hooks); assert.deepEqual(effects(on.log).find((entry) => entry[0] === 'append'), ['append', 0x1234, 'z', CONTEXT]);
    assert.throws(() => dispatchChatPacket(packet({}), {}), { name: 'Error', message: /실제 장치 미연결/ });
});

test('필수 장치: 도달한 위치의 장치가 없을 때만 Error 이고 미도달 장치는 요구하지 않는다', () => {
    const base = packet({ sender: 5, body: 'hi' });
    for (const name of ['read', 'ignored', 'wake', 'sound', 'append', 'show']) { const r = rig(); delete r.hooks[name]; assert.throws(() => receiveChatPacket(0x99, Uint8Array.from(base), r.hooks), { name: 'Error', message: /실제 장치 미연결/ }, name); }
    const noLog = rig(); delete noLog.hooks.log; delete noLog.hooks.config; delete noLog.hooks.owner; delete noLog.hooks.news; delete noLog.hooks.setNewsTime; receiveChatPacket(0x99, Uint8Array.from(base), noLog.hooks);
    const lacking = rig({ globals: { 0x594fbc: 1 } }); delete lacking.hooks.config; assert.throws(() => receiveChatPacket(0x99, Uint8Array.from(base), lacking.hooks), /실제 장치 미연결/);
    const noOwner = rig({ globals: { 0x594fbc: 1, 0x548710: 8 }, options: { ShowBattle: 1 } }); delete noOwner.hooks.owner; assert.throws(() => receiveChatPacket(0x99, packet({ body: '123x' }), noOwner.hooks), /실제 장치 미연결/);
    const noNews = rig(); delete noNews.hooks.news; assert.throws(() => receiveChatPacket(0x99, packet({ flags: 0x81, sender: 0, body: 'n' }), noNews.hooks), /실제 장치 미연결/);
});

test('경계: 짧은 패킷의 읽기 범위, 비뉴스 flags 와 발신자 0, 이름 길이 1 의 시각 검사 제외, 시각 검사의 첫 BYTE 폭', () => {
    for (const length of [5, 11, 13, 14]) assert.throws(() => receiveChatPacket(0x99, new Uint8Array(length).map((value, index) => (index === 5 ? 1 : value)), rig().hooks), RangeError, String(length));
    // flags 1(뉴스 비트 없음)과 발신자 0 은 뉴스가 아니다: 무시 조회를 거쳐 일반 수신으로 간다.
    const normal = rig(); receiveChatPacket(0x99, packet({ flags: 1, sender: 0, body: 'zero' }), normal.hooks); assert.equal(normal.log.some((entry) => entry[0] === 'ignored'), true); assert.equal(effects(normal.log).some((entry) => entry[0] === 'news'), false);
    // 이름 길이가 정확히 1 이면 첫 BYTE 가 1 이어도 시각 검사를 하지 않고 뉴스만 보낸다.
    const one = rig(); receiveChatPacket(0x99, packet({ flags: 0x81, sender: 0, name: String.fromCharCode(1), nameLength: 1, body: 'x' }), one.hooks); assert.deepEqual(effects(one.log), [['news', '', 400, 30]]);
    // 첫 BYTE 만 1 인지 본다: +16 이 0 이 아니어도(이름 01 5a) 시각 검사를 한다.
    const stamped = packet({ flags: 0x81, sender: 0, name: String.fromCharCode(1, 0x5a, 0, 0, 150, 0, 0, 0), body: 'n' });
    const fresh = rig(); receiveChatPacket(0x99, stamped, fresh.hooks); assert.deepEqual(effects(fresh.log).map((entry) => entry[0]), ['news-time', 'news']);
    // 이름 길이가 정확히 2(첫 BYTE 1 과 NUL)이면 시각 검사를 한다(1 을 넘는 경계): 시각은 +19 부터 네 BYTE 다.
    const two = rig(); receiveChatPacket(0x99, packet({ flags: 0x81, sender: 0, name: String.fromCharCode(1), body: 'ABCDE' }), two.hooks); assert.deepEqual(effects(two.log), [['news-time', 0x454443], ['news', 'ABCDE', 400, 30]]);
});

test('경계: 전투 채널 마스크의 정확한 비트와 이웃 값, 숨김 메시지의 부작용 없음, 관계 재조회(같은 주소의 둘째 읽기)', () => {
    const opts = { ShowBattle: 1, ShowEveryone: 1, ShowAllies: 1, ShowObserver: 1, ShowWisper: 1, ShowMyEmenys: 1, ShowChatWindow: 1, PlaySoundMessage: 1 };
    for (const mask of [0, 1, 2, 4, 16, 512, 1024]) { const r = rig({ globals: { 0x594fbc: 1, 0x548710: mask }, options: opts }); receiveChatPacket(0x99, packet({ body: '123x' }), r.hooks); assert.deepEqual(effects(r.log), [], String(mask)); }
    for (const mask of [8, 32, 64, 128, 256, 8 | 1, 32 | 1, 64 | 1, 128 | 1, 256 | 1]) { const r = rig({ globals: { 0x594fbc: 1, 0x548710: mask }, options: opts }); receiveChatPacket(0x99, packet({ body: '123x' }), r.hooks); assert.equal(effects(r.log).some((entry) => entry[0] === 'append'), true, String(mask)); }
    // 마스크 비트가 켜져도 해당 옵션이 꺼져 있으면(다른 옵션이 켜져 있어도) 숨긴다.
    for (const [mask, key] of [[8, 'ShowBattle'], [32, 'ShowEveryone'], [64, 'ShowAllies'], [128, 'ShowObserver'], [256, 'ShowWisper']]) { const off = { ...opts, [key]: 0 }; const r = rig({ globals: { 0x594fbc: 1, 0x548710: mask }, options: off }); receiveChatPacket(0x99, packet({ body: '123x' }), r.hooks); assert.deepEqual(effects(r.log), [], key); }
    // 관계표를 두 번 읽는 원문의 순서: 첫 읽기는 비영(동맹 분기), 둘째 읽기가 0 이면 ShowMyEmenys 로 정한다.
    const relation = 0x595200 + (0 + 1 * 9) * 4; const flip = (second) => { let calls = 0; return () => { calls += 1; return calls === 1 ? 1 : second; }; };
    const shown = rig({ globals: { 0x594fbc: 1, 0x548710: 8, [relation]: flip(0) }, options: { ShowBattle: 1, ShowMyAllies: 1, ShowMyEmenys: 1 } }); receiveChatPacket(0x99, packet({ body: '123x' }), shown.hooks); assert.equal(effects(shown.log).some((entry) => entry[0] === 'append'), true);
    const hidden = rig({ globals: { 0x594fbc: 1, 0x548710: 8, [relation]: flip(0) }, options: { ShowBattle: 1, ShowMyAllies: 1, ShowMyEmenys: 0 } }); receiveChatPacket(0x99, packet({ body: '123x' }), hidden.hooks); assert.deepEqual(effects(hidden.log), []);
    const still = rig({ globals: { 0x594fbc: 1, 0x548710: 8, [relation]: flip(5) }, options: { ShowBattle: 1, ShowMyAllies: 1, ShowMyEmenys: 0 } }); receiveChatPacket(0x99, packet({ body: '123x' }), still.hooks); assert.equal(effects(still.log).some((entry) => entry[0] === 'append'), true);
});
