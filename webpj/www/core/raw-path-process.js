// 이동 유닛의 경로 프로세스(Path 프로세스 8종)의 Kernel 프레임·도착·수명 분배를 옮긴 모듈이다(cpppj `o/RawPathProcess.cpp/.h`, 10.78 전용).
// 한 클래스가 원본의 서로 다른 가상 표(텔레포트 23·사제 54·마나 볼트 비행 39·웜 40·벌룬 42·Dude 24·Bulf 41·Walker 38)를 생성자 플래그로 고르며, 같은 풀·해시·spot·Kernel 위에서
// 별도의 좌표/공간 복사본 없이 실행한다. 부모(이동하는 객체)의 좌표·프레임·TYPE 은 매번 풀의 현재 raw 슬롯에서 읽는다.
//  - 생성: new RawPathProcess(pool, hash, types, frames, shapes, spots, state, hooks, { priest, flying, worm, balloon, dude, bulf, walker }) — 종류 플래그는 서로 배타적이다(경로 이미지 생성자가 거부).
//    types/frames/shapes 는 같은 길이의 TYPE 표(`flags2` 등)·FrameCodeTable 표·`{ frames: [{ walkStepX, walkStepY }] }` SHP 표, spots 는 65,536 바이트 지도이며 모두 프로세스보다 오래 살아야 한다.
//    생성만으로는 부착하지 않는다. 부착은 `RawPathProcess.attach(host, process, parent, flags, form)` 가 원본처럼 부모 TYPE(사제 158·Dude 70·웜 133/187)을 검사하고, 비행·벌룬·Walker 는 같은 종류 기존 프로세스를 먼저 제거한 뒤 한다.
//  - 상태(RawPathProcessState): clock(같은 호스트가 쓰는 SquidProcessState)·observation({ flags, stamp })·종류별 프로세스/TYPE 번호·windWalkerCounter({ value } 세션 공유)·checking(현재 DWORD)·blockingGenus(sharedBlockingGenus 로 빌릴 수 있음).
//  - 훅(hooks): unpop(sid, flags)·repop(sid, flags)·pop(sid, point, flags)·halt(sid)→10/11/12·arrival(sid, 대상A, 대상B)→1000/1001/1002·stepSound(sid, x, y, sound)·soundId(sid)·
//    search(runtime, sid, start, destination, flags)(`makePathSearchHooks` 가 실제 BFS 를 연결)·retarget(runtime, sid)·clearBlocked(sid, point, type, owner)·cleanup(runtime, sid, flags)·
//    observationStamp()·balloonFlame(x, y)·bulfRandom(low, high)·bulfSound(x, y, name)·assertion(expression, file, line). 필수는 unpop·repop·pop·halt·arrival·soundId·stepSound·clearBlocked·cleanup 이며
//    나머지는 해당 분기를 지날 때만 요구한다(없으면 빈 효과로 대체하지 않고 오류).
//  - 실행: runFrame() 은 종류별 전용 프레임(Walker38 0048b080·Bulf41 0048b760·Balloon42 0048ad60 — 기존 raw-*-frame 모듈)을 쓰고, 그 밖에는 사제/웜/Dude 의 SHP 보행 프레임 선택(발자국 거리·45도 배수)과
//    텔레포트/비행의 단순 Unpop→Pop(0x40)을 한다. arrive() 는 사제/웜/Dude 의 정지 프레임 접두(RawPathAnimation) 뒤 공통 도착 부모(runPathArrivalParent)이다.
//    onFormDestroy(flags) 는 공간 접두(권한 있고 Dead 가 아니면 Unpop→Repop)와 정리 하위 몸체(cleanup 훅)를 실행한다.
//  - 자기 삭제: 원본의 stack deadFlag 는 소멸자에서 켜진다. 이 모듈은 Kernel 이 부르는 destroy() 에서 같은 `{ value }` 감시 상자를 꺼서, 가상 효과가 자신을 지운 뒤에는 상자와 보존한 훅 사본만 읽는다.
// 10.72(CD) 분기(실제 프레임 BYTE·Robust 호스트 검사·관찰 시계)는 옮기지 않았다. 이 모듈은 DOM 을 쓰지 않는다.
// 근거: cpppj `o/RawPathProcess.cpp/.h`(기준선 1f00a25a 뒤 `20261109-53 CP` 의 shared 마스크·Boss() 반영), 원본 관측 pathruntime-x86.tsv frame 60행·arrival 12행·flypath-x86.tsv frame 63행·walk-shape-x86.tsv 1,170행·pathinit-x86.tsv 32행.
// (2026-11-13 WB04-E349: 6-P 경로 부품 RawPathProcess 이식으로 추가)
import { SquidProcess } from './squid-process.js';
import { RawSquidFinder, RAW_FINDER_FLAGS } from './raw-squid-finder.js';
import { RawPathRuntime } from './raw-path-runtime.js';
import { RawPathAnimation } from './raw-path-animation.js';
import { rawPathSurfaceAt } from './raw-path-surface.js';
import { pathArrivalBlocked, runPathArrivalParent, runPathBaseRobust } from './raw-path-arrival-parent.js';
import { runPathHaltParent } from './raw-path-halt-parent.js';
import { removeAttachedProcesses } from './raw-process-removal.js';
import { runBalloonFrame } from './raw-balloon-frame.js';
import { runBulfFrame } from './raw-bulf-frame.js';
import { runWalkerFrame } from './raw-walker-frame.js';

