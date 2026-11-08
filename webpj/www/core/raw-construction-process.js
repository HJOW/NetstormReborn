// 건설 프로세스(원본 0x38바이트 객체, 프로세스 타입 30)의 시작·생성자·진행·완료·form 삭제 통지를 옮긴 모듈이다(cpppj `o/RawConstructionProcess.cpp/.h`, 10.78 전용).
// 건설 중인 abstract 객체의 HP 를 시작~끝 시각의 비율로 올리고, 다 찼을 때 실제 객체로 바꾼다. 원본: 00443fe0·00443c40·004428c0·00443e20·00444030.
//  - RawConstructionProcess.start(parent, duration): 객체 확보 → 표시 효과(지속 0 이면 즉시 건설) → 시작/끝 시각(효과 뒤의 현재 시각; 지속 0 은 10초) → 부착(host 가 있으면 실제 ConstructionProcess 를 붙임)
//    → 지속 시간이 1초보다 크면(실수 비교, NaN 은 소리 없음) 진행음 jimBuild.wav → 갱신 요청.
//  - run(fields, parent, kill, currentForm, currentParent): 한 프레임의 진행. 선택된 checking 에서는 현재 Form 조회와 연결 진단을 먼저 한 뒤, 최대 HP 경계 → HP = ftol((현재-시작)/(끝-시작)×최대) 를 0~최대로 제한해 부모에
//    쓰고 표시 갱신 → 부모가 abstract 가 아니면 Kill(프로세스가 사라지면 종료) → HP 가 최대이고 권한이 있으면 Kill. quickBuild(또는 vortex 의 quickVortex)이면 바로 최대이다.
//  - complete(parent): 진행음 프로세스 삭제 → 갱신 → (로컬 소유·예측 번호 아님) 완료음과 타입 준비 소리 → (권한·예측 번호 아님·HP 가 최대·abstract) Unpop → abstract 해제 → 재등록(flags 9) → genus 가 맞으면 유닛 밀어내기.
//  - formDestroyed(parent): complete 뒤 부모가 free/dead 가 아니고 dais 타입이 아니면 HP 를 최대로 채운다. resetEnd/resetEndFromForm: 끝 시각을 현재+10초로 바꾸고 변경 비트 4 만 켠다.
//  - ConstructionProcess: Kernel 이 소유하는 실제 건설 프로세스(runFrame 이 run, onFormDestroy 가 formDestroyed 를 부른다).
//  - makeConstructionProcessHooks / makeConstructionProcessNoticeHooks: 최대 HP 를 같은 풀의 보상 모듈에, 유닛 밀어내기를 밀어내기 모듈에, 건설 배치 통지의 시작을 start 에 잇는다.
// 경계 호출은 같은 풀·전역을 바꿀 수 있으며 경계 뒤의 현재 값을 다시 읽는다. 원본이 해제된 객체로 둘째 Kill 을 부르는 동작은 옮기지 않는다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E132: RawConstructionProcess 이식으로 추가)
import { SquidProcess } from './squid-process.js';
import { setRawSquidHitPoints } from './raw-squid-word.js';

// raw 타입·상태·좌표·HP 위치와 10.78 의 소유자/extra(abstract 비트) 위치이다. (2026-11-08 WB04-E132)
const TYPE = 10;
const STATE = 11;
const X = 14;
const Y = 18;
const HP = 26;
const OWNER = 34;
const EXTRA = 40;

// 상태 비트 free·dead·void 와 extra 의 abstract 비트이다. (2026-11-08 WB04-E132)
const FREE = 1;
const DEAD = 2;
const VOID = 4;
const ABSTRACT = 1;

// 지속 시간 0 의 기본 건설 시간과 진행음을 붙이는 경계(초)이다. (2026-11-08 WB04-E132)
const DEFAULT_SECONDS = 10;
const SOUND_SECONDS = 1;

// 부착 flags(client 번호 form), 소리 프로세스 flags(부모 위치 반복 재생), 완성 뒤 재등록 flags 이다. (2026-11-08 WB04-E132)
const ATTACH_FLAGS = 0x10;
const SOUND_FLAGS = 1;
const REPOP_FLAGS = 9;

// quickVortex 가 적용되는 genus(vortex), 예측 영역의 첫 번호, 원본 소리 이름, checking 진단의 원문이다. (2026-11-08 WB04-E132)
const VORTEX_GENUS = 0x200;
const FIRST_SID = 5;
const BUILD_SOUND = 'jimBuild.wav';
const DONE_SOUND = 'buildDone.wav';
const FORM_DIAGNOSTIC = 'getForm().isRobust()';
const CONSTRUCTION_CPP = '\\Ns\\O\\Construction.cpp';
const FORM_DIAGNOSTIC_LINE = 0x1c4;

