// 채팅 창(ChatWindow, 0x13c 바이트 root)의 원본 10.78 독립 관측 180행 대조: 생성/Create/정규화/사건 처리/도우미 메뉴/소멸의 root 16필드·중첩 Console 48필드·로그 4필드·접두·입력 본문·자동완성 목록·호출 순서·반환을
// cpppj `ChatWindowTests` 와 같은 입력 구성으로 비교한다. 노드/버퍼/Slider/할당 블록 장치는 TextEntryGump 시험과 같은 지원 틀(`text-entry-gump-native-support.mjs`)을 쓰고, 그 위에 Chat 의 자식 생성·메뉴·전역 읽기 장치를 얹는다.
// 앞 21열(입력)만 장치에 공급하고 뒤 10열(관측)은 비교에만 쓴다. 원본 게임을 실행하지 않는다. (2026-11-15 WB04-E485)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { ChatWindow, ChatWindowState } from '../www/ui/chat-window.js';
import { HudChatNode } from '../www/ui/hud-chat-geometry.js';
import { GumpBackgroundNode } from '../www/ui/gump-background-create.js';
import { ScrollingTextGumpNode } from '../www/ui/text-gump.js';
import { EntryScene, ENTRY_NODE, bitsOf, entryNumber, hexOf, packed, signed, unhex } from './text-entry-gump-native-support.mjs';

// 원문 격리 장치의 root/helper/배경/로그/공유 버퍼/문자열 입력 토큰과 Chat 가상 표이다.
const CHAT_ROOT = 0x01003000; const CHAT_HELPER = 0x01004000; const CHAT_BACKGROUND = 0x01005000; const CHAT_SCROLL = 0x01006000; const CHAT_SHARED = 0x01007000; const CHAT_PAYLOAD = 0x01008000; const CHAT_VTABLE = 0x0117f000;
// 바이트 문자열을 소문자 16진으로 만든다(빈 문자열은 `-`).
const chatHex = (value) => (value.length === 0 ? '-' : Buffer.from(value, 'latin1').toString('hex'));
// 원문 Console 입력 장치에 넘길 앞부분 입력 행을 chat 행의 열로 만든다(관측 열은 쓰지 않는다).
function chatEntryInput(row) {
    const input = new Array(23).fill('');
    input[2] = row[18]; input[3] = row[19]; input[4] = row[20]; input[6] = row[8]; input[8] = '4109420a43'; input[9] = '0.5:0.75'; input[10] = '776f726c64'; input[11] = row[13]; input[12] = '165';
    return input;
}

