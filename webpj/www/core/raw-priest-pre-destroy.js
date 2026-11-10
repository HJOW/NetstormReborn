// 사제 preDestroy(004919b0)의 목록 제거·보호막 삭제 요청·Carrier 호출 순서를 옮긴 모듈이다(cpppj `o/RawPriestPreDestroy.cpp/.h`, 10.78 전용).
// 순서: 사제 TYPE/가상 표(0x50f210)와 free/contained 상태(비트 9)를 확인한다(dead/void 여도 실행). extra 의 abstract/buried 비트(9)가 없으면(일반) 사제 목록 개수를 확인하고, Carrier 사전 검사를 한 뒤,
//   목록에서 이 SID 를 모두 지워 활성 DWORD 만 압축하고(비활성 꼬리는 지우지 않음) 같은 소유자·위치의 보호막을 조회해 1 이상 풀 범위 안이면 보호막 삭제(가상 destroy, flags 0)를 요청한다. 마지막에 항상 Carrier 삭제 준비를 호출한다.
// 깊이 감소는 연결한 Carrier 의 책임이다. 이 몸체는 일반 사제 Pop/공간 해제를 허용하지 않는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E63: 사제 삭제 준비 이식으로 추가)

import { PRIEST_TYPE, PRIEST_VTABLE } from './raw-priest-owner.js';
import { SQUID_DESTROY_EFFECT } from './squid-destroy.js';
import { removeStateSidList } from './raw-state-sid-lists.js';

// 슬롯의 상태/extra 위치, 무효 상태 비트(free+contained), 접두를 건너뛰는 extra 비트이다. (2026-11-08 WB04-E63)
const STATE_OFFSET = 11;
const EXTRA_OFFSET = 40;
const INVALID_STATE = 9;
const SKIP_PREFIX = 9;

// pool 은 10.78 SidPool, state 는 PriestPostPopState, hooks = { findForcefield(sid) → sid, destroyForcefield(sid, flags), validateCarrier(sid, flags), carrierPre(sid, flags) } 이며 모두 필요하다. (2026-11-08 WB04-E63)
export class RawPriestPreDestroy {
    // 객체 슬롯 풀, 사제 목록 상태, 외부 효과이다.
    #pool;
    #state;
    #hooks;

    // 모든 외부 경계를 필수로 받아 부분 복원에 따른 효과 누락을 막는다. (2026-11-08 WB04-E63)
    constructor(pool, state, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('사제 삭제 준비는 10.78 풀만 받습니다.');
        if (typeof hooks?.findForcefield !== 'function' || typeof hooks.destroyForcefield !== 'function' || typeof hooks.validateCarrier !== 'function' || typeof hooks.carrierPre !== 'function') {
            throw new TypeError('사제 삭제 준비 경계 미연결');
        }
        this.#pool = pool;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 실제 분배기가 같은 풀인지 확인하는 풀이다. (2026-11-08 WB04-E63)
    get pool() {
        return this.#pool;
    }

    // 예약/풀 밖 번호 및 다른 파생 표는 사제 경로로 분배하지 않는다. (2026-11-08 WB04-E63)
    handles(sid) {
        if (sid < 5 || sid >= this.#pool.capacity) return false;
        const raw = this.#pool.slot(sid);
        return raw[10] === PRIEST_TYPE && ((raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24)) >>> 0) === PRIEST_VTABLE;
    }

    // 원본은 공통 Destroy 가 dead 를 켠 뒤 진입한다. 여기서 dead 를 이유로 생략하면 목록/회복이 남는다. (2026-11-08 WB04-E63)
    preDestroy(sid, flags) {
        if (!this.handles(sid) || (this.#pool.slot(sid)[STATE_OFFSET] & INVALID_STATE)) throw new TypeError('사제 삭제 준비 raw 상태 오류');
        const ordinary = (this.#pool.slot(sid)[EXTRA_OFFSET] & SKIP_PREFIX) === 0;
        const list = this.#state.currentPriests();
        if (ordinary) list.items();
        this.#hooks.validateCarrier(sid, flags);
        if (ordinary) {
            // 모든 일치 항목을 지우고 남은 활성 DWORD 만 압축한다. 비활성 꼬리는 원본처럼 지우지 않는다.
            //  PATCH 0040ea00 의 signed count 반복을 쓴다(음수 count 는 읽기/쓰기 없이 지나감, 물리 범위 밖은 그 칸에서 오류). (2026-11-10 WB04-E260: cpppj RemoveStateSidList 공유 재반영)
            removeStateSidList(list, sid);
            const forcefield = this.#hooks.findForcefield(sid);
            // 원본은 SID 1~4 도 주소 범위 안으로 인정한다. 타입/free/dead 를 여기서 추가로 검사하지 않는다.
            if (forcefield >= 1 && forcefield < this.#pool.capacity) this.#hooks.destroyForcefield(forcefield, 0);
        }
        this.#hooks.carrierPre(sid, flags);
    }
}

// 기존 삭제/ProcessHost 연결을 유지하면서 사제의 가상 preDestroy 만 처리하는 훅을 만든다. 다른 삭제 사건/선택/form 훅은 fallback 으로 넘긴다. (2026-11-08 WB04-E63)
export function makePriestPreDestroyHooks(pool, priest, fallback) {
    if (priest.pool !== pool || typeof fallback?.emit !== 'function') throw new TypeError('사제 삭제 분배 풀/후속 경계 오류');
    const emit = fallback.emit;
    return {
        ...fallback,
        emit: (event) => {
            if (event.effect === SQUID_DESTROY_EFFECT.preDestroy && priest.handles(event.sid)) priest.preDestroy(event.sid, event.flags);
            else emit(event);
        }
    };
}
