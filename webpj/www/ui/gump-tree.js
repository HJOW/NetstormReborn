// 같은 Gump 생성 저장소의 부착·정렬·밀어내기·깊이·이동 순서를 이식한다(cpppj client/GumpTree.cpp/.h, 기준선1f00a25a).
// 사용: new GumpTree(createState,treeState,{node,redraw,push,attach,kill,project?,diagnostic?}). 콜백 뒤 현재 필드를 읽으며 부분 실패를 되돌리지 않는다. (2026-11-14 WB04-E411)
import { GUMP_REGISTRY_CAPACITY, gumpNodeFloatBits } from './gump-create.js';

// 기본 void·월드 부착·동종 교체·화면 제한/깊이 상속 생략·드래그 기준점 비트다.
const VOID = 8; const WORLD = 0x2000; const REPLACE = 0x4000; const UNCLAMPED = 0x20000; const OWN_DEPTH = 0x200000; const GRAB = 0x10;
// signed DWORD 지도 끝점 합계는 넘친 뒤0방향으로2로 나눈다.
const center = (low, high) => Math.trunc(((low + high) | 0) / 2);
// 원본 float 비트를 숫자로 본다. 단순 복사에는 별도 비트를 함께 전달할 수 있다.
function float(bits) { const view = new DataView(new ArrayBuffer(4)); view.setUint32(0, bits >>> 0, true); return view.getFloat32(0, true); }
// 산술 쓰기는 이전 복사 비트를 버리고 단순 복사는 명시 원문 비트를 보존한다.
function store(node, field, value, bits) { node[field] = Math.fround(value); node.floatBits ??= {}; if (bits === undefined) delete node.floatBits[field]; else node.floatBits[field] = bits >>> 0; }
// 한 형제 체인 안의 순환/과도한 길이만 거부한다. 콜백 뒤에도 현재 방문 경로를 쓴다.
function enter(seen, token) { if (seen.size >= GUMP_REGISTRY_CAPACITY || seen.has(token)) throw new Error('Gump 트리 링크 순환/한도 초과'); seen.add(token); }

// 실제 Client checking getter와 signed 타입 상한·지도 영역/진단 모드다.
export class GumpTreeState {
    // 원본 지도 화면 영역과 패치판 링크 진단3/4 모드다.
    mapView = { left: 0, top: 0, right: 0, bottom: 0 }; pushDebugMode = 0;
    // 소비 시마다 읽는 같은 현재DWORD와 보호PE 초기 signed BYTE0x43이다.
    checking = () => 0; typeLimit = 67;
}

// 첫 상태 쓰기 전의 signed GT 진단이다. 기록만 하고 복귀하면 같은 노드로 부착을 계속한다.
export function recordGumpAttachDiagnostic(node, checking, limit, diagnostic = null) {
    if ((checking() >>> 0) === 0) return; const type = node.command | 0; const maximum = (limit << 24) >> 24;
    if (type < 1 || type >= maximum) { if (typeof diagnostic !== 'function') throw new Error('Gump 부착 원문 상세 writer 미연결'); diagnostic(`gt = ${type}`); }
}

