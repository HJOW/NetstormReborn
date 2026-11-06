// 원본 10.78 감시 쌍(DependForm, TYPE 1) Form 의 생성 부모(004b1510)와 조회·제거·통지·가상 삭제 호스트를 옮긴 모듈이다.
//  - createDependForms: 어느 부모든 client 번호(5..14999)면 두 Form 을 client 풀(flags 2)에, 아니면 서버 권한을 진단하고 flags 0 으로 만든다.
//    두 Create 는 각각 직전에 현재 Depend TYPE DWORD 를 읽고, 링크 WORD(+20 상대 watched·+18 watcher)를 원문 순서로 쓴 뒤 첫/둘째 Form 의 현재 VT 로 +74 부착을 부른다.
//  - DependFormHost: watched 에는 watchedForm, watcher 에는 watcherForm 이 붙는다. 조회(has)·제거(remove)는 부모 +6 체인에서 next 를 먼저 저장하며
//    현재 Depend TYPE 과 상대 부모가 맞는 Form 을 찾는다. 가상 preDestroy 는 watched→watcher 상대 Form 을 같은 삭제기로 지우고, release 는 watcher 부모에 통지한다.
//    클래스 판정은 바뀔 수 있는 TYPE 이 아니라 현재 VT(Depend 생성자 VT)로 한다.
// 근거: cpppj `o/RawDependCreate.cpp/.h`(886f2d23 계열), `o/DependFormHost.cpp/.h`(commit 시점 cpppj HEAD). 원본 관측: depend-create-parent-current-x86.tsv(10.78 810행),
//       formlifecycle-x86.tsv 의 Pair/Destroy/Notify/Has/Remove 행(tests/form-hosts.test.mjs).
// CD 10.72 경로는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-06 WB04-D5a: Form 호스트 이식으로 추가)
import { SquidForms } from './squid-forms.js';
import { SquidFactory } from './squid-factory.js';
import { SQUID_FORM_VTABLES } from './squid-vtables.js';
import { SQUID_DESTROY_EFFECT } from './squid-destroy.js';

// 원문 진단 파일 이름이다(10.78). (2026-11-06 WB04-D5a)
const SQUID_CPP = '\\Ns\\O\\Squid.cpp';
// 공통 링크·감시 쌍 필드 위치이다: next·머리·TYPE·상태·watcherForm(+18)·watchedForm(+20). (2026-11-06 WB04-D5a)
const NEXT = 4, HEAD = 6, TYPE = 10, STATE = 11, WATCHER = 18, WATCHED = 20;
// free/dead 상태 비트와 첫 서버 SID·첫 자산 TYPE 이다. (2026-11-06 WB04-D5a)
const FREE = 1, DEAD = 2, FIRST_SERVER_SID = 15000, FIRST_ASSET_TYPE = 70;
// 독립 입력의 원문 초기 Depend TYPE 이다(실제 세션은 GameAuxState.dependFormType 을 빌림). (2026-11-06 WB04-D5a)
const DEFAULT_DEPEND_TYPE = 1;
// +74 공통 부착 thunk 를 쓰는 Form VT(Depend·Gump·Content)이다. Process(2)는 별도 직렬화 접두라 아직 연결하지 않는다. (2026-11-06 WB04-D5a)
const COMMON_ATTACH_VTABLES = new Set([SQUID_FORM_VTABLES.depend, SQUID_FORM_VTABLES.gump, SQUID_FORM_VTABLES.content]);

// 선택된 장치를 꺼낸다. 없으면 그 위치에서 오류이다(앞선 할당/링크 쓰기는 남는다). (2026-11-06 WB04-D5a)
function need(device, name) {
    if (typeof device !== 'function') throw new Error(`Depend 생성의 선택된 실제 장치(${name})가 연결되지 않았습니다.`);
    return device;
}

// 슬롯 WORD/DWORD 읽기이다. (2026-11-06 WB04-D5a)
function word(raw, offset) {
    return raw[offset] | (raw[offset + 1] << 8);
}
function vtableOf(raw) {
    return (raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24)) >>> 0;
}

