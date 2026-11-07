// 원본 10.78 의 객체 부착 프로세스(ProcessForm 종속 SID + Kernel 슬롯)와 지연/주기 이벤트(Regular)를 옮긴 모듈이다(cpppj `o/SquidProcess.cpp/.h`).
//  - SquidProcess(원본 BaseProcess +4 formSid, +8 attachToSid): 프로세스마다 ProcessForm 타입의 SID 하나가 부모 객체 종속 체인에 들어간다.
//    가상 통지는 form 삭제 직전(onFormDestroy, vtable +0xc)·부모 삭제 중(onParentDying, +0x10)·부착 직후(onAttached, +0x14)·Depend 통지(notify, ProcessForm 가상 34)이다.
//  - RegularProcess(타입 46)와 공유 Regular(타입 61): 정해진 시각에 부모의 이벤트 처리기를 불러 반환값(양수=재예약, 0=종료, 음수=그대로)으로 다음 시각을 정한다.
//  - SquidProcessState: 프로세스 계층이 읽는 원본 전역(게임 시각 0055b4d0·권한 00540bc4·서버 00540bc0·타입 순환 0059af74·geyser 00541240·일반/공유/상한 TYPE·ProcessForm TYPE).
//    독립 입력은 자기 값을, 실제 세션은 bindAuthority/bindNow/bindGameAux 로 같은 장기 전역을 빌린다(사본을 동기화하지 않고 매번 현재 값을 읽는다).
//  - SquidProcessHost: Kernel 등록→ProcessForm 생성/지정 슬롯 Take→타입/PID 기록→부모 부착→onAttached(attach/reattach), Regular 부착(addRegular/addSharedRegular/attachRegular),
//    Form 갱신(refreshForm)·삭제(kill)·조회(fromForm/findEvent/findEventMasked)·Depend 통지·삭제 훅(hooks)을 맡는다. 장기 호스트는 `SquidProcessHost.borrowing`으로 만들고
//    호출 위치마다 현재 World 의 처리기 묶음(createProcessHostRuntime)을 빌린다(진행 중 호출 수 activeCalls 로 반납을 거부).
// 동작은 cpppj 의 원문 대조 순서(진단 정상 복귀·부분 쓰기·현재 TYPE/VT/슬롯 재조회)를 그대로 따른다. 미복원 파생 통지는 임의 성공 대신 오류로 거부한다.
// 근거: cpppj `o/SquidProcess.cpp/.h`(기준선 1f00a25a, 원본 Baseprocess.cpp/Regular.cpp: 0041bd20 부착, 0041bf10 Kill, 0041c010 갱신, 004ae240/004ae2b0 form 통지,
//       004ae2d0 FromForm, 004afb40/004afbe0 조회, 00496e80/00496f00 Regular 생성자, 00496cb0 Regular 실행). 원본 관측 fixture는 tests/squid-process.test.mjs 에서 대조한다.
// CD 10.72 경로는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-D5c: Process 호스트 이식으로 추가)
import { BaseProcess } from './kernel.js';
import { RawSquidDestroy, SQUID_DESTROY_EFFECT } from './squid-destroy.js';
import { SquidFactory } from './squid-factory.js';
import { SquidForms } from './squid-forms.js';
import { SQUID_FORM_VTABLES } from './squid-vtables.js';
import { commonContainedParent } from './squid-contained-unpop.js';
import { attachBaseProcess, attachProcessForm } from './raw-process-attach.js';
import { BRIDGE_FALL_EVENT, RawBridgeLifecycle } from './bridge-lifecycle.js';

// 공통 raw 필드 위치이다: 종속 체인의 다음(+4)·머리(+6)·TYPE(+10)·상태(+11)·장르 BYTE(+40)·Form 의 Kernel 번호 DWORD(+18, 비정렬). (2026-11-08 WB04-D5c)
const NEXT = 4, HEAD = 6, TYPE = 10, STATE = 11, GENUS = 40, FORM_PID = 18;
// 상태 비트: free·dead·void·contained. (2026-11-08 WB04-D5c)
const FREE = 1, DEAD = 2, VOID = 4;
// 타입 번호: 프로세스 타입의 첫 번호, form 으로 취급하는 마지막 번호(finder 상한). (2026-11-08 WB04-D5c)
const FIRST_PROCESS_TYPE = 10, LAST_FORM_TYPE = 0x45;
// 10.78 타입 표의 길이이다(판본 검사). (2026-11-08 WB04-D5c)
const TYPE_COUNT = 188;
// 부착 flags: 0x10 client 번호로 form 할당, 0x40 전송 억제, 0x50 로컬 전용(Regular 생성자의 고정 flags). (2026-11-08 WB04-D5c)
const CLIENT_FORM = 0x10, LOCAL_MASK = 0x50;
// abstract 상태여도 실제 Regular 를 실행하는 부모 장르(factory·vortex·dais 제외) 비트이다. (2026-11-08 WB04-D5c)
const ABSTRACT_RUN_GENUS = 0x404200;
// geyser 에 붙은 Regular 가 허용하는 가장 먼 예약(초)이다. (2026-11-08 WB04-D5c)
const POLL_LIMIT = 0.5;
// SharedRegular 진단의 원문 파일 이름과 행이다. (2026-11-08 WB04-D5c)
const REGULAR_CPP = '\\Ns\\O\\Regular.cpp';
const SHARED_REGULAR_DIAGNOSTIC_LINE = 0x70;
// 갱신 진단의 원문 파일 이름이다. (2026-11-08 WB04-D5c)
const BASEPROCESS_CPP = '\\Ns\\O\\Baseprocess.cpp';

// 슬롯 WORD 를 읽는다. (2026-11-08 WB04-D5c)
function word(raw, offset) {
    return raw[offset] | (raw[offset + 1] << 8);
}

// 슬롯의 VT DWORD 를 읽는다. (2026-11-08 WB04-D5c)
function vtableOf(raw) {
    return (raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24)) >>> 0;
}

// 슬롯의 비정렬 Kernel 번호 DWORD 전체를 읽는다(WORD 로 절단하지 않음). (2026-11-08 WB04-D5c)
function pidOf(raw) {
    return (raw[FORM_PID] | (raw[FORM_PID + 1] << 8) | (raw[FORM_PID + 2] << 16) | (raw[FORM_PID + 3] << 24)) >>> 0;
}

