// 총(Gun) 프로세스의 실제 0x38바이트 필드·상태 기계·표적 종속·패킷을 현재 Kernel/Form 풀에 연결하는 모듈이다(cpppj `o/RawGunProcess.cpp/.h`, 10.78 전용: 0046a1a0 생성자·0046b830 프레임·0046a290 표적 해제·0046a960 표적 지정·
// 0046a340 통지·0046bd20 발사·0046a4b0 수신 관찰·00468ed0/0046a090 상태 writer·0046a030 수신 기본 생성자·00468f80 수신 읽기·004690f0 부착 직후).
//  - GunProcessFields: 원문 new(0x38)의 `bytes`(+8 dirty·+0x10 다음 시각 double·+0x18 WORD 표적·+0x1c 단계·+0x20 group(signed)·+0x28 inner VT·+0x2c inner Form·+0x30 inner 부모). 생성자가 쓰지 않는 padding 도 입력 그대로 보존한다.
//  - new RawGunProcess(pool, types, globals, hooks): types 는 188칸 TYPE 표(group 만 생성 때 복사한다, C++ vector 복사와 같음), globals = { now(), boss(), server(), fortMode(), fixedMode(), disabled(), friendlyFire(), checking(),
//    windArcherType(), thunderCannonType(), rainCannonType(), alliance(owner, target) } 는 모두 소비 위치마다 현재 값을 읽는 함수이다(boss/server 는 같은 RawAuthorityState 를 빌려도 됨, alliance 는 방향별 DWORD 한 칸으로 0 이면 비동맹).
//    hooks = { removeDependency(form, target), addDependency(target, fields), repopForm(fields), animationForm(parent)→SID, killAnimation(form, flags), refreshRange(parent), targetingMode(parent, owner)→모드,
//    findTarget(x, y, type, mode, owner)→SID, randomBetween(low, high)→double, startAnimation(stage, form, target), validateIslandOwners(parent), checkForms(parent), parentBlocked(parent)→bool,
//    shotContext(parent)→DWORD 8개, shoot(context, target)→bool, firingInterval(parent)→double, attach(fields, parent), assertion(식, 파일, 줄) } 이다.
//    선택하지 않은 경계는 요구하지 않으며 선택된 누락은 앞선 쓰기를 보존한 채 거부한다. 미복원 효과를 빈 함수로 성공시키지 않는다.
//  - initialize(fields, parent): 0046a1a0 앞부분 — 부모 group 이 0/1 이 아니면 assert, 두 VT(0x50b668/0x50b648)·dirty 0xff·Form/부모 0·표적 0·시각 now·단계 0·group 기록. construct 는 attach 훅(실제 flags 0 부착)까지 이어 부른다.
//  - initializeReceived(fields): 0046a030 — 수신 observer 가 읽을 표적/시각/단계와 두 VT 만 초기화한다(group·padding 은 보존).
//  - frame(fields): inner 0046b830 — 부모 group/abstract/astral assert, 정지·고정·비활성이면 무동작, boss 이면 표적 유효성(죽음/동맹)으로 해제, 시각·부모 extra 관문 뒤 단계 0~10(검색 1: 서버 전용 표적 찾기·지연 난수, 회전 2, 발사 대기 5, 발사 7, 종료 8)을 실행한다.
//  - clearTarget(fields, restart)/setTarget(fields, target): 0046a290/0046a960 — 표적 감시 종속 제거·WORD 표적·선택적 단계 1·현재 Form 재전송, 감시 교체·단계 2·현재 AnimationForm 중지(조회를 반복)·사거리/전송 갱신.
//  - static attached(fields): 004690f0 — 단계 3→2, 6→5 만 되돌린다. notify(fields, event, info, value): 0046a340 — 애니메이션 완료(4/7)와 표적 삭제 통지, checking 진단 복귀 뒤 현재 필드를 다시 읽는다.
//  - fire(fields): 0046bd20 — 정지/고정 관문 뒤 유효 표적으로 shot 훅을 부르고 성공이면 단계 2, 아니면 idle 재시작, 어느 쪽이든 현재 시각 + 발사 간격을 기록한다.
//  - receiveUpdate(fields, dirty, beforeRead)/readData(fields, packet, parent): 0046a4b0/00468f80 — 수신 전후 observer(옛 단계 3/6 의 애니메이션 중지)와 dirty 비트별 부분 쓰기(부모 DWORD 는 `parent.value` 상자로 돌려줌).
//  - static writeData/writeProcess: 00468ed0/0046a090 — 헤더·선택 dirty 필드·부모·길이·dirty 0 을 원본 순서로 기록하고 모자란 버퍼는 쓰기 전에 거부한다.
//  - GunProcess: inner Baseprocess 의 실제 Kernel 수명(`SquidProcess` 파생). runFrame·onAttached(Form/부모 반영 뒤 attached)·notify·fields·writePacket. attachGunProcess/createGunProcess 는 실제 flags 0 부착 공장이다.
// 부모 슬롯은 C++ span 처럼 `pool.physical` 살아 있는 보기로 읽어 효과 뒤 현재 값을 본다. 10.72(CD)는 범위 밖이고 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E359: 7-P2 RawGunProcess 이식으로 추가)
import { SquidProcess } from './squid-process.js';

