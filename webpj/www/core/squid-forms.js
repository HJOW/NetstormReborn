// 원본 10.78 Squid.cpp 의 내장 Form 네 종(Depend 1·Process 2·Gump 3·Content 6) 공통 층을 옮긴 모듈이다.
//  - 생성자: 지정 물리 슬롯에 생성자 가상 표(VT) DWORD 하나만 쓴다(constructVtableAt). create 는 SID 할당 뒤 VT→TYPE 만 쓴다(004af530, 빈 postCreate).
//  - take: 지정 서버/예측 슬롯을 수신한다(004af610). 사용 중이면 같은 TYPE 의 Form Unpop 을 먼저 하고 payload·풀 목록/카운터는 보존한다.
//  - attachCommonForm: 공통 부착 부모(004ac800). 현재 TYPE genus 진단→현재 VT 부모 setter→부모 머리 갱신과 이웃/자신의 이전 setter→contained 비트→직접 Pop(flags|0x100)→꼬리 진단 순서다.
//  - popCommonForm: 공통 Pop 부모(004ac560). 진단 복귀→void 해제→권한별 flags 정규화와 서버 전송(xmit 비트)→공유 postPop 깊이와 현재 VT+20 순서다.
//  - SquidForms.unpop: Form 공통 Unpop(004ae140). 세 진단→void 쓰기→현재 VT 부모 getter→부모 free 관문→공통 종속 분리 순서다.
// 진단은 정상 복귀할 수 있고, 돌아오면 그 시점의 현재 바이트를 다시 읽어 계속한다. 선택된 장치가 없으면 그 위치에서 오류이며 앞선 부분 쓰기는 되돌리지 않는다.
// 근거: cpppj `o/SquidForms.cpp/.h`(09ad72b0), `o/RawFormAttach.cpp/.h`(d678fcad), `o/RawFormPop.cpp/.h`(09ad72b0), `o/RawSquidContainedUnpop.cpp`(공통 링크 getter/setter).
// 원본 관측: cpppj/tests/fixtures 의 form-constructor·form-attach-current·form-pop-current·form-unpop-current·formtake 10.78 행(tests/squid-forms.test.mjs).
// CD 10.72 경로는 범위 밖이다. Form 호스트(Depend/Gump/Content/Process)와 SquidFactory 의 Form 진입은 각 후속 단위에서 이 층을 빌린다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-06 WB04-D3: 공통 Form 층 이식으로 추가)
import { SQUID_FORM_VTABLES } from './squid-vtables.js';
import {
    RawSquidContainedUnpop, commonContainedParent, commonContainedPrevious, commonContainedSetParent, commonContainedSetPrevious,
} from './squid-contained-unpop.js';

// 원문 진단 파일 이름이다(10.78). 공통 부착의 자기 머리 진단만 헤더 파일 이름을 쓴다. (2026-11-06 WB04-D3)
const SQUID_CPP = '\\Ns\\O\\Squid.cpp';
const SQUID_H = 'c:\\ns\\o\\squid.h';

// 네 내장 Form 타입 번호와 생성자 가상 표이다. 타입 번호는 고정 생성자 선택에만 쓰며 기존 Form 의 클래스는 현재 VT 로 판정한다. (2026-11-06 WB04-D3)
export const SQUID_FORM_TYPES = Object.freeze([1, 2, 3, 6]);
export const SQUID_FORM_CONSTRUCTOR_VTABLES = Object.freeze({
    1: SQUID_FORM_VTABLES.depend, 2: SQUID_FORM_VTABLES.process, 3: SQUID_FORM_VTABLES.gump, 6: SQUID_FORM_VTABLES.content,
});

