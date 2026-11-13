// 실제 Gump 기본160BYTE의 초기화/등록과 화면 크기의 루트 생성을 이식한다(cpppj client/GumpCreate.cpp/.h, 기준선1f00a25a).
// 사용: 같은 GumpCreateState와 안정된 파생 노드 저장소를 new GumpCreate(state,{node,allocate})에 빌린다. 트리 부착/삭제는 별도다. (2026-11-14 WB04-E410)

// 원본 등록 한도와 기본 new 요청 크기다.
export const GUMP_REGISTRY_CAPACITY = 1000;
export const GUMP_BASE_SIZE = 0xa0;
// 10.78 기본 VT와 등록 생략/초기 void 비트다. VT는 실행 포인터가 아닌 근거 식별값이다.
const PATCH_VTABLE = 0x50ac70; const UNREGISTERED = 0x100000; const VOID = 8;
// 원본 float 인자의 신호NaN까지 복사할 때 쓰는 숫자 보기다. 산술 결과는 새 비트를 가진다.
function fromBits(bits) { const view = new DataView(new ArrayBuffer(4)); view.setUint32(0, bits >>> 0, true); return view.getFloat32(0, true); }
// raw 기록/장치 경계는 현재 숫자와 같은 복사 비트만 빌린다. 후속 산술로 바뀐 값이면 현재 float32를 기록한다.
export function gumpNodeFloatBits(node, field) {
    const bits = node.floatBits?.[field]; if (bits !== undefined && Object.is(fromBits(bits), node[field])) return bits >>> 0;
    const view = new DataView(new ArrayBuffer(4)); view.setFloat32(0, node[field], true); return view.getUint32(0, true);
}

// 표시 수집/Draw와 같은 주소로 빌리는 기본/파생 노드의 공통 필드다.
export class GumpCreateNode {
    // DWORD 가상 표·표시/정렬 플래그와 형제/부모/자식 링크다.
    vtable = 0; flags = 0; alignFlags = 0; next = 0; previous = 0; parent = 0; child = 0;
    // 등록 번호·명령·월드 객체·Form·일련번호·기본/보조 깊이다. form은 기본 생성에서 보존한다.
    id = 0; command = 0; object = 0; form = 0; serial = 0; depth = 0; secondaryDepth = 0;
    // float32 배율/기본 크기/표시 크기와 로컬 위치다.
    scale = 0; baseWidth = 0; baseHeight = 0; width = 0; height = 0; x = 0; y = 0;
    // +54..68의 위치/속도 DWORD와 같은 표시 캐시다.
    positioning = new Uint32Array(6); cached = { x: 0, y: 0 };
    // +74는 기본 생성에서 보존한다. 나머지는 두 문자열 토큰/글자 문맥8DWORD다.
    untouched = 0; textPointer = 0; alternateTextPointer = 0; textContext = new Uint32Array(8);
    // 숫자로 표현할 수 없는 원래 신호NaN 비트다. raw 직렬화 경계가 현재 값과 함께 사용한다.
    floatBits = {};
}

// 한 Client의 현재 등록표/카운터/화면 크기와 이미 이식한 수집기 상태를 소유한다.
export class GumpCreateState {
    // 등록표의0번은 실제 루트이며 구멍은0이다. 같은 배열을 빌려 초기화할 수 있다.
    entries = new Uint32Array(GUMP_REGISTRY_CAPACITY);
    // 현재 등록 개수와 모든 생성의 DWORD 일련번호다.
    count = 0; serial = 0;
    // 원본 signed 화면 크기다. 루트 new 뒤 현재 값을 float32로 바꾼다.
    screenWidth = 0; screenHeight = 0;
    // 같은 수집기/Draw가 읽는 루트와 임시 머리다. 루트 초기화도 overrideHead를 지우지 않는다.
    collection = { root: 0, overrideHead: 0 };
}

// 파생 고유 필드/기본 배열 주소를 보존하면서 기본160BYTE의 작성 필드만 초기화한다.
function resetBase(node) {
    const form = node.form ?? 0; const untouched = node.untouched ?? 0; const positioning = node.positioning ?? new Uint32Array(6); const cached = node.cached ?? { x: 0, y: 0 }; const textContext = node.textContext ?? new Uint32Array(8);
    Object.assign(node, new GumpCreateNode(), { form, untouched, positioning, cached, textContext }); positioning.fill(0); cached.x = 0; cached.y = 0; textContext.fill(0); textContext[6] = 1; textContext[7] = 1;
}

// 같은 저장소만 초기화/등록하며 별도 표시 노드나 등록 사본을 만들지 않는다.
export class GumpCreate {
    // 장기 상태와 현재 노드/할당 장치다.
    #state; #hooks;
    // 노드 조회는 필수이고 allocate는 루트 초기화에서만 요구한다.
    constructor(state, hooks = {}) { if (typeof hooks.node !== 'function') throw new TypeError('Gump 생성 저장소 필요'); this.#state = state; this.#hooks = hooks; }
    // 작성하지 않는 form/untouched를 보존하고 현재 표의 첫 구멍/모든 생성의 serial을 갱신한다. bits.widthBits/heightBits는 원문 float의 직접 복사 비트다.
    construct(token, command, width, height, flags, bits = {}) {
        token >>>= 0; flags >>>= 0; const s = this.#state; const node = token ? this.#hooks.node(token) : null; if (!node) throw new Error('Gump 생성 토큰/저장소 없음');
        if (!(flags & UNREGISTERED) && (((s.count + 1) >>> 0) | 0) > 999) throw new RangeError('Gump 원본 개수 한도 초과');
        resetBase(node); node.vtable = PATCH_VTABLE; node.command = command >>> 0; node.baseWidth = node.width = bits.widthBits === undefined ? Math.fround(width) : fromBits(bits.widthBits); node.baseHeight = node.height = bits.heightBits === undefined ? Math.fround(height) : fromBits(bits.heightBits);
        if (bits.widthBits !== undefined) node.floatBits.baseWidth = node.floatBits.width = bits.widthBits >>> 0;
        if (bits.heightBits !== undefined) node.floatBits.baseHeight = node.floatBits.height = bits.heightBits >>> 0;
        node.flags = (flags | VOID) >>> 0; node.scale = 1; node.depth = 0xffffffd8; s.serial = (s.serial + 1) >>> 0; node.serial = s.serial;
        if (!(node.flags & UNREGISTERED)) {
            // 표가 가득 차도 id0과 개수 증가를 유지하며 첫 빈 칸 하나만 기록한다.
            for (let index = 0; index < s.entries.length; index += 1) if (!s.entries[index]) { s.entries[index] = token; node.id = index; break; }
            s.count = (s.count + 1) >>> 0;
        }
        s.collection.root = s.entries[0]; return token;
    }
    // 표만 비운 뒤 new(160)를 요청한다. 실패0도 표/루트0을 남기고 나머지 상태와 이전 객체는 보존한다.
    initializeRoot() {
        if (typeof this.#hooks.allocate !== 'function') throw new TypeError('Gump 루트 할당 경계 필요'); const s = this.#state; s.entries.fill(0); s.collection.root = 0; const token = this.#hooks.allocate(GUMP_BASE_SIZE) >>> 0;
        if (token) this.construct(token, 0, Math.fround(s.screenWidth), Math.fround(s.screenHeight), 1);
        s.entries[0] = token; s.collection.root = token; return token;
    }
    // 같은 기본/파생 노드 참조를 수집기에 제공한다. 월드 투영은 선택될 때 실제 장치가 필요하다.
    collectionHooks(project = null) { return { node: (token) => token ? this.#hooks.node(token) : null, project }; }
}
