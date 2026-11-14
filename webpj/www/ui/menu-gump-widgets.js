// 메뉴의 정상 생성·장식/제목 자식·controlled 시작·가상 부착·역순 항목 소멸을 기존 처리기에 연결한다.
// (cpppj client/MenuGumpWidgets.cpp/.h, 기준선 1f00a25a + 4ee69f4f, 10.78 전용 — CD 10.72 의 vtable(0x501fa0)·x87 중간값 저장 차이·객체 연결의 쓰기 순서는 옮기지 않는다)
// 부분 초기화(미작성 offsets/목표 크기/문맥의 다른 DWORD 는 보존)·조건부 항목·현재 전역 재조회와 패치의 x87 저장 위치(Limit 의 float32 좁힘)를 유지한다.
// 기준선 뒤 `4ee69f4f`(20261109-105): 객체 부착 Form 의 TYPE 을 사본(objectFormType) 대신 연결 시 공급한 현재 Gump TYPE 조회(currentObjectFormType)로 읽는다.
// 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다).
// (2026-11-14 WB04-E432: RawMenuControls 선행 MenuGumpWidgets 이식)
import { styleTextField } from './style-text-fragments.js';
import { MENU_ITEM_CAPACITY } from './menu-gump.js';

// 원본 정상 생성의 파생 가상 표(식별값, 호스트에서 실행하지 않는다)와 배경 flags 다.
const VTABLE = 0x0050c6f0; const BACKGROUND = 0x03e00000;
// 항목/메뉴 flags: 문자열/메뉴 자식/가로/장식 비트다.
const TEXT = 0x10000; const MENU = 0x20000; const HORIZONTAL = 0x40000; const FANCY = 0x800000;
// 객체 연결 helper 의 기본 flags 비트와 Form 생성 인자(mode 2)다. 화면 투영 비트 0x2000 과 구분한다.
const OBJECT_BINDING = 0x40; const OBJECT_FORM_MODE = 2;
// 실제 세 파생 객체의 원본 new 크기다. 호스트 클래스 크기와 혼용하지 않는다.
const MENU_BYTES = 0xb90; const DECORATION_BYTES = 0xc4; const TITLE_BYTES = 0xd8;

// CRT _ftol 의 64비트 절삭에서 하위 signed DWORD 를 반환한다. 무효 정수 결과의 하위 값은 0 이다.
function ftol(value) { if (!Number.isFinite(value) || value < -9223372036854775808 || value >= 9223372036854775808) return 0; return Number(BigInt.asIntN(32, BigInt(Math.trunc(value)))); }
// 좌표 차이를 signed DWORD 감김으로 계산한다.
const subtract = (first, second) => (first - second) | 0;
// 두 판본의 min 은 unordered 에서 요청값을 선택한다. 패치는 요청(FILD/add)을 float32 로 먼저 저장한 뒤 절삭한다.
function limit(request, maximum) { request = Math.fround(request); return ftol(maximum < request ? maximum : request); }
// 선택된 장치가 없으면 같은 지점의 TypeError(C++ std::invalid_argument)로 거부한다.
function need(fn, message) { if (typeof fn !== 'function') throw new TypeError(message); return fn; }

// 원본 varargs 의 문자열/자식 토큰과 조건부 DWORD 입력을 호스트 값으로 표현한다. flags 의 1/80/100 이 command/parameter/condition 을 소비하며 조건 false 에서도 명령/인자 쓰기는 수행한다.
export class MenuGumpConstructionEntry {
    // 문자열(바이트 문자열 또는 Uint8Array)이거나 자식 Gump 토큰이다.
    source = '';
    // 항목 flags, 명령, 명시 인자, 포함 조건(0 이면 제외, 100 비트일 때만 쓰임)이다.
    flags = 0; command = 0; parameter = 0; condition = 1;
}

// +20 식별 BYTE(두 PE 초기값 26)와 배경/색/객체 부착 Form 타입 전역이다. 실제 초기화 값을 공급해야 한다.
export class MenuGumpWidgetState {
    // 메뉴 식별 BYTE(signed)다.
    alignment = 26;
    // 배경 타입/프레임, 본문 색, 글자 그림자 색, 독립 입력의 객체 Form 타입이다.
    backgroundType = 0; backgroundFrame = 0; foreground = 0; shadow = 0; objectFormType = 0;
    // 실제 메뉴는 연결 시 사본 대신 같은 현재 Gump TYPE 를 생성 직전에 읽는다(기준선 뒤 4ee69f4f).
    currentObjectFormType = null;
    // 현재 객체 Form 타입이다. 조회 함수가 있으면 호출하고, 없으면 독립 값 사본을 쓴다.
    objectFormTypeNow() { return (this.currentObjectFormType ? this.currentObjectFormType() : this.objectFormType) >>> 0; }
}

