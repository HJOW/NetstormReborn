// 원본 10.78 공통 Squid 삭제(destroy, 004af780) 몸체를 옮긴 모듈이다. 순서: 범위/권한 확인 → dead 비트(중첩 재삭제 차단) → preDestroy 훅
// → 현재 종속 머리(+6)부터 next 를 먼저 저장하며 ReleaseDependent·(아직 free 가 아니면) DestroyDependent → 서버 객체 전파 → 선택 해제
// → 현재 TYPE/void 로 공간 해제(자산은 SquidUnpop flags 0, Form 은 가상 Unpop 훅) → postDestroy 훅 → SID 반납.
// 가상 삭제 훅·종속 release/destroy·전파·선택 해제는 호출자 장치(emit)이며, pre/post 훅은 completePreDestroy/completePostDestroy 를 정확히 한 번 불러야 한다.
// 다리의 vtable +0x10 재정의(가상 분배)는 이 몸체 앞단이며 다리 수명 단계에서 옮긴다.
// 근거: cpppj `o/RawSquidDestroy.cpp/.h`(f2215dc5·886f2d23), 원본 관측 `cpppj/tests/fixtures/destroy-x86.tsv`(10.78 Destroy 600행·Bridge 64행).
// CD 10.72·10.37 경로는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-05 WB04-D2: 공통 삭제기 이식으로 추가)
import { SquidUnpop } from './squid-unpop.js';

// 삭제 훅이 받는 효과 이름이다. (2026-11-05 WB04-D2)
export const SQUID_DESTROY_EFFECT = Object.freeze({
    preDestroy: 'preDestroy',
    releaseDependent: 'releaseDependent',
    destroyDependent: 'destroyDependent',
    transmit: 'transmit',
    clearSelection: 'clearSelection',
    postDestroy: 'postDestroy',
});

// 슬롯 필드 위치와 상태 비트이다(10.78). (2026-11-05 WB04-D2)
const TYPE = 10, STATE = 11, NEXT = 4, HEAD = 6;
const FREE = 1, DEAD = 2, VOID = 4, CONTAINED = 8;

// 원본 서버 수신·client 전파 억제·종속 삭제 플래그이다. (2026-11-05 WB04-D2)
const SERVER = 8, CLIENT = 0x10, DEPENDENT = 0x40;

// 첫 자산 TYPE 번호이다. 그보다 작으면 Form 이다. (2026-11-05 WB04-D2)
const FIRST_ASSET_TYPE = 70;

// 슬롯 WORD 를 읽는다. (2026-11-05 WB04-D2)
function word(raw, offset) {
    return raw[offset] | (raw[offset + 1] << 8);
}

// 공통 삭제 몸체이다. pool 은 SidPool, unpop 은 같은 풀의 SquidUnpop, types 는 188칸 TYPE 레코드 표이다(공간 해제 인자로 넘김).
// 사용: const destroy = new RawSquidDestroy(pool, unpop, types);
//       destroy.destroy(sid, flags, { emit(event) { … destroy.completePreDestroy() … }, selected: () => selectedSid, unpopForm(sid) { … } }). (2026-11-05 WB04-D2)
export class RawSquidDestroy {
    // 같은 SID 풀·공간 해제 처리기·TYPE 표이다.
    #pool;
    #unpop;
    #types;
    // 원본 curDestroy/curPostDestroy 깊이 카운터(DWORD 감김)이다.
    #preDepth = 0;
    #postDepth = 0;

    // 다른 풀의 공간 해제와 섞는 연결을 거부한다.
    constructor(pool, unpop, types) {
        if (pool?.edition !== 'patch1078' || !(unpop instanceof SquidUnpop)) throw new TypeError('Destroy 의 10.78 풀/Unpop 연결 오류');
        if (unpop.pool !== pool) throw new TypeError('Destroy pool mismatch');
        if (types?.length !== 188) throw new RangeError('Destroy TYPE 표는 188칸이어야 합니다.');
        this.#pool = pool;
        this.#unpop = unpop;
        this.#types = types;
    }

