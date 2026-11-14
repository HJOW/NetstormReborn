// ChatWindow(생성·명령 등록·접두 편집·사건 처리·로그 추가·도우미 메뉴)를 기준선 소스 계약과 손 계산 값으로 검증한다.
// 실제 TextEntryGump/TextEntryBuffer/TextEntryStorage(E439 지원 틀)와 실제 GumpCreate 위에서 동작하며 나머지 큰 하위(배경/스크롤/메뉴/Chatmanager)는 기록 훅이다.
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 실제 게임·브라우저 실행이 아니다. (2026-11-14 WB04-E447)
import test from 'node:test';
import assert from 'node:assert/strict';
import { GumpCreate, GumpCreateNode, GumpCreateState } from '../www/ui/gump-create.js';
import { ConsoleGumpNode } from '../www/ui/text-entry-gump.js';
import { ScrollingTextGumpNode } from '../www/ui/text-gump.js';
import { HudChatNode } from '../www/ui/hud-chat-geometry.js';
import { ChatWindow, ChatWindowState } from '../www/ui/chat-window.js';
import { bytesOf, cstr, rig as entryRig } from './text-entry-gump-support.mjs';

// 현재 전역 표(주소 → 값)의 기본값이다.
const C = (value) => 0x02000000 | value; // 명령 사건 값은 명령 비트(0x02000000)를 포함한다.
const GLOBALS = { 0x540424: 0x80, 0x5acd24: 0x77, 0x5b5dc0: 0x10, 0x5b5e60: 0x22, 0x5b5dd4: 0x33, 0x5b5dd8: 0x44, 0x541348: 3, 0x542b24: C(0x5001), 0x5427a8: C(0x5002), 0x5429f0: C(0x5003), 0x5427d8: C(0x5004), 0x542794: C(0x5005), 0x542af8: C(0x5006), 0x54298c: C(0x5007),
    0x542914: C(0x5008), 0x548714: 0x5009, 0x5428ac: C(0x500a), 0x542b08: C(0x500b), 0x542b0c: C(0x500c), 0x594fc8: 0, 0x594fbc: 0, 0x5cae94: 11, 0x5cae98: 12, 0x563e38: 13 };

// 기록 틀이다. 노드는 입력 틀(entry)과 같은 표를 공유하고, ChatWindow 자식은 600번대 토큰으로 만든다. 프레임 표: TYPE 0 은 높이 [7,8,16,17], TYPE 1 은 [10], TYPE 2 는 없음이다.
function make({ globals = {}, config = {}, frames = [[7, 8, 16, 17], [10], []] } = {}) {
    const entry = entryRig({ console: true }); const log = []; const menu = []; const nodes = entry.nodes; const factory = new GumpCreate(new GumpCreateState(), { node: (token) => nodes.get(token) ?? null });
    const table = { ...GLOBALS, ...globals }; const cfg = { ...config }; let next = 600; let menuToken = 800;
    const state = new ChatWindowState(); const root = new HudChatNode(); nodes.set(500, root); root.scale = 1; root.serial = 5;
    const hooks = {
        chat: (token) => (nodes.get(token) instanceof HudChatNode ? nodes.get(token) : null), base: (token) => nodes.get(token) ?? null,
        console: (token) => (nodes.get(token) instanceof ConsoleGumpNode ? nodes.get(token) : null), text: (token) => (nodes.get(token) instanceof ScrollingTextGumpNode ? nodes.get(token) : null),
        buffer: (token) => entry.storage.find(token), entry: { constructConsole: (...args) => { const result = entry.gump.constructConsole(...args); nodes.get(args[0]).flags = (nodes.get(args[0]).flags | 1) >>> 0; return result; }, setText: (...args) => entry.gump.setText(...args), appendText: (...args) => entry.gump.appendText(...args), copyText: (...args) => entry.gump.copyText(...args) }, geometry: { attach: (...args) => log.push(['geometry.attach', ...args]) },
        global: (address) => table[address] ?? 0,
        allocate: (size) => { if (hooks.failAllocate === size) { log.push(['allocate', size, 0]); return 0; } const token = next; next += 1;
            const node = size === 0x13c ? new HudChatNode() : size === 0x128 ? new ConsoleGumpNode() : size === 0xd8 ? new ScrollingTextGumpNode() : new GumpCreateNode(); node.serial = token; nodes.set(token, node); log.push(['allocate', size, token]); return token; },
        constructBase: (token, w, h) => { log.push(['constructBase', token, w, h]); factory.construct(token, 0, w, h, 1); nodes.get(token).scale = 1; },
        constructHelper: (token, label, color) => { log.push(['constructHelper', token, label, color]); factory.construct(token, 0, 0, 0, 1); nodes.get(token).scale = 1; }, fitHelper: (token) => log.push(['fitHelper', token]),
        title: (token, value) => log.push(['title', token, value]), help: (token, value) => log.push(['help', token, value]), translate: (key) => `<${key}>`, measure: (token, value) => { log.push(['measure', token, value]); return [8, 16]; },
        constructBackground: (token, w, h, flags) => { log.push(['constructBackground', token, w, h, flags]); factory.construct(token, 0, w, h, 1); },
        constructScrolling: (token, w, h, font, color) => { log.push(['constructScrolling', token, w, h, font, color]); factory.construct(token, 0, w, h, 1); nodes.get(token).scale = 1; nodes.get(token).textFlags = 0x13; },
        replaceBuffer: (token, buffer, owned) => { log.push(['replaceBuffer', token, buffer, owned]); if (nodes.get(token)) nodes.get(token).buffer = buffer; }, sharedBuffer: () => 900,
        setSlider: (token, value) => log.push(['setSlider', token, value]), setScrollable: (token, value) => log.push(['setScrollable', token, value]),
        attach: (token, x, y, parent, mode) => log.push(['attach', token, x, y, parent, mode]), push: (token) => log.push(['push', token]), destructBase: (token) => log.push(['destructBase', token]),
        registerCommand: (name, callback, flag) => log.push(['registerCommand', name, callback, flag]), bufferAppendCallback: (buffer, callback) => log.push(['bufferAppendCallback', buffer, callback]),
        frameCount: (type) => { log.push(['frameCount', type]); return frames[type].length; }, frameHeight: (type, frame) => frames[type][frame],
        requestScroll: (token, value) => log.push(['requestScroll', token, value]), relativeScroll: (token, value) => log.push(['relativeScroll', token, value]), requestBottom: (token) => log.push(['requestBottom', token]),
        atBottom: (token) => { log.push(['atBottom', token]); return Boolean(hooks.bottom); }, appendLog: (token, value) => log.push(['appendLog', token, value]),
        baseHandle: (...args) => { log.push(['baseHandle', ...args]); return 0x1234; }, bubble: (...args) => log.push(['bubble', ...args]),
        recipient: (name) => { log.push(['recipient', name]); return hooks.valid ? hooks.valid(name) : ['Ann', 'Bob', 'Bob Ann', 'Ann Bob', 'Bob Ann Cat', 'Ann Cat', ''].includes(name.trimEnd()); }, sendChat: (manager, value) => log.push(['sendChat', manager, value]),
        resolve: (record) => { const subscriber = record[4] + record[5] * 256 + record[6] * 65536 + record[7] * 16777216; log.push(['resolve', subscriber, record.length, record[52] + record[53] * 256, record[8]]); const name = hooks.names?.[subscriber]; if (hooks.rawName) { record.set(hooks.rawName, 8); return true; } if (!name) return false; for (let index = 0; index < name.length; index += 1) record[8 + index] = name.charCodeAt(index); record[8 + name.length] = 0; return true; },
        log: (level, format, value) => log.push(['log', level, format, value]), config: (key) => { menu.push(`config:${key}`); return cfg[key] ?? 0; },
        menuBegin: () => menu.push('begin'), menuFinalize: () => menu.push('finalize'), menuTitle: (value) => menu.push(`title:${value}`),
        menuItem: (label, event, payload) => menu.push(`item:${label}|${event}|${payload}`), menuChecked: (checked, label, event, payload) => menu.push(`checked:${checked}|${label}|${event}|${payload}`),
        menuTake: () => { menuToken += 1; return menuToken; }, menuSubmenu: (label, child) => menu.push(`sub:${label}|${child}`), menuOpen: (x, y, reference) => menu.push(`open:${x},${y},${reference}`), assertion: (line) => log.push(['assertion', line]),
    };
    const window = new ChatWindow(state, hooks); return { window, state, hooks, entry, nodes, log, menu, root, table, cfg, factory };
}
// 구성까지 끝낸 틱 틀이다(로그/메뉴 기록은 비운다). 구성한 root 토큰은 600 번 이후다.
function built(options) { const r = make(options); r.token = r.window.create(); r.log.length = 0; r.menu.length = 0; return r; }
const setInput = (r, value) => { r.entry.gump.setText(r.nodes.get(r.token).console, bytesOf(value)); };
const inputText = (r) => cstr(r.entry.storage.find(r.nodes.get(r.token).console === undefined ? 0 : r.nodes.get(r.nodes.get(r.token).console).buffer).data());