// 같은 생성 상태/트리 전역과 현재 가상 장치를 빌리는 처리기다.
export class GumpTree {
    // 안정된 노드/현재 전역·가상 경계다.
    #state; #tree; #hooks;
    // 투영/상세 writer는 해당 분기에서만 요구한다.
    constructor(state, tree, hooks = {}) { if (['node', 'redraw', 'push', 'attach', 'kill'].some((name) => typeof hooks[name] !== 'function')) throw new TypeError('Gump 트리 저장소/처리기 필요'); this.#state = state; this.#tree = tree; this.#hooks = hooks; }
    // 콜백 동안 같은 객체 주소를 돌려주어야 한다. 없는 토큰은 즉시 거부한다.
    #node(token) { const node = token ? this.#hooks.node(token >>> 0) : null; if (!node) throw new Error('Gump 트리 토큰/저장소 없음'); return node; }
    // 부모 기본 크기·원본 float 저장 순서로 정렬값을 구하며 자신의 위치는 쓰지 않는다.
    align(token) {
        const node = this.#node(token); const parent = this.#node(node.parent); const root = node.parent === this.#state.collection.root; const width = root ? Math.fround(this.#state.screenWidth) : parent.baseWidth; const height = root ? Math.fround(this.#state.screenHeight) : parent.baseHeight; let x = node.x; let y = node.y; let xBits = gumpNodeFloatBits(node, 'x'); let yBits = gumpNodeFloatBits(node, 'y'); const flags = node.alignFlags;
        if ((flags & 0xc0) && !root) throw new Error('지도 중심 정렬의 루트 부모 필요');
        if (flags & 1) { store(node, 'baseWidth', width, root ? undefined : gumpNodeFloatBits(parent, 'baseWidth')); store(node, 'width', width * node.scale); }
        if (flags & 2) { store(node, 'baseHeight', height, root ? undefined : gumpNodeFloatBits(parent, 'baseHeight')); store(node, 'height', height * node.scale); }
        if (flags & 4) { x = Math.fround((width - node.baseWidth) * 0.5); xBits = undefined; }
        if (flags & 8) { y = Math.fround((height - node.baseHeight) * 0.5); yBits = undefined; }
        if (flags & 0x10) { x = Math.fround(width - node.baseWidth); xBits = undefined; }
        if (flags & 0x20) { y = Math.fround(height - node.baseHeight); yBits = undefined; }
        if (flags & 0x40) { x = Math.fround(center(this.#tree.mapView.left, this.#tree.mapView.right) - node.baseWidth * 0.5); xBits = undefined; }
        if (flags & 0x80) { y = Math.fround(center(this.#tree.mapView.top, this.#tree.mapView.bottom) - node.baseHeight * 0.5); yBits = undefined; }
        const result = [x, y]; Object.defineProperties(result, { xBits: { value: xBits }, yBits: { value: yBits } }); return result;
    }
    // 자신의 parent/next/previous는 남기고 전역 링크를 먼저 분리한 뒤 redraw(true)를 호출한다.
    push(token) {
        const node = this.#node(token); if (node.flags & VOID) throw new Error('이미 void인 Gump 밀어내기'); if (!node.previous && !node.parent && node.id) throw new Error('부모 없는 Gump의 id0 필요'); node.flags = (node.flags | VOID) >>> 0;
        if (node.previous) { this.#node(node.previous).next = node.next; if (this.#tree.pushDebugMode === 3) this.#node(node.previous).next = node.previous; }
        else if (node.parent) { this.#node(node.parent).child = node.next; if (this.#tree.pushDebugMode === 4) this.#node(node.parent).child = node.parent; }
        if (node.next) this.#node(node.next).previous = node.previous; this.#hooks.redraw(token, true);
    }
    // 부착→교체 요청→머리 삽입→제한/정렬→redraw→현재 자식 재배치 순서다. bits는 원문 float의 직접 복사 경계다.
    attach(token, x, y, parent, mode, bits = {}) {
        x = Math.fround(x); y = Math.fround(y); parent >>>= 0; mode >>>= 0; let xBits = bits.xBits; let yBits = bits.yBits; const node = this.#node(token); recordGumpAttachDiagnostic(node, this.#tree.checking, this.#tree.typeLimit, this.#hooks.diagnostic); const destination = parent || node.parent; this.#node(destination);
        node.flags = (node.flags & ~VOID) >>> 0; if (parent) node.parent = parent;
        if (node.parent !== this.#state.collection.root && !(node.flags & OWN_DEPTH)) node.depth = (this.#node(node.parent).depth + (node.secondaryDepth || 0xfffffffd)) >>> 0;
        if (node.flags & REPLACE) {
            const seen = new Set();
            // kill 전 next를 저장하며 dead 항목도 같은 교체 요청에 포함한다.
            for (let current = this.#node(node.parent).child; current;) { enter(seen, current); const next = this.#node(current).next; if (this.#node(current).flags & REPLACE) this.#hooks.kill(current); current = next; }
        }
        node.previous = 0; node.next = this.#node(node.parent).child; this.#node(node.parent).child = token; if (node.next) this.#node(node.next).previous = token;
        if (!(mode & 1) && !(node.flags & UNCLAMPED)) {
            let origin = { x: 0, y: 0 };
            if (node.flags & WORLD) { if (typeof this.#hooks.project !== 'function') throw new Error('월드 Gump 부착의 투영 필요'); origin = this.#hooks.project(node.object); x = Math.fround(origin.x + x); y = Math.fround(origin.y + y); xBits = undefined; yBits = undefined; }
            if (node.parent === this.#state.collection.root) {
                if (this.#state.screenWidth < x + node.width) { x = Math.fround(this.#state.screenWidth - node.width); xBits = undefined; }
                if (this.#state.screenHeight < y + node.height) { y = Math.fround(this.#state.screenHeight - node.height); yBits = undefined; }
                if (x < 0) { x = 0; xBits = undefined; } if (y < 0) { y = 0; yBits = undefined; }
            }
            if (node.flags & WORLD) { x = Math.fround(x - origin.x); y = Math.fround(y - origin.y); xBits = undefined; yBits = undefined; }
        }
        store(node, 'x', x, xBits); store(node, 'y', y, yBits); if (mode & 2) { const position = this.align(token); store(node, 'x', position[0], position.xBits); store(node, 'y', position[1], position.yBits); }
        this.#hooks.redraw(token, false); const seen = new Set();
        // redraw 뒤 현재 child를 읽으며 각 next는 가상 Push 전에 저장한다.
        for (let child = node.child; child;) { enter(seen, child); const below = this.#node(child); const next = below.next; if (below.alignFlags || below.secondaryDepth) { this.#hooks.push(child); if (below.secondaryDepth) below.depth = (node.depth + below.secondaryDepth) >>> 0; const position = this.align(child); this.#hooks.attach(child, position[0], position[1], 0, 0, { xBits: position.xBits, yBits: position.yBits }); } child = next; }
    }
    // 모든 자식부터 갱신하며 재귀 뒤 현재 next를 읽는다. visible/dead/void 필터를 추가하지 않는다.
    #depth(token, value, add, active) {
        if (active.length >= GUMP_REGISTRY_CAPACITY || active.includes(token)) throw new Error('Gump 깊이 재귀 순환/한도 초과'); active.push(token); const node = this.#node(token); const seen = new Set();
        // 형제 체인 검사와 자식 우선 순서를 유지한다.
        for (let child = node.child; child; child = this.#node(child).next) { enter(seen, child); this.#depth(child, value, add, active); }
        node.depth = (add ? node.depth + value : value) >>> 0; active.pop();
    }
    // 현재 하위DWORD 깊이에 delta를 자식부터 더한다.
    addDepth(token, delta) { this.#depth(token, delta >>> 0, true, []); }
    // 현재 모든 하위 깊이를 지정DWORD로 쓴다.
    setDepth(token, depth) { this.#depth(token, depth >>> 0, false, []); }
    // 드래그 기준점은 Push 전에 읽고 현재 void를 조사한 뒤 가상 부착으로 넘긴다.
    move(token, x, y, parent, bits = {}) {
        const node = this.#node(token); let xBits = bits.xBits; let yBits = bits.yBits; x = Math.fround(x); y = Math.fround(y);
        if (node.flags & GRAB) { x = Math.fround(x - float(node.positioning[0])); y = Math.fround(y - float(node.positioning[1])); xBits = undefined; yBits = undefined; }
        if (!(node.flags & VOID)) this.#hooks.push(token); this.#hooks.attach(token, x, y, parent, 0, { xBits, yBits });
    }
    // 먼저 Push한 뒤 바뀐 기준 노드/부모/크기와 현재 anchor 하위BYTE를 사용한다.
    moveRelative(token, reference, x, y, anchor, parent, bits = {}) {
        const node = this.#node(token); let xBits = bits.xBits; let yBits = bits.yBits; x = Math.fround(x); y = Math.fround(y); anchor &= 255;
        if (!(node.flags & VOID)) this.#hooks.push(token); const base = this.#node(reference);
        if (reference !== node.parent) { x = Math.fround(x + base.x); y = Math.fround(y + base.y); xBits = undefined; yBits = undefined; }
        if (anchor & 8) { x = Math.fround(x + base.width); xBits = undefined; } if (anchor & 4) { y = Math.fround(y + base.height); yBits = undefined; }
        this.#hooks.attach(token, x, y, parent, 0, { xBits, yBits });
    }
}
