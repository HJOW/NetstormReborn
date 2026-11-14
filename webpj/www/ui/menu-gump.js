// 원본 MenuGump 의 항목/노드 저장소, 크기·장식 프레임, 확장 idle, controlled 메뉴 스택을 같은 Gump 저장소에 연결한다.
// (cpppj client/MenuGump.cpp/.h, 기준선 1f00a25a, 10.78 전용 — CD 10.72 의 index 쓰기 순서·x87 unordered 분기는 옮기지 않는다)
// 크기 변경은 x87 중간값(double)을 float32 로 좁히기 전까지 유지하고, 콜백(Push/Attach/Redraw) 뒤에는 현재 필드/표/루트를 다시 읽는다.
// 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// (2026-11-14 WB04-E428: RawMenuControls 선행 MenuGump 이식)
import { GumpBackgroundNode } from './gump-background-create.js';

// 원본 메뉴 항목 한도와 controlled 메뉴 표 길이다. 스택 0번은 예약 칸이다.
export const MENU_ITEM_CAPACITY = 40; export const CONTROLLED_MENU_CAPACITY = 20;
// 원본 가로/장식/가로 확장 방향 비트와 장식 여백 상수다.
const HORIZONTAL = 0x40000; const FANCY = 0x800000; const EXPAND_WIDTH = 0x80000;
// 장식은 메뉴보다 폭 +32, 높이 +48(제목 높이 포함) 크며 위로 32+제목 높이만큼 올라간다.
const FANCY_WIDTH = 32; const FANCY_HEIGHT = 48; const FANCY_TOP = 32;
// 장식 자식의 x 부착 위치(-16)다.
const FANCY_LEFT = -16;

// CRT _ftol 의 signed 64비트 절삭 결과에서 하위 signed DWORD 를 취한다. 무효 정수 결과의 하위 DWORD 는 0 이다.
function ftol(value) { if (!Number.isFinite(value) || value < -9223372036854775808 || value >= 9223372036854775808) return 0; return Number(BigInt.asIntN(32, BigInt(Math.trunc(value)))); }
// 선택된 장치가 없으면 같은 지점의 TypeError(C++ std::invalid_argument)로 거부한다.
function need(fn, message) { if (typeof fn !== 'function') throw new TypeError(message); return fn; }

// 원본 항목 0x3c바이트다. 40바이트 라벨 뒤 +28 자식 등록 ID/+2c 명령/+30 인자/+34 StyleText 토큰/+38 flags 다.
export class MenuGumpItem {
    // 화면에 표시할 항목 문구(NUL 로 끝나지 않을 수 있는 40바이트)다.
    label = new Uint8Array(40);
    // 하위 항목/자식 노드를 구별하는 등록 ID, 선택 시 실행할 명령, 명령에 전달할 인자, StyleText 소유 head 토큰, 동작 flags 다(모두 DWORD).
    childId = 0; command = 0; parameter = 0; styled = 0; flags = 0;
}

// 기본 배경 필드를 공유하며 메뉴 크기/프레임/항목 레이아웃이 읽는 파생 필드를 보관한다. offsets 는 +17c, itemWidth/targetWidth/targetHeight 는 +170/+174/+178 이다.
export class MenuGumpNode extends GumpBackgroundNode {
    // 하위 메뉴 확장 중 여부, 표시·입력 동작 flags, 항목 수(DWORD), 장식 자식 토큰, 제목 Gump 토큰이다.
    expanding = 0; menuFlags = 0; itemCount = 0; fancy = 0; title = 0;
    // 항목 폭(가로 메뉴는 균일 폭), UI 크기 조정의 목표 폭/높이(float32)다.
    itemWidth = 0; targetWidth = 0; targetHeight = 0;
    // 항목별 출력 시작 offset(float32 40칸)과 여백 4칸(+bc/+c0 폭 여백, +c4/+c8 높이 여백)이다.
    offsets = new Float32Array(MENU_ITEM_CAPACITY); margins = new Float32Array(4);
    // 항목별 현재 측정(또는 가로 메뉴에서 맞춘) 높이(float32 40칸)와 40개 항목 저장소다.
    heights = new Float32Array(MENU_ITEM_CAPACITY); items = Array.from({ length: MENU_ITEM_CAPACITY }, () => new MenuGumpItem());
    // 현재 선택 항목(-1 이면 없음)과 선택 소멸 통지 토큰이다. 실제 호스트 호출은 생성/수명 처리기가 분배한다.
    selection = -1; selectionCallback = 0;
}

// 원본 controlled 메뉴 표와 signed index 의 DWORD 비트, 메뉴 생성/소멸 개수다.
export class MenuGumpState {
    // controlled 메뉴 토큰 표(20칸)다. 0번은 예약 칸이다.
    entries = new Uint32Array(CONTROLLED_MENU_CAPACITY);
    // 현재 표 index(DWORD, 원본은 signed 로 읽는다)와 10.78 00593024 의 현재 메뉴 개수(정상 생성/소멸에서 DWORD 로 증감)다.
    index = 0; instances = 0;
}