test('ChatWindowState: 새 상태의 current/등록/초기화는 0 이고 세 자동완성 목록은 비어 있다', () => {
    const state = new ChatWindowState(); assert.deepEqual([state.current, state.registered, state.completionReady, state.colors, state.backgrounds, state.icons], [0, 0, 0, [], [], []]);
});

test('생성자: chat/base 조회가 없으면 Error 다(나머지 장치는 도달할 때만 요구)', () => {
    assert.throws(() => new ChatWindow(new ChatWindowState(), { base: () => null }), { name: 'Error', message: /실제 하위 연결 누락/ }); assert.throws(() => new ChatWindow(new ChatWindowState(), { chat: () => null }), /연결 누락/);
    assert.throws(() => new ChatWindow(new ChatWindowState(), undefined), Error); const bare = new ChatWindow(new ChatWindowState(), { chat: () => null, base: () => null }); assert.throws(() => bare.attach(5, 0, 0, 0, 0), /실제 Attach 본체 없음/); assert.throws(() => bare.destruct(5), /공유 root 없음/);
});

test('create: clear/quit 명령은 처음 한 번만 등록하고 200×100 root 를 만들어 현재로 저장하며 로그 버퍼에 append 콜백을 설치한다', () => {
    const r = make(); const token = r.window.create(); assert.equal(token, 600); assert.equal(r.state.current, 600);
    assert.deepEqual(r.log.filter((entry) => ['registerCommand', 'bufferAppendCallback'].includes(entry[0])), [['registerCommand', 'clear', 0x42f680, 1], ['registerCommand', 'quit', 0x42e520, 1], ['bufferAppendCallback', 900, 0x42f590]]);
    assert.deepEqual(r.log.find((entry) => entry[0] === 'constructBase'), ['constructBase', 600, 200, 100]); r.log.length = 0; const second = r.window.create(); assert.equal(second !== token, true); assert.equal(r.log.some((entry) => entry[0] === 'registerCommand'), false); assert.equal(r.state.registered, 1);
    const fail = make(); fail.hooks.failAllocate = 0x13c; assert.throws(() => fail.window.create(), { name: 'Error', message: /원본 new 실패/ }); assert.equal(fail.state.current, 0); assert.equal(fail.state.registered, 1);
});

