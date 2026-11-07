// 원본 10.78 Squid.cpp 의 객체 생성 공장(SquidFactory)을 옮긴 모듈이다. 같은 SID 풀에서 슬롯 할당·생성자 쓰기·가상 초기화·지정 SID 수신(Take)을 한다.
//  - createRawSquid(004af530): fakeSurface 진단→할당→signed SID 진단→현재 TYPE 생성자→반환 객체 VT 저장→TYPE BYTE→저장한 VT 의 +28(postCreate) 순서의 부모.
//  - takeRawSquid(004af610): SID 범위 진단→free 가 아니면 TYPE 불일치 상세/진단과 현재 VT+48(Unpop)→상태 &0xfc→현재 생성자→반환 객체의 상태/VT→TYPE→void→VT+2c(postTake).
//  - readRiftHitPoints(0049fce0): checking 의 HP 플래그 진단과 사제 1/4 HP(현재 전역을 호출 시점마다 읽음).
//  - SquidFactory: 위 부모를 같은 풀·현재 TYPE 표·생성자 표(squid-constructors.js)·공통 Form 층(squid-forms.js)·공간 해제(SquidUnpop)에 연결한다.
//    postCreate(004b0c60)·postTake(004ad4a0)·firstPopFlags(004ad470)·construct/constructCurrent(004af720)/constructByAddress·retype(004b1620)과
//    Form 진입(createContained·createGumpForm·attachForm·popForm·unpopForm)을 제공한다.
// 진단은 정상 복귀할 수 있고, 돌아오면 그 시점의 현재 슬롯/TYPE 표/전역을 다시 읽는다. 선택된 장치가 없으면 그 위치에서 오류이며 앞선 쓰기(할당 포함)는 되돌리지 않는다.
// 근거: cpppj `o/SquidFactory.cpp/.h`(cf4bdec8), `o/RawSquidCreation.cpp`(e6259e97), `o/RawSquidTake.cpp`(e1a617ac), `o/RawRiftHitPoints.cpp`.
// 웹 적응: cpppj 가 장기 주소로 빌리는 005ca8f0(사제 1/4 HP)·005412d0(사제 TYPE)·005412e8/ec(다이스 TYPE)·005412e0(fakeSurface TYPE)은 bindGameAux 로 같은 GameAuxState 를 빌려
//           호출 시점마다 읽는다. HP 의 getNum 은 레코드 객체가 현재 표 배열에서 차지하는 위치로 계산한다(같은 레코드를 공유하지 않는 다른 표는 거부).
// CD 10.72 경로는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-06 WB04-D4: SquidFactory 이식으로 추가)
import { SQUID_BASE_VTABLE } from './squid-vtables.js';
import {
    typeConstructorAddress, constructorRecipe, applyConstructorRecipe, supportsConstructedVtable,
} from './squid-constructors.js';
import { SquidForms, isFormConstructorVtable, SQUID_FORM_TYPES } from './squid-forms.js';

// 원문 진단 파일 이름과 TYPE HP 진단식이다(10.78). (2026-11-06 WB04-D4)
const SQUID_CPP = '\\Ns\\O\\Squid.cpp';
const DAIS_CPP = '.\\Dais.cpp';
const RIFTTYPE_CPP = '\\Ns\\O\\Rifttype.cpp';
const HP_EXPRESSION = '(typeFlags & TYPE_FLAG_HAS_HIT_POINTS) != 0 || getNum() == daisType';