// 좌표를 칸으로 바꿀 때 더하는 단정도 0.9999 올림 보정과 지도 한 변, 사제 프레임 선택의 각도 주기이다. (2026-11-13 WB04-E349)
const CELL_BIAS = Math.fround(0.9999);
const MAP_SIDE = 256;
const STEP_ANGLE = 45;
// 슬롯 필드 위치(TYPE +10·상태 +11·좌표 +14/+18·그래프/고도 +30·소유자 +34·프레임 +36·강제 정지 장르 +40)와 상태 비트(Dead 2·Free 1·Void 4)이다. (2026-11-13 WB04-E349)
const TYPE_AT = 10, STATE_AT = 11, X_AT = 14, Y_AT = 18, HEIGHT_AT = 30, OWNER_AT = 34, FRAME_AT = 36, EXTRA_AT = 40;
const FREE_BIT = 1, DEAD_BIT = 2, VOID_BIT = 4;
// 경로 이미지 안의 Form(+4)·부모(+8) 위치와 Pop 재등록 flags(공간 갱신 0x40)이다. (2026-11-13 WB04-E349)
const IMAGE_FORM_AT = 4, IMAGE_PARENT_AT = 8, MOVE_POP_FLAGS = 0x40;
// 부모 TYPE genus 비트: 얼어붙는 Regular(0x5105) 확인 대상 0x10000, 운반 계열 0x30000, 길 밖 정지를 막는 gDAIS 0x400000, 벌룬 비행 순항 고도 44이다. (2026-11-13 WB04-E349)
const FROZEN_GENUS = 0x10000, CARRIER_GENUS = 0x30000, DAIS_GENUS = 0x400000, BALLOON_CRUISE = 44;
// 얼어붙은 부모에 붙은 Regular 사건 번호(0x5105)와 강제 정지 flags 비트(0x20)이다. (2026-11-13 WB04-E349)
const FROZEN_EVENT = 0x5105, FORCED_STOP_BIT = 0x20;
// 비행(39)의 다음 프레임 예약 간격(float 0.02 를 배정도로 읽은 값)이다. (2026-11-13 WB04-E349)
const FLIGHT_INTERVAL = 0.019999999552965164;
// 도착 반환 코드(1000 종료·1001 이미 죽음·1002 대기)와 Halt 결과 코드이다. (2026-11-13 WB04-E349)
const ARRIVAL_KILL = 1000, ARRIVAL_ALREADY_DEAD = 1001, ARRIVAL_LINGER = 1002;
// 길 밖 정지 검사에서 추가로 허용하는 타입 수(10.78 은 5개)와 spot 통행 비트이다. (2026-11-13 WB04-E349)
const GROUND_TYPE_COUNT = 5, SPOT_WALKABLE = 6;
// 프레임 글자 A 의 문자 코드이다. (2026-11-13 WB04-E349)
const LETTER_A = 0x41;

// float 비트 변환용 공용 보기이다. (2026-11-13 WB04-E349)
const FLOAT_VIEW = new DataView(new ArrayBuffer(4));