test('construct 결과: root 필드·자식 토큰·크기·Console 설정·플래그·접두·최초 완성 목록이 원문대로다', () => {
    const r = make(); const token = r.window.create(); const root = r.nodes.get(token);
    assert.deepEqual([root.vtable, root.command, root.helperVisible, root.capacity, root.leftPadding, root.baseWidth, root.baseHeight, root.width, root.height], [0x504a94, 0xffffff80, 1, 128, 0, 200, 100, 200, 100]);
    assert.deepEqual([root.helper, root.background, root.console, root.text], [601, 602, 603, 604]); assert.equal(root.flags & 0xe00, 0xe00); assert.equal(root.inputPrefix[0], 0);
    const console = r.nodes.get(root.console); assert.deepEqual([console.changeNotice, console.maxLength, console.multiline, console.flags & 1, console.autocomplete], [1, 80, 0, 0, 0x42e540]);
    const text = r.nodes.get(root.text); assert.deepEqual([text.callback, text.buffer, text.textFlags], [1, 900, 3]); assert.equal(cstr(r.entry.storage.find(console.buffer).data()), '');
    assert.equal(r.state.completionReady, 1); assert.equal(r.state.colors.length, 256); assert.equal(r.state.backgrounds.length, 256); assert.deepEqual([r.state.colors[0], r.state.colors[255], r.state.backgrounds[7]], ['~[C.0]', '~[C.255]', '~[B.7]']);
    assert.deepEqual(r.state.icons, ['~[I0.1]', '~[I0.2]', '~[I1.0]']); // 높이 8..16 만(7·17 제외), TYPE 수는 전역 541348=3 이다.
});

test('construct 호출 순서: 기본 생성 → helper → 제목/도움말 → 측정 → 배경 → Console → 로그 → 버퍼 교체 → 네 자식 부착 → Slider/스크롤', () => {
    const r = make(); r.window.create(); const names = r.log.map((entry) => entry[0] === 'allocate' ? `allocate:${entry[1].toString(16)}` : entry[0]);
    const wanted = ['registerCommand', 'registerCommand', 'allocate:13c', 'constructBase', 'allocate:170', 'constructHelper', 'fitHelper', 'title', 'help', 'measure', 'allocate:b8', 'constructBackground', 'allocate:128', 'allocate:d8', 'constructScrolling', 'replaceBuffer', 'attach', 'attach', 'attach', 'attach', 'setSlider', 'setScrollable', 'frameCount'];
    const picked = names.filter((name) => wanted.includes(name) || name === 'allocate:28'); const trimmed = picked.filter((name) => name !== 'allocate:28'); assert.deepEqual(trimmed.slice(0, wanted.length), wanted);
    assert.deepEqual(r.log.filter((entry) => ['constructHelper', 'title', 'help', 'measure', 'constructBackground', 'constructScrolling', 'replaceBuffer'].includes(entry[0])), [['constructHelper', 601, '>', 0x77], ['title', 601, '<Chat Helper>'], ['help', 601, 'chathelper'], ['measure', 600, 'M'],
        ['constructBackground', 602, 180, 84, 0x04000010], ['constructScrolling', 604, 180, 84, 5, 0x44], ['replaceBuffer', 604, 900, 0]]);
    assert.deepEqual(r.log.filter((entry) => entry[0] === 'attach').slice(0, 4).map((entry) => entry.slice(1)), [[602, 0, 0, 600, 0], [603, 0, 84, 600, 0], [601, 0, 84, 600, 0], [604, 0, 0, 602, 0]]);
});

test('construct 실패 경로: 자식 할당 0 은 하위 생성을 건너뛰고 이후 접근에서 거부되며, 도달한 장치가 없으면 Error 다', () => {
    for (const size of [0x128, 0xd8]) { const r = make(); r.hooks.failAllocate = size; assert.throws(() => r.window.create(), { name: 'Error' }, size.toString(16)); }
    // 배경/helper 할당 0 은 하위 생성만 건너뛰고(부착 훅은 0 토큰을 그대로 받는다) 구성은 계속된다.
    const noBackground = make(); noBackground.hooks.failAllocate = 0xb8; noBackground.window.create(); assert.equal(noBackground.nodes.get(600).background, 0); assert.equal(noBackground.log.some((entry) => entry[0] === 'constructBackground'), false);
    const noHelper = make(); noHelper.hooks.failAllocate = 0x170; noHelper.window.create(); assert.equal(noHelper.nodes.get(600).helper, 0); assert.equal(noHelper.log.some((entry) => entry[0] === 'constructHelper'), false);
    for (const name of ['constructBase', 'translate', 'measure', 'replaceBuffer', 'sharedBuffer', 'setSlider', 'global', 'allocate']) { const r = make(); delete r.hooks[name]; assert.throws(() => r.window.create(), { name: 'Error', message: /연결 누락/ }, name); }
    const noEntry = make(); noEntry.hooks.entry = null; assert.throws(() => noEntry.window.create(), /실제 TextEntry 본체 없음/);
});

test('완성 목록: 처음 한 번만 만들고(이미 준비됐으면 건너뜀) TYPE 수는 매 반복 다시 읽으며 프레임 높이 경계(7/8/16/17)와 TYPE 별 프레임 수를 따른다', () => {
    const first = make(); first.window.create(); const count = first.state.icons.length; assert.equal(count, 3);
    const ready = make(); ready.state.completionReady = 1; ready.window.create(); assert.deepEqual([ready.state.colors.length, ready.state.icons.length], [0, 0]);
    const dynamic = make({ frames: [[8], [16], [9]] }); let reads = 0; const global = dynamic.hooks.global; dynamic.hooks.global = (address) => { if (address === 0x541348) { reads += 1; return reads === 1 ? 5 : 1; } return global(address); };
    dynamic.window.create(); assert.deepEqual(dynamic.state.icons, ['~[I0.0]']); const lacking = make({ frames: [[8], [16], [9]] }); lacking.hooks.frameHeight = undefined; assert.throws(() => lacking.window.create(), /연결 누락/);
    const negative = make({ globals: { 0x541348: 0xffffffff } }); negative.window.create(); assert.deepEqual(negative.state.icons, []);
});

test('normalizePrefix: 입력의 첫 ":" 앞(없으면 빈 접두)+":" 를 앞에 두고 본문(첫 ":" 뒤)을 이어 쓰며 접두 버퍼에도 쓴다', () => {
    const r = built(); const run = (value) => { setInput(r, value); r.window.normalizePrefix(r.token); return [cstr(r.entry.storage.find(r.nodes.get(r.nodes.get(r.token).console).buffer).data()), cstr(r.nodes.get(r.token).inputPrefix)]; };
    assert.deepEqual(run('ann:hello'), ['ann:hello', 'ann:']); assert.deepEqual(run('hello'), ['hello', '']); assert.deepEqual(run('a:b:c'), ['a:b:c', 'a:']); assert.deepEqual(run(':x'), ['x', '']); assert.deepEqual(run(''), ['', '']);
});