// 타입 표 길이(10.78)와 ftol 의 64비트 한계이다. (2026-11-08 WB04-E132)
const TYPE_COUNT = 188;
const INT64_LIMIT = 9223372036854775808;

// 필수 경계 이름이다(attach 는 host 가 없을 때만 필수). (2026-11-08 WB04-E132)
const HOOK_NAMES = ['reserve', 'effect', 'loadSound', 'reserveSound', 'attachSound', 'refresh', 'maxHitPoints', 'updateDisplay', 'removeContained',
    'playAt', 'readySound', 'play', 'unpop', 'repop', 'push'];

// 건설 프로세스 한 개가 가진 값이다(원본 객체 +8 dirty·+0x10 start·+0x18 end·+0x20 additional·+0x34 sound). 사용: const fields = createConstructionProcessFields(). dirty 는 생성자가 0xff 로 둔다. (2026-11-08 WB04-E132)
export function createConstructionProcessFields() {
    return { dirty: 0xff, start: 0, end: 0, sound: 0, additional: 0 };
}

// 건설 프로세스가 경계 호출 뒤에도 다시 읽는 원본 전역이다. now(게임 시각 double)·processType(30)·soundProcessType(47)·localPlayer·boss·pushGenus·daisType(165)·
// quickBuild/quickVortex/checking(현재 DWORD 비영 뷰: 숫자·bool 또는 현재 값을 돌려주는 함수)·sharedBoss(실제 Client boss 를 빌리는 함수)·currentProcessType(실제 현재 타입을 빌리는 함수)이다.
// 사용: 월드마다 하나를 두고 RawConstructionProcess 에 넘긴다. 프로세스가 살아 있는 동안 유지해야 한다. (2026-11-08 WB04-E132)
export class ConstructionProcessState {
    now = 0;
    processType = 30;
    soundProcessType = 47;
    localPlayer = 1;
    boss = true;
    pushGenus = 0x10444200;
    daisType = 165;
    quickBuild = 0;
    quickVortex = 0;
    checking = 0;
    sharedBoss = null;
    currentProcessType = null;

    // 이름의 진단 뷰를 현재 DWORD 로 읽는다(함수면 호출, bool 은 0/1). 소비 위치마다 부른다. (2026-11-08 WB04-E132)
    read(name) {
        const value = typeof this[name] === 'function' ? this[name]() : this[name];
        return typeof value === 'boolean' ? (value ? 1 : 0) : value >>> 0;
    }

    // 권한 쪽에서만 건설을 끝낸다. 빌린 Client boss 가 있으면 그 현재 값을 읽는다. (2026-11-08 WB04-E132)
    isBoss() {
        return this.sharedBoss ? Boolean(this.sharedBoss()) : this.boss;
    }

    // 부착할 건설 프로세스 타입 번호이다. 장기 대여가 없을 때만 독립 입력의 30 을 쓴다. (2026-11-08 WB04-E132)
    currentType() {
        return this.currentProcessType ? this.currentProcessType() >>> 0 : this.processType;
    }
}

// CRT ftol 의 하위 signed DWORD 이다. 비유한 값과 64비트 범위 밖은 하위 DWORD 가 0 이다. (2026-11-08 WB04-E132)
function ftol(value) {
    if (!Number.isFinite(value) || Math.abs(value) >= INT64_LIMIT) return 0;
    const truncated = Math.trunc(value);
    if (Math.abs(truncated) <= 2147483647) return truncated | 0;
    return Number(BigInt.asIntN(32, BigInt(truncated)));
}

// 정렬과 무관한 리틀엔디언 DWORD 를 읽는다. (2026-11-08 WB04-E132)
function readUnsigned(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// raw 의 단정도 float 를 읽는다. (2026-11-08 WB04-E132)
function readFloat(raw, offset) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, readUnsigned(raw, offset), true);
    return view.getFloat32(0, true);
}

// 건설 프로세스 몸체이다. 프로세스 객체 없이도 부를 수 있게 필드와 부모 SID 를 인자로 받는다.
// 사용: const process = new RawConstructionProcess(pool, types, state, hooks, host); process.start(parentSid, 8). host 를 주면 start 가 실제 ConstructionProcess 를 Kernel 에 붙이고,
// host 없이 attach 경계만 주면 호출자가 필드를 보관한다. 풀·타입·상태·host 는 이 객체보다 오래 살아야 한다(types 는 사본이 아니라 현재 표를 읽는다). (2026-11-08 WB04-E132)
export class RawConstructionProcess {
    #pool;
    #types;
    #state;
    #hooks;
    #host;

