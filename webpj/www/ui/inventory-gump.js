// 실제 내용물 SID 를 쓰는 생산 격자(5068 00 가상 표 506800)와 항목(5068f4)의 원본 생성·배치·통지·점멸을 같은 Gump 저장소에 옮긴다(cpppj `client/InventoryGump.cpp/.h`, 기준선 1f00a25a, 10.78 전용).
// ConstructGrid(43d790)/ConstructItem(43e120)/BeginBlink(43d690)/IdleItem(43d6e0)/AttachGrid(43e1e0)/HandleGrid(43d850)/ParentGone(43d9c0)를 포함한다. 디컴파일 C 에서 생략된 ceil/ftol 과 단정도 중간 저장은 실제 명령 순서로 유지한다
// (격자 열/행: sqrt 는 float, 비율·몫·칸 크기는 배정도로 계산한 뒤 저장 시점에만 float32). 항목 아이콘 자식 부착은 별도 단위(InventoryItemGump)이다.
// 사용: const inventory = new InventoryGump(create, factory, pool, finderState, state, hooks); inventory.constructGrid(token, columns, rows, mask); inventory.attachGrid(token, x, y, parent, mode); inventory.handleGrid(token, event, first, second).
//   state = { cursorPoint: {x,y}, cursorFlags }(55a2f8/fc·55a2ac 의 현재 값, 호출 때마다 읽음), hooks = { grid(token), item(token), clearChildren(token), baseAttach(token,x,y,parent,mode), allocateItem(size)→token, attach(token,x,y,parent,mode), push(token), bind(token,sid),
//   idle(token,enabled), now()→초, contentParent(sid), forward(token,event,first,second), baseHandle(token,event,first,second), local(token,code,point)→[x,y], inside(token,point), cancelCursor(), assertion(text), redraw(token,erase), clearBlink(sid) }(도달한 경계만 요구).
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-15 WB04-E472: 5-B InventoryGump 이식, 기준선 뒤 cpppj 변경 없음)
import { GumpCreateNode } from './gump-create.js';
import { RawContainedFinder } from '../core/raw-contained-finder.js';
import { cameraFtolLow } from '../render/camera-view.js';

// 10.78 실제 가상 표(격자/항목)와 초기 사건 DWORD(내용물 통지·커서 질의·mask 질의·전달)이다.
const GRID_VTABLE = 0x506800; const ITEM_VTABLE = 0x5068f4;
const CONTENT_NOTICE = 0x02000031; const CURSOR_QUERY = 0x0200003f; const MASK_QUERY = 0x0200005f; const FORWARD = 0x0200005e;
// 43e1e0 의 실제 항목 할당 크기이다.
const ITEM_SIZE = 0xc0;

// 506800 의 +a0~c0 필드이다. 기본 생성/표시/수명 저장소를 그대로 상속한다.
export class InventoryGridNode extends GumpCreateNode {
    // +a0/+a4: 자동 배치의 최소값 또는 고정 열/행 수(signed)이다.
    minimumColumns = 0; minimumRows = 0;
    // +a8/+ac: 실제 부착에서 계산한 배율 적용 칸 크기(float32)이다.
    cellWidth = 0; cellHeight = 0;
    // +b0/+b4/+b8/+bc: 목록·커서·클릭·고정 배치 조건이다. +c0: 비0 이면 각 항목의 아이콘 배율에 그대로 전달한다.
    contentMask = 0; acceptedCursor = 0; requireImageHit = 0; fixed = 0; itemScale = 0;
}

// 5068f4 의 +a0~bc 이다. +a4/+b2 는 생성자가 쓰지 않는 원문을 보존한다.
export class InventoryItemNode extends GumpCreateNode {
    // +a0 의 실제 내용물 SID 와 미작성 DWORD 이다.
    content = 0; reservedA4 = 0;
    // +a8: 점멸 단계의 다음 실제 시각(double)이다.
    blinkUntil = 0;
    // +b0/+b1: 점멸 단계와 항목 Draw 장식 조건이다. +b2: 미작성 WORD 이다.
    blink = 0; decoration = 0; reservedB2 = 0;
    // +b4/+b8: 실제 그림 hit 요구와 원본 초기 0 필드, +bc: 0 이면 원본 경계에 맞추고 비0 이면 지정 배율이다.
    requireImageHit = 0; additional = 0; iconScale = 0;
}

// 도달한 실제 외부 효과만 필수로 검사한다(C++ logic_error → Error).
function need(callback) { if (typeof callback !== 'function') throw new Error('생산 격자 실제 하위 미연결'); return callback; }
// 원문의 비정렬 flags WORD 이다.
const flagsOf = (raw) => raw[20] | (raw[21] << 8);
// 플래그를 unsigned DWORD 로 유지한다.
const setFlag = (node, mask) => { node.flags = (node.flags | mask) >>> 0; };
const clearFlag = (node, mask) => { node.flags = (node.flags & ~mask) >>> 0; };