test('changeRecipient: 작업 0 은 없을 때 공백+이름+공백을 더하고 1 은 찾은 이름과 뒤 공백을 지우며 유효하지 않은 접두는 비우고 본문 접두는 지우지 않는다', () => {
    const r = built(); const run = (input, name, operation) => { setInput(r, input); r.log.length = 0; r.window.changeRecipient(r.token, name, operation); return [cstr(r.entry.storage.find(r.nodes.get(r.nodes.get(r.token).console).buffer).data()), cstr(r.nodes.get(r.token).inputPrefix)]; };
    assert.deepEqual(run('hello', 'Ann', 0), ['Ann :hello', 'Ann :']); assert.deepEqual(run('Bob:hi', 'Ann', 0), ['Bob Ann :hi', 'Bob Ann :']); assert.deepEqual(run('Bob Ann :hi', 'Ann', 0), ['Bob Ann :hi', 'Bob Ann :']);
    assert.deepEqual(run('Bob Ann Cat:hi', 'Ann', 1), ['Bob Cat:hi', 'Bob Cat:']); assert.deepEqual(run('Ann:hi', 'Ann', 1), ['hi', '']);
    // 유효하지 않은 접두는 비우고 본문은 첫 ':' 이후를 지우지 않는다.
    assert.deepEqual(run('Zed:a:b', 'Ann', 0), ['Ann :Zed:a:b', 'Ann :']); assert.deepEqual(run('hello', null, 0)[0], 'hello'); assert.equal(r.log.length, 0);
    run('Ann:hi', 'Ann', 2); assert.ok(r.log.some((entry) => entry[0] === 'assertion' && entry[1] === 0x561)); const q = built(); delete q.hooks.assertion; setInput(q, 'x'); q.window.changeRecipient(q.token, 'Ann', 2);
    const noRecipient = built(); delete noRecipient.hooks.recipient; setInput(noRecipient, 'Ann:x'); assert.throws(() => noRecipient.window.changeRecipient(noRecipient.token, 'Ann', 0), /연결 누락/);
});

test('containsRecipient: 유효한 접두만 남기고 대소문자 구분 부분 문자열 검색이며 빈 이름은 항상 일치한다', () => {
    const r = built(); setInput(r, 'Bob Ann:hi'); assert.equal(r.window.containsRecipient(r.token, 'Ann'), true); assert.equal(r.window.containsRecipient(r.token, 'ann'), false); assert.equal(r.window.containsRecipient(r.token, ''), true);
    setInput(r, 'hi'); assert.equal(r.window.containsRecipient(r.token, 'Ann'), false); assert.equal(r.window.containsRecipient(r.token, ''), true); setInput(r, 'Zed:hi'); assert.equal(r.window.containsRecipient(r.token, 'Zed'), false); assert.equal(r.window.containsRecipient(r.token, 'Ann\0x'), false);
});

test('handle 일반 사건: 스크롤 키는 로그에 요청하고 좌우 버튼·해제는 기본 입력으로 넘기며 그 밖도 기본 입력으로 위임한다', () => {
    const r = built(); const text = r.nodes.get(r.token).text; const root = r.nodes.get(r.token); root.baseHeight = 100.75;
    const run = (event) => { r.log.length = 0; const result = r.window.handle(r.token, event, 7, 8); return [result, r.log.map((entry) => entry.join(':'))]; };
    assert.deepEqual(run(0x240000), [1, [`requestScroll:${text}:0`]]); assert.deepEqual(run(0x230000), [1, [`requestBottom:${text}`]]);
    assert.deepEqual(run(0x210000), [1, [`relativeScroll:${text}:-100`]]); assert.deepEqual(run(0x220000), [1, [`relativeScroll:${text}:100`]]); assert.deepEqual(run(0x260000), [1, [`relativeScroll:${text}:-5`]]); assert.deepEqual(run(0x280000), [1, [`relativeScroll:${text}:5`]]);
    for (const event of [0x10000, 0x20000, 0x80240000, 0x80230000, 0x270000, 0x41, 0x04210000]) assert.deepEqual(run(event)[1].map((line) => line.split(':')[0]), event === 0x04210000 ? [`relativeScroll`] : ['baseHandle'], event.toString(16));
    assert.equal(run(0x240001)[0], 0x1234); assert.deepEqual(run(0x41)[1], [`baseHandle:${r.token}:65:7:8`]);
});

test('handle 명령 사건 삽입/복사/붙여넣기: 같은 전역 사건이면 문자열을 입력에 끝 삽입하고 copy/paste 는 버퍼에 위임하며 문자열이 없으면 Error 다', () => {
    const r = built(); const COMMAND = 0x02000000; const event = COMMAND | 0x5001; const buffer = () => r.entry.storage.find(r.nodes.get(r.nodes.get(r.token).console).buffer);
    r.window.handle(r.token, event, 0, 0, '~[C.1]'); assert.equal(cstr(buffer().data()), '~[C.1]'); assert.equal(r.log.some((entry) => entry[0] === 'baseHandle'), true);
    assert.throws(() => r.window.handle(r.token, event, 0, 0), { name: 'Error', message: /삽입 사건의 실제 문자열 없음/ });
    r.entry.clip.text = 'PASTED'; r.window.handle(r.token, event, 0, 0, 'paste'); assert.equal(cstr(buffer().data()), '~[C.1]PASTED'); r.window.handle(r.token, event, 0, 0, 'copy'); assert.deepEqual(r.entry.clip.written.at(-1), '~[C.1]PASTED');
});

