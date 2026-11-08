// 건설용 사제와 자원 운반 객체를 작업장/공급원에서 생성해 목표로 보내는 전체 이동(Construction.cpp 00443000)을 옮긴 모듈이다(cpppj `o/RawConstructionBuilder.cpp/.h`, 10.78).
//  - move(player, target, timeLow, timeHigh): 목표의 (타입, 좌표)로 기준점(anchor)을 찾아 없으면 5.0 을 반환한다. 있으면 건설 사제(builder 타입)를 클라이언트 플래그 2 로 생성하고 소유자를 정한 뒤, 목표 발자국의
//    위쪽 가운데를 floor(좌표+0.5)로 반올림한 도착점으로 삼고 기준점 genus 가 접근 mask 와 겹치면 mask|1, 아니면 1 의 플래그로 접근점을 구해 첫 객체를 Pop(0x10)하고 경로를 시작한다(실패는 5.0 이며 앞선 효과는 되돌리지 않는다).
//    요청 도착 시각(timeLow/High 의 double)이 0 이 아니면 (요청−now)×속도/(도착−now)(10.78: 곱한 뒤 나눔)를 경로 속도로 전달한다. 이어 현재 목표 좌표·타입과 기준점 소유자로 자원 선택(chooseSources 또는 실제 에너지 공급)과
//    현재 목표 좌표로 정렬을 한 뒤 공유 공급원 목록을 처음부터 훑어 첫 0 이 아닌 항목을 0 으로 소비하며 하나씩 운반 객체(carrier 타입)를 만든다: 단어(index)·(현재 다리 타입이면 표면 알림)·기준점 소유자·공급원 타입의 첫 프레임·
//    공급원 중심(좌표−발자국/2)에서 Pop(0)·타이머 확보에 성공하면 Regular(event 0, payload) 부착·목표로 이동(경로 실패는 무시)한다. 마지막에 첫 경로의 현재 도착 시각을 반환한다.
//    생성·경로 경계 뒤에는 현재 목표/기준점/타입/공급원 좌표를 다시 읽는다.
//  - takeSource(): 첫 0 이 아닌 공급원 DWORD 를 0 으로 바꾸어 소비한다(signed 개수가 0 이하이면 0). 실제 에너지 공급이 연결되면 그 마지막 목록을 쓴다.
//  - makeConstructionBuilderHooks(pool, anchor, factory, owner, hooks, process): 기준점/factory/일반 소유자와 선택적 Regular 부착을 같은 풀의 실제 모듈로 잇는다. makeConstructionBuilderLocalHooks / makeConstructionBuilderNoticeHooks:
//    로컬 배치·통지의 moveBuilder 를 이 이동에 잇는다(double 결과는 버린다).
// 10.72(CD) 경로(소유자 +32·프레임 BYTE·나눈 뒤 곱함)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E142: 건설 사제 이동 이식으로 추가)
import { SquidPostPopList } from './squid-postpop-state.js';
import { makePlayerAnchorQuery } from './raw-player-placement-anchor.js';

// raw 타입(+10)·단어(+12)·좌표(+14/+18)·PATCH 소유자(+34)·프레임 DWORD(+36) 위치이다. (2026-11-09 WB04-E142)
const TYPE = 10;
const WORD = 12;
const X_AT = 14;
const Y_AT = 18;
const OWNER = 34;
const FRAME = 36;
// 기준점/첫 경로 실패의 원본 double 반환값, 처음 객체를 놓는 플래그, 클라이언트 생성 플래그, 타입 표 크기, 프레임 표 칸 수이다. (2026-11-09 WB04-E142)
const FAILURE_TIME = 5;
const FIRST_POP = 0x10;
const CLIENT_CREATE = 2;
const TYPE_COUNT = 188;
const FIRST_FRAME_COUNT = 16;
// 필수 하위 효과 이름이다(chooseSources/sortSources 는 실제 에너지 공급이 없을 때만 필요하다). (2026-11-09 WB04-E142)
const HOOK_NAMES = Object.freeze(['anchor', 'create', 'setOwner', 'approach', 'pop', 'move', 'speed', 'notifySurface', 'reserveTimer', 'addTimer']);

// 생성/자원 선택 경계 뒤에도 다시 읽는 전역과 공유 공급원 목록이다: builderType(00541270)·carrierType(005411fc)·bridgeType(005411a0, 82)·approachMask(005424b8)·
// timerPayload(00532750, 0.05f)·now(0055b4d0, 게임 시각 double)·sources(00592f4c/54, SquidPostPopList). 사용: 월드마다 하나를 두고 RawConstructionBuilder 에 넘긴다. (2026-11-09 WB04-E142)
export class ConstructionBuilderState {
    builderType = 0;
    carrierType = 0;
    bridgeType = 82;
    approachMask = 0;
    timerPayload = Math.fround(0.05);
    now = 0;
    sources = new SquidPostPopList();
}

