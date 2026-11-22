// 실제 미션 Client 에 송금 숫자 입력 창(Send X Storm Power)을 조립한다. 공통 장식 틀·숫자 입력창·테두리·제목 글자·OK/Cancel 돌 버튼을 같은 Gump 트리에 만들고 모달로 올린다(cpppj `RawMenuControls::ShowSendMoneyDialog`/`RawMenuSaveFort.inc`).
// 사용: const dialog = new ClientMoneyDialog(root, game, fonts, screen, { text, decoration, background, backgroundType }, devices); const token = dialog.open(대상 owner); dialog.close().
//   devices = { now(), poll(), sound(이름), baseHandle(토큰, 사건, 첫 값, 둘째 값), translate(문자열), confirm(대상 owner, 입력 문자열, 창 vtable) → 0/1 (원본 004c9350 송금 확정), changed() }.
// parts.format 이 참이면 같은 조립으로 채팅 서식 입력 창(`Edit Back/Front Format`, ChatFormatDialog)을 만든다: dialog.openFormat(front, 초기 문자열)로 열고 confirm(1, 입력 문자열, vtable)이 서식 확정(원문 0x4c9600/0x4c9680)을 맡으며 vtable 이 Back/Front 를 가른다. (2026-11-22 WB04-E1179)
// 입력 창은 한 번에 하나만 열고(열려 있으면 거부), 닫히면 입력창·돌 버튼의 소유 객체를 정리한다. 키보드 사건은 앱 입력 프레임이 Gump 입력(`root.input().route`)으로 보내며 이 모듈은 출력·수명·확정만 맡는다.
// (2026-11-22 WB04-E1168: Send X 숫자 입력 창 앱 연결로 추가)
import { ClientTextEntryControls, createMixedGumpTextMeasure } from './client-text-entry-controls.js';
import { createClientTextOutput, createMixedGumpTextDrawer } from './client-text-entry-draw.js';
import { ClientStoneControls } from './client-stone-controls.js';
import { ButtonGumpState } from './button-gump.js';
import { StoneButtonGumpState } from './stone-button-gump.js';
import { ScrollGumpState } from './scroll-gump.js';
import { ScrollGumpWidgetState } from './scroll-gump-widgets.js';
import { TextEntryGumpNode, TextEntryGumpState } from './text-entry-gump.js';
import { SaveFortNameDialog, SaveFortNameDialogNode } from './save-fort-name-dialog.js';
import { ChatFormatDialog } from './chat-format-dialog.js';
import { GumpBackgroundNode } from './gump-background-create.js';
import { GumpBackgroundAssets } from '../render/gump-background-draw.js';
import { displayTextCodec } from '../assets/display-text.js';
import { GumpGlyphColorState, decodeGumpTextContext } from '../render/gump-text-draw.js';

// 입력 테두리 배경 flags(원문 0x03e40000)와 제목 글자 문맥 [flags 2(그림자), font 3, 색, 0, 0, 0, 1, 1]의 앞 두 값이다. (2026-11-22 WB04-E1168)
const BORDER_FLAGS = 0x03e40000;
const TITLE_FLAGS = 2;
const TITLE_FONT = 3;
// Gump 버튼을 누를 때 나는 소리 이름이다(원본 UberGump 의 `button.wav`). 버튼 본체는 인자 없이 소리를 요청하므로 이 이름을 여기서 채운다. (2026-11-22 WB04-E1168)
const BUTTON_SOUND = 'button.wav';
// 돌 버튼 크기(70×18)와 라벨 저장소 크기(원문 64바이트)이다.
const STONE_WIDTH = 70;
const STONE_HEIGHT = 18;
const LABEL_BYTES = 64;

// 현재 모달(Gump dialog)의 하위 노드 중 깊이가 가장 낮은 노드를 원문 순서대로 찾는다(원문 464090, 이전 창 위에 새 모달을 쌓을 때 사용). (2026-11-22 WB04-E1168)
function lowestModalDepth(root, token, state, active) {
    if (active.includes(token)) throw new Error('모달 최소 깊이 트리 순환'); active.push(token);
    const seen = new Set();
    // 보이고 지워지지 않은 비void 자식과 그 깊이를 원문 순서(자식 먼저 비교 뒤 재귀)로 읽는다.
    for (let child = root.node(token).child; child; child = root.node(child).next) {
        if (seen.has(child)) throw new Error('모달 최소 깊이 형제 순환'); seen.add(child);
        const node = root.node(child);
        if ((node.flags & 1) && !(node.flags & 0x10008)) { if ((node.depth | 0) < state.minimum) { state.minimum = node.depth | 0; state.lowest = child; } lowestModalDepth(root, child, state, active); }
    }
    const depth = root.node(token).depth | 0; if (depth < state.minimum) { state.minimum = depth; state.lowest = token; }
    active.pop();
}

