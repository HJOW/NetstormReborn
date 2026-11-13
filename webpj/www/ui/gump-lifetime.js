// 같은 생성/트리 저장소의 지연 삭제·기본 소멸자·idle/overlay/dialog 표를 이식한다(cpppj client/GumpLifetime.cpp/.h, 기준선1f00a25a).
// 사용: new GumpLifetime(createState,lifeState,{node,push?,deleteForm?,deletingDestructor?,release?,assertion?}). 선택된 장치만 요구한다. (2026-11-14 WB04-E412)

// 원본 기본 상태 비트와 패치 기본VT다. VT는 호스트에서 호출하지 않는다.
const IDLE = 0x80; const OVERLAY = 0x80000; const VOID = 8; const DEAD = 0x10000; const DESTROYING = 4; const UNREGISTERED = 0x100000; const PATCH_VTABLE = 0x50ac70;
// 도달한 실제 장치만 요구한다. 진단이 복귀하면 바로 다음 원문 단계로 이어진다.
function need(device) { if (typeof device !== 'function') throw new Error('Gump 수명 진단의 선택 장치 미연결'); return device; }

// 원본 고정 표와 현재 삭제/진단 전역을 같은 Client 수명에 소유한다.
export class GumpLifetimeState {
    // 현재 idle500·overlay100·삭제큐1000·dialog16DWORD다.
    idle = new Uint32Array(500); overlay = new Uint32Array(100); kill = new Uint32Array(1000); dialogs = new Uint32Array(16);
    // 삭제/대화상자 개수와 focus/현재dialog, 독립 진단DWORD다.
    killCount = 0; dialogCount = 0; focus = 0; currentDialog = 0; diagnostics = 0;
    // 실제 Client의 현재 checking getter다. null이면 기존 독립 diagnostics를 호출 때 읽는다.
    checking = null;
    // 0/1 사본 대신 원래 전체DWORD를 소비 지점에서 읽는다.
    currentChecking() { return (this.checking ? this.checking() : this.diagnostics) >>> 0; }
}

// 4653c0의 마지막 free/found를 고정한다. 음수 쓰기의 호스트 장치는 호출자가 실제 경계에서 구별한다.
export function runGumpOverlayParent(token, enabled, entries, checking, hooks) {
    let found = -1; let empty = -1;
    // 진단 전에 전체100칸을 순서대로 읽으며 마지막 두 후보를 고른다.
    for (let index = 0; index < 100; index += 1) { if (!entries[index]) empty = index; if (entries[index] === token) found = index; }
    if (!enabled) {
        if (found === -1) { if ((checking() >>> 0) !== 0) need(hooks.assertion)('found!=-1', '.\\Gump.cpp', 0x4f4); return; }
        need(hooks.write)(found, 0); need(hooks.setFlags)((need(hooks.flags)() & ~OVERLAY) >>> 0); return;
    }
    if (found !== -1 || empty === -1) need(hooks.assertion)('found==-1 && free!=-1', '.\\Gump.cpp', 0x4ef);
    need(hooks.write)(empty, token); need(hooks.setFlags)((need(hooks.flags)() | OVERLAY) >>> 0);
}