test('handle 명령 사건 helper 메뉴/부모 전달/재부착: helper 의 serial 이 일치하면 메뉴를 열고, 부모 전달은 현재 root 가 있을 때만 하며 1 을 돌려주고, 재부착은 Push 뒤 현재 좌표로 부착한다', () => {
    const r = built(); const COMMAND = 0x02000000; const helper = r.nodes.get(r.nodes.get(r.token).helper); r.nodes.get(r.token).x = 3; r.nodes.get(r.token).y = 4;
    r.window.handle(r.token, COMMAND | 0x5002, helper.serial + 1, 0); assert.equal(r.menu.length, 0); r.window.handle(r.token, COMMAND | 0x5002, helper.serial, 0); assert.ok(r.menu.includes('title:<Chat Helper:>'));
    r.log.length = 0; assert.equal(r.window.handle(r.token, COMMAND | 0x5003, 1, 2), 1); assert.deepEqual(r.log, [['bubble', r.token, C(0x5008), 0, 1]]); r.state.current = 0; r.log.length = 0; assert.equal(r.window.handle(r.token, COMMAND | 0x5003, 1, 2), 1); assert.deepEqual(r.log, []);
    r.log.length = 0; assert.equal(r.window.handle(r.token, COMMAND | 0x5007, 0, 0), 0x1234); assert.deepEqual(r.log.map((entry) => entry[0]), ['push', 'attach', 'baseHandle']); assert.deepEqual(r.log[1], ['attach', r.token, 3, 4, 0, 0]);
    assert.equal(r.window.handle(r.token, COMMAND | 0x7777, 0, 0), 0x1234);
});

test('handle 입력줄 제출: Console serial 이 일치하면 개행을 붙여 보내고(길이 -2 가 아니면) 유효 접두만 남겨 다시 정규화한다', () => {
    const r = built(); const COMMAND = 0x02000000; const console = r.nodes.get(r.nodes.get(r.token).console); setInput(r, 'Ann:hello');
    assert.equal(r.window.handle(r.token, COMMAND | 0x5004, console.serial + 1, 0), 0x1234); assert.equal(r.log.some((entry) => entry[0] === 'sendChat'), false);
    r.log.length = 0; assert.equal(r.window.handle(r.token, COMMAND | 0x5004, console.serial, 0), 1); assert.deepEqual(r.log.filter((entry) => entry[0] === 'sendChat'), [['sendChat', 0x5009, 'Ann:hello\n']]);
    assert.deepEqual([cstr(r.entry.storage.find(console.buffer).data()), cstr(r.nodes.get(r.token).inputPrefix)], ['Ann:', 'Ann:']); setInput(r, 'Zed:hi'); r.window.handle(r.token, COMMAND | 0x5004, console.serial, 0); assert.deepEqual([cstr(r.entry.storage.find(console.buffer).data()), cstr(r.nodes.get(r.token).inputPrefix)], ['', '']);
    setInput(r, 'plain'); r.log.length = 0; r.window.handle(r.token, COMMAND | 0x5004, console.serial, 0); assert.equal(r.log.filter((entry) => entry[0] === 'sendChat')[0][2], 'plain\n');
});

test('handle 수신자 추가/제거: 사건의 subscriber 로 이름을 조회해 접두를 바꾸고 1 을 돌려주며 조회 실패는 진단을 알리고 1 이다(remove 는 add 가 아닐 때만 조회)', () => {
    const r = built(); r.hooks.names = { 31: 'Ann', 32: 'Bob' }; const COMMAND = 0x02000000; const buffer = () => cstr(r.entry.storage.find(r.nodes.get(r.nodes.get(r.token).console).buffer).data());
    setInput(r, 'hello'); assert.equal(r.window.handle(r.token, COMMAND | 0x5005, 9, 31), 1); assert.equal(buffer(), 'Ann :hello'); assert.equal(r.window.handle(r.token, COMMAND | 0x5005, 9, 32), 1); assert.equal(buffer(), 'Ann Bob :hello');
    assert.equal(r.window.handle(r.token, COMMAND | 0x5006, 9, 31), 1); assert.equal(buffer(), 'Bob :hello'); r.log.length = 0; assert.equal(r.window.handle(r.token, COMMAND | 0x5005, 12, 99), 1); assert.deepEqual(r.log.filter((entry) => entry[0] === 'log'), [['log', 0x13, 'Could not lookup a name for subId == %d\n', 12]]);
    const noResolve = built(); delete noResolve.hooks.resolve; assert.throws(() => noResolve.window.handle(noResolve.token, COMMAND | 0x5005, 1, 1), /연결 누락/);
});

test('appendLog: 맨 아래였다면 추가 뒤 맨 아래 요청을 하고, 지정한 문맥(정확히 8개)은 부모 root 문맥에 쓴 뒤 정상 복귀에서 원래 값으로 복원한다', () => {
    const r = built(); const root = r.nodes.get(r.token); const child = root.text; const textNode = r.nodes.get(child); root.textContext.set([1, 2, 3, 4, 5, 6, 7, 8]); textNode.textContext.set([9, 9, 9, 9, 9, 9, 9, 9]);
    r.hooks.bottom = true; r.window.appendLog(r.token, 'line', [10, 20, 30, 40, 50, 60, 70, 80]); assert.deepEqual(r.log.map((entry) => entry[0]), ['atBottom', 'appendLog', 'requestBottom']); assert.deepEqual(Array.from(root.textContext), [9, 9, 9, 9, 9, 9, 9, 9]); // 원문은 자식 문맥을 저장하고 부모에 복원한다.
    r.log.length = 0; r.hooks.bottom = false; r.window.appendLog(r.token, 'line2'); assert.deepEqual(r.log.map((entry) => entry[0]), ['atBottom', 'appendLog']);
    // 문맥은 추가 도중 root 에 쓰여 있고 자식 문맥은 건드리지 않는다.
    let seen = null; r.hooks.appendLog = () => { seen = Array.from(root.textContext); }; r.window.appendLog(r.token, 'x', [10, 20, 30, 40, 50, 60, 70, 80]); assert.deepEqual(seen, [10, 20, 30, 40, 50, 60, 70, 80]); assert.deepEqual(Array.from(textNode.textContext), [9, 9, 9, 9, 9, 9, 9, 9]);
    assert.throws(() => r.window.appendLog(r.token, 'x', [1, 2, 3]), { name: 'TypeError', message: /8DWORD/ });
    // 하위가 던지면 문맥을 복원하지 않는다(원문의 부분 효과 유지).
    r.hooks.appendLog = () => { throw new Error('실패'); }; assert.throws(() => r.window.appendLog(r.token, 'x', [10, 20, 30, 40, 50, 60, 70, 80]), /실패/); assert.deepEqual(Array.from(root.textContext), [10, 20, 30, 40, 50, 60, 70, 80]);
});