// 원본 메뉴의 정상 프레임/스택 몸체다. 레이아웃/생성/출력/입력은 같은 저장소의 별도 메뉴 처리기가 담당한다.
// 사용: new MenuGump(createState, menuState, lifetime, { node, baseNode, push, pop, attach, attachObject, redraw, baseRedraw }).setWidth(token, 120)
export class MenuGump {
    // 같은 등록/controlled 상태와 수명 처리기, 현재 가상 경계다.
    #create; #state; #lifetime; #hooks;
    // 파생 메뉴 조회와 기본 저장소 조회는 필수이며 가상 콜백은 실제 도달 시 검사한다. 같은 상태/노드 저장소는 처리기보다 오래 살아야 한다.
    constructor(create, state, lifetime, hooks = {}) {
        if (typeof hooks.node !== 'function' || typeof hooks.baseNode !== 'function') throw new TypeError('메뉴/기본 Gump 저장소 필요');
        this.#create = create; this.#state = state; this.#lifetime = lifetime; this.#hooks = hooks;
    }

    // 표시/수명/파생 메뉴가 복사본이 아닌 같은 기본 노드를 공유하는지 확인한다. 토큰 0 은 조회하지 않는다.
    #node(token) {
        const node = token ? this.#hooks.node(token) : null;
        if (!node || node !== this.#hooks.baseNode(token)) throw new Error('메뉴 공유 저장소 없음');
        return node;
    }