// 호스트가 쓰는 프로세스 내부 필드 접근자이다. 클래스 비공개 필드를 같은 모듈의 호스트만 만지게 하려고 정적 블록에서 채운다(C++ friend 에 해당). (2026-11-08 WB04-D5c)
let friend;
let regularFriend;

// 원본 BaseProcess 에 부착되는 프로세스 공통 기반이다. 부착 전에는 form/parent/pid 가 모두 0, host 는 null 이다.
// 사용: class Thunder extends SquidProcess { runFrame() { … this.host.kill(this, 0); } }; host.attach(new Thunder(), 12, parentSid, 0x50). (2026-11-08 WB04-D5c)
export class SquidProcess extends BaseProcess {
    // 부모의 종속 체인에 들어 있는 form 과 부착 대상 객체이다. 부착 전에는 0.
    #form = 0;
    #parent = 0;
    // Kernel 슬롯 번호이다. 부착 전에는 0. (2026-10-11 cpppj 추가: 건설 프로세스가 한 프레임에 Kill 을 두 번 판단하므로 Kill 전에 읽어 둔다)
    #pid = 0;
    // 부착한 호스트와, 그 부착 때의 현재 World 처리기 묶음(독립 World 최종 반납 식별용)이다.
    #host = null;
    #binding = null;