// 10.78 TYPE 표 크기와 첫 자산 TYPE 번호이다. (2026-11-06 WB04-D4)
const TYPE_COUNT = 188, FIRST_ASSET_TYPE = 70;
// 슬롯 필드 위치: TYPE·상태·HP DWORD(+26)·owner(+34)·zOrder(+35)·extra(+40, abstract/buried 비트). (2026-11-06 WB04-D4)
const TYPE = 10, STATE = 11, HP = 26, OWNER = 34, ZORDER = 35, EXTRA = 40;
// 상태 BYTE 의 free·void·contained 비트이다. (2026-11-06 WB04-D4)
const FREE = 1, VOID = 4, CONTAINED = 8;
// TYPE flags1 의 HAS_HIT_POINTS 와 flags2 의 다이스(HP 를 extra 비트로 결정) 비트이다. (2026-11-06 WB04-D4)
const HAS_HIT_POINTS = 0x10, DAIS_FLAG2 = 0x400000;
// 첫 서버 SID 와 첫 예측 SID(10.78)이다. (2026-11-06 WB04-D4)
const FIRST_SERVER_SID = 15000, FIRST_PREDICTABLE_SID = 23001;
// 원문 NULL 생성자(SID 를 읽지 않음)·다이스 보조 생성자(진단 뒤 VT 만 저장)와 그 VT 이다. (2026-11-06 WB04-D4)
const NULL_CONSTRUCTOR = 0x4429e0, DAIS_CONSTRUCTOR = 0x44a420, DAIS_ASSERT_VTABLE = 0x507950;
// GameAuxState 를 빌리지 않은 독립 입력이 쓰는 원문 초기 전역값이다(사제 158·다이스 165·fakeSurface 162). (2026-11-06 WB04-D4)
const DEFAULT_PRIEST_TYPE = 158, DEFAULT_DAIS_TYPE = 165, DEFAULT_FAKE_SURFACE_TYPE = 162;
// 내장 Form 네 생성자 주소 → Form 타입이다(일반 Create 도 이 주소면 Form VT 만 쓴다). (2026-11-06 WB04-D4)
const FORM_CONSTRUCTORS = new Map(SQUID_FORM_TYPES.map((type) => [typeConstructorAddress(type), type]));
// contained 내용물 Form 과 GumpForm 의 내장 타입 번호이다. (2026-11-06 WB04-D4)
const CONTAINED_TYPE = 6, GUMP_FORM_TYPE = 3;

// 부호 없는 DWORD 정수만 받는다. (2026-11-06 WB04-D4)
function uint32(value, name) {
    if (!Number.isInteger(value) || value < 0 || value > 0xffffffff) throw new RangeError(`${name} 는 부호 없는 DWORD 여야 합니다.`);
    return value;
}

// 선택된 장치를 꺼낸다. 없으면 그 위치에서 오류이다. (2026-11-06 WB04-D4)
function need(device, name, where) {
    if (typeof device !== 'function') throw new Error(`${where}의 선택된 실제 장치(${name})가 연결되지 않았습니다.`);
    return device;
}

// 슬롯 +0 의 현재 VT DWORD 를 읽는다. (2026-11-06 WB04-D4)
function vtableOf(raw) {
    return (raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24)) >>> 0;
}

// 지정 위치에 DWORD 를 낮은 바이트부터 쓴다(정렬 무관, 이웃 보존). (2026-11-06 WB04-D4)
function putDword(raw, offset, value) {
    // 네 바이트만 쓴다.
    for (let index = 0; index < 4; index += 1) raw[offset + index] = (value >>> (index * 8)) & 0xff;
}

// 004af530: 원문 Create 부모의 호출 순서만 실행한다. 반환 객체는 장치가 해석하는 32비트 토큰이며 할당 SID 와 다를 수 있다.
// hooks: { fakeSurfaceType() → DWORD, assertion(expression, file, line), allocate(flags) → sid, constructor(type) → 주소, construct(address, sid) → 객체,
//          vtable(객체) → DWORD, storeType(객체, BYTE), postCreate(객체, 저장한 VT) }. (2026-11-06 WB04-D4)
export function createRawSquid(type, flags, hooks = {}) {
    uint32(type, 'Create TYPE');
    uint32(flags, 'Create flags');
    const where = 'Create 부모';
    // 원문 Squid.cpp 진단이다(정상 복귀하면 다음 명령부터 계속).
    const diagnose = (expression, line) => need(hooks.assertion, 'assertion', where)(expression, SQUID_CPP, line);
    if (type === (need(hooks.fakeSurfaceType, 'fakeSurfaceType', where)() >>> 0)) diagnose('type != fakeThreeByThreeSurfaceType', 0x189);
    const sid = need(hooks.allocate, 'allocate', where)(flags) >>> 0;
    // 원문은 signed 비교로 예측 영역 경계를 확인한다.
    const signedSid = sid | 0;
    if (flags & 1) {
        if (signedSid < FIRST_PREDICTABLE_SID) diagnose('sid >= FIRST_PREDICTABLE_SID', 0x191);
    } else if (signedSid >= FIRST_PREDICTABLE_SID) diagnose('sid < FIRST_PREDICTABLE_SID', 0x194);
    const address = need(hooks.constructor, 'constructor', where)(type) >>> 0;
    const object = need(hooks.construct, 'construct', where)(address, sid) >>> 0;
    // ASM 은 TYPE BYTE 쓰기 전에 VT DWORD 만 저장한다.
    const table = need(hooks.vtable, 'vtable', where)(object) >>> 0;
    need(hooks.storeType, 'storeType', where)(object, type & 0xff);
    need(hooks.postCreate, 'postCreate', where)(object, table);
    return object;
}

