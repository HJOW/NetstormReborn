// 한 줄 TextGump 와 다중 줄 표시기의 원본 크기·클립·기준선·스크롤·조각 선택을 이식한다.
// (cpppj client/TextGump.cpp/.h, 기준선 1f00a25a, 10.78 전용 — CD 10.72 의 vtable·float32 중간 저장·NaN 높이 순회·dirty 순서는 옮기지 않는다)
// 원본의 DWORD 감김·float32 저장 위치(정수를 먼저 float 로 좁히지 않고 현재 scale 과 곱한 뒤 저장)·현재 필드 재조회, 표시/선택의 서로 다른 세로 경계를 유지한다.
// 다중 줄 클래스의 이름은 호스트 구분이며 원본 클래스 이름을 확정한 것이 아니다. 전체 다중 줄 생성자·입력은 ScrollingTextGumpWidgets 단위의 몫이다.
// 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// (2026-11-14 WB04-E423: RawMenuControls 선행 TextGump 이식)
import { GumpCreateNode } from './gump-create.js';
import { GUMP_DRAW_CLIP, GumpClipScope } from '../render/gump-draw.js';
import { styleTextField } from './style-text-fragments.js';

// 10.78 한 줄 TextGump 가상 표 식별값이다(실행 포인터가 아닌 근거 식별값).
const PATCH_TEXT_VTABLE = 0x005144d4;
// 다중 줄 노드 textFlags 의 dirty 비트와 slider 사용 비트다.
const DIRTY = 1; const SLIDER = 2;
// 원본 임시 레이아웃 폭과 맨 아래 판정 여유의 ROM float 값이다.
const FIT_WIDTH = 10000; const BOTTOM_TOLERANCE = 10;
// 원본 signed 문자열 길이 상한이다.
const INT32_MAXIMUM = 0x7fffffff;
// 원본 정수 ADD/SUB 의 하위 DWORD 를 signed 로 해석한다.
const add = (first, second) => (first + second) | 0;
const subtract = (first, second) => (first - second) | 0;
// CRT _ftol 의 signed 64비트 절삭 결과의 하위 DWORD 다. 무효 변환의 하위 DWORD 는 0 이다.
function ftol(value) { if (!Number.isFinite(value) || value < -9223372036854775808 || value >= 9223372036854775808) return 0; return Number(BigInt.asIntN(32, BigInt(Math.trunc(value)))); }
// 산술 결과는 새 float 저장이다. 원문 직접 복사의 신호 NaN 비트를 재사용하지 않는다.
function setDimension(node, field, value) { node[field] = Math.fround(value); if (node.floatBits) delete node.floatBits[field]; }
// 순환 목록만 거부하고 정상 토큰의 가상 측정/출력 호출 수와 순서는 바꾸지 않는다.
function visit(seen, token) { if (seen.has(token)) throw new Error('텍스트 표시 목록 순환'); seen.add(token); }
// 패치는 x87 C0/C2 parity 로 unordered(NaN)를 제외한다: 줄 시작 y 가 viewport 높이 아래이면 참이다.
const belowViewport = (y, height) => y < height;
// 선택된 장치가 없으면 같은 지점의 TypeError(C++ std::invalid_argument)로 거부한다.
function need(fn, message) { if (typeof fn !== 'function') throw new TypeError(message); return fn; }

// 원본 TextGump +a0 의 소유 체인이다. 기본 생성/표시/수명 저장소와 같은 객체여야 한다.
export class TextGumpNode extends GumpCreateNode {
    // 이 한 줄 TextGump 가 소유한 StyleText 조각 체인의 머리 토큰이다.
    head = 0;
}

// 실제 생성자가 +20(command)에 signed 로 확장하는 종류 BYTE 전역이다. 기본값 45 는 배포 PE 의 초기값이며 호출자가 바꿀 수 있다.
export class TextGumpState {
    // 기존 이름을 유지한 종류 BYTE 이며 위치 정렬 필드와 구별한다.
    alignment = 45;
}