// Form 공통 필드 위치이다: 종속 next·종속 머리·TYPE·상태·이전(+14)·부모(+16). contained 자산은 이전/부모 위치가 반대다. (2026-11-06 WB04-D3)
const NEXT = 4, HEAD = 6, TYPE = 10, STATE = 11, PREVIOUS = 14, PARENT = 16;
// 상태 BYTE 의 free·dead·void·contained·전송 직후(xmitJustCreated) 비트이다. (2026-11-06 WB04-D3)
const FREE = 1, DEAD = 2, VOID = 4, CONTAINED = 8, XMIT_JUST_CREATED = 0x10;
// Form 으로 인정하는 마지막 TYPE 과 일반 할당의 첫 번호이다. (2026-11-06 WB04-D3)
const LAST_FORM_TYPE = 0x45, FIRST_SID = 5;
// 공통 부착이 금지하는 genus(gEMPLACEMENT|gVORTEX|gFACTORY) 비트이다. (2026-11-06 WB04-D3)
const ATTACH_FORBIDDEN_GENUS = 0x44200;
// 서버 Pop 의 client 번호 끝(10.78)이다. 예약 0~4 는 client 범위에 넣지 않는다. (2026-11-06 WB04-D3)
const CLIENT_POP_END = 15000;
// 독립 입력의 기본 종속 분리가 쓰는 원문 초기 전역값(contentFormType 6·gumpFormType 3·사건 0)이다. 실제 세션은 bindUnpop 으로 현재 전역 종단을 빌린다. (2026-11-06 WB04-D3)
const INDEPENDENT_GLOBALS = Object.freeze({ contentFormType: () => 6, gumpFormType: () => 3, uiEvent: () => 0 });

// 네 Form VT 집합이다(현재 클래스 판정용). (2026-11-06 WB04-D3)
const FORM_TABLES = new Set(Object.values(SQUID_FORM_CONSTRUCTOR_VTABLES));

// 부호 없는 DWORD 정수만 받는다(flags 등). (2026-11-06 WB04-D3)
function uint32(value, name) {
    if (!Number.isInteger(value) || value < 0 || value > 0xffffffff) throw new RangeError(`${name} 는 부호 없는 DWORD 여야 합니다.`);
    return value;
}

// 슬롯 WORD 를 읽는다(정렬 무관). (2026-11-06 WB04-D3)
function word(raw, offset) {
    return raw[offset] | (raw[offset + 1] << 8);
}

// 슬롯 WORD 만 쓴다(이웃 payload 보존). (2026-11-06 WB04-D3)
function putWord(raw, offset, value) {
    raw[offset] = value & 0xff;
    raw[offset + 1] = (value >>> 8) & 0xff;
}

// 슬롯 +0 의 현재 가상 표 DWORD 를 읽는다. 각 가상 호출 직전에 다시 읽는다. (2026-11-06 WB04-D3)
function vtableOf(raw) {
    return (raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24)) >>> 0;
}

// 슬롯 +0 에 가상 표 DWORD 하나만 쓴다(생성자 저장). (2026-11-06 WB04-D3)
function putVtable(raw, value) {
    // 낮은 바이트부터 네 바이트만 쓴다.
    for (let index = 0; index < 4; index += 1) raw[index] = (value >>> (index * 8)) & 0xff;
}

// 선택된 장치를 꺼낸다. 없으면 그 위치에서 오류이며 앞선 부분 쓰기는 남는다. (2026-11-06 WB04-D3)
function need(device, name, where) {
    if (typeof device !== 'function') throw new Error(`${where}의 선택된 실제 장치(${name})가 연결되지 않았습니다.`);
    return device;
}

// 10.78 SID 풀인지 확인한다(다른 판본·다른 객체를 첫 쓰기 전에 거부). (2026-11-06 WB04-D3)
function checkPool(pool) {
    if (pool?.edition !== 'patch1078' || typeof pool.physical !== 'function') throw new TypeError('공통 Form 층은 10.78 SidPool 만 받습니다.');
}

// 내장 Form 타입의 생성자 가상 표를 돌려준다. 네 타입 밖은 자산 Factory 의 책임이라 거부한다. (2026-11-06 WB04-D3)
export function formConstructorVtable(type) {
    const table = Number.isInteger(type) ? SQUID_FORM_CONSTRUCTOR_VTABLES[type] : undefined;
    if (table === undefined) throw new RangeError('미복원 내장 Form 생성자 타입');
    return table;
}

