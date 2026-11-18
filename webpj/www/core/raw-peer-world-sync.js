// 참가자별 월드 동기화의 서버 SID·종속 객체·이동 객체 순회를 수행한다(cpppj o/RawPeerWorldSync.cpp, 225553df).
// 사용: new RawPeerWorldSync(pool, types, state, transmission, hooks).synchronize(owner).
// pool·TYPE·송신 전역·전송기는 같은 실제 월드를 빌린다. 선택된 sharedRegularType·unpop·repop 장치는 호출 위치에서 요구한다.

// 원본 서버 영역의 양끝과 표면·이동 객체 TYPE 플래그다.
const FIRST_SERVER = 15000, LAST_SERVER = 23000;
const SURFACE = 0x800, WALKER = 0x10000;

// 현재 슬롯의 비정렬 WORD를 읽는다.
function word(pool, sid, offset) { const raw = pool.slot(sid); return raw[offset] | (raw[offset + 1] << 8); }

// 선택된 실제 장치만 호출 위치에서 요구한다.
function need(hook) { if (typeof hook !== 'function') throw new Error('월드 동기화의 선택된 실제 장치 미연결'); return hook; }

// 장기 풀·TYPE·송신 전역과 외부 가상 호출 장치를 빌리는 동기화 부모다.
export class RawPeerWorldSync {
    // 순회 중 현재 물리와 장치를 직접 다시 읽는다.
    #pool; #types; #state; #transmission; #hooks;

    // 생성만으로 전역이나 슬롯을 변경하지 않는다.
    constructor(pool, types, state, transmission, hooks = {}) {
        if (!pool || pool.edition !== 'patch1078') throw new TypeError('현재 월드 동기화는 10.78 전용');
        this.#pool = pool; this.#types = types; this.#state = state; this.#transmission = transmission; this.#hooks = hooks;
    }

    // 자식 호출이 반환된 뒤 sibling을 다시 읽는다. 명시 스택은 깊은 종속 순환을 Pool 한도로 진단한다.
    #children(parent) {
        const pool = this.#pool, stack = [{ child: word(pool, parent, 6), visited: 0, resume: null }];
        // 각 프레임은 같은 부모의 현재 sibling 사슬과 재귀 복귀 위치를 보관한다.
        while (stack.length) {
            const frame = stack[stack.length - 1];
            if (frame.resume !== null) { frame.child = word(pool, frame.resume, 4); frame.resume = null; continue; }
            if (!frame.child) { stack.pop(); continue; }
            if (++frame.visited > pool.capacity) throw new RangeError('동기화 종속 sibling 순환');
            const sid = frame.child;
            if (sid >= FIRST_SERVER && sid <= LAST_SERVER) {
                this.#transmission.sendTo(sid, this.#state.syncTarget | 0, 1);
                const child = word(pool, sid, 6);
                if (child) {
                    if (stack.length > pool.capacity) throw new RangeError('동기화 종속 깊이의 실제 Pool 범위');
                    frame.resume = sid; stack.push({ child, visited: 0, resume: null }); continue;
                }
            } else if (sid >= 5 && sid < FIRST_SERVER) {
                const type = pool.slot(sid)[10];
                if (type < 70 && type === need(this.#hooks.sharedRegularType)()) this.#transmission.sendTo(sid, this.#state.syncTarget | 0, 1);
            }
            frame.child = word(pool, sid, 4);
        }
    }

    // 표면·그 외 서버 자산을 차례로 송신하고 현재 이동 객체를 공간에서 다시 등록한다.
    synchronize(owner) {
        const pool = this.#pool, state = this.#state, types = this.#types, wire = this.#transmission;
        state.clientSidAllowed = 1; wire.sameOwnerMode(1); state.syncTarget = owner >>> 0;
        // 각 단계는 서버 고정 범위를 처음부터 다시 읽는다.
        for (const surface of [true, false]) {
            for (let sid = FIRST_SERVER; sid <= LAST_SERVER; sid += 1) {
                const raw = pool.slot(sid); if ((raw[11] & 1) || raw[10] <= 69) continue;
                const type = types[raw[10]]; if (!type) throw new RangeError('동기화 현재 TYPE 범위');
                if (Boolean(type.flags1 & SURFACE) !== surface) continue;
                wire.sendTo(sid, state.syncTarget | 0, 1); this.#children(sid);
            }
        }
        wire.sameOwnerMode(0); const limit = pool.capacity; let sid = 2;
        // 처음 살아 있는 슬롯은 2부터 찾고 가상 호출 뒤 현재 free 상태를 다시 읽는다.
        while (sid < limit && (pool.slot(sid)[11] & 1)) sid += 1;
        for (;;) {
            if (sid < 1 || sid >= pool.capacity) { state.clientSidAllowed = 0; return; }
            const type = types[pool.slot(sid)[10]]; if (!type) throw new RangeError('동기화 walker의 현재 TYPE 범위');
            if (type.flags2 & WALKER) { need(this.#hooks.unpop)(sid, 0); need(this.#hooks.repop)(sid, 0); }
            sid += 1;
            while (sid < limit && (pool.slot(sid)[11] & 1)) sid += 1;
        }
    }
}