// Kill 요청과 Cleanup의 실제 해제를 분리하며 같은 등록표/노드 주소를 빌린다.
export class GumpLifetime {
    // 장기 생성/수명 상태와 현재 가상·해제 장치다.
    #state; #life; #hooks;
    // 노드 조회만 선행 필수다. 수명보다 먼저 저장소를 반납하지 않는다.
    constructor(state, lifetime, hooks = {}) { if (typeof hooks.node !== 'function') throw new TypeError('Gump 삭제 저장소 필요'); this.#state = state; this.#life = lifetime; this.#hooks = hooks; }
    // 같은 안정 노드를 공급하며 없는 토큰은 도달한 때 거부한다.
    #node(token) { const node = token ? this.#hooks.node(token >>> 0) : null; if (!node) throw new Error('Gump 삭제 토큰/저장소 없음'); return node; }
    // 첫 빈/일치 idle 한 칸만 쓰고 마지막에 현재 노드 플래그를 쓴다.
    setIdle(token, enabled) {
        const node = this.#node(token); enabled = !!enabled; if (enabled === !!(node.flags & IDLE)) return;
        // 패치의 실제500칸 한계다. 표/플래그 불일치 시 성공으로 생략하지 않는다.
        for (let i = 0; i < 500; i += 1) if (this.#life.idle[i] === (enabled ? 0 : token)) { this.#life.idle[i] = enabled ? token : 0; node.flags = (enabled ? node.flags | IDLE : node.flags & ~IDLE) >>> 0; return; }
        throw new RangeError('Gump idle 원본 표/플래그 계약 위반');
    }
    // 선택 진단 뒤의 signed 쓰기만 실제 배열 경계에서 검사한다. 선행 노드 조회를 덧붙이지 않는다.
    setOverlay(token, enabled) {
        runGumpOverlayParent(token, !!enabled, this.#life.overlay, () => this.#life.currentChecking(), { assertion: this.#hooks.assertion,
            write: (index, value) => { if (index < 0 || index >= this.#life.overlay.length) throw new RangeError('Gump overlay 진단 복귀 뒤 실제 배열 범위'); this.#life.overlay[index] = value; },
            flags: () => this.#node(token).flags, setFlags: (value) => { this.#node(token).flags = value; } });
    }
    // 첫 dialog 일치만 앞으로 압축한다. 없는 토큰에는 노드 조회가 필요 없다.
    removeDialog(token) {
        const life = this.#life; if (life.dialogCount > life.dialogs.length) throw new RangeError('Gump dialog 표 범위 초과');
        // 뒤 미사용 칸은 보존하며 사용 구간 마지막 한 칸만0으로 쓴다.
        for (let i = 0; i < life.dialogCount; i += 1) if (life.dialogs[i] === token) { life.dialogs.copyWithin(i, i + 1, life.dialogCount); life.dialogCount = (life.dialogCount - 1) >>> 0; life.dialogs[life.dialogCount] = 0; life.currentDialog = life.dialogCount ? life.dialogs[life.dialogCount - 1] : 0; return true; }
        return false;
    }
    // 공개 요청마다 독립 자식 재귀 경로를 쓴다. 중복 Kill을 제거하지 않는다.
    kill(token) { this.#recurse(token, false, []); }
    // 즉시 파괴도 자식 우선으로 현재 등록 해제/가상 소멸자를 실행한다.
    destroy(token) { this.#recurse(token, true, []); }
    // next는 자식 호출 전에 저장하고 콜백 뒤 현재 부모 플래그/카운터를 읽는다.
    #recurse(token, destroy, active) {
        const node = this.#node(token); if (active.includes(token)) throw new Error('Gump 삭제 자식 순환'); active.push(token); const siblings = new Set();
        // 자식 소멸자가 해제할 수 있으므로 재귀 뒤 자식을 다시 읽지 않는다.
        for (let child = node.child; child;) { if (siblings.has(child)) throw new Error('Gump 삭제 형제 순환'); siblings.add(child); const next = this.#node(child).next; this.#recurse(child, destroy, active); child = next; }
        if (destroy) {
            if (!(node.flags & UNREGISTERED)) {
                if (node.id >= this.#state.entries.length) throw new RangeError('Gump 등록 해제 id 범위 초과'); this.#state.entries[node.id] = 0; this.#state.count = (this.#state.count - 1) >>> 0; this.#state.collection.root = this.#state.entries[0]; if ((this.#state.count | 0) < 0) throw new Error('Gump 등록 개수 음수');
            }
            node.flags = ((node.flags & ~DEAD) | DESTROYING) >>> 0; need(this.#hooks.deletingDestructor)(token, 1);
        } else {
            if (node.flags & IDLE) this.setIdle(token, false); if (node.flags & OVERLAY) this.setOverlay(token, false);
            if (node.flags & 0x40) need(this.#hooks.deleteForm)(node.form, 0); if (!(node.flags & VOID)) need(this.#hooks.push)(token);
            if (this.#life.killCount >= 999) throw new RangeError('Gump 삭제 큐 원본 한도 초과'); this.#life.kill[this.#life.killCount] = token; this.#life.killCount = (this.#life.killCount + 1) >>> 0;
            if (this.#life.focus === token) this.#life.focus = 0; if (this.#state.collection.overrideHead === token) this.#state.collection.overrideHead = 0; this.removeDialog(token); node.flags = (node.flags | DEAD) >>> 0;
        }
        active.pop();
    }
    // 비0항목의 실제 Destroy 직후만 현재 개수를 다시 읽는다. null항목은 로컬 반복 한계를 바꾸지 않는다.
    cleanup() {
        let count = this.#life.killCount; if (count > this.#life.kill.length) throw new RangeError('Gump 정리 큐 범위 초과');
        // 현재 큐를 순서대로 진행하며 소멸자가 뒤의 중복을0으로 만들 수 있다.
        for (let i = 0; i < count; i += 1) { const token = this.#life.kill[i]; if (!token) continue; if (!(this.#node(token).flags & DEAD)) throw new Error('Gump 정리 대상 dead 필요'); this.destroy(token); count = this.#life.killCount; if (count > this.#life.kill.length) throw new RangeError('Gump 정리 콜백 큐 범위 초과'); }
        this.#life.kill.fill(0, 0, count); this.#life.killCount = 0;
    }
    // 루트 밖999칸 전체를 보며 각 현재 등록 칸의 제외 signed BYTE를 getter로 다시 읽는다.
    killExceptType(excludedType) {
        // count/visible/void/미등록을 추가 필터로 쓰지 않는다.
        for (let index = 1; index < this.#state.entries.length; index += 1) { const token = this.#state.entries[index]; if (!token) continue; const excluded = ((excludedType() << 24) >> 24) >>> 0; if (this.#node(token).command !== excluded && !(this.#node(token).flags & DEAD)) this.kill(token); }
    }
    // 기존 루트가 없어도 현재 삭제 큐를 정리한다. 개수/serial/신규 객체의 이전 등록 칸은 그대로다.
    replaceRoot(token) { if (this.#state.entries[0]) this.kill(this.#state.entries[0]); this.cleanup(); this.#state.entries[0] = token; this.#state.collection.root = token; this.#node(token).id = 0; }
    // 기본 VT→선택 진단→두 현재 텍스트 해제→세 표의 모든 일치 제거 순서다.
    destructBase(token) {
        const node = this.#node(token); node.vtable = PATCH_VTABLE; if (this.#life.currentChecking() && !(node.flags & DESTROYING)) need(this.#hooks.assertion)('flags & GF_DESTROYING', '.\\Gump.cpp', 0x1e1);
        if (node.textPointer) { need(this.#hooks.release)(node.textPointer); node.textPointer = 0; } if (node.alternateTextPointer) { need(this.#hooks.release)(node.alternateTextPointer); node.alternateTextPointer = 0; }
        // 각 표의 모든 중복을 지우되 flags/개수/dialog에는 쓰지 않는다.
        for (let i = 0; i < this.#life.overlay.length; i += 1) if (this.#life.overlay[i] === token) this.#life.overlay[i] = 0;
        for (let i = 0; i < 500; i += 1) if (this.#life.idle[i] === token) this.#life.idle[i] = 0;
        for (let i = 0; i < this.#life.kill.length; i += 1) if (this.#life.kill[i] === token) this.#life.kill[i] = 0;
    }
    // flags의 하위BYTE bit0일 때 객체 자체를 해제하고 객체를 다시 읽지 않은 채 원래 토큰을 반환한다.
    deleteScalar(token, flags) { this.destructBase(token); if (flags & 1) need(this.#hooks.release)(token); return token; }
}