// 현재 VT 가 네 Form 생성자 가상 표 중 하나인지 확인한다. (2026-11-06 WB04-D3)
export function isFormConstructorVtable(table) {
    return FORM_TABLES.has(table >>> 0);
}

// 공통 Form 부착 부모(004ac800, 10.78)이다. 선행 상태 검사나 실패 되돌리기를 추가하지 않는다.
// hooks: { genus(typeByte) → flags2, setParent(sid, vtable, parent), setPrevious(sid, vtable, previous), pop(sid, flags), contentType() → DWORD, assertion(expression, file, line) }.
// 패치판은 parent 0 이면 아무것도 하지 않는다. 각 가상 호출은 호출 직전의 현재 VT 를 받는다. (2026-11-06 WB04-D3)
export function attachCommonForm(pool, form, parent, flags, hooks = {}) {
    checkPool(pool);
    uint32(flags, '공통 부착 flags');
    const where = '공통 Form 부착';
    // 원문 Squid.cpp 진단이다(정상 복귀하면 다음 명령을 계속).
    const assertion = (expression, line, file = SQUID_CPP) => need(hooks.assertion, 'assertion', where)(expression, file, line);
    if (!parent) return;
    const raw = pool.physical(form);
    if ((need(hooks.genus, 'genus', where)(raw[TYPE]) & ATTACH_FORBIDDEN_GENUS) !== 0) {
        assertion('!(getGenus() & (gEMPLACEMENT|gVORTEX |gFACTORY))', 0x3d0);
    }
    need(hooks.setParent, 'setParent', where)(form, vtableOf(raw), parent);
    // 부모 머리는 부모 setter 뒤에 한 번 읽어 이후 쓰기에 그대로 쓴다.
    const parentRaw = pool.physical(parent);
    const head = word(parentRaw, HEAD);
    if (head === form) assertion('n != getSid()', 0x123, SQUID_H);
    putWord(raw, NEXT, head);
    if (head) need(hooks.setPrevious, 'setPrevious', where)(head, vtableOf(pool.slot(head)), form);
    putWord(parentRaw, HEAD, form);
    need(hooks.setPrevious, 'setPrevious', where)(form, vtableOf(raw), 0);
    raw[STATE] |= CONTAINED;
    need(hooks.pop, 'pop', where)(form, (flags | 0x100) >>> 0);
    // Pop 뒤 현재 TYPE BYTE 를 현재 전체 contentFormType DWORD 와 비교한다.
    const typeAfterPop = raw[TYPE];
    if (typeAfterPop === (need(hooks.contentType, 'contentType', where)() >>> 0)) assertion('getType()!=contentFormType', 0x3e0);
    if (raw[TYPE] > LAST_FORM_TYPE) assertion('isForm()', 0x3e3);
}

// 공통 Form Pop 부모(004ac560, 10.78)이다. 원문에 없는 선행 검사를 추가하지 않는다.
// hooks: { server() → boolean, transmit(sid, x, y, z, flags), depth() → DWORD, setDepth(value), postPop(sid, vtable, flags), assertion(expression, file, line) }.
// 서버 전송은 고정 -1/-1/0 과 정규화 flags 를 전달하며, 전송 동안만 xmitJustCreated 비트를 켠다(flags 하위 3비트가 있을 때). (2026-11-06 WB04-D3)
export function popCommonForm(pool, form, flags, hooks = {}) {
    checkPool(pool);
    uint32(flags, '공통 Pop flags');
    const where = '공통 Form Pop';
    // 원문 Squid.cpp 진단이다(정상 복귀하면 현재값을 다시 읽어 계속).
    const assertion = (expression, line) => need(hooks.assertion, 'assertion', where)(expression, SQUID_CPP, line);
    if (!form || form >= pool.capacity) assertion('isValid()', 0x35b);
    const raw = pool.physical(form);
    if (!(raw[STATE] & VOID)) assertion('isVoid()', 0x35c);
    const state = raw[STATE];
    raw[STATE] = state & ~VOID & 0xff;
    if (state & XMIT_JUST_CREATED) assertion('!xmitJustCreatedFlag', 0x35f);
    let current = flags;
    if (!need(hooks.server, 'server', where)()) {
        if (!(current & 8)) current = (current | 0x10) >>> 0;
    } else {
        if (form < FIRST_SID || form >= CLIENT_POP_END) {
            if (!(current & 0x10)) current = (current | 8) >>> 0;
        } else {
            if (current & 8) assertion('!(popFlags & pfSERVER_POP)', 0x364);
            current = (current | 0x10) >>> 0;
        }
        if (!(current & 0x50)) {
            if (current & 7) raw[STATE] |= XMIT_JUST_CREATED;
            need(hooks.transmit, 'transmit', where)(form, 0xffffffff, 0xffffffff, 0, current);
            raw[STATE] &= ~XMIT_JUST_CREATED & 0xff;
        }
    }
    const base = need(hooks.depth, 'depth', where)() >>> 0;
    need(hooks.setDepth, 'setDepth', where)((base + 1) >>> 0);
    need(hooks.postPop, 'postPop', where)(form, vtableOf(raw), current);
    if (base !== (need(hooks.depth, 'depth', where)() >>> 0)) assertion('curPostPop == basePostPop', 0x392);
}