test('attach/destruct: 재부착은 기하 본체에 위임하고, 소멸은 가상 표를 쓰고 현재 root 를 0 으로 쓴 뒤 기본 소멸을 호출한다', () => {
    const r = built(); r.window.attach(r.token, 1, 2, 3, 4); assert.deepEqual(r.log.at(-1), ['geometry.attach', r.token, 1, 2, 3, 4]);
    r.nodes.get(r.token).vtable = 5; r.window.destruct(r.token); assert.deepEqual([r.nodes.get(r.token).vtable, r.state.current], [0x504a94, 0]); assert.deepEqual(r.log.at(-1), ['destructBase', r.token]); assert.throws(() => r.window.destruct(0), /공유 root 없음/);
});

// 도우미 메뉴의 기록 형식 보조: item:라벨|사건|인자.
test('도우미 메뉴(일반 미션): 제목 → 두 begin → 세 그룹 목록(색/배경/아이콘) → 키워드 → 스타일 → 미리 정의된 색 → 명령/삽입 → finalize/open 순서다(메타맵/전투 항목 없음)', () => {
    const r = built(); r.state.colors = ['x', 'y']; r.state.backgrounds = ['p', 'q']; r.state.icons = ['i', 'j']; const COMMAND = 0x02000000; const helper = r.nodes.get(r.nodes.get(r.token).helper); r.window.handle(r.token, COMMAND | 0x5002, helper.serial, 0);
    const insert = C(0x5001); const expected = ['begin', 'title:<Chat Helper:>', 'begin', 'begin', 'begin', `item:x~x|${insert}|x`, 'sub:<Colors 1 to 0>|801', 'sub:<Text Colors>|802', 'begin', 'begin', `item:p~p|${insert}|p`, 'sub:<BGColors 1 to 0>|803', 'sub:<Background Colors>|804', 'begin',
        'begin', `item:i~i|${insert}|i`, 'sub:<Icons 1 to 0>|805', 'sub:<Icons>|806', 'begin', `item:Mimic|${insert}|~[]`, `item:Sound|${insert}|~[S.name]`, `item:Icon|${insert}|~[I.name]`, `item:Emboss Offset|${insert}|~[E.X#xY#]`, 'sub:<KeyWords>|807', 'begin'];
    assert.deepEqual(r.menu.slice(0, expected.length), expected);
    const rest = r.menu.slice(expected.length); assert.deepEqual(rest.slice(0, 8), ['<~BBold>', '<~EEmboss>', '<~IItalic>', '<~SStrikeout>', '<~UUnderline>', '<~.Reset Style>', '<~NReset Color & Emboss>', '<~*Reset to Default>'].map((label, index) => `item:${label}|${insert}|${['~B', '~E', '~I', '~S', '~U', '~.', '~N', '~*'][index]}`));
    assert.deepEqual(rest.slice(8, 10), ['sub:<Styles>|808', 'begin']); assert.equal(rest[10], `item:<~kBlack>|${insert}|~k`); assert.equal(rest[29], `item:<~nReset Color>|${insert}|~n`); assert.deepEqual(rest.slice(30, 32), ['sub:<Prefabricated Colors>|809', 'begin']);
    assert.deepEqual(rest.slice(32), [`item:<.front>|${insert}|.front`, `item:<.back>|${insert}|.back`, `item:<.format>|${insert}|.format`, 'sub:<Commands>|810', 'sub:<Insert>|811', `item:<Copy>|${insert}|copy`, `item:<Paste>|${insert}|paste`, 'finalize', 'open:11,12,13']);
});

test('도우미 메뉴 그룹: 21개 단위(색/배경)와 23개 단위(아이콘) 묶음 제목·마지막 원소 제외·빈 목록 Error', () => {
    const r = built(); r.state.colors = Array.from({ length: 25 }, (_, index) => `c${index}`); r.state.backgrounds = ['b']; r.state.icons = Array.from({ length: 30 }, (_, index) => `i${index}`);
    const COMMAND = 0x02000000; const helper = r.nodes.get(r.nodes.get(r.token).helper); r.window.handle(r.token, COMMAND | 0x5002, helper.serial, 0); const subs = r.menu.filter((line) => line.startsWith('sub:')).map((line) => line.split('|')[0]);
    assert.deepEqual(subs.slice(0, 6), ['sub:<Colors 0 to 21>', 'sub:<Colors 22 to 23>', 'sub:<Text Colors>', 'sub:<BGColors 1 to 4294967295>', 'sub:<Background Colors>', 'sub:<Icons 1 to 23>']); assert.deepEqual(subs.slice(6, 8), ['sub:<Icons 24 to 28>', 'sub:<Icons>']);
    assert.equal(r.menu.filter((line) => line.startsWith('item:c')).length, 24); assert.equal(r.menu.filter((line) => line.startsWith('item:i')).length, 29); assert.equal(r.menu.includes('item:c24~c24|20481|c24'), false);
    for (const list of ['colors', 'backgrounds', 'icons']) { const q = built(); q.state[list] = []; assert.throws(() => q.window.handle(q.token, COMMAND | 0x5002, q.nodes.get(q.nodes.get(q.token).helper).serial, 0), { name: 'Error', message: /빈 자동완성 벡터 범위/ }, list); }
    const single = built(); single.state.colors = ['only']; single.window.handle(single.token, COMMAND | 0x5002, single.nodes.get(single.nodes.get(single.token).helper).serial, 0); assert.ok(single.menu.includes('sub:<Colors 1 to 4294967295>|802'.replace('|802', '|802')) || single.menu.some((line) => line.startsWith('sub:<Colors 1 to 4294967295>')));
});

