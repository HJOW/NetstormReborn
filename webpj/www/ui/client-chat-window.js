// 실제 미션 Client 의 채팅 창(원본 `ChatWindow`, 0x13c root)을 같은 Gump 트리에 조립한다. root·도우미(`>` 돌 버튼)·배경·입력줄(Console)·로그(ScrollingText) 네 자식을 `ui/chat-window.js` 본체와 `ui/hud-chat-geometry.js` 기하로 만들고
// 공유 로그 버퍼(`0x548758`, Client 장기 `globals.log`)를 로그 자식이 빌려 쓴다(cpppj `client/RawMenuChat.inc`·`UberHud.inc`).
// 사용: const chat = new ClientChatWindow(root, game, fonts, screen, parts, devices); chat.create()(Uber 루트의 createChat) → root 토큰; chat.setVisible(bool)/setInput(bool)(HudDock 의 chatVisible/chatInput);
//   chat.appendLog(text); chat.close().
//   parts = { scrolling(ClientScrollingText), buffers(ClientStyleTextBuffers), background(ClientGumpBackground), menu(ClientMenuGump 작성기), globals(ClientChatWindowGlobals), backgroundType }.
//   devices = { now(), poll(), sound(이름), baseHandle(토큰, 사건, 첫 값, 둘째 값), translate(문자열), changed(), read(원본 전역 주소) → 값|undefined, config(키) → 정수,
//               recipient(접두) → boolean, sendChat(manager, 글), resolve(기록 60바이트) → boolean, log(수준, 서식, 값) }. 선택 장치가 없으면 로컬 기본 동작(수신자 없음·입력은 로그에 되울림)을 쓴다.
// (2026-11-22 WB04-E1169: 채팅 창 조립 1단계 — 입력줄·로그·도우미·HUD 표시 전환)
import { ClientTextEntryControls, createMixedGumpTextMeasure } from './client-text-entry-controls.js';
import { createClientTextOutput, createMixedGumpTextDrawer } from './client-text-entry-draw.js';
import { ClientStoneControls } from './client-stone-controls.js';
import { ButtonGumpState } from './button-gump.js';
import { StoneButtonGumpState } from './stone-button-gump.js';
import { ScrollGumpState } from './scroll-gump.js';
import { ScrollGumpWidgetState } from './scroll-gump-widgets.js';
import { ConsoleGumpNode, TextEntryGumpState } from './text-entry-gump.js';
import { ScrollingTextGumpNode } from './text-gump.js';
import { ChatWindow } from './chat-window.js';
import { HudChatGeometry, HudChatNode } from './hud-chat-geometry.js';
import { GumpBackgroundNode } from './gump-background-create.js';
import { ClientDotCommandGlobals } from './dot-command-globals.js';
import { copyDotCommandName } from './dot-command-register.js';
import { GumpBackgroundAssets } from '../render/gump-background-draw.js';
import { GumpGlyphColorState, decodeGumpTextContext } from '../render/gump-text-draw.js';
import { displayTextCodec } from '../assets/display-text.js';

// 공유 로그 버퍼의 원문 주소 토큰과 줄 수 한도(원본 `0x548758`, 128줄)이다. (2026-11-22 WB04-E1169)
const SHARED_LOG = 0x548758;
const LOG_LINES = 128;
// 로그 한 줄의 끝 개행이다(원본 메시지가 포함하는 LF).
const NEWLINE = String.fromCharCode(10);
// Gump 버튼 소리 이름(원본 UberGump `button.wav`)이다.
const BUTTON_SOUND = 'button.wav';
// 도우미 라벨 저장소 크기(원문 64바이트)이다.
const LABEL_BYTES = 64;
// 원본 PE 의 이벤트 번호 표(0x542b24 등 전역 주소 → 사건 코드)이다. (2026-11-22 WB04-E1169)
const EVENT_GLOBALS = new Map([[0x542b24, 0x02000115], [0x5427a8, 0x0200002c], [0x5429f0, 0x020000bf], [0x542914, 0x02000088], [0x5427d8, 0x02000038], [0x542794, 0x02000027],
    [0x542af8, 0x02000101], [0x54298c, 0x020000a6], [0x542b08, 0x02000105], [0x542b0c, 0x02000106], [0x5428ac, 0x0200006d]]);
// 채팅 창 원본 팔레트 전역 주소 → 이름 색 번호이다(도우미 갈색 9, 배경 32, 입력줄 배경 30, 입력줄 글자 31, 로그 글자 33).
const PALETTE_GLOBALS = new Map([[0x5acd24, 9], [0x5b5dc0, 32], [0x5b5e60, 30], [0x5b5dd4, 31], [0x5b5dd8, 33]]);