// 004af610: 원문 Take 부모의 진단/현재 가상 해제/생성자 반환 객체와 저장 순서를 실행한다. 할당/자유 목록은 바꾸지 않는다.
// hooks: { capacity() → numSquids, slot(sid) → 객체, state(객체) → BYTE, type(객체) → BYTE, vtable(객체) → DWORD, detail(serverType, clientType),
//          assertion(expression, file, line), unpop(객체, VT), storeState(객체, BYTE), storeType(객체, BYTE), constructor(type) → 주소,
//          construct(address, sid) → 객체, postTake(객체, 저장한 VT) }. (2026-11-06 WB04-D4)
export function takeRawSquid(type, sid, hooks = {}) {
    uint32(type, 'Take TYPE');
    uint32(sid, 'Take SID');
    const where = 'Take 부모';
    // 원문 Squid.cpp 진단이다(정상 복귀하면 계속).
    const diagnose = (expression, line) => need(hooks.assertion, 'assertion', where)(expression, SQUID_CPP, line);
    const number = sid | 0;
    if (number < FIRST_SERVER_SID || number >= (need(hooks.capacity, 'capacity', where)() | 0)) diagnose('(FIRST_SERVER_SID <= sid && sid < numSquids)', 0x1a7);
    const original = need(hooks.slot, 'slot', where)(sid) >>> 0;
    if (!(need(hooks.state, 'state', where)(original) & FREE)) {
        const previous = need(hooks.type, 'type', where)(original);
        if (previous !== type) {
            need(hooks.detail, 'detail', where)(type, previous);
            diagnose('0', 0x1ba);
        }
        const table = need(hooks.vtable, 'vtable', where)(original) >>> 0;
        need(hooks.unpop, 'unpop', where)(original, table);
    }
    const state = need(hooks.state, 'state', where)(original);
    need(hooks.storeState, 'storeState', where)(original, state & 0xfc);
    const address = need(hooks.constructor, 'constructor', where)(type) >>> 0;
    const object = need(hooks.construct, 'construct', where)(address, sid) >>> 0;
    // 패치판은 반환 객체의 현재 상태와 VT 를 읽은 뒤 TYPE→void→postTake 순서다.
    const current = need(hooks.state, 'state', where)(object);
    const table = need(hooks.vtable, 'vtable', where)(object) >>> 0;
    need(hooks.storeType, 'storeType', where)(object, type & 0xff);
    need(hooks.storeState, 'storeState', where)(object, (current | VOID) & 0xff);
    need(hooks.postTake, 'postTake', where)(object, table);
    return object;
}

// 0049fce0: TYPE 자체의 최대 HP 를 읽는다. globals 는 { checking(), quarter(), priest(), dais() } 현재값 함수, hooks 는 { number() → getNum, assertion }.
// checking 이 켜졌고 HAS_HIT_POINTS 가 없으면 getNum 을 다이스 TYPE 과 비교해 진단하고, 1/4 HP 가 켜졌으면 진단 뒤 getNum 을 다시 계산해 사제 TYPE 과 비교한다. (2026-11-06 WB04-D4)
export function readRiftHitPoints(type, globals, hooks = {}) {
    const where = 'TYPE HP';
    if ((globals.checking() >>> 0) !== 0 && !((type.flags1 >>> 0) & HAS_HIT_POINTS)) {
        const number = need(hooks.number, 'number', where)() >>> 0;
        if (number !== (globals.dais() >>> 0)) need(hooks.assertion, 'assertion', where)(HP_EXPRESSION, RIFTTYPE_CPP, 0x1e3);
    }
    let quarter = false;
    if (globals.quarter()) {
        const number = need(hooks.number, 'number', where)() >>> 0;
        quarter = number === (globals.priest() >>> 0);
    }
    const hp = type.maxHitPoints | 0;
    // signed 나눗셈은 0 방향으로 절삭한다. 음수 몫 0 이 JS -0 으로 남지 않게 정수로 되돌린다(2026-11-07 WB04-D5b: TYPE HP 579행 대조에서 -1/4 결과 교정).
    return quarter ? Math.trunc(hp / 4) | 0 : hp;
}

// 선택한 TYPE 레코드의 현재 표 기준 번호(getNum)이다. 같은 레코드 객체가 선택 표와 현재 표에 모두 있어야 하며, 현재 표가 선택 표를 옮겨 본 것이면 그 상대 위치다. (2026-11-06 WB04-D4)
export function currentRiftTypeNumber(record, selectedBase, currentBase) {
    if (!selectedBase?.length || !currentBase?.length) throw new RangeError('TYPE HP 표의 범위 오류');
    if (!selectedBase.includes(record)) throw new Error('TYPE HP 의 선택 레코드가 선택 표에 없습니다.');
    const index = currentBase.indexOf(record);
    if (index < 0) throw new Error('TYPE HP 의 서로 다른 물리 표 연결');
    return index >>> 0;
}