test('도우미 메뉴 설정 항목: 메타맵/전투 모드에서만 서식·효과(+전투는 채널/플레이어/채팅) 항목이 붙고 체크 항목은 설정을 두 번 읽어 할당 문자열(=0/=1)과 체크 값을 만든다', () => {
    const open = (globals, config) => { const r = built({ globals, config }); r.state.colors = ['a', 'b']; r.state.backgrounds = ['a', 'b']; r.state.icons = ['a', 'b']; const COMMAND = 0x02000000; r.window.handle(r.token, COMMAND | 0x5002, r.nodes.get(r.nodes.get(r.token).helper).serial, 0); return r; };
    const plain = open({}, {}); assert.equal(plain.menu.some((line) => line.startsWith('checked:')), false); assert.equal(plain.menu.some((line) => line.startsWith('config:')), false);
    const meta = open({ 0x594fc8: 1 }, { allowzonesounds: 1, zonebgcolor: 0 }); assert.deepEqual(meta.menu.slice(0, 4), ['begin', 'title:<Chat Helper:>', 'begin', 'item:<Edit Back Format>|' + C(0x500b) + '|null']);
    assert.deepEqual(meta.menu.filter((line) => line.startsWith('checked:')), [`checked:1|<Allow Chat Sound>|${C(0x500a)}|allowzonesounds=0`, `checked:0|<Allow Background Colors>|${C(0x500a)}|zonebgcolor=1`, `checked:0|<Allow Foreground Colors>|${C(0x500a)}|zonefgcolor=1`, `checked:0|<Allow Embossed Offset>|${C(0x500a)}|zoneoffset=1`]);
    assert.deepEqual(meta.menu.filter((line) => line.startsWith('config:')).slice(0, 4), ['config:allowzonesounds', 'config:allowzonesounds', 'config:zonebgcolor', 'config:zonebgcolor']); assert.equal(meta.menu.some((line) => line.includes('Show General Channel')), false);
    const battle = open({ 0x594fbc: 1 }, { ShowEveryone: 1, PlaySoundMessage: 1 }); assert.equal(battle.menu.filter((line) => line.startsWith('checked:')).length, 4 + 4 + 3 + 2); assert.ok(battle.menu.includes(`checked:1|<Show General Channel>|${C(0x500a)}|ShowEveryone=0`)); assert.ok(battle.menu.includes(`checked:1|<Play sound on Message>|${C(0x500a)}|PlaySoundMessage=0`));
    assert.ok(battle.menu.includes('sub:<Channel Controls>|801') || battle.menu.some((line) => line.startsWith('sub:<Channel Controls>'))); assert.deepEqual(['Chat Effects ', 'Channel Controls', 'Player Controls', 'Chat Controls'].map((name) => battle.menu.some((line) => line.startsWith(`sub:<${name}>`))), [true, true, true, true]);
    const missing = built(); delete missing.hooks.menuOpen; missing.state.colors = ['a', 'b']; missing.state.backgrounds = ['a', 'b']; missing.state.icons = ['a', 'b']; assert.throws(() => missing.window.handle(missing.token, 0x02000000 | 0x5002, missing.nodes.get(missing.nodes.get(missing.token).helper).serial, 0), /연결 누락/);
});

test('생성 상세: 입력줄 크기·글꼴·색·배경 플래그, Slider/스크롤 인자, 표시 비트 해제, 완성 목록 TYPE 수 경계', () => {
    const r = make(); const token = r.window.create(); const root = r.nodes.get(token); const console = r.nodes.get(root.console);
    assert.deepEqual([console.baseWidth, console.baseHeight, console.backgroundFlags, console.textContext[1], console.textContext[2]], [180, 16, 0x04000022, 5, 0x33]); assert.equal(console.flags & 1, 0);
    assert.deepEqual(r.log.filter((entry) => ['setSlider', 'setScrollable'].includes(entry[0])), [['setSlider', root.text, 1], ['setScrollable', root.text, 1]]);
    const counted = make({ globals: { 0x541348: 2 }, frames: [[8], [9], [10]] }); counted.window.create(); assert.deepEqual(counted.state.icons, ['~[I0.0]', '~[I1.0]']);
    const none = make({ globals: { 0x541348: 0 }, frames: [[8]] }); none.window.create(); assert.deepEqual(none.state.icons, []);
});

test('Console/ScrollingText 조회: 기본 주소와 다른 파생 객체를 돌려주면 거부한다', () => {
    const copyOf = (node) => (node ? Object.assign(Object.create(Object.getPrototypeOf(node)), node) : null);
    const a = make(); const original = a.hooks.console; a.hooks.console = (token) => copyOf(original(token)); assert.throws(() => a.window.create(), { name: 'Error', message: /실제 Console 없음/ });
    const b = make(); const originalText = b.hooks.text; b.hooks.text = (token) => copyOf(originalText(token)); assert.throws(() => b.window.create(), { name: 'Error', message: /실제 ScrollingText 없음/ });
});

test('입력 복사 128 한도와 접두 128 한도: 127바이트까지 허용하고 128바이트부터 Error 다', () => {
    const r = built(); setInput(r, 'a'.repeat(127)); r.window.normalizePrefix(r.token); setInput(r, 'a'.repeat(128)); assert.throws(() => r.window.normalizePrefix(r.token), { name: 'Error', message: /복사128 범위/ });
    const q = built(); q.hooks.valid = () => true; setInput(q, 'p'.repeat(100) + ':x'); q.window.changeRecipient(q.token, 'n'.repeat(24), 0); assert.equal(cstr(q.nodes.get(q.token).inputPrefix).length, 127);
    const s = built(); s.hooks.valid = () => true; setInput(s, 'p'.repeat(100) + ':x'); assert.throws(() => s.window.changeRecipient(s.token, 'n'.repeat(25), 0), { name: 'Error', message: /접두128 범위/ });
});