// 원본 MenuGump 생성/수명/부착을 복원한다. 출력/입력은 별도 처리기, 객체 부착 Form 은 GumpFormHost 를 사용한다.
// 사용: new MenuGumpWidgets(createState, menuState, widgetState, backgroundCreate, decoration, titleWidgets, layout, style, lifetime, collection, tree, { node, base, allocate, attach, selectionNotice, objectForm, formOwner, formObject }).beginVertical()
export class MenuGumpWidgets {
    // 모든 상태/기존 처리기와 저장소와 현재 가상 경계다.
    #create; #menus; #state; #background; #decoration; #title; #layout; #style; #lifetime; #collection; #tree; #hooks;
    // 같은 메뉴/기본 저장소만 필수이며 할당/선택 통지/가상 부착은 실제 도달할 때 요구한다. 모든 상태/기존 처리기와 저장소는 생성/부착/수명 분배보다 오래 살아야 한다.
    constructor(create, menus, state, background, decoration, title, layout, style, lifetime, collection, tree, hooks = {}) {
        if (typeof hooks.node !== 'function' || typeof hooks.base !== 'function') throw new TypeError('메뉴 생성 공유 노드 필요');
        this.#create = create; this.#menus = menus; this.#state = state; this.#background = background; this.#decoration = decoration; this.#title = title; this.#layout = layout; this.#style = style; this.#lifetime = lifetime; this.#collection = collection; this.#tree = tree; this.#hooks = hooks;
    }

    // 기존 메뉴 크기/레이아웃/표시 수명과 같은 주소여야 한다. 토큰 0 은 조회하지 않는다.
    #node(token) {
        const node = token ? this.#hooks.node(token) : null;
        if (!node || node !== this.#hooks.base(token)) throw new Error('메뉴 생성 공유 노드 없음');
        return node;
    }