// 같은 Client Gump 에 채팅 창을 만들고 수명을 관리한다. (2026-11-22 WB04-E1169)
export class ClientChatWindow {
    // 공통 소유자·입력창/돌 버튼 소유자·원본 채팅 본체·기하와 직접 할당한 노드, 삭제 중 flags 이다.
    #root; #entries; #stones; #window; #geometry; #devices; #parts; #game; #fonts; #screen; #owned = new Set(); #deleteFlags = new Map(); #glyphColors = new GumpGlyphColorState(); #dot = new ClientDotCommandGlobals();
    #measure; #labelColors; #backgroundType;
    // 같은 Client 의 Gump 루트·자산·글꼴·팔레트와 이미 조립된 로그·버퍼·배경·메뉴 작성기를 빌린다.
    constructor(root, game, fonts, screen, parts, devices) {
        if (!root || !game || !fonts || !screen || !parts?.scrolling || !parts?.buffers || !parts?.background || !parts?.menu || !parts?.globals || !Number.isInteger(parts.backgroundType) || !devices ||
            ['now', 'poll', 'sound', 'baseHandle', 'translate', 'changed', 'config'].some((name) => typeof devices[name] !== 'function')) throw new TypeError('채팅 창의 실제 장치 누락');
        this.#root = root; this.#game = game; this.#fonts = fonts; this.#screen = screen; this.#parts = parts; this.#devices = devices; this.#backgroundType = parts.backgroundType;
        const colors = screen.colors(), assets = new GumpBackgroundAssets(game); this.#measure = createMixedGumpTextMeasure(fonts);
        const buttonState = new ButtonGumpState(), stoneState = new StoneButtonGumpState(), scrollState = new ScrollGumpState(), widgetState = new ScrollGumpWidgetState(), entryState = new TextEntryGumpState();
        Object.assign(buttonState, { valueEvent: 0x02000070, clickEvent: 0x0200002c, rightEvent: 0x02000012, repeatEvent: 0x0200002f, baseColor: 56 });
        Object.assign(scrollState, { clickEvent: 0x0200002c, repeatEvent: 0x0200002f, stepEvent: 0x0200002e, changeEvent: 0x02000036 });
        stoneState.color = 49; stoneState.palette = { black: colors.basic[0], white: colors.basic[7], edge: colors.basic[8] }; stoneState.backgroundType = parts.backgroundType; stoneState.backgroundFrame = 1; widgetState.defaultType = parts.backgroundType;
        if (colors.named.length <= 33) throw new Error('채팅 창 실제 이름 팔레트 없음');
        entryState.brown = colors.named[9].index; entryState.black = colors.basic[0]; entryState.yellow = colors.basic[5];
        const shared = { buttonState, stoneState, scrollState, widgetState, backgroundColor: () => 35, type: (type) => assets.type(type), frame: (...args) => assets.frame(...args),
            output: { fonts, background: parts.background }, now: devices.now, poll: devices.poll, sound: () => devices.sound(BUTTON_SOUND), baseEvent: devices.baseHandle };
        this.#stones = new ClientStoneControls(root, { ...shared, measure: (token, label, context, output, maximum) => { const [width, height] = this.#measure(label, decodeGumpTextContext(context), maximum | 0); output.width = width; output.height = height; } });
        this.#entries = new ClientTextEntryControls(root, { ...shared, entryState, fonts,
            drawBackground: (token) => this.#active().drawBackground(token), drawText: (...args) => this.#active().drawText(...args), fill: (...args) => this.#active().fill(...args),
            recipient: (text) => this.#recipient(text) });
        // 로그 자식이 빌려 쓸 Client 장기 공유 버퍼를 한 번 만든다(이미 있으면 같은 토큰을 쓴다).
        try { parts.buffers.constructBorrowed(SHARED_LOG, parts.globals.log, LOG_LINES); } catch (error) { if (!/중복/u.test(error.message)) throw error; }
        this.#geometry = new HudChatGeometry({
            chat: (token) => { const node = root.node(token); return node instanceof HudChatNode ? node : null; }, node: (token) => root.node(token), baseAttach: (...args) => root.attachBase(...args),
            push: (token) => root.tree().push(token), attach: (...args) => root.attach(...args), bottomIndex: (token) => Number(parts.scrolling.atBottom(token)),
            requestBottom: (token, extra) => parts.scrolling.requestBottom(token, extra | 0), challenge: () => this.#read(0x594fc8), current: () => parts.globals.window.current,
        });
        this.#window = new ChatWindow(parts.globals.window, this.#hooks());
    }
    // 지금 출력 중인 입력창 출력 장치이다(출력 밖에서 불리면 거부).
    #output = null;
    #active() { if (!this.#output) throw new Error('채팅 창의 현재 출력 표면 누락'); return this.#output; }
    // 입력 가능한 수신자 접두인지 장치에 묻는다(없으면 수신자 없음).
    #recipient(text) { return typeof this.#devices.recipient === 'function' ? Boolean(this.#devices.recipient(text)) : false; }
    // 원본 `ChatRead` 전역 표: PE 의 사건·팔레트 상수, Client 의 창 상태, 그 밖의 현재 값은 장치에 묻는다. (2026-11-22 WB04-E1169)
    #read(address) {
        const globals = this.#parts.globals, window = globals.window, colors = this.#screen.colors();
        if (EVENT_GLOBALS.has(address)) return EVENT_GLOBALS.get(address);
        if (PALETTE_GLOBALS.has(address)) return colors.named[PALETTE_GLOBALS.get(address)].index >>> 0;
        switch (address) {
        case 0x540424: return 9;
        case 0x548740: return window.current >>> 0;
        case 0x548744: return window.registered >>> 0;
        case 0x54da91: return window.completionReady >>> 0;
        case 0x541348: return this.#game.types().length >>> 0;
        case 0x563e38: return this.#root.createState().entries[0] >>> 0;
        default: {
            const value = typeof this.#devices.read === 'function' ? this.#devices.read(address) : undefined;
            if (value === undefined) throw new Error(`채팅 창의 미연결 전역: ${address.toString(16)}`);
            return value >>> 0;
        }
        }
    }
    // 번역·확장 문자가 든 문자열을 원본 표시 부호화 바이트(NUL 종결 64바이트)로 만든다. 63바이트를 넘으면 완전한 문자 경계에서 자른다. (2026-11-22 WB04-E1169)
    #label(text) {
        const encoded = displayTextCodec.encode(text), label = new Uint8Array(LABEL_BYTES); let length = Math.min(encoded.length, LABEL_BYTES - 1);
        // 네 바이트 확장 문자의 중간에서 잘리지 않게 완전한 문자 경계까지만 복사한다.
        for (let at = 0; at < length;) { const step = encoded[at] === 0x1f && at + 4 <= encoded.length ? 4 : 1; if (at + step > length) { length = at; break; } at += step; }
        label.set(encoded.subarray(0, length)); return label;
    }
    // 입력줄·돌 버튼·배경에 현재 출력 표면을 빌려 주는 그리기 처리기를 등록한다. (2026-11-22 WB04-E1169)
    #bindEntryDraw(token) {
        const root = this.#root;
        root.bindDraw(token, (current, target, clip) => {
            const previous = this.#output; this.#output = createClientTextOutput(root, this.#fonts, { draw: (value) => this.#parts.background.draw(value, target, clip) }, target, this.#glyphColors);
            try { this.#entries.backend().draw(current); } finally { this.#output = previous; }
        });
    }
    // 원본 `ChatWindow` 가 쓰는 실제 장치 묶음이다(cpppj `RawMenuControls::BindChat`). (2026-11-22 WB04-E1169)
    #hooks() {
        const root = this.#root, devices = this.#devices, parts = this.#parts, scrolling = parts.scrolling, menu = parts.menu, colors = () => this.#screen.colors();
        const types = () => this.#game.types();
        const node = (token) => root.node(token);
        return {
            chat: (token) => { const value = node(token); return value instanceof HudChatNode ? value : null; }, base: node,
            console: (token) => { const value = node(token); return value instanceof ConsoleGumpNode ? value : null; },
            text: (token) => { const value = node(token); return value instanceof ScrollingTextGumpNode ? value : null; },
            buffer: (token) => this.#entries.storage().find(token), entry: this.#entries.backend(), geometry: this.#geometry, global: (address) => this.#read(address),
            allocate: (bytes) => {
                if (bytes === 0x128) { const token = this.#entries.allocate(true); this.#bindEntryDraw(token); return token; }
                if (bytes === 0xd8) return parts.scrolling.allocate();
                if (bytes === 0x170) { const token = this.#stones.allocate(); root.bindDraw(token, (current, target) => this.#stones.backend().draw(current, root.collection(), createMixedGumpTextDrawer(this.#fonts, target, this.#glyphColors), target)); return token; }
                if (bytes === 0xb8) {
                    const value = new GumpBackgroundNode(); let token = 0;
                    token = root.allocateDerived(value, (event, first, second) => devices.baseHandle(token, event, first, second), (current, flags) => root.lifetime().deleteScalar(current, flags), null, () => {});
                    root.bindDraw(token, (current, target, clip) => parts.background.draw(current, target, clip)); this.#owned.add(token); return token;
                }
                if (bytes === 0x13c) {
                    const value = new HudChatNode(); let token = 0;
                    token = root.allocateDerived(value, (event, first, second) => this.#window.handle(token, event, first, second, (event & 0x03ffffff) === 0x02000115 ? this.#parts.globals.strings.get(second >>> 0) : undefined),
                        (current, flags) => { this.#deleteFlags.set(current, flags); try { this.#window.destruct(current); } finally { this.#deleteFlags.delete(current); } return current; },
                        (current, x, y, parent, mode) => this.#window.attach(current, x, y, parent, mode));
                    root.bindDraw(token, () => {}); this.#owned.add(token); return token;
                }
                throw new RangeError('채팅 창 실제 할당 크기 오류');
            },
            constructBase: (token, width, height) => root.factory().construct(token, 0, width, height, 1),
            constructHelper: (token, label, color) => this.#stones.construct(token, 0, 0, this.#label(label), color, null), fitHelper: (token) => this.#stones.backend().fitToText(token),
            title: () => {}, help: () => {}, translate: (text) => devices.translate(text),
            measure: (token, text) => this.#measure(Uint8Array.from([...displayTextCodec.encode(text), 0]), decodeGumpTextContext(node(token).textContext), -1),
            constructBackground: (token, width, height, flags) => parts.background.construct(token, width, height, flags, 0, 0, 1),
            constructScrolling: (token, width, height, font, color) => scrolling.construct(token, width, height, font, color),
            replaceBuffer: (token, buffer, owns) => scrolling.replaceBuffer(token, buffer, owns), sharedBuffer: () => SHARED_LOG,
            setSlider: (token, value) => scrolling.setSlider(token, value & 1), setScrollable: (token, value) => scrolling.setScrollable(token, value),
            attach: (...args) => root.attach(...args), push: (token) => root.tree().push(token), destructBase: (token) => root.lifetime().deleteScalar(token, this.#deleteFlags.get(token) ?? 0),
            registerCommand: (name, callback, flag) => this.#dot.register(Uint8Array.from([...name].map((letter) => letter.charCodeAt(0)).concat(0)), callback, flag, { copy: copyDotCommandName }),
            bufferAppendCallback: (buffer, callback) => { if (buffer !== SHARED_LOG || callback !== 0x42f590) throw new Error('채팅 창 공유 로그 callback 불일치'); },
            frameCount: (type) => { const list = types(); return type < 70 || type - 70 >= list.length ? 0 : list[type - 70].definition.clusters.length; },
            frameHeight: (type, frame) => { const asset = types()[type - 70], header = this.#game.shapes.squidHeader(asset.block, frame); return (((header[0x1a] | (header[0x1b] << 8)) << 16) >> 16); },
            requestScroll: (token, value) => scrolling.requestScroll(token, value), relativeScroll: (token, value) => scrolling.requestRelativeScroll(token, value), requestBottom: (token) => scrolling.requestBottom(token),
            atBottom: (token) => scrolling.atBottom(token), appendLog: (token, text) => scrolling.style(token, text),
            baseHandle: (token, event, first, second) => devices.baseHandle(token, event, first, second), bubble: (...args) => root.events().bubble(...args),
            recipient: (text) => this.#recipient(text),
            sendChat: (manager, text) => { if (typeof devices.sendChat === 'function') devices.sendChat(manager, text); else this.appendLog(text); },
            resolve: (record) => (typeof devices.resolve === 'function' ? Boolean(devices.resolve(record)) : false),
            log: (level, format, value) => { if (typeof devices.log === 'function') devices.log(level, format, value); else console.warn(`Netstorm: ${format.replace('%d', String(value))}`); },
            config: (key) => devices.config(key),
            menuBegin: () => menu.beginVertical(0), menuFinalize: () => { menu.current(); }, menuTitle: (text) => menu.title(text),
            menuItem: (label, command, payload) => menu.entry(label, command, payload === null ? 0 : parts.globals.store(payload), true, true, false),
            menuChecked: (checked, label, command, payload) => menu.checked(checked !== 0, label, command, parts.globals.store(payload), true, true),
            menuTake: () => menu.takeCurrent(), menuSubmenu: (label, child) => menu.submenu(label, child, true, false),
            menuOpen: (x, y, parent) => { menu.attach({ x: x | 0, y: y | 0 }, parent); devices.changed(); },
            assertion: (line) => { throw new Error(`ChatWindow 원본 진단: ${line}`); },
        };
    }
    // 채팅 창 root 를 만든다(Uber 루트의 `createChat`). 이미 있는 root 는 새로 만들지 않고 거부한다. (2026-11-22 WB04-E1169)
    create() { const token = this.#window.create(); this.#devices.changed(); return token; }
    // 현재 root 토큰이다(없으면 0).
    current() { return this.#parts.globals.window.current >>> 0; }
    // HUD 채팅 구분선 전환의 도우미 표시(원본 `SetHelperVisible`)와 입력줄 표시(`SetInputVisible`)이다. 아직 만들어지지 않았으면 아무것도 하지 않는다. (2026-11-22 WB04-E1169)
    setVisible(visible) { const token = this.current(); if (token) { this.#geometry.setHelperVisible(token, visible ? 1 : 0); this.#devices.changed(); } }
    setInput(visible) { const token = this.current(); if (token) { this.#geometry.setInputVisible(token, visible ? 1 : 0); this.#devices.changed(); } }
    // 현재 입력줄의 수신자 접두에 그 이름이 들어 있는지이다(원본 `ContainsRecipient`, Player 메뉴의 Whisper To 체크 상태). 창이 없으면 false 이다. (2026-11-22 WB04-E1169)
    containsRecipient(name) { const token = this.current(); return token ? this.#window.containsRecipient(token, name) : false; }
    // 입력줄이 지금 보이는지이다(없으면 false). 키보드 경로가 입력줄에 키를 보낼지 판단한다.
    inputVisible() { return this.current() ? Boolean(this.#geometry.inputVisible()) : false; }
    // 로그에 한 줄을 더한다(원본 `AppendLog`, 맨 아래를 보고 있었다면 맨 아래를 유지). 창이 없으면 무시한다. (2026-11-22 WB04-E1169)
    appendLog(text, context = []) {
        const token = this.current(); if (!token) return;
        // 원본 메시지는 줄 끝 개행을 포함한다. 없는 문자열은 한 줄로 완성되도록 개행을 더한다.
        const line = String(text), complete = line.endsWith(NEWLINE) ? line : line + NEWLINE; this.#window.appendLog(token, complete, context); this.#devices.changed();
    }
    // 현재 입력줄·로그를 포함한 창 상태 진단이다(시험용).
    report() {
        const token = this.current(), node = token ? this.#root.node(token) : null; if (!node) return null;
        const child = (field) => { const value = node[field]; return value ? { token: value, flags: this.#root.node(value)?.flags ?? null, bounds: this.#root.collection().bounds(value) } : null; };
        return { token, flags: node.flags, helperVisible: node.helperVisible, bounds: this.#root.collection().bounds(token), console: child('console'), text: child('text'), helper: child('helper'), background: child('background') };
    }
    // 열린 창과 소유 객체를 정리한다(미션을 떠날 때). 창 상태는 원본처럼 root 소멸에서 0 이 된다. (2026-11-22 WB04-E1169)
    close() {
        const root = this.#root, token = this.current();
        if (token && root.node(token) && !(root.node(token).flags & 0x10000)) root.lifetime().kill(token);
        root.lifetime().cleanup();
        // 부모 삭제가 자식을 반납하므로 각 토큰의 현재 상태를 다시 확인한다.
        for (const owned of [...this.#owned]) {
            const value = root.node(owned); if (!value) { this.#owned.delete(owned); continue; }
            if (root.createState().entries[value.id] === owned) root.remove(owned); else root.discardUnregistered(owned);
            this.#owned.delete(owned);
        }
        this.#entries.close(); this.#stones.close(); this.#parts.globals.window.current = 0;
        // 미션마다 새로 만든 스타일 저장소가 빌렸던 공유 로그 버퍼를 반납하고 Client 로그 상태를 새로 바꿔 다음 미션이 다시 빌릴 수 있게 한다.
        try { this.#parts.buffers.destroy(SHARED_LOG); } catch (error) { if (!/등록 토큰 없음/u.test(error.message)) throw error; }
        this.#parts.globals.resetLog();
    }
}