// 메뉴 제목 등에 쓰는 원본 한 줄 클래스다. StyleText 조각/글꼴/그림과 실제 Gump 클립을 공유한다.
// 사용: new TextGump(create, state, factory, lifetime, collection, clipState, style, { node, baseNode }).
export class TextGump {
    // 같은 생성/수명/수집 저장소와 클립 상태·StyleText·노드 조회 장치다.
    #create; #state; #factory; #lifetime; #collection; #clip; #style; #hooks;
    // 파생/기본 노드를 같은 주소로 조회하는 두 경계는 필수다. 모든 처리기/전역/안정된 노드의 수명은 가상 Draw/소멸 분배보다 길어야 한다.
    constructor(create, state, factory, lifetime, collection, clip, style, hooks = {}) {
        if (typeof hooks.node !== 'function' || typeof hooks.baseNode !== 'function') throw new TypeError('TextGump 공유 저장소 필요');
        this.#create = create; this.#state = state; this.#factory = factory; this.#lifetime = lifetime; this.#collection = collection; this.#clip = clip; this.#style = style; this.#hooks = hooks;
    }

    // null/미등록/서로 다른 기본·파생 저장소를 호스트 오류로 거부한다. 기본 수집/생성과 다른 복사본을 출력하는 잘못된 연결은 실제 호출 전에 걸러낸다.
    #node(token) {
        const node = token ? this.#hooks.node(token) : null;
        if (!node || node !== this.#hooks.baseNode(token)) throw new Error('TextGump 공유 노드 없음');
        return node;
    }

