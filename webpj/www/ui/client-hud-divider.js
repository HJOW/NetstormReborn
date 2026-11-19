// 실제 Client Gump 저장소에 원본 HUD 구분선 생성·부착·입력을 연결한다.
// 사용: new ClientHudDivider(owner, devices).allocate() → construct(token, axis, width, height, flags, type, frame).
import { GumpBackgroundCreate } from './gump-background-create.js';
import { HudDividerGump, HudDividerNode } from './hud-dock.js';
import { HudDividerInput } from './hud-divider-input.js';
import { RawUberRootLookup } from './raw-uber-root-lookup.js';

// Client와 같은 등록표·트리·수명에서 실제 파생 구분선을 관리한다.
export class ClientHudDivider {
    // 원본 생성·부착·입력 본체와 같은 Client 소유자다.
    #owner; #divider; #input; #background;
    // 실제 색과 popup·입력 장치는 호출자가 제공하고 도달한 분기에서 사용한다.
    constructor(owner, devices) {
        if (!owner || typeof owner.createState !== 'function' || typeof devices?.backgroundColor !== 'function' || typeof devices?.dividerColor !== 'function') throw new TypeError('HUD 구분선의 실제 소유자와 색 전역 필요');
        this.#owner = owner; this.#background = devices.background ?? null;
        const node = (token) => owner.node(token);
        const divider = (token) => { const value = node(token); return value instanceof HudDividerNode ? value : null; };
        const lookup = new RawUberRootLookup(owner);
        const background = new GumpBackgroundCreate(owner.createState(), owner.factory(), devices.backgroundColor,
            { node, type: devices.type, frame: devices.frame });
        const hooks = { node, divider,
            constructBackground: (token, width, height, flags, type, frame) => this.#background ? this.#background.construct(token, width, height, flags, type, frame) : background.construct(token, width, height, flags, 1, type, frame),
            activate: (token, enabled) => owner.lifetime().setOverlay(token, enabled),
            baseAttach: (...args) => owner.attachBase(...args), push: (token) => owner.tree().push(token),
            attach: (...args) => owner.attach(...args), move: (...args) => owner.move(...args),
            resolveSerial: (serial) => lookup.reference(serial), bubble: (...args) => owner.events().bubble(...args),
            closePopups: () => { if (typeof devices.closePopups !== 'function') throw new Error('HUD popup 정리 장치 누락'); devices.closePopups(); },
            assertion: devices.assertion,
        };
        this.#divider = new HudDividerGump(owner.createState(), devices.dividerColor, hooks);
        this.#input = new HudDividerInput(owner.collection(), owner.input(), owner.tree(), { divider, base: node,
            baseHandle: devices.baseHandle, cursor: devices.cursor, menuInstances: devices.menuInstances, poll: devices.poll });
    }
    // 실제 파생 노드와 가상 입력·삭제·부착을 공통 Client에 할당한다.
    allocate() {
        const node = new HudDividerNode();
        let token = 0;
        token = this.#owner.allocateDerived(node, (event, first, second) => this.#input.handle(token, event, first, second),
            (current, flags) => this.#owner.lifetime().deleteScalar(current, flags),
            (current, x, y, parent, mode) => this.#divider.attach(current, x, y, parent, mode),
            (current, event, first, second) => this.#input.broadcast(current, event, first, second));
        if (this.#background) this.#owner.bindDraw(token, (current, target, clip) => this.#background.draw(current, target, clip));
        return token;
    }
    // 할당 토큰에 실제 배경과 구분선 생성 순서를 적용한다.
    construct(token, axis, width, height, flags, type, frame) { return this.#divider.construct(token, axis, width, height, flags, type, frame); }
    // 현재 등록된 실제 자식 serial과 관계를 구분선에 저장한다.
    registerSlave(master, slave, flags) { this.#divider.registerSlave(master, slave, flags); }
    // 공통 방송이 선택한 실제 구분선의 원본 드래그 본체를 실행한다.
    broadcast(token, event, first, second) { this.#input.broadcast(token, event, first, second); }
}