    // 자식 ID/visible/깊이와 장식 크기는 실제 기본 저장소를 사용한다.
    #base(token) {
        const node = token ? this.#hooks.base(token) : null; if (!node) throw new Error('메뉴 생성 자식/부모 노드 없음'); return node;
    }

    // 원본 항목 배열 밖 쓰기는 호스트 오류로 거부하고 이미 수행한 생성/통지는 되돌리지 않는다.
    #item(token, index) {
        index >>>= 0; if (index >= MENU_ITEM_CAPACITY) throw new RangeError('메뉴 생성 항목 한도 초과');
        return this.#node(token).items[index];
    }

    // 콜백이 바꾼 index 도 읽는다. 예약 0번은 시작 시만 별도로 거부하며 이후 원본 쓰기 순서대로 허용한다.
    #index() {
        if (this.#menus.index >>> 0 >= this.#menus.entries.length) throw new RangeError('controlled 메뉴 생성 칸 범위 초과');
        return this.#menus.index >>> 0;
    }

    // 부착할 실제 저장소를 검사하고 각 원본 자식의 가상 +20 경계로 전달한다.
    #attachChild(token, x, y, parent) {
        this.#base(token); need(this.#hooks.attach, '메뉴 생성 자식 Attach 경계 필요')(token, x, y, parent, 0);
    }

    // 생성 중 문자열 항목은 Append 와 달리 매번 Reflow 하지 않는다. 조건 false 항목도 command/parameter 를 먼저 쓴다.
    #entry(token, entry) {
        const flags = entry.flags >>> 0;
        if (flags & 1) this.#item(token, this.#node(token).itemCount).command = entry.command >>> 0;
        if (flags & 0x80) this.#item(token, this.#node(token).itemCount).parameter = entry.parameter >>> 0;
        const enabled = !(flags & 0x100) || entry.condition !== 0;
        if (flags & TEXT) {
            if (flags & MENU) throw new TypeError('메뉴 생성 문자열/자식 플래그 중복');
            if (enabled) {
                const text = entry.source; if (typeof text !== 'string' && !(text instanceof Uint8Array)) throw new TypeError('메뉴 생성 실제 문자열 필요');
                const index = this.#node(token).itemCount; this.#item(token, index).flags = flags; const context = Uint32Array.from(this.#node(token).textContext);
                this.#layout.setText(token, index, text, context); this.#node(token).itemCount = (this.#node(token).itemCount + 1) >>> 0;
            }
            if ((this.#node(token).itemCount | 0) >= MENU_ITEM_CAPACITY) throw new RangeError('메뉴 생성 numItems < 40 계약 위반');
        } else {
            if (!(flags & MENU)) throw new TypeError('메뉴 생성 문자열 또는 자식 플래그 필요');
            if (enabled) {
                const child = entry.source; if (typeof child !== 'number') throw new TypeError('메뉴 생성 실제 자식 토큰 필요');
                this.#base(child).flags = (this.#base(child).flags & ~1) >>> 0; this.#attachChild(child, 0, 0, token);
                if ((this.#node(token).itemCount | 0) > 0) {
                    const id = this.#base(child).id; const item = this.#item(token, (this.#node(token).itemCount - 1) >>> 0);
                    item.flags = (item.flags | 0x20) >>> 0; item.childId = id;
                }
            }
        }
    }

    // 장식과 제목은 별도 등록 객체이며 제목 자신의 버퍼/Slider/overlay 는 기존 실제 생성자가 만든다.
    #fancy(token) {
        if (this.#node(token).menuFlags & HORIZONTAL) throw new TypeError('장식 메뉴는 가로 플래그를 사용할 수 없음');
        const allocate = need(this.#hooks.allocate, '메뉴 장식/제목 할당 경계 필요');
        const decoration = allocate(DECORATION_BYTES) >>> 0;
        this.#node(token).fancy = decoration ? this.#decoration.construct(decoration, 1, 1) : 0;
        this.#attachChild(this.#node(token).fancy, 0, 0, token); this.#base(this.#node(token).fancy).secondaryDepth = 1;
        const title = allocate(TITLE_BYTES) >>> 0;
        this.#node(token).title = title ? this.#title.construct(title, 0, 0, 0, this.#state.foreground) : 0;
        this.#attachChild(this.#node(token).title, 16, 16, this.#node(token).fancy); this.#base(this.#node(token).title).secondaryDepth = 0xffffffff;
        this.#node(token).backgroundFlags = (this.#node(token).backgroundFlags | 0x40000) >>> 0;
    }

    // 정상 생성이다(004775f0). 기본 배경/부분 초기화→조건부 항목·이전 항목의 자식 연결→최종 메뉴 flags/장식/제목→Reflow/notify 순서다.
    // native varargs 의 마지막 null 뒤 flags 는 별도 인자다. 미작성 offsets/목표 크기·문맥의 다른 DWORD 는 보존한다. 사용: construct(token, [{source:'열기', flags:0x10001, command:5}], 0x280000)
    construct(token, entries, flags) {
        this.#node(token); this.#background.construct(token, 0, 0, BACKGROUND, 0x201, this.#state.backgroundType, this.#state.backgroundFrame);
        const node = this.#node(token); node.vtable = VTABLE;
        node.fancy = 0; node.title = 0; node.selectionCallback = 0; node.textContext[1] = 0;
        node.command = ((this.#state.alignment << 24) >> 24) >>> 0; node.textContext[2] = this.#state.foreground;
        node.textContext[7] = 0; node.textContext[5] = this.#state.shadow; node.textContext[6] = 1; node.textContext[0] |= 2;
        this.#menus.instances = (this.#menus.instances + 1) >>> 0; node.expanding = 0; node.itemWidth = 0; node.menuFlags = 0; node.itemCount = 0;
        node.flags = (node.flags | 0x20) >>> 0; node.depth = 0xffffffc4; node.selection = -1; node.heights.fill(0);
        // 40개 항목을 모두 값 초기화한다(저장소 객체는 보존한다).
        for (const item of node.items) { item.label.fill(0); item.childId = 0; item.command = 0; item.parameter = 0; item.styled = 0; item.flags = 0; }
        // 호스트 입력 순서를 유지하며 조건부 항목 콜백 뒤 현재 항목 개수를 다시 읽는다.
        for (const entry of entries) this.#entry(token, entry);
        this.#node(token).menuFlags = flags >>> 0; if (flags & FANCY) this.#fancy(token);
        this.#layout.reflow(token); this.#node(token).flags = (this.#node(token).flags | 0x100) >>> 0; return token;
    }

    // 세 controlled 시작 함수의 공통 칸/할당/생성/idle/추가 flags 순서다(00477a10 외). 스택 칸 증가는 assert/할당보다 앞이고 생성/idle 콜백 이후의 현재 index 로 칸/메뉴를 다시 읽는다.
    #begin(flags, idle, extra, backgroundFlags) {
        this.#menus.index = (this.#menus.index + 1) >>> 0; if (!this.#menus.index) throw new RangeError('controlled 메뉴 시작 예약 칸 금지');
        if (this.#menus.entries[this.#index()]) throw new TypeError('controlled 메뉴 시작 칸이 비어 있어야 함');
        const token = need(this.#hooks.allocate, 'controlled 메뉴 할당 경계 필요')(MENU_BYTES) >>> 0; const constructed = token ? this.construct(token, [], flags) : 0;
        this.#menus.entries[this.#index()] = constructed;
        if (idle) this.#lifetime.setIdle(this.#menus.entries[this.#index()], true);
        if (extra) { const node = this.#node(this.#menus.entries[this.#index()]); node.backgroundFlags = (node.backgroundFlags | backgroundFlags) >>> 0; }
        return this.#menus.entries[this.#index()];
    }

    // 가로 controlled 메뉴를 만든다(00477a10): 0x140000 으로 생성하고 별도 idle 표에 등록한다.
    beginHorizontal() { return this.#begin(0x140000, true, false, 0); }

    // 장식 controlled 메뉴를 만든다(00477ae0): 0xa80000 으로 생성하며 idle 은 등록하지 않는다.
    beginFancy() { return this.#begin(0xa80000, false, false, 0); }

    // 세로 controlled 메뉴를 만든다(00477bb0): 0x280000 으로 생성하고 idle 등록 뒤 현재 메뉴의 배경 flags 에 전달한 값을 OR 한다.
    beginVertical(backgroundFlags = 0) { return this.#begin(0x280000, true, true, backgroundFlags >>> 0); }

    // 가상 Attach 다(00476be0). root 부착 비트·parent 0 재사용·일반/장식 전체 화면 보정→기존 기본 Attach 다. mode 1 또는 기본 0x20000 이면 보정을 생략한다. 패치의 float32 중간 저장을 유지한다.
    attach(token, x, y, parent, mode) {
        x = Math.fround(x); y = Math.fround(y); parent >>>= 0; mode >>>= 0; if (!parent) parent = this.#node(token).parent;
        if (parent === this.#create.collection.root) this.#node(token).flags = (this.#node(token).flags | 0x4000) >>> 0;
        if (!(mode & 1) && !(this.#node(token).flags & 0x20000)) {
            const origin = this.#collection.position(parent); let screenX = 0; let screenY = 0;
            if (this.#node(token).menuFlags & FANCY) {
                const decoration = this.#base(this.#node(token).fancy);
                screenX = ftol((origin.x + decoration.x) + x); screenY = ftol((origin.y + y) + decoration.y);
                if (screenX < 0) screenX = 0; if (screenY < 0) screenY = 0;
                screenX = limit(screenX, this.#create.screenWidth - decoration.width); screenY = limit(screenY, this.#create.screenHeight - decoration.height);
                screenX = ftol(screenX - decoration.x); screenY = ftol(screenY - decoration.y);
            } else {
                screenX = limit(origin.x + x, this.#create.screenWidth - this.#node(token).width); screenY = limit(origin.y + y, this.#create.screenHeight - this.#node(token).height);
            }
            x = Math.fround(subtract(screenX, origin.x)); y = Math.fround(subtract(screenY, origin.y));
        }
        this.#tree.attach(token, x, y, parent, mode);
    }

    // 메뉴 소멸이다(00476ec0/00476f40). 파생 표/개수 감소→선택 통지→40개 조각 역순 Clear→기본 scalar 다. 선택 콜백 뒤 현재 전체 항목을 마지막부터 삭제하며 itemCount 나 styled 플래그로 소멸 대상을 줄이지 않는다.
    // 자식 장식/제목 삭제는 기존 GumpLifetime.destroy 의 자식 우선 순서를 따른다.
    deleteScalar(token, flags) {
        this.#node(token).vtable = VTABLE; this.#menus.instances = (this.#menus.instances - 1) >>> 0;
        if (this.#node(token).selectionCallback) need(this.#hooks.selectionNotice, '메뉴 소멸 선택 통지 경계 필요')(this.#node(token).selectionCallback, this.#node(token).selection);
        // 원본 vector destructor 는 40개 전체를 역순으로 해제한다. 스타일 head 는 같은 실제 소유 필드다.
        for (let index = MENU_ITEM_CAPACITY; index > 0; index -= 1) this.#style.clear(styleTextField(this.#item(token, index - 1), 'styled'));
        return this.#lifetime.deleteScalar(token, flags);
    }

    // 객체 연결 helper(004644e0)다. Form 생성/소유 등록/연결 뒤 같은 기본 노드의 연결 필드만 쓴다. object 0 은 기존 연결을 지우지 않는다.
    reference(token, object) {
        if (object >>> 0) {
            if (typeof this.#hooks.objectForm !== 'function' || typeof this.#hooks.formOwner !== 'function' || typeof this.#hooks.formObject !== 'function') throw new TypeError('메뉴 객체 부착 Form 경계 필요');
            const form = this.#hooks.objectForm(this.#state.objectFormTypeNow(), OBJECT_FORM_MODE);
            if (!form.token) throw new Error('메뉴 객체 부착 Form 생성 실패');
            this.#hooks.formOwner(form.token, this.#base(token).id); this.#hooks.formObject(form.token, object >>> 0, 0);
            this.#base(token).flags = (this.#base(token).flags | OBJECT_BINDING) >>> 0; this.#base(token).object = object >>> 0; this.#base(token).form = form.index >>> 0;
        }
    }

    // 실제 정보 메뉴는 화면 부착 전에 같은 내용물 Form 을 먼저 연결한 다음 현재 가상 Attach 를 호출한다(00476dd0).
    attachObject(token, object, x, y, parent, mode) {
        this.reference(token, object); need(this.#hooks.attach, '메뉴 객체 가상 Attach 경계 필요')(token, x, y, parent, mode);
    }

    // fancy 정보 메뉴의 원본 별도 465330 호출을 같은 등록표로 분배한다.
    setIdle(token, enabled) { this.#lifetime.setIdle(token, enabled); }
}

// 기존 제목 Reflow 경계를 원본 RequestScroll/FitContent 몸체에 직접 연결한다. 사용: bindMenuTitleLayout(layoutHooks, scrollingTextGump)
export function bindMenuTitleLayout(hooks, text) {
    hooks.titleFit = (token, position) => text.requestScroll(token, position | 0);
    hooks.titleLayout = (token, width, height) => text.fitContent(token, width | 0, height | 0);
}

// 생성 이후에도 현재 메뉴 소속을 조회하여 실제 트리/수명 분배를 같은 처리기에 연결한다. 메뉴 파생 Attach/scalar 만 분배하고 기존 트리/장식/텍스트/버튼 처리를 보존한다.
// 사용: bindMenuGumpWidgets(treeHooks, lifetimeHooks, (token) => menus.get(token) ?? null)
export function bindMenuGumpWidgets(tree, lifetime, resolve) {
    if (typeof resolve !== 'function') throw new TypeError('메뉴 생성/수명 분배 조회 필요');
    const previousAttach = tree.attach; const previousDelete = lifetime.deletingDestructor;
    // 메뉴이면 실제 Attach(화면 보정 포함)로, 아니면 이전 처리기로 보낸다.
    tree.attach = (token, x, y, parent, mode, bits) => {
        const menu = resolve(token);
        if (menu) menu.attach(token, x, y, parent, mode);
        else { if (typeof previousAttach !== 'function') throw new TypeError('메뉴 외 Attach 경계 필요'); previousAttach(token, x, y, parent, mode, bits); }
    };
    // 메뉴이면 실제 scalar 소멸로, 아니면 이전 처리기로 보낸다.
    lifetime.deletingDestructor = (token, flags) => {
        const menu = resolve(token);
        if (menu) menu.deleteScalar(token, flags);
        else { if (typeof previousDelete !== 'function') throw new TypeError('메뉴 외 소멸 경계 필요'); previousDelete(token, flags); }
    };
}

// controlled 객체 부착의 가상 +3c 를 실제 메뉴 몸체에 연결하고 다른 타입의 기존 처리를 보존한다. 실제 Form 공급과 가상 Attach 는 동일 저장소를 사용한다.
export function bindMenuGumpObjectAttach(hooks, resolve) {
    if (typeof resolve !== 'function') throw new TypeError('메뉴 객체 부착 분배 조회 필요'); const previous = hooks.attachObject;
    // 같은 controlled 메뉴의 가상 +3c 호출을 현재 분류 결과로 분배한다.
    hooks.attachObject = (token, object, x, y, parent, mode) => {
        const menu = resolve(token);
        if (menu) menu.attachObject(token, object, x, y, parent, mode);
        else { if (typeof previous !== 'function') throw new TypeError('메뉴 외 객체 부착 경계 필요'); previous(token, object, x, y, parent, mode); }
    };
}
