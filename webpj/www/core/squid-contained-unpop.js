// 원본 10.78 공통 contained(종속) 해제 종단을 옮긴 모듈이다. 공통 Unpop 이 종속 객체를 만나면 이 종단으로 끝난다(004ac6b0 → 004ad230).
//  - detach: 진단 복귀를 보존하며 부모 종속 체인(+6 머리·+4 next·이전 링크)에서 자신을 빼고 contained 비트를 지운 뒤, 자산 또는 내용물 Form 이면 부모에 제거 이유 2 를 통지한다.
//  - notifyUi: 자신의 종속 Form 목록에 GumpForm 이 있으면 현재 내용물 변경 사건을 입력 큐에 한 번 넣는다(UI 검색 진단은 10.78 전용).
//  - 공통 링크 getter/setter: 실제 생성자 가상 표(VT)로 Form(+16 부모/+14 이전)과 자산(+14 부모/+16 이전)의 WORD 배치를 고른다. 모르는 VT 는 거부한다.
// void 비트는 여기서 쓰지 않는다(Form 전용 Unpop 과 구별). 부분 변경은 되돌리지 않고, 선택된 장치가 없으면 그 시점에 오류이다.
// 근거: cpppj `o/RawSquidContainedUnpop.cpp/.h`(d678fcad), `o/ContentNoticeVtables.inc`. 원본 x86 관측 표는 없고 cpppj 계약 검사(SquidContainedUnpopTests)를 옮긴다.
// CD 10.72 경로는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-05 WB04-D2: Unpop 처리기의 contained 종단으로 추가)
import { isSquidFormVtable, squidAssetVtable } from './squid-vtables.js';

// 원문 진단 파일 이름이다(10.78). (2026-11-05 WB04-D2)
const SQUID_CPP = '\\Ns\\O\\Squid.cpp';
const SQUID_H = 'c:\\ns\\o\\squid.h';
const SQUID_FINDER_CPP = '\\Ns\\O\\Squidfinder.cpp';

// 상태 BYTE 의 free·dead·contained 비트와 Form 검색의 마지막 TYPE(0x45)이다. (2026-11-05 WB04-D2)
const FREE = 1, DEAD = 2, CONTAINED = 8, LAST_FORM_TYPE = 0x45;

// 슬롯 WORD 를 읽는다(정렬 무관). (2026-11-05 WB04-D2)
function word(raw, offset) {
    return raw[offset] | (raw[offset + 1] << 8);
}

// 물리 슬롯의 WORD 만 쓴다(이웃 바이트 보존, 진단 복귀 뒤 free 슬롯도 물리 범위 안이면 쓴다). (2026-11-05 WB04-D2)
function putWord(pool, sid, offset, value) {
    const raw = pool.physical(sid);
    raw[offset] = value & 0xff;
    raw[offset + 1] = (value >>> 8) & 0xff;
}

// 선택된 장치를 꺼낸다. 없으면 오류이며 앞선 부분 변경은 그대로 남는다. (2026-11-05 WB04-D2)
function need(device, name) {
    if (typeof device !== 'function') throw new Error(`공통 contained 해제의 선택된 실제 장치(${name})가 연결되지 않았습니다.`);
    return device;
}

// 선택한 VT 가 Form 링크 배치이면 true, 지원 자산이면 false, 모르면 오류이다(타입/상태로 추정하지 않는다).
// (2026-11-05 WB04-D2, 2026-11-06 WB04-D5a: 저장한 VT 로도 고를 수 있게 분리 — cpppj f83db66c FormLinkLayoutAtTable 재반영)
function formLinkLayoutAtTable(table) {
    if (isSquidFormVtable(table)) return true;
    if (squidAssetVtable(table) !== null) return false;
    throw new Error('공통 contained 링크의 미복원 현재 가상 표');
}

