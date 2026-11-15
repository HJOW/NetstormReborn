// 기존 Start/BFS/Process/수신·전송을 같은 실제 World/사제/도착/정리 모듈에 연결한다(cpppj client/RawCampaignPathBindings, 225553df).
// 사용: 같은 clock의 start/process 상태와 필수 effects를 공급하고 startPriest/startCarrier/currentPath를 사용한다.
// 상위는 부착한 Path/Kernel을 비운 뒤 이 연결부의 콜백 수명을 끝낸다. (2026-11-16 WB04-E568)
import { PRIEST_TYPE, PRIEST_VTABLE } from './raw-priest-owner.js';
import { RawPathStart } from './raw-path-start.js';
import { RawPathProcess } from './raw-path-process.js';
import { RawPathSearch, makePathSearchHooks } from './raw-path-search.js';
import { RawPathData, makePathFormTransmission } from './raw-path-data.js';

// 10.78 실제 사제 이동 Form 번호이며 변경된 다른 경로를 이 구현으로 가장하지 않는다.
const PRIEST_PROCESS = 54;
// 수신의 cdecl 생성자 주소가 고정한 실제 종류이다. 현재 mutable 번호와 별개로 선택한다.
const CONSTRUCTORS = new Map([[0x48be40, {}], [0x48be70, { balloon: true }], [0x48bea0, { flying: true }], [0x48bed0, { walker: true }], [0x48bf00, { bulf: true }], [0x48bf30, { dude: true }], [0x48bf60, { priest: true }], [0x48bf90, { worm: true }]]);

// 현재 raw 가상 표 DWORD를 비정렬 little endian으로 읽는다.
function vtable(raw) { return new DataView(raw.buffer, raw.byteOffset, raw.byteLength).getUint32(0, true); }
// C++ span의 data/size 정체성에 대응하는 같은 BYTE 보기 검사다.
function sameBytes(first, second) { return first === second || (first?.buffer === second?.buffer && first?.byteOffset === second?.byteOffset && first?.byteLength === second?.byteLength); }
// 수신 live Form 상세를 원문의 256BYTE C 문자열 버퍼와 같은 최대255 UTF-8 바이트로 제한한다.
function invalidUpdateDetail(name) {
    // 원문의 고정 크기 상세 버퍼에 넣을 현재 TYPE 문구이다.
    const text = `Receiving an invalid update for a path process. Client thinks the form is a ${String(name).split('\0')[0]}.\n`;
    return new TextDecoder().decode(new TextEncoder().encode(text).subarray(0, 255));
}

// 실제 모듈/상태의 참조와 효과 함수 사본만 소유한다. 좌표·경로·지면을 복제하지 않는다.
export class RawCampaignPathBindings {
    // 상위 World/Core·사제·도착/정리·현재 Form 조회기이다.
    #world; #core; #priests; #halt; #arrival; #cleanup; #lookup;
    // 같은 clock을 참조하는 공유 Start/Process 상태와 값으로 보관한 효과 함수이다.
    #startState; #processState; #effects;
    // 같은 장기 거리표/큐와 기존 Start 몸체이다.
    #search; #starter;

    // C++와 같이 검색 작업 물리를 구성한 뒤 같은 World/현재 clock/실제54/필수 장치를 검증한다.
    constructor(world, core, priests, halt, arrival, cleanup, lookup, startState, processState, searchState, effects) {
        this.#world = world; this.#core = { ...core }; this.#priests = priests; this.#halt = halt; this.#arrival = arrival; this.#cleanup = cleanup; this.#lookup = lookup;
        this.#startState = startState; this.#processState = processState; this.#effects = { ...effects };
        this.#search = new RawPathSearch(core.pool, core.hash, core.spots, core.types, core.frames, searchState, world.storage.pathSearchWork);
        // 현재 World의 같은 배열·Host·시계·Kernel만 허용한다.
        const current = world.core();
        if (['pool', 'hash', 'types', 'frames', 'shapes', 'processes', 'processState', 'kernel'].some((name) => current[name] !== core[name]) || !sameBytes(current.spots, core.spots)
            || [priests.priestState, halt, arrival, cleanup, lookup].some((body) => body.pool !== core.pool)
            || startState.clock !== core.processState || processState.clock !== core.processState || startState.priestType !== PRIEST_TYPE || processState.priestType !== PRIEST_TYPE || processState.priestProcess !== PRIEST_PROCESS
            || ['metadata', 'soundId', 'stepSound', 'startBlocked', 'arrivalBlocked', 'retarget'].some((name) => typeof this.#effects[name] !== 'function')) throw new TypeError('캠페인 사제 경로의 같은 월드/시계/실제54/필수 효과 연결 오류');
        this.#starter = new RawPathStart(core.pool, core.hash, core.types, core.processes, lookup, core.frame, startState, {
            processType: (type) => priests.movementProcessType(type), create: (priest) => this.#createType(priest ? processState.priestProcess : processState.teleportProcess),
            createType: (type) => this.#createType(type), metadata: this.#effects.metadata, clearBlocked: this.#effects.startBlocked,
        });
    }

