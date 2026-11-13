// 기본0xa0와 같은 주소의 배경0xb8 파생 생성·현재 타입/SHP 조회 순서다(cpppj client/GumpBackgroundCreate.cpp/.h, 기준선1f00a25a). (2026-11-14 WB04-E415)
import { GumpCreateNode } from './gump-create.js';

// 보호10.78 배경 VT는 식별값이며 호스트 함수 포인터로 실행하지 않는다.
const PATCH_BACKGROUND_VTABLE = 0x50ae40;

// +a0..b4의 파생 필드는 기본 생성자가 지우지 않는다.
export class GumpBackgroundNode extends GumpCreateNode {
    // 실제 shape 토큰·signed 타일 크기·현재 타입/프레임/배경 플래그다.
    shape = 0; tileWidth = 0; tileHeight = 0; type = 0; frame = 0; backgroundFlags = 0;
}

// 같은 기본 생성/등록기와 현재 signed color BYTE getter를 빌린다.
export class GumpBackgroundCreate {
    // 장기 생성 상태/공장·색/타입 장치다.
    #state; #factory; #color; #hooks;
    // node만 필수다. 사용: new GumpBackgroundCreate(state,factory,()=>color,{node,type?,frame?}).
    constructor(state, factory, color, hooks = {}) { if (typeof hooks.node !== 'function') throw new TypeError('배경 Gump 저장소 필요'); this.#state = state; this.#factory = factory; this.#color = color; this.#hooks = hooks; }
    // 기본 수집기가 같은 실제 파생 노드 주소를 제공하는지 확인한다.
    #node(token) { const node = token ? this.#hooks.node(token) : null; if (!node || this.#factory.collectionHooks().node(token) !== node) throw new Error('배경 Gump 토큰/공유 저장소 없음'); return node; }
    // type0의 shape/타일 미작성과 프레임 콜백 뒤 현재 shape 읽기를 유지한다. bits는 순수 float 인자 복사다.
    construct(token, width, height, backgroundFlags, flags, type, frame, bits = {}) {
        const node = this.#node(token); this.#factory.construct(token, 0, width, height, flags, bits); node.vtable = PATCH_BACKGROUND_VTABLE; node.command = ((this.#color() << 24) >> 24) >>> 0; node.backgroundFlags = backgroundFlags >>> 0; node.type = type >>> 0; node.frame = frame >>> 0;
        if (type) { if (typeof this.#hooks.type !== 'function' || typeof this.#hooks.frame !== 'function') throw new TypeError('배경 타입/SHP 프레임 경계 필요'); const asset = this.#hooks.type(type); if (!asset) throw new Error('배경 타입 저장소 없음'); const header = this.#hooks.frame(asset.token, 0, node.frame); if (!header) throw new Error('배경 SHP 프레임 헤더 없음'); node.tileWidth = header.width << 16 >> 16; node.tileHeight = header.height << 16 >> 16; node.shape = asset.shape >>> 0; }
        return token;
    }
}
