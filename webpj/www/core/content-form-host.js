// 원본 10.78 ContentForm(TYPE 6, 생산 칸/유닛 목록 내용물 Form)의 정렬 부착 부모(004aec10)와 호스트를 옮긴 모듈이다.
//  - attachSortedContent: 부모/자신의 dead·free 진단(정상 복귀 뒤 다시 판정해 dead/free 면 false) → 같은 목록 그룹(+20 flags 의 0x300 제외)의
//    현재 contentFormType 항목 중 순서 BYTE(+23)가 지금 순서보다 큰 첫 항목 앞에 삽입(없으면 끝) → 순서 BYTE·부모 setter·contained 비트
//    → 공통 Pop(flags|0x100) → 저장한 부모에 이유 1 통지. 링크 getter/setter 는 저장한 VT 또는 현재 VT 로 고른다. 부분 실패는 되돌리지 않는다.
//  - ContentFormHost: 정렬 부착 뒤 전투 중 사제 포획 꼬리(내용물 TYPE genus 0x200000·부모 genus 0x30000·현재 전투)와 checking 진단,
//    Unpop(부모 제거 통지 2), Robust(+20 의 0x34, WORLD 0x80 진단), Linked(부모 체인 전체 검사), Damage(양수면 Unpop→0x100 해제→현재 가상 재부착),
//    BindCreate(contained 생성기 연결)를 제공한다.
// 근거: cpppj `o/RawContentAttach.cpp/.h`(f83db66c), `o/ContentFormHost.cpp/.h`. 원본 관측: content-attach-current-x86.tsv(10.78 41행),
//       formlifecycle-x86.tsv 의 Attach/Robust/Damage 행(tests/form-hosts.test.mjs). CD 10.72 의 ValidatePop 기반 정렬 경로는 범위 밖이다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-06 WB04-D5a: Form 호스트 이식으로 추가)
import { SquidForms } from './squid-forms.js';
import { SQUID_FORM_VTABLES } from './squid-vtables.js';
import { commonContainedParent, commonContainedPreviousAtTable, commonContainedSetParent, commonContainedSetPreviousAtTable } from './squid-contained-unpop.js';
import { checkRawFormLinked } from './raw-form-linked.js';
import { makeContainedCreateHooks } from './contained-create.js';

// 원문 진단 파일 이름이다(10.78). (2026-11-06 WB04-D5a)
const SQUID_CPP = '\\Ns\\O\\Squid.cpp';
const SQUID_H = 'c:\\ns\\o\\squid.h';
// 공통 링크·TYPE·상태·포획 이전 소유자(+12)·생산 TYPE(+18)·목록 flags(+20)·순서 BYTE(+23) 위치이다. (2026-11-06 WB04-D5a)
const NEXT = 4, HEAD = 6, TYPE = 10, STATE = 11, PREVIOUS_OWNER = 12, CONTENT_TYPE = 18, LIST_FLAGS = 20, ORDER = 23;
// dead|free·contained 비트, 그룹 비교 마스크, WORLD·Robust·변경(0x100) flags 이다. (2026-11-06 WB04-D5a)
const DEAD_OR_FREE = 3, CONTAINED = 8, GROUP_MASK = 0xfcff, WORLD = 0x80, ROBUST = 0x34;
// 사제 포획 꼬리의 내용물 genus(사제)·부모 genus(워커/벌룬) 비트이다. (2026-11-06 WB04-D5a)
const PRIEST_GENUS = 0x200000, CARRIER_GENUS = 0x30000;

// 선택된 장치를 꺼낸다(호출 순간의 장치를 끝까지 쓴다). 없으면 그 위치에서 오류이다. (2026-11-06 WB04-D5a)
function need(device, name, where) {
    if (typeof device !== 'function') throw new Error(`${where}의 선택된 장치(${name})가 연결되지 않았습니다.`);
    return device;
}

// 슬롯 WORD/VT 읽기와 WORD 쓰기이다. (2026-11-06 WB04-D5a)
function word(raw, offset) {
    return raw[offset] | (raw[offset + 1] << 8);
}
function vtableOf(raw) {
    return (raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24)) >>> 0;
}
function putWord(raw, offset, value) {
    raw[offset] = value & 0xff;
    raw[offset + 1] = (value >>> 8) & 0xff;
}

