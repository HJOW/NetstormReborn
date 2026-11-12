// 이동 시작의 최상위 몸체(패치 00489080)를 옮긴 모듈이다(cpppj `o/RawPathStart.cpp/.h`, 10.78 전용). 같은 SID 풀·검색·서버 Host·부모 통지에 연결해 경로 프로세스를 만들고 부착한다.
// 순서: 입력 검사(부모 SID·유한 목적지) → 목적지 [0, 254] clamp → 부모 TYPE 으로 프로세스 종류 선택(`hooks.processType`)·부착 flags(Dude/마나 볼트/텔레포트는 0x10) →
//   사제 목적지 스냅(`snapPriestDestination`: 올림 칸 − 0.5) → 권한이 있으면 기존 같은 종류 경로(`lookup.find`)의 두 대상을 이어받고 시작점을 반 칸 보정한 뒤 지움 →
//   실제 생성(`createType`/`create`)·Init(`initializePath`)·두 대상·완료·명령 필드 기록 → 꼭짓점이 없으면 실패 정리(부모 발자국 주변의 막는 부류에 `clearBlocked` 후 재귀 한 번) →
//   `replaceStart` 면 시작의 연속 동일 꼭짓점을 현재 부모 좌표로 교체 → `prepare` → 부착(`RawPathProcess.attach`) → 부모 통지(`notifyStarted`).
//  - 상태(state): { clock, dudeType, teleportType, manaBoltType, priestType, geyserType, snapPriestDestination, retrying } 객체이며 호출마다 현재 값을 읽는다. `retrying` 은 실패 정리 재귀의 실제 guard 라서 이 객체에 쓴다(복사하지 않는다).
//  - 훅(hooks): processType(parentType) → 프로세스 번호, create(priest) → RawPathProcess, metadata(sid) → { speed, turningSpeed, defaultFrame }, clearBlocked(point, parentType, owner),
//    createType(processType) → RawPathProcess(파생 종류 39/40/42/24/41/38 용; 없으면 파생 선택은 거부). 필수는 processType·create·metadata·clearBlocked 이다.
//  - 부모 통지(notifyStarted): 텔레포트/마나 볼트/Dude 부모는 빈 몸체, 사제는 목적지 정수 칸의 finder 에서 마지막으로 만난 간헐천을 targetA 로 기록하고 사건 0x25c·0x25d 를 전부 지우고 현재 기본 프레임을 지정한다. Walker/웜/Bulf/벌룬은 간헐천 기록만 하고 반환한다.
//    그 밖의 부모는 미복원 경계로 거부한다.
// 10.72(CD) 분기(실패 정리 생략·시작점 보정 없음)는 옮기지 않았다. 이 모듈은 DOM 을 쓰지 않는다.
// 근거: cpppj `o/RawPathStart.cpp/.h`(기준선 1f00a25a 뒤 `20261109-53 CP` 의 `clock.Boss()` 반영), 원본 관측 pathstart-x86.tsv 10.78 256행·pathprocesstype-x86.tsv.
// (2026-11-13 WB04-E351: 6-P RawPathStart 이식으로 추가)
import { RawSquidFinder, RAW_FINDER_FLAGS } from './raw-squid-finder.js';
import { RawPathProcess } from './raw-path-process.js';

// 목적지 최대값, 반 칸, 올림 보정(float 0.9999), 실패 정리가 찾는 막는 부류 genus 이다. (2026-11-13 WB04-E351)
const MAXIMUM_DESTINATION = 254;
const HALF_CELL = 0.5;
const CELL_BIAS = Math.fround(0.9999);
const FAILURE_GENUS = 0x8c044200;
// 슬롯 필드 위치(TYPE +10·좌표 +14/+18·소유자 +34)와 타입 표 길이이다. (2026-11-13 WB04-E351)
const TYPE_AT = 10, X_AT = 14, Y_AT = 18, OWNER_AT = 34;
const TYPE_COUNT = 188;
// 내장 경로 프로세스 번호: 텔레포트 23·사제 54, 파생 비행 39·웜 40·벌룬 42·Dude 24·Bulf 41·Walker 38이다. (2026-11-13 WB04-E351)
const TELEPORT_PROCESS = 23, PRIEST_PROCESS = 54, FLY_PROCESS = 39, WORM_PROCESS = 40, BALLOON_PROCESS = 42, DUDE_PROCESS = 24, BULF_PROCESS = 41, WALKER_PROCESS = 38;
// 사제 시작이 지우는 두 회복 사건과 client 부착 flags 이다. (2026-11-13 WB04-E351)
const PRIEST_EVENTS = Object.freeze([0x25c, 0x25d]);
const CLIENT_ATTACH = 0x10;