// 원본 outer/inner 필드 위치이다: dirty·시각·WORD 표적·단계·group·inner VT·inner Form·inner 부모. (2026-11-13 WB04-E359)
const DIRTY = 8, TIME = 16, TARGET = 24, STAGE = 28, GROUP = 32, INNER = 40, FORM = 44, PARENT = 48;
// 부모 슬롯 필드 위치(10.78): TYPE·상태 비트·소유자·extra BYTE 이다. (2026-11-13 WB04-E359)
const TYPE_AT = 10, STATE_AT = 11, OWNER_AT = 34, EXTRA_AT = 40;
// 10.78 의 outer/inner 가상 표 주소(기록용)이다. (2026-11-13 WB04-E359)
const OUTER_VTABLE = 0x50b668, INNER_VTABLE = 0x50b648;
// idle 검색의 float 지연 바이어스(0.4f)와 난수 구간 상한(0.2f)이다. (2026-11-13 WB04-E359)
const POLL_BIAS = Math.fround(0.4), POLL_RANDOM = Math.fround(0.2);
// 플레이어 번호 상한(0~8 은 9칸)과 10.78 의 TYPE 표 길이, 총 원문 소스 파일 이름, 총 프로세스 TYPE 의 기본 번호이다. (2026-11-13 WB04-E359)
const PLAYER_COUNT = 8, TYPE_COUNT = 188, GUN_FILE = '\\Ns\\O\\Gunprocess.cpp', GUN_PROCESS_TYPE = 34;

// 비정렬 리틀엔디언 값을 읽는다(부호 없는 수). (2026-11-13 WB04-E359)
function read(raw, at, width = 4) {
    let value = 0;
    // 낮은 바이트부터 지정 폭만 조합한다.
    for (let index = 0; index < width; index += 1) value += raw[at + index] * 2 ** (8 * index);
    return value;
}

// 지정 폭만 리틀엔디언으로 쓴다(주변 padding 은 건드리지 않음). (2026-11-13 WB04-E359)
function put(raw, at, value, width = 4) {
    // 낮은 바이트부터 지정 폭만 쓴다.
    for (let index = 0; index < width; index += 1) raw[at + index] = Math.floor(value / 2 ** (8 * index)) & 0xff;
}

// double/float 비트 변환용 공용 보기이다. (2026-11-13 WB04-E359)
const SCRATCH = new DataView(new ArrayBuffer(8));

// 필드의 raw double 시각을 읽는다. (2026-11-13 WB04-E359)
function readTime(raw) {
    // 여덟 바이트를 그대로 옮겨 IEEE double 로 본다.
    for (let index = 0; index < 8; index += 1) SCRATCH.setUint8(index, raw[TIME + index]);
    return SCRATCH.getFloat64(0, true);
}