// 같은 실제 TextEntry 상태와 외부 공통 Gump/메뉴 장치를 연결한다.
class ChatScene extends EntryScene {
    constructor(row) {
        super(chatEntryInput(row));
        this.root = new HudChatNode(); this.helper = new GumpBackgroundNode(); this.background = new GumpBackgroundNode(); this.scrolling = new ScrollingTextGumpNode();
        this.chatState = new ChatWindowState(); this.menuNumber = 0; this.resolved = null;
        for (const node of [this.root, this.helper, this.background, this.scrolling]) { node.vtable = CHAT_VTABLE; node.flags = 1; node.scale = 1; }
        this.root.baseWidth = Math.fround(signed(entryNumber(row[2]))); this.root.baseHeight = Math.fround(signed(entryNumber(row[3])));
        this.root.console = ENTRY_NODE; this.root.text = CHAT_SCROLL; this.root.helper = CHAT_HELPER; this.root.background = CHAT_BACKGROUND;
        this.node.serial = 21; this.helper.serial = 23; this.scrolling.buffer = CHAT_SHARED;
        this.chatState.current = CHAT_ROOT; this.chatState.registered = entryNumber(row[9]); this.chatState.completionReady = row[15] === '~' ? 0 : 1;
        // 전역 읽기 장치의 현재 값 표이다. 없는 주소는 오류이다(std::map::at).
        this.values = new Map([[0, entryNumber(row[12])], [0x540424, 9], [0x5acd24, 9], [0x5b5dc0, 32], [0x5b5e60, 30], [0x5b5dd4, 31], [0x5b5dd8, 33], [0x594fbc, entryNumber(row[10])], [0x594fc8, entryNumber(row[11])], [0x548714, 0x11223344],
            [0x5cae94, 11], [0x5cae98, 22], [0x563e38, 33], [0x542b24, 0x2000201], [0x5427a8, 0x2000021], [0x5429f0, 0x20000bf], [0x542914, 0x2000202], [0x5427d8, 0x2000038], [0x542794, 0x2000203], [0x542af8, 0x2000204], [0x54298c, 0x2000205],
            [0x542b08, 0x2000206], [0x542b0c, 0x2000207], [0x5428ac, 0x2000051]]);
        // 세 자동완성 목록의 원시 입력이다(`~` 는 빈 목록).
        const list = (value) => (value === '~' ? [] : value.split(':').map((hex) => Buffer.from(unhex(hex)).toString('latin1')));
        this.values.set(0x541348, 2); this.chatState.colors = list(row[15]); this.chatState.backgrounds = list(row[16]); this.chatState.icons = list(row[17]);
        if (row[14] !== '~') this.resolved = Buffer.from(unhex(row[14])).toString('latin1');
        this.chat = new ChatWindow(this.chatState, this.chatHooks());
    }
    // 같은 기본 주소를 공유하는 root/세 자식과 기존 Console/Slider 의 조회이다.
    chatBase(token) { return token === CHAT_ROOT ? this.root : token === CHAT_HELPER ? this.helper : token === CHAT_BACKGROUND ? this.background : token === CHAT_SCROLL ? this.scrolling : this.base(token); }
    // root 16필드를 원문 순서의 부호 없는 값으로 읽는다.
    rootFields() { const root = this.root; return [root.vtable, root.flags, root.command, bitsOf(root.baseWidth), bitsOf(root.baseHeight), bitsOf(root.width), bitsOf(root.height), root.console, root.text, root.helper, root.background, root.capacity, root.leftPadding, this.chatState.current, this.chatState.registered, this.chatState.completionReady].map((value) => value >>> 0); }
    // 현재 세 문자열 목록을 `:`(목록 안)/`;`(목록 사이) 구분의 16진으로 관측한다(빈 목록은 `~`).
    completionFields() { return [this.chatState.colors, this.chatState.backgrounds, this.chatState.icons].map((list) => (list.length === 0 ? '~' : list.map((value) => hexOf(Buffer.from(value, 'latin1'))).join(':'))).join(';'); }
    // 값 표의 읽기(없으면 오류)이다.
    global(address) { if (!this.values.has(address)) throw new Error(`ChatWindow 전역 주소 없음 ${address.toString(16)}`); return this.values.get(address); }
    // 실제 부모와 큰 공통 장치 경계를 같은 순서/인자로 연결한다.
    chatHooks() {
        const e = (name, values = []) => this.event(name, values);
        return {
            chat: (token) => (token === CHAT_ROOT ? this.root : null), base: (token) => this.chatBase(token), console: (token) => (token === ENTRY_NODE ? this.node : null), text: (token) => (token === CHAT_SCROLL ? this.scrolling : null),
            buffer: (token) => (token === 0x01001000 ? this.buffer : null), entry: this.gump, global: (address) => this.global(address),
            allocate: (size) => { const token = size === 0x13c ? CHAT_ROOT : size === 0x170 ? CHAT_HELPER : size === 0xb8 ? CHAT_BACKGROUND : size === 0x128 ? ENTRY_NODE : size === 0xd8 ? CHAT_SCROLL : 0; e('new', [size, token]); return token; },
            constructBase: (token, width, height) => { const root = this.root; root.vtable = CHAT_VTABLE; root.flags = 1; root.scale = 1; root.baseWidth = root.width = width; root.baseHeight = root.height = height; e('constructBase', [token, 0, bitsOf(width), bitsOf(height), 1]); },
            constructHelper: (token, label, brown) => { this.helper.vtable = CHAT_VTABLE; this.helper.flags = 1; this.helper.scale = 1; e('constructHelper', [token, chatHex(label), brown >>> 0]); },
            fitHelper: (token) => { this.helper.baseWidth = 9; this.helper.baseHeight = 14; e('fitHelper', [token]); },
            title: (token, label) => e('title', [token, chatHex(label)]), help: (token, label) => e('help', [token, chatHex(label)]), translate: (label) => { e('translate', [chatHex(label)]); return label; },
            measure: (token, label) => { e('measureChat', [token, chatHex(label)]); return [6, 14]; },
            constructBackground: (token, width, height, flags) => { const node = this.background; node.vtable = CHAT_VTABLE; node.flags = 1; node.scale = 1; node.baseWidth = width; node.baseHeight = height; node.backgroundFlags = flags >>> 0; e('constructChatBackground', [token, bitsOf(width), bitsOf(height), flags >>> 0]); },
            constructScrolling: (token, width, height, font, ink) => { const node = this.scrolling; node.vtable = CHAT_VTABLE; node.flags = 1; node.scale = 1; node.baseWidth = width; node.baseHeight = height; node.buffer = CHAT_SHARED + 0x100; node.textFlags = 0x10; e('constructScrolling', [token, width >>> 0, height >>> 0, font, ink]); },
            replaceBuffer: (token, shared, owns) => { this.scrolling.buffer = shared; this.scrolling.ownsBuffer = owns; e('replaceBuffer', [token, shared, owns]); }, sharedBuffer: () => 0x00548758,
            setSlider: (token, value) => e('setSlider', [token, value]), setScrollable: (token, value) => e('setScrollable', [token, value]),
            attach: (token, x, y, parent, mode) => { if (token === ENTRY_NODE) this.gump.attach(token, x, y, parent, mode); else e('attach', [token, bitsOf(x), bitsOf(y), parent, mode]); },
            push: (token) => e('push', [token]), destructBase: (token) => e('destructBase', [token]),
            registerCommand: (name, callback, flag) => e('register', [chatHex(name), callback, flag]), bufferAppendCallback: (token, callback) => e('appendCallback', [token, callback]),
            frameCount: (type) => { e('frameCount', [type, 7]); return 7; },
            frameHeight: (type, frame) => { const heights = [-1, 7, 8, 9, 16, 17, 32767]; if (frame >= heights.length) throw new Error('ChatWindow 높이 범위'); const value = heights[frame]; e('frameHeight', [type, frame, value >>> 0]); return value; },
            requestScroll: (token, value) => e('requestScroll', [token, value >>> 0]), relativeScroll: (token, value) => e('relativeScroll', [token, value >>> 0]), requestBottom: (token) => e('requestBottom', [token]),
            baseHandle: (token, event, first, second) => { e('baseHandle', [token, event >>> 0, first >>> 0, second >>> 0]); return 77; }, bubble: (token, event, first, second) => e('bubble', [token, event >>> 0, first >>> 0, second >>> 0]),
            recipient: (label) => { e('recipient', [chatHex(label)]); return this.recipientFlag; }, sendChat: (manager, label) => e('sendChat', [manager >>> 0, chatHex(label)]),
            resolve: (record) => {
                const subscriber = (record[4] | (record[5] << 8) | (record[6] << 16) | (record[7] << 24)) >>> 0; e('resolve', [subscriber]);
                if (this.resolved !== null) { for (let index = 0; index < this.resolved.length; index += 1) record[8 + index] = this.resolved.charCodeAt(index); record[8 + this.resolved.length] = 0; }
                return this.resolved !== null;
            },
            log: (level, label, subscriber) => e('log', [level >>> 0, chatHex(label), subscriber >>> 0]),
            config: (key) => { e('config', [chatHex(key)]); return signed(this.values.get(0)); },
            menuBegin: () => e('menuBegin'), menuTitle: (label) => e('menuTitle', [chatHex(label)]),
            menuItem: (label, event, payload) => e('menuItem', [chatHex(label), event >>> 0, payload === null || payload === undefined ? '~' : chatHex(payload)]),
            menuChecked: (check, label, event, payload) => e('menuChecked', [check >>> 0, chatHex(label), event >>> 0, chatHex(payload)]),
            menuTake: () => { this.menuNumber += 1; e('menuTake', [this.menuNumber]); return this.menuNumber; }, menuSubmenu: (label, child) => e('menuSubmenu', [chatHex(label), child]),
            menuFinalize: () => e('menuFinalize'), menuOpen: (x, y, object) => e('menuOpen', [x >>> 0, y >>> 0, object >>> 0]),
        };
    }
}