test('changeRecipient 경계: 작업 0/1 은 진단 없이 처리하고 0 번째 위치의 이름은 다시 더하지 않으며 빈 접두의 ":" 입력은 유효 검사를 하고 본문의 첫 ":" 를 지운다', () => {
    const r = built(); const text = () => cstr(r.entry.storage.find(r.nodes.get(r.nodes.get(r.token).console).buffer).data());
    for (const operation of [0, 1]) { setInput(r, 'Ann:hi'); r.log.length = 0; r.window.changeRecipient(r.token, 'Ann', operation); assert.equal(r.log.some((entry) => entry[0] === 'assertion'), false, String(operation)); }
    setInput(r, 'Ann Bob:hi'); r.window.changeRecipient(r.token, 'Ann', 0); assert.equal(text(), 'Ann Bob:hi'); setInput(r, ':x'); r.log.length = 0; r.window.changeRecipient(r.token, 'Ann', 0); assert.equal(text(), 'Ann :x'); assert.deepEqual(r.log.filter((entry) => entry[0] === 'recipient'), [['recipient', '']]);
    setInput(r, ':x'); r.log.length = 0; assert.equal(r.window.containsRecipient(r.token, 'Ann'), false); assert.deepEqual(r.log.filter((entry) => entry[0] === 'recipient'), [['recipient', '']]);
    const nul = String.fromCharCode(0); setInput(r, 'Ann:hi'); assert.equal(r.window.containsRecipient(r.token, nul + 'zzz'), true); setInput(r, 'Ann:hi'); r.window.changeRecipient(r.token, nul + 'Cat', 0); assert.equal(text(), 'Ann:hi');
});

test('handle 경계: 명령 비트가 없는 사건은 전역 값과 같은 코드여도 명령으로 처리하지 않고, 입력 길이 -2 이면 보내지 않으며, 접두 조회가 빈 접두도 유효 검사를 한다', () => {
    const r = built({ globals: { 0x54298c: 0x41 } }); assert.equal(r.window.handle(r.token, 0x41, 0, 0), 0x1234); assert.equal(r.log.some((entry) => entry[0] === 'push'), false);
    const q = built(); const console = q.nodes.get(q.nodes.get(q.token).console); const buffer = () => q.entry.storage.find(console.buffer); setInput(q, 'x');
    const original = q.hooks.entry; q.hooks.entry = { ...original, appendText: (...args) => { original.appendText(...args); buffer().state().length = -2; } }; q.log.length = 0; q.window.handle(q.token, 0x02005004, console.serial, 0);
    assert.equal(q.log.some((entry) => entry[0] === 'sendChat'), false);
    const p = built(); setInput(p, ':x'); p.log.length = 0; p.window.handle(p.token, 0x02005004, p.nodes.get(p.nodes.get(p.token).console).serial, 0); assert.deepEqual(p.log.filter((entry) => entry[0] === 'recipient'), [['recipient', '']]);
});

test('handle 수신자 사건: subscriber 4바이트·지역 60바이트·모드 0 으로 조회하고 이름은 40바이트까지만 쓴다', () => {
    const r = built(); const COMMAND = 0x02000000; const buffer = () => cstr(r.entry.storage.find(r.nodes.get(r.nodes.get(r.token).console).buffer).data()); r.hooks.valid = () => true;
    r.hooks.names = { [0x01020304]: 'Ann' }; setInput(r, 'hi'); r.log.length = 0; assert.equal(r.window.handle(r.token, COMMAND | 0x5005, 7, 0x01020304), 1); assert.deepEqual(r.log.find((entry) => entry[0] === 'resolve'), ['resolve', 0x01020304, 60, 0, 0]); assert.equal(buffer(), 'Ann :hi');
    r.hooks.names = {}; r.hooks.rawName = new Uint8Array(52).fill(0x78); setInput(r, ''); r.window.handle(r.token, COMMAND | 0x5005, 7, 1); assert.equal(buffer(), 'x'.repeat(40) + ' :');
    r.hooks.rawName = Uint8Array.from([...new Uint8Array(40).fill(0x79), 0x7a, 0x7a, 0]); setInput(r, ''); r.window.handle(r.token, COMMAND | 0x5005, 7, 1); assert.equal(buffer(), 'y'.repeat(40) + ' :');
    r.hooks.rawName = Uint8Array.from([...new Uint8Array(39).fill(0x6b), 0]); setInput(r, ''); r.window.handle(r.token, COMMAND | 0x5005, 7, 1); assert.equal(buffer(), 'k'.repeat(39) + ' :');
    r.hooks.rawName = Uint8Array.from([0x6d, 0]); setInput(r, ''); r.window.handle(r.token, COMMAND | 0x5005, 7, 1); assert.equal(buffer(), 'm :');
    // 이름 칸을 넘은 위치(+49)의 NUL 은 40바이트로 자른다. NUL 뒤 바이트는 이름이 아니다(원문 C 문자열).
    r.hooks.rawName = Uint8Array.from([...new Uint8Array(41).fill(0x71), 0]); setInput(r, ''); r.window.handle(r.token, COMMAND | 0x5005, 7, 1); assert.equal(buffer(), 'q'.repeat(40) + ' :');
    setInput(r, 'Bob:hi'); r.window.changeRecipient(r.token, 'Ann' + String.fromCharCode(0) + 'junk', 0); assert.equal(buffer(), 'Bob Ann :hi');
});

test('도우미 메뉴 보강: 메타맵 모드의 Edit Front Format 사건, 배경 목록의 21개 묶음, 긴 목록의 둘째 묶음 제목', () => {
    const r = built({ globals: { 0x594fc8: 1 } }); const COMMAND = 0x02000000; r.state.colors = Array.from({ length: 50 }, (_, index) => 'c' + index); r.state.backgrounds = Array.from({ length: 25 }, (_, index) => 'b' + index); r.state.icons = ['i', 'j'];
    r.window.handle(r.token, COMMAND | 0x5002, r.nodes.get(r.nodes.get(r.token).helper).serial, 0);
    assert.ok(r.menu.includes('item:<Edit Front Format>|' + C(0x500c) + '|null')); const subs = r.menu.filter((line) => line.startsWith('sub:<Colors') || line.startsWith('sub:<BGColors')).map((line) => line.split('|')[0]);
    assert.deepEqual(subs, ['sub:<Colors 0 to 21>', 'sub:<Colors 22 to 42>', 'sub:<Colors 43 to 48>', 'sub:<BGColors 0 to 21>', 'sub:<BGColors 22 to 23>']);
});
