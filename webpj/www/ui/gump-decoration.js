// 장식 배경의 같은 생성/현재 모서리·확장 redraw/수명과 실제 자산 픽셀을 연결한다(cpppj client/GumpDecoration.cpp/.h, 기준선1f00a25a). (2026-11-14 WB04-E419)
import { GumpBackgroundNode } from './gump-background-create.js';
import { GUMP_REGISTRY_CAPACITY } from './gump-create.js';

// 10.78 VT 식별값과 질감/네 테두리 원문 배경 비트다.
const PATCH_VTABLE = 0x50ae7c; const BACKGROUND = 0x03e00000;
// 실제 사용한 가상 장치만 요구한다.
function need(fn) { if (typeof fn !== 'function') throw new TypeError('장식 배경 선택 가상/장치 경계 필요'); return fn; }
// CRT signed64 절삭의 하위 signedDWORD다. 무효 결과의 하위값은0이다.
function ftol(value) { if (!Number.isFinite(value) || value < -9223372036854775808 || value >= 9223372036854775808) return 0; return Number(BigInt.asIntN(32, BigInt(Math.trunc(value)))); }
// 원문 좌표 합의 하위DWORD를 보존한다.
const add = (first, second) => (first + second) | 0;

// 기본/배경과 같은 주소의 +b8/+bc/+c0 모서리 캐시다.
export class GumpDecorationNode extends GumpBackgroundNode {
    // 배경 타입/프레임과 독립인 실제 캐시 SHP·모서리 타입/첫 I 프레임이다.
    cornerShape = 0; cornerType = 0; cornerFrame = 0;
}
// 실제 생성 시점의 signed 식별 BYTE와 현재 배경/장식 자산이다.
export class GumpDecorationState {
    // 장식 종류의 원문 기본값57이다.
    alignment = 57;
    // 현재 배경 자산 타입/프레임과 모서리 자산 타입이다.
    backgroundType = 0; backgroundFrame = 0; cornerType = 0;
}