test('ChatWindow 원본 10.78 180행: 생성/Create/정규화/사건 처리/도우미 메뉴/소멸의 root·Console·로그·접두·본문·자동완성·호출 순서·반환이 일치한다', () => {
    const table = fs.readFileSync(new URL('../../cpppj/tests/fixtures/chat-window-x86.tsv', import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && !line.startsWith('name\t')).map((line) => line.split('\t'));
    assert.equal(table.length, 180);
    const failures = [];
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 31, `행 ${index}`);
        const scene = new ChatScene(row); const operation = row[1]; let result = 0;
        if (operation === 'construct') scene.chat.construct(CHAT_ROOT, signed(entryNumber(row[2])), signed(entryNumber(row[3])));
        else if (operation === 'create') scene.chat.create();
        else if (operation === 'destroy') scene.chat.destruct(CHAT_ROOT);
        else if (operation === 'normalize') scene.chat.normalizePrefix(CHAT_ROOT);
        else {
            const text = Buffer.from(unhex(row[7])).toString('latin1');
            result = scene.chat.handle(CHAT_ROOT, entryNumber(row[4]), entryNumber(row[5]), row[6] === '~' ? CHAT_PAYLOAD : entryNumber(row[6]), row[6] === '~' ? text : undefined) >>> 0;
        }
        const problems = [];
        if (packed(scene.rootFields()) !== row[21]) problems.push('root');
        if (packed(scene.nodeFields()) !== row[22]) problems.push('console');
        if (packed([scene.scrolling.buffer, scene.scrolling.ownsBuffer, scene.scrolling.textFlags, scene.scrolling.callback].map((value) => value >>> 0)) !== row[23]) problems.push('scroll');
        let prefix = ''; for (const byte of scene.root.inputPrefix) { if (!byte) break; prefix += String.fromCharCode(byte); }
        if (chatHex(prefix) !== row[24]) problems.push('prefix');
        if (chatHex(scene.buffer.text()) !== row[25]) problems.push('input');
        if (scene.completionFields() !== row[26]) problems.push('completions');
        if ((scene.trace === '' ? '-' : scene.trace) !== row[27]) problems.push('trace');
        if (result !== entryNumber(row[28])) problems.push('result');
        if (entryNumber(row[29]) !== (operation === 'construct' ? 12 : operation === 'handle' ? 16 : 4)) problems.push('stack');
        if (!(entryNumber(row[30]) > 0)) problems.push('instructions');
        if (problems.length) failures.push(`${index}/${row[0]}/${operation}: ${problems.join(',')}`);
    }
    assert.deepEqual(failures.slice(0, 20), []);
});