    // 기본 (0,0,0,1) 생성 뒤 가상 표/head/정렬/크기/32바이트 문맥/클립 플래그를 쓴다. 정수 폭/높이를 먼저 float 로 좁히지 않고 현재 scale 과 곱한다.
    // 원본 기본 생성은 기존 head 를 반납하지 않는다(소유 체인이 이미 있어도 0 으로 쓴다). 사용: textGump.construct(token, 200, 20, font, color).
    construct(token, width, height, font, color) {
        this.#node(token); this.#factory.construct(token, 0, 0, 0, 1); const node = this.#node(token);
        node.vtable = PATCH_TEXT_VTABLE;
        node.head = 0;
        node.command = ((this.#state.alignment << 24) >> 24) >>> 0;
        width |= 0; height |= 0;
        setDimension(node, 'baseWidth', width); setDimension(node, 'width', width * node.scale);
        setDimension(node, 'baseHeight', height); setDimension(node, 'height', height * node.scale);
        node.textContext.set([0, font >>> 0, color >>> 0, 0, 0, 0, 1, 1]);
        node.flags = (node.flags | GUMP_DRAW_CLIP) >>> 0;
        return token;
    }

    // 현재 소유 체인을 scalar 삭제하고 head 를 0 으로 쓴다(공통 StyleText.clear 의 삭제/통지 순서 그대로).
    clear(token) { this.#style.clear(styleTextField(this.#node(token), 'head')); }

    // -1 만 범위 안 NUL 탐색이다. 길이 0 은 조각을 만들지 않고, 그 밖의 음수는 호스트에서 거부한다. 나머지는 명시 길이이며 태그 0 으로 소유 체인에 붙인다.
    // 조각을 먼저 만든 뒤 노드를 조회한다(원본 순서). 사용: textGump.appendText(token, bytes, context8).
    appendText(token, bytes, context, length = -1) {
        length |= 0;
        if (length === -1) {
            const end = bytes.indexOf(0);
            if (end < 0) throw new TypeError('TextGump NUL 범위 필요');
            if (end > INT32_MAXIMUM) throw new RangeError('TextGump 문자열 길이 초과');
            length = end;
        }
        if (!length) return;
        if (length < 0) throw new TypeError('TextGump 명시 길이 음수');
        const fragment = this.#style.newText(bytes, context, length >>> 0);
        this.#style.append(styleTextField(this.#node(token), 'head'), fragment);
    }

    // 전체 조각 측정 뒤 정수 여백을 DWORD 로 더하고 기본 크기는 float32 로 저장한다. 배율 크기는 저장 전 정수 값에 측정 콜백이 바꾼 현재 scale 을 곱한다.
    fit(token, padWidth = 0, padHeight = 0) {
        const size = this.#style.measure(styleTextField(this.#node(token), 'head'), { width: 0, height: 0 }); const node = this.#node(token);
        const width = add(size.width, padWidth | 0); setDimension(node, 'baseWidth', width); setDimension(node, 'width', width * node.scale);
        const height = add(size.height, padHeight | 0); setDimension(node, 'baseHeight', height); setDimension(node, 'height', height * node.scale);
    }

    // 빈 head 는 조기 반환한다. 그 밖에는 독립 내부 클립 → 현재 위치 → 현재 head 출력 순서이며 공통 Gump Draw 의 바깥 클립에 플래그가 없어도 내부 클립은 적용한다.
    draw(token) {
        if (!this.#node(token).head) return;
        const bounds = this.#collection.bounds(token); const scope = new GumpClipScope(this.#clip, bounds);
        try { const position = this.#collection.position(token); this.#style.draw(this.#node(token).head, position); } finally { scope.dispose(); }
    }

    // 파생 가상 표 → clear 두 번 → 기본 scalar 소멸 순서다(두 번째 clear 는 원본 멤버 소멸 순서이며 첫 삭제 콜백이 갱신한 현재 head 를 다시 읽는다). 반환은 입력 토큰이다.
    deleteScalar(token, flags) {
        this.#node(token).vtable = PATCH_TEXT_VTABLE;
        this.clear(token); this.clear(token); return this.#lifetime.deleteScalar(token, flags);
    }
}

// 원본 다중 줄 TextGump 의 slider/버퍼/소유·표시·입력 필드다. 생성/수명/입력은 ScrollingTextGumpWidgets 가 맡는다.
export class ScrollingTextGumpNode extends GumpCreateNode {
    // 스크롤바 토큰과 연결 버퍼 토큰이다.
    slider = 0; buffer = 0;
    // 현재 스크롤 위치, 대기 중인 스크롤 요청(-1 없음, -2 맨 아래), 맨 아래 추가 여백이다.
    scroll = 0; scrollRequest = -1; bottomPadding = 0;
    // +a4 textFlags(bit0 dirty, bit1 slider 사용)다.
    textFlags = 0;
    // +a8 소유 여부/+ac 미작성이다.
    ownsBuffer = 0; unwritten = 0;
    // +b0..b4 보조 칸이다.
    reserved = [0, 0];
    // +c0 마지막 배치 폭(float32), +c4 이전 drag y 다.
    layoutWidth = 0; dragY = 0;
    // +c8 태그, +d4 콜백 토큰이다.
    metadata = 0; callback = 0;
}

// 다중 줄 TextGump 표시/스크롤 처리기다. 조각 표는 style/모든 연결 버퍼와 같아야 한다. 스크롤바 가상 경계는 실제 도달 시 검사한다.
// hooks: { node, baseNode, buffer(token)→{ state, handler }, sliderRange(token,min,max), sliderPage(token,page), sliderSetPosition(token,position), sliderPosition(token), redraw(token,erase) }.
// 조회는 안정된 저장소를 반환해야 하며 출력 도중 현재 표시 줄/조각을 반납하거나 buffer 를 교체하면 안 된다.
export class ScrollingTextGump {
    // 같은 생성 상태·수집기·클립·조각 표·StyleText·가상 경계다.
    #create; #collection; #clip; #fragments; #style; #hooks;
    // node/baseNode/buffer 조회만 생성 시 요구한다. 미복원 slider 동작을 가짜 기본값으로 대체하지 않는다.
    constructor(create, collection, clip, fragments, style, hooks = {}) {
        if (typeof hooks.node !== 'function' || typeof hooks.baseNode !== 'function' || typeof hooks.buffer !== 'function') throw new TypeError('다중 줄 TextGump 공유 저장소 필요');
        this.#create = create; this.#collection = collection; this.#clip = clip; this.#fragments = fragments; this.#style = style; this.#hooks = hooks;
    }

    // 기본 생성/수집/파생 표시가 같은 주소를 사용하도록 실제 호출 시 확인한다.
    #node(token) {
        const node = token ? this.#hooks.node(token) : null;
        if (!node || node !== this.#hooks.baseNode(token)) throw new Error('다중 줄 TextGump 공유 노드 없음');
        return node;
    }

    // slider 의 현재 기본 폭은 동일한 기본 Gump 저장소에서 읽는다.
    #baseNode(token) { const node = token ? this.#hooks.baseNode(token) : null; if (!node) throw new Error('TextGump slider 노드 없음'); return node; }

    // handler/state 는 같은 원본 버퍼를 나타내야 한다. 소멸한 버퍼를 layout/표시하는 입력은 거부한다. 반환 { state, handler }.
    #buffer(token) {
        const buffer = this.#hooks.buffer(this.#node(token).buffer);
        if (!buffer?.state || !buffer.handler || !buffer.state.initialized) throw new Error('TextGump 연결 버퍼 없음');
        return buffer;
    }

    // 패치는 버퍼 dirty 를 먼저 지우고 노드 비트를 지운다. 이후 layout 의 실패도 저장은 되돌리지 않는다.
    #clearDirty(token) { this.#buffer(token).state.dirty = 0; const node = this.#node(token); node.textFlags = (node.textFlags & ~DIRTY) >>> 0; }

    // 기본 폭에서(slider 사용 비트이면) 현재 slider 의 기본 폭을 빼고 x87 중간값을 유지한 채 CRT 절삭해 버퍼에 폭으로 넘긴다.
    #layout(token) {
        const node = this.#node(token); let width = node.baseWidth;
        if (node.textFlags & SLIDER) width -= this.#baseNode(node.slider).baseWidth;
        this.#buffer(token).handler.layout(ftol(width));
    }

    // 필수 가상 redraw(false)를 호출한다. 요청 저장은 호출자가 이미 끝낸 뒤다.
    #redraw(token) { need(this.#hooks.redraw, 'TextGump Redraw 경계 필요')(token, false); }

    // 두 dirty 조건 중 하나가 참일 때만 두 플래그를 먼저 비우고 현재 폭으로 layout 한다. 폭 0 이하에서 이전 표시 목록을 유지하는 동작은 버퍼에 위임한다.
    ensureLayout(token) {
        if (!(this.#node(token).textFlags & DIRTY) && !this.#buffer(token).state.dirty) return;
        this.#clearDirty(token); this.#layout(token);
    }

    // 강제 layout 뒤 전체 표시 높이 + 1 을 기본/배율 높이에 저장한다(빈 버퍼도 높이 1).
    fitHeight(token) {
        this.#clearDirty(token); this.#layout(token);
        const height = add(this.#buffer(token).handler.totalHeight(), 1); const node = this.#node(token);
        setDimension(node, 'baseHeight', height); setDimension(node, 'height', height * node.scale);
    }

    // 임시 폭 10000 으로 배치한 뒤 최대 폭 + 가로 여백 + (slider 사용이면 slider 기본 폭) + 1 과 전체 높이 + 세로 여백으로 맞춘다.
    // 배율 폭은 base 폭의 float32 좁힘 전 합계로 계산한다. 사용: fitContent(token, 8, 4).
    fitContent(token, padWidth = 0, padHeight = 0) {
        const node = this.#node(token); setDimension(node, 'baseWidth', FIT_WIDTH); setDimension(node, 'width', FIT_WIDTH * node.scale);
        this.#clearDirty(token); this.#layout(token);
        const extra = this.#node(token).textFlags & SLIDER ? this.#baseNode(this.#node(token).slider).baseWidth : 0;
        const width = this.#buffer(token).handler.maximumWidth() + (padWidth | 0) + extra + 1;
        setDimension(node, 'baseWidth', width); setDimension(node, 'width', width * node.scale);
        const height = add(this.#buffer(token).handler.totalHeight(), padHeight | 0);
        setDimension(node, 'baseHeight', height); setDimension(node, 'height', height * node.scale);
    }

    // 명시 문맥 입력이다. 버퍼 appendText → 현재 textFlags|dirty → 가상 redraw(false) 순서이며 문자열 통지/버퍼 추가 콜백 이후의 현재 textFlags 를 갱신한다.
    appendText(token, bytes, context, length = -1, metadata = 0) {
        this.#buffer(token).handler.appendText(bytes, context, length, metadata);
        const node = this.#node(token); node.textFlags = (node.textFlags | DIRTY) >>> 0; this.#redraw(token);
    }

    // 현재 기본 32바이트 문맥의 복사본을 명시 문맥 입력에 넘긴다(onAppend 가 원래 문맥을 바꾸어도 이번 입력은 복사본을 쓴다).
    appendTextDefault(token, bytes, length = -1, metadata = 0) { const context = Uint32Array.from(this.#node(token).textContext); this.appendText(token, bytes, context, length, metadata); }

    // 현재 버퍼에 그림을 추가하고 dirty 비트/redraw(false)를 문자열과 같은 순서로 쓴다(그림은 버퍼 문자열 통지를 호출하지 않는다).
    appendPicture(token, pictures, type, frame, metadata = 0, fitWidth = 0, fitHeight = 0) {
        this.#buffer(token).handler.appendPicture(pictures, type, frame, metadata, fitWidth, fitHeight);
        const node = this.#node(token); node.textFlags = (node.textFlags | DIRTY) >>> 0; this.#redraw(token);
    }

    // 기본 문맥을 먼저 복사하고 완료 입력 줄만 제거한 뒤 일반 문자열을 추가한다. 미완료 끝 줄·기존 표시 복제·현재 태그/스크롤 요청은 보존하며 스타일 명령을 해석하지 않는다.
    replaceText(token, bytes, metadata = 0) {
        const context = Uint32Array.from(this.#node(token).textContext); const buffer = this.#buffer(token);
        // 각 제거 직후 같은 버퍼의 현재 완료 개수를 다시 읽는다. 표시 복제는 다음 layout 까지 남는다.
        while (buffer.state.lineCount !== 0) buffer.handler.removeLine(0);
        buffer.handler.appendText(bytes, context, -1, metadata); const node = this.#node(token); node.textFlags = (node.textFlags | DIRTY) >>> 0; this.#redraw(token);
    }

    // 현재 높이/내용으로 위치를 제한한 뒤 slider range → page → position → 현재 값 재조회를 한다. 저장한 내용/viewport 는 세 콜백 동안 유지하고 slider 노드/scroll 은 각 호출 직전에 재조회한다.
    setScroll(token, position) {
        position |= 0; const height = this.#buffer(token).handler.totalHeight(); const viewport = this.#node(token).baseHeight;
        const page = ftol(viewport < 0 ? 0 : viewport);
        if (height < add(page, position)) position = subtract(height, page);
        if (position < 0) position = 0;
        this.#node(token).scroll = position;
        need(this.#hooks.sliderRange, 'TextGump slider range 경계 필요')(this.#node(token).slider, 0, height);
        need(this.#hooks.sliderPage, 'TextGump slider page 경계 필요')(this.#node(token).slider, page);
        need(this.#hooks.sliderSetPosition, 'TextGump slider position 쓰기 경계 필요')(this.#node(token).slider, this.#node(token).scroll);
        this.#node(token).scroll = need(this.#hooks.sliderPosition, 'TextGump slider position 조회 경계 필요')(this.#node(token).slider) | 0;
    }

    // 내용 높이 - 기본 높이를 CRT 절삭해 맨 아래로 간다. extra 는 원본 ret 4 의 인자지만 몸체에서 읽지 않는다.
    scrollBottom(token, extra = 0) { void extra; const content = this.#buffer(token).handler.totalHeight(); this.setScroll(token, ftol(content - this.#node(token).baseHeight)); }

    // 요청을 저장하고 redraw(false)를 호출한다(음수도 저장한다). 적용은 processRequest 에서 한다.
    requestScroll(token, position) { this.#node(token).scrollRequest = position | 0; this.#redraw(token); }

    // 현재 scroll + delta 를 DWORD 로 더하고 음수이면 요청 0 으로 제한한다(현재 대기 요청에 delta 를 더하지 않는다).
    requestRelativeScroll(token, delta) { this.requestScroll(token, Math.max(add(delta | 0, this.#node(token).scroll), 0)); }

    // -2 요청과 추가 여백을 저장한 뒤 redraw(false)를 호출한다(패치 저장 순서: 요청 → 여백).
    requestBottom(token, extra = 0) { const node = this.#node(token); node.scrollRequest = -2; node.bottomPadding = extra | 0; this.#redraw(token); }

    // -1 은 무동작, -2 는 맨 아래, 나머지는 setScroll 뒤 현재 요청을 -1 로 쓴다. 콜백에서 새 요청을 써도 정상 반환 뒤 -1 로 덮고, 예외로 끝나면 정상 반환 저장은 수행하지 않는다.
    processRequest(token) {
        const request = this.#node(token).scrollRequest; if (request === -1) return;
        if (request === -2) this.scrollBottom(token, this.#node(token).bottomPadding);
        else {
            if (request < 0) throw new TypeError('TextGump scrollRequest 음수');
            this.setScroll(token, this.#node(token).scrollRequest);
        }
        this.#node(token).scrollRequest = -1;
    }

    // 내용 높이 - 기본 높이 - 10 <= scroll 의 ordered 비교다(NaN 은 false, float32 임시 저장 없음).
    atBottom(token) { const threshold = this.#buffer(token).handler.totalHeight() - this.#node(token).baseHeight - BOTTOM_TOLERANCE; return threshold <= this.#node(token).scroll; }

    // 센티널(0)도 원본 필드가 있으므로 별도 자료로 돌려준다. 다른 미등록 줄은 거부한다.
    #line(storage, line) {
        if (!line) return storage.sentinel;
        const found = storage.softLines.get(line); if (!found) throw new Error('TextGump 표시 줄 없음');
        return found;
    }

    // 측정/출력 콜백 뒤 next 는 같은 공통 표의 현재 값이다. 잘못 삭제한 조각은 참조 전에 거부한다.
    #fragment(token) { const found = token ? this.#fragments.fragments.get(token) : null; if (!found) throw new Error('TextGump 표시 조각 없음'); return found; }

    // y+높이가 scroll 보다 작은 줄만 건너뛴다(줄 끝점과 scroll 이 같으면 그 줄을 유지). 센티널을 넘거나 순환하는 잘못된 목록은 거부한다.
    #firstVisible(token, storage) {
        let line = storage.sentinel.next; const seen = new Set();
        // 현재 줄의 저장 y+높이로 시작 줄만 찾는다. 이후 출력/선택은 높이를 누적한다.
        while (add(this.#line(storage, line).y, this.#line(storage, line).height) < this.#node(token).scroll) {
            if (!line) throw new Error('TextGump scroll이 센티널을 넘음');
            visit(seen, line); line = this.#line(storage, line).next;
        }
        return line;
    }

    // 현재 half-open 경계/표시 줄/조각 폭·높이로 화면 좌표의 조각 토큰을 돌려준다(없으면 0). layout/스크롤 요청을 자동 적용하지 않으며 조각의 위쪽 경계는 원본처럼 엄격히 제외한다.
    // 출력의 ascent 가 아닌 (줄 높이 - 조각 높이)로 클릭의 세로 유효 범위를 계산한다. 사용: hitTest(token, { x, y }).
    hitTest(token, position) {
        const bounds = this.#collection.bounds(token);
        if (position.x < bounds.left || position.x >= bounds.right || position.y < bounds.top || position.y >= bounds.bottom) return 0;
        const storage = this.#buffer(token).state; let line = this.#firstVisible(token, storage);
        let y = subtract(this.#line(storage, line).y, this.#node(token).scroll); const localX = subtract(position.x, bounds.left); const localY = subtract(position.y, bounds.top); const rows = new Set();
        // 원본은 x87 정수를 float 로 저장하지 않고 현재 baseHeight 와 비교한다.
        while (line && belowViewport(y, this.#node(token).baseHeight)) {
            visit(rows, line);
            if (localY < add(this.#line(storage, line).height, y)) {
                if (this.#line(storage, line).width <= localX) return 0;
                let fragment = this.#line(storage, line).head; let width = 0; const seen = new Set();
                // 다음 조각은 가상 측정 이후의 현재 next 다. 정확한 위 경계에서는 선택하지 않는다.
                while (fragment) {
                    visit(seen, fragment); const size = this.#style.measureFragment(fragment); width = add(width, size[0]);
                    if (localX < width && add(subtract(this.#line(storage, line).height, size[1]), y) < localY) return fragment;
                    fragment = this.#fragment(fragment).next;
                }
                return 0;
            }
            const height = this.#line(storage, line).height; line = this.#line(storage, line).next; y = add(y, height);
        }
        return 0;
    }

    // layout → 스크롤 요청 → 내부 클립 → 위치 → 보이는 줄의 기준선 두 번 순회 출력이다. 저장 y 는 첫 줄에만 쓰고 이후 행은 이전 행 높이를 누적한다(viewport 끝 동률은 그리지 않음).
    // 실제 모든 글리프/그림은 기존 StyleText 가상 분배다. 빈 줄도 높이 이동은 수행한다.
    draw(token) {
        this.ensureLayout(token); this.processRequest(token);
        const bounds = this.#collection.bounds(token); const scope = new GumpClipScope(this.#clip, bounds);
        try {
            const position = this.#collection.position(token); const storage = this.#buffer(token).state; let line = this.#firstVisible(token, storage);
            let y = subtract(this.#line(storage, line).y, this.#node(token).scroll); const rows = new Set();
            // 보이는 줄마다 두 번 순회한다(첫 순회는 최대 상승, 둘째는 기준선 재측정 후 출력).
            while (line && belowViewport(y, this.#node(token).baseHeight)) {
                visit(rows, line); let ascent = 0; let fragment = this.#line(storage, line).head; const seen = new Set();
                // 첫 순회는 현재 줄의 최대 상승만 구한다.
                while (fragment) { visit(seen, fragment); ascent = Math.max(ascent, this.#style.baseline(fragment)[1]); fragment = this.#fragment(fragment).next; }
                const baseline = add(ascent, y); let x = 0; fragment = this.#line(storage, line).head; seen.clear();
                // 두 번째 순회는 기준선을 다시 측정하고 가상 크기 → 출력 → 저장 폭으로 전진한다.
                while (fragment) {
                    visit(seen, fragment); const metrics = this.#style.baseline(fragment); const size = this.#style.measureFragment(fragment);
                    this.#style.drawFragment(fragment, { x: add(position.x, x), y: add(subtract(position.y, metrics[1]), baseline) });
                    x = add(x, size[0]); fragment = this.#fragment(fragment).next;
                }
                const height = this.#line(storage, line).height; line = this.#line(storage, line).next; y = add(y, height);
            }
        } finally { scope.dispose(); }
    }
}

// 같은 kind 2 가상 Draw 에 한 줄/다중 줄 처리를 연결한다. 현재 토큰의 처리기를 매번 조회해 등록/수명 변경을 반영하며 한 줄을 먼저 조회한다. 나머지는 이전 분배기를 호출한다.
// 사용: bindTextGumpDraw(hooks, (token) => single.get(token), (token) => scrolling.get(token)).
export function bindTextGumpDraw(hooks, single, scrolling) {
    if (typeof single !== 'function' || typeof scrolling !== 'function') throw new TypeError('TextGump Draw 분류 경계 필요');
    const previous = hooks.draw;
    hooks.draw = (token) => {
        const text = single(token); if (text) { text.draw(token); return; }
        const multi = scrolling(token); if (multi) { multi.draw(token); return; }
        if (previous) previous(token); else throw new Error('Gump 가상 Draw 처리기 없음');
    };
}

// 한 줄 파생 scalar 소멸자만 같은 GumpLifetime 에 연결하고 이전 분배를 보존한다. resolve 는 호출 때 평가해 처리기 생성 전에 구성할 수 있다.
export function bindTextGumpLifetime(hooks, resolve) {
    if (typeof resolve !== 'function') throw new TypeError('TextGump 수명 분류 경계 필요');
    const previous = hooks.deletingDestructor;
    // 파생 head 반납 뒤 기본 소멸자로 직접 위임하므로 가상 분배 재귀는 생기지 않는다.
    hooks.deletingDestructor = (token, flags) => {
        const text = resolve(token); if (text) { text.deleteScalar(token, flags); return; }
        if (previous) previous(token, flags); else throw new Error('Gump scalar 소멸자 처리기 없음');
    };
}