// 슬롯의 현재 VT 로 링크 배치를 고른다(일반 경로는 호출 직전 현재 VT). (2026-11-05 WB04-D2)
function formLinkLayout(pool, sid) {
    const raw = pool.slot(sid);
    return formLinkLayoutAtTable((word(raw, 0) | (word(raw, 2) << 16)) >>> 0);
}

// 현재 VT+0 부모 getter 이다(Form +16, 자산 +14 WORD). (2026-11-05 WB04-D2)
export function commonContainedParent(pool, sid) {
    return word(pool.slot(sid), formLinkLayout(pool, sid) ? 16 : 14);
}

// 현재 VT+4 이전 getter 이다(Form +14, 자산 +16 WORD). (2026-11-05 WB04-D2)
export function commonContainedPrevious(pool, sid) {
    return word(pool.slot(sid), formLinkLayout(pool, sid) ? 14 : 16);
}

// 현재 VT+8 부모 setter 이다. 해당 WORD 만 바꾼다. (2026-11-05 WB04-D2)
export function commonContainedSetParent(pool, sid, parent) {
    putWord(pool, sid, formLinkLayout(pool, sid) ? 16 : 14, parent);
}

// 현재 VT+0xc 이전 setter 이다. 해당 WORD 만 바꾼다. (2026-11-05 WB04-D2)
export function commonContainedSetPrevious(pool, sid, previous) {
    putWord(pool, sid, formLinkLayout(pool, sid) ? 14 : 16, previous);
}

// 정렬 Content 부착이 앞서 저장한 VT 의 +4 이전 getter 이다(현재 VT 를 다시 읽지 않음). (2026-11-06 WB04-D5a: cpppj f83db66c CommonContainedPreviousAtTable 재반영)
export function commonContainedPreviousAtTable(pool, sid, table) {
    return word(pool.slot(sid), formLinkLayoutAtTable(table >>> 0) ? 14 : 16);
}

// 정렬 Content 부착이 고른 VT 의 +0xc 이전 setter 이다(해당 WORD 만 씀). (2026-11-06 WB04-D5a: cpppj f83db66c CommonContainedSetPreviousAtTable 재반영)
export function commonContainedSetPreviousAtTable(pool, sid, table, previous) {
    putWord(pool, sid, formLinkLayoutAtTable(table >>> 0) ? 14 : 16, previous);
}

// 공통 contained 해제 종단이다. pool 은 SidPool, state 는 현재 전역 getter 묶음(호출 시점마다 다시 읽음):
//  { contentFormType() (00541088, 원본 초기 6), gumpFormType() (0054107c, 초기 3), uiEvent() (005427dc), checking() (005e4794),
//    inMetamapMode() (00594fc8), boss() (00540bc4) },
// hooks 는 { parent(sid), previous(sid), setPrevious(sid, previous), parentNotice(parent, sid, reason), command(event, object, value), assertion(expression, file, line) } 이다.
// 사용: const contained = new RawSquidContainedUnpop(pool, state, hooks); unpop.bindContainedUnpop((sid) => contained.unpop(sid)). (2026-11-05 WB04-D2)
export class RawSquidContainedUnpop {
    // 같은 실제 풀이다.
    #pool;
    // 현재 전역 getter 묶음이다.
    #state;
    // 가상 호출·입력 큐·진단 장치이다.
    #hooks;

    // 생성 때는 장치를 요구하거나 풀을 바꾸지 않는다.
    constructor(pool, state, hooks = {}) {
        if (pool?.edition !== 'patch1078') throw new TypeError('공통 contained 해제는 10.78 SID 풀만 받습니다.');
        for (const name of ['contentFormType', 'gumpFormType', 'uiEvent']) {
            if (typeof state?.[name] !== 'function') throw new TypeError(`공통 contained 해제 전역 ${name} 은 함수여야 합니다.`);
        }
        this.#pool = pool;
        this.#state = state;
        this.#hooks = hooks ?? {};
    }