// 관측 fixture 의 frameCount 장치는 항상 7 을 돌려줘 자동 변형 `count | 0`→`count | 1`(짝수 개수가 홀수로 바뀜)이 가려지지 않았다. 기준선 소스(ChatWindow.cpp)의 계약으로 독립 확인한다:
// 완성 목록이 비어 있는 첫 구성에서 TYPE 마다 frameCount 만큼만 frameHeight 를 읽고, 높이 8..16 인 프레임만 `~[I<type>.<frame>]` 아이콘 후보로 저장한다. (2026-11-15 WB04-E485)
test('ChatWindow 구성: 프레임 수가 짝수(6)여도 그 수만큼만 높이를 읽고 높이 8..16 프레임만 아이콘 후보가 된다(독립 확인)', () => {
    const table = fs.readFileSync(new URL('../../cpppj/tests/fixtures/chat-window-x86.tsv', import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line.length > 0 && !line.startsWith('name\t')).map((line) => line.split('\t'));
    const template = table.find((row) => row[1] === 'construct' && row[15] === '~'); assert.ok(template);
    const scene = new ChatScene(template);
    // 프레임 수를 6 으로 바꾼 장치로 같은 상태에 새 ChatWindow 를 만든다(높이는 같은 7칸 표의 앞 6칸).
    const heights = [-1, 7, 8, 9, 16, 17]; const asked = [];
    scene.chat = new ChatWindow(scene.chatState, { ...scene.chatHooks(), frameCount: () => 6, frameHeight: (type, frame) => { asked.push(`${type}.${frame}`); return heights[frame]; } });
    scene.chat.construct(CHAT_ROOT, 200, 100);
    assert.deepEqual(asked, ['0.0', '0.1', '0.2', '0.3', '0.4', '0.5', '1.0', '1.1', '1.2', '1.3', '1.4', '1.5']);
    assert.deepEqual(scene.chatState.icons, ['~[I0.2]', '~[I0.3]', '~[I0.4]', '~[I1.2]', '~[I1.3]', '~[I1.4]']);
});
