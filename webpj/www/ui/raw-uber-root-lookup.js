// 실제 Client Gump 등록표에서 현재 Uber HUD의 자식과 serial을 조회한다.
// 사용: new RawUberRootLookup(clientGump, readGlobal).reference(serial) 또는 .current().
import { UberRootNode } from './uber-root-gump.js';

// 원본 RawUberRootControls가 쓰는 삭제 플래그와 현재 미니맵 전역 주소다.
const DEAD = 0x10000, CURRENT_MINIMAP = 0x005949d0;

// 공통 소유자의 실제 등록표와 노드를 매 호출 시 빌린다.
export class RawUberRootLookup {
    // 현재 Client의 같은 수명과 선택적인 원본 전역 읽기 장치를 보유한다.
    #owner; #read;
    // 실제 Client 소유자만 사용하며 전역 읽기가 없으면 미니맵 예외를 허용하지 않는다.
    constructor(owner, readGlobal = null) {
        if (!owner || typeof owner.createState !== 'function' || typeof owner.node !== 'function') throw new TypeError('Uber HUD의 실제 Gump 소유자 누락');
        if (readGlobal !== null && typeof readGlobal !== 'function') throw new TypeError('Uber HUD 전역 읽기 계약');
        this.#owner = owner; this.#read = readGlobal;
    }
    // 루트 0을 제외하고 등록 순서로 살아 있는 같은 serial을 반환한다.
    reference(serial) {
        const entries = this.#owner.createState().entries;
        // 원본의 현재 등록표만 순회하며 반납한 이전 세대를 재사용하지 않는다.
        for (let index = 1; index < entries.length; index++) {
            const token = entries[index], node = token ? this.#owner.node(token) : null;
            if (node && node.serial === (serial >>> 0) && !(node.flags & DEAD)) return token;
        }
        return 0;
    }
    // 실제 파생 루트와 필수 자식이 살아 있어야 완성된 현재 HUD다.
    current() {
        const root = this.#owner.node(this.#owner.createState().entries[0]);
        if (!(root instanceof UberRootNode) || (root.flags & DEAD)) return false;
        // 원본 순서대로 각 필수 자식을 검사하고 토글된 옛 미니맵만 예외로 둔다.
        for (const token of [root.vertical, root.minimapDivider, root.chatDivider, root.minimap, root.production, root.knowledge, root.chat]) {
            const child = token ? this.#owner.node(token) : null;
            if (!child || (child.flags & DEAD)) {
                if (token && token === root.minimap && this.#read && (this.#read(CURRENT_MINIMAP) >>> 0) !== token) continue;
                return false;
            }
        }
        return true;
    }
}