// 같은 SID 풀의 객체 생성 공장이다(cpppj SquidFactory, 10.78).
// 사용: const factory = new SquidFactory(pool, types, { unpop }); factory.bindDiagnostics({ types, assertion, detail, checking }); factory.bindGameAux(aux);
//       const sid = factory.create(74, 2); factory.take(74, 15009).
// types 는 188칸 레코드 배열({ constructorAddress, flags1, flags2, maxHitPoints, zOrder, footX, footY, name })이며 생성 때 레코드를 복사한다.
// bindDiagnostics 의 types 를 주면 그 배열을 복사 없이 빌려 매 조회에 현재 값을 읽는다. (2026-11-06 WB04-D4)
export class SquidFactory {
    // 같은 실제 풀이다.
    #pool;
    // 생성 때 복사한 독립 TYPE 표와 빌린 현재 진단 묶음이다.
    #types;
    #diagnostics = { types: null, assertion: null, detail: null, checking: () => 0 };
    // 독립 입력의 1/4 HP 선택과 빌린 GameAuxState(장기 전역 소유자)이다.
    #weakenedMana;
    #aux = null;
    // Take 의 자산 VT+48 이 부르는 같은 풀의 공간 해제 처리기이다.
    #unpop;
    // Form 진입이 빌린 공유 postPop 깊이 소유자({value})·현재 contentFormType getter·Pop 전송·종속 분리이다.
    #formPostPopDepth = null;
    #formContentType = null;
    #formTransmit = null;
    #formDetach = null;

    // 판본 크기(188)의 표만 받고 다른 풀의 Unpop 은 거부한다.
    constructor(pool, types, { weakenedMana = false, unpop = null } = {}) {
        if (pool?.edition !== 'patch1078' || typeof pool.physical !== 'function') throw new TypeError('SquidFactory 는 10.78 SidPool 만 받습니다.');
        if (!Array.isArray(types) || types.length !== TYPE_COUNT) throw new RangeError('SID 판본과 타입 수가 다릅니다.');
        if (typeof weakenedMana !== 'boolean') throw new TypeError('약화 HP 선택은 boolean 이어야 합니다.');
        if (unpop !== null && unpop?.pool !== pool) throw new TypeError('Unpop 과 Factory 의 SID 풀이 다릅니다.');
        this.#pool = pool;
        this.#types = types.map((record) => ({ ...record }));
        this.#weakenedMana = weakenedMana;
        this.#unpop = unpop;
    }

    // 같은 풀을 쓰는지 확인하기 위한 실제 풀이다.
    get pool() { return this.#pool; }

    // 현재 TYPE 표(188칸, 복사 없이 빌림)·원문 진단·Take 상세 writer·checking DWORD 공급자를 연결한다. 실행 중 다시 연결해도 진행 중인 장치는 끝까지 쓴다.
    bindDiagnostics({ types = null, assertion = null, detail = null, checking = () => 0 } = {}) {
        if (types !== null && (!Array.isArray(types) || types.length !== TYPE_COUNT)) throw new RangeError('Factory 현재 TYPE 의 판본/길이 오류');
        if (typeof checking !== 'function') throw new TypeError('Factory checking 공급자는 함수여야 합니다.');
        this.#diagnostics = { types, assertion, detail, checking };
    }

    // 생성 HP·fakeSurface 진단이 읽는 장기 전역 소유자(GameAuxState)를 빌린다. 다른 소유자로 다시 연결하는 것은 거부한다.
    bindGameAux(aux) {
        // 네 장기 전역 칸을 모두 가진 소유자만 받는다(사본·기본값으로 대신하지 않는다).
        for (const name of ['priestQuarterHp', 'priestType', 'daisType', 'fakeSurfaceType']) {
            if (!(name in (aux ?? {}))) throw new TypeError(`Factory 장기 전역 소유자에 ${name} 이 없습니다(GameAuxState 를 넘겨야 합니다).`);
        }
        if (this.#aux !== null && this.#aux !== aux) throw new Error('Factory 장기 전역의 다른 소유자 재연결');
        this.#aux = aux;
    }

    // 현재 빌린 표 또는 독립 사본을 읽는다.
    #currentTypes() { return this.#diagnostics.types ?? this.#types; }

    // 현재 표의 TYPE 한 항목이다(Form 번호 포함). 범위 밖은 거부한다.
    #typeRecord(number) {
        const types = this.#currentTypes();
        if (!Number.isInteger(number) || number < 0 || number >= types.length) throw new RangeError('Factory 현재 TYPE 번호 오류');
        return types[number];
    }

    // 자산 TYPE(70 이상)만 받고, requireSupported 면 미복원 생성자 주소를 거부한다.
    #assetType(number, requireSupported) {
        if (!Number.isInteger(number) || number < FIRST_ASSET_TYPE) throw new RangeError('base Squid 타입 번호 오류');
        const record = this.#typeRecord(number);
        const address = record.constructorAddress >>> 0;
        if (requireSupported && address && !constructorRecipe(address)) throw new Error('null/assert 또는 미복원 Squid 생성자는 지원하지 않습니다.');
        return record;
    }