// GameAuxState 의 현재 Form 전역(00541088 contentFormType·0054107c gumpFormType·005427dc 내용물 변경 사건)을 호출 시점마다 다시 읽는 getter 묶음으로 빌려 준다.
// 사용: new RawSquidContainedUnpop(pool, squidFormGlobals(aux), hooks) 또는 forms.bindAttach({ contentType: squidFormGlobals(aux).contentFormType, ... }).
// 사본을 만들지 않으므로 aux 의 이후 쓰기가 다음 호출에 그대로 보인다. (2026-11-06 WB04-D3)
export function squidFormGlobals(aux) {
    // 세 전역이 모두 정수 DWORD 로 있는 소유자만 받는다(사본·기본값으로 대신하지 않는다).
    for (const name of ['contentFormType', 'gumpFormType', 'contentUnpopEvent']) {
        if (!Number.isInteger(aux?.[name])) throw new TypeError(`Form 전역 소유자에 ${name} DWORD 가 없습니다(GameAuxState 를 넘겨야 합니다).`);
    }
    return Object.freeze({
        contentFormType: () => aux.contentFormType,
        gumpFormType: () => aux.gumpFormType,
        uiEvent: () => aux.contentUnpopEvent,
    });
}

// 네 내장 Form 의 생성자·Create·Take·공통 부착/Pop·Form Unpop 을 같은 SID 풀에서 처리한다(cpppj SquidForms).
// options: { notice(parent, form) — 기본 종속 분리가 내용물 Form 을 살아 있는 부모에서 뺀 뒤 부르는 부모 +0x24(이유 2),
//            transmit(form, flags) — 기본 Pop 의 서버 전송, unpop: { parent(form, vtable), detach(form), assertion(expression, file, line) } }.
// 기본 장치는 독립 입력용이다: genus 0·contentType 6·현재 VT 링크 setter·지역 postPop 깊이·초기 전역 6/3/0 의 종속 분리.
// 실제 세션은 bindAttach/bindPop/bindUnpop 으로 현재 TYPE 표·GameAuxState·공유 깊이·종속 분리를 빌린다. (2026-11-06 WB04-D3)
export class SquidForms {
    // 같은 실제 풀이다.
    #pool;
    // 내용물 Form 부모 제거 통지와 Pop 서버 전송 장치이다.
    #notice;
    #transmit;
    // 공통 Pop·부착·Unpop 에 빌린 현재 장치 묶음이다.
    #pop = {};
    #attach = {};
    #unpop = {};
    // 공유 깊이를 빌리지 않는 독립 입력의 postPop 깊이 DWORD 이다.
    #localPostPopDepth = 0;

    // 생성 때는 풀을 바꾸거나 선택되지 않은 장치를 요구하지 않는다.
    constructor(pool, { notice = null, transmit = null, unpop = {} } = {}) {
        checkPool(pool);
        if (notice !== null && typeof notice !== 'function') throw new TypeError('Form 부모 제거 통지는 함수여야 합니다.');
        if (transmit !== null && typeof transmit !== 'function') throw new TypeError('Form Pop 전송은 함수여야 합니다.');
        this.#pool = pool;
        this.#notice = notice;
        this.#transmit = transmit;
        this.#unpop = { ...(unpop ?? {}) };
    }

