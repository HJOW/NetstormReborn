// 실제 Client Gump와 GameAssets의 이미지 자식 생성·출력·불투명 적중을 연결한다.
// 사용: new ClientImageGump(root, assets, state, colors, baseHandle).allocate() 후 construct(token,type,frame,mode).
import { ImageGump, ImageGumpNode, ImageGumpPixelTarget, bindImageGumpAssets } from './image-gump.js';

// 실제 Gump 수명과 현재 출력 표면을 공유하는 이미지 가상 장치다.
export class ClientImageGump {
    // 현재 루트·실제 자산·색표와 재진입 가능한 출력 문맥이다.
    #root; #assets; #colors; #baseHandle; #currentColors = new Uint8Array(256); #active = null; #image;
    // 상태의 owner/색표는 호출 시점에 읽고 실제 색표 공급자는 선택된 색에서만 호출한다.
    constructor(root, assets, state, colors, baseHandle) {
        if (!root || !assets || !state || typeof colors !== 'function' || typeof baseHandle !== 'function') throw new TypeError('Client 이미지 Gump의 실제 수명·색표 누락');
        this.#root = root; this.#assets = assets; this.#colors = colors; this.#baseHandle = baseHandle;
        const hit = new ImageGumpPixelTarget(null, assets, this.#currentColors, colors).hooks({}).hit;
        const hooks = bindImageGumpAssets({ node: (token) => root.node(token), position: (token) => root.collection().position(token),
            attach: (token, x, y, parent, mode) => root.attachBase(token, x, y, parent, mode), hit,
            color: (token) => { if (!this.#active) throw new Error('Client 이미지의 현재 출력 색표 누락'); this.#active.color(token); },
            draw: (request) => { if (!this.#active) throw new Error('Client 이미지의 현재 출력 표면 누락'); this.#active.draw(request); },
        }, assets);
        this.#image = new ImageGump(root.factory(), state, hooks);
    }
    // 등록 전 파생 노드를 할당하고 같은 가상 부착·출력·삭제 경계를 설치한다.
    allocate() {
        const root = this.#root, image = this.#image, node = new ImageGumpNode(); let token = 0;
        token = root.allocateDerived(node, (event, first, second) => this.#baseHandle(token, event, first, second), (current, flags) => root.lifetime().deleteScalar(current, flags),
            (current, x, y, parent, mode) => image.attach(current, x, y, parent, mode), () => {});
        root.bindDraw(token, (current, target) => {
            const previous = this.#active; this.#active = new ImageGumpPixelTarget(target, this.#assets, this.#currentColors, this.#colors).hooks({});
            try { image.draw(current); } finally { this.#active = previous; }
        });
        return token;
    }
    // 원본 이미지 생성과 현재 프레임·적중 판정을 같은 저장소에 전달한다.
    construct(token, type, frame, mode) { return this.#image.construct(token, type, frame, mode); }
    // 실제 200×200 버퍼를 쓰는 원본 이미지 적중을 조회한다.
    hit(token, x, y) { return this.#image.hit(token, x, y); }
}