    // 원문 Squid.cpp 진단을 현재 장치로 부른다(실행 중 재연결돼도 꺼낸 장치를 끝까지 쓴다).
    #assert(expression, line) {
        const assertion = this.#diagnostics.assertion;
        need(assertion, 'assertion', 'Factory')(expression, SQUID_CPP, line);
    }

    // 장기 전역 현재값이다(빌리지 않은 독립 입력은 원문 초기값).
    #quarter() { return this.#aux ? this.#aux.priestQuarterHp : this.#weakenedMana; }
    #priestType() { return this.#aux ? this.#aux.priestType : DEFAULT_PRIEST_TYPE; }
    #daisType() { return this.#aux ? this.#aux.daisType : DEFAULT_DAIS_TYPE; }
    #fakeSurfaceType() { return this.#aux ? this.#aux.fakeSurfaceType : DEFAULT_FAKE_SURFACE_TYPE; }

    // 선택한 TYPE 의 현재 HP 를 현재 전역·같은 레코드로 읽는다.
    #typeHitPoints(record, selectedBase) {
        return readRiftHitPoints(record, {
            checking: () => this.#diagnostics.checking(),
            quarter: () => this.#quarter(),
            priest: () => this.#priestType(),
            dais: () => this.#daisType(),
        }, {
            // 진단 뒤에도 같은 레코드의 현재 표 번호를 다시 계산한다.
            number: () => currentRiftTypeNumber(record, selectedBase, this.#currentTypes()),
            assertion: (expression, file, line) => need(this.#diagnostics.assertion, 'assertion', '생성 TYPE HP')(expression, file, line),
        });
    }

    // 표시/전송이 저장한 TYPE 레코드를 같은 현재 HP getter 로 읽는다.
    readSelectedHitPoints(record, selectedBase) { return this.#typeHitPoints(record, selectedBase); }

    // 현재 TYPE 생성자의 최종 VT 기록값이다. 주소 0 은 base, 네 Form 주소는 Form VT, 그 밖은 검증 recipe 이며 미등록은 거부한다.
    constructorVtable(record) {
        const address = record?.constructorAddress >>> 0;
        if (!address) return SQUID_BASE_VTABLE;
        const formType = FORM_CONSTRUCTORS.get(address);
        if (formType !== undefined) return new SquidForms(this.#pool).vtable(formType);
        const recipe = constructorRecipe(address);
        if (!recipe) throw new Error('현재 TYPE 의 미등록 생성자 가상 표 조회');
        return recipe.vtable;
    }

    // 현재 VT 가 base 또는 검증 recipe 의 VT 인지 확인한다.
    static supportsConstructedVtable(table) { return supportsConstructedVtable(table); }

    // 주소별 cdecl 생성자의 원문 쓰기만 물리 슬롯에 적용한다. NULL 함수(0x4429e0)는 SID 를 읽지 않고 true, 미지원/주소 0 은 false 이다.
    // 다이스 보조 생성자는 진단(정상 복귀) 뒤에야 슬롯 범위를 확인하고 VT 0x507950 만 쓴다.
    constructByAddress(address, sid) {
        const value = address >>> 0;
        if (value === NULL_CONSTRUCTOR) return true;
        const recipe = constructorRecipe(value);
        const dais = value === DAIS_CONSTRUCTOR;
        if (!recipe && !dais) return false;
        if (dais) need(this.#diagnostics.assertion, 'assertion', '다이스 보조 생성자')('false', DAIS_CPP, 0x484);
        const raw = this.#pool.physical(sid);
        if (recipe) applyConstructorRecipe(raw, recipe);
        else putDword(raw, 0, DAIS_ASSERT_VTABLE);
        return true;
    }

    // 생성자 쓰기만 이미 확보된 슬롯에 적용한다(TYPE/상태/풀 목록은 덮지 않음). 예약/free/contained 슬롯과 미복원 생성자는 쓰기 전에 거부한다.
    construct(type, sid) {
        const record = this.#assetType(type, true);
        const raw = this.#pool.physical(sid);
        if (sid < 5 || (raw[STATE] & (FREE | CONTAINED))) throw new Error('생성자 슬롯 상태 오류');
        const address = record.constructorAddress >>> 0;
        if (!address) putDword(raw, 0, SQUID_BASE_VTABLE);
        else applyConstructorRecipe(raw, constructorRecipe(address));
        return sid;
    }

    // 현재 원문 cdecl 주소를 타입/상태 선행 검사 없이 같은 물리 슬롯에 적용한다(base·Form·자산 recipe·다이스/NULL).
    #constructRaw(address, sid, requireObject) {
        if (!address) {
            putDword(this.#pool.physical(sid), 0, SQUID_BASE_VTABLE);
            return sid;
        }
        const formType = FORM_CONSTRUCTORS.get(address);
        if (formType !== undefined) {
            new SquidForms(this.#pool).constructVtableAt(formType, sid);
            return sid;
        }
        if (!this.constructByAddress(address, sid)) throw new Error('Create/현재 TYPE 의 선택된 cdecl 생성자 미연결');
        if (requireObject && address === NULL_CONSTRUCTOR) throw new Error('void NULL 생성자는 Create 의 반환 객체를 제공하지 않습니다.');
        return sid;
    }

    // 004af720: 현재 raw TYPE 의 생성자만 실행한다(free/dead/contained/예약도 선행 제외하지 않음, TYPE/HP 미기록).
    constructCurrent(sid) {
        const address = this.#typeRecord(this.#pool.slot(sid)[TYPE]).constructorAddress >>> 0;
        this.#constructRaw(address, sid, false);
    }

    // 생성자가 남긴 VT 의 +28 을 고른다. 네 Form 은 빈 함수이고, 지원 VT 는 공통 postCreate, 그 밖은 미연결 오류다.
    #virtualPostCreate(sid, table) {
        if (isFormConstructorVtable(table)) return;
        if (!supportsConstructedVtable(table)) throw new Error('Create 의 선택된 실제 VT+28 미연결');
        this.postCreate(sid);
    }

    // 004b1620: void/발자국 진단(정상 복귀 시 계속)→TYPE BYTE→현재 TYPE 생성자. HP/소유자/할당은 보존한다. assertion(line) 은 호출자의 원문 진단 장치다.
    retype(sid, type, assertion = null) {
        uint32(type, 'Retype TYPE');
        // 선택된 진단 위치에서만 장치를 요구한다.
        const diagnose = (line) => need(assertion, 'assertion', '타입 변경')(line);
        const pool = this.#pool;
        if (!(pool.slot(sid)[STATE] & VOID)) diagnose(0x22d);
        if (this.#typeRecord(pool.slot(sid)[TYPE]).footX !== this.#typeRecord(type).footX) diagnose(0x22e);
        if (this.#typeRecord(pool.slot(sid)[TYPE]).footY !== this.#typeRecord(type).footY) diagnose(0x22f);
        pool.physical(sid)[TYPE] = type & 0xff;
        this.constructCurrent(sid);
    }

    // 004af530: 실제 Create 부모를 같은 풀·현재 TYPE 표·생성자 표에 연결한다. 할당 뒤 실패해도 할당은 되돌리지 않는다.
    create(type, flags = 0) {
        const pool = this.#pool;
        return createRawSquid(type, flags, {
            fakeSurfaceType: () => this.#fakeSurfaceType(),
            assertion: (expression, file, line) => need(this.#diagnostics.assertion, 'assertion', 'Factory Create')(expression, file, line),
            allocate: (value) => pool.allocate(value),
            constructor: (number) => this.#typeRecord(number).constructorAddress >>> 0,
            // DWORD SID 를 물리 범위로 확인한 뒤 생성자를 적용한다.
            construct: (address, number) => {
                if (number >= pool.capacity) throw new RangeError('Create 생성자의 실제 물리 SID 범위');
                return this.#constructRaw(address, number, true);
            },
            vtable: (object) => vtableOf(pool.slot(object)),
            storeType: (object, value) => { pool.physical(object)[TYPE] = value; },
            postCreate: (object, table) => this.#virtualPostCreate(object, table),
        });
    }

    // 생산 칸 contained 내용물 Form(TYPE 6)도 같은 현재 Create 부모를 쓴다(현재 TYPE 6 생성자와 반환 VT+28).
    createContained(flags = 0) { return this.create(CONTAINED_TYPE, flags); }

    // UI 부착용 GumpForm(TYPE 3)도 같은 현재 Create 부모를 쓴다. 메뉴의 기본 할당 flags 는 2 다.
    createGumpForm(flags = 2) { return this.create(GUMP_FORM_TYPE, flags); }

    // 004af610: 실제 Take 부모를 연결한다. 현재 VT 가 Form 이면 Form Unpop, 지원 자산이면 SquidUnpop.unpop(…, 현재 TYPE, 0)을 부르며 free 목록/카운터는 바꾸지 않는다.
    take(type, sid) {
        const pool = this.#pool;
        // DWORD 토큰을 물리 범위로 확인한다.
        const object = (number) => {
            if (number >= pool.capacity) throw new RangeError('Take 의 실제 물리 SID 범위');
            return number;
        };
        const raw = (number) => pool.physical(object(number));
        return takeRawSquid(type, sid, {
            capacity: () => pool.capacity,
            slot: (number) => object(number),
            state: (number) => raw(number)[STATE],
            type: (number) => raw(number)[TYPE],
            vtable: (number) => vtableOf(raw(number)),
            // 진단보다 먼저 현재 두 TYPE 이름(첫 NUL 까지)을 복사해 상세 writer 에 넘긴다.
            detail: (serverType, clientType) => {
                const name = (record) => {
                    const text = String(record.name ?? '');
                    const end = text.indexOf('\0');
                    return end < 0 ? text : text.slice(0, end);
                };
                const clientName = name(this.#typeRecord(clientType));
                const serverName = name(this.#typeRecord(serverType));
                const text = `Invalid take. Server type ${serverName}, client type ${clientName}`;
                if (text.length >= 256) throw new RangeError('Take 원문 상세 256BYTE 범위');
                need(this.#diagnostics.detail, 'detail', 'Take 오류 상세')(text);
            },
            assertion: (expression, file, line) => this.#assert(expression, line),
            unpop: (number, table) => {
                if (isFormConstructorVtable(table)) {
                    this.unpopForm(object(number));
                    return;
                }
                if (!supportsConstructedVtable(table)) throw new Error('Take 의 선택된 실제 VT+48 미연결');
                if (!this.#unpop) throw new Error('Take 의 선택된 실제 공간 해제 연결이 없습니다.');
                this.#unpop.unpop(object(number), this.#typeRecord(raw(number)[TYPE]), 0);
            },
            storeState: (number, value) => { raw(number)[STATE] = value; },
            storeType: (number, value) => { raw(number)[TYPE] = value; },
            constructor: (number) => this.#typeRecord(number).constructorAddress >>> 0,
            construct: (address, number) => this.#constructRaw(address, object(number), true),
            // 네 Form 의 +2c 는 빈 함수이고 지원 자산은 공통 postTake 다.
            postTake: (number, table) => {
                if (isFormConstructorVtable(table)) return;
                if (!supportsConstructedVtable(table)) throw new Error('Take 의 선택된 실제 VT+2c 미연결');
                this.postTake(object(number));
            },
        });
    }

    // 004ad470: 공통 firstPop 의 직접 몸체다. TYPE/상태/VT 없이 extra(+40)의 abstract(1)/buried(8) 비트만 읽는다(쓰기 없음).
    firstPopFlags(sid) {
        const extra = this.#pool.slot(sid)[EXTRA];
        return (extra & 9) === 0 ? 1 : (((extra & 1) ? 2 : 0) | ((extra & 8) ? 4 : 0));
    }

    // 004b0c60: !isForm/!isContained 진단(정상 복귀 시 현재 슬롯으로 계속)→zOrder BYTE→(서버면) owner 0 과 HP→마지막 contained 진단.
    // HP 필요 여부는 다이스 flags2 면 extra 비트0, 아니면 HAS_HIT_POINTS 이며 HP 는 처음 고른 TYPE 레코드로 읽는다.
    postCreate(sid) {
        const pool = this.#pool;
        const bytes = pool.physical(sid);
        if (bytes[TYPE] < FIRST_ASSET_TYPE) this.#assert('!isForm()', 0x7d4);
        if (bytes[STATE] & CONTAINED) this.#assert('!isContained()', 0x7d5);
        // 두 입구 진단 뒤 고른 TYPE 레코드와 그 표를 저장한다.
        const selectedBase = this.#currentTypes();
        const initial = this.#typeRecord(bytes[TYPE]);
        bytes[ZORDER] = initial.zOrder & 0xff;
        if (pool.isServer) {
            bytes[OWNER] = 0;
            const current = this.#typeRecord(bytes[TYPE]);
            const needsHp = ((current.flags2 >>> 0) & DAIS_FLAG2) ? (bytes[EXTRA] & 1) !== 0 : ((current.flags1 >>> 0) & HAS_HIT_POINTS) !== 0;
            if (needsHp) putDword(bytes, HP, this.#typeHitPoints(initial, selectedBase) >>> 0);
        }
        if (bytes[STATE] & CONTAINED) this.#assert('!isContained()', 0x7df);
    }

    // 004ad4a0: !isForm 진단 뒤 현재 TYPE 의 zOrder BYTE 를 쓰고 마지막에 contained 를 진단한다(owner·HP 보존).
    postTake(sid) {
        const pool = this.#pool;
        if (pool.slot(sid)[TYPE] < FIRST_ASSET_TYPE) this.#assert('!isForm()', 0x7f6);
        const zOrder = this.#typeRecord(pool.slot(sid)[TYPE]).zOrder;
        pool.physical(sid)[ZORDER] = zOrder & 0xff;
        if (pool.slot(sid)[STATE] & CONTAINED) this.#assert('!isContained()', 0x7fa);
    }

    // 현재 World/Session 의 공통 종속 분리만 빌린다(UI 검색 없음). null 이면 독립 기본 분리이다.
    bindFormDetach(detach) {
        if (detach !== null && typeof detach !== 'function') throw new TypeError('Form 종속 분리는 함수여야 합니다.');
        this.#formDetach = detach;
    }

    // 실제 World 의 같은 postPop 깊이 소유자({value: DWORD})만 빌린다. 다른 소유자 재연결은 거부한다.
    bindFormPopDepth(owner) {
        if (!Number.isInteger(owner?.value)) throw new TypeError('Form Pop 깊이 소유자는 {value: DWORD} 여야 합니다.');
        if (this.#formPostPopDepth !== null && this.#formPostPopDepth !== owner) throw new Error('Form Pop 의 다른 공유 깊이 소유자 재연결');
        this.#formPostPopDepth = owner;
    }

    // 현재 Session 의 contentFormType getter(예: squidFormGlobals(aux).contentFormType)와 Pop 전송을 빌린다.
    bindFormAttach(contentType = null, transmit = null) {
        if (contentType !== null && typeof contentType !== 'function') throw new TypeError('Form 부착 contentType 은 함수여야 합니다.');
        if (transmit !== null && typeof transmit !== 'function') throw new TypeError('Form Pop 전송은 함수여야 합니다.');
        this.#formContentType = contentType;
        this.#formTransmit = transmit;
    }

    // 같은 공장의 현재 진단과 공유 깊이를 Form 층의 Pop 에 빌려 준다.
    #bindCurrentFormPop(forms) {
        const hooks = { assertion: this.#diagnostics.assertion };
        const owner = this.#formPostPopDepth;
        if (owner) {
            hooks.depth = () => owner.value >>> 0;
            hooks.setDepth = (value) => { owner.value = value >>> 0; };
        }
        forms.bindPop(hooks);
    }

    // 현재 진단/깊이와 실제 VT+20 의 공통 Form Pop 을 실행한다. transmit 을 주면 그 전송을 쓴다.
    popForm(form, flags, transmit = null) {
        const forms = new SquidForms(this.#pool, { transmit: transmit ?? this.#formTransmit });
        this.#bindCurrentFormPop(forms);
        forms.pop(form, flags);
    }

    // 공통 Form 부착 부모를 현재 TYPE genus·현재 contentFormType(없으면 6)·진단·공유 깊이로 실행한다.
    attachForm(form, parent, flags = 0, transmit = null) {
        const forms = new SquidForms(this.#pool, { transmit: transmit ?? this.#formTransmit });
        this.#bindCurrentFormPop(forms);
        forms.bindAttach({
            genus: (number) => this.#typeRecord(number).flags2 >>> 0,
            contentType: this.#formContentType ?? (() => 6),
            assertion: this.#diagnostics.assertion,
        });
        forms.attach(form, parent, flags);
    }

    // Form 공통 Unpop 을 현재 진단과 빌린 종속 분리로 실행한다.
    unpopForm(sid) {
        new SquidForms(this.#pool, { unpop: { detach: this.#formDetach, assertion: this.#diagnostics.assertion } }).unpop(sid);
    }
}