// 004b1510: 감시 Form 두 개의 생성/링크/가상 부착 순서만 실행한다(원문은 void). 반환 { watched, watcher } 는 두 번째/첫 번째 Create 의 객체 토큰이다.
// hooks: { server() → boolean, formType() → DWORD, create(type, flags) → 토큰, sid(토큰) → SID, word(토큰, offset, WORD), vtable(토큰) → DWORD,
//          attach(토큰, VT, 부모 DWORD, flags), assertion(expression, file, line) }. (2026-11-06 WB04-D5a)
export function createDependForms(watched, watcher, hooks = {}) {
    // 원문 signed 비교의 client 범위(예약 0~4 제외)이다.
    const client = (parent) => parent >= 5 && parent < FIRST_SERVER_SID;
    const flags = client(watched) || client(watcher) ? 2 : 0;
    if (!flags && !need(hooks.server, 'server')()) need(hooks.assertion, 'assertion')('server', SQUID_CPP, 0xb29);
    const firstType = need(hooks.formType, 'formType')() >>> 0;
    const first = need(hooks.create, 'create')(firstType, flags) >>> 0;
    const secondType = need(hooks.formType, 'formType')() >>> 0;
    const second = need(hooks.create, 'create')(secondType, flags) >>> 0;
    // 두 번째 객체 번호는 링크 쓰기보다 먼저 계산한다.
    const secondSid = need(hooks.sid, 'sid')(second) & 0xffff;
    const put = need(hooks.word, 'word');
    put(first, WATCHED, secondSid);
    put(first, WATCHER, 0);
    put(second, WATCHER, need(hooks.sid, 'sid')(first) & 0xffff);
    put(second, WATCHED, 0);
    const firstTable = need(hooks.vtable, 'vtable')(first) >>> 0;
    // 첫 부착이 두 번째 VT/필드를 바꿀 수 있어 둘째 VT 는 그 뒤에 읽는다.
    need(hooks.attach, 'attach')(first, firstTable, watcher, 0);
    const secondTable = need(hooks.vtable, 'vtable')(second) >>> 0;
    need(hooks.attach, 'attach')(second, secondTable, watched, 0);
    return { watched: second, watcher: first };
}

// 감시 쌍 호스트이다. options: { notice(watcher, watched, form) — watched 부모 삭제 때 watcher 부모 +0x34 통지, fallback: 다른 Form/자산 삭제 훅({emit, selected, unpopForm}),
// transmit(form, flags) — Pop 서버 전송 }. 사용: const host = new DependFormHost(pool, destroy, { notice }); host.bindCreation(factory, () => aux.dependFormType); host.create(watched, watcher). (2026-11-06 WB04-D5a)
export class DependFormHost {
    // 같은 풀·삭제기·통지·fallback 훅·전송·Form 층이다.
    #pool;
    #destroy;
    #notice;
    #fallback;
    #transmit;
    #forms;
    // 빌린 Form 해제·생성 공장·현재 Depend TYPE getter·생성 진단이다.
    #unpop = null;
    #factory = null;
    #dependType = null;
    #creationAssertion = null;

    // 다른 풀의 삭제기를 거부한다.
    constructor(pool, destroy, { notice = null, fallback = {}, transmit = null } = {}) {
        if (pool?.edition !== 'patch1078' || destroy?.pool !== pool) throw new TypeError('DependForm 삭제 SID 풀 불일치');
        this.#pool = pool;
        this.#destroy = destroy;
        this.#notice = notice;
        this.#fallback = fallback ?? {};
        this.#transmit = transmit;
        this.#forms = new SquidForms(pool, { transmit });
    }