// 내용물 격자/항목의 생성·배치·통지 처리기이다. 모든 노드/현재 전역/공통 생성기는 이 처리기와 호출보다 오래 살아야 한다. (2026-11-15 WB04-E472)
export class InventoryGump {
    // 같은 실제 등록표/기본 생성자, 내용물 flags 와 slot BYTE 의 유일한 저장소, 현재 순회 정책, 현재 커서 전역, 선택된 외부 효과이다.
    #factory; #pool; #finder; #state; #hooks;

    // 다른 판본을 10.78 주소/필드로 잘못 조립하지 않도록 생성 시 검사한다(C++ invalid_argument → TypeError). create.collection.edition 과 풀 판본이 모두 patch1078 이어야 한다.
    constructor(create, factory, pool, finderState, state, hooks) {
        if (create?.collection?.edition !== undefined && create.collection.edition !== 'patch1078') throw new TypeError('생산 격자 PATCH 자료 필요');
        if (pool?.edition !== 'patch1078') throw new TypeError('생산 격자 PATCH 자료 필요');
        this.#factory = factory; this.#pool = pool; this.#finder = finderState; this.#state = state; this.#hooks = hooks;
    }

    // 가상 분배가 공급하는 파생 노드의 안정된 주소를 읽는다(없으면 RangeError, C++ out_of_range).
    #grid(token) {
        const node = token ? need(this.#hooks.grid)(token) : null;
        if (!node) throw new RangeError('생산 격자 저장소 없음');
        return node;
    }

    // 실제 항목 생성 전후에 같은 저장소를 다시 읽는다.
    #item(token) {
        const node = token ? need(this.#hooks.item)(token) : null;
        if (!node) throw new RangeError('생산 항목 저장소 없음');
        return node;
    }

    // 원본의 포인터 산술에 앞서 SID 의 실제 범위만 확인한다.
    #content(sid) {
        if (!sid || sid >= this.#pool.capacity || sid > 65535) throw new RangeError('생산 격자 내용물 SID 범위 오류');
        return sid;
    }