// 비정렬 DWORD 를 부호 없이 읽는다. (2026-11-13 WB04-E351)
function readDword(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// DWORD 비트를 float32 로 읽는다. (2026-11-13 WB04-E351)
function floatAt(raw, offset) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, readDword(raw, offset), true);
    return view.getFloat32(0, true);
}

// 원본 0041d7a0 의 float32 에 double 편향을 더하고 0 쪽 절삭한 float 좌표이다. 비유한 좌표는 거부한다. (2026-11-13 WB04-E351)
function ceilPoint(point) {
    if (!Number.isFinite(point.x) || !Number.isFinite(point.y)) throw new RangeError('Path 시작 비정상 좌표');
    return { x: Math.fround(Math.trunc(point.x + CELL_BIAS)), y: Math.fround(Math.trunc(point.y + CELL_BIAS)) };
}

// 004ade70/49ae80 의 min/max 확장 뒤 finder 영역이다: 왼쪽/위는 절삭, 오른쪽/아래는 거의 올림이다. point 는 부모의 float32 좌표, footX/footY 는 현재 TYPE 발자국이다.
// 40e880/40e8f0 은 확장 입력만 먼저 1..255 로 자르고, 40e800 은 한 축이 0 이어도 원래 점 전체를 무효로 판정해 확장 입력만 영역으로 쓴다. 비유한 좌표나 음수 발자국은 거부한다. (2026-11-13 WB04-E351)
export function startFootprint(point, footX, footY) {
    if (!Number.isFinite(point.x) || !Number.isFinite(point.y) || footX < 0 || footY < 0) throw new RangeError('Path 시작 부모 발자국');
    const clamp = (value) => Math.min(Math.max(value, 1), 255);
    const other = { x: clamp(Math.fround(point.x - (footX - 1))), y: clamp(Math.fround(point.y - (footY - 1))) };
    const valid = point.x > 0 && point.y > 0 && point.x < 256 && point.y < 256;
    // 0 크기는 기준점보다 큰 반대쪽 끝도 허용한다.
    const low = valid ? { x: Math.min(point.x, other.x), y: Math.min(point.y, other.y) } : other;
    const high = valid ? { x: Math.max(point.x, other.x), y: Math.max(point.y, other.y) } : other;
    const end = ceilPoint(high);
    return { left: Math.trunc(low.x), top: Math.trunc(low.y), right: Math.trunc(end.x), bottom: Math.trunc(end.y) };
}

// 이동 시작 몸체이다. 모든 참조는 시작기보다 오래 살아야 한다. (2026-11-13 WB04-E351)
export class RawPathStart {
    // 유일한 실제 SID 저장소, 공간 해시, 현재 타입 표, 실제 Host·종속 조회·프레임 지정, 공유 상태, 효과 훅이다.
    #pool;
    #hash;
    #types;
    #host;
    #lookup;
    #frame;
    #state;
    #hooks;

    // 새 객체 생성/기존 삭제 이전에 같은 월드/필수 효과 계약을 확인한다. state 는 얕은 복사 없이 같은 객체를 참조한다. (2026-11-13 WB04-E351)
    constructor(pool, hash, types, host, lookup, frame, state, hooks) {
        if (pool?.edition !== 'patch1078' || host?.pool !== pool || lookup?.pool !== pool || frame?.pool !== pool || types.length !== TYPE_COUNT ||
            typeof hooks?.processType !== 'function' || typeof hooks.create !== 'function' || typeof hooks.metadata !== 'function' || typeof hooks.clearBlocked !== 'function') {
            throw new TypeError('Path 시작 월드/필수 효과 연결 누락');
        }
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#host = host;
        this.#lookup = lookup;
        this.#frame = frame;
        this.#state = state;
        this.#hooks = { ...hooks };
    }