// 정렬되지 않은 리틀엔디언 값을 폭만큼 읽는다. (2026-11-09 WB04-E142)
function read(raw, offset, width = 4) {
    let value = 0;
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value;
}

// 정렬되지 않은 좌표의 비트를 단정도로 해석한다. (2026-11-09 WB04-E142)
const view = new DataView(new ArrayBuffer(8));
function coordinate(raw, offset) {
    view.setUint32(0, read(raw, offset), true);
    return view.getFloat32(0, true);
}

// raw WORD/프레임 쓰기는 지정 폭의 낮은 바이트만 바꾸며 주변 필드를 보존한다. (2026-11-09 WB04-E142)
function write(raw, offset, value, width) {
    for (let index = 0; index < width; index += 1) raw[offset + index] = Math.floor(value / 2 ** (8 * index)) & 0xff;
}

// 실제 발자국/중심 계산 전에 호스트의 비유한 좌표와 잘못된 크기를 거부한다. (2026-11-09 WB04-E142)
function validate(x, y, type) {
    if (!Number.isFinite(x) || !Number.isFinite(y) || type.footX < 1 || type.footY < 1 || type.footX > 256 || type.footY > 256) throw new RangeError('건설 사제 좌표/발자국 오류');
}

// 1~255 로 자른다. (2026-11-09 WB04-E142)
function clamp(value) {
    return Math.min(Math.max(value, 1), 255);
}

// 건설 사제 이동의 실행 계층이다. pool(10.78 SidPool)·types(188칸)·metadata(타입별 { speed, sourceFrameIndex, firstFrames[16] }, 같은 길이)·state·hooks·supply·reads 는 객체보다 오래 살아야 한다.
// reads 는 실제 Session 이 같은 전역을 읽게 하는 현재 getter 묶음({ builderType, carrierType, bridgeType, approachMask, timerPayload, now })이고 독립 검사는 기존 값형 state 를 쓴다.
// 경로 객체는 { currentStep, arrivalTimes } 또는 { readArrival(), prepare }이다. 사용: new RawConstructionBuilder(pool, types, metadata, state, hooks).move(player, targetSid, low, high). (2026-11-09 WB04-E142)
export class RawConstructionBuilder {
    #pool;
    #types;
    #metadata;
    #state;
    #hooks;
    #supply;
    #reads;

    // 미연결 하위 효과는 첫 생성/소유자 쓰기 전에 확인한다. (2026-11-09 WB04-E142)
    constructor(pool, types, metadata, state, hooks, supply = null, reads = {}) {
        if (pool?.edition !== 'patch1078') throw new TypeError('건설 사제는 10.78 SidPool 만 받습니다.');
        if (!Array.isArray(types) || types.length !== TYPE_COUNT || !Array.isArray(metadata) || metadata.length !== types.length || !state) throw new TypeError('건설 사제 자료/효과 연결 오류');
        for (const name of HOOK_NAMES) {
            if (typeof hooks?.[name] !== 'function') throw new TypeError('건설 사제 자료/효과 연결 오류');
        }
        if ((typeof hooks.chooseSources !== 'function' || typeof hooks.sortSources !== 'function') && !supply) throw new TypeError('건설 사제 자료/효과 연결 오류');
        if (supply && supply.pool !== pool) throw new TypeError('건설 사제 자료/효과 연결 오류');
        this.#pool = pool;
        this.#types = types;
        this.#metadata = metadata;
        this.#state = state;
        this.#hooks = hooks;
        this.#supply = supply;
        this.#reads = reads ?? {};
    }

    // 동일 풀 연결 검사용 참조이다. (2026-11-09 WB04-E142)
    get pool() { return this.#pool; }

    // DWORD 가 풀 안인지 먼저 확인해 임의 WORD SID 로 감기지 않도록 한다. 예약 슬롯 0 도 조회는 허용한다. (2026-11-09 WB04-E142)
    #checked(sid) {
        if (sid >= this.#pool.capacity) throw new RangeError('건설 사제 SID 범위 오류');
        return sid;
    }

    // 현재 raw 타입을 읽어 발자국/genus 자료를 찾는다. (2026-11-09 WB04-E142)
    #type(sid) {
        const type = this.#pool.slot(sid)[TYPE];
        if (type >= this.#types.length) throw new RangeError('건설 사제 타입 오류');
        return this.#types[type];
    }

    // 경계 호출 뒤의 현재 raw 타입으로 속도/프레임 메타 자료를 다시 읽는다. (2026-11-09 WB04-E142)
    #meta(sid) {
        const type = this.#pool.slot(sid)[TYPE];
        if (type >= this.#metadata.length) throw new RangeError('건설 사제 메타 타입 오류');
        return this.#metadata[type];
    }

    // 현재 발자국의 두 끝을 지도 범위로 제한해 [left, top, right, bottom] 사각형을 만든다. (2026-11-09 WB04-E142)
    #footprint(sid) {
        const raw = this.#pool.slot(sid);
        const x = coordinate(raw, X_AT);
        const y = coordinate(raw, Y_AT);
        const type = this.#type(sid);
        validate(x, y, type);
        const px = clamp(Math.fround(x + 1 - type.footX));
        const py = clamp(Math.fround(y + 1 - type.footY));
        const valid = x > 0 && y > 0 && x < 256 && y < 256;
        return [valid ? Math.min(x, px) : px, valid ? Math.min(y, py) : py, valid ? Math.max(x, px) : px, valid ? Math.max(y, py) : py];
    }