// 004aba70: 자기 next 진단(정상 복귀 시 계속) 뒤 저장한 this 의 +4 에 next 를 쓴다. (2026-11-06 WB04-D5a)
function contentNext(pool, sid, next, hooks) {
    const raw = pool.physical(sid);
    if (next === sid) need(hooks.assertion, 'assertion', '정렬 Content 부착')('n != getSid()', SQUID_H, 0x123);
    putWord(raw, NEXT, next);
}

// 004aec10(10.78): 정렬 Content 부착 부모이다. hooks: { contentType() → DWORD, previous(sid, VT) → SID, setPrevious(sid, VT, SID), setParent(sid, VT, SID),
// pop(sid, flags), notice(parent, form, reason), assertion(expression, file, line) }. dead/free 반환은 false, 통지까지 마치면 true 다. (2026-11-06 WB04-D5a)
export function attachSortedContent(pool, form, parent, flags, hooks = {}) {
    if (pool?.edition !== 'patch1078') throw new TypeError('현재 정렬 Content 부모의 10.78 계약');
    const where = '정렬 Content 부착';
    // 부모 this 를 첫 진단 앞에서 저장한다.
    const parentRaw = pool.physical(parent);
    const raw = pool.physical(form);
    if (parentRaw[STATE] & DEAD_OR_FREE) need(hooks.assertion, 'assertion', where)('!into.isDead() && !into.isFree()', SQUID_CPP, 0xbe6);
    if (raw[STATE] & DEAD_OR_FREE) need(hooks.assertion, 'assertion', where)('!isDead() && !isFree()', SQUID_CPP, 0xbe7);
    if ((parentRaw[STATE] & DEAD_OR_FREE) || (raw[STATE] & DEAD_OR_FREE)) return false;
    const group = word(raw, LIST_FLAGS);
    let order = 0;
    let current = word(parentRaw, HEAD);
    let previous = 0;
    let inserted = false;
    const visited = new Set();
    // 다음 WORD 와 현재 content DWORD 를 원문 위치에서 읽는다.
    while (current) {
        if (visited.has(current)) throw new Error('정렬 Content 부착의 비종료 체인 순환');
        visited.add(current);
        const candidate = pool.physical(current);
        const content = need(hooks.contentType, 'contentType', where)() >>> 0;
        if (candidate[TYPE] === content && ((word(candidate, LIST_FLAGS) ^ group) & GROUP_MASK) === 0) {
            if (order < candidate[ORDER]) {
                contentNext(pool, form, current, hooks);
                // 이웃 getter 보다 먼저 저장한 자기 VT 이다.
                const ownTable = vtableOf(raw);
                const before = need(hooks.previous, 'previous', where)(current, vtableOf(candidate));
                need(hooks.setPrevious, 'setPrevious', where)(form, ownTable, before);
                need(hooks.setPrevious, 'setPrevious', where)(current, vtableOf(candidate), form);
                if (!need(hooks.previous, 'previous', where)(form, vtableOf(raw))) putWord(parentRaw, HEAD, form);
                else contentNext(pool, need(hooks.previous, 'previous', where)(form, vtableOf(raw)), form, hooks);
                inserted = true;
                break;
            }
            order = (order + 1) >>> 0;
        }
        previous = current;
        current = word(candidate, NEXT);
    }
    if (!inserted) {
        if (previous) {
            contentNext(pool, previous, form, hooks);
            need(hooks.setPrevious, 'setPrevious', where)(form, vtableOf(raw), previous);
            contentNext(pool, form, 0, hooks);
        } else {
            putWord(parentRaw, HEAD, form);
            contentNext(pool, form, 0, hooks);
            need(hooks.setPrevious, 'setPrevious', where)(form, vtableOf(raw), 0);
        }
    }
    raw[ORDER] = order & 0xff;
    need(hooks.setParent, 'setParent', where)(form, vtableOf(raw), parent);
    raw[STATE] |= CONTAINED;
    need(hooks.pop, 'pop', where)(form, (flags | 0x100) >>> 0);
    need(hooks.notice, 'notice', where)(parent, form, 1);
    return true;
}

// ContentForm 호스트이다. notice(parent, form, reason) 는 부모 가상 +0x24(부착 1/제거 2) 통지이며 필수다.
// options: { transmit(form, flags) — Pop 서버 전송, state: { inBattle(), checkingLinks(), assertion(expression, file, line), contentType() } — 실제 월드 전역(호출마다 읽음),
//            captured(parent, previousOwner) — 전투 중 사제 포획 알림 }. 사용: const content = new ContentFormHost(pool, types, notice, { state }); content.attach(form, parent, 0x10). (2026-11-06 WB04-D5a)
export class ContentFormHost {
    // 같은 풀·TYPE 사본·빌린 현재 표·통지·Form 층·전역 상태·포획 알림이다.
    #pool;
    #types;
    #currentTypes = null;
    #notice;
    #forms;
    #state;
    #captured;
    // 실제 World 의 공통 Pop·Form 해제·가상 재부착 대여이다.
    #pop = null;
    #unpop = null;
    #reattach = null;

