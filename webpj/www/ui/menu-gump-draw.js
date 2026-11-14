// 원본 메뉴의 실제 배경/강조/구분선/체크 그림/항목 글자/하위 메뉴 표시를 같은 픽셀 대상에 출력한다.
// (cpppj client/MenuGumpDraw.cpp/.h, 기준선 1f00a25a, 10.78 전용 — CD 10.72 의 bottom Y 정수 유지(004523b0)는 옮기지 않는다)
// 이미 배율 적용된 크기/원본 여백을 사용하며 styled 글자의 색을 plain 문맥으로 덮어쓰지 않는다. 패치만 bottom 계산의 Y 를 float32 로 한 번 저장해 재사용한다.
// 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// (2026-11-14 WB04-E430: RawMenuControls 선행 MenuGumpDraw 이식)
import { GumpClipScope } from '../render/gump-draw.js';
import { decodeGumpTextContext } from '../render/gump-text-draw.js';
import { menuGumpItemBytes } from './menu-gump-layout.js';
import { MENU_ITEM_CAPACITY } from './menu-gump.js';

// 항목의 체크/선택/구분선/자식/굵게/비활성/스타일 문자열/동적 자식 비트다.
const CHECKED = 4; const SELECTED = 8; const SEPARATOR = 0x10; const CHILD = 0x20; const BOLD = 0x40; const DISABLED = 0x200; const STYLED = 0x400; const DYNAMIC = 0x800;
// 세로/compact 방향과 강조/구분선 배경 flags 다. 그림자 명암 번호와 Gump 표시 flags 를 혼동하지 않는다.
const VERTICAL = 0x80000; const COMPACT = 0x1000000; const HIGHLIGHT = 0x2100000; const SEPARATOR_FIRST = 0x2100000; const SEPARATOR_SECOND = 0x2080000;
// 원본 선택 안쪽 여백·구분선 길이/높이 보정·체크 그림 위치와 왼쪽 여백 비율이다.
const INSET = 2; const LINE_TRIM = 4; const HALF = 0.5; const LINE_RAISE = 1; const QUARTER = 0.25;
// 첫 자산 타입 번호(내장 타입은 70 미만)와 프레임 구간 글자 'J' 의 구간 번호('J'-'A')다.
const FIRST_ASSET_TYPE = 70; const J_LETTER = 9;
// 하위 메뉴 표시는 같은 원본 글꼴의 두 ASCII 바이트 " >" 와 NUL(길이 -1 출력에 필요)이다.
const CHILD_MARKER = Uint8Array.of(0x20, 0x3e, 0);

// CRT 64비트 절삭의 signed 하위 DWORD 이며 무효 정수 결과는 하위 DWORD 0 이다.
function ftol(value) { if (!Number.isFinite(value) || value < -9223372036854775808 || value >= 9223372036854775808) return 0; return Number(BigInt.asIntN(32, BigInt(Math.trunc(value)))); }
// 원본 화면 위치/그림 위치 덧셈의 DWORD 감김을 보존한다.
const add = (first, second) => (first + second) | 0;
// 선택된 장치가 없으면 같은 지점의 TypeError(C++ std::invalid_argument)로 거부한다.
function need(fn, message) { if (typeof fn !== 'function') throw new TypeError(message); return fn; }

// 원본 현재 흰색/비활성 회색·구분선 배경 타입/프레임·체크 그림 타입 전역이다. 미초기화 PE 값을 기본값으로 쓰지 않고 실제 초기화 값을 공급해야 한다.
export class MenuGumpDrawState {
    // 본문 출력의 팔레트 색 번호와 비활성 출력 색, 구분선 배경 타입/프레임, 선택·체크 표식 그림 타입이다.
    foreground = 0; disabled = 0; backgroundType = 0; backgroundFrame = 0; markerType = 0;
}

// 00476080 의 정상 출력과 임시 클립 복구다. 자식 장식/제목은 수집 목록에서 따로 출력한다.
// 사용: new MenuGumpDraw(createState, clipState, drawState, collection, backgroundDraw, gumpTextDraw, styleText, layout, { node, base, marker }).draw(token)
export class MenuGumpDraw {
    // 같은 생성/클립/전역과 기존 수집기·배경·글자·StyleText·레이아웃 처리기, 현재 가상 경계다.
    #create; #clip; #state; #collection; #background; #text; #style; #layout; #hooks;
    // 모든 참조와 출력 표면은 Draw 처리기 및 연결한 가상 훅보다 오래 살아야 한다. 메뉴 노드 조회만 필수로 확인한다.
    constructor(create, clip, state, collection, background, text, style, layout, hooks = {}) {
        if (typeof hooks.node !== 'function' || typeof hooks.base !== 'function') throw new TypeError('메뉴 출력 공유 저장소 필요');
        this.#create = create; this.#clip = clip; this.#state = state; this.#collection = collection; this.#background = background; this.#text = text; this.#style = style; this.#layout = layout; this.#hooks = hooks;
    }