    // 실제 살아 있는 사제 생성자가 설치한 TYPE/VT와 현재 이동 번호를 검사한다.
    #validatePriest(parent) {
        if (parent < 5 || parent >= this.pool.capacity) throw new RangeError('캠페인 사제 명령 SID 범위');
        // 현재 부모/Form의 실제 슬롯 보기이다.
        const raw = this.pool.allocated(parent);
        if (raw[10] !== this.#startState.priestType || vtable(raw) !== PRIEST_VTABLE || (raw[11] & 9)) throw new TypeError('캠페인 사제 명령의 실제 타입/가상 표/살아 있는 자산 오류');
        if (this.#priests.movementProcessType(raw[10]) !== PRIEST_PROCESS || this.#processState.priestProcess !== PRIEST_PROCESS) throw new Error('캠페인 사제의 다른 mutable 이동 타입 미복원 경계');
    }

    // 현재 번호 비교로 생성할 실제 종류를 선택한다. 수신의 고정 생성자 선택은 이 경계를 쓰지 않는다.
    #createType(type) {
        // 실제 원문 모드/타입을 공급하는 공유 상태이다.
        const state = this.#processState;
        // 현재 번호 비교가 선택한 실제 파생 종류이다.
        const kinds = { priest: type === state.priestProcess, flying: type === state.flyProcess, worm: type === state.wormProcess, balloon: type === state.balloonProcess, dude: type === state.dudeProcess, bulf: type === state.bulfProcess, walker: type === state.walkerProcess };
        if (!Object.values(kinds).some(Boolean) && type !== state.teleportProcess) throw new Error('다른 캠페인 경로 생성자 미연결');
        return this.#createVariant(kinds);
    }

