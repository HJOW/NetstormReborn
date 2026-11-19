// 원본 생산 패널의 실제 자식 배치와 선택 분배를 하나의 Client Gump 트리에 조립한다.
import { GumpCreateNode } from './gump-create.js';

// 생산 유지 상태와 자식 토큰은 같은 루트 노드에서 소유한다.
class ProductionPanelNode extends GumpCreateNode {
    // 원본 예산·세 배경·두 격자·구분선과 실제 생산 유지 상태다.
    budget = 0; top = 0; middle = 0; bottom = 0; bridge = 0; teleport = 0; separator = 0; panel = null;
}

// 기존 Session과 원본 구성 요소를 같은 루트의 수명으로 결합한다.
export class ClientProductionPanel {
    // 현재 소유자·실제 장치·전역과 생성한 루트다.
    #root; #session; #parts; #globals; #panel; #token = 0; #used = false;
    // 이미 생성된 생산 유지 상태와 같은 Session 구성 요소를 빌린다.
    constructor(root, session, parts, globals, panel) {
        if (!root || !session?.core || !parts?.grid || !parts?.background || !parts?.divider || !parts?.budget || !parts?.selection || !globals || !panel || typeof parts.technology !== 'function' || typeof parts.changed !== 'function') throw new TypeError('생산 패널의 실제 구성 요소 누락');
        this.#root = root; this.#session = session; this.#parts = parts; this.#globals = globals; this.#panel = panel;
    }
    // 실제 루트와 현재 자식 필드를 호출자에게 빌린다.
    token() { return this.#token; }
    node() { return this.#root.node(this.#token); }
    // 원본 선택 명령과 다리 슬롯 명령만 생산 처리기에 전달한다.
    #handle(event, first, second) {
        const code = event & 0x03ffffff;
        if (code === 0x02000008) return this.#parts.selection.choose(first, second);
        if (code === 0x020000f4) return this.#parts.selection.shortcut(first);
        return 0;
    }
    // 원본 생산 루트의 기본 부착 뒤 예산·하단·두 번째 격자와 깊이를 재배치한다.
    #attach(token, x, y, parent, mode) {
        const root = this.#root, n = this.node(); root.attachBase(token, x, y, parent, mode);
        root.tree().push(n.budget); root.attach(n.budget, 0, 2, token, 2);
        root.tree().push(n.bottom); const bottom = root.node(n.bottom); bottom.baseHeight = n.baseHeight - bottom.y; bottom.height = Math.fround(bottom.baseHeight * bottom.scale); root.attach(n.bottom, bottom.x, bottom.y, 0, 0);
        root.tree().push(n.teleport); const teleport = root.node(n.teleport), middle = root.node(n.middle); teleport.baseHeight = n.baseHeight - teleport.y; teleport.height = Math.fround(teleport.baseHeight * teleport.scale); root.attach(n.teleport, 0, middle.y + middle.baseHeight, token, 0);
        root.node(n.top).depth += 1; root.node(n.budget).depth = root.node(n.top).depth - 1; middle.depth += 1; bottom.depth += 1;
    }
    // 생산 유지가 갱신한 격자만 원본 순서(텔레포트→다리)로 재부착한다.
    refresh(mask) {
        const root = this.#root, n = this.node();
        for (const [token, bit] of [[n.teleport, 1], [n.bridge, 2]]) if (mask & bit) { root.tree().push(token); const item = root.node(token); root.attach(token, item.x, item.y, 0, 0); }
        this.#parts.changed();
    }
    // 같은 원본 80폭·하단 80 예약 크기로 열고 실제 기술 내용물을 연결한다.
    open() {
        if (this.#used) throw new Error('생산 패널 중복 생성'); this.#used = true;
        const root = this.#root, parts = this.#parts, globals = this.#globals, n = new ProductionPanelNode();
        this.#token = root.allocateDerived(n, (event, first, second) => this.#handle(event, first, second),
            (token, flags) => root.lifetime().deleteScalar(token, flags), (token, x, y, parent, mode) => this.#attach(token, x, y, parent, mode), () => {});
        root.factory().construct(this.#token, 0, 0, 0, 1); root.bindDraw(this.#token, () => {});
        n.vtable = 0x5069c0; n.command = 44; n.flags |= 0xe00; n.panel = this.#panel; n.baseWidth = n.width = 40; n.baseHeight = n.height = root.createState().screenHeight;
        globals.policyFirst = 0; globals.policyByte = 0;
        n.budget = parts.budget.create(this.#token); globals.budgetSerial = root.node(n.budget).serial;
        n.top = parts.background.create(40, n.baseHeight, 0x02000000, globals.backgroundType, 4); let child = root.node(n.top);
        child.alignFlags |= 1; child.baseHeight = child.height = 23; root.attach(n.top, 0, 0, this.#token, 0);
        const slots = this.#session.battleOption(1) * 2 + 2; this.#panel.slots = slots;
        n.bridge = parts.grid.create(2, slots / 2, 0x40); child = root.node(n.bridge); child.fixed = 1; child.baseHeight = child.height = slots * 18; child.acceptedCursor = 0x40; child.alignFlags |= 1; child.itemScale = Math.fround(0.63); root.attach(n.bridge, 0, 26, this.#token, 0);
        n.middle = parts.background.create(40, n.baseHeight, 0x02000000, globals.backgroundType, globals.panelFrame); child = root.node(n.middle); child.alignFlags |= 1; child.baseHeight = child.height = slots * 18; root.attach(n.middle, 0, 26, this.#token, 0);
        n.teleport = parts.grid.create(1, 6, 8); child = root.node(n.teleport); child.baseHeight = child.height = 250; child.acceptedCursor = 8; child.alignFlags |= 1; root.attach(n.teleport, 0, 26 + slots * 18, this.#token, 0);
        n.bottom = parts.background.create(40, n.baseHeight, 0x02000000, globals.backgroundType, globals.panelFrame); child = root.node(n.bottom); child.alignFlags |= 1; child.baseHeight = child.height = 250; root.attach(n.bottom, 0, root.node(n.teleport).y, this.#token, 0);
        n.separator = parts.divider.allocate(); parts.divider.construct(n.separator, 0, 40, 5, 0x02a00000, globals.backgroundType, globals.backgroundFrame); root.node(n.separator).alignFlags |= 1; root.attach(n.separator, 0, 21, this.#token, 0);
        globals.rootSerial = n.serial; n.baseWidth = n.width = 80; n.baseHeight = n.height = root.createState().screenHeight - 80;
        root.attach(this.#token, 0, 0); const technology = parts.technology(); root.bindObject(n.bridge, technology); root.bindObject(n.teleport, technology);
        this.#session.synchronizeProductionContents(); this.refresh(3); return this.#token;
    }
    // 화면 높이가 바뀌면 기존 자식·Form을 유지하며 원본 하단 여백으로 다시 배치한다.
    resize(height) {
        if (!this.#token || !Number.isInteger(height) || height <= 80) throw new RangeError('생산 패널 화면 높이 또는 수명 오류');
        const node = this.node(), value = Math.fround(height - 80); if (node.baseHeight === value) return;
        node.baseHeight = node.height = value; this.#root.tree().push(this.#token); this.#root.attach(this.#token, 0, 0, this.#root.createState().collection.root, 0);
    }
    // 실제 사제 메뉴 열기 장치는 같은 패널 수명에 연결되어 있다.
    openPriest(sid, event) { return this.#parts.openPriest?.(sid, event) ?? 0; }
    // 공통 수명이 자식과 실제 Form을 먼저 반납한다.
    close() { this.#parts.close?.(); if (this.#token) { this.#root.remove(this.#token); this.#token = 0; } }
}