    // 같은 풀인지 확인하기 위한 실제 풀이다.
    get pool() { return this.#pool; }

    // 실제 World 공장의 현재 진단/종속 분리를 쓰는 Form 해제를 빌린다(null 이면 독립 기본 해제).
    bindUnpop(unpop) {
        if (unpop !== null && typeof unpop !== 'function') throw new TypeError('DependForm 해제는 함수여야 합니다.');
        this.#unpop = unpop;
    }

    // 같은 풀의 공장·현재 Depend TYPE getter(예: () => aux.dependFormType)·생성 진단을 빌린다.
    bindCreation(factory, dependType, assertion = null) {
        if (factory?.pool !== this.#pool) throw new TypeError('Depend 생성 공장의 실제 SID 풀 불일치');
        if (typeof dependType !== 'function') throw new TypeError('Depend TYPE 은 현재 DWORD getter 여야 합니다.');
        this.#factory = factory;
        this.#dependType = dependType;
        this.#creationAssertion = assertion;
    }

    // 검색/금지 비교와 각 Create 가 읽는 현재 전체 Depend TYPE 이다.
    #currentDependType() { return this.#dependType ? this.#dependType() >>> 0 : DEFAULT_DEPEND_TYPE; }

    // 현재 VT 가 감시 클래스인지 본다(예약/범위 밖은 false, free 여부는 호출자가 따로 본다).
    isDependForm(form) {
        const pool = this.#pool;
        if (!Number.isInteger(form) || form < 5 || form >= pool.capacity || form === pool.layout.predictableFirst) return false;
        return vtableOf(pool.slot(form)) === SQUID_FORM_VTABLES.depend;
    }

    // 살아 있거나 삭제 중인 감시 Form 만 받는다(free 는 거부).
    #form(form) {
        if (!this.isDependForm(form) || (this.#pool.slot(form)[STATE] & FREE)) throw new Error('DependForm 슬롯 상태 오류');
        this.#forms.parent(form);
    }

    // 할당된 슬롯의 WORD 만 쓴다.
    #put(sid, offset, value) {
        const raw = this.#pool.allocated(sid);
        raw[offset] = value & 0xff;
        raw[offset + 1] = (value >>> 8) & 0xff;
    }

    // remove 의 부모 확인: 두 부모의 예약/free 를 거부하고 같은 객체·watcher 가 Depend TYPE·watched 가 Form 이면 거부한다.
    #parents(watcher, watched) {
        const pool = this.#pool;
        // 두 부모 번호를 같은 규칙으로 확인한다.
        for (const parent of [watcher, watched]) {
            if (parent < 5 || parent >= pool.capacity || parent === pool.layout.predictableFirst || (pool.slot(parent)[STATE] & FREE)) throw new Error('DependForm 부모 상태 오류');
        }
        if (watcher === watched || pool.slot(watcher)[TYPE] === this.#currentDependType() || pool.slot(watched)[TYPE] < FIRST_ASSET_TYPE) throw new Error('DependForm 감시 대상 타입 오류');
    }

    // 004b1510: watcher 쪽을 먼저 만들고 부착한다. 공장이 없으면 삭제기의 TYPE 표로 독립 공장을 만든다(고정 VT 경로를 추가하지 않음).
    create(watched, watcher) {
        const pool = this.#pool;
        // 링크/부착이 실제 소비하는 위치에서만 물리 범위를 확인한다.
        const object = (number) => {
            if (number >= pool.capacity) throw new RangeError('Depend 생성의 물리 객체 범위');
            return number;
        };
        let factory = this.#factory;
        if (!factory) {
            factory = new SquidFactory(pool, this.#destroy.types);
            factory.bindDiagnostics({ types: this.#destroy.types, assertion: this.#creationAssertion });
        }
        const pair = createDependForms(watched, watcher, {
            server: () => pool.isServer,
            formType: () => this.#currentDependType(),
            assertion: this.#creationAssertion,
            create: (type, flags) => factory.create(type, flags),
            sid: (token) => token,
            word: (token, offset, value) => {
                const raw = pool.physical(object(token));
                raw[offset] = value & 0xff;
                raw[offset + 1] = (value >>> 8) & 0xff;
            },
            vtable: (token) => vtableOf(pool.physical(object(token))),
            // Depend/Gump/Content 는 실제 공통 thunk 이며 Process 의 직렬화 접두는 미연결이다.
            attach: (token, table, parent, flags) => {
                if (!COMMON_ATTACH_VTABLES.has(table)) throw new Error('Depend 생성의 선택된 실제 VT+74 미연결');
                factory.attachForm(object(token), object(parent), flags, this.#transmit);
            },
        });
        return { watchedForm: object(pair.watched), watcherForm: object(pair.watcher) };
    }

    // +18 watcherForm 의 Form 부모(상대 watcher 부모)를 읽는다. +18 이 0 이면 0 이다.
    watcher(form) {
        this.#form(form);
        const partner = word(this.#pool.slot(form), WATCHER);
        return partner ? this.#forms.parent(partner) : 0;
    }

    // watched 의 체인에서 현재 Depend TYPE 이고 상대 부모가 watcher 인 첫 Form 이 있는지 본다(free/dead/void 필터 없음).
    has(watched, watcher) {
        const pool = this.#pool;
        if (!watched || watched >= pool.capacity) throw new RangeError('DependForm 조회 부모 번호 오류');
        const visited = new Set();
        let current = word(pool.slot(watched), HEAD);
        // 후보를 판정하기 전에 next 를 저장한다.
        while (current) {
            if (current >= pool.capacity || visited.has(current)) throw new Error('DependForm 조회 체인 손상');
            visited.add(current);
            const next = word(pool.slot(current), NEXT);
            if (pool.slot(current)[TYPE] === this.#currentDependType() && this.watcher(current) === watcher) return true;
            current = next;
        }
        return false;
    }

    // watched 가 dead 면 아무것도 하지 않고, 아니면 맞는 모든 감시 Form 을 같은 삭제기로 지운다(삭제가 슬롯을 반납해도 저장한 next 로 계속).
    remove(watcher, watched, flags = 0) {
        const pool = this.#pool;
        if (!watched || watched >= pool.capacity) throw new RangeError('DependForm 제거 부모 번호 오류');
        if (pool.slot(watched)[STATE] & DEAD) return;
        this.#parents(watcher, watched);
        const visited = new Set();
        let current = word(pool.slot(watched), HEAD);
        // next 를 먼저 저장하고 맞는 Form 을 지운다.
        while (current) {
            if (current >= pool.capacity || visited.has(current)) throw new Error('DependForm 제거 체인 손상');
            visited.add(current);
            const next = word(pool.slot(current), NEXT);
            if (pool.slot(current)[TYPE] === this.#currentDependType() && this.watcher(current) === watcher) this.destroy(current, flags);
            current = next;
        }
    }

    // 가상 +0x1c: 상대가 client 번호이거나 서버 권한이면 watcher 부모에 통지하고, 통지 뒤 watchedFid(+20)가 0 인지 다시 확인한다.
    notify(form, watched) {
        this.#form(form);
        const partner = word(this.#pool.slot(form), WATCHER);
        if (!partner) return;
        const parent = this.#forms.parent(partner);
        if (partner < FIRST_SERVER_SID || this.#pool.isServer) {
            if (typeof this.#notice !== 'function') throw new Error('DependForm watcher 통지 경계 필요');
            this.#notice(parent, watched, form);
        }
        if (word(this.#pool.slot(form), WATCHED)) throw new Error('DependForm 통지 뒤 watchedFid 오류');
    }

    // 가상 +0x10: 공통 삭제(상대 먼저 삭제→자신 Unpop→SID 반납)를 같은 훅으로 실행한다.
    destroy(form, flags = 0) {
        if (!this.isDependForm(form)) throw new Error('DependForm 삭제 타입 오류');
        this.#destroy.destroy(form, flags, this.hooks());
    }

    // 감시 클래스만 처리하고 다른 Form/자산은 fallback 훅으로 넘기는 삭제 훅이다(전체 월드 분배기로도 쓴다).
    hooks() {
        const fallback = this.#fallback;
        return {
            selected: typeof fallback.selected === 'function' ? fallback.selected : () => 0,
            // 감시 Form 은 빌린 해제(없으면 독립 기본 해제), 그 밖은 fallback 이다.
            unpopForm: (sid) => {
                if (this.isDependForm(sid)) {
                    if (this.#unpop) this.#unpop(sid);
                    else this.#forms.unpop(sid);
                    return;
                }
                if (typeof fallback.unpopForm !== 'function') throw new Error('DependForm 외 Unpop 경계 필요');
                fallback.unpopForm(sid);
            },
            // preDestroy 는 watched→현재 watcher 상대를 지우고 공통 깊이를 반납한다.
            emit: (event) => {
                if (this.isDependForm(event.sid)) {
                    const raw = this.#pool.slot(event.sid);
                    switch (event.effect) {
                    case SQUID_DESTROY_EFFECT.preDestroy: {
                        const watchedForm = word(raw, WATCHED);
                        if (watchedForm) this.destroy(watchedForm, event.flags);
                        const watcherForm = word(this.#pool.slot(event.sid), WATCHER);
                        if (watcherForm) this.destroy(watcherForm, event.flags);
                        this.#destroy.completePreDestroy();
                        return;
                    }
                    case SQUID_DESTROY_EFFECT.postDestroy:
                        this.#destroy.completePostDestroy();
                        return;
                    case SQUID_DESTROY_EFFECT.releaseDependent:
                        this.notify(event.sid, event.parent);
                        return;
                    case SQUID_DESTROY_EFFECT.destroyDependent:
                        this.destroy(event.sid, event.flags);
                        return;
                    default:
                        break;
                    }
                }
                if (typeof fallback.emit !== 'function') throw new Error('DependForm 외 삭제 효과 경계 필요');
                fallback.emit(event);
            },
        };
    }
}