    // 통지는 필수이고 TYPE 표는 판본 크기(188)로 복사한다. state 가 있으면 Form 해제에도 같은 진단을 빌려 준다(생성 시점의 장치).
    constructor(pool, types, notice, { transmit = null, state = null, captured = null } = {}) {
        if (pool?.edition !== 'patch1078') throw new TypeError('ContentForm 호스트는 10.78 SidPool 만 받습니다.');
        if (typeof notice !== 'function') throw new TypeError('ContentForm 부모 알림 경계 필요');
        if (!Array.isArray(types) || types.length !== 188) throw new RangeError('ContentForm 타입 판본/크기 오류');
        this.#pool = pool;
        this.#types = types.map((record) => ({ ...record }));
        this.#notice = notice;
        this.#state = state;
        this.#captured = captured;
        this.#forms = new SquidForms(pool, {
            notice: (parent, form) => this.#notice(parent, form, 2),
            transmit,
            unpop: state ? { assertion: state.assertion ?? null } : {},
        });
    }

    // 같은 풀인지 확인하기 위한 실제 풀이다.
    get pool() { return this.#pool; }

    // 선택한 10.78 진단만 요구하고 돌아오면 원문 본문을 계속한다.
    #assert(expression, line) {
        need(this.#state?.assertion, 'assertion', 'ContentForm')(expression, SQUID_CPP, line);
    }

    // 통지 뒤 현재 TYPE 의 genus 를 실제 소비 위치에서 읽는다(빌린 표가 있으면 그 표).
    #typeGenus(type) {
        const current = this.#currentTypes ?? this.#types;
        if (type >= current.length) throw new RangeError('Content 통지 뒤 현재 TYPE 물리 범위');
        return current[type].flags2 >>> 0;
    }

    // 현재 전역 상태 값(함수면 호출, 없으면 0)이다.
    #stateValue(name) {
        const value = this.#state?.[name];
        return typeof value === 'function' ? value() : (value ?? 0);
    }

    // 004aec10: 정렬 부착 뒤, 통지/전송이 바꾼 현재 내용물 TYPE·부모 genus·전투 상태를 다시 읽어 사제 포획 알림 조건을 평가한다.
    attach(form, parent, flags = 0) {
        const pool = this.#pool;
        const state = this.#state;
        const attached = attachSortedContent(pool, form, parent, flags, {
            contentType: () => (typeof state?.contentType === 'function' ? state.contentType() : 6),
            previous: (sid, table) => commonContainedPreviousAtTable(pool, sid, table),
            setPrevious: (sid, table, value) => commonContainedSetPreviousAtTable(pool, sid, table, value),
            setParent: (sid, table, value) => commonContainedSetParent(pool, sid, value),
            // 빌린 실제 Pop(없으면 같은 Form 층의 공통 Pop)이다.
            pop: (sid, value) => {
                const selected = this.#pop;
                if (selected) selected(sid, value);
                else this.#forms.pop(sid, value);
            },
            notice: this.#notice,
            assertion: state?.assertion ?? null,
        });
        if (!attached) return;
        if ((this.#typeGenus(word(pool.slot(form), CONTENT_TYPE)) & PRIEST_GENUS) && (this.#typeGenus(pool.slot(parent)[TYPE]) & CARRIER_GENUS) && state && this.#stateValue('inBattle')) {
            if (this.#stateValue('checkingLinks')) {
                if (form < 1 || form >= pool.capacity) this.#assert('isValid()', 0xc2b);
                if (this.#stateValue('checkingLinks') && !this.linked(form)) this.#assert('isRobust()', 0xc2c);
            }
            if (form >= 1 && form < pool.capacity && this.linked(form)) {
                if (typeof this.#captured !== 'function') throw new Error('ContentForm 전투 사제 포획 알림 경계 필요');
                this.#captured(parent, pool.slot(form)[PREVIOUS_OWNER] & 0x7f);
            }
        }
    }

    // 공통 Form Unpop 뒤 살아 있는 부모에 제거 2 를 알린다(빌린 해제가 있으면 그것). 고정 TYPE/free 선행 검사를 추가하지 않는다.
    unpop(form) {
        if (this.#unpop) this.#unpop(form);
        else this.#forms.unpop(form);
    }

    // 정렬 링크 뒤 실제 World 공통 Pop(깊이/진단)을 빌린다.
    bindPop(pop) {
        if (pop !== null && typeof pop !== 'function') throw new TypeError('Content Pop 은 함수여야 합니다.');
        this.#pop = pop;
    }

    // 실제 World 공장이 현재 번호/부모 알림을 고르는 Form 해제를 빌린다.
    bindUnpop(unpop) {
        if (unpop !== null && typeof unpop !== 'function') throw new TypeError('Content 해제는 함수여야 합니다.');
        this.#unpop = unpop;
    }

    // Regular 분리 뒤 현재 VT+50 을 고르는 재부착(form, parent, flags)을 빌린다.
    bindReattach(reattach) {
        if (reattach !== null && typeof reattach !== 'function') throw new TypeError('Content 재부착은 함수여야 합니다.');
        this.#reattach = reattach;
    }

    // 같은 World 의 현재 TYPE 표(188칸)를 복사 없이 빌린다. null 이면 생성 때 사본으로 돌아간다.
    bindCurrentTypes(types) {
        if (types !== null && (!Array.isArray(types) || types.length !== this.#types.length)) throw new RangeError('Content 현재 TYPE 표의 판본/길이 계약');
        this.#currentTypes = types;
    }

    // 가상 +0x60: WORLD(0x80)면 진단(정상 복귀 뒤 현재 flags 재조회) 뒤 0x34 비트를 본다. TYPE/상태 선행 검사는 하지 않는다.
    robust(form) {
        if (word(this.#pool.slot(form), LIST_FLAGS) & WORLD) {
            const assertion = this.#state?.assertion;
            need(assertion, 'assertion', 'ContentForm Robust')('!(getListFlags() & CFF_WORLD)', SQUID_CPP, 0xbb1);
        }
        return (word(this.#pool.slot(form), LIST_FLAGS) & ROBUST) !== 0;
    }

    // 004ae080: 현재 부모 getter 를 한 번 읽고 부모 체인 전체에 자기가 있는지 본다(예약 부모 0 도 물리 머리를 읽음).
    linked(form) {
        const pool = this.#pool;
        return checkRawFormLinked(pool, form, (current) => commonContainedParent(pool, current));
    }

    // 가상 +0x5c: damage 가 0/음수면 amount 를 그대로 돌려준다. 양수면 현재 부모 getter→Unpop→+21 의 bit0(0x100) 해제→현재 가상 재부착(+50, flags 0x10)→0 이다.
    damage(form, damage, amount) {
        if ((damage | 0) <= 0) return amount;
        const pool = this.#pool;
        const parent = commonContainedParent(pool, form);
        this.unpop(form);
        pool.physical(form)[LIST_FLAGS + 1] &= 0xfe;
        // Unpop 이 장치를 바꿀 수 있으므로 소비 위치에서 고른다.
        this.#attachCurrent(form, parent, 0x10);
        return 0;
    }

    // 선택한 마지막 가상 부착(+50)이다: 생성/통지 또는 Unpop 이 바꾼 현재 장치(bindReattach)를 호출 순간에 읽는다. 장치가 없으면 현재 VT 가 Content(6)일 때만
    //   기존 정렬 부착을 쓰고, 다른 미설치 가상 표는 앞선 생성/필드 쓰기 뒤에 거부한다(TYPE BYTE 를 VT 의 대용으로 쓰지 않는다).
    //   (2026-11-10 WB04-E259: cpppj ContentFormHost::AttachCurrent 3f766a97 재반영 — Regular 재부착과 contained 생성의 마지막 부착이 같은 선택을 쓴다)
    #attachCurrent(form, parent, flags) {
        const attach = this.#reattach;
        if (attach) { attach(form, parent, flags); return; }
        if (vtableOf(this.#pool.slot(form)) !== SQUID_FORM_VTABLES.content) throw new Error('ContentForm 현재 가상+50 부착 장치 미연결');
        this.attach(form, parent, flags);
    }

    // 기존 contained 생성기의 TYPE 6 생성(같은 공장)과 필드 쓰기/표면 통지 뒤 마지막 현재 가상+50 부착만 이 호스트로 연결한다.
    bindCreate(factory, hooks = {}) {
        const bound = makeContainedCreateHooks(this.#pool, factory, hooks);
        return { ...bound, attach: (form, parent, flags) => this.#attachCurrent(form, parent, flags) };
    }
}