// 같은 실제 GumpBackgroundCreate/Draw·현재 위치/기본 수명에 위임한다.
export class GumpDecoration {
    // 장기 공통 상태/장치와 안정된 현재 파생/기본 저장소다.
    #create; #state; #factory; #background; #collection; #life; #hooks;
    // 노드/기본 주소만 생성 때 필수다. 나머지는 선택 분기에서 요구한다.
    constructor(create, state, factory, background, collection, lifetime, hooks = {}) { need(hooks.node); need(hooks.base); this.#create = create; this.#state = state; this.#factory = factory; this.#background = background; this.#collection = collection; this.#life = lifetime; this.#hooks = hooks; }
    // 기본과 실제 같은 파생 주소여야 한다.
    #node(token) { const n = token ? this.#hooks.node(token) : null; if (!n || this.#hooks.base(token) !== n) throw new Error('장식 배경 공유 노드 없음'); return n; }
    // 부모 visible/현재 parent를 실제 기본 저장소에서 읽는다.
    #base(token) { const n = token ? this.#hooks.base(token) : null; if (!n) throw new Error('장식 배경 부모 노드 없음'); return n; }
    // 배경 생성 뒤 현재 정렬/장식 타입·자산 결과를 별도 캐시에 저장한다.
    construct(token, width, height, bits = {}) { this.#node(token); this.#factory.construct(token, width, height, BACKGROUND, 1, this.#state.backgroundType, this.#state.backgroundFrame, bits); this.#node(token).vtable = PATCH_VTABLE; this.#node(token).command = ((this.#state.alignment << 24) >> 24) >>> 0; this.#node(token).cornerType = this.#state.cornerType >>> 0; const type = need(this.#hooks.type)(this.#state.cornerType); this.#node(token).cornerShape = type.shape >>> 0; this.#node(token).cornerFrame = type.firstFrame >>> 0; return token; }
    // 앞 모서리 callback이 바꾼 현재 캐시/첫 프레임을 매 호출마다 읽는다.
    #corner(token, offset, position) { const shape = need(this.#hooks.shape); const n = this.#node(token); shape(n.cornerShape, (n.cornerFrame + offset) >>> 0, { ...position }); }
    // 배경 뒤 저장한 Position은 유지하고 각 모서리의 크기는 원문 시점에 현재 값을 절삭한다.
    draw(token) { this.#background.draw(token); const position = this.#collection.position(token); this.#corner(token, 0, position); this.#corner(token, 1, { x: add(position.x, ftol(this.#node(token).width)), y: position.y }); const bottom = add(position.y, ftol(this.#node(token).height)); this.#corner(token, 2, { x: add(position.x, ftol(this.#node(token).width)), y: bottom }); this.#corner(token, 3, { x: position.x, y: add(position.y, ftol(this.#node(token).height)) }); }
    // 자신/모든 부모 visible만 필터하고(x-1,y-1,width+4,height+4) 원문 영역을 요청한다.
    redraw(token, erase) { void erase; if (!(this.#node(token).flags & 1)) return; const seen = new Set();
        // dead/void는 추가 필터가 아니다. 현재 parent 링크의 순환/원문 한도만 거부한다.
        for (let parent = this.#node(token).parent; parent; parent = this.#base(parent).parent) { if (seen.has(parent) || seen.size >= GUMP_REGISTRY_CAPACITY) throw new Error('장식 배경 부모 순환'); seen.add(parent); if (!(this.#base(parent).flags & 1)) return; }
        const position = this.#collection.position(token); const height = ftol(this.#node(token).height + 4); const width = ftol(this.#node(token).width + 4); if (width <= 0 || height <= 0) return; const left = add(position.x, -1); const top = add(position.y, -1); need(this.#hooks.invalidate)({ left, top, right: add(left, width), bottom: add(top, height) });
    }
    // SHP 자산을 별도 해제하지 않고 실제 기본 scalar 소멸/토큰 반납에 위임한다.
    deleteScalar(token, flags) { this.#node(token); return this.#life.deleteScalar(token, flags); }
}

// 실제 TypeTable의 같은 첫 글자 구간을 공급한다. I가 없으면 원문 signed-1의 DWORD를 유지한다.
export function bindGumpDecorationAssets(hooks, assets) { hooks.type = (type) => { type >>>= 0; if (type < 70 || type - 70 >= assets.types().length) throw new RangeError('장식 배경 실제 타입 범위 초과'); return { shape: type, firstFrame: assets.typeTable.view(type).first[8] >>> 0 }; }; }

// 배경/글자와 같은 현재 GumpPixelTarget에 캐시 SHP 원색 모서리를 출력한다.
export class GumpDecorationPixelTarget {
    // 장기 현재 표면/실제 자산과 캐시(shape,물리frame) 그림이다.
    #target; #assets; #images = new Map();
    // 생성 시 픽셀 출력은 하지 않는다.
    constructor(target, assets) { this.#target = target; this.#assets = assets; }
    // 생성/조회/변경 영역 경계는 보존하며 실제 모서리 출력만 설치한다.
    hooks(hooks = {}) { return { ...hooks, shape: (shape, frame, position) => this.#shape(shape, frame, position) }; }
    // 생성 때 캐시한 SHP의 실제 물리 프레임만 읽으며 논리/레이어 개수로 다시 제한하지 않는다.
    #shape(shape, frame, position) { shape >>>= 0; frame >>>= 0; if (shape < 70 || shape - 70 >= this.#assets.types().length) throw new RangeError('장식 배경 캐시 SHP 범위 초과'); const block = this.#assets.types()[shape - 70].block; const frames = this.#assets.shapes.blocks[block].frames; if (frame >= frames.length) throw new RangeError('장식 배경 모서리 물리 프레임 범위 초과'); const source = frames[frame]; if (source.special) return; const key = `${shape}:${frame}`; let image = this.#images.get(key); if (!image) { image = this.#assets.shapes.decode(block, frame); this.#images.set(key, image); } this.#target.image(image, { x: add(position.x, source.rect.left), y: add(position.y, source.rect.top) }); }
}

// 실제 장식의 Draw/Redraw/scalar만 선택하고 다른 파생 fallback은 각각 보존한다.
export function bindGumpDecoration(draw, frame, lifetime, resolve) { need(resolve); const previousDraw = draw.draw; const previousRedraw = frame.redraw; const previousDelete = lifetime.deletingDestructor;
    // 같은 GumpDraw의 현재 clip 보호 안에서 출력한다.
    draw.draw = (token) => { const decoration = resolve(token); if (decoration) decoration.draw(token); else need(previousDraw)(token); };
    // 프레임이 전달한 erase 인자와 현재 토큰을 그대로 보존한다.
    frame.redraw = (token, erase) => { const decoration = resolve(token); if (decoration) decoration.redraw(token, erase); else need(previousRedraw)(token, erase); };
    // 자식 파괴/등록 해제는 공통 수명이 이미 수행한다.
    lifetime.deletingDestructor = (token, flags) => { const decoration = resolve(token); if (decoration) decoration.deleteScalar(token, flags); else need(previousDelete)(token, flags); };
}