    // 기본 배경/입력/출력이 같은 객체를 가리켜야 한다. 토큰 0 은 조회하지 않는다.
    #node(token) {
        const node = token ? this.#hooks.node(token) : null;
        if (!node || node !== this.#hooks.base(token)) throw new Error('메뉴 출력 공유 노드 없음');
        return node;
    }

    // 출력 콜백이 바꾼 항목 수도 이후 반복에 반영하되 배열 밖 접근은 거부한다.
    #item(token, index) {
        if (index < 0 || index >= MENU_ITEM_CAPACITY) throw new RangeError('메뉴 출력 항목 범위 오류');
        return this.#node(token).items[index];
    }

    // 00464930: 명시 float32 로컬 rect 를 현재 화면 Position 에 더해 signed 절삭한다. X 와 top Y 는 원래 정수의 확장값이고 패치만 bottom 계산의 Y 를 float32 로 저장해 재사용한다.
    #rect(token, local) {
        const point = this.#collection.position(token); const y = Math.fround(point.y);
        return { left: ftol(point.x + local[0]), top: ftol(point.y + local[1]), right: ftol(point.x + local[2]), bottom: ftol(y + local[3]) };
    }

    // 행/가로/compact 선택 배경은 현재 메뉴 type/frame 으로 0x2100000 배경을 출력한다. 원본이 넘기는 로컬 사각형 네 값은 float32 로 저장된다.
    #highlight(token, index) {
        const node = this.#node(token); let local;
        if (node.menuFlags & COMPACT) local = [2, 2, node.width - INSET, node.height - INSET];
        else if (node.menuFlags & VERTICAL) local = [2, node.offsets[index] + INSET, node.width - INSET, node.heights[index] + node.offsets[index] - INSET];
        else local = [index * node.itemWidth + INSET, 2, add(index, 1) * node.itemWidth - INSET, node.height - INSET];
        const rect = this.#rect(token, Float32Array.from(local)); const scope = new GumpClipScope(this.#clip, rect);
        try { this.#background.background(rect, HIGHLIGHT, this.#node(token).type, this.#node(token).frame); } finally { scope.dispose(); }
    }

    // 세로 구분선은 두 개의 1픽셀 높이 명암 배경이며 두 번째 pass 는 이전 화면 rect 의 top/bottom 을 1 씩 증가시킨다. 매 pass 클립을 개별 복구하고 각 pass 에서 현재 전역을 다시 읽는다.
    #separator(token, index) {
        const node = this.#node(token);
        const y = ftol((node.offsets[index] + node.margins[2]) - (node.targetHeight - node.height) + node.heights[index] * HALF - LINE_RAISE);
        const right = ftol(node.width + INSET - LINE_TRIM);
        const rect = this.#rect(token, Float32Array.of(2, y, right, add(y, 1)));
        // 두 번의 pass 로 위쪽 어두운 선과 아래쪽 밝은 선을 그린다(명암 flags 순서는 두 PE 에서 같다).
        for (let pass = 0; pass < 2; pass += 1) {
            rect.top = add(rect.top, pass); rect.bottom = add(rect.bottom, pass); const scope = new GumpClipScope(this.#clip, rect);
            try { this.#background.background(rect, pass ? SEPARATOR_SECOND : SEPARATOR_FIRST, this.#state.backgroundType, this.#state.backgroundFrame); } finally { scope.dispose(); }
        }
    }

    // styled 이면 저장 조각의 문맥을 쓰고(override 는 null 이다) plain 이면 항목의 60바이트/NUL 과 임시 문맥을 사용한다.
    #drawText(token, index, point, context) {
        const item = this.#item(token, index);
        if (item.flags & STYLED) this.#style.draw(item.styled, point, null);
        else this.#text.draw(menuGumpItemBytes(item), point, decodeGumpTextContext(context));
    }

    // 자체 경계 클립→기본 배경→Position→항목별 현재 문맥/강조/그림/글자/하위 표시→클립 복구다. 원본은 별도 부모 필터 없이 자신의 Draw 몸체를 실행하며 표시/삭제 필터는 실제 수집기에 맡긴다.
    draw(token) {
        const bounds = this.#collection.bounds(token); const scope = new GumpClipScope(this.#clip, bounds);
        try {
            this.#background.draw(token); const point = this.#collection.position(token);
            // 매 항목에서 현재 문맥/흰색/비활성 색을 복사하고 출력 콜백 뒤 현재 flags 를 다시 읽는다.
            for (let index = 0; index < (this.#node(token).itemCount | 0); index += 1) {
                const flags = this.#item(token, index).flags; const context = Uint32Array.from(this.#node(token).textContext); context[1] = 0; context[2] = this.#state.foreground;
                if (flags & BOLD) context[3] = 1;
                if (flags & DISABLED) { context[2] = this.#state.disabled; context[0] = 0; }
                if ((flags & SELECTED) && !(flags & SEPARATOR)) this.#highlight(token, index);
                if (!(this.#node(token).menuFlags & VERTICAL)) {
                    const y = ftol(point.y + this.#node(token).margins[2]); const x = ftol(index * this.#node(token).itemWidth + this.#node(token).margins[0] + point.x);
                    this.#drawText(token, index, { x, y }, context); continue;
                }
                if (this.#item(token, index).flags & SEPARATOR) { this.#separator(token, index); continue; }
                if (this.#item(token, index).flags & CHECKED) {
                    const x = ftol(this.#node(token).margins[0] * QUARTER - (this.#node(token).targetWidth - this.#node(token).width));
                    const y = ftol(this.#node(token).offsets[index] + this.#node(token).margins[2] - (this.#node(token).targetHeight - this.#node(token).height));
                    need(this.#hooks.marker, '메뉴 체크 SHP 출력 경계 필요')(this.#state.markerType, { x: add(add(point.x, x), 5), y: add(add(point.y, y), 7) });
                }
                const y = ftol(point.y + this.#node(token).offsets[index] + this.#node(token).margins[2] - (this.#node(token).targetHeight - this.#node(token).height));
                const x = ftol(point.x + this.#node(token).margins[0] - (this.#node(token).targetWidth - this.#node(token).width));
                this.#drawText(token, index, { x, y }, context);
                if (this.#item(token, index).flags & (CHILD | DYNAMIC)) {
                    const measured = this.#layout.measureItem(token, index, context);
                    const markerX = ftol((this.#node(token).margins[0] - (this.#node(token).targetWidth - this.#node(token).width)) + measured[0]);
                    const markerY = ftol(this.#node(token).offsets[index] + this.#node(token).margins[2] - (this.#node(token).targetHeight - this.#node(token).height));
                    this.#text.drawGump(this.#collection, token, CHILD_MARKER, { x: markerX, y: markerY }, this.#node(token).textContext, decodeGumpTextContext(context));
                }
            }
        } finally { scope.dispose(); }
    }
}

// 현재 분류 결과를 호출마다 조회한다. 메뉴이면 실제 Draw 로 보내고 다른 타입의 기존 실제 출력은 보존한다(원본의 가상 +24 분배).
// 사용: bindMenuGumpDraw(drawHooks, (token) => menus.get(token) ?? null)
export function bindMenuGumpDraw(hooks, resolve) {
    if (typeof resolve !== 'function') throw new TypeError('메뉴 출력 분배 조회 필요'); const previous = hooks.draw;
    // 임시 클립은 실제 메뉴 Draw 가 담당한다.
    hooks.draw = (token) => {
        const menu = resolve(token);
        if (menu) menu.draw(token);
        else { if (typeof previous !== 'function') throw new TypeError('메뉴 외 Draw 경계 필요'); previous(token); }
    };
}

// 체크 표시는 같은 장식 SHP 의 J 구간 첫 번호+2 를 출력한다. 실제 fortGump 에서는 J02(물리 60번)다.
// 원본: 10.78 00476080 / CD 10.72 004523b0 은 타입+154+2 를 사용한다. 0049b060 의 첫 번호 표는 +130, 개수 표는 +170 이다.
// 이력: 2026-10-27 수정 — +154 를 I 구간 개수로 오인해 B01(물리 6번)을 선택하던 표시 오류를 정정했다. (webpj 2026-11-14 WB04-E430 이식)
export function bindMenuGumpDrawAssets(hooks, assets, pixels) {
    const shape = pixels.hooks({}).shape;
    // 매 표시 요청의 현재 타입 정의를 조회하고 기존 원색 해제 캐시/클립/투명 합성을 사용한다.
    hooks.marker = (type, point) => {
        type >>>= 0; if (type < FIRST_ASSET_TYPE || type - FIRST_ASSET_TYPE >= assets.types().length) throw new RangeError('메뉴 체크 실제 타입 범위 초과');
        const first = assets.typeTable.view(type).first[J_LETTER]; shape(type, (first + 2) >>> 0, point);
    };
}
