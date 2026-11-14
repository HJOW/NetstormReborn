// 원본 MenuGump 항목 추가·문자열 설정·측정·전체 레이아웃을 복원한다.
// (cpppj client/MenuGumpLayout.cpp/.h, 기준선 1f00a25a, 10.78 전용 — CD 10.72 의 자식 연결 쓰기 순서(숨김→ID)는 옮기지 않는다)
// DWORD 감김·CRT 절삭과 콜백(Attach/StyleText/제목/크기 변경) 뒤 현재 항목 수·문맥·플래그를 다시 읽는 원본 순서를 유지한다.
// 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// (2026-11-14 WB04-E428: RawMenuControls 선행 MenuGumpLayout 이식)
import { MENU_ITEM_CAPACITY } from './menu-gump.js';
import { decodeGumpTextContext } from '../render/gump-text-draw.js';

// 항목 flags: 명령/구분선/자식/명시 인자/비활성/스타일 문자열/분리 자식/문자열 비트다.
const COMMAND = 1; const SEPARATOR = 0x10; const CHILD = 0x20; const PARAMETER = 0x80; const DISABLED = 0x200; const STYLED = 0x400; const DETACHED = 0x800; const TEXT = 0x10000;
// 메뉴 flags: 세로/가로/장식/compact 비트다.
const HORIZONTAL = 0x40000; const VERTICAL = 0x80000; const FANCY = 0x800000; const COMPACT = 0x1000000;
// 비활성 항목의 StyleText 접두(회색 색상 복귀+그림자 없음)와 원본 StyleText 파싱 버퍼 크기(NUL 포함)다.
const DISABLED_PREFIX = '~*~x'; const STYLE_BUFFER = 256;
// 자식 표시 " >" 와 종료 NUL 이다(자식 표식을 같은 글꼴로 측정할 때 쓴다).
const MARKER_BYTES = Uint8Array.of(0x20, 0x3e, 0);

// signed DWORD 덧셈은 x86 처럼 하위 32비트만 보존한다.
const add = (first, second) => (first + second) | 0;
// 항목 수(DWORD)를 signed 로 읽는다. 음수 입력은 원본처럼 반복을 생략한다.
const count = (node) => node.itemCount | 0;
// CRT _ftol 의 signed 64비트 절삭 결과에서 하위 signed DWORD 를 취한다. 무효 정수 결과의 하위 DWORD 는 0 이다.
function ftol(value) { if (!Number.isFinite(value) || value < -9223372036854775808 || value >= 9223372036854775808) return 0; return Number(BigInt.asIntN(32, BigInt(Math.trunc(value)))); }
// 선택된 장치가 없으면 같은 지점의 TypeError(C++ std::invalid_argument)로 거부한다.
function need(fn, message) { if (typeof fn !== 'function') throw new TypeError(message); return fn; }
// 원문 바이트 문자열(바이트 하나가 문자 하나) 또는 Uint8Array 를 첫 NUL 앞까지의 바이트 문자열로 바꾼다. null/undefined 는 원본 널 포인터와 같다.
function cString(text) {
    if (text === null || text === undefined) return null;
    let value = typeof text === 'string' ? text : Array.from(text, (byte) => String.fromCharCode(byte)).join('');
    const nul = value.indexOf('\0'); if (nul >= 0) value = value.slice(0, nul); return value;
}

// 원본 항목의 60바이트를 직렬화한다. NUL 이 없는 40바이트 라벨도 뒤 DWORD 의 바이트까지 측정할 수 있다.
// 사용: menuGumpItemBytes(node.items[0])
export function menuGumpItemBytes(item) {
    const result = new Uint8Array(60); result.set(item.label, 0);
    const words = [item.childId, item.command, item.parameter, item.styled, item.flags];
    // 라벨 뒤 다섯 DWORD 를 원본 필드 순서대로 쓴다.
    for (let i = 0; i < words.length; i += 1) {
        // 각 DWORD 의 낮은 BYTE 부터 네 바이트를 기록한다.
        for (let b = 0; b < 4; b += 1) result[40 + i * 4 + b] = (words[i] >>> (b * 8)) & 255;
    }
    return result;
}

// 같은 메뉴 프레임/기본 저장소를 사용하며 문자열 처리 뒤 실제 전체 크기를 다시 계산한다.
// 사용: new MenuGumpLayout(createState, menu, { node, baseNode, plain, styled, marker, clearStyle, parseStyle, push, titleFit, titleLayout, attach }).append(token, '항목', 0x10000)
export class MenuGumpLayout {
    // 생성 상태/메뉴 크기 처리기와 현재 가상 경계다.
    #create; #menu; #hooks;
    // 파생/기본 조회만 필수다. 텍스트/제목/부착 경계는 실제 도달 시 요구한다. 생성 상태·메뉴 처리기와 안정된 저장소는 처리기보다 오래 살아야 한다.
    constructor(create, menu, hooks = {}) {
        if (typeof hooks.node !== 'function' || typeof hooks.baseNode !== 'function') throw new TypeError('메뉴 레이아웃 공유 저장소 필요');
        this.#create = create; this.#menu = menu; this.#hooks = hooks;
    }