    // 43d790: 실제 기본 생성 뒤 격자 필드를 쓴다. 0열은 원본처럼 진단 뒤 계속한다. +a8/+ac 는 이 생성자가 쓰지 않는다.
    constructGrid(token, columns, rows, mask) {
        const node = this.#grid(token); this.#factory.construct(token, 0, 0, 0, 1); node.vtable = GRID_VTABLE;
        if (!columns) need(this.#hooks.assertion)('_numSlotsX != 0');
        node.command = 12; node.minimumColumns = columns | 0; setFlag(node, 0xa00); node.acceptedCursor = 0; node.contentMask = mask >>> 0; node.fixed = 0; node.itemScale = 0; node.minimumRows = rows | 0; node.requireImageHit = 1;
        return token;
    }

    // 43e120: 실제 내용물 SID/크기와 flags 200 의 점멸 시작을 반영한다. 마지막에 클릭 요구 1 을 쓴다.
    constructItem(token, width, height, content) {
        const node = this.#item(token); this.#factory.construct(token, 0, width, height, 1); node.vtable = ITEM_VTABLE; node.blinkUntil = 0; setFlag(node, 0x1a00);
        node.content = content >>> 0; node.command = 8; node.blink = 0; node.decoration = 0; node.additional = 0; node.iconScale = 0;
        if (flagsOf(this.#pool.slot(this.#content(content))) & 0x200) this.beginBlink(token);
        node.requireImageHit = 1;
        return token;
    }

    // 43d690: 단계 4 → 실제 idle 등록 → 현재 시각 + 0.1 을 보존한다(idle 소비자의 부수 효과 뒤에 시각을 읽는다).
    beginBlink(token) {
        const node = this.#item(token); node.blink = 4; need(this.#hooks.idle)(token, true); node.blinkUntil = need(this.#hooks.now)() + 0.1;
    }

    // 43d6e0: 표시 상태를 바꾸는 앞/뒤의 실제 가상 갱신 순서와 별도 두 번의 시각 조회를 보존한다. 단계가 0 이면 Idle 해제·표시·다시 그리기·내용물 점멸 비트 정리로 끝낸다.
    idleItem(token) {
        const node = this.#item(token);
        if (!node.blink) {
            need(this.#hooks.idle)(token, false); setFlag(node, 1); need(this.#hooks.redraw)(token, false); need(this.#hooks.clearBlink)(node.content);
            return;
        }
        // 원문은 시각 조회가 반환한 뒤 현재 기한을 읽는다.
        const now = need(this.#hooks.now)();
        if (node.blinkUntil <= now) {
            node.blink = (node.blink - 1) & 0xff; node.blinkUntil = need(this.#hooks.now)() + 0.1;
            if (node.flags & 1) { need(this.#hooks.redraw)(token, false); clearFlag(node, 1); } else { setFlag(node, 1); need(this.#hooks.redraw)(token, false); }
        }
    }

    // 43e1e0: 자식 제거 → 기본 부착 → 두 번의 현재 목록 순회 → 실제 항목 생성·가상 부착이다. 첫 순회는 mask 에 맞는 slot BYTE+1 의 최댓값, 자동 배치는 열=ceil(비율×sqrt(개수))·행=몫(+1 보정)이며
    // 칸 크기는 배정도 나눗셈을 float32 로 저장한다. 두 번째 순회는 현재 목록과 mask 를 다시 읽는다.
    attachGrid(token, x, y, parent, mode) {
        const node = this.#grid(token); need(this.#hooks.clearChildren)(token); need(this.#hooks.baseAttach)(token, x, y, parent, mode);
        const finder = new RawContainedFinder(this.#pool, this.#finder); let maximum = 0;
        // 빈 목록/불일치만 있으면 0 이다.
        for (let sid = finder.begin(node.object ? this.#content(node.object) : 0); sid; sid = finder.next()) {
            const raw = this.#pool.slot(sid); if (flagsOf(raw) & node.contentMask) maximum = Math.max(maximum, raw[23] + 1);
        }
        let columns = node.minimumColumns; let rows = node.minimumRows;
        if (!node.fixed) {
            // 원본은 순회 전 스택 칸을 0 으로 지우며 부착 mode 를 배치 계산에 사용하지 않는다.
            const count = Math.fround(maximum); const root = Math.fround(Math.sqrt(count));
            columns = Math.max(1, cameraFtolLow(Math.ceil((node.baseWidth / node.baseHeight) * root)));
            rows = Math.max(1, cameraFtolLow(count / columns));
            if (Math.imul(rows, columns) < maximum) rows += 1;
            if (columns === 1 && rows < node.minimumRows) rows = node.minimumRows;
            if (rows === 1 && columns < node.minimumColumns) columns = node.minimumColumns;
        }
        if (!columns || !rows) throw new RangeError('생산 격자 원본 0열/행 나눗셈 경로');
        node.cellWidth = Math.fround(node.width / columns); node.cellHeight = Math.fround(node.height / rows);
        // 두 번째 순회는 현재 목록과 현재 mask 를 다시 읽어 실제 내용물 SID 로 항목을 만든다.
        for (let sid = finder.begin(node.object ? this.#content(node.object) : 0); sid; sid = finder.next()) {
            const raw = this.#pool.slot(sid); if (!(flagsOf(raw) & node.contentMask)) continue;
            const slot = raw[23]; const child = need(this.#hooks.allocateItem)(ITEM_SIZE);
            // 원본은 이 뒤 null 을 역참조하므로 성공으로 바꾸지 않는다.
            if (!child) throw new Error('생산 항목 할당 실패');
            this.constructItem(child, node.cellWidth, node.cellHeight, sid); const item = this.#item(child);
            if (node.itemScale !== 0) item.iconScale = node.itemScale;
            item.requireImageHit = node.requireImageHit;
            need(this.#hooks.attach)(child, Math.fround((slot % columns) * node.cellWidth), Math.fround(Math.trunc(slot / columns) * node.cellHeight), token, 0);
        }
    }

    // 43d850: 내용물 통지/커서 허용/목록 mask/부모 전달/왼쪽 취소/기본 처리를 실행한다. 내용물 통지의 재부착은 부모 전달의 반환값을 무시하고 기본 처리까지 계속한다.
    handleGrid(token, event, first, second) {
        const node = this.#grid(token); const code = (event >>> 0) & 0x03ffffff;
        if (code === CONTENT_NOTICE) {
            const content = this.#content(first); const parent = need(this.#hooks.contentParent)(first);
            if (parent === node.object && (node.contentMask & flagsOf(this.#pool.slot(content)))) {
                need(this.#hooks.forward)(token, event, first, second); need(this.#hooks.push)(token); need(this.#hooks.attach)(token, node.x, node.y, 0, 0);
            }
        }
        if (code === CURSOR_QUERY) {
            const point = need(this.#hooks.local)(token, code, this.#state.cursorPoint);
            if (need(this.#hooks.inside)(token, point) && (node.acceptedCursor & this.#state.cursorFlags)) return 1;
        }
        if (code === MASK_QUERY) return node.contentMask >>> 0;
        if (code === FORWARD) return need(this.#hooks.forward)(token, event, first, second);
        if (!((event >>> 0) & 0x80000000) && code === 0x10000) { need(this.#hooks.cancelCursor)(); return 1; }
        return need(this.#hooks.baseHandle)(token, event, first, second);
    }

    // 43d9c0: 현재 부모 삭제 통지의 진단 → Push → 연결 해제 → 가상 재부착이다(Push/연결 해제의 부수 효과 뒤 현재 x/y 를 읽는다).
    parentGone(token, parent) {
        const node = this.#grid(token);
        if (parent !== node.object) need(this.#hooks.assertion)('sid == parentSid');
        need(this.#hooks.push)(token); need(this.#hooks.bind)(token, 0); need(this.#hooks.attach)(token, node.x, node.y, 0, 0);
    }
}
