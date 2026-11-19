// 실제 Client Gump 트리와 현재 픽셀 표면에 원본 배경 생성·출력을 연결한다.
// 사용: new ClientGumpBackground(root, game, color, maps, palette, baseHandle).create(width,height,flags,type,frame).
import { GumpBackgroundCreate, GumpBackgroundNode } from './gump-background-create.js';
import { GumpBackgroundAssets, GumpBackgroundDraw, GumpBackgroundDrawState, GumpBackgroundPixelTarget, bindGumpBackgroundAssets, setGumpBackgroundColors } from '../render/gump-background-draw.js';

// 같은 Gump 수명·실제 자산·현재 팔레트와 출력 클립을 사용하는 배경 공장이다.
export class ClientGumpBackground {
    // 생성 자산과 장기 진단 상태, 현재 팔레트/명암 및 기본 가상 입력을 빌린다.
    #root; #assets; #create; #state = new GumpBackgroundDrawState(); #maps; #palette; #baseHandle; #colors = new Uint8Array(256);
    // 자원이나 현재 장치가 없으면 처음부터 거부한다.
    constructor(root, game, color, maps, palette, baseHandle) {
        if (!root || !game || [color, maps, palette, baseHandle].some((value) => typeof value !== 'function')) throw new TypeError('Client Gump 배경의 실제 장치 누락');
        this.#root = root; this.#assets = new GumpBackgroundAssets(game); this.#maps = maps; this.#palette = palette; this.#baseHandle = baseHandle;
        const hooks = { node: (token) => root.node(token) }; bindGumpBackgroundAssets(hooks, {}, this.#assets);
        this.#create = new GumpBackgroundCreate(root.createState(), root.factory(), color, hooks);
    }
    // 현재 픽셀 표면·공유 클립·명암표로 같은 배경 출력기를 만든다.
    #drawing(target, clip) {
        setGumpBackgroundColors(this.#state, this.#palette());
        const hooks = { node: (token) => this.#root.node(token) }; bindGumpBackgroundAssets({}, hooks, this.#assets);
        const pixels = new GumpBackgroundPixelTarget(target, this.#assets, this.#maps(), this.#colors);
        return new GumpBackgroundDraw(this.#root.collection(), clip, this.#state, pixels.hooks(clip, hooks));
    }
    // 실제 파생 배경 노드와 가상 입력·출력·삭제를 생성한다.
    create(width, height, flags, type, frame) {
        const root = this.#root, node = new GumpBackgroundNode(); let token = 0;
        token = root.allocateDerived(node, (event, first, second) => this.#baseHandle(token, event, first, second),
            (current, destroyFlags) => root.lifetime().deleteScalar(current, destroyFlags), null, () => {});
        this.construct(token, width, height, flags, type, frame);
        root.bindDraw(token, (current, target, clip) => this.draw(current, target, clip)); return token;
    }
    // 구분선처럼 배경에서 파생된 기존 노드도 같은 자산 생성자를 사용한다.
    construct(token, width, height, flags, type, frame) { return this.#create.construct(token, width, height, flags, 1, type, frame); }
    // 구분선의 가상 출력에서도 같은 현재 표면과 클립을 사용한다.
    draw(token, target, clip) { this.#drawing(target, clip).draw(token); }
    // 항목 장식 등 직접 배경 질감도 같은 실제 화면과 클립을 사용한다.
    texture(target, clip, rect, flags, type, frame) { this.#drawing(target, clip).texture(rect, flags, type, frame); }
}