// 슬롯의 비정렬 DWORD 를 부호 없이 읽는다. (2026-11-13 WB04-E349)
function readDword(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 슬롯의 비정렬 float 를 읽는다. (2026-11-13 WB04-E349)
function readFloat(raw, offset) {
    FLOAT_VIEW.setUint32(0, readDword(raw, offset), true);
    return FLOAT_VIEW.getFloat32(0, true);
}

// 원본 _ftol 의 칸 좌표이다. 편향을 더한 값을 0 쪽으로 절삭하며 비유한·지도 밖은 호스트 오류로 거부한다. (2026-11-13 WB04-E349)
function cellOf(value, biased) {
    const adjusted = Math.fround(value) + (biased ? CELL_BIAS : 0);
    if (!Number.isFinite(adjusted) || adjusted < 0 || adjusted >= MAP_SIDE) throw new RangeError('경로 지도 좌표');
    return Math.trunc(adjusted);
}

// 훅 사본에서 이름의 함수를 호출 문맥을 지켜 묶는다. 없으면 undefined 로 두어 사용 위치의 공용 모듈이 "미연결"로 거부하게 한다. (2026-11-13 WB04-E349)
function bound(hooks, name) {
    return typeof hooks[name] === 'function' ? (...args) => hooks[name](...args) : undefined;
}

// 경로 프로세스가 읽는 원본 전역이다. 독립 입력은 자기 값을 쓰고, 실제 세션은 bindChecking/sharedBlockingGenus 로 같은 장기 전역을 빌려 매번 현재 값을 읽는다.
// 사용: const state = new RawPathProcessState(clock); state.checking = 0; state.windWalkerCounter.value. (2026-11-13 WB04-E349)
export class RawPathProcessState {
    // 독립 checking 의 자기 값과 빌린 getter 이다. getter 가 있으면 그 현재 DWORD 를 읽는다.
    #checking = 0;
    #checkingSource = null;

    // 같은 호스트가 쓰는 SquidProcessState(게임 시각·권한)를 연결한다. 다른 시계를 섞지 않으려고 반드시 받는다.
    constructor(clock) {
        if (!clock || typeof clock !== 'object') throw new TypeError('경로 상태에는 호스트와 같은 SquidProcessState 가 필요합니다.');
        // 프레임 판정에 현재 게임 시각·권한을 공급하는 상태이다.
        this.clock = clock;
        // 경로 진행 중 활성화 비트(flags)와 관찰 시각(stamp)을 보관하는 상태이다.
        this.observation = { flags: 0, stamp: 0 };
        // 원본 mutable 프로세스/자산 번호: 텔레포트 23·사제 54·사제 TYPE 158.
        this.teleportProcess = 23;
        this.priestProcess = 54;
        this.priestType = 158;
        // PATCH0054110c 마나 볼트의 비행 경로 번호이다(텔레포트 23 과 구분).
        this.flyProcess = 39;
        // 00427da0 의 길 밖 정지 허용 타입이다(10.78 은 다섯 개 모두 확인).
        this.groundTypes = [158, 137, 156, 135, 187];
        // 005325b4: 도착 칸의 원본 겹침 제거 부류이다. sharedBlockingGenus 가 있으면 그 현재 값이 우선한다.
        this.blockingGenus = 0x10444200;
        // 웜 40 의 실제 부착 검사에 쓰는 현재 두 부모 타입(sunWalker 133·empty 187)이다.
        this.wormProcess = 40;
        this.sunWalkerType = 133;
        this.emptyType = 187;
        // 00541118/00541214 의 벌룬 이동·화염 타입이다.
        this.balloonProcess = 42;
        this.balloonType = 111;
        // 005410d0/00541170 의 Dude 부착 종류와 부모 타입이다.
        this.dudeProcess = 24;
        this.dudeType = 70;
        // 00541108/54127c/5412c8 의 Walker 부착 종류와 윈드·레인 보행 타입, 5951c4 의 윈드 세션 공유 counter 상자이다.
        this.walkerProcess = 38;
        this.windWalkerType = 137;
        this.rainWalkerType = 156;
        this.windWalkerCounter = { value: 0 };
        // 00541114 의 Bulf 전용 회전/보행 경로 번호이다.
        this.bulfProcess = 41;
        // 실제 005325b4 같은 장기 마스크를 돌려주는 getter(() => number)이다. 독립 입력은 null.
        this.sharedBlockingGenus = null;
    }

    // 패치 Halt 의 현재 진단 DWORD 이다. 빌린 getter 가 있으면 매번 그 현재 값을 읽는다(삭제 뒤에도 원래 소유자를 읽음).
    get checking() {
        return this.#checkingSource ? this.#checkingSource() >>> 0 : this.#checking;
    }

    // 독립 값으로 되돌려 쓴다(빌린 getter 는 해제한다).
    set checking(value) {
        this.#checkingSource = null;
        this.#checking = value >>> 0;
    }

    // 실제 Client 의 진단 DWORD getter(() => number)를 빌린다. null 이면 자기 값으로 돌아간다.
    bindChecking(source) {
        if (source !== null && typeof source !== 'function') throw new TypeError('checking 은 getter 함수여야 합니다.');
        this.#checkingSource = source;
    }

    // 도착 판정 위치에서 읽는 현재 겹침 부류 마스크이다.
    currentBlockingGenus() {
        return this.sharedBlockingGenus ? this.sharedBlockingGenus() >>> 0 : this.blockingGenus;
    }
}

// 같은 pool/hash/spot/Kernel 위에서 원본 이동 프로세스를 실행한다. 사용 예는 파일 머리 설명을 본다. (2026-11-13 WB04-E349)
export class RawPathProcess extends SquidProcess {
    // 객체 슬롯 풀·공간 해시·TYPE/프레임/SHP 표·spot 지도·전역 상태이다. 모두 호출자 소유이며 사본을 만들지 않는다.
    #pool;
    #hash;
    #types;
    #frames;
    #shapes;
    #spots;
    #state;
    // 훅 사본이다(원본 std::function 복사). 부모/자신이 삭제된 뒤에도 이 사본만 읽는다.
    #hooks;
    // 종류 플래그(서로 배타적): 사제·비행 39·웜 40·벌룬 42·Dude 24·Walker 38·Bulf 41.
    #priest;
    #flying;
    #worm;
    #balloon;
    #dude;
    #walker;
    #bulf;
    // 원본 경로 이미지(100080바이트)이다.
    #runtime;
    // 원본 destructor 의 stack deadFlag 대체이다. destroy() 에서 꺼지며 게임 상태를 대체하지 않는다.
    #alive = { value: true };

    // 필수 하위 가상 효과를 확인한다. 아직 연결되지 않은 BFS·재탐색 같은 선택 효과는 사용 위치에서만 거부한다.
    // kinds = { priest, flying, worm, balloon, dude, bulf, walker } 플래그이다(41 은 48b700/48b760/48b340, 사제는 54 분배).
    constructor(pool, hash, types, frames, shapes, spots, state, hooks, kinds = {}) {
        super();
        const { priest = false, flying = false, worm = false, balloon = false, dude = false, bulf = false, walker = false } = kinds;
        if (pool?.edition !== 'patch1078') throw new TypeError('경로 프로세스는 10.78 풀만 받습니다.');
        if (!(state instanceof RawPathProcessState)) throw new TypeError('경로 프로세스에는 RawPathProcessState 가 필요합니다.');
        // 필수 훅 아홉 개를 하나라도 빠뜨리면 실행 가능한 빈 효과로 바꾸지 않고 거부한다.
        const required = ['unpop', 'repop', 'pop', 'halt', 'arrival', 'soundId', 'stepSound', 'clearBlocked', 'cleanup'];
        if (types.length !== frames.length || types.length !== shapes.length || spots.length !== 65536 || !hooks || required.some((name) => typeof hooks[name] !== 'function')) {
            throw new RangeError('경로 월드 연결 누락');
        }
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#frames = frames;
        this.#shapes = shapes;
        this.#spots = spots;
        this.#state = state;
        this.#hooks = { ...hooks };
        this.#priest = Boolean(priest);
        this.#flying = Boolean(flying);
        this.#worm = Boolean(worm);
        this.#balloon = Boolean(balloon);
        this.#dude = Boolean(dude);
        this.#walker = Boolean(walker);
        this.#bulf = Boolean(bulf);
        this.#runtime = new RawPathRuntime({ priest, flying, worm, balloon, dude, bulf, walker });
    }

    // 부착 전/중의 공유 버퍼 작성도 생성 당시의 같은 SID 풀만 쓰도록 풀을 보인다.
    get pool() { return this.#pool; }

    // 원본 경로 이미지이다. 탐색/건설 시각 조정과 네트워크 어댑터가 같은 저장소를 직접 쓴다.
    get runtime() { return this.#runtime; }

    // Kernel 이 삭제할 때 부르는 가상 소멸자이다. 원본 destructor 처럼 deadFlag 를 켠다(감시 상자를 끈다).
    destroy() {
        this.#alive.value = false;
    }

    // 직접 부착도 이 모듈의 동일 풀 계약을 확인한다. 사제·Dude·웜은 부모 TYPE 을 현재 전역과 대조하고, 비행·벌룬·Walker 는 부착 전에 같은 종류 기존 프로세스를 제거한다.
    // 사용: RawPathProcess.attach(host, process, parentSid, 0x10). 반환은 부착된 프로세스(패치판이 부모 free/dead 로 등록하지 않으면 null). (2026-11-13 WB04-E349)
    static attach(host, process, parent, flags = 0, form = 0) {
        if (!(process instanceof RawPathProcess) || host?.pool !== process.#pool) throw new TypeError('경로 부착 풀 불일치');
        const parentType = process.#pool.physical(parent)[TYPE_AT];
        if (process.#priest && parentType !== process.#state.priestType) throw new Error('사제 경로 부모 타입');
        if (process.#dude && parentType !== process.#state.dudeType) throw new Error('Dude24 경로 부모 타입');
        if (process.#worm && parentType !== process.#state.sunWalkerType && parentType !== process.#state.emptyType) throw new Error('웜40의 실제 sunWalker/empty 부착 검사');
        const type = process.movementType();
        // 실제 48af50 은 새 비행 프로세스를 부착하기 전에 기존 39 를 모두 제거한다(벌룬·Walker 도 같다).
        if (process.#flying || process.#balloon || process.#walker) removeAttachedProcesses(host, parent, type);
        return host.attach(process, type, parent, flags, form);
    }

    // 실제 종류가 쓰는 현재 프로세스 번호(23/39/54/40/42/24/41/38)이다. 서로 다른 가상 표의 종류를 번호만으로 대체하지 않는다.
    movementType() {
        const state = this.#state;
        if (this.#walker) return state.walkerProcess;
        if (this.#bulf) return state.bulfProcess;
        if (this.#dude) return state.dudeProcess;
        if (this.#balloon) return state.balloonProcess;
        if (this.#worm) return state.wormProcess;
        if (this.#flying) return state.flyProcess;
        return this.#priest ? state.priestProcess : state.teleportProcess;
    }

    // +0x14 원본 base 통지는 빈 몸체이며 현재 이미지의 번호만 같은 base 필드에 기록한다.
    onAttached() {
        this.#runtime.setAttachment(this.form, this.parent);
    }

    // 같은 풀의 raw 좌표이다(보관 사본이 아닌 현재 슬롯).
    #position(sid) {
        const raw = this.#pool.physical(sid);
        return { x: readFloat(raw, X_AT), y: readFloat(raw, Y_AT) };
    }

    // 패치 frame 은 네 바이트 부호 있는 DWORD 이다.
    #frame(sid) {
        return readDword(this.#pool.physical(sid), FRAME_AT) | 0;
    }

    // 프레임 바꾸기는 DWORD 전체만 쓰며 가상 Pop 은 호출자가 순서를 보존한다.
    #setFrame(sid, frame) {
        const raw = this.#pool.allocated(sid);
        const value = frame >>> 0;
        // 낮은 바이트부터 네 바이트를 쓴다.
        for (let index = 0; index < 4; index += 1) raw[FRAME_AT + index] = (value >>> (8 * index)) & 0xff;
    }

    // 실제 Init +0x20 이다. 비행/벌룬은 직선 비행 Init 만 하고, 그 밖에는 시작 표면이 있을 때만 (void 면 Pop→)검색→current=0 을 실행한다. flags 1 은 직선 방향만 축약하며 Bulf/Walker 는 항상 1 이다.
    // 표면 번호가 0 이면 Pop/검색/current 쓰기를 모두 건너뛴다(좌표 범위만 검사하지 않음). 사용: process.initializePath(parentSid, destination, start, flags). (2026-11-13 WB04-E349)
    initializePath(parent, destination, start, flags = 0) {
        if (this.#flying || this.#balloon) {
            this.#runtime.initializeFlight(this.#position(parent), destination, this.#balloon ? BALLOON_CRUISE : 0, this.#balloon ? this.#pool.physical(parent)[HEIGHT_AT] : 0);
            return;
        }
        if (!rawPathSurfaceAt(this.#hash, start.x, start.y)) return;
        if (typeof this.#hooks.search !== 'function') throw new Error('실제 Path BFS 미복원 경계');
        if ((this.#pool.physical(parent)[STATE_AT] & VOID_BIT) !== 0) this.#hooks.pop(parent, start, 0);
        this.#hooks.search(this.#runtime, parent, start, destination, (this.#bulf || this.#walker) ? 1 : flags);
        this.#runtime.resetCurrent();
    }

    // +0x30 의 현재 speed/turningSpeed 입력이다. 패치만 nonzero override 를 쓴다(0 이면 타입 속도). 외부 타입 속도는 mutable 원본 속성을 매 호출에서 읽어 공급해야 한다.
    prepare(speedOverride, speed, turningSpeed) {
        if (speed === 0 || turningSpeed === 0) throw new RangeError('경로 타입 속도0');
        const effective = speedOverride !== 0 ? speedOverride : speed;
        const now = this.#state.clock.now;
        this.#runtime.prepare(now, now, effective, turningSpeed, this.#runtime.direction(0));
    }

    // 원본 권한 또는 로컬 form 번호 범위이다(클라이언트 범위는 현재 풀 판본의 serverFirst).
    #mayMutate() {
        return this.#state.clock.boss || (this.form > 4 && this.form < this.#pool.layout.serverFirst);
    }

    // 패치는 두 현재 SID 의 범위를 진단한 뒤 항상 참이다.
    #robust() {
        const image = this.#runtime.bytes;
        return runPathBaseRobust({
            form: () => readDword(image, IMAGE_FORM_AT),
            parent: () => readDword(image, IMAGE_PARENT_AT),
            capacity: () => this.#pool.capacity,
            assertion: bound(this.#hooks, 'assertion'),
        });
    }

    // 부모 가상 Halt 의 self-delete/extra/운반 대상 재설정 분기(00489ff0)이다. 반환은 "정지(true) 또는 계속(false)"이다.
    #halt() {
        const hooks = this.#hooks;
        const watch = this.#alive;
        const runtime = this.#runtime;
        const pool = this.#pool;
        const types = this.#types;
        // 자기 삭제 뒤에도 진단/수명 조회의 복사본만 남긴다.
        return runPathHaltParent(() => this.#state.checking, {
            parent: () => this.parent,
            halt: bound(hooks, 'halt'),
            alive: () => watch.value,
            assertion: bound(hooks, 'assertion'),
            forcedStop: (parent) => (pool.physical(parent)[EXTRA_AT] & FORCED_STOP_BIT) !== 0,
            mayKill: () => this.#mayMutate(),
            kill: () => { this.host.kill(this, 0); },
            boss: () => this.#state.clock.boss,
            carrier: (parent) => (types[pool.physical(parent)[TYPE_AT]].flags2 & CARRIER_GENUS) !== 0,
            hasTargets: () => runtime.targetB !== 0 || runtime.targetA !== 0,
            retarget: (parent) => {
                if (typeof hooks.retarget !== 'function') throw new Error('실제 운반 재탐색 몸체 미복원 경계');
                hooks.retarget(runtime, parent);
            },
            // 원문 지역 watch 의 활성 의미만 raw 필드에도 남긴다.
            armWatch: () => { runtime.setDeathWatchToken(1); },
            clearWatch: () => { runtime.setDeathWatchToken(0); },
        });
    }

    // 원본 +0x28 다음에 조회하는 길 밖 지면/주변 부류 검사이다. 타입 허용→ceil/floor spot 6→권한→실제 일반 finder 순서이며 정지하면 자신을 삭제하고 참을 반환한다.
    #groundStop(point) {
        const pool = this.#pool;
        const type = pool.physical(this.parent)[TYPE_AT];
        if (!this.#state.groundTypes.slice(0, GROUND_TYPE_COUNT).includes(type)) return false;
        const cx = cellOf(point.x, true), cy = cellOf(point.y, true), fx = cellOf(point.x, false), fy = cellOf(point.y, false);
        if ((this.#spots[cy * MAP_SIDE + cx] & SPOT_WALKABLE) !== 0 && (this.#spots[fy * MAP_SIDE + fx] & SPOT_WALKABLE) !== 0) return false;
        if (!this.#mayMutate()) return false;
        const finder = new RawSquidFinder(pool, this.#hash, this.#types);
        // 원본 finder 순서 그대로 주변의 gDAIS(0x400000)만 확인한다.
        for (let candidate = finder.begin({ left: cx - 1, top: cy - 1, right: cx + 1, bottom: cy + 1 }, RAW_FINDER_FLAGS.skipSurface); candidate !== 0; candidate = finder.next()) {
            if ((this.#types[pool.physical(candidate)[TYPE_AT]].flags2 & DAIS_GENUS) !== 0) return false;
        }
        this.host.kill(this, 0);
        return true;
    }

    // spot 8 의 실제 일반 finder 로 막힌 부류를 확인한다(도착 칸의 현재 겹침 마스크).
    #blocked(point) {
        return pathArrivalBlocked(this.#pool, this.#hash, this.#types, this.#spots, () => this.#state.currentBlockingGenus(), point);
    }

    // 현재 raw 부모 위치에서 실제 Step 을 호출하는 공통 입구이다. 짝수 전환의 발소리는 stepSound 훅에 현재 부모 좌표·soundId 로 요청한다(stamp 는 10.72 관찰 시계라 10.78 Step 이 무시한다).
    #step(parent, withStamp) {
        const hooks = this.#hooks;
        const stepSound = () => {
            const position = this.#position(parent);
            hooks.stepSound(parent, position.x, position.y, hooks.soundId(parent));
        };
        const stamp = withStamp && typeof hooks.observationStamp === 'function' ? hooks.observationStamp() : 0;
        return this.#runtime.step(this.#state.clock.now, this.#position(parent), this.#state.observation, stepSound, stamp);
    }

    // 사제/웜/Dude/Walker/Bulf 가 얼어붙음(Regular 0x5105)이면 이동하지 않는 판정(004287a0)이다. TYPE genus 0x10000 부모에 사건이 있어야 한다.
    #immobile(parent) {
        return (this.#types[this.#pool.physical(parent)[TYPE_AT]].flags2 & FROZEN_GENUS) !== 0 && this.host.findEvent(parent, FROZEN_EVENT) !== null;
    }

    // type23 0048a950 및 type54 0048bb40 등 종류별 실제 실행 분기이다. Walker/Bulf/Balloon 은 전용 프레임 모듈에 위임하고, 그 밖은 SHP 보행 프레임 선택 또는 단순 Unpop→Pop 을 한다. (2026-11-13 WB04-E349)
    runFrame() {
        if (!this.host) throw new Error('미부착 경로 RunFrame');
        const hooks = this.#hooks;
        const pool = this.#pool;
        const state = this.#state;
        const runtime = this.#runtime;
        const watch = this.#alive;
        const parent = this.parent;
        if (this.#walker) {
            // 같은 실제 경로 수명만 관찰한다.
            runWalkerFrame(pool, parent, runtime, state.clock.now, this.#frames, state.windWalkerType, state.rainWalkerType, state.windWalkerCounter, {
                immobile: () => this.#immobile(parent),
                halt: () => this.#halt(),
                step: () => this.#step(parent, true),
                groundStop: (point) => this.#groundStop(point),
                arrive: () => { this.arrive(); },
                unpop: bound(hooks, 'unpop'),
                pop: bound(hooks, 'pop'),
            }, watch);
            return;
        }
        if (this.#bulf) {
            // 같은 실제 부모/경로의 수명을 효과 뒤 확인한다.
            runBulfFrame(pool, parent, runtime, this.#frames, this.#shapes, {
                immobile: () => this.#immobile(parent),
                halt: () => this.#halt(),
                step: () => this.#step(parent, true),
                arrive: () => { this.arrive(); },
                unpop: bound(hooks, 'unpop'),
                pop: bound(hooks, 'pop'),
                randomInt: bound(hooks, 'bulfRandom'),
                sound: bound(hooks, 'bulfSound'),
            }, watch);
            return;
        }
        if (this.#balloon) {
            // 가상 효과가 자신을 지운 뒤에는 watch 만 읽는다.
            runBalloonFrame(pool, parent, runtime, state.clock.now, state.balloonType, {
                halt: () => this.#halt(),
                step: () => this.#step(parent, false),
                arrive: () => { this.arrive(); },
                unpop: bound(hooks, 'unpop'),
                pop: bound(hooks, 'pop'),
                flame: (x, y) => {
                    if (typeof hooks.balloonFlame !== 'function') throw new Error('벌룬 화염 음향 미연결');
                    hooks.balloonFlame(x, y);
                },
            }, watch);
            return;
        }
        const type = pool.physical(parent)[TYPE_AT];
        const walking = this.#priest || this.#worm || this.#dude;
        if (walking && this.#immobile(parent)) return;
        if (runtime.finished || this.#halt()) return;
        if (this.#flying) {
            // 48af90 의 예약 경계이며 Halt 는 예약 전에도 실행한다.
            if (runtime.nextFrame > state.clock.now) return;
            runtime.setNextFrame(state.clock.now + FLIGHT_INTERVAL);
        }
        const sample = this.#step(parent, true);
        if (!watch.value) return;
        if (sample.finished) {
            this.arrive();
            return;
        }
        if (!walking) {
            // 텔레포트/비행은 좌표가 바뀔 때만 Unpop(0)→Pop(0x40)이다.
            const current = this.#position(parent);
            if (sample.position.x !== current.x || sample.position.y !== current.y) {
                hooks.unpop(parent, 0);
                hooks.pop(parent, sample.position, MOVE_POP_FLAGS);
            }
            return;
        }
        if (this.#groundStop(sample.position) || !watch.value || (pool.physical(parent)[STATE_AT] & FREE_BIT) !== 0) return;
        const current = this.#position(parent);
        const originalFrame = this.#frame(parent);
        let nextFrame = originalFrame;
        const shapeFrames = this.#shapes[type].frames;
        if (originalFrame < 0 || originalFrame >= shapeFrames.length) throw new RangeError('경로 SHP 프레임');
        const shape = shapeFrames[originalFrame];
        const footX = Math.fround(current.x + shape.walkStepX);
        const footY = Math.fround(current.y + shape.walkStepY);
        const direction = runtime.direction(runtime.current);
        if (sample.angle % STEP_ANGLE === 0) {
            const dx = sample.position.x - current.x, dy = sample.position.y - current.y;
            const hx = footX - current.x, hy = footY - current.y;
            // 발자국이 이동 거리보다 짧으면(엄격) 같은 방향 글자 구간의 다음 프레임으로 넘어간다.
            if (hx * hx + hy * hy < dx * dx + dy * dy) {
                if (direction < 0 || direction > 15) throw new RangeError('경로 프레임 글자');
                const run = this.#frames[type].run(LETTER_A + direction);
                if (run.count === 0) throw new RangeError('경로 프레임 구간0');
                nextFrame = (((originalFrame - run.first) + 1) % run.count + run.first) | 0;
                if (nextFrame < 0 || nextFrame >= this.#frames[type].codes.length) throw new RangeError('경로 다음 프레임');
            }
        }
        if (nextFrame !== originalFrame) {
            hooks.unpop(parent, 0);
            this.#setFrame(parent, nextFrame);
            hooks.pop(parent, sample.position, MOVE_POP_FLAGS);
        }
    }

    // 도착의 사제 정지 프레임 접두→final Pop(0)→Halt→가상 도착→1000/1001/1002 종단이다. 도착이 부모·Form·프로세스 자신을 지워도 호출 뒤에는 watch 와 보존한 외부 효과만 쓴다. (2026-11-13 WB04-E349)
    arrive() {
        const hooks = this.#hooks;
        const pool = this.#pool;
        const runtime = this.#runtime;
        const watch = this.#alive;
        // 사제/웜/Dude 는 도착 프레임 접두(정지 프레임)를 먼저 쓴다.
        if (this.#priest || this.#worm || this.#dude) {
            new RawPathAnimation(pool, this.#frames, { unpop: bound(hooks, 'unpop'), repop: bound(hooks, 'repop') }).stop(this.parent);
        }
        if (!watch.value) return;
        const currentParent = () => {
            const value = readDword(runtime.bytes, IMAGE_PARENT_AT);
            if (value > 0xffff) throw new RangeError('도착 현재 부모의 물리 SID 폭');
            return value;
        };
        runPathArrivalParent(() => this.#state.checking, {
            alive: () => watch.value,
            assertion: bound(hooks, 'assertion'),
            parent: currentParent,
            validAlive: (parent) => parent > 0 && parent < pool.capacity && (pool.physical(parent)[STATE_AT] & (FREE_BIT | DEAD_BIT)) === 0,
            step: () => this.#step(currentParent(), true).position,
            unpop: (parent) => { hooks.unpop(parent, 0); },
            pop: (parent, point) => { hooks.pop(parent, point, 0); },
            robust: () => { this.#robust(); },
            blocked: (point) => this.#blocked(point),
            halt: () => this.#halt(),
            finished: (finished) => { runtime.setFinished(finished); },
            deadOrFree: (parent) => (pool.physical(parent)[STATE_AT] & (FREE_BIT | DEAD_BIT)) !== 0,
            watchPresent: () => runtime.deathWatchToken !== 0,
            armWatch: () => { runtime.setDeathWatchToken(1); },
            clearWatch: () => { runtime.setDeathWatchToken(0); },
            arrival: (parent) => hooks.arrival(parent, runtime.targetA, runtime.targetB),
            kill: () => { this.host.kill(this, 0); },
            // 같은 Pool/훅 사본만 삭제 뒤 사용한다.
            clear: (parent) => {
                const raw = pool.physical(parent);
                hooks.clearBlocked(parent, { x: readFloat(raw, X_AT), y: readFloat(raw, Y_AT) }, raw[TYPE_AT], raw[OWNER_AT]);
            },
        });
    }

    // 실제 Form 삭제 통지의 공간 접두(권한 있고 부모가 Dead 가 아니면 Unpop→Repop)와 명시된 정리 하위 몸체(cleanup 훅: 예약·결정·사제 의식·대상 삭제)를 실행한다. (2026-11-13 WB04-E349)
    onFormDestroy(flags) {
        const parent = this.parent;
        if (this.#mayMutate() && (this.#pool.physical(parent)[STATE_AT] & DEAD_BIT) === 0) {
            this.#hooks.unpop(parent, 0);
            this.#hooks.repop(parent, 0);
        }
        this.#hooks.cleanup(this.#runtime, parent, flags);
    }
}