    // 호스트가 만지는 비공개 필드 접근자를 만든다.
    static {
        friend = {
            fields: (process) => ({
                get form() { return process.#form; },
                set form(value) { process.#form = value >>> 0; },
                get parent() { return process.#parent; },
                set parent(value) { process.#parent = value >>> 0; },
            }),
            host: (process) => process.#host,
            binding: (process) => process.#binding,
            setHost: (process, host, binding) => {
                process.#host = host;
                process.#binding = binding;
            },
            setPid: (process, pid) => { process.#pid = pid >>> 0; },
        };
    }

    // ProcessForm 의 SID 이다. 부착 전에는 0.
    get form() { return this.#form; }

    // 부착 대상 부모 SID 이다. 부착 전에는 0.
    get parent() { return this.#parent; }

    // 부착한 호스트이다(파생 프로세스가 Kill 등을 부를 때 쓴다). 부착 전에는 null.
    get host() { return this.#host; }

    // Kernel 슬롯 번호이다. Kill 뒤에도 등록 여부를 물으려면 Kill 전에 읽어 둔 값을 쓴다. 부착 전에는 0.
    get pid() { return this.#pid; }

    // form 삭제 직전 가상 통지(vtable +0xc)이다. 원본 기반 클래스는 빈 함수이다.
    onFormDestroy(flags) {}

    // 부모 삭제 중 가상 통지(vtable +0x10)이다. 원본 기반 클래스는 빈 함수이다.
    onParentDying(parent) {}

    // 부착 직후 가상 통지(vtable +0x14)이다. 원본 기반 클래스는 빈 함수이다.
    onAttached() {}

    // ProcessForm 가상 34 가 같은 Kernel 객체의 실제 내장 notify1c 에 전달하는 event/info/value 이다. 복원하지 않은 파생 통지는 선택 시 예외로 거부한다.
    notify(event, info, value) {
        throw new Error('현재 파생 프로세스의 notify1c 미복원');
    }
}

// 정해진 시각에 부모의 이벤트 처리기를 부르고 반환값으로 다음 시각을 정하는 프로세스이다. Regular(타입 46)와 SharedRegular(타입 61)는 같은 실행 몸체를 쓴다.
// 사용: host.addRegular(parentSid, 0x2692, 1.5). 시각은 부착 때의 현재 시각으로 정해진다. (2026-11-08 WB04-D5c)
export class RegularProcess extends SquidProcess {
    // 원본 +0x10 다음 실행 시각(double), +0x18 이벤트 번호, +0x1c payload(재예약 뒤에는 직전 반환값, float32), +0x20 호출 횟수이다.
    #time = 0;
    #event = 0;
    #payload = 0;
    #count = 0;

    // 호스트의 실행 몸체가 시각/payload/횟수를 직접 쓰는 접근자를 만든다.
    static {
        regularFriend = {
            setTime: (process, value) => { process.#time = value; },
            setPayload: (process, value) => { process.#payload = Math.fround(value); },
            incrementCount: (process) => { process.#count = (process.#count + 1) >>> 0; },
        };
    }

    // 00496e80 ↔ CD 0048efd0 의 필드 초기값이다. 시각은 부착 때의 현재 시각으로 정해진다.
    constructor(event, payload) {
        super();
        this.#event = event >>> 0;
        this.#payload = Math.fround(payload);
    }

    // 00496cb0: 부착된 호스트의 실행 몸체를 부른다. 부착하지 않은 프로세스는 Kernel 에 없으므로 실행되지 않는다.
    runFrame() {
        const host = this.host;
        if (host) host.runRegular(this);
    }

    // 이벤트 번호이다.
    get event() { return this.#event; }

    // payload(float32)이다.
    get payload() { return this.#payload; }

    // 호출 횟수이다.
    get count() { return this.#count; }

    // 다음 실행 시각이다.
    get time() { return this.#time; }

    // Dude task 변경(패치 00450670/CD 004e8e70)의 event/count0/다음 double 시각 직접 쓰기를 같은 기존 Regular 에 적용한다. payload/Form/부착은 보존한다.
    resetTask(event, time) {
        this.#event = event >>> 0;
        this.#count = 0;
        this.#time = time;
    }

    // Carrier 426db0 의 생성 이후 +10/+14 쓰기이다. 다른 필드는 재초기화하지 않고 실제 기한만 덮어쓴다.
    setNextTime(time) {
        this.#time = time;
    }

    // Damageable 삭제 뒤 새 Regular 에 원본 +0x20 초기 count 를 쓴다(패치 0044b9e0/CD 00461b40). event/payload/time/Form 은 보존한다.
    setInitialCount(count) {
        this.#count = count >>> 0;
    }
}

// 프로세스 계층이 읽거나 바꾸는 원본 전역이다. 독립 입력은 자기 값을 쓰고, 실제 세션은 bind* 로 같은 장기 전역을 빌려 매번 현재 값을 읽는다.
// 사용: const state = new SquidProcessState(); state.now = 12.5; state.bindGameAux(aux). (2026-11-08 WB04-D5c)
export class SquidProcessState {
    // 독립 입력의 자기 값이다: 게임 시각(DAT_0055b4d0)·권한(00540bc4)·서버(00540bc0)·geyser TYPE(00541240)·일반/공유 Regular TYPE·프로세스 TYPE 상한(signed 비교 비트)·ProcessForm TYPE.
    #now = 0;
    #boss = true;
    #server = true;
    #pollType = 122;
    #regularType = 46;
    #sharedRegularType = 61;
    #processTypeEnd = 63;
    #processFormType = 2;
    // 빌린 권한 소유자(RawAuthorityState)·현재 게임 시각 getter·장기 TYPE 소유자(GameAuxState)이다. 없으면 null.
    #authority = null;
    #clock = null;
    #aux = null;

    // 패치 Kernel 등록이 전진시키는 타입 순환 값(DAT_0059af74)이다. CD 에는 없다.
    typeRotation = 0;

    // 독립 SharedRegular 생성의 현재 basePostPop 입력이다. 실제 World 는 처리기 묶음의 postPopDepth getter 를 쓴다.
    postPopDepth = 0;

    // 독립 진단(expression, file, line)이다. 정상 복귀/명시 중단은 이 함수가 정한다. 기본 깊이 0 에서는 요구하지 않는다.
    assertion = null;

    // 실제 두 bool(RawAuthorityState)을 빌린다. 이후 boss/server 는 그 객체의 현재 값을 읽고 쓴다. null 이면 자기 값으로 돌아간다.
    bindAuthority(authority) {
        if (authority !== null && (typeof authority !== 'object' || typeof authority.boss !== 'boolean' || typeof authority.server !== 'boolean')) throw new TypeError('권한 소유자는 boss/server 를 가진 객체여야 합니다.');
        this.#authority = authority;
    }

    // 실제 Client 프레임 시각의 현재 getter 를 빌린다(() => number). null 이면 자기 값으로 돌아간다. 읽기만 하며 쓰지 않는다.
    bindNow(clock) {
        if (clock !== null && typeof clock !== 'function') throw new TypeError('게임 시각은 getter 함수여야 합니다.');
        this.#clock = clock;
    }

    // 실제 장기 TYPE 소유자(GameAuxState)를 빌린다. geyserType·regularProcessType·sharedRegularProcessType·processTypeEnd·processFormType 를 매번 현재 값으로 읽는다.
    bindGameAux(aux) {
        if (aux !== null && ['geyserType', 'regularProcessType', 'sharedRegularProcessType', 'processTypeEnd', 'processFormType'].some((name) => !(name in aux))) {
            throw new TypeError('GameAuxState 의 프로세스 TYPE 다섯 칸이 필요합니다.');
        }
        this.#aux = aux;
    }

    // 게임 시각이다(double). 빌린 getter 가 있으면 그 현재 값을 읽는다.
    get now() { return this.#clock ? this.#clock() : this.#now; }
    set now(value) { this.#now = value; }

    // 권한 플래그이다. 꺼져 있으면 서버 번호 form 은 flag 0x10 없이 지우지 않는다.
    get boss() { return this.#authority ? this.#authority.boss : this.#boss; }
    set boss(value) {
        if (this.#authority) this.#authority.boss = Boolean(value);
        else this.#boss = Boolean(value);
    }

    // ProcessForm 통지가 매번 읽는 서버 전역이다.
    get server() { return this.#authority ? this.#authority.server : this.#server; }
    set server(value) {
        if (this.#authority) this.#authority.server = Boolean(value);
        else this.#server = Boolean(value);
    }

    // geyser TYPE 이다: 0.5초보다 먼 Regular 예약을 현재 시각으로 당긴다.
    get pollType() { return this.#aux ? this.#aux.geyserType >>> 0 : this.#pollType; }
    set pollType(value) {
        if (this.#aux) this.#aux.geyserType = value;
        else this.#pollType = value >>> 0;
    }

    // 일반 Regular TYPE 이다(현재 전체 DWORD).
    get regularType() { return this.#aux ? this.#aux.regularProcessType >>> 0 : this.#regularType; }
    set regularType(value) {
        if (this.#aux) this.#aux.regularProcessType = value;
        else this.#regularType = value >>> 0;
    }

    // 공유 Regular TYPE 이다(생성/조회가 같은 번호).
    get sharedRegularType() { return this.#aux ? this.#aux.sharedRegularProcessType >>> 0 : this.#sharedRegularType; }
    set sharedRegularType(value) {
        if (this.#aux) this.#aux.sharedRegularProcessType = value;
        else this.#sharedRegularType = value >>> 0;
    }

    // 프로세스 TYPE 상한(10.78, signed 비교)이다. 읽기 전에 signed 변환이나 정규화를 하지 않는다.
    get processTypeEnd() { return this.#aux ? this.#aux.processTypeEnd >>> 0 : this.#processTypeEnd; }
    set processTypeEnd(value) {
        if (this.#aux) this.#aux.processTypeEnd = value;
        else this.#processTypeEnd = value >>> 0;
    }

    // ProcessForm TYPE 이다(각 생성/Take 위치마다 현재 전체 번호를 읽는다).
    get processFormType() { return this.#aux ? this.#aux.processFormType >>> 0 : this.#processFormType; }
    set processFormType(value) {
        if (this.#aux) this.#aux.processFormType = value;
        else this.#processFormType = value >>> 0;
    }
}

// 한 World 가 호스트에 빌려주는 현재 삭제/사건/전송 처리기 묶음을 만든다. 호스트 자체의 장기 수명과 구별한다.
// 필드: destroy(현재 World 의 RawSquidDestroy, SID 풀 정체성 검증), regular(부모의 가상 Regular 사건 (parent, event, count, payload) → float), asset({ emit, selected, unpopForm? }),
// transmit({ writeData(process, parentChanged) → int32, transmit(length) }), postPopDepth(() → 현재 공통 Pop 깊이), assertion((expression, file, line)), factory(같은 풀의 SquidFactory).
// activeCalls 는 선택된 호스트 효과가 진행 중인 호출 수이며 진행 중 반납을 거부하는 데 쓴다. (2026-11-08 WB04-D5c)
export function createProcessHostRuntime({ destroy = null, regular = null, asset = {}, transmit = {}, postPopDepth = null, assertion = null, factory = null } = {}) {
    return { destroy, regular, asset, transmit, activeCalls: 0, postPopDepth, assertion, factory };
}

// 객체 Form 과 커널 프로세스의 부착·통지·수명을 연결하는 호스트이다.
// 사용(직접): const host = new SquidProcessHost(pool, types, kernel, destroy, state, { regular, asset, transmit });
//       (장기): const host = SquidProcessHost.borrowing(pool, types, kernel, state, () => currentRuntime). (2026-11-08 WB04-D5c)
export class SquidProcessHost {
    // 풀·TYPE 표(직접 호스트는 생성 때의 사본, 장기 호스트는 같은 저장소의 실제 현재 표)·Kernel·상태이다.
    #pool;
    #types;
    #sharedTypes;
    #kernel;
    #state;
    // 직접 호스트의 삭제기·부모 Regular 처리기·자산 훅·전송 경계·직접 연결한 Form 공장이다. 장기 호스트에서는 쓰지 않는다.
    #destroy;
    #regular;
    #asset;
    #transmit;
    #factory = null;
    // 장기 호스트의 현재 World 처리기 묶음 getter 이다. 직접 호스트는 null.
    #current;

    // 직접 호스트를 만든다. 풀/Kernel/공통 삭제/상태는 호출자가 소유하며 호스트보다 오래 살아야 한다. asset 은 form 이 아닌 객체의 삭제 훅이다.
    // 장기 호스트는 `borrowing` 으로 만든다(destroy 를 null 로 두고 options.current 를 준다).
    constructor(pool, types, kernel, destroy, state, { regular = null, asset = null, transmit = null, current = null } = {}) {
        if (pool?.edition !== 'patch1078') throw new TypeError('프로세스 호스트는 10.78 SID 풀만 받습니다.');
        if (!Array.isArray(types) || types.length !== TYPE_COUNT) throw new RangeError('프로세스 타입 판본/크기 오류');
        if (!(state instanceof SquidProcessState)) throw new TypeError('프로세스 상태가 필요합니다.');
        if (current !== null) {
            if (typeof current !== 'function') throw new TypeError('장기 ProcessHost의 실제 판본/현재 대여 계약');
            this.#sharedTypes = types;
            this.#types = null;
            this.#destroy = null;
        } else {
            if (!(destroy instanceof RawSquidDestroy)) throw new TypeError('프로세스 삭제기가 필요합니다.');
            if (destroy.pool !== pool) throw new TypeError('프로세스/삭제 SID 풀이 다릅니다');
            this.#types = types.slice();
            this.#sharedTypes = null;
            this.#destroy = destroy;
        }
        this.#pool = pool;
        this.#kernel = kernel;
        this.#state = state;
        this.#regular = regular;
        this.#asset = asset;
        this.#transmit = transmit;
        this.#current = current;
    }

    // 장기 호스트이다. Pool/Kernel/상태를 빌리고 현재 World 의 가상 처리기만 선택 위치에서 조회한다. types 는 같은 저장소의 실제 현재 TYPE 표이다.
    static borrowing(pool, types, kernel, state, current) {
        return new SquidProcessHost(pool, types, kernel, null, state, { current });
    }

    // 같은 실제 풀이다(모듈을 연결할 때 다른 월드의 풀을 섞지 않도록 제공).
    get pool() { return this.#pool; }

    // 판본의 프로세스 타입 번호 범위 끝이다(10.78: 같은 현재 0054116c 의 signed 상한 비트).
    get processTypeEnd() { return this.#state.processTypeEnd; }

    // form 의 실제 가상 표 기록값이다(10.78 ProcessForm).
    get formVtable() { return SQUID_FORM_VTABLES.process; }

    // 선택된 현재 World 처리기 묶음을 요구한다. 빈 World 를 합성하지 않고 부재를 그 위치에서 보고한다.
    #runtime() {
        const value = this.#current();
        if (!value || !value.destroy || value.destroy.pool !== this.#pool) throw new Error('장기 ProcessHost의 현재 World 처리기 미연결');
        return value;
    }

    // 현재 대여(있으면)를 호출 동안 붙잡고 work(runtime|null)를 실행한다. 정상/예외 모두 끝난 뒤 반납한다. 직접 호스트는 runtime 이 null.
    #withCall(work) {
        const runtime = this.#current ? this.#current() : null;
        if (runtime) runtime.activeCalls += 1;
        try {
            return work(runtime);
        } finally {
            if (runtime) runtime.activeCalls -= 1;
        }
    }

    // 소비 위치마다 현재 공통 삭제 몸체를 조회한다.
    #destroyer() {
        return this.#current ? this.#runtime().destroy : this.#destroy;
    }

    // 현재 TYPE 표 배열(직접 호스트의 사본, 장기 호스트의 실제 현재 표)이다.
    #typeList() {
        return this.#sharedTypes ?? this.#types;
    }

    // 현재 TYPE 배열의 정확한 한 항목이다.
    #type(number) {
        const list = this.#typeList();
        if (!Number.isInteger(number) || number < 0 || number >= list.length) throw new RangeError('ProcessHost 현재 TYPE 범위');
        return list[number];
    }

    // 부모 Regular 사건 처리기이다(호출 중 현재 대여 반납이 있어도 복사한 함수의 수명을 지킨다).
    #regularHandler() {
        return this.#current ? this.#runtime().regular : this.#regular;
    }

    // 자산 가상 분배(현재 자산 효과)이다.
    #assetHooks() {
        return this.#current ? this.#runtime().asset : this.#asset;
    }

    // 직렬화/송신 경계이다. 각 소비 위치에서 현재 경계를 얻는다.
    #transmission() {
        return (this.#current ? this.#runtime().transmit : this.#transmit) ?? {};
    }

    // 원문 진단 장치이다(현재 대여의 것 또는 독립 상태의 것). 없으면 호출자가 그 위치에서 오류를 낸다.
    #assertionDevice() {
        return this.#current ? this.#runtime().assertion : this.#state.assertion;
    }

    // 같은 수명의 직접 호스트에만 Form 공장을 연결한다. 장기 호스트는 현재 대여의 factory 를 소비 위치에서 조회한다.
    bindFormFactory(factory) {
        if (this.#current || factory?.pool !== this.#pool || (this.#factory && this.#factory !== factory)) throw new TypeError('ProcessForm 직접 공장 연결의 같은 수명/Pool 계약');
        this.#factory = factory;
    }

    // 독립 입력도 같은 현재 표/진단 부모를 쓰는 공장을 만든다(고정 VT 경로를 추가하지 않는다).
    #independentFactory() {
        const factory = new SquidFactory(this.#pool, this.#typeList());
        factory.bindDiagnostics({ types: this.#typeList(), assertion: this.#state.assertion });
        return factory;
    }

    // 인자 계산 뒤 현재 번호를 읽고 실제 선택 장치(World 공장 / 직접 공장 / 독립 공장)만 요구한다. 지정 슬롯이면 Take 하고 그 SID 를, 아니면 Create(client flag 면 flags 2)한다.
    #createOrTakeForm(flags, form) {
        const apply = (factory) => {
            if (factory.pool !== this.#pool) throw new TypeError('ProcessForm 공장의 실제 Pool 불일치');
            const type = this.#state.processFormType;
            // 지정 슬롯의 Take 반환은 원문처럼 무시하고 원래 SID 를 후단에 사용한다.
            if (form) {
                factory.take(type, form);
                return form;
            }
            return factory.create(type, (flags & CLIENT_FORM) ? 2 : 0);
        };
        if (this.#current) {
            const runtime = this.#runtime();
            if (!runtime.factory) throw new Error('현재 World의 ProcessForm 공장 미연결');
            return apply(runtime.factory);
        }
        if (this.#factory) return apply(this.#factory);
        return apply(this.#independentFactory());
    }

    // 이 호스트가 부착한 프로세스인지 본다(PID 나 SID 를 정규화하지 않는다).
    owns(process) {
        return process instanceof SquidProcess && friend.host(process) === this;
    }

    // 독립 World 의 최종 반납: 자기 현재 대여가 만든 프로세스만 정리하며 이전 Native 잔류는 보존한다(Kernel.clear 의 단일 순회를 대신하지 않는다).
    drainRuntimeBindings(runtime) {
        const pinned = runtime;
        pinned.activeCalls += 1;
        try {
            // 슬롯 0/덮어쓴 소유권도 같은 모듈이 살아 있는 대여 반납에서 정리한다.
            this.#kernel.drainOwnedForShutdown((candidate) => candidate instanceof SquidProcess && friend.host(candidate) === this && friend.binding(candidate) === pinned);
        } finally {
            pinned.activeCalls -= 1;
        }
    }

    // 현재 호스트의 잔류가 모두 Regular 인지 본다(단일 현재 표를 읽으며 다른 호스트/Client 프로세스는 자기 소유로 판정하지 않는다).
    hasOnlyRegularProcesses() {
        for (let id = 1; id <= this.#kernel.capacity; id += 1) {
            const process = this.#kernel.get(id);
            if (process && this.owns(process) && !(process instanceof RegularProcess)) return false;
        }
        return true;
    }

    // 정렬 없이 form 의 VT 로 가상 삭제 효과를 처리할 현재 Process VT 인지 판단한다. finder 의 원문 TYPE/상한 관문과 구별한다.
    #isProcessForm(sid) {
        if (!sid || sid >= this.#pool.capacity) return false;
        return vtableOf(this.#pool.slot(sid)) === this.formVtable;
    }

    // 004b1440→004ac800→004ac560: 부모의 종속 체인 머리에 넣고 contained 를 켠 뒤 Pop 한다. ProcessForm 이면 직렬화 접두(attachProcessForm)를 먼저 실행하고,
    // Depend·Gump·Content 의 현재 VT 는 공통 부모로 보낸다(그 밖은 미복원 오류).
    #attachForm(form, parent, flags) {
        return this.#withCall(() => {
            let selected = null;
            const hooks = {
                server: () => this.#pool.isServer,
                // flags 가 로컬이어도 서버 lookup 은 먼저 실행한다. NULL 은 writer 선택 때만 오류이다.
                lookup: (sid) => { selected = this.fromForm(sid); },
                writeData: (changed) => {
                    if (!selected) throw new Error('Process 부착의 선택된 실제 PID 미연결');
                    const device = this.#transmission().writeData;
                    if (typeof device !== 'function') throw new Error('Process 부착의 선택된 실제 writer 미연결');
                    return device(selected, changed);
                },
                transmit: (size) => {
                    const device = this.#transmission().transmit;
                    if (typeof device !== 'function') throw new Error('Process 부착의 선택된 실제 송신 장치 미연결');
                    device(size);
                },
                assertion: (expression, file, line) => {
                    const device = this.#assertionDevice();
                    if (typeof device !== 'function') throw new Error('Process 부착의 선택된 원문 진단 장치 미연결');
                    device(expression, file, line);
                },
                // 파생 직렬화 접두 뒤에만 공통 현재 genus/링크/Pop 진단을 실행한다.
                attach: (sid, into, popFlags) => {
                    if (this.#current) {
                        const runtime = this.#runtime();
                        if (!runtime.factory) throw new Error('현재 World의 공통 Form 부착 공장 미연결');
                        runtime.factory.attachForm(sid, into, popFlags);
                    } else if (this.#factory) this.#factory.attachForm(sid, into, popFlags);
                    else this.#independentFactory().attachForm(sid, into, popFlags);
                },
            };
            // TYPE 을 바꾸어도 실제 생성 VT 의 +74 를 선택한다.
            const table = vtableOf(this.#pool.slot(form));
            if (table === this.formVtable) {
                attachProcessForm(this.#pool, form, parent, flags, hooks);
                return;
            }
            // 확인된 내장 Form 세 클래스(Depend·Gump·Content)의 +74 는 공통 부모이다.
            const common = [SQUID_FORM_VTABLES.depend, SQUID_FORM_VTABLES.gump, SQUID_FORM_VTABLES.content].includes(table);
            if (!common) throw new Error('BaseProcess 부착의 미복원 현재 VT+74');
            hooks.attach(form, parent, flags);
        });
    }

    // 004ae140/004ac6b0: void 를 켜고, 부모가 free/dead 가 아니면 체인에서 뺀다(현재 World 공장 / 직접 공장 / 독립 공통 Unpop).
    #unpopForm(form) {
        this.#withCall((runtime) => {
            if (runtime) {
                if (!runtime.factory) throw new Error('Process Unpop의 현재 공장 미연결');
                runtime.factory.unpopForm(form);
                return;
            }
            if (this.#factory) {
                this.#factory.unpopForm(form);
                return;
            }
            // 독립 호스트도 같은 공통 해제/진단 순서를 사용한다.
            new SquidForms(this.#pool, { unpop: { assertion: this.#state.assertion } }).unpop(form);
        });
    }

    // 0041bd20 ↔ CD 0048f590: Kernel 등록 → ProcessForm 생성/지정 슬롯 Take → 타입/PID 기록 → 부모 부착 → onAttached. 이미 Kernel 에 있는 같은 객체는 reattach 로 넘긴다.
    // 패치판은 부모가 free/dead 이면 등록하지 않고 null 을 반환한다. form=0 은 기존 번호 할당이다. 지정 form 은 실제 공통 Take/현재 getter와 늦은 부모/PID 진단을 사용한다.
    // 사용: host.attach(new MyProcess(), 12, parentSid, 0x50). (2026-11-08 WB04-D5c)
    attach(process, processType, parent, flags = 0, form = 0) {
        if (!(process instanceof SquidProcess)) throw new TypeError('부착할 프로세스가 없습니다');
        if (this.#kernel.find(process)) return this.reattach(process, processType, parent, flags, form);
        return this.#attachBase(process, processType, parent, flags, form) ? process : null;
    }

    // 같은 Kernel 이 이미 소유한 프로세스의 지정 Form 을 재수신한다(0041bd20/CD 0048f590). live Form 은 기존 부모/PID 를 원문 위치에서 진단한 뒤 onAttached 만 부르고
    // 타입·링크·상태를 그대로 둔다. free 서버 Form 은 기존 PID 로 Take/타입 쓰기/부착을 수행한다. 선행 genus/체인 검사나 생성 실패 rollback 은 추가하지 않는다.
    reattach(process, processType, parent, flags, form) {
        if (!(process instanceof SquidProcess) || !this.#kernel.find(process) || (friend.host(process) && friend.host(process) !== this)) throw new TypeError('재수신 프로세스의 Kernel/host 소유권 불일치');
        return this.#attachBase(process, processType, parent, flags, form) ? process : null;
    }

    // 같은 상위 원문 부모에서 신규/기존 Kernel 등록을 선택한다. 장기 호스트의 현재 장치와 프로세스의 실제 두 필드를 원문 부모에 제공하며 부분 실패를 숨기지 않는다.
    #attachBase(process, processType, parent, flags, form) {
        return this.#withCall((runtime) => {
            friend.setHost(process, this, runtime);
            const hooks = {
                typeEnd: () => this.processTypeEnd,
                processFormType: () => this.#state.processFormType,
                assertion: (expression, file, line) => {
                    const device = this.#assertionDevice();
                    if (typeof device !== 'function') throw new Error('BaseProcess 부착의 선택된 원문 진단 장치 미연결');
                    device(expression, file, line);
                },
                // 원문 Kernel helper 는 슬롯 검색 전에 타입 순환을 전진시킨다.
                registerProcess: () => {
                    this.#state.typeRotation = ((this.#state.typeRotation + 1) >>> 0) % this.#typeList().length;
                    const pid = this.#kernel.add(process);
                    friend.setPid(process, pid);
                    return pid;
                },
                create: (allocationFlags) => this.#createOrTakeForm(allocationFlags === 2 ? CLIENT_FORM : 0, 0),
                take: (selected) => { this.#createOrTakeForm(0, selected); },
                parent: (selected) => commonContainedParent(this.#pool, selected),
                attach: (selected, into, popFlags) => this.#attachForm(selected, into, popFlags),
                onAttached: () => process.onAttached(),
            };
            return attachBaseProcess(this.#pool, friend.fields(process), processType, parent, form, flags, hooks);
        });
    }

    // 00496e80 ↔ CD 0048efd0: RegularProcess 를 붙이고 시각을 현재 시각으로 둔다. 원본 생성자의 flags 는 항상 로컬 전용 0x50 이며 다른 값은 BaseProcess 생성자의 할당/권한 분기 검사에만 쓴다.
    // 사용: host.addRegular(parentSid, event, payload). (2026-11-08 WB04-D5c)
    addRegular(parent, event, payload, flags = LOCAL_MASK) {
        return this.attachRegular(new RegularProcess(event, payload), parent, false, flags);
    }

    // 일반 Regular 와 같은 실행 몸체를 타입 61 로 부착한다. 시각은 부착 후 현재 시각이다(원본 00496f00 / CD 0048f1d0). 네트워크 저장 가상 함수는 별도 미복원이다.
    addSharedRegular(parent, event, payload) {
        return this.attachRegular(new RegularProcess(event, payload), parent, true);
    }

    // 이미 확보한 예약을 현재 생성자 번호로 부착한다. SharedRegular 의 원문 진단(basePostPop == 0, Regular.cpp 0x70) 뒤에도 현재 번호를 다시 읽고,
    // 부착이 돌아온 뒤 현재 시각을 읽어 쓴다(caller 의 추가 계수/기한 쓰기는 유지).
    attachRegular(process, parent, shared = false, flags = LOCAL_MASK) {
        if (shared) {
            this.#withCall(() => {
                const runtime = this.#current ? this.#runtime() : null;
                const depth = runtime ? runtime.postPopDepth : null;
                if (runtime && typeof depth !== 'function') throw new Error('SharedRegular 현재 Pop 깊이 미연결');
                if ((depth ? depth() : this.#state.postPopDepth) !== 0) {
                    const assertion = runtime ? runtime.assertion : this.#state.assertion;
                    if (typeof assertion !== 'function') throw new Error('SharedRegular 원문 진단 장치 미연결');
                    assertion('basePostPop == 0', REGULAR_CPP, SHARED_REGULAR_DIAGNOSTIC_LINE);
                }
            });
        }
        const type = shared ? this.#state.sharedRegularType : this.#state.regularType;
        const attached = this.attach(process, type, parent, flags);
        if (attached) regularFriend.setTime(attached, this.#state.now);
        return attached;
    }

    // 월드 파생 사건을 등록할 때 기존 fallback 을 합성한 실제 처리기를 교체한다. 다음 Regular 호출은 현재 처리기를 읽는다.
    setRegularHandler(handler) {
        if (this.#current) this.#runtime().regular = handler;
        else this.#regular = handler;
    }

    // 미션 runtime 의 실제 직렬화기를 연결한다. 기존 프로세스도 다음 Pop 에서 이 경계를 읽는다.
    setFormTransmit(transmit) {
        if (this.#current) this.#runtime().transmit = transmit;
        else this.#transmit = transmit;
    }

    // 0041c010/CD 0048f8d0: 현재 부모를 읽고 live Form Unpop(0)→같은 부모 Attach(flags 0)을 수행한다. 파생 onAttached 를 다시 부르지 않으며 서버 전송이 필요하면 현재 전송 경계를 쓴다.
    refreshForm(process) {
        this.#withCall(() => {
            if (friend.host(process) !== this || !this.isRegistered(process.pid, process)) throw new Error('Form 갱신 프로세스의 Kernel 소유권 불일치');
            // 부모는 첫 getter 에서 저장하고 Form 필드는 각 가상 진입 전에 다시 읽는다.
            const parent = commonContainedParent(this.#pool, process.form);
            const raw = this.#pool.slot(parent);
            const assertion = (expression, line) => {
                const device = this.#assertionDevice();
                if (typeof device !== 'function') throw new Error('Form 갱신의 선택된 원문 진단 장치 미연결');
                device(expression, BASEPROCESS_CPP, line);
            };
            if (raw[STATE] & DEAD) assertion('!parent.isDead()', 0xb0);
            if (raw[STATE] & FREE) assertion('!parent.isFree()', 0xb1);
            this.#unpopForm(process.form);
            this.#attachForm(process.form, parent, 0);
        });
    }

    // 004ae240: 프로세스에 통지하고 Kernel 에서 제거한 뒤 삭제 깊이를 줄인다. 통지 뒤 Form 의 현재 PID 를 다시 읽고(콜백이 바꾼 번호), 처음 조회한 프로세스 포인터는 다시 읽지 않는다.
    #formPreDestroy(form, flags) {
        this.#withCall(() => {
            let pid = pidOf(this.#pool.slot(form));
            const process = this.#kernel.get(pid);
            if (!(process instanceof SquidProcess)) throw new Error('form의 프로세스가 Kernel에 없습니다');
            process.onFormDestroy(flags);
            pid = pidOf(this.#pool.slot(form));
            this.#kernel.remove(pid);
            this.#destroyer().completePreDestroy();
        });
    }

    // 004ae2b0: 부모가 지워질 때 프로세스에 통지한다. Kernel 에 없으면(원문 +1c NULL 조회) 빈 통지로 생략하고, 다른 Kernel 클래스는 빈 성공으로 숨기지 않는다.
    #formParentDying(form, parent) {
        const current = this.#kernel.get(pidOf(this.#pool.slot(form)));
        if (current === null) return;
        if (!(current instanceof SquidProcess)) throw new Error('현재 Process 부모 통지의 실제 가상+10 미연결');
        current.onParentDying(parent);
    }

    // 0041bf10 ↔ CD 0048f7b0: 권한 조건이 맞으면 form 을 공통 destroy 로 지운다. 프로세스는 form 의 preDestroy 에서 소멸한다.
    kill(process, flags) {
        this.#withCall(() => {
            const form = process.form;
            // 권한이 없고 form 이 서버 번호이며 client flag 도 없으면 아무것도 하지 않는다.
            if (!this.#state.boss && !(form > 4 && form < this.#pool.layout.serverFirst) && !((flags >>> 0) & CLIENT_FORM)) return;
            if (form === 0 || form >= this.#pool.capacity || (this.#pool.slot(form)[STATE] & DEAD)) throw new Error('프로세스 form 상태 오류');
            this.#destroyer().destroy(form, flags >>> 0, this.hooks());
        });
    }

    // 그 번호의 Kernel 슬롯에 지금도 이 프로세스가 등록돼 있는지 본다. 주소(객체 동일성)만 비교하므로 Kill 이 프로세스를 없앴는지 확인할 때 쓴다. 번호와 객체는 Kill 전에 읽어 둔 값을 넘긴다.
    // (cpppj 2026-10-11 추가: 건설 프로세스의 둘째 Kill 판단)
    isRegistered(pid, process) {
        return Boolean(process) && this.#kernel.get(pid) === process;
    }

    // 004ae2d0 / CD 004af4e0: form +0x12 의 현재 DWORD 번호로 Kernel 의 프로세스를 조회한다. 비어 있거나 다른 종류의 BaseProcess 면 null.
    fromForm(form) {
        const process = this.#kernel.get(pidOf(this.#pool.slot(form)));
        return process instanceof SquidProcess ? process : null;
    }

    // PATCH 004ae280/CD 004af490: server 일 때 현재 Form PID 의 실제 notify(2, dependForm, watched)를 호출한다. server 0 이면 PID 조차 읽지 않는다.
    notifyDependency(watcher, watched, dependForm) {
        if (!this.#state.server) return;
        const process = this.fromForm(watcher);
        if (!process) throw new Error('현재 Depend 통지의 실제 ProcessForm/PID 미연결');
        this.#withCall(() => process.notify(2, dependForm, watched));
    }

    // 부모의 종속 체인(가장 최근 부착부터)에서 match 를 만족하는 첫 Regular 를 찾는다. 자산/일반 Form 은 건너뛰고(프로세스 TYPE 범위 10..0x45 이면서 signed 상한 미만) 일반/공유 TYPE 만 후보이다.
    #find(parent, match) {
        const pool = this.#pool;
        if (!parent || parent >= pool.capacity) throw new RangeError('이벤트 조회 SID 범위 오류');
        let current = word(pool.slot(parent), HEAD);
        let guard = 0;
        // 가장 최근에 붙은 종속부터 다음 링크를 따라간다.
        while (current) {
            guard += 1;
            if (current >= pool.capacity || guard > pool.capacity) throw new Error('종속 체인 손상');
            const raw = pool.slot(current);
            const next = word(raw, NEXT);
            const type = raw[TYPE];
            // 원문의 프로세스 finder 는 자산/일반 Form 을 먼저 건너뛴다.
            const process = type >= FIRST_PROCESS_TYPE && type <= LAST_FORM_TYPE && type < (this.processTypeEnd | 0);
            if (process && (type === this.#state.regularType || type === this.#state.sharedRegularType)) {
                // 원문은 현재 예약의 포인터+18 을 읽는다. 미등록/다른 클래스를 빈 검색으로 숨기지 않는다.
                const regular = this.#kernel.get(pidOf(raw));
                if (!(regular instanceof RegularProcess)) throw new Error('현재 Regular 종속의 Kernel PID/실제 클래스 미연결');
                if (match(regular)) return regular;
            }
            current = next;
        }
        return null;
    }

    // 004afb40 ↔ CD 004ac890: 부모의 종속 체인에서 이벤트 번호가 같은 첫 Regular 프로세스를 찾는다. 없으면 null.
    findEvent(parent, event) {
        return this.#find(parent, (process) => process.event === (event >>> 0));
    }

    // 004afbe0 ↔ CD 004ac9c0: (이벤트 & mask) == value 인 첫 Regular 프로세스를 찾는다. 없으면 null.
    findEventMasked(parent, mask, value) {
        return this.#find(parent, (process) => ((process.event & mask) >>> 0) === (value >>> 0));
    }

    // 00496cb0 Regular 실행 몸체이다(RegularProcess.runFrame 이 부른다). 부모 free/dead·void·시각 관문을 지나면 현재 부모 처리기를 부르고 반환값으로 재예약/종료를 정한다.
    // geyser 는 0.5초보다 먼 예약을 현재 시각으로 당기고, 패치판은 abstract 부모(factory·vortex·dais 제외)를 건너뛴다. NaN 시각은 실행하지 않으며 NaN 반환은 재예약이다.
    runRegular(process) {
        const state = this.#state;
        const raw = this.#pool.slot(process.parent);
        if (raw[STATE] & (FREE | DEAD)) return;
        // geyser 는 0.5초보다 먼 예약을 현재 시각으로 당긴다.
        if (raw[TYPE] === state.pollType && process.time - state.now > POLL_LIMIT) regularFriend.setTime(process, state.now);
        // 패치판만 abstract 부모를 건너뛴다(장르 BYTE +40 의 비트 0, 아니면 genus flags2 에 실행 비트가 있어야 한다).
        if ((raw[GENUS] & 1) && !(this.#type(raw[TYPE]).flags2 & ABSTRACT_RUN_GENUS)) return;
        // 아직 시각이 안 됐거나 비교할 수 없으면(NaN) 실행하지 않는다.
        if (!(process.time <= state.now)) return;
        if (raw[STATE] & VOID) return;
        const pid = process.pid;
        this.#withCall(() => {
            const handler = this.#regularHandler();
            const result = Math.fround(handler ? handler(process.parent, process.event, process.count, process.payload) : process.payload);
            // 처리기가 이 프로세스를 지웠으면 더 건드리지 않는다(원본의 해제 표식 검사에 해당).
            if (this.#kernel.get(pid) !== process) return;
            if (result > 0 || Number.isNaN(result)) {
                // 반환값이 다음 payload 와 지연이 된다. 패치판은 NaN 도 재예약으로 취급한다.
                regularFriend.setPayload(process, result);
                regularFriend.setTime(process, result + state.now);
                regularFriend.incrementCount(process);
            } else if (result === 0) this.kill(process, 0);
        });
    }

    // 공통 destroy 에 줄 훅이다. ProcessForm 루트/종속의 가상 pre/post/release/destroy/Unpop 을 처리하고 나머지는 자산으로 넘긴다.
    // pre/post 는 루트가, release/destroy 는 종속이 ProcessForm 일 때만 여기서 처리한다.
    hooks() {
        return {
            // form 은 선택 대상이 아니므로 자산 훅이 없으면 선택 없음(0)으로 답한다.
            selected: () => {
                if (this.#current) return this.#withCall(() => {
                    const asset = this.#assetHooks();
                    return typeof asset?.selected === 'function' ? asset.selected() : 0;
                });
                return typeof this.#asset?.selected === 'function' ? this.#asset.selected() : 0;
            },
            unpopForm: (sid) => this.#unpopForm(sid),
            emit: (event) => {
                this.#withCall(() => {
                    if (this.#isProcessForm(event.sid)) {
                        switch (event.effect) {
                        case SQUID_DESTROY_EFFECT.preDestroy:
                            this.#formPreDestroy(event.sid, event.flags);
                            return;
                        case SQUID_DESTROY_EFFECT.postDestroy:
                            this.#destroyer().completePostDestroy();
                            return;
                        case SQUID_DESTROY_EFFECT.releaseDependent:
                            this.#formParentDying(event.sid, event.parent);
                            return;
                        case SQUID_DESTROY_EFFECT.destroyDependent:
                            this.#destroyer().destroy(event.sid, event.flags, this.hooks());
                            return;
                        default:
                            break;
                        }
                    }
                    const asset = this.#assetHooks();
                    if (typeof asset?.emit !== 'function') throw new Error('프로세스 host의 자산 삭제 훅 미연결');
                    asset.emit(event);
                });
            },
        };
    }
}

// 00421530 ↔ CD 004490b0: 다리 칸에 지연 낙하 이벤트(0x2692)를 예약한다. payload 는 좌표를 포장한 값(float 로 바꾼 signed DWORD)이다.
// 사용: scheduleBridgeFall(host, bridgeSid, x, y). (2026-11-08 WB04-D5c)
export function scheduleBridgeFall(host, bridge, x, y) {
    return host.addRegular(bridge, BRIDGE_FALL_EVENT, RawBridgeLifecycle.delayedFallPayload(x, y));
}

// 00421fe0 ↔ CD 00449170: 그 칸에 예약된 지연 낙하가 있는지 본다. (2026-11-08 WB04-D5c)
export function hasScheduledBridgeFall(host, bridge) {
    return host.findEvent(bridge, BRIDGE_FALL_EVENT) !== null;
}