// 모달 등록이다(원문 00466600): dialog 표에 올리고, 이미 다른 모달이 있으면 이전 창 하위의 가장 낮은 깊이 바로 앞으로 깊이를 맞춘다. 사용: setModal(root, token). (2026-11-22 WB04-E1168)
export function setModal(root, token) {
    if (root.input().addDialog(token) && root.lifetimeState().dialogCount > 1) {
        const state = { minimum: 10000, lowest: 0 }; lowestModalDepth(root, root.lifetimeState().dialogs[root.lifetimeState().dialogCount - 2], state, []);
        if (!state.lowest) throw new Error('원문 모달 최소 깊이 assertion504');
        root.tree().addDepth(token, ((root.node(state.lowest).depth | 0) - (root.node(token).depth | 0) - 3) >>> 0);
    }
}

// 같은 Client Gump 에 송금 숫자 입력 창을 만들고 수명을 관리한다. (2026-11-22 WB04-E1168)
export class ClientMoneyDialog {
    // 공통 소유자·입력창/돌 버튼 소유자·원본 입력 창 본체와 지금 열린 창, 직접 할당한 노드 토큰이다.
    #root; #entries; #stones; #dialog; #devices; #current = 0; #owned = new Set();
    // 지금 출력 중인 현재 표면과 글자 색 상태, 장치 묶음이다.
    #glyphColors = new GumpGlyphColorState(); #fonts; #screen; #text; #decoration; #background; #backgroundType; #output = null;
    // 같은 Client 의 Gump 루트·자산·글꼴·팔레트와 이미 조립된 글자·장식·배경 공장을 빌린다.
    constructor(root, game, fonts, screen, parts, devices) {
        if (!root || !game || !fonts || !screen || !parts?.text || !parts?.decoration || !parts?.background || !Number.isInteger(parts.backgroundType) || !devices ||
            ['now', 'poll', 'sound', 'baseHandle', 'translate', 'confirm', 'changed'].some((name) => typeof devices[name] !== 'function')) throw new TypeError('송금 숫자 창의 실제 장치 누락');
        this.#root = root; this.#fonts = fonts; this.#screen = screen; this.#text = parts.text; this.#decoration = parts.decoration; this.#background = parts.background; this.#backgroundType = parts.backgroundType; this.#devices = devices;
        const colors = screen.colors(), assets = new GumpBackgroundAssets(game), measureMixed = createMixedGumpTextMeasure(fonts);
        // 입력창·돌 버튼 상태는 생산 메뉴와 같은 원본 전역 값이다.
        const buttonState = new ButtonGumpState(), stoneState = new StoneButtonGumpState(), scrollState = new ScrollGumpState(), widgetState = new ScrollGumpWidgetState(), entryState = new TextEntryGumpState();
        Object.assign(buttonState, { valueEvent: 0x02000070, clickEvent: 0x0200002c, rightEvent: 0x02000012, repeatEvent: 0x0200002f, baseColor: 56 });
        Object.assign(scrollState, { clickEvent: 0x0200002c, repeatEvent: 0x0200002f, stepEvent: 0x0200002e, changeEvent: 0x02000036 });
        stoneState.color = 49; stoneState.palette = { black: colors.basic[0], white: colors.basic[7], edge: colors.basic[8] }; stoneState.backgroundType = parts.backgroundType; stoneState.backgroundFrame = 1; widgetState.defaultType = parts.backgroundType;
        // 원본 SetPalette: 갈색은 이름 색 9, 검정은 기본 0, 노랑은 기본 5 이다.
        if (colors.named.length <= 9) throw new Error('송금 숫자 창 실제 갈색 팔레트 없음');
        entryState.brown = colors.named[9].index; entryState.black = colors.basic[0]; entryState.yellow = colors.basic[5];
        const shared = { buttonState, stoneState, scrollState, widgetState, backgroundColor: () => 35, type: (type) => assets.type(type), frame: (...args) => assets.frame(...args),
            output: { fonts, background: parts.background }, now: devices.now, poll: devices.poll, sound: () => devices.sound(BUTTON_SOUND), baseEvent: devices.baseHandle };
        // 돌 버튼 글자 측정은 현재 문맥으로 원본 비트맵 글꼴과 확장(한국어 등) 글꼴을 함께 잰다.
        this.#stones = new ClientStoneControls(root, { ...shared, measure: (token, label, context, output, maximum) => { const [width, height] = measureMixed(label, decodeGumpTextContext(context), maximum | 0); output.width = width; output.height = height; } });
        // 입력창 출력은 출력 시점의 표면으로 매번 새로 만든다(앱의 Gump 출력은 항목마다 표면을 넘긴다).
        this.#entries = new ClientTextEntryControls(root, { ...shared, entryState, fonts,
            drawBackground: (token) => this.#active().drawBackground(token), drawText: (...args) => this.#active().drawText(...args), fill: (...args) => this.#active().fill(...args) });
        // 서식 창은 송금 숫자 창과 같은 부모 노드·장치를 쓰는 형제 창이다. (2026-11-22 WB04-E1179)
        this.#dialog = parts.format ? new ChatFormatDialog(root.createState(), this.#hooks()) : new SaveFortNameDialog(root.createState(), this.#hooks());
    }
    // 지금 출력 중인 입력창 출력 장치이다(출력 밖에서 불리면 거부).
    #active() { if (!this.#output) throw new Error('송금 숫자 창의 현재 출력 표면 누락'); return this.#output; }
    // 직접 할당한 노드 토큰을 소유 목록에 둔다.
    #own(token) { this.#owned.add(token); return token; }
    // 원본 입력 창 본체가 쓰는 실제 장치 묶음이다(cpppj `RawMenuControls::BindSaveFortDialog`).
    #hooks() {
        const root = this.#root, devices = this.#devices, colors = () => this.#screen.colors();
        return {
            node: (token) => { const node = root.node(token); return node instanceof SaveFortNameDialogNode ? node : null; }, base: (token) => root.node(token),
            entry: (token) => { const node = root.node(token); return node instanceof TextEntryGumpNode ? node : null; },
            allocate: (bytes) => {
                if (bytes === 0x110) {
                    const token = this.#entries.allocate(false);
                    root.bindDraw(token, (current, target, clip) => {
                        const previous = this.#output; this.#output = createClientTextOutput(root, this.#fonts, { draw: (value) => this.#background.draw(value, target, clip) }, target, this.#glyphColors);
                        try { this.#entries.backend().draw(current); } finally { this.#output = previous; }
                    });
                    return token;
                }
                if (bytes === 0x170) { const token = this.#stones.allocate(); root.bindDraw(token, (current, target) => this.#stones.backend().draw(current, root.collection(), createMixedGumpTextDrawer(this.#fonts, target, this.#glyphColors), target)); return token; }
                if (bytes === 0xd8) return this.#own(this.#decoration.allocate(SaveFortNameDialogNode, (token, event, first, second) => this.#dialog.handle(token, event, first, second)));
                if (bytes === 0xb8) {
                    const node = new GumpBackgroundNode(); let token = 0;
                    token = root.allocateDerived(node, (event, first, second) => devices.baseHandle(token, event, first, second), (current, flags) => root.lifetime().deleteScalar(current, flags), null, () => {});
                    root.bindDraw(token, (current, target, clip) => this.#background.draw(current, target, clip)); return this.#own(token);
                }
                if (bytes === 0xa4) return this.#own(this.#text.allocate());
                throw new RangeError('송금 숫자 창 실제 할당 크기 오류');
            },
            decoration: (token, width, height) => this.#decoration.construct(token, width, height),
            textEntry: (token, width, height, text) => this.#entries.construct(token, width, height, text, 0, 0, colors().basic[5]),
            excluded: (token, text) => this.#entries.backend().setExcluded(token, text),
            // 입력창 가로 표시 필드(원문 00451c60)이다: 이미 붙은 입력창이면 가상 Push 뒤 같은 위치로 재부착한다. (2026-11-22 WB04-E1179)
            horizontal: (token, value) => this.#entries.backend().setHorizontal(token, value),
            background: (token, width, height) => this.#background.construct(token, width, height, BORDER_FLAGS, this.#backgroundType, 0, 1),
            // 제목은 창 기본 폭의 0.8 에 높이 20, 그림자 글꼴 3·밝은 글자 색의 한 조각이다.
            title: (token, text, width) => { this.#text.construct(token, width, 20, TITLE_FONT, colors().basic[7]); this.#root.node(token).textContext[0] = TITLE_FLAGS; this.#text.style(token, text); },
            stone: (token, text) => this.#stones.construct(token, STONE_WIDTH, STONE_HEIGHT, this.#label(text), colors().named[8].index, null),
            fitTitle: (token) => this.#text.fit(token), fitStone: (token) => this.#stones.backend().fitToText(token), push: (token) => root.tree().push(token),
            attach: (token, x, y, parent, mode) => root.attach(token, x, y, parent, mode), depth: (token, delta) => root.tree().addDepth(token, delta >>> 0),
            modal: (token) => { setModal(root, token); devices.changed(); }, kill: (token) => { root.lifetime().kill(token); this.#current = 0; devices.changed(); },
            accept: (token) => {
                const node = root.node(token), entry = root.node(node.entry); const buffer = this.#entries.storage().find(entry.buffer); if (!buffer) throw new Error('송금 숫자 창의 실제 문자 저장소 없음');
                const target = node.fortMode >>> 0; return devices.confirm(target, buffer.text(), node.vtable >>> 0) ? 1 : 0;
            },
            cancel: (token) => (root.node(token).canCancel ? 1 : 0),
            baseHandle: (token, event, first, second) => devices.baseHandle(token, event, first, second), translate: (text) => devices.translate(text),
        };
    }
    // 돌 버튼 라벨을 원본 64바이트 NUL 종결 바이트 배열로 만든다. 번역된 한국어·일본어 문자는 게임 표시 부호화(선두 0x1f 네 바이트)로 넣어 입력창·Player 이름과 같은 혼합 글자 출력을 쓴다. 63바이트를 넘으면 자른다. (2026-11-22 WB04-E1168)
    #label(text) {
        const encoded = displayTextCodec.encode(text), label = new Uint8Array(LABEL_BYTES); let length = Math.min(encoded.length, LABEL_BYTES - 1);
        // 네 바이트 확장 문자의 중간에서 잘리지 않게 완전한 문자 경계까지만 복사한다.
        for (let at = 0; at < length;) { const step = encoded[at] === 0x1f && at + 4 <= encoded.length ? 4 : 1; if (at + step > length) { length = at; break; } at += step; }
        label.set(encoded.subarray(0, length)); return label;
    }
    // 지금 열려 있는 입력 창 토큰이다(없으면 0). 닫힌(지워진) 창은 0 이다.
    current() { const node = this.#current ? this.#root.node(this.#current) : null; if (!node || (node.flags & 0x10000)) { this.#current = 0; return 0; } return this.#current; }
    // 열린 창의 현재 입력 문자열이다(바이트 문자열). 열린 창이 없으면 null 이다. 시험·진단용이다.
    text() {
        const token = this.current(); if (!token) return null;
        const entry = this.#root.node(this.#root.node(token).entry); return this.#entries.storage().find(entry.buffer)?.text() ?? null;
    }
    // 대상 owner 에게 보낼 금액을 묻는 창을 연다. 이미 열려 있으면 거부하고 기존 창 토큰을 돌려준다. 만들다 실패하면 부분 객체를 정리한 뒤 오류를 다시 던진다.
    open(target) {
        const existing = this.current(); if (existing) return existing;
        // 이미 지워진 이전 창의 토큰은 소유 목록에서 뺀다.
        for (const token of [...this.#owned]) if (!this.#root.node(token)) this.#owned.delete(token);
        try { this.#current = this.#dialog.showMoney(target); this.#devices.changed(); return this.#current; }
        catch (error) { this.#current = 0; this.close(); throw error; }
    }
    // 채팅 서식 입력 창을 연다(parts.format 으로 만든 경우만). front 가 참이면 Front, 거짓이면 Back 이며 초기 문자열은 현재 설정 값이다. 이미 열려 있으면 거부하고 기존 창 토큰을 돌려준다. 만들다 실패하면 부분 객체를 정리한 뒤 오류를 다시 던진다. (2026-11-22 WB04-E1179)
    openFormat(front, initialText) {
        if (!(this.#dialog instanceof ChatFormatDialog)) throw new Error('채팅 서식 창이 아닌 대화창에서 서식 열기 요청');
        const existing = this.current(); if (existing) return existing;
        for (const token of [...this.#owned]) if (!this.#root.node(token)) this.#owned.delete(token);
        try { this.#current = this.#dialog.show(front, initialText); this.#devices.changed(); return this.#current; }
        catch (error) { this.#current = 0; this.close(); throw error; }
    }
    // 열린 창과 소유 객체를 정리한다(미션을 떠나거나 부분 생성이 실패한 경우). 이미 정리됐으면 아무것도 하지 않는다.
    close() {
        const root = this.#root, open = this.#current; this.#current = 0;
        if (open && root.node(open) && !(root.node(open).flags & 0x10000)) root.lifetime().kill(open);
        root.lifetime().cleanup();
        // 부모 삭제가 자식을 반납하므로 각 토큰의 현재 상태를 다시 확인한다.
        for (const token of [...this.#owned]) {
            const node = root.node(token); if (!node) { this.#owned.delete(token); continue; }
            if (root.createState().entries[node.id] === token) root.remove(token); else root.discardUnregistered(token);
            this.#owned.delete(token);
        }
        this.#entries.close(); this.#stones.close();
    }
}