    // fancy/title 은 메뉴 클래스가 아닌 기본 Gump 파생 저장소도 허용한다.
    #baseNode(token) {
        const node = token ? this.#hooks.baseNode(token) : null;
        if (!node) throw new Error('메뉴 장식/제목 저장소 없음');
        return node;
    }

    // 장식 token/title/가상 Attach 는 Push 와 크기 변경을 마친 시점의 현재 값이다. y = -(제목 기본 높이)-32 를 절삭한 정수의 float32 다.
    #attachFancy(token) {
        const menu = this.#node(token);
        const y = Math.fround(ftol(-this.#baseNode(menu.title).baseHeight - FANCY_TOP));
        const attach = need(this.#hooks.attach, '메뉴 장식 Attach 경계 필요');
        attach(menu.fancy, FANCY_LEFT, y, token, 0);
    }

    // 부모 크기를 쓰고 fancy 이면 현재 장식 Push→크기 재계산→Attach 순서다(00475f20). 장식 폭은 기본 폭+32 의 double 합계를 float32 로 한 번만 좁힌다.
    setWidth(token, width) {
        const menu = this.#node(token); menu.baseWidth = Math.fround(width); menu.width = Math.fround(menu.baseWidth * menu.scale);
        if (!(menu.menuFlags & FANCY)) return;
        need(this.#hooks.push, '메뉴 장식 Push 경계 필요')(menu.fancy);
        const size = menu.baseWidth + FANCY_WIDTH; const fancy = this.#baseNode(menu.fancy);
        fancy.baseWidth = Math.fround(size); fancy.width = Math.fround(size * fancy.scale);
        this.#attachFancy(token);
    }

    // 제목 높이+현재 메뉴 높이+48 의 중간 합계를 float32 로 먼저 좁히지 않는다(00475fb0).
    setHeight(token, height) {
        const menu = this.#node(token); menu.baseHeight = Math.fround(height); menu.height = Math.fround(menu.baseHeight * menu.scale);
        if (!(menu.menuFlags & FANCY)) return;
        need(this.#hooks.push, '메뉴 장식 Push 경계 필요')(menu.fancy);
        const size = this.#baseNode(menu.title).baseHeight + menu.baseHeight + FANCY_HEIGHT; const fancy = this.#baseNode(menu.fancy);
        fancy.baseHeight = Math.fround(size); fancy.height = Math.fround(size * fancy.scale);
        this.#attachFancy(token);
    }

    // 장식의 유무는 menuFlags 와 무관하다. 장식 콜백 후 기본 요청도 항상 호출한다(00476050).
    redraw(token, erase) {
        const fancy = this.#node(token).fancy;
        if (fancy) need(this.#hooks.redraw, '메뉴 장식 Redraw 경계 필요')(fancy, erase);
        need(this.#hooks.baseRedraw, '메뉴 기본 Redraw 경계 필요')(token, erase);
    }

    // 가로 메뉴는 index×항목 폭/현재 높이, 세로 메뉴는 현재 폭/저장 offset 을 반환한다(00476630). 세로 offset 배열 범위를 벗어나는 호스트 입력은 조회 직전에 거부한다.
    itemPosition(token, index) {
        const menu = this.#node(token); index |= 0;
        if (menu.menuFlags & HORIZONTAL) return [Math.fround(index * menu.itemWidth), menu.height];
        if (index < 0 || index >= menu.offsets.length) throw new RangeError('메뉴 항목 offset 범위 오류');
        return [menu.width, menu.offsets[index]];
    }

    // 방향에 따라 한 축을 0으로 시작하고 실제 idle 표에 등록한 뒤 expanding 을 세운다(004767d0). SetWidth 의 콜백 이후 목표 높이를 다시 읽는다.
    startExpand(token, flags) {
        const menu = this.#node(token);
        if (flags & EXPAND_WIDTH) { this.setWidth(token, menu.targetWidth); this.setHeight(token, 0); }
        else { this.setWidth(token, 0); this.setHeight(token, menu.targetHeight); }
        this.#lifetime.setIdle(token, true); menu.expanding = 1;
    }

    // 현재 목표보다 작으면 가상 Push→높이→폭→Pop, 다음 완료 호출에서 expanding 만 지운다(00476700). 비교는 plain float 비교다(NaN 은 비교 실패 → 확장 시도).
    idle(token) {
        const menu = this.#node(token); if (!menu.expanding) return;
        if (menu.height >= menu.targetHeight && menu.width >= menu.targetWidth) { menu.expanding = 0; return; }
        const push = need(this.#hooks.push, '메뉴 확장 Push/Pop 경계 필요'); const pop = need(this.#hooks.pop, '메뉴 확장 Push/Pop 경계 필요');
        push(token);
        if (menu.height < menu.targetHeight) this.setHeight(token, menu.targetHeight);
        if (menu.width < menu.targetWidth) this.setWidth(token, menu.targetWidth);
        pop(token);
    }

    // DWORD index 의 signed 음수도 배열 한도를 넘으므로 접근 전에 거부한다.
    #index() {
        if (this.#state.index >>> 0 >= this.#state.entries.length) throw new RangeError('현재 메뉴 index 범위 오류');
        return this.#state.index >>> 0;
    }

    // 현재 표 토큰을 읽는다(00475c40). 메뉴가 없으면 0 을 반환한다(항목 수 조회와 달리 null 자체는 허용한다).
    current() { return this.#state.entries[this.#index()]; }

    // 유효 현재 메뉴의 DWORD 항목 수를 그대로 반환한다(00475c50).
    itemCount() { return this.#node(this.current()).itemCount; }

    // 패치는 칸 clear→index 감소 순서다. 예약 0번 꺼내기는 원본 assert 계약 밖이므로 변경 전에 거부한다.
    #dropCurrent() {
        const index = this.#index(); if (!index) throw new RangeError('메뉴 예약 칸 꺼내기 오류');
        this.#state.entries[index] = 0; this.#state.index = (this.#state.index - 1) >>> 0;
    }

    // 현재 토큰을 꺼내고 칸을 지운다(00475c00). 반환 토큰은 clear 전에 읽는다.
    takeCurrent() { const token = this.current(); this.#dropCurrent(); return token; }

    // 현재 메뉴의 가상 Attach 뒤 다시 읽은 현재 칸을 지우고 index 를 내린다(00475ca0). 두 정수 좌표는 float32 로 변환해 전달한다.
    attachCurrent(x, y, parent) {
        const token = this.current(); this.#node(token);
        need(this.#hooks.attach, '현재 메뉴 Attach 경계 필요')(token, Math.fround(x | 0), Math.fround(y | 0), parent >>> 0, 0);
        this.#dropCurrent();
    }

    // 객체 부착 가상 +3c 에 현재 등록표 루트(등록 0번)를 전달한 뒤 현재 메뉴 칸을 제거한다(00475cf0). 단순 화면 상대 이동으로 바꾸지 않는다.
    attachObjectCurrent(object, x, y) {
        const token = this.current(); this.#node(token);
        need(this.#hooks.attachObject, '현재 메뉴 객체 Attach 경계 필요')(token, object >>> 0, Math.fround(x | 0), Math.fround(y | 0), this.#create.entries[0], 0);
        this.#dropCurrent();
    }
}

// 메뉴 가상 +8(idle)/+30(redraw)만 분배하고 다른 타입의 기존 프레임 처리기를 보존한다. 호출 시 현재 분배기를 조회해 순환 의존 처리기 생성 이후에도 연결할 수 있다.
// 사용: bindMenuGumpFrame(frameHooks, (token) => menus.get(token) ?? null)
export function bindMenuGumpFrame(hooks, resolve) {
    if (typeof resolve !== 'function') throw new TypeError('메뉴 프레임 분배 조회 필요');
    const idle = hooks.idle; const redraw = hooks.redraw;
    // 메뉴이면 실제 메뉴 idle 로, 아니면 이전 처리기로 전달한다.
    hooks.idle = (token) => {
        const menu = resolve(token);
        if (menu) menu.idle(token);
        else { if (typeof idle !== 'function') throw new TypeError('메뉴 외 idle 경계 필요'); idle(token); }
    };
    // 메뉴이면 실제 메뉴 redraw 로, 아니면 이전 처리기로 전달한다.
    hooks.redraw = (token, erase) => {
        const menu = resolve(token);
        if (menu) menu.redraw(token, erase);
        else { if (typeof redraw !== 'function') throw new TypeError('메뉴 외 Redraw 경계 필요'); redraw(token, erase); }
    };
}