    // 미연결 효과는 첫 확보/쓰기 전에 거부한다. host 가 있으면 attach 경계는 쓰지 않으므로 비어 있어도 된다. (2026-11-08 WB04-E132)
    constructor(pool, types, state, hooks, host = null) {
        if (pool?.edition !== 'patch1078') throw new TypeError('건설 프로세스는 10.78 SidPool 만 받습니다.');
        if (!Array.isArray(types) || types.length !== TYPE_COUNT || (host && host.pool !== pool)) throw new TypeError('건설 프로세스 자료/효과 연결 오류');
        for (const name of HOOK_NAMES) {
            if (typeof hooks?.[name] !== 'function') throw new TypeError('건설 프로세스 자료/효과 연결 오류');
        }
        if (typeof hooks.attach !== 'function' && !host) throw new TypeError('건설 프로세스 자료/효과 연결 오류');
        this.#pool = pool;
        this.#types = types;
        this.#state = state;
        this.#hooks = { ...hooks };
        this.#host = host;
    }

    // 동일 풀 연결 검사용 참조이다. (2026-11-08 WB04-E132)
    get pool() { return this.#pool; }

    // 부모의 현재 raw 타입 번호를 읽고 타입 표 범위를 확인한다. (2026-11-08 WB04-E132)
    #typeOf(sid) {
        const type = this.#pool.slot(sid)[TYPE];
        if (type >= this.#types.length) throw new RangeError('건설 프로세스 부모 타입 오류');
        return type;
    }

    // 부모의 현재 타입에서 genus(flags2)를 읽는다. 경계 호출 뒤의 재조회를 위해 값을 보관하지 않는다. (2026-11-08 WB04-E132)
    #genus(sid) {
        return this.#types[this.#typeOf(sid)].flags2 >>> 0;
    }

    // 현재 HP 를 DWORD 로 읽는다. (2026-11-08 WB04-E132)
    #hitPoints(sid) {
        return readUnsigned(this.#pool.slot(sid), HP) | 0;
    }

