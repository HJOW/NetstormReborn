// 실제 생산 격자 항목을 같은 Client Gump·Session·이미지 자산에 연결한다.
// 사용: new ClientInventoryItem(root, session, assets, geometry, measure, images, state, devices).allocate(0xc0).
import { InventoryItemNode } from './inventory-gump.js';
import { InventoryItemGump, bindInventoryItemAssets } from './inventory-item-gump.js';

// 실제 항목의 가상 입력·부착·출력과 그림 자식 수명을 같은 트리에 소유한다.
export class ClientInventoryItem {
    // 현재 루트와 실제 항목 본체, 출력 중에만 유효한 픽셀 대상이다.
    #root; #item; #devices; #target = null;
    // 텍스트·도움말·배경 등 선택된 UI 경계는 호출자가 제공한다.
    constructor(root, session, assets, geometry, measure, images, state, devices) {
        if (!root || !session?.core || !images || !state || !devices) throw new TypeError('Client 생산 항목의 실제 수명 누락');
        this.#root = root; this.#devices = devices; const pool = session.core().pool, finder = session.productionFinder();
        const node = (token) => { const value = root.node(token); if (!value) throw new Error('Client 생산 항목의 현재 노드 누락'); return value; };
        const device = (name, ...args) => { if (typeof devices[name] !== 'function') throw new Error(`Client 생산 항목 실제 ${name} 미연결`); return devices[name](...args); };
        const hooks = {
            node: (token) => { const value = node(token); if (!(value instanceof InventoryItemNode)) throw new TypeError('Client 생산 항목 노드 오류'); return value; }, base: node,
            baseAttach: (token, x, y, parent, mode) => root.attachBase(token, x, y, parent, mode),
            clearChildren: (token) => { while (node(token).child) root.lifetime().kill(node(token).child); }, cleanup: () => root.lifetime().cleanup(),
            robust: (sid) => sid < pool.capacity && sid <= 0xffff && pool.slot(sid)[10] === finder.containedType,
            assertion: (text) => { throw new Error(`생산 항목 원본 진단: ${text}`); },
            allocate: (size) => size === 0xb4 ? images.allocate() : size === 0xa4 ? device('allocateText', size) : (() => { throw new RangeError('생산 항목 자식 할당 크기'); })(),
            image: (...args) => images.construct(...args), text: (...args) => device('text', ...args),
            styledText: (...args) => device('styledText', ...args), fitText: (...args) => device('fitText', ...args),
            attach: (...args) => root.attach(...args), costUnits: (type) => session.hitPoints().typeCostUnits(type), money: (owner) => session.sp(owner),
            position: (token) => root.collection().position(token),
            background: (...args) => { if (!this.#target) throw new Error('생산 항목의 현재 출력 표면 누락'); device('background', this.#target, ...args); },
            local: (token, _event, x, y) => root.collection().screenToLocal(token, { x: x | 0, y: y | 0 }),
            knowledgeOpen: () => session.cursorDropSettings().lockDepth === 0 ? 1 : 0, imageHit: (...args) => images.hit(...args),
            denied: (...args) => device('denied', ...args), tell: (...args) => device('tell', ...args),
            forward: (...args) => root.events().bubble(...args), about: (...args) => device('about', ...args),
        };
        bindInventoryItemAssets(hooks, assets, geometry, measure); this.#item = new InventoryItemGump(pool, state, hooks);
    }
    // 격자가 요청한 0xc0 파생 노드를 할당하고 원본 가상 분배기를 설치한다.
    allocate(size) {
        if (size !== 0xc0) throw new RangeError('Client 생산 항목 할당 크기 오류');
        const root = this.#root, item = this.#item, node = new InventoryItemNode(); let token = 0;
        token = root.allocateDerived(node, (event, first, second) => item.handle(token, event, first, second),
            (current, flags) => root.lifetime().deleteScalar(current, flags),
            (current, x, y, parent, mode) => item.attach(current, x, y, parent, mode), () => {},
            (current) => { if (typeof this.#devices.idleItem !== 'function') throw new Error('생산 항목 실제 점멸 Idle 미연결'); this.#devices.idleItem(current); });
        root.bindDraw(token, (current, target) => { const previous = this.#target; this.#target = target; try { item.draw(current); } finally { this.#target = previous; } });
        return token;
    }
}
