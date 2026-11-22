// 실제 Client Gump와 패널 자산에 원본 장식 모서리·배경·재그리기를 연결한다.
import { GumpDecoration, GumpDecorationNode, GumpDecorationPixelTarget, GumpDecorationState, bindGumpDecorationAssets } from './gump-decoration.js';

// 같은 Client 트리·배경·실제 화면의 장식 파생 노드를 소유한다.
export class ClientGumpDecoration {
    // 장기 자산·수명과 출력 중에만 유효한 현재 클립이다.
    #root; #assets; #background; #baseHandle; #invalidate; #active = null; #decoration;
    // 배경과 모서리 타입은 호출 시점의 상태에서 읽는다.
    constructor(root, assets, background, state, baseHandle, invalidate) {
        if (!root || !assets || !background || !state || typeof baseHandle !== 'function' || typeof invalidate !== 'function') throw new TypeError('Client 장식 Gump 실제 장치 누락');
        this.#root = root; this.#assets = assets; this.#background = background; this.#baseHandle = baseHandle; this.#invalidate = invalidate;
        const hooks = { node: (token) => root.node(token), base: (token) => root.node(token), invalidate: (rect) => this.#invalidate(rect),
            shape: (...args) => { if (!this.#active) throw new Error('Client 장식의 현재 출력 표면 누락'); this.#active.raster.shape(...args); } };
        bindGumpDecorationAssets(hooks, assets);
        const factory = { construct: (token, width, height, flags, _mode, type, frame) => background.construct(token, width, height, flags, type, frame) };
        const drawing = { draw: (token) => { if (!this.#active) throw new Error('Client 장식의 현재 배경 표면 누락'); background.draw(token, this.#active.target, this.#active.clip); } };
        this.#decoration = new GumpDecoration(root.createState(), state, factory, drawing, root.collection(), root.lifetime(), hooks);
    }
    // 실제 파생 노드의 출력·확장 재그리기·소멸을 공통 루트에 등록한다. 장식 노드에서 파생한 클래스(입력 창 부모 등)와 그 가상 입력 처리기(토큰, 사건, 첫 값, 둘째 값)를 고르면 기본 Help 대신 그 처리기를 쓴다.
    // (2026-11-22 WB04-E1168: 송금 숫자 입력 창 부모를 같은 장식 출력으로 만들기 위해 파생 노드·처리기 선택 추가)
    allocate(NodeClass = GumpDecorationNode, handler = null) {
        if (typeof NodeClass !== 'function' || !(new NodeClass() instanceof GumpDecorationNode) || (handler !== null && typeof handler !== 'function')) throw new TypeError('Client 장식 Gump 파생 노드·처리기 계약');
        const root = this.#root, node = new NodeClass(); let token = 0;
        token = root.allocateDerived(node, (event, first, second) => (handler ?? this.#baseHandle)(token, event, first, second),
            (current, flags) => this.#decoration.deleteScalar(current, flags), null, () => {});
        root.bindRedraw(token, (current, erase) => this.#decoration.redraw(current, erase));
        root.bindDraw(token, (current, target, clip) => { const previous = this.#active; this.#active = { target, clip, raster: new GumpDecorationPixelTarget(target, this.#assets).hooks({}) }; try { this.#decoration.draw(current); } finally { this.#active = previous; } });
        return token;
    }
    // 원본 장식 생성자를 같은 실제 노드에 적용한다.
    construct(token, width, height) { return this.#decoration.construct(token, width, height); }
}

// 현재 fortGump 타입을 원본 배경·모서리 전역으로 설정한다.
export function createClientDecorationState(type, frame = 0) { const state = new GumpDecorationState(); state.backgroundType = type; state.backgroundFrame = frame; state.cornerType = type; return state; }