    // 함수 주소가 고정한 클래스를 생성하며 Init/Attach/Frame은 실행하지 않는다. 미지원은 null이다.
    createByConstructor(address) { return this.pool.edition === 'patch1078' && CONSTRUCTORS.has(address) ? this.#createVariant(CONSTRUCTORS.get(address)) : null; }

    // 실제 부모 Halt/Arrival과 공간·BFS·정리·외부 장치의 같은 호출 순서를 조립한다.
    #createVariant(kinds) {
        const { priest, worm, balloon, dude, bulf, walker } = kinds;
        // 값 사본으로 보관한 큰 외부 효과 함수이다.
        const effects = this.#effects; const world = this.#world;
        // 실제 하위 모듈에 연결할 함수 묶음이다.
        const hooks = { assertion: effects.assertion,
            unpop: (sid, flags) => world.unpop(sid, flags), repop: (sid, flags) => world.repop(sid, flags), pop: (sid, point, flags) => world.pop(sid, point.x, point.y, flags),
            // Dude·사제·운반체·건설 부모를 원문 우선순위로 분배한다.
            halt: (sid) => {
                if (dude) { if (typeof effects.dudeHalt !== 'function') throw new Error('실제 Dude 부모 정지 미연결'); return effects.dudeHalt(sid); }
                if (priest) return this.#halt.halt(sid);
                if (worm || balloon || bulf || walker) { this.#validateCarrier(sid); if (walker && this.pool.slot(sid)[10] === this.#processState.rainWalkerType) { if (typeof effects.crabHalt !== 'function') throw new Error('크리스탈 크랩42ab80 미연결'); return effects.crabHalt(sid); } return this.#halt.commonHalt(sid); }
                this.#validateConstruction(sid); return 10;
            },
            // 선택한 큰 도착 소비자는 미공급 시에만 해당 호출에서 거부한다.
            arrival: (sid, first, second) => {
                if (dude) { if (typeof effects.dudeArrival !== 'function') throw new Error('실제 Dude 부모 도착 미연결'); return effects.dudeArrival(sid); }
                if (priest) return this.#arrival.arrival(sid, first, second);
                if (worm || balloon || bulf || walker) { this.#validateCarrier(sid); if (typeof effects.carrierArrival !== 'function') throw new Error('실제 Carrier 도착 미연결'); return effects.carrierArrival(sid, first, second); }
                this.#validateConstruction(sid); if (typeof effects.constructionArrival !== 'function') throw new Error('실제 건설 운반 도착 미연결'); return effects.constructionArrival(sid, first, second);
            },
            soundId: effects.soundId, stepSound: effects.stepSound, clearBlocked: effects.arrivalBlocked, retarget: effects.retarget, observationStamp: effects.observationStamp,
            balloonFlame: effects.balloonFlame, bulfRandom: effects.bulfRandom, bulfSound: effects.bulfSound,
            cleanup: (runtime, parent) => this.#cleanup.afterSpatial(runtime, parent),
        };
        // 같은 Pool/배열/Host/시계를 빌리는 현재 Core이다.
        const core = this.#core;
        return new RawPathProcess(core.pool, core.hash, core.types, core.frames, core.shapes, core.spots, this.#processState, makePathSearchHooks(core.pool, this.#search, hooks), kinds);
    }

    // 가상 주소를 실행하지 않고 실제 텔레포트/마나 볼트 두 부모의 생성자 계약만 허용한다.
    #validateConstruction(parent) {
        if (this.pool.edition !== 'patch1078' || parent < 5 || parent >= this.pool.capacity) throw new RangeError('건설 운반 경로 판본/SID 오류');
        // 현재 부모/Form의 실제 슬롯 보기이다.
        const raw = this.pool.allocated(parent); const table = vtable(raw);
        if ((raw[11] & 9) || !((raw[10] === this.#startState.teleportType && table === 0x5070c0 && this.#priests.movementProcessType(raw[10]) === this.#processState.teleportProcess) || (raw[10] === this.#startState.manaBoltType && table === 0x509810 && this.#priests.movementProcessType(raw[10]) === this.#processState.flyProcess))) throw new Error('건설 운반의 실제 타입/VT/이동 종류 오류');
    }
    // 실제 Sun Walker·Bulf·벌룬/클라우드 플로터·Wind/Rain Walker의 현재 부모 표와 이동 종류다.
    #validateCarrier(parent) {
        if (this.pool.edition !== 'patch1078' || parent < 5 || parent >= this.pool.capacity) throw new RangeError('웜40 부모 판본/SID 오류');
        // 현재 부모/Form의 실제 슬롯 보기이다.
        const raw = this.pool.allocated(parent); const table = vtable(raw); const state = this.#processState; const movement = this.#priests.movementProcessType(raw[10]);
        // 실제 Sun Walker 부모와 웜40의 일치 여부이다.
        const worm = raw[10] === state.sunWalkerType && table === 0x50e568 && movement === state.wormProcess;
        // 실제 벌룬/클라우드 플로터 부모와42의 일치 여부이다.
        const balloon = (((raw[10] === 111 || raw[10] === 141) && table === 0x5040c0) || (raw[10] === 146 && table === 0x504198)) && movement === state.balloonProcess;
        // 실제 Bulf 부모와41의 일치 여부이다.
        const bulf = raw[10] === 135 && table === 0x503e50 && movement === state.bulfProcess;
        // 실제 Wind/Rain Walker 부모와38의 일치 여부이다.
        const walker = ((raw[10] === state.windWalkerType && table === 0x503d80) || (raw[10] === state.rainWalkerType && table === 0x503f20)) && movement === state.walkerProcess;
        if ((raw[11] & 9) || (!worm && !balloon && !bulf && !walker)) throw new Error('Carrier 실제 부모 타입/가상 표/이동 종류 오류: empty42ac60은 별도 미연결');
    }
    // 실제 Dude24의 부모 타입/VT와 현재 사제 월드 이동 번호를 함께 검사한다.
    #validateDude(parent) {
        if (this.pool.edition !== 'patch1078' || parent < 5 || parent >= this.pool.capacity) throw new RangeError('Dude24 부모 판본/SID 오류');
        // 현재 부모/Form의 실제 슬롯 보기이다.
        const raw = this.pool.allocated(parent);
        if (raw[10] !== this.#processState.dudeType || raw[10] !== this.#startState.dudeType || vtable(raw) !== 0x508cc8 || (raw[11] & 9) || this.#processState.dudeProcess !== 24 || this.#priests.movementProcessType(raw[10]) !== this.#processState.dudeProcess) throw new Error('Dude24 실제 부모/가상 표/이동 종류 오류');
    }

    // 기존 Start가 명령/스냅/BFS/기존 경로 교체/부모 통지를 모두 담당한다.
    startPriest(request) { this.#validatePriest(request.parent); return this.#starter.start(request); }
    // 건설 경로는 사제 스냅 대신 해당 실제 종류/flags10을 사용한다.
    startConstruction(request) { this.#validateConstruction(request.parent); if (typeof this.#effects.constructionArrival !== 'function') throw new Error('건설 운반의 실제 도착 소비자 미연결'); return this.#starter.start(request); }
    // 운반체는 검증한 현재 원문 종류의 기존 시작/Prepare/Attach를 사용한다.
    startCarrier(request) { this.#validateCarrier(request.parent); return this.#starter.start(request); }
    // Dude의 두 큰 부모 가상 효과를 시작 전에 요구한다.
    startDude(request) { this.#validateDude(request.parent); if (typeof this.#effects.dudeHalt !== 'function' || typeof this.#effects.dudeArrival !== 'function') throw new Error('Dude24 실제 부모 효과 미연결'); return this.#starter.start(request); }

    // 매 호출 실제 종속 Form과 현재 PID를 조회하며 오래된 Path 포인터를 캐시하지 않는다.
    currentPath(parent) {
        if (parent < 5 || parent >= this.pool.capacity) throw new RangeError('캠페인 현재 Path 부모 SID 범위');
        // 현재 부모/Form의 실제 슬롯 보기이다.
        const raw = this.pool.slot(parent); const type = raw[10]; const state = this.#processState;
        let movement = PRIEST_PROCESS;
        if (type !== this.#startState.priestType) {
            if (type === this.#startState.dudeType) this.#validateDude(parent);
            else if ([state.sunWalkerType, state.emptyType, 111, 141, 146, 135, state.windWalkerType, state.rainWalkerType].includes(type)) this.#validateCarrier(parent);
            else this.#validateConstruction(parent);
            movement = this.#priests.movementProcessType(type);
        }
        // 현재 종속 체인이 반환한 실제 Form이다.
        const form = this.#lookup.find(parent, movement);
        if (!form) return null;
        // 현재 Form PID가 가리키는 실제 Path 객체이다.
        const process = this.#core.processes.fromForm(form);
        if (!(process instanceof RawPathProcess) || process.pool !== this.pool || process.parent !== parent || process.form !== form) throw new Error('캠페인 현재54 Form/실제Host Path 계약 오류');
        return process;
    }
    // 현재 실제 Path의 저장소만 빌린다.
    currentRuntime(parent) { return this.currentPath(parent)?.runtime ?? null; }
    // 같은 Runtime의 두 대상 DWORD setter를 재사용한다.
    setTargets(parent, first, second) { const runtime = this.currentRuntime(parent); if (!runtime) return false; runtime.setTargets(first, second); return true; }
    // 재탐색이 현재 Path를 지울 수 있어 콜백 뒤에는 다시 읽지 않는다.
    retargetCurrent(parent) { const runtime = this.currentRuntime(parent); if (!runtime) return false; this.#effects.retarget(runtime, parent); return true; }
    // 실제 현재 후보용 임시 Path만 같은 BFS로 채우며 Form을 만들지 않는다.
    searchCandidate(temporary, parent, start, destination, flags) { return this.#search.search(temporary, parent, start, destination, flags); }
    // 실제 writer/공유 버퍼에 연결하고 공급된 Native 장치만 별도로 선택한다.
    transmission(header, buffer, transport, other = {}) {
        // 같은 World의 장기 물리/진단/전송 장치이다.
        const storage = this.#world.storage;
        // 생성 시점에 선택한 실제 Native writer 계약이다.
        const native = storage.usesNativeTransmissionDevices?.() ? (process, bytes, changed, force) => storage.serializeProcessForm(process, bytes, changed, force) : null;
        return makePathFormTransmission(this.pool, this.#core.types, header, buffer, transport, { ...other }, native);
    }
    // 같은 Start/Pool을 후속 의식 및 운반 모듈에 빌린다.
    get starter() { return this.#starter; }
    get pool() { return this.#core.pool; }

    // free Form만 원문의 실제 생성자/decoder/가상 Attach로 복원한다. live Form은 진단 뒤 반환한다.
    receivePacket(packet) {
        // 같은 World의 장기 물리/진단/전송 장치이다.
        const storage = this.#world.storage; const core = this.#core;
        if (this.pool.edition !== 'patch1078') throw new TypeError('Path 수신의 실제 PATCH 연결 필요');
        if (packet.length < 6) throw new RangeError('Path 수신의 실제 Form WORD 부족');
        if (core.processState.boss) storage.assertDisplay('!boss', '\\Ns\\O\\Pathprocess.cpp', 0x224);
        // 패킷의 실제 물리 범위에서만 WORD를 읽는 함수이다.
        const word = (at) => { if (at > packet.length || 2 > packet.length - at) throw new RangeError('Path 수신의 선택된 WORD 부족'); return packet[at] | (packet[at + 1] << 8); };
        // 현재 종속 체인이 반환한 실제 Form이다.
        const form = word(4);
        if (!form || form >= this.pool.capacity) throw new RangeError('Path 수신의 명시 Form 물리 범위');
        // 현재 부모/Form의 실제 슬롯 보기이다.
        const raw = this.pool.slot(form);
        if (!(raw[11] & 1)) { if (raw[10] >= core.types.length) throw new RangeError('Path 기존 Form의 현재 TYPE 범위'); storage.recordAssertionDetail(invalidUpdateDetail(core.types[raw[10]].name)); storage.assertDisplay('0', '\\Ns\\O\\Pathprocess.cpp', 0x24f); return; }
        // 원문 수신 헤더의 현재 부모 번호이다.
        const parent = word(6);
        if (parent >= this.pool.capacity) throw new RangeError('Path 수신의 부모 물리 범위');
        // 사제 월드가 읽은 현재 이동 프로세스 번호이다.
        const movement = this.#priests.movementProcessType(this.pool.slot(parent)[10]);
        if (movement >= core.types.length) throw new RangeError('Path 수신의 현재 이동 TYPE 범위');
        // 수신 생성자가 만든 아직 부착하지 않은 실제 Path이다.
        const pending = this.createByConstructor(core.types[movement].constructorAddress);
        if (!pending) throw new Error('Path 수신의 실제 TYPE 생성자 미연결');
        new RawPathData().read(pending.runtime, packet, this.pool, core.types, {
            stagePoint: this.#effects.stagePoint,
            metadata: (sid) => { const type = this.#effects.metadata(sid); return [Math.fround(type.speed), Math.fround(type.turningSpeed)]; }, now: () => core.processState.now,
            assertion: (expression, file, line) => storage.assertDisplay(expression, file, line), trace: (text) => storage.traceGraph(storage.gameAux.pathTraceLevel, text),
            clearAnimation: (sid) => { const process = core.processes.findEvent(sid, 0x5105); if (process) core.processes.kill(process, 0x40); },
        });
        // 부착 전 보존할 경로 꼭짓점 수이다.
        const count = pending.runtime.count; const first = count ? pending.runtime.point(0) : { x: 0, y: 0 };
        // 실제 가상 Attach가 반환한 현재 경로이다.
        const attached = RawPathProcess.attach(core.processes, pending, parent, 0, form);
        if ((attached ? attached.runtime.count : count) !== 0) { this.#world.unpop(parent, 0); const point = attached ? attached.runtime.point(0) : first; this.#world.pop(parent, point.x, point.y, 0); }
    }
}
