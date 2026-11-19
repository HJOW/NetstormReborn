// 실제 Client Gump 트리와 Session 내용물 풀에 원본 생산 격자를 조립한다.
// 사용: new ClientInventoryGrid(root, session, state, devices).create(columns, rows, mask). 항목 그림·생산 패널은 호출자가 연결한다.
import { InventoryGump, InventoryGridNode, InventoryItemNode } from './inventory-gump.js';

// 같은 현재 UI·Session과 원본 격자의 가상 부착·입력·객체 통지를 공유한다.
export class ClientInventoryGrid {
    // 현재 Gump와 격자 실행기 및 선택적 실제 항목 공장을 빌린다.
    #root; #inventory;
    // 현재 풀·검색 전역·커서 상태와 항목 공장/기본 UI 경계를 받는다.
    constructor(root, session, state, devices) {
        if (!root || !session?.core || !state || !devices || typeof devices.allocateItem !== 'function' || typeof devices.baseHandle !== 'function' || typeof devices.now !== 'function') throw new TypeError('Client 생산 격자의 실제 수명·항목 장치 누락');
        this.#root = root; const pool = session.core().pool;
        const node = (token) => { const result = root.node(token); if (!result) throw new Error('Client 생산 격자의 현재 노드 누락'); return result; };
        const hooks = {
            grid: (token) => { const value = node(token); if (!(value instanceof InventoryGridNode)) throw new TypeError('Client 생산 격자 노드 오류'); return value; },
            item: (token) => { const value = node(token); if (!(value instanceof InventoryItemNode)) throw new TypeError('Client 생산 항목 노드 오류'); return value; },
            clearChildren: (token) => { while (node(token).child) root.lifetime().kill(node(token).child); },
            baseAttach: (token, x, y, parent, mode) => root.attachBase(token, x, y, parent, mode),
            allocateItem: (size) => devices.allocateItem(size), attach: (...args) => root.attach(...args), push: (token) => root.tree().push(token),
            bind: (token, object) => root.bindObject(token, object), idle: (token, enabled) => root.lifetime().setIdle(token, enabled),
            now: () => devices.now(), contentParent: (sid) => { const raw = pool.slot(sid); return raw[16] | (raw[17] << 8); },
            forward: (...args) => root.events().bubble(...args), baseHandle: (...args) => devices.baseHandle(...args),
            local: (token, _event, point) => root.collection().screenToLocal(token, point),
            inside: (token, point) => { const current = node(token); return point[0] >= 0 && point[1] >= 0 && point[0] < current.width && point[1] < current.height; },
            cancelCursor: () => session.cursor().cancel(), assertion: (text) => { throw new Error(`생산 격자 원본 진단: ${text}`); },
            redraw: (token, erase) => root.redraw(token, erase), clearBlink: (sid) => { pool.allocated(sid)[21] &= ~2; },
        };
        this.#inventory = new InventoryGump(root.createState(), root.factory(), pool, session.productionFinder(), state, hooks);
    }
    // 실제 파생 노드와 원본 가상 입력·부착·객체 소멸 경계를 생성한다.
    create(columns, rows, mask) {
        const root = this.#root, inventory = this.#inventory, node = new InventoryGridNode(); let token = 0;
        token = root.allocateDerived(node, (event, first, second) => inventory.handleGrid(token, event, first, second),
            (current, flags) => root.lifetime().deleteScalar(current, flags),
            (current, x, y, parent, mode) => inventory.attachGrid(current, x, y, parent, mode), () => {});
        inventory.constructGrid(token, columns, rows, mask);
        root.bindDraw(token, () => {}); root.bindObjectGone(token, (current, parent) => inventory.parentGone(current, parent));
        return token;
    }
}
