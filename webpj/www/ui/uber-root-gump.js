// 원본 Uber 루트의 실제 자식 필드와 현재 HUD 노드 뷰를 옮긴다(cpppj client/UberRootGump.cpp/.h, 10.78).
// 사용: const view = uberRootDockView(token, { node, base }); new HudDock(view, globals, color, hooks).
import { GumpCreateNode } from './gump-create.js';
import { HudDividerNode, HudDockPart } from './hud-dock.js';

// 원본의 Uber 가상 표와 현재 배경·화면·제목 전역 주소다.
const UBER_VTABLE = 0x0051503c, BACKGROUND_TYPE = 0x005ca920, BACKGROUND_FRAME = 0x005ca924, PANEL_FRAME = 0x005ca928;
const WIDTH = 0x005c78f0, HEIGHT = 0x005c78f4, FORT_GUMP = 0x005411c8, TITLE = 0x00565e70;

// 실제로 도달한 하위 장치만 요구한다.
function need(hook) { if (typeof hook !== 'function') throw new Error('Uber 루트의 실제 하위 몸체 미연결'); return hook; }

// 원본 0xc8BYTE Uber 파생 노드의 10개 DWORD 자식·예약 필드이다.
export class UberRootNode extends GumpCreateNode {
    // 생성자가 보존하는 예약 필드와 현재 자식 토큰이다.
    vertical = 0; minimapDivider = 0; chatDivider = 0; minimap = 0; production = 0;
    reservedB4 = 0; chat = 0; reservedBC = 0; knowledge = 0; message = 0;
}

// 같은 실제 파생/기본 주소를 매 조회마다 확인하고 현재 필드를 직접 빌린다.
export function uberRootDockView(token, hooks) {
    token >>>= 0;
    const current = () => {
        if (!token || typeof hooks?.node !== 'function' || typeof hooks?.base !== 'function') throw new Error('Uber 루트 실제 저장소 누락');
        const derived = hooks.node(token), base = hooks.base(token);
        if (!(derived instanceof UberRootNode) || derived !== base) throw new Error('Uber 루트 파생 저장소 불일치');
        return derived;
    };
    current();
    return (part) => {
        const node = current();
        switch (part) {
        case HudDockPart.root: return token;
        case HudDockPart.vertical: return node.vertical;
        case HudDockPart.minimapDivider: return node.minimapDivider;
        case HudDockPart.chatDivider: return node.chatDivider;
        case HudDockPart.minimap: return node.minimap;
        case HudDockPart.production: return node.production;
        case HudDockPart.chat: return node.chat;
        case HudDockPart.knowledge: return node.knowledge;
        default: throw new RangeError('Uber 현재 HUD 필드 종류 오류');
        }
    };
}