    // 같은 풀인지 확인하기 위한 실제 풀이다.
    get pool() { return this.#pool; }

    // 실제 부착 꼬리가 읽는 현재 내용물 Form TYPE 이다.
    get contentFormType() { return this.#state.contentFormType() >>> 0; }

    // Squid.cpp 진단을 부른다(정상 복귀하면 계속).
    #assert(expression, line) {
        need(this.#hooks.assertion, 'assertion')(expression, SQUID_CPP, line);
    }

    // 004ac6b0: 각 원문 위치에서 현재 슬롯을 다시 읽으며 종속 체인에서 자신을 뺀다. 부모가 dead 이면 링크를 그대로 둔다.
    detach(sid) {
        const pool = this.#pool;
        const hooks = this.#hooks;
        if (!(pool.slot(sid)[11] & CONTAINED)) this.#assert('containedFlag', 0x3a3);
        const parent = need(hooks.parent, 'parent')(sid);
        if (!parent) this.#assert('parentSid', 0x3a5);
        if (pool.slot(parent)[11] & FREE) this.#assert('!p.isFree()', 0x3a8);
        if (pool.slot(parent)[11] & DEAD) return;
        if (!(pool.slot(sid)[11] & CONTAINED)) this.#assert('containedFlag', 0x3b1);
        // 이전 getter 보다 먼저 읽은 next 는 이후 진단/통지에도 그대로 쓴다.
        const next = word(pool.slot(sid), 4);
        const previous = need(hooks.previous, 'previous')(sid);
        if (previous) {
            if (previous === next) need(hooks.assertion, 'assertion')('n != getSid()', SQUID_H, 0x123);
            putWord(pool, previous, 4, next);
        }
        if (next) need(hooks.setPrevious, 'setPrevious')(next, previous);
        if (word(pool.slot(parent), 6) === sid) {
            if (previous) this.#assert('prev == 0', 0x3c0);
            putWord(pool, parent, 6, next);
        }
        pool.physical(sid)[11] &= ~CONTAINED & 0xff;
        const type = pool.slot(sid)[10];
        if (type > LAST_FORM_TYPE || type === this.#state.contentFormType() >>> 0) need(hooks.parentNotice, 'parentNotice')(parent, sid, 2);
    }

    // 004ad230: (10.78) 현재 진단이 비0 이고 메타맵이 아니며 자신이 dead 이고 boss 가 아니면 UI 검색 진단을 부른다.
    // 그 뒤 자신의 +6 종속 목록을 next 를 먼저 저장하며 따라가 Form(TYPE ≤ 0x45) 중 현재 GumpForm TYPE 이 있으면 현재 사건을 한 번 큐에 넣는다.
    notifyUi(sid) {
        const pool = this.#pool;
        const state = this.#state;
        const checking = typeof state.checking === 'function' ? state.checking() : 0;
        if (checking !== 0 && !need(state.inMetamapMode, 'inMetamapMode')() && (pool.slot(sid)[11] & DEAD) && !need(state.boss, 'boss')()) {
            need(this.#hooks.assertion, 'assertion')('inMetamapMode || !cont.isDead() || boss || yesIKnowThatIShouldntLookInsideMyselfDuringADestroy', SQUID_FINDER_CPP, 0x158);
        }
        let next = word(pool.slot(sid), 6);
        const seen = new Set();
        // 순환만 소비 시점에 거부한다.
        while (next) {
            const current = next;
            if (seen.has(current)) throw new Error('공통 contained UI 검색의 종속 순환');
            seen.add(current);
            const raw = pool.slot(current);
            next = word(raw, 4);
            if (raw[10] > LAST_FORM_TYPE) continue;
            if (pool.slot(current)[10] === state.gumpFormType() >>> 0) {
                need(this.#hooks.command, 'command')(state.uiEvent() >>> 0, sid, 0);
                return;
            }
        }
    }

    // 두 하위 종단을 원래 순서로 실행한다. 분리가 정상 반환하면 부모 dead 여부와 무관하게 UI 검색을 한다.
    unpop(sid) {
        this.detach(sid);
        this.notifyUi(sid);
    }
}