    // 번호가 클라이언트 예측 영역(5 이상, 서버 영역 시작 미만) 밖인지 본다. 완료 효과는 예측 객체에서 내지 않는다. (2026-11-08 WB04-E132)
    #confirmed(sid) {
        return sid < FIRST_SID || sid >= this.#pool.layout.serverFirst;
    }

    // 건설을 시작한다. 객체 확보에 실패하면 아무 효과도 없다. duration 이 0(또는 -0)이면 10초 건설이 된다. 원본: 00443fe0 → 00443c40. (2026-11-08 WB04-E132)
    start(parent, duration) {
        if (!this.#hooks.reserve()) return;
        this.#construct(parent, duration);
    }

    // 최대 HP 조회 뒤의 현재 시각을 사용한다. 시작·소리 필드나 부모 HP 를 덮어쓰지 않고 끝 시각만 현재+10초로 바꾸며 변경 비트 4 만 켠다. 원본: 00443dc0. (2026-11-08 WB04-E132)
    resetEnd(fields, parent, damage) {
        void damage;
        this.#hooks.maxHitPoints(parent);
        fields.end = this.#state.now + DEFAULT_SECONDS;
        fields.dirty |= 4;
    }

    // 실제 host 의 form→Kernel 조회로 건설 프로세스를 찾아 resetEnd 를 실행한다. 빈 슬롯·다른 프로세스·오래된 form 연결은 필드 쓰기 전에 거부한다. (2026-11-08 WB04-E132)
    resetEndFromForm(form, damage) {
        if (!this.#host) throw new Error('건설 끝 시각 변경에 프로세스 host가 필요합니다');
        const process = this.#host.fromForm(form);
        if (!(process instanceof ConstructionProcess) || process.form !== form) throw new TypeError('건설 끝 시각 변경의 form 연결 오류');
        this.resetEnd(process.fields, process.parent, damage);
    }

    // 표시 효과 뒤의 현재 시각으로 시작/끝을 정하고, 부착 뒤에 진행음을 붙인다. (2026-11-08 WB04-E132)
    #construct(parent, duration) {
        const state = this.#state;
        const fields = createConstructionProcessFields();
        // 패치판의 실수 비교는 0/-0 에서만 참이다(NaN 은 거짓).
        const instant = duration === 0;
        this.#hooks.effect(parent, instant);
        fields.start = state.now;
        fields.dirty |= 2;
        if (instant) {
            fields.end = state.now + DEFAULT_SECONDS;
            fields.dirty |= 4;
            duration = DEFAULT_SECONDS;
        }
        fields.end = duration + state.now;
        fields.dirty |= 4;
        let attached = null;
        if (this.#host) {
            // 부모가 free/dead 이면 붙지 않는다. 원본은 그 객체를 해제하지 않고 남은 쓰기를 계속한다.
            const process = this.#host.attach(new ConstructionProcess(this, fields), state.currentType(), parent, ATTACH_FLAGS);
            if (process) attached = process.fields;
        } else attached = this.#hooks.attach(fields, state.currentType(), parent, ATTACH_FLAGS) ?? null;
        const live = attached ?? fields;
        if (duration > SOUND_SECONDS) {
            live.sound = this.#hooks.loadSound(BUILD_SOUND) >>> 0;
            // 확보 경계 뒤의 현재 소리 필드를 다시 읽어 넘긴다.
            if (this.#hooks.reserveSound()) this.#hooks.attachSound(parent, live.sound, 0, SOUND_FLAGS);
        } else live.sound = 0;
        this.#hooks.refresh();
    }

    // 최대 HP 경계 뒤의 현재 시각으로 비율을 계산하고, 표시 경계 뒤의 abstract 비트와 Kill 뒤의 권한을 다시 읽는다.
    // kill(flags)은 호출 뒤에도 프로세스가 남아 있으면 참을 돌려준다. currentForm/currentParent 는 선택된 checking 과 부모 조회 때만 쓴다. 원본: 004428c0. (2026-11-08 WB04-E132)
    run(fields, parent, kill, currentForm = null, currentParent = null) {
        const state = this.#state;
        if (state.read('checking')) {
            if (!currentForm) throw new Error('건설 현재 Form 조회 장치 미연결');
            const form = currentForm();
            if (typeof this.#hooks.formRobust !== 'function') throw new Error('건설 현재 Form 연결 진단 장치 미연결');
            if (!this.#hooks.formRobust(form)) {
                if (typeof this.#hooks.assertion !== 'function') throw new Error('건설 Form 원문 진단 미연결');
                this.#hooks.assertion(FORM_DIAGNOSTIC, CONSTRUCTION_CPP, FORM_DIAGNOSTIC_LINE);
            }
        }
        if (currentParent) parent = currentParent();
        const maximum = this.#hooks.maxHitPoints(parent) | 0;
        // x87 순서: (현재-시작)/(끝-시작)에 정수 최대 HP 를 곱한 뒤 0 방향으로 절삭한다.
        let value = ftol(((state.now - fields.start) / (fields.end - fields.start)) * maximum);
        if (state.read('quickBuild') || (state.read('quickVortex') && (this.#genus(parent) & VORTEX_GENUS))) value = maximum;
        if (value > maximum) value = maximum;
        if (value < 0) value = 0;
        setRawSquidHitPoints(this.#pool, parent, value);
        this.#hooks.updateDisplay(parent);
        if (!(this.#pool.slot(parent)[EXTRA] & ABSTRACT) && !kill(0)) return;
        if (value === maximum && state.isBoss()) kill(0);
    }

    // 건설을 끝낸다. 각 경계 뒤에 부모의 현재 HP·소유자·좌표·타입과 전역을 다시 읽는다. 최대 HP 와 HP 는 처음 읽은 값을 끝까지 쓴다. 원본: 00443e20. (2026-11-08 WB04-E132)
    complete(parent) {
        const state = this.#state;
        const pool = this.#pool;
        const maximum = this.#hooks.maxHitPoints(parent) | 0;
        const hitPoints = this.#hitPoints(parent);
        this.#hooks.removeContained(parent, state.soundProcessType);
        this.#hooks.refresh();
        if (pool.slot(parent)[OWNER] === state.localPlayer && this.#confirmed(parent)) {
            const raw = pool.slot(parent);
            this.#hooks.playAt(readFloat(raw, X), readFloat(raw, Y), DONE_SOUND, 0);
            const ready = this.#hooks.readySound(this.#typeOf(parent));
            if (ready) this.#hooks.play(ready);
        }
        if (!state.isBoss() || !this.#confirmed(parent) || hitPoints !== maximum || !(pool.slot(parent)[EXTRA] & ABSTRACT)) return;
        this.#hooks.unpop(parent, 0);
        // 0042b050 인라인: void 가 아니면 원본 assert 이다. abstract 비트만 끄고 다른 비트는 보존한다.
        if (!(pool.slot(parent)[STATE] & VOID)) throw new Error('건설 완료의 abstract 해제는 void 상태여야 합니다');
        pool.allocated(parent)[EXTRA] &= ~ABSTRACT & 0xff;
        this.#hooks.repop(parent, REPOP_FLAGS);
        if (!(state.pushGenus & this.#genus(parent))) return;
        const raw = pool.slot(parent);
        this.#hooks.push(readFloat(raw, X), readFloat(raw, Y), raw[TYPE], raw[OWNER]);
    }

    // form 이 지워지기 직전의 통지다. complete 뒤 부모가 free/dead 가 아니고 dais 타입이 아니면 HP 를 최대로 채운다. 원본: 00444030. (2026-11-08 WB04-E132)
    formDestroyed(parent) {
        this.complete(parent);
        const raw = this.#pool.slot(parent);
        if ((raw[STATE] & (FREE | DEAD)) || raw[TYPE] === this.#state.daisType) return;
        setRawSquidHitPoints(this.#pool, parent, this.#hooks.maxHitPoints(parent) | 0);
    }
}

// Kernel 이 소유하는 실제 건설 프로세스이다(원본 객체 0x38바이트의 BaseProcess 부분). 매 프레임 진행을 실행하고 form 삭제 통지에서 건설을 끝낸다.
// 사용: 직접 만들지 않는다. host 를 준 RawConstructionProcess.start 가 만들어 부모에 붙인다. logic 은 이 프로세스보다 오래 살아야 한다. (2026-11-08 WB04-E132)
export class ConstructionProcess extends SquidProcess {
    #logic;
    #fields;

    // 생성자가 계산한 필드를 복사해 보관한다. (2026-11-08 WB04-E132)
    constructor(logic, fields) {
        super();
        this.#logic = logic;
        this.#fields = { ...fields };
    }

    // 현재 필드이다. start 는 부착 뒤 소리 항목을 여기에 쓴다. (2026-11-08 WB04-E132)
    get fields() { return this.#fields; }

    // 한 프레임 진행이다. Kill 뒤에는 미리 읽어 둔 host·번호·주소만으로 등록 여부를 묻는다. 부착 전에는 Kernel 에 없으므로 실행되지 않는다. (2026-11-08 WB04-E132)
    runFrame() {
        const host = this.host;
        if (!host) return;
        const pid = this.pid;
        this.#logic.run(this.#fields, 0, (flags) => { host.kill(this, flags); return host.isRegistered(pid, this); }, () => this.form, () => this.parent);
    }

    // form 삭제 통지이다. 원본 flags 인자는 읽지 않는다. (2026-11-08 WB04-E132)
    onFormDestroy() {
        this.#logic.formDestroyed(this.parent);
    }
}

// 최대 HP 조회를 같은 풀의 실제 보상 모듈로, 유닛 밀어내기를 밀어내기 모듈(displace)로 잇는다(effects 가 null 이면 hooks.push 유지). 밀어내기의 반환값(적대 거부)은 원본처럼 버린다.
// 사용: 나머지 경계를 채운 hooks 를 넘긴다. reward·effects 는 돌려준 hooks 보다 오래 살아야 한다. 풀이 다르면 거부한다. (2026-11-08 WB04-E132)
export function makeConstructionProcessHooks(pool, reward, effects, hooks) {
    if (pool !== reward?.pool || (effects && pool !== effects.pool)) throw new TypeError('건설 프로세스 어댑터의 풀 연결 오류');
    const connected = { ...hooks, maxHitPoints: (sid) => reward.maxHitPoints(sid) };
    if (effects) connected.push = (x, y, type, owner) => { effects.displace(x, y, type, owner); };
    return connected;
}

// 건설 배치 통지의 건설 시작 경계를 실제 start 에 잇는다. 둘째 인자는 지속 시간의 단정도 비트이다(통지는 0 을 넘긴다). 풀이 다르면 거부한다. (2026-11-08 WB04-E132)
export function makeConstructionProcessNoticeHooks(pool, process, hooks) {
    if (pool !== process.pool) throw new TypeError('통지 건설 시작 어댑터의 풀 연결 오류');
    const view = new DataView(new ArrayBuffer(4));
    return { ...hooks, start: (sid, durationBits) => { view.setUint32(0, durationBits >>> 0, true); process.start(sid, view.getFloat32(0, true)); } };
}