// 시각 double 을 두 DWORD 로 쓴다(dirty 는 건드리지 않음). (2026-11-13 WB04-E359)
function putTime(fields, value) {
    SCRATCH.setFloat64(0, value, true);
    put(fields.bytes, TIME, SCRATCH.getUint32(0, true));
    put(fields.bytes, TIME + 4, SCRATCH.getUint32(4, true));
}

// 다음 시각/단계/표적 변경은 해당 원본 dirty 비트만 OR 한다. (2026-11-13 WB04-E359)
function setTime(fields, value) {
    fields.bytes[DIRTY] |= 2;
    putTime(fields, value);
}

// 단계 DWORD 를 쓰고 dirty 비트 8 을 켠다. (2026-11-13 WB04-E359)
function setStage(fields, value) {
    fields.bytes[DIRTY] |= 8;
    put(fields.bytes, STAGE, value);
}

// WORD 표적을 쓰고 dirty 비트 4 를 켠다. (2026-11-13 WB04-E359)
function setTarget(fields, value) {
    fields.bytes[DIRTY] |= 4;
    put(fields.bytes, TARGET, value, 2);
}

// 현재 raw IEEE float 좌표를 읽는다. (2026-11-13 WB04-E359)
function coordinate(raw, at) {
    SCRATCH.setUint32(0, read(raw, at), true);
    return SCRATCH.getFloat32(0, true);
}

// 선택된 외부 경계가 없으면 앞선 쓰기를 보존한 채 거부한다. (2026-11-13 WB04-E359)
function need(hook, message) {
    if (typeof hook !== 'function') throw new Error(message);
    return hook;
}

// 원본 new(0x38)의 미초기화 padding 도 독립 대조에서 보존하도록 raw 필드를 소유한다. (2026-11-13 WB04-E359)
export class GunProcessFields {
    constructor() {
        // 필드 접근이나 파일 읽기에 쓰는 원시 바이트이다.
        this.bytes = new Uint8Array(0x38);
    }

    // +8 의 원본 변경 비트이다.
    get dirty() { return this.bytes[DIRTY]; }

    // +0x10 의 다음 판정 시각(raw double)이다.
    get time() { return readTime(this.bytes); }

    // +0x18 의 WORD 표적 SID 이다.
    get target() { return read(this.bytes, TARGET, 2); }

    // +0x1c 의 단계 DWORD 이다.
    get stage() { return read(this.bytes, STAGE); }

    // +0x20 의 group 이다(signed 32비트).
    get group() { return read(this.bytes, GROUP) | 0; }

    // +0x2c(inner Form) 하위 WORD 이다.
    get form() { return read(this.bytes, FORM) & 0xffff; }

    // +0x30(inner 부모) 하위 WORD 이다.
    get parent() { return read(this.bytes, PARENT) & 0xffff; }
}

// 원본 GunProcess 몸체를 raw 필드와 같은 풀의 자산 상태로 실행한다. (2026-11-13 WB04-E359)
export class RawGunProcess {
    #pool;
    #groups;
    #globals;
    #hooks;

    // PATCH·188칸 TYPE 표를 확인하고 group 만 복사한다. 전역과 훅은 객체보다 오래 살아야 한다. (2026-11-13 WB04-E359)
    constructor(pool, types, globals, hooks = {}) {
        if (pool?.edition !== 'patch1078') throw new TypeError('GunProcess 는 PATCH 만 복원했습니다');
        if (!Array.isArray(types) || types.length !== TYPE_COUNT) throw new RangeError('GunProcess 타입 판본/크기 오류');
        this.#pool = pool;
        this.#groups = types.map((type) => type.group | 0);
        this.#globals = globals;
        this.#hooks = hooks;
    }