    // 다른 복사본을 반환하는 파생 조회를 거부한다. 토큰 0 은 조회하지 않는다.
    #node(token) {
        const node = token ? this.#hooks.node(token) : null;
        if (!node || node !== this.#hooks.baseNode(token)) throw new Error('메뉴 레이아웃 저장소 없음');
        return node;
    }

    // 자식/제목은 기본 Gump 파생 타입도 허용한다.
    #baseNode(token) {
        const node = token ? this.#hooks.baseNode(token) : null;
        if (!node) throw new Error('메뉴 자식/제목 저장소 없음');
        return node;
    }

    // 항목 수가 콜백에서 손상되어도 원본 배열 밖 호스트 메모리를 읽거나 쓰지 않는다.
    #item(token, index) {
        const node = this.#node(token); index >>>= 0;
        if (index >= node.items.length) throw new RangeError('메뉴 항목 범위 오류');
        return node.items[index];
    }

    // 항목을 하나 추가한다(00477450). 40개 한도·선두/연속 구분선 생략·자식 숨김/부착·조건부 명령/인자를 보존한다.
    // 명령/인자/자식은 조건에 따라 일부 필드만 쓰며 미작성 값은 보존한다. 자식 부착/StyleText 콜백 뒤 바뀐 현재 항목 수를 다시 읽고, 이미 쓴 필드는 예외 시에도 유지한다.
    // text 는 NUL 로 끝나는 바이트 문자열(또는 Uint8Array)이다. 사용: append(token, '항목', 0x10000 | 0x80, 5, 7)
    append(token, text, flags, command = 0, parameter = 0, child = 0) {
        const node = this.#node(token); flags >>>= 0; child >>>= 0;
        if (count(node) >= MENU_ITEM_CAPACITY) return;
        if (flags & SEPARATOR) {
            const first = cString(text);
            if (child || (first !== null && first.length > 0)) throw new TypeError('메뉴 구분선의 빈 문자열/자식 없음 필요');
            if (count(node) < 1 || (this.#item(token, (node.itemCount - 1) >>> 0).flags & SEPARATOR)) return;
        }
        if (flags & COMMAND) this.#item(token, node.itemCount).command = command >>> 0;
        if (flags & PARAMETER) this.#item(token, node.itemCount).parameter = parameter >>> 0;
        if (child) {
            const below = this.#baseNode(child);
            if (flags & DETACHED) this.#item(token, node.itemCount).childId = below.id >>> 0;
            else {
                // 패치는 ID 저장→숨김 순서이며 같은 가상 부착에 도달한다.
                this.#item(token, node.itemCount).childId = below.id >>> 0; below.flags = (below.flags & ~1) >>> 0;
                need(this.#hooks.attach, '메뉴 자식 Attach 경계 필요')(child, 0, 0, token, 0); flags = (flags | CHILD) >>> 0;
            }
        }
        this.#item(token, node.itemCount).flags = (flags | TEXT) >>> 0;
        const index = node.itemCount; const context = Uint32Array.from(node.textContext); this.setText(token, index, text, context);
        node.itemCount = (node.itemCount + 1) >>> 0;
        if (count(node) > MENU_ITEM_CAPACITY) throw new RangeError('메뉴 추가 후 항목 수 한도 초과');
        this.reflow(token);
    }

    // plain 은 strncpy(40)로 NUL 이후를 0 으로 채우고 40번째 뒤에 별도 종료 문자를 쓰지 않는다(00475dd0).
    // styled 는 문맥 font=0·Clear→비활성 접두→Parse 순서이며 StyleText 의 256바이트 원본 버퍼를 넘는 입력은 거부한다.
    setText(token, index, text, context) {
        const item = this.#item(token, index); const source = cString(text); if (source === null) throw new TypeError('메뉴 항목 문자열 필요');
        if (!(item.flags & STYLED)) {
            let ended = false;
            // NUL 을 읽은 뒤 나머지를 0 으로 채운다. 입력이 40바이트보다 짧으면 끝으로 본다.
            for (let i = 0; i < item.label.length; i += 1) { const value = ended || i >= source.length ? 0 : source.charCodeAt(i) & 255; item.label[i] = value; if (!value) ended = true; }
            return;
        }
        const copy = Uint32Array.from(context); copy[1] = 0;
        need(this.#hooks.clearStyle, '메뉴 StyleText 소유 경계 필요')(token, index);
        const styled = ((item.flags & DISABLED) ? DISABLED_PREFIX : '') + source;
        if (styled.length >= STYLE_BUFFER) throw new RangeError('메뉴 StyleText 원본 버퍼 한도 초과');
        need(this.#hooks.parseStyle, '메뉴 StyleText 파싱 경계 필요')(token, index, styled, copy);
    }

    // styled 이면 항목 +34 측정으로, 그 외에는 전달 문맥의 기본 글자 측정으로 위임한다(00475d70). 측정 함수가 읽는 시점의 항목 플래그로 선택한다.
    measureItem(token, index, context) {
        if (this.#item(token, index).flags & STYLED) { const size = need(this.#hooks.styled, '메뉴 StyleText 측정 경계 필요')(token, index); return [size[0] | 0, size[1] | 0]; }
        const size = need(this.#hooks.plain, '메뉴 일반 글자 측정 경계 필요')(token, index, context); return [size[0] | 0, size[1] | 0];
    }

    // 현재 항목 수·문맥·플래그를 매 반복/콜백 뒤 다시 읽어 두 방향 크기/offset 을 쓴다(00476f60). 남는 배열 칸은 초기화하지 않는다.
    reflow(token) {
        const node = this.#node(token); const vertical = (node.menuFlags & VERTICAL) !== 0;
        node.margins.set([vertical ? 15 : 4, vertical ? 15 : 4, 1, 1]);
        if (node.menuFlags & COMPACT) { node.margins[0] = 3; node.margins[1] = 3; }
        let width = 0; let height = 0; let total = 0;
        // 원본은 매 항목에서 현재 기본 문맥을 복사하고 콜백 뒤 현재 항목 수를 비교한다.
        for (let i = 0; i < count(node); i += 1) {
            const context = Uint32Array.from(node.textContext); const size = this.measureItem(token, i, context);
            if ((node.menuFlags & VERTICAL) && (this.#item(token, i).flags & (CHILD | DETACHED))) {
                const marker = need(this.#hooks.marker, '메뉴 자식 표시 측정 경계 필요')(token, node.textContext); size[0] = add(size[0], marker[0] | 0);
                if (size[1] <= (marker[1] | 0)) size[1] = marker[1] | 0;
            }
            size[0] = ftol(size[0] + node.margins[0] + node.margins[1]);
            size[1] = ftol(size[1] + node.margins[2] + node.margins[3]);
            width = Math.max(width, size[0]); height = Math.max(height, size[1]); total = add(total, size[1]);
            node.heights[i] = size[1];
        }
        if (node.menuFlags & FANCY) {
            if (node.menuFlags & HORIZONTAL) throw new TypeError('장식 메뉴의 세로 방향 필요');
            need(this.#hooks.push, '메뉴 제목 Push 경계 필요')(node.title);
            need(this.#hooks.titleFit, '메뉴 제목 Fit 경계 필요')(node.title, 0);
            need(this.#hooks.titleLayout, '메뉴 제목 Layout 경계 필요')(node.title, 0, 0);
            const title = node.title; const above = this.#baseNode(title);
            width = ftol(width > above.baseWidth ? width : above.baseWidth);
            need(this.#hooks.attach, '메뉴 제목 Attach 경계 필요')(title, above.x, above.y, 0, 0);
        }
        if (node.menuFlags & VERTICAL) {
            if (node.menuFlags & HORIZONTAL) throw new TypeError('일반 세로 메뉴의 가로 비트 없음 필요');
            node.targetWidth = Math.fround(width); node.targetHeight = Math.fround(total);
            this.#menu.setWidth(token, node.targetWidth); this.#menu.setHeight(token, node.targetHeight); let offset = 0;
            // 크기 변경 콜백 뒤 현재 항목 수와 측정 높이를 다시 읽어 누적 offset 을 계산한다.
            for (let i = 0; i < count(node); i += 1) { this.#item(token, i); node.offsets[i] = offset; offset = ftol(offset + node.heights[i]); }
        } else if (node.menuFlags & HORIZONTAL) {
            node.targetWidth = Math.fround(Math.imul(node.itemCount, width)); node.targetHeight = Math.fround(height);
            const commonHeight = Math.fround(height);
            this.#menu.setWidth(token, node.targetWidth); this.#menu.setHeight(token, node.targetHeight);
            // 모든 현재 항목 높이를 같은 높이로 맞추고 위치 offset 은 0 으로 쓴다.
            for (let i = 0; i < count(node); i += 1) { this.#item(token, i); node.heights[i] = commonHeight; node.offsets[i] = 0; }
        }
        node.itemWidth = Math.fround(width);
    }
}

// 일반 항목과 자식 표시의 측정을 실제 글꼴 캐시(GumpText)에 연결한다. 호출 때 현재 노드 저장소에서 항목을 조회한다.
// 사용: bindMenuLayoutText(layoutHooks, gumpText)
export function bindMenuLayoutText(hooks, text) {
    const lookup = hooks.node; if (typeof lookup !== 'function') throw new TypeError('메뉴 글자 측정 저장소 필요');
    // 항목 60바이트(라벨+필드 바이트)를 전달 문맥의 글꼴로 측정한다.
    hooks.plain = (token, index, context) => {
        const node = lookup(token); if (!node || index >= node.items.length) throw new RangeError('메뉴 글자 측정 항목 범위 오류');
        const size = { width: 0, height: 0 }; text.measure(menuGumpItemBytes(node.items[index]), decodeGumpTextContext(context), size); return [size.width | 0, size.height | 0];
    };
    // 원본 두 BYTE 문자와 NUL 을 같은 글꼴로 측정한다.
    hooks.marker = (token, context) => {
        void token; const size = { width: 0, height: 0 }; text.measure(MARKER_BYTES, decodeGumpTextContext(context), size); return [size.width | 0, size.height | 0];
    };
}