    // 후속 월드 연결에서 다른 풀을 합성하지 않도록 같은 풀을 공개한다. (2026-11-13 WB04-E351)
    get pool() { return this.#pool; }

    // 같은 raw 위치를 읽어 callback 뒤의 현재 좌표를 사용한다. (2026-11-13 WB04-E351)
    #position(sid) {
        const raw = this.#pool.physical(sid);
        return { x: floatAt(raw, X_AT), y: floatAt(raw, Y_AT) };
    }

    // 부모 슬롯의 현재 좌표와 TYPE 발자국으로 finder 영역을 계산한다(순수 함수 startFootprint 에 위임). (2026-11-13 WB04-E351)
    #footprint(sid) {
        const record = this.#types[this.#pool.physical(sid)[TYPE_AT]];
        return startFootprint(this.#position(sid), record.footX, record.footY);
    }

    // 목적지 clamp → 기존 경로/시작 보정 → 생성/Init → 명령 필드 → 유효 검사 → Prepare/Attach → 부모 통지이다.
    // request = { parent, destination: { x, y }, replaceStart, helper, tailWord, tailByte, reserved }. 반환은 Host 가 소유한 RawPathProcess 이고, 꼭짓점이 없어 실패하면 실패 정리 재귀가 성공해도 외부 호출은 null 이다. (2026-11-13 WB04-E351)
    start(request) {
        const pool = this.#pool;
        const state = this.#state;
        if (!request.parent || request.parent >= pool.capacity || !Number.isFinite(request.destination.x) || !Number.isFinite(request.destination.y)) throw new RangeError('Path 시작 입력 범위');
        let destination = {
            x: Math.min(Math.max(request.destination.x, 0), MAXIMUM_DESTINATION),
            y: Math.min(Math.max(request.destination.y, 0), MAXIMUM_DESTINATION),
        };
        const parentType = pool.physical(request.parent)[TYPE_AT];
        const processType = this.#hooks.processType(parentType);
        const flags = (parentType === state.dudeType || parentType === state.manaBoltType || parentType === state.teleportType) ? CLIENT_ATTACH : 0;
        if (!flags && state.snapPriestDestination && parentType === state.priestType) {
            destination = ceilPoint(destination);
            destination = { x: Math.fround(destination.x - HALF_CELL), y: Math.fround(destination.y - HALF_CELL) };
        }
        const start = this.#position(request.parent);
        let first = 0;
        let second = 0;
        if (state.clock.boss) {
            const oldForm = this.#lookup.find(request.parent, processType);
            if (oldForm) {
                const old = this.#host.fromForm(oldForm);
                if (!(old instanceof RawPathProcess) || old.pool !== pool) throw new Error('기존 다른 파생 Path 수명 미복원 경계');
                // 패치는 기존 경로가 있으면 시작점을 목적지 쪽으로 반 칸 물린다.
                if (start.x <= destination.x) start.x = Math.fround(start.x - HALF_CELL);
                if (start.y <= destination.y) start.y = Math.fround(start.y - HALF_CELL);
                first = old.runtime.targetA;
                second = old.runtime.targetB;
                this.#host.kill(old, 0);
            }
        }
        const derived = [FLY_PROCESS, WORM_PROCESS, BALLOON_PROCESS, DUDE_PROCESS, BULF_PROCESS, WALKER_PROCESS];
        if (processType !== TELEPORT_PROCESS && processType !== PRIEST_PROCESS && !derived.includes(processType)) throw new Error('다른 파생 이동 생성/Init 미복원 경계');
        if (derived.includes(processType) && typeof this.#hooks.createType !== 'function') throw new Error('파생 이동의 실제 생성자 미연결');
        const owned = typeof this.#hooks.createType === 'function' ? this.#hooks.createType(processType) : this.#hooks.create(processType === PRIEST_PROCESS);
        if (!owned) throw new Error('Path 시작 생성 실패(메모리 부족)');
        if (owned.pool !== pool) throw new TypeError('Path 시작 생성 풀 불일치');
        if (owned.movementType() !== processType) throw new TypeError('Path 시작의 실제 생성 종류 불일치');
        owned.initializePath(request.parent, destination, start, 0);
        const runtime = owned.runtime;
        runtime.setTargets(first, second);
        runtime.setFinished(false);
        runtime.setCommandFields(request.helper, request.tailWord, request.tailByte, destination);
        if (runtime.count < 1) {
            let cleared = false;
            const finder = new RawSquidFinder(pool, this.#hash, this.#types);
            // 실제 finder 의 현재 후보 순서로 해당 genus 마다 현재 부모 위치/타입/소유자를 다시 읽는다.
            for (let candidate = finder.begin(this.#footprint(request.parent), RAW_FINDER_FLAGS.skipSurface); candidate !== 0; candidate = finder.next()) {
                if ((this.#types[pool.physical(candidate)[TYPE_AT]].flags2 & FAILURE_GENUS) !== 0) {
                    const current = pool.physical(request.parent);
                    this.#hooks.clearBlocked(this.#position(request.parent), current[TYPE_AT], current[OWNER_AT]);
                    cleared = true;
                }
            }
            if (state.retrying) {
                state.retrying = false;
                return null;
            }
            if (cleared) {
                state.retrying = true;
                this.start(request);
            }
            return null;
        }
        if (request.replaceStart) {
            const rounded = ceilPoint(this.#position(request.parent));
            // 시작의 연속 동일 꼭짓점만 현재 부모 좌표로 교체하고 시간/방향은 보존한다.
            for (let index = 0; index < runtime.count; index += 1) {
                const point = runtime.point(index);
                if (point.x !== rounded.x || point.y !== rounded.y) break;
                runtime.replacePoint(index, start);
            }
        }
        const meta = this.#hooks.metadata(request.parent);
        owned.prepare(0, meta.speed, meta.turningSpeed);
        const result = RawPathProcess.attach(this.#host, owned, request.parent, flags);
        if (!result) throw new Error('Path 시작 중 부모 소멸 계약 미복원 경계');
        this.#notifyStarted(request.parent, destination, result);
        return result;
    }

    // 지원된 원본 부모의 두 가상 몸체를 실제 같은 공간/Host/Frame 으로 실행한다. 사제만 사건 0x25c/0x25d 를 지우고 기본 프레임을 지정한다. (2026-11-13 WB04-E351)
    #notifyStarted(parent, destination, process) {
        const pool = this.#pool;
        const state = this.#state;
        const type = pool.physical(parent)[TYPE_AT];
        // 세 실제 부모의 +44/+40 은 480260/4437f0 빈 몸체다.
        if (type === state.teleportType || type === state.manaBoltType || type === state.dudeType) return;
        const kind = process.movementType();
        const carrier = kind === WALKER_PROCESS || kind === WORM_PROCESS || kind === BULF_PROCESS || kind === BALLOON_PROCESS;
        if (type !== state.priestType && !carrier) throw new Error('다른 부모 이동 통지 미복원 경계');
        const x = Math.trunc(destination.x);
        const y = Math.trunc(destination.y);
        const finder = new RawSquidFinder(pool, this.#hash, this.#types);
        // 원본은 첫 일치에서 멈추지 않고 마지막 간헐천을 targetA 에 기록한다.
        for (let candidate = finder.begin({ left: x, top: y, right: x, bottom: y }, RAW_FINDER_FLAGS.skipSurface); candidate !== 0; candidate = finder.next()) {
            if (pool.physical(candidate)[TYPE_AT] === state.geyserType) process.runtime.setTargets(candidate, process.runtime.targetB);
        }
        // 세 Carrier 의 427750 공통 +44 뒤 +40 은 4437f0 이다.
        if (carrier) return;
        // 두 사건 각각을 처음부터 다시 검색하므로 삭제 callback 이 추가한 같은 사건도 원본처럼 처리한다.
        for (const event of PRIEST_EVENTS) {
            for (let regular = this.#host.findEvent(parent, event); regular; regular = this.#host.findEvent(parent, event)) this.#host.kill(regular, 0);
        }
        this.#frame.set(parent, this.#hooks.metadata(parent).defaultFrame, 0);
    }
}