// 원본 전체 Uber 생성·교체 부모이다. 하위 공장과 UI 장치는 호출자가 같은 현재 저장소에 연결한다.
export class UberRootGump {
    // 공통 등록표와 현재 실제 하위 장치이다.
    #create; #hooks;
    // 10.78의 공통 등록표를 빌리며 다른 판본은 거부한다.
    constructor(create, hooks, edition = 'patch1078') {
        if (edition !== 'patch1078') throw new TypeError('Uber 루트 10.78 판본 필요');
        this.#create = create; this.#hooks = hooks;
    }
    // 실제 기본 노드만 현재 토큰에서 조회한다.
    #base(token) { const value = token ? need(this.#hooks.base)(token) : null; if (!value) throw new Error('Uber 루트 실제 기본노드 없음'); return value; }
    // 파생과 기본의 같은 실제 주소를 요구한다.
    #node(token) { const value = token ? need(this.#hooks.node)(token) : null; if (!(value instanceof UberRootNode) || value !== this.#base(token)) throw new Error('Uber 루트 파생 저장소 불일치'); return value; }
    // 구분선도 같은 실제 기본 주소를 요구한다.
    #divider(token) { const value = token ? need(this.#hooks.divider)(token) : null; if (!(value instanceof HudDividerNode) || value !== this.#base(token)) throw new Error('Uber 구분선 파생 저장소 불일치'); return value; }
    // 현재 원본 DWORD를 소비 위치마다 다시 읽는다.
    #read(address) { return need(this.#hooks.read)(address) >>> 0; }
    // 원본 4437f0은 빈 본문이며 관찰기만 선택적으로 호출한다.
    #empty() { this.#hooks.emptyObserver?.(); }
    // 실제 자식의 가상 부착에 원본 mode 0을 전달한다.
    #attach(child, x, y, parent) { this.#base(child); need(this.#hooks.attach)(child, Math.fround(x), Math.fround(y), parent, 0); }
    // 고정 this의 현재 HUD 필드 뷰를 빌린다.
    dockView(token) { return uberRootDockView(token, this.#hooks); }
    // 각 콜백 뒤 현재 Uber 필드를 다시 읽으며 10개 관계와 세 제한을 쓴다.
    #layout(token) {
        const registrations = [['vertical', 'minimapDivider', 1], ['vertical', 'chatDivider', 4], ['vertical', 'minimap', 1], ['vertical', 'production', 1],
            ['vertical', 'knowledge', 1], ['vertical', 'chat', 4], ['minimapDivider', 'minimap', 8], ['minimapDivider', 'production', 2],
            ['minimapDivider', 'knowledge', 2], ['chatDivider', 'chat', 8]];
        // 원문 순서대로 현재 두 자식 포인터를 다시 조회한다.
        for (const [master, slave, flags] of registrations) need(this.#hooks.registerSlave)(this.#node(token)[master], this.#node(token)[slave], flags);
        const vertical = this.#divider(this.#node(token).vertical); vertical.minX = 60; vertical.maxX = 130; vertical.axis = (vertical.axis | 4) >>> 0;
        const height = this.#read(HEIGHT), minimap = this.#divider(this.#node(token).minimapDivider);
        minimap.axis = (minimap.axis | 8) >>> 0; minimap.minY = (height - 123) | 0; minimap.maxY = height | 0;
        const chat = this.#divider(this.#node(token).chatDivider); chat.axis = (chat.axis | 8) >>> 0; chat.minY = 25; chat.maxY = (this.#read(HEIGHT) - 25) | 0;
    }
    // 004c7160의 공통 생성·자식 공장·배치·선택 제목 순서를 실행한다.
    construct(token) {
        const h = this.#hooks; this.#node(token); need(h.constructBase)(token, 0, 0, 0, 1); this.#node(token).vtable = UBER_VTABLE;
        const color = this.#read(0x00540451); this.#node(token).command = ((color << 24) >> 24) >>> 0; this.#node(token).flags = (this.#node(token).flags | 0x800) >>> 0;
        need(h.write)(BACKGROUND_TYPE, this.#read(FORT_GUMP)); h.write(BACKGROUND_FRAME, 0); h.write(PANEL_FRAME, 2);
        this.#node(token).chatDivider = 0; this.#node(token).minimapDivider = 0; this.#node(token).vertical = 0; this.#node(token).chat = 0;
        let next = need(h.allocate)(0x11c) >>> 0;
        if (next) { const frame = this.#read(0x00542664), type = this.#read(BACKGROUND_TYPE), height = this.#read(HEIGHT) | 0; need(h.constructDivider)(next, 0, 5, height, 0x03400000, type, frame); }
        this.#node(token).vertical = next; this.#attach(next, 80, 0, token);
        next = h.allocate(0x11c) >>> 0;
        if (next) { const frame = this.#read(0x00542664), type = this.#read(BACKGROUND_TYPE); need(h.constructDivider)(next, 0, 80, 5, 0x02a00000, type, frame); }
        this.#node(token).minimapDivider = next; this.#attach(next, 0, 400, token);
        next = h.allocate(0x11c) >>> 0;
        if (next) { const frame = this.#read(BACKGROUND_FRAME), type = this.#read(BACKGROUND_TYPE), width = this.#read(WIDTH) | 0; need(h.constructDivider)(next, 18, width, 5, 0x02a00000, type, frame); }
        this.#node(token).chatDivider = next; this.#attach(next, 85, (this.#read(HEIGHT) - 96) | 0, token);
        this.#empty(); next = need(h.createMinimap)() >>> 0; this.#node(token).minimap = next;
        let base = this.#base(this.#node(token).minimap); base.flags = (base.flags | 0x20000) >>> 0;
        let child = this.#node(token).minimap; base = this.#base(child); base.baseHeight = 128; base.height = Math.fround(base.scale * 128);
        child = this.#node(token).minimap; base = this.#base(child); base.baseWidth = 128; base.width = Math.fround(base.scale * 128);
        this.#attach(this.#node(token).minimap, 0, 0, token);
        next = need(h.createProduction)() >>> 0; this.#node(token).production = next; base = this.#base(this.#node(token).production); base.flags = (base.flags | 0x20000) >>> 0; this.#attach(this.#node(token).production, 0, 0, token);
        next = need(h.createKnowledge)() >>> 0; this.#node(token).knowledge = next; child = this.#node(token).knowledge;
        let size = this.#read(WIDTH) | 0; base = this.#base(child); base.baseWidth = Math.fround(size); base.width = Math.fround(size * base.scale);
        child = this.#node(token).knowledge; size = this.#read(HEIGHT) | 0; base = this.#base(child); base.baseHeight = Math.fround(size); base.height = Math.fround(size * base.scale);
        base = this.#base(this.#node(token).knowledge); base.flags = (base.flags | 0x20000) >>> 0; this.#attach(this.#node(token).knowledge, 0, 0, token);
        need(h.type)(this.#read(FORT_GUMP)); next = need(h.createChat)() >>> 0; this.#node(token).chat = next; base = this.#base(this.#node(token).chat); base.flags = (base.flags | 0x20000) >>> 0;
        const label = need(h.translate)('Chat Window'); need(h.help)(this.#node(token).chat, label); need(h.helpWord)(this.#node(token).chat, 'chatView');
        this.#empty(); this.#attach(this.#node(token).chat, 0, 0, token); this.#empty(); need(h.refreshTechnology)(); this.#layout(token);
        // 각 현재 구분선을 정리 뒤 다시 조회해 같은 좌표로 이동한다.
        for (const field of ['vertical', 'minimapDivider', 'chatDivider']) { need(h.cleanup)(); child = this.#node(token)[field]; need(h.move)(child, this.#base(child).x, this.#base(child).y, 0); }
        need(h.cleanup)();
        if ((this.#read(TITLE) & 0xff) !== 0) {
            next = need(h.allocate)(0xb0) >>> 0; if (next) need(h.constructMessage)(next, need(h.savedMessage)()); this.#node(token).message = next;
            const x = Math.trunc((this.#read(0x00531860) | 0) / 2) - this.#base(next).baseWidth * 0.5;
            const y = Math.trunc((this.#read(0x00531864) | 0) / 2) - this.#base(next).baseHeight * 0.5; this.#attach(next, x, y, token);
            if (!need(h.equalText)(need(h.savedMessage)(), 'defeat') && !h.equalText(h.savedMessage(), 'titleMenu') && !h.equalText(h.savedMessage(), 'victory')) {
                const expression = [0xfffffffe, this.#read(0x00540cec), 0xfffffffe, this.#read(0x00540cf8), 0xfffffffd, h.savedMessage(), 0xfffffffc, 0x0051501c, 0];
                const condition = need(h.formatMessage)(expression); if (need(h.testMessage)(condition)) need(h.visitMessage)(h.savedMessage());
                need(h.messageEvent)(this.#node(token).message, this.#read(0x00542a04), 0, 1); need(h.overlay)(this.#node(token).message, true); need(h.setDepth)(this.#node(token).message, 0xffffff9c);
            }
        }
        return token;
    }
    // 004c77c0: 새 루트를 만든 뒤 이전 직계 자식을 원문 순서로 옮긴다.
    replace() {
        const h = this.#hooks; this.#empty(); const allocated = need(h.allocate)(0xc8) >>> 0, token = allocated ? this.construct(allocated) : 0;
        const node = this.#base(token); if (node.id >= this.#create.entries.length) throw new RangeError('Uber 임시 등록ID 범위 오류');
        this.#create.entries[node.id] = 0; this.#create.collection.root = this.#create.entries[0]; node.id = 0;
        const root = this.#create.entries[0], visited = new Set();
        // Push 이전 next를 저장하여 각 기존 직계 자식만 한 번 옮긴다.
        for (let child = this.#base(root).child; child;) {
            if (visited.has(child)) throw new Error('Uber 이전루트 형제 순환'); visited.add(child); const next = this.#base(child).next;
            need(h.push)(child); this.#attach(child, this.#base(child).x, this.#base(child).y, token); child = next;
        }
        need(h.replaceRoot)(token);
    }
    // 004c7880: 이전 루트를 삭제·정리한 뒤 새 루트를 공개한다.
    recreate() {
        const h = this.#hooks; if (this.#create.entries[0]) need(h.kill)(this.#create.entries[0]); need(h.cleanup)();
        const allocated = need(h.allocate)(0xc8) >>> 0, token = allocated ? this.construct(allocated) : 0, node = this.#base(token);
        if (node.id >= this.#create.entries.length) throw new RangeError('Uber 재생성 임시 등록ID 물리 범위');
        this.#create.entries[node.id] = 0; this.#create.collection.root = this.#create.entries[0]; node.id = 0; need(h.replaceRoot)(token);
    }
}
