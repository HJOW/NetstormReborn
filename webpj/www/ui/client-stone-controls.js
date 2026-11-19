// 같은 Client Gump 등록·트리·입력·수명 위에 실제 돌 버튼 본체를 연결한다.
// 사용: new ClientStoneControls(owner, devices).allocate() → construct(token, width, height, label, unused, context).
import { GumpBackgroundCreate, GumpBackgroundNode } from './gump-background-create.js';
import { ButtonGump, ButtonGumpNode } from './button-gump.js';
import { StoneButtonGump, StoneButtonGumpNode } from './stone-button-gump.js';

// 실제 공통 수명과 버튼·배경 생성 장치를 한 번 묶는다.
export class ClientStoneControls {
    // Client 소유자와 기존 원본 버튼·돌 버튼 본체다.
    #owner; #buttons; #stones; #background; #owned = new Set();
    // 실제 색 상태와 호출할 때 필요한 시간·소리·폰트·타입 장치를 빌린다.
    constructor(owner, devices) {
        if (!owner || typeof owner.createState !== 'function' || !devices?.buttonState || !devices?.stoneState || typeof devices.backgroundColor !== 'function') throw new TypeError('Client 돌 버튼의 실제 소유자·상태 필요');
        this.#owner = owner;
        const base = (token) => owner.node(token);
        const button = (token) => { const node = base(token); return node instanceof ButtonGumpNode ? node : null; };
        const stone = (token) => { const node = base(token); return node instanceof StoneButtonGumpNode ? node : null; };
        const background = new GumpBackgroundCreate(owner.createState(), owner.factory(), devices.backgroundColor, { node: base, type: devices.type, frame: devices.frame });
        this.#background = background;
        this.#buttons = new ButtonGump(owner.createState(), devices.buttonState, owner.factory(), owner.collection(), owner.events(), owner.input(), owner.lifetime(), {
            node: button, baseNode: base, now: devices.now, poll: devices.poll, sound: devices.sound, baseEvent: devices.baseEvent,
            redraw: (token, erase) => owner.redraw(token, erase), backgroundFlags: (token) => {
                const node = base(token); if (!(node instanceof GumpBackgroundNode)) return null;
                return { get value() { return node.backgroundFlags; }, set value(value) { node.backgroundFlags = value >>> 0; } };
            },
        });
        this.#stones = new StoneButtonGump(owner.createState(), devices.stoneState, owner.factory(), this.#buttons, background, owner.tree(), owner.lifetime(), {
            node: stone, allocate: (size) => {
                if (size !== 0xb8) throw new RangeError('돌 버튼 실제 배경 할당 크기 오류');
                const node = new GumpBackgroundNode(); let token = 0;
                token = owner.allocateDerived(node, (event, first, second) => {
                    if (typeof devices.baseEvent !== 'function') throw new Error('돌 버튼 배경 기본 입력 미연결');
                    return devices.baseEvent(token, event, first, second);
                }, (current, flags) => this.#deleteBase(current, flags));
                if (devices.output?.background) owner.bindDraw(token, (current, target, clip) => devices.output.background.draw(current, target, clip));
                this.#owned.add(token); return token;
            },
            attach: (...args) => owner.attach(...args), push: (token) => owner.tree().push(token), measure: devices.measure,
        });
    }
    // 실제 돌 버튼 또는 그 파생 노드를 Client 주소와 가상 처리기에 등록한다.
    allocate(node = new StoneButtonGumpNode()) {
        if (!(node instanceof StoneButtonGumpNode)) throw new TypeError('Client 돌 버튼 파생 노드 필요');
        let token = 0;
        token = this.#owner.allocateDerived(node, (event, first, second) => this.#buttons.handle(token, event, first, second),
            (current, flags) => { const result = this.#stones.deleteScalar(current, flags); if (flags & 1) this.#owned.delete(current); return result; },
            (current, x, y, parent, mode, bits) => this.#stones.attach(current, x, y, parent, mode, bits),
            (current, event, first, second) => this.#buttons.broadcast(current, event, first, second),
            (current) => this.#buttons.idle(current));
        this.#owned.add(token); return token;
    }
    // 배경의 실제 공통 소멸과 토큰 반납을 함께 처리한다.
    #deleteBase(token, flags) { const result = this.#owner.lifetime().deleteScalar(token, flags); if (flags & 1) this.#owned.delete(token); return result; }
    // 실패한 미부착 노드도 공통 등록 여부에 맞게 반납한다.
    close() {
        this.#owner.lifetime().cleanup();
        // 부모 삭제가 자식을 반납할 수 있으므로 각 현재 소유 토큰을 다시 확인한다.
        for (const token of [...this.#owned]) {
            const node = this.#owner.node(token); if (!node) { this.#owned.delete(token); continue; }
            if (this.#owner.createState().entries[node.id] === token) this.#owner.remove(token);
            else { this.#owner.discardUnregistered(token); this.#owned.delete(token); }
        }
    }
    // 기존 원본 돌 버튼 생성자에 같은 주소·자산·문맥을 전달한다.
    construct(token, width, height, label = null, unused = 0, context = null) { return this.#stones.construct(token, width, height, label, unused, context); }
    // 같은 실제 배경 생성기를 Slider 손잡이가 빌린다.
    background() { return this.#background; }
    // 같은 원본 돌 버튼 본체를 Slider 생성기가 빌린다.
    backend() { return this.#stones; }
    // 같은 원본 버튼 본체를 Slider 입력기가 빌린다.
    buttons() { return this.#buttons; }
}