    // 같은 실제 풀이다.
    get pool() { return this.#pool; }

    // 삭제 장부(SquidDestroyLifecycle)의 Graph 가 이 삭제기의 공간 해제 어댑터와 같은 해시/spot 을 읽는지 확인한다(다른 지도에 Graph 를 갱신하는 연결을 막는다). (2026-11-10 WB04-E213: 삭제 장부 이식으로 추가)
    validateGraph(graph) { this.#unpop.validateGraph(graph); }

    // 같은 TYPE 표이다(복사하지 않음).
    get types() { return this.#types; }

    // 현재 preDestroy 깊이이다.
    get preDepth() { return this.#preDepth; }

    // 현재 postDestroy 깊이이다.
    get postDepth() { return this.#postDepth; }

    // 공통 preDestroy 의 마지막 깊이 감소이다. 그 밖의 효과는 하지 않는다.
    completePreDestroy() { this.#preDepth = (this.#preDepth - 1) >>> 0; }

    // 공통 postDestroy 의 마지막 깊이 감소이다.
    completePostDestroy() { this.#postDepth = (this.#postDepth - 1) >>> 0; }

    // 004af780: 이미 dead 이면 바로 끝난다. 원본 assert 에 해당하는 미지원 루트·권한은 dead 표시 전에 거부한다.
    // hooks: { emit(event: { effect, sid, parent, flags }), selected(): SID, unpopForm?(sid) }. emit 은 같은 풀을 바꿀 수 있고 각 단계 뒤 현재 필드를 다시 읽는다.
    destroy(sid, flags, hooks) {
        const pool = this.#pool;
        const layout = pool.layout;
        if (!Number.isInteger(sid) || sid < 5 || sid === layout.predictableFirst || sid >= pool.capacity) throw new RangeError('Destroy SID');
        let raw = pool.slot(sid);
        if (raw[STATE] & DEAD) return;
        // 자산 루트는 contained 가 아니어야 하고, Form 루트는 전용 Unpop 훅이 있어야 한다.
        const form = raw[TYPE] < FIRST_ASSET_TYPE;
        if ((raw[STATE] & FREE) || raw[TYPE] >= this.#types.length || (form ? typeof hooks?.unpopForm !== 'function' : (raw[STATE] & CONTAINED) !== 0)) {
            throw new Error('Destroy unsupported root');
        }
        if (typeof hooks?.emit !== 'function' || typeof hooks?.selected !== 'function') throw new TypeError('Destroy hooks');
        let current = flags >>> 0;
        const client = sid < layout.serverFirst;
        if (!pool.isServer && !client && !(current & SERVER)) throw new Error('Destroy server authority');
        if (pool.isServer && client) current = (current | CLIENT) >>> 0;
        pool.allocated(sid)[STATE] |= DEAD;
        const pre = this.#preDepth;
        this.#preDepth = (this.#preDepth + 1) >>> 0;
        hooks.emit({ effect: SQUID_DESTROY_EFFECT.preDestroy, sid, parent: 0, flags: current });
        if (this.#preDepth !== pre) throw new Error('Destroy pre completion');
        let dependent = word(pool.slot(sid), HEAD);
        const visited = new Set();
        // preDestroy 가 바꾼 현재 머리부터 처리하며 각 next 는 release 전에 저장한다.
        while (dependent !== 0) {
            if (dependent >= pool.capacity || visited.has(dependent)) throw new Error('Destroy dependent chain');
            visited.add(dependent);
            const child = pool.slot(dependent);
            if ((child[STATE] & FREE) || (!(child[STATE] & CONTAINED) && child[TYPE] >= FIRST_ASSET_TYPE)) throw new Error('Destroy dependent state');
            const next = word(child, NEXT);
            hooks.emit({ effect: SQUID_DESTROY_EFFECT.releaseDependent, sid: dependent, parent: sid, flags: 0 });
            if (!(pool.slot(dependent)[STATE] & FREE)) {
                hooks.emit({ effect: SQUID_DESTROY_EFFECT.destroyDependent, sid: dependent, parent: sid, flags: (current | DEPENDENT) >>> 0 });
            }
            dependent = next;
        }
        if (pool.slot(sid)[STATE] & FREE) throw new Error('Destroy root released during pre');
        if (!(current & (CLIENT | DEPENDENT)) && pool.isServer && !client) {
            hooks.emit({ effect: SQUID_DESTROY_EFFECT.transmit, sid, parent: 0, flags: (current | SERVER) >>> 0 });
        }
        if (hooks.selected() === sid) hooks.emit({ effect: SQUID_DESTROY_EFFECT.clearSelection, sid, parent: 0, flags: 0 });
        raw = pool.slot(sid);
        // 파생 효과 뒤의 현재 TYPE/void 로 공간 해제를 고른다.
        if (!(raw[STATE] & VOID)) {
            if (raw[TYPE] >= this.#types.length) throw new RangeError('Destroy live type');
            if (raw[TYPE] < FIRST_ASSET_TYPE) {
                // Form 의 가상 Unpop 은 void 를 켜고 부모 종속 체인에서 빠진다.
                if (typeof hooks.unpopForm !== 'function') throw new Error('Destroy form unpop hook');
                hooks.unpopForm(sid);
                if (!(pool.slot(sid)[STATE] & VOID)) throw new Error('Destroy form unpop result');
            } else {
                this.#unpop.unpop(sid, this.#types[raw[TYPE]], 0);
            }
        }
        const post = this.#postDepth;
        this.#postDepth = (this.#postDepth + 1) >>> 0;
        hooks.emit({ effect: SQUID_DESTROY_EFFECT.postDestroy, sid, parent: 0, flags: current });
        if (this.#postDepth !== post) throw new Error('Destroy post completion');
        pool.release(sid);
    }
}