    // 원본 발자국의 위쪽 가운데를 구하고 floor(좌표+0.5)로 반올림한다. (2026-11-09 WB04-E142)
    #destination(sid) {
        const [left, top, right] = this.#footprint(sid);
        const middle = Math.fround((right - left) * 0.5 + left);
        return { x: Math.fround(Math.floor(middle + 0.5)), y: Math.fround(Math.floor(top + 0.5)) };
    }

    // 004adea0: 좌표에서 발자국 폭/높이의 절반을 뺀다((크기−1)/2 가 아니다). (2026-11-09 WB04-E142)
    #center(sid) {
        const raw = this.#pool.slot(sid);
        const x = coordinate(raw, X_AT);
        const y = coordinate(raw, Y_AT);
        const type = this.#type(sid);
        validate(x, y, type);
        return { x: Math.fround(x - type.footX * 0.5), y: Math.fround(y - type.footY * 0.5) };
    }

    // 속도 조정/운반 이동 뒤에도 현재 경로 단계로 시각을 다시 읽는다. (2026-11-09 WB04-E142)
    #arrival(path) {
        if (typeof path.readArrival === 'function') return path.readArrival();
        if (path.currentStep >= path.arrivalTimes.length) throw new RangeError('건설 사제 경로 시각 인덱스 오류');
        return path.arrivalTimes[path.currentStep];
    }

    // factory 가 새 객체를 실제 풀 안에 만들었는지 효과 전달 전에 확인한다. (2026-11-09 WB04-E142)
    #create(type) {
        if (type >= this.#types.length) throw new RangeError('건설 사제 생성 타입 오류');
        const sid = this.#hooks.create(type, CLIENT_CREATE);
        if (sid < 5 || sid >= this.#pool.capacity || (this.#pool.slot(sid)[11] & 1) !== 0) throw new Error('건설 사제 생성 결과 오류');
        return sid;
    }

    // 원본 cursor 는 개수를 줄이지 않고 활성 저장소의 첫 0 이 아닌 항목만 지운다. 경계가 목록을 바꾼 경우에도 매번 처음부터 현재 순서로 소비한다. (2026-11-09 WB04-E142)
    takeSource() {
        if (this.#supply) return this.#supply.takeSource();
        const sources = this.#state.sources;
        const count = sources.readCount();
        if ((count | 0) <= 0) return 0;
        const entries = sources.storage();
        if (count > entries.length) throw new RangeError('건설 공급원 목록 범위 오류');
        for (let index = 0; index < count; index += 1) {
            const sid = entries[index];
            if (!sid) continue;
            entries[index] = 0;
            return sid;
        }
        return 0;
    }

    // 생성/경로 실패의 앞선 효과를 유지하고, 공급원 경계 뒤에는 현재 목표/기준점/타입을 다시 읽는다. 반환은 원본의 숨은 double 결과 포인터이다. (2026-11-09 WB04-E142)
    move(player, target, timeLow, timeHigh) {
        const hooks = this.#hooks;
        const state = this.#state;
        const reads = this.#reads;
        const raw = this.#pool.slot(target);
        const anchorValue = hooks.anchor(player, raw[TYPE], coordinate(raw, X_AT), coordinate(raw, Y_AT));
        if (!anchorValue) return FAILURE_TIME;
        const anchor = this.#checked(anchorValue);
        const first = this.#create(reads.builderType ? reads.builderType() : state.builderType);
        hooks.setOwner(first, player);
        const destination = this.#destination(target);
        const mask = (reads.approachMask ? reads.approachMask() : state.approachMask) >>> 0;
        const flags = ((this.#type(anchor).flags2 & mask) !== 0) ? (mask | 1) >>> 0 : 1;
        const start = hooks.approach(this.#footprint(anchor), destination.x, destination.y, flags);
        hooks.pop(first, start.x, start.y, FIRST_POP);
        const path = hooks.move(first, destination.x, destination.y);
        if (!path) return FAILURE_TIME;
        view.setUint32(0, timeLow >>> 0, true);
        view.setUint32(4, timeHigh >>> 0, true);
        const requested = view.getFloat64(0, true);
        if (requested !== 0) {
            const arrival = this.#arrival(path);
            const speed = this.#meta(first).speed;
            // 패치는 먼저 속도를 곱한다. x87 중간값을 단정도로 조기 절단하지 않는다.
            const now = reads.now ? reads.now() : state.now;
            const rate = ((requested - now) * speed) / (arrival - now);
            hooks.speed(path, Math.fround(rate), 0);
        }
        const current = this.#pool.slot(target);
        let demand = '';
        if (this.#supply) demand = this.#supply.evaluate(demand, coordinate(current, X_AT), coordinate(current, Y_AT), current[TYPE], this.#pool.slot(anchor)[OWNER]).output;
        else hooks.chooseSources(coordinate(current, X_AT), coordinate(current, Y_AT), current[TYPE], this.#pool.slot(anchor)[OWNER]);
        const sorted = this.#pool.slot(target);
        if (this.#supply) this.#supply.sortSources(demand, coordinate(sorted, X_AT), coordinate(sorted, Y_AT));
        else hooks.sortSources(coordinate(sorted, X_AT), coordinate(sorted, Y_AT));
        let index = 0;
        // first 의 경로 객체를 보관하며 운반 객체의 경로 실패는 다음 공급원 처리를 막지 않는다.
        for (let sourceValue = this.takeSource(); sourceValue; sourceValue = this.takeSource()) {
            const source = this.#checked(sourceValue);
            const carrier = this.#create(reads.carrierType ? reads.carrierType() : state.carrierType);
            write(this.#pool.allocated(carrier), WORD, index, 2);
            if (this.#pool.slot(carrier)[TYPE] === (reads.bridgeType ? reads.bridgeType() : state.bridgeType)) hooks.notifySurface(carrier);
            hooks.setOwner(carrier, this.#pool.slot(anchor)[OWNER]);
            const frameIndex = this.#meta(source).sourceFrameIndex;
            const meta = this.#meta(carrier);
            if (frameIndex >= meta.firstFrames.length) throw new RangeError('건설 운반 프레임 표 인덱스 오류');
            write(this.#pool.allocated(carrier), FRAME, meta.firstFrames[frameIndex] >>> 0, 4);
            const position = this.#center(source);
            hooks.pop(carrier, position.x, position.y, 0);
            if (hooks.reserveTimer()) hooks.addTimer(carrier, 0, reads.timerPayload ? reads.timerPayload() : state.timerPayload);
            hooks.move(carrier, destination.x, destination.y);
            index += 1;
        }
        return this.#arrival(path);
    }
}

// 같은 풀의 실제 기준점/factory/일반 소유자 지정과 선택적으로 Regular/Kernel 를 연결한다. 파생 owner 재정의는 별도 분배한다. 풀이 다르면 거부한다. (2026-11-09 WB04-E142)
export function makeConstructionBuilderHooks(pool, anchor, factory, owner, hooks, process = null) {
    if (pool !== anchor?.pool || pool !== factory?.pool || pool !== owner?.pool || (process && pool !== process.pool)) throw new TypeError('건설 사제 어댑터의 풀 연결 오류');
    const connected = {
        ...hooks,
        anchor: makePlayerAnchorQuery(pool, anchor),
        create: (type, flags) => factory.create(type, flags),
        setOwner: (sid, player) => { owner.set(sid, player); },
    };
    if (process) connected.addTimer = (sid, event, payload) => { process.addRegular(sid, event, payload); };
    return connected;
}

// double 반환값을 사용하지 않는 로컬 배치 경계를 원본 전체 이동에 연결한다. (2026-11-09 WB04-E142)
export function makeConstructionBuilderLocalHooks(pool, builder, hooks) {
    if (pool !== builder?.pool) throw new TypeError('로컬 건설 사제 어댑터의 풀 연결 오류');
    return { ...hooks, moveBuilder: (player, sid, low, high) => { builder.move(player, sid, low, high); } };
}

// 통지의 double 시각 비트를 원래 순서로 실제 생성/이동에 전달한다. (2026-11-09 WB04-E142)
export function makeConstructionBuilderNoticeHooks(pool, builder, hooks) {
    if (pool !== builder?.pool) throw new TypeError('통지 건설 사제 어댑터의 풀 연결 오류');
    return { ...hooks, moveBuilder: (player, sid, low, high) => { builder.move(player, sid, low, high); } };
}