    // 다른 모듈과 같은 풀인지 확인하기 위한 실제 풀이다.
    get pool() { return this.#pool; }

    // 다음 Pop 부터 현재 권한·공유 깊이·진단·전송·postPop 장치를 빌린다(없는 칸은 기본 장치).
    bindPop(hooks = {}) { this.#pop = { ...(hooks ?? {}) }; }

    // 다음 부착부터 현재 genus·contentType·진단·setter·pop 장치를 빌린다(없는 칸은 기본 장치).
    bindAttach(hooks = {}) { this.#attach = { ...(hooks ?? {}) }; }

    // 다음 Unpop 부터 현재 부모 getter·종속 분리·진단을 빌린다.
    bindUnpop(hooks = {}) { this.#unpop = { ...(hooks ?? {}) }; }

    // 내장 Form 타입의 생성자 가상 표이다.
    vtable(type) { return formConstructorVtable(type); }

    // 원문 생성자(cdecl 1/2/3/6)의 단일 DWORD 저장만 한다. 할당·TYPE·다른 바이트는 쓰지 않으며 SID 0/free/dead 도 물리 범위 안이면 쓴다.
    constructVtableAt(type, sid) {
        const table = formConstructorVtable(type);
        putVtable(this.#pool.physical(sid), table);
    }

    // 004af530: 타입 확인 → SID 할당 → VT → TYPE 순서다. 타입이 틀리면 할당 전에 거부한다.
    create(type, flags = 0) {
        const table = formConstructorVtable(type);
        const sid = this.#pool.allocate(uint32(flags, 'Form Create flags'));
        const raw = this.#pool.physical(sid);
        putVtable(raw, table);
        raw[TYPE] = type;
        return sid;
    }

    // 004af610: 서버 영역 SID 만 받는다. 사용 중이면 같은 TYPE 의 Form Unpop 을 먼저 하고, 통지가 바꾼 현재 상태에서 free/dead 만 끈 뒤 VT→TYPE→void 를 쓴다.
    take(type, sid) {
        const table = formConstructorVtable(type);
        const pool = this.#pool;
        if (!Number.isInteger(sid) || sid < pool.layout.serverFirst) throw new RangeError('Form Take 는 서버 영역 SID 만 받습니다.');
        if (!(pool.slot(sid)[STATE] & FREE)) {
            if (pool.slot(sid)[TYPE] !== type) throw new Error('Form Take 타입 불일치');
            this.unpop(sid);
        }
        const raw = pool.physical(sid);
        raw[STATE] &= ~(FREE | DEAD) & 0xff;
        putVtable(raw, table);
        raw[TYPE] = type;
        raw[STATE] |= VOID;
        return sid;
    }

    // 공통 부착 부모를 빌린 장치(없으면 기본 장치)로 실행한다. 정렬 Content 부착 API 와는 별개다.
    attach(form, parent, flags = 0) {
        const pool = this.#pool;
        const hooks = { ...this.#attach };
        if (typeof hooks.genus !== 'function') hooks.genus = () => 0;
        if (typeof hooks.contentType !== 'function') hooks.contentType = () => 6;
        if (typeof hooks.setParent !== 'function') hooks.setParent = (sid, table, value) => commonContainedSetParent(pool, sid, value);
        if (typeof hooks.setPrevious !== 'function') hooks.setPrevious = (sid, table, value) => commonContainedSetPrevious(pool, sid, value);
        if (typeof hooks.pop !== 'function') hooks.pop = (sid, value) => this.pop(sid, value);
        attachCommonForm(pool, form, parent, flags, hooks);
    }

    // 공통 Pop 부모를 빌린 장치(없으면 현재 풀 권한·지역 깊이·Unpop 진단·전송 경계·네 Form 빈 postPop)로 실행한다.
    pop(form, flags) {
        const pool = this.#pool;
        const hooks = { ...this.#pop };
        if (typeof hooks.server !== 'function') hooks.server = () => pool.isServer;
        if (typeof hooks.depth !== 'function') hooks.depth = () => this.#localPostPopDepth;
        if (typeof hooks.setDepth !== 'function') hooks.setDepth = (value) => { this.#localPostPopDepth = value >>> 0; };
        if (typeof hooks.assertion !== 'function') hooks.assertion = this.#unpop.assertion;
        if (typeof hooks.transmit !== 'function') {
            hooks.transmit = (sid, x, y, z, value) => {
                if (!this.#transmit) throw new Error('현재 공통 Form Pop 전송이 연결되지 않았습니다.');
                this.#transmit(sid, value);
            };
        }
        if (typeof hooks.postPop !== 'function') {
            // 네 Form 의 빈 후처리(004ac1d0)는 같은 공유 깊이만 하나 줄인다. 다른 VT 는 미연결 오류다.
            hooks.postPop = (sid, table) => {
                if (!FORM_TABLES.has(table)) throw new Error('공통 Form Pop 의 선택된 현재 VT+20 이 연결되지 않았습니다.');
                hooks.setDepth((hooks.depth() - 1) >>> 0);
            };
        }
        popCommonForm(pool, form, flags, hooks);
    }

    // 004ae140: 세 진단 복귀→void 설정→현재 VT 부모 getter→부모 free 관문→종속 분리 순서다. 타입으로 VT 를 추정하지 않는다.
    // detach 가 연결되지 않으면 독립 입력의 공통 종속 분리(초기 전역 6/3/0, 생성 때 받은 notice)를 쓴다.
    unpop(form) {
        const pool = this.#pool;
        const hooks = this.#unpop;
        // 원문 Squid.cpp 진단이다(정상 복귀하면 현재 바이트로 계속).
        const assertion = (expression, line) => need(hooks.assertion, 'assertion', 'Form Unpop')(expression, SQUID_CPP, line);
        const raw = pool.physical(form);
        if (raw[TYPE] > LAST_FORM_TYPE) assertion('isForm()', 0xa59);
        if (!form || form >= pool.capacity) assertion('isValid()', 0xa5a);
        if (raw[STATE] & VOID) assertion('!isVoid()', 0xa5b);
        raw[STATE] |= VOID;
        const table = vtableOf(raw);
        const parent = typeof hooks.parent === 'function' ? hooks.parent(form, table) : commonContainedParent(pool, form);
        if (pool.slot(parent)[STATE] & FREE) return;
        if (typeof hooks.detach === 'function') {
            hooks.detach(form);
            return;
        }
        new RawSquidContainedUnpop(pool, INDEPENDENT_GLOBALS, {
            parent: (sid) => commonContainedParent(pool, sid),
            previous: (sid) => commonContainedPrevious(pool, sid),
            setPrevious: (sid, previous) => commonContainedSetPrevious(pool, sid, previous),
            // 선택된 통지가 없으면 앞선 링크 쓰기를 남긴 채 오류이다.
            parentNotice: (owner, sid) => {
                if (!this.#notice) throw new Error('ContentForm 부모 제거 통지가 연결되지 않았습니다.');
                this.#notice(owner, sid);
            },
            assertion: hooks.assertion,
        }).detach(form);
    }

    // Form +16 부모 WORD 를 읽는다. 예약/free 슬롯이나 Form 이 아닌 현재 VT 는 거부한다(dead 는 공통 삭제 중이라 허용).
    parent(form) { return word(this.#form(form), PARENT); }

    // Form +14 이전 WORD 를 읽는다(같은 확인).
    previous(form) { return word(this.#form(form), PREVIOUS); }

    // 기존 Form 의 현재 VT 클래스를 확인한다. 변경 가능한 TYPE 번호로 VT 를 추정하지 않는다.
    #form(sid) {
        const raw = this.#pool.slot(sid);
        if (sid < FIRST_SID || (raw[STATE] & FREE)) throw new Error('Form 슬롯 상태 오류');
        if (!FORM_TABLES.has(vtableOf(raw))) throw new Error('Form 가상 표 미복원');
        return raw;
    }
}