    // Host 가 같은 풀인지 확인할 수 있게 공개한다. (2026-11-13 WB04-E359)
    get pool() { return this.#pool; }

    // 부모 슬롯의 살아 있는 보기이다. 풀 밖 SID/TYPE 만 거부하고 free/dead 는 여기서 거부하지 않는다. (2026-11-13 WB04-E359)
    #parentOf(fields) {
        const parent = fields.parent;
        if (!parent || parent >= this.#pool.capacity) throw new RangeError('GunProcess 부모 SID 오류');
        const raw = this.#pool.physical(parent);
        if (raw[TYPE_AT] >= this.#groups.length) throw new RangeError('GunProcess 부모 타입 오류');
        return raw;
    }

    // 현재 동맹 한 칸을 원본 방향대로 읽는다(반대 방향을 자동으로 읽지 않음). (2026-11-13 WB04-E359)
    #isAlly(owner, target) {
        return Boolean(this.#globals.alliance(owner, target));
    }

    // checking 이 켜진 때만 현재 AnimationForm 이 없음을 검사한다. (2026-11-13 WB04-E359)
    #checkNoAnimation(fields) {
        if (this.#globals.checking()) {
            if (!this.#hooks.animationForm || this.#hooks.animationForm(fields.parent)) throw new Error('GunProcess getAnimFormSid==0 원본 assert');
        }
    }

    // 0046a1a0 앞부분: 생성자 필드를 쓰고 host flags 0 부착은 별도 공장이 즉시 수행한다. (2026-11-13 WB04-E359)
    initialize(fields, parent) {
        if (!parent || parent >= this.#pool.capacity) throw new TypeError('GunProcess 생성 부모 SID 오류');
        const raw = this.#pool.physical(parent);
        if (raw[TYPE_AT] >= this.#groups.length) throw new RangeError('GunProcess 부모 타입 오류');
        const group = this.#groups[raw[TYPE_AT]];
        if (group !== 0 && group !== 1) throw new Error('GunProcess 생성 group 원본 assert');
        put(fields.bytes, 0, OUTER_VTABLE);
        put(fields.bytes, INNER, INNER_VTABLE);
        fields.bytes[DIRTY] = 0xff;
        put(fields.bytes, FORM, 0);
        put(fields.bytes, PARENT, 0);
        setTarget(fields, 0);
        setTime(fields, this.#globals.now());
        setStage(fields, 0);
        fields.bytes[DIRTY] |= 0x10;
        put(fields.bytes, GROUP, group >>> 0);
    }

    // 0046a1a0: 필드를 만든 뒤 실제 부착 경계로 넘긴다(경계가 없으면 필드를 바꾸기 전에 거부). (2026-11-13 WB04-E359)
    construct(fields, parent) {
        if (!this.#hooks.attach) throw new TypeError('GunProcess 생성 부착 경계 오류');
        this.initialize(fields, parent);
        this.#hooks.attach(fields, parent);
    }

    // 0046a030: 수신 observer 가 읽을 표적/시각/단계와 두 VT 만 초기화한다. group 과 주변 padding 은 보존한다. (2026-11-13 WB04-E359)
    initializeReceived(fields) {
        put(fields.bytes, 0, OUTER_VTABLE);
        put(fields.bytes, INNER, INNER_VTABLE);
        fields.bytes[DIRTY] = 0xff;
        put(fields.bytes, FORM, 0);
        put(fields.bytes, PARENT, 0);
        put(fields.bytes, TARGET, 0, 2);
        putTime(fields, this.#globals.now());
        put(fields.bytes, STAGE, 0);
    }

    // 004690f0: 새 부착과 같은 Form 재수신은 애니메이션 대기 단계만 되돌린다. (2026-11-13 WB04-E359)
    static attached(fields) {
        if (fields.stage === 3) setStage(fields, 2);
        if (fields.stage === 6) setStage(fields, 5);
    }

    // 0046a290: clear(false)는 단계를 보존하고 clear(true)는 idle 로 돌아간다. 전송 필요성은 변경 전 값으로 정한다. (2026-11-13 WB04-E359)
    clearTarget(fields, restart) {
        const hooks = this.#hooks;
        if (!this.#globals.boss()) return;
        if (!this.#globals.server()) throw new Error('GunProcess clear 서버 원본 assert');
        const old = fields.target;
        const transmit = restart ? (old !== 0 || fields.stage !== 1) : old !== 0;
        // 표적 0/전송 없음의 자연 분기는 실제 외부 호출이 없으므로 미사용 경계를 요구하지 않는다.
        if ((old && !hooks.removeDependency) || (transmit && !hooks.repopForm)) throw new Error('GunProcess clear 실제 효과 경계 누락');
        if (old) hooks.removeDependency(fields.form, old);
        setTarget(fields, 0);
        if (restart) setStage(fields, 1);
        if (transmit) hooks.repopForm(fields);
    }

    // 0046a960: 표적 교체는 공격 모드와 무관한 서버 작업이다. 원본은 현재 AnimationForm 을 반복 조회한다. (2026-11-13 WB04-E359)
    setTarget(fields, target) {
        const hooks = this.#hooks;
        if (!this.#globals.server() || !hooks.removeDependency || !hooks.addDependency || !hooks.animationForm || !hooks.killAnimation || !hooks.refreshRange || !hooks.repopForm) {
            throw new Error('GunProcess 표적 지정 효과 누락');
        }
        if (fields.target) hooks.removeDependency(fields.form, fields.target);
        setTarget(fields, target);
        hooks.addDependency(fields.target, fields);
        setStage(fields, 2);
        // 중지 helper 도 현재 AnimationForm 을 검사한 뒤 다시 조회한다.
        if (hooks.animationForm(fields.parent)) {
            if (hooks.animationForm(fields.parent)) hooks.killAnimation(hooks.animationForm(fields.parent), 0x40);
        }
        hooks.refreshRange(fields.parent);
        hooks.repopForm(fields);
    }

    // 0046b830: 상태 0~10 의 시간·권한·현재 표적·애니메이션 흐름이다. 한 프레임에 한 단계만 진행한다. (2026-11-13 WB04-E359)
    frame(fields) {
        const globals = this.#globals;
        const hooks = this.#hooks;
        const pool = this.#pool;
        let parent = this.#parentOf(fields);
        if (fields.group !== this.#groups[parent[TYPE_AT]] || (parent[EXTRA_AT] & 9)) throw new Error('GunProcess Frame group/abstract/astral 원본 assert');
        if ((parent[EXTRA_AT] & 0x20) || globals.fixedMode() || globals.disabled()) return;
        // 원본 boss 조건이 거짓이면 표적 슬롯/소유자/동맹 조회 자체를 실행하지 않는다.
        if (globals.boss()) {
            const target = fields.target;
            const valid = target !== 0 && target < pool.capacity;
            let clear = !valid;
            if (valid) {
                const raw = pool.physical(target);
                const first = parent[OWNER_AT];
                const second = raw[OWNER_AT];
                if (first > PLAYER_COUNT || second > PLAYER_COUNT) throw new RangeError('GunProcess 동맹 소유자 범위 오류');
                clear = (raw[STATE_AT] & 3) !== 0 || (this.#isAlly(first, second) && !globals.friendlyFire());
            }
            if (clear) this.clearTarget(fields, false);
        }
        parent = this.#parentOf(fields);
        // 10.78 의 x87 분기: unordered(NaN) 시각은 진행하지 않고 반환한다.
        if (!(fields.time <= globals.now())) return;
        if (parent[EXTRA_AT] & 1) return;
        const type = parent[TYPE_AT];
        const stage = fields.stage;
        switch (stage) {
        case 0:
            this.#checkNoAnimation(fields);
            if (globals.checking() && fields.target) throw new Error('GunProcess idle 표적 원본 assert');
            break;
        case 1:
            if (globals.checking() && fields.target) throw new Error('GunProcess 검색 표적 원본 assert');
            if (globals.server() && (!globals.fortMode() || globals.friendlyFire())) {
                if (!hooks.targetingMode || !hooks.findTarget || !hooks.randomBetween) throw new Error('GunProcess 검색 경계 누락');
                const mode = hooks.targetingMode(fields.parent, parent[OWNER_AT]);
                const found = hooks.findTarget(coordinate(parent, 14), coordinate(parent, 18), type, mode, parent[OWNER_AT]);
                if (found) this.setTarget(fields, found);
                else {
                    setTime(fields, globals.now());
                    const random = hooks.randomBetween(0, POLL_RANDOM);
                    putTime(fields, (random + POLL_BIAS) + globals.now());
                }
            }
            if (globals.checking()) {
                need(hooks.validateIslandOwners, 'GunProcess debug 지면 진단 누락')(fields.parent);
            }
            return;
        case 2:
            this.#checkNoAnimation(fields);
            if (fields.target) {
                const group = this.#groups[this.#parentOf(fields)[TYPE_AT]];
                if ((group !== 0 || type === globals.windArcherType()) && type !== globals.thunderCannonType() && type !== globals.rainCannonType()) {
                    need(hooks.startAnimation, 'GunProcess 회전 애니메이션 경계 누락')(stage, fields.form, fields.target);
                    setStage(fields, 3);
                } else setStage(fields, 4);
                return;
            }
            break;
        case 3: case 6: case 9:
            return;
        case 4:
            this.#checkNoAnimation(fields);
            fields.bytes[DIRTY] |= 8;
            if (fields.target) { put(fields.bytes, STAGE, 5); return; }
            put(fields.bytes, STAGE, 1);
            return;
        case 5:
            if (fields.target) {
                this.#checkNoAnimation(fields);
                need(hooks.startAnimation, 'GunProcess 발사 애니메이션 경계 누락')(stage, fields.form, fields.target);
                setStage(fields, 6);
                return;
            }
            break;
        case 7:
            this.#checkNoAnimation(fields);
            this.fire(fields);
            return;
        case 8:
            need(hooks.startAnimation, 'GunProcess 종료 애니메이션 경계 누락')(8, fields.form, 0);
            setStage(fields, 9);
            return;
        case 10:
            break;
        default:
            throw new Error('GunProcess 단계 원본 assert');
        }
        setStage(fields, 1);
    }

    // 0046a340: 애니메이션 종료 메시지와 감시 표적 삭제 메시지를 원본 단계/권한 조건으로 처리한다. 진단이 복귀하면 현재 필드로 계속한다. (2026-11-13 WB04-E359)
    notify(fields, event, info, value) {
        const globals = this.#globals;
        const hooks = this.#hooks;
        // 선택된 진단만 요구하며 복귀 뒤 현재 필드를 다시 읽는다.
        const assertion = (expression, line) => {
            if (!hooks.assertion) throw new Error('GunProcess 통지 원문 진단 장치 미연결');
            hooks.assertion(expression, GUN_FILE, line);
        };
        if (event === 1) {
            if (value === 4 || (value === 7 && fields.stage === 6)) {
                if (globals.checking()) {
                    if (!hooks.animationForm) throw new Error('GunProcess 통지 animation getter 미연결');
                    if (!hooks.animationForm(fields.parent)) assertion('getAnimFormSid() != 0', value === 4 ? 0x154 : 0x160);
                }
                setStage(fields, value);
                return;
            }
            if (value !== 7 && value !== 10) {
                assertion('false', 0x16a);
                return;
            }
        } else if (globals.server() && event === 2) {
            if (globals.checking() && value !== fields.target) assertion('info2 == getTargetSid()', 0x16e);
            if (value === fields.target) {
                this.clearTarget(fields, true);
                if (!(this.#parentOf(fields)[STATE_AT] & 2)) need(hooks.repopForm, 'GunProcess notify 재전송 누락')(fields);
            }
        }
    }

    // 0046bd20: 실제 shot 판정/생성이 성공하면 단계 2, 실패하면 idle 재시작이며 두 경우 현재 시각 cooldown 은 기록한다. (2026-11-13 WB04-E359)
    fire(fields) {
        const globals = this.#globals;
        const hooks = this.#hooks;
        this.#parentOf(fields);
        if (!hooks.parentBlocked || !hooks.shotContext || !hooks.shoot || !hooks.firingInterval) throw new Error('GunProcess 실제 발사 경계 누락');
        if (globals.checking()) need(hooks.checkForms, 'GunProcess 발사 Form 진단 누락')(fields.parent);
        if (hooks.parentBlocked(fields.parent) || globals.fixedMode()) return;
        const context = hooks.shotContext(fields.parent);
        const target = fields.target;
        const valid = target !== 0 && target < this.#pool.capacity && !(this.#pool.physical(target)[STATE_AT] & 3);
        if (valid && hooks.shoot(context, target)) setStage(fields, 2);
        else this.clearTarget(fields, true);
        const interval = hooks.firingInterval(fields.parent);
        setTime(fields, globals.now() + interval);
        if (globals.checking()) hooks.checkForms(fields.parent);
    }

    // 0046a4b0: 수신 전 phase/dirty 8·옛 단계 3/6 의 현재 AnimationProcess 를 중지한다(중지 helper 의 조회 두 번을 보존). (2026-11-13 WB04-E359)
    receiveUpdate(fields, dirty, beforeRead) {
        if (!beforeRead || !(dirty & 8) || (fields.stage !== 3 && fields.stage !== 6)) return;
        const hooks = this.#hooks;
        if (!hooks.animationForm || !hooks.killAnimation) throw new Error('GunProcess 수신 animation 중지 경계 누락');
        if (hooks.animationForm(fields.parent)) hooks.killAnimation(hooks.animationForm(fields.parent), 0x40);
    }

    // 00468f80: before/after observer 와 dirty 별 부분 쓰기이다. 원문은 dirty 를 필드에 복사하거나 소비하지 않는다. 부모는 별도 DWORD 상자(`parent.value`)로 돌려준다. (2026-11-13 WB04-E359)
    readData(fields, packet, parent) {
        parent.value = 0;
        if (packet.length < 9) throw new RangeError('총 수신의 9BYTE 머리 부족');
        const dirty = packet[8];
        let offset = 9;
        this.receiveUpdate(fields, dirty, 1);
        // 앞선 observer/필드 효과는 되돌리지 않고 선택된 필드 폭만 검사한다.
        const require = (width) => {
            if (offset > packet.length || width > packet.length - offset) throw new RangeError('총 수신의 선택된 dirty 필드 부족');
        };
        // 선택 필드만 원래 비트로 저장한다.
        const copy = (destination, width) => {
            require(width);
            fields.bytes.set(packet.subarray(offset, offset + width), destination);
            offset += width;
        };
        if (dirty & 1) {
            require(4);
            parent.value = read(packet, offset);
            offset += 4;
        }
        if (dirty & 2) copy(TIME, 8);
        if (dirty & 4) copy(TARGET, 2);
        if (dirty & 8) copy(STAGE, 4);
        if (dirty & 0x10) copy(GROUP, 4);
        this.receiveUpdate(fields, dirty, 0);
    }

    // 00468ed0: 헤더·선택 dirty 필드·부모·길이·dirty 0 을 원본 순서로 기록한다. 쓰기 폭/dirty 소비를 유지하며 부족한 버퍼는 변경 전에 거부한다. (2026-11-13 WB04-E359)
    static writeData(fields, packet, form, parent, force, protocol, message) {
        const dirty = force ? 0xff : fields.dirty;
        const length = 9 + (parent ? 4 : 0) + ((dirty & 2) ? 8 : 0) + ((dirty & 4) ? 2 : 0) + ((dirty & 8) ? 4 : 0) + ((dirty & 0x10) ? 4 : 0);
        if (packet.length < length) throw new RangeError('GunProcess 패킷 버퍼 크기 오류');
        if (force) fields.bytes[DIRTY] = 0xff;
        packet[0] = protocol;
        packet[1] = message;
        put(packet, 4, form);
        let cursor = 9;
        if (parent) {
            fields.bytes[DIRTY] |= 1;
            put(packet, cursor, parent);
            cursor += 4;
        }
        // 현재 dirty 가 선택한 8/2/4/4바이트 raw 필드를 순서대로 복사한다.
        for (const [bit, offset, width] of [[2, TIME, 8], [4, TARGET, 2], [8, STAGE, 4], [16, GROUP, 4]]) {
            if (fields.dirty & bit) {
                packet.set(fields.bytes.subarray(offset, offset + width), cursor);
                cursor += width;
            }
        }
        put(packet, 2, cursor, 2);
        packet[8] = fields.dirty;
        fields.bytes[DIRTY] = 0;
        return cursor;
    }

    // 0046a090: includeParent 또는 force 일 때 현재 inner 부모 DWORD 를 쓰는 가상 writer 이다. (2026-11-13 WB04-E359)
    static writeProcess(fields, packet, includeParent, force = false, protocol = 1, message = 0x3a) {
        return RawGunProcess.writeData(fields, packet, read(fields.bytes, FORM), (includeParent || force) ? read(fields.bytes, PARENT) : 0, force, protocol, message);
    }
}

// 실제 Kernel 의 inner Baseprocess 를 수명/가상 메서드로 표현한다. 원본 ctor 필드 초기화와 flags 0 부착은 별도 공장이 끝낸다. (2026-11-13 WB04-E359)
export class GunProcess extends SquidProcess {
    #logic;
    #fields;

    // 원본 raw 필드의 소유권을 실제 Kernel 객체에 넘긴다. (2026-11-13 WB04-E359)
    constructor(logic, fields) {
        super();
        this.#logic = logic;
        this.#fields = fields;
    }

    // 실제 Kernel 한 프레임을 원본 상태 기계로 넘긴다. (2026-11-13 WB04-E359)
    runFrame() { this.#logic.frame(this.#fields); }

    // 부착 뒤 현재 Form/부모를 기록하고 원본 attached 를 실행한다. (2026-11-13 WB04-E359)
    onAttached() {
        put(this.#fields.bytes, FORM, this.form);
        put(this.#fields.bytes, PARENT, this.parent);
        RawGunProcess.attached(this.#fields);
    }

    // 현재 상태 패킷 및 외부 통지에 쓰는 같은 raw 필드이다. (2026-11-13 WB04-E359)
    get fields() { return this.#fields; }

    // inner 가상 통지를 같은 raw 상태 기계에 전달한다. (2026-11-13 WB04-E359)
    notify(event, info, value) { this.#logic.notify(this.#fields, event, info, value); }

    // 부착 직후 writer 는 OnAttached 전에 불리므로 현재 base Form/부모를 먼저 필드에 동기화한다. (2026-11-13 WB04-E359)
    writePacket(packet, parentChanged, force = false, protocol = 1, message = 0x3a) {
        put(this.#fields.bytes, FORM, this.form);
        put(this.#fields.bytes, PARENT, this.parent);
        return RawGunProcess.writeProcess(this.#fields, packet, parentChanged, force, protocol, message);
    }
}

// 원본 flags 0 을 유지한 실제 Kernel 공장이다. 필요한 서버 전송을 host 가 연결하지 않았으면 기존 예외를 그대로 전달한다. (2026-11-13 WB04-E359)
export function attachGunProcess(host, logic, fields, parent, type = GUN_PROCESS_TYPE) {
    if (host.pool !== logic.pool) throw new TypeError('GunProcess/Host SID 풀 불일치');
    const attached = host.attach(new GunProcess(logic, fields), type, parent, 0);
    return attached instanceof GunProcess ? attached : null;
}

// RawAssetPostPop 의 createGunProcess 에 쓸 생성자 + 실제 Kernel/전송 공장이다. 0x38 padding 은 0 으로 확보한다. (2026-11-13 WB04-E359)
export function createGunProcess(host, logic, parent, type = GUN_PROCESS_TYPE) {
    if (host.pool !== logic.pool) throw new TypeError('GunProcess/Host SID 풀 불일치');
    const fields = new GunProcessFields();
    logic.initialize(fields, parent);
    return attachGunProcess(host, logic, fields, parent, type);
}
