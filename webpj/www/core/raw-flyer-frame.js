// 맨 오 워 수명/프레임과 공통 비행 제어(이동·공격·표적·착륙·통지)를 같은 raw 필드로 실행하는 모듈이다(cpppj `o/RawFlyerFrame.cpp/.h`, 10.78 전용: 0045d630 수명 wrapper·0045c9d0 공통 Frame·0045bf50 맨 오 워 프레임·0045b6f0 표적 해제·0045b760 통지·0045b7a0 홈 통지).
//  - new RawFlyerFrame(pool, types, details, soundFrames, globals, hooks): pool 은 같은 PATCH SidPool, types = 188칸 TYPE({ footX, footY }, 만들 때 사본), details = 188칸 { range(float 사거리), sound(firingSound 토큰) }(만들 때 사본),
//    soundFrames = 길이 188 Uint32Array/배열(타입별 마지막 재생 프레임 장부, 호출자와 공유하며 이 모듈이 쓴다), globals = { server(), boss(), checking(), now(), frame(), sunType(), windType(), rainType(), sunAviaryProcess(), soundProcess() } 는 모두 현재 값을 읽는 함수이고,
//    hooks 는 원문의 큰 하위 함수와 가상 호출이다: assertion(line), robust(fields), sidRobust(sid, live), destroy(sid, flags), finish(fields), otherFrame(fields)→int32, offset(sid, home)→[x, y], scatter(target, parent)→[x, y], refreshTime(sid), hit(sid, target), removeSound(sid, type),
//    unpop(sid, flags), pop(sid, x, y, flags), repop(sid, flags), movedCell(sid), checkForms(sid), mayTarget(sid, target), closerCount(target, sid), capacity(sid), findTarget(fields), addDepend(target, form), removeDepend(form, target), refreshForm(fields), returnDelay(fields)→초,
//    command(sid, command), sunProcess(sid, type)→SunAviaryProcessFields, refreshSun(sun), homeNotice(sid, a, b), runCount(sid)→int32, launch(sid, target, flags), sound(x, y, token), sunInitialize(fields), sunNotify(fields, message, info1, info2).
//    선택하지 않은 경계는 요구하지 않으며 선택된 누락은 앞선 쓰기를 보존하고 거부한다.
//  - initialize(fields, rain): 0045bf00/0045b800 — 맨 오 워면 수명 타이머 104 = now + 60, 공격·착륙 0, 초기화 표식 1, 최초 대기 72 = now + 20. frame(fields): 0045d630 — 맨 오 워 수명(boss 만: 부모 정지이거나 수명이 지나면 부모 삭제 0x200000) 뒤 baseFrame.
//  - baseFrame(fields): 0045c9d0 — 초기화·부모 상태·진단 관문 뒤 경과 시간이 양수이면 귀환 기한(훨리기그)·표적 위치 계산(float 저장 벡터 합·clamp·거리·비율)·공격/착륙, 이어서 프레임(맨 오 워 rainFrame, 그 밖 otherFrame)·Unpop→프레임 쓰기→Pop(0x40)→칸이 바뀌면 movedCell,
//    서버면 표적 유지 검사(사거리·가까운 비행체 수·자리)와 표적 찾기를 한다. 효과 뒤 필드/전역/슬롯은 원문 위치에서 다시 읽는다. rainFrame(fields): 0045bf50 — 0.1초 애니메이션 타이머, 프레임 6·표적·공격이면 발사와 타입별 소리 억제, 다음 프레임 = (프레임 + 1) % 글자 수.
//  - clearTarget(fields): 0045b6f0 — 표적 의존 해제·dirty 4·표적 0·통지(1)·Form 갱신. notify/baseNotify: 0045b760(메시지 2 서버만 표적 해제, 전용 훨리기그 표는 sunNotify 로 먼저 분배). parentDying(fields): 0045b7a0 — 더스트 데빌은 생략하고 살아 있는 홈만 가상 34(0, 0). checkForms(parent): 004ad730.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E355: 7-P1 RawFlyerFrame 이식으로 추가)
import { checkRawObjectForms } from './raw-checking-diagnostics.js';

// outer 기준 BYTE 오프셋이다: dirty·홈·표적·속도·반경·마지막 갱신·귀환 기한·표적 검색(64)·최초 20초 타이머(72)·공격·초기화 표식·착륙·애니메이션·수명이다. (2026-11-13 WB04-E355)
const DIRTY = 8, TARGET = 12, SPEED = 16, RADIUS = 20, LAST = 32, RETURN = 40, SEARCH = 64, INITIAL_DELAY = 72, ATTACK = 80, INITIALIZED = 84, LANDING = 88, ANIMATION = 96, LIFETIME = 104;
// 귀환 홈 확인이 비교하는 부모 DWORD 위치(inner 부모 전체 DWORD)이다. (2026-11-13 WB04-E355)
const PARENT_DWORD = 56;
// 외부 가상 표 값: 맨 오 워(0x509f60)와 훨리기그(0x509f24)이다. (2026-11-13 WB04-E355)
const RAIN_TABLE = 0x509f60;
const SUN_TABLE = 0x509f24;
// 실제 00509c58 의 double(0.1f 를 배정도로 본 값)이며 0.1 로 줄이지 않는다. (2026-11-13 WB04-E355)
const RAIN_DELAY = 0.10000000149011612;
// 이동 거리 하한(float 0.0001 을 배정도로 본 값)이다. (2026-11-13 WB04-E355)
const MIN_DISTANCE = 0.00009999999747378752;
// TYPE 표 크기(PATCH)이다. (2026-11-13 WB04-E355)
const TYPE_COUNT = 188;

// 비정렬 값을 리틀엔디언 부호 없는 수로 읽는다. (2026-11-13 WB04-E355)
function read(raw, at, width = 4) {
    let value = 0;
    // 필드 폭만 읽는다.
    for (let index = 0; index < width; index += 1) value += raw[at + index] * 2 ** (8 * index);
    return value;
}

// 지정 폭만 쓴다(다른 payload 를 건드리지 않음). (2026-11-13 WB04-E355)
function put(raw, at, value, width = 4) {
    // 낮은 바이트부터 지정 폭만 쓴다.
    for (let index = 0; index < width; index += 1) raw[at + index] = Math.floor(value / 2 ** (8 * index)) & 0xff;
}

// float/double 비트 변환용 공용 보기이다. (2026-11-13 WB04-E355)
const SCRATCH = new DataView(new ArrayBuffer(8));

// 필드의 raw double 을 비트 그대로 읽는다. (2026-11-13 WB04-E355)
function time(fields, at) {
    SCRATCH.setUint32(0, read(fields.bytes, at), true);
    SCRATCH.setUint32(4, read(fields.bytes, at + 4), true);
    return SCRATCH.getFloat64(0, true);
}

// double 의 두 DWORD 만 쓴다. (2026-11-13 WB04-E355)
function putTime(fields, at, value) {
    SCRATCH.setFloat64(0, value, true);
    put(fields.bytes, at, SCRATCH.getUint32(0, true));
    put(fields.bytes, at + 4, SCRATCH.getUint32(4, true));
}

// 현재 raw float 필드의 입력 비트를 읽는다. (2026-11-13 WB04-E355)
function float(raw, at) {
    SCRATCH.setUint32(0, read(raw, at), true);
    return SCRATCH.getFloat32(0, true);
}

// 부모/표적의 Pop 좌표 한 쌍이다. (2026-11-13 WB04-E355)
function position(raw) {
    return [float(raw, 14), float(raw, 18)];
}

// 실제 WORD 표적을 필드에서 다시 읽는다. (2026-11-13 WB04-E355)
function targetOf(fields) {
    return read(fields.bytes, TARGET, 2);
}

// 실제 위치 검사는 0/256 경계를 제외하며 NaN 도 거부한다. (2026-11-13 WB04-E355)
function valid(point) {
    return point[0] > 0 && point[1] > 0 && point[0] < 256 && point[1] < 256;
}

// 원본 Point.clamp 의 축마다 1..255 포함 경계이다(비교 순서 보존: NaN 은 1 이 된다). (2026-11-13 WB04-E355)
function clamp(point) {
    // 각 축을 1 이상으로 올리고 255 를 넘으면 255 로 줄인다.
    for (let axis = 0; axis < 2; axis += 1) {
        point[axis] = point[axis] >= 1 ? point[axis] : 1;
        if (point[axis] > 255) point[axis] = 255;
    }
}

// 원본 Point 거리의 double 곱/합/제곱근이며 float delta 와 구별한다. (2026-11-13 WB04-E355)
function distance(first, second) {
    const x = first[0] - second[0];
    const y = first[1] - second[1];
    return Math.sqrt(x * x + y * y);
}

// 원본 CRT 정상 좌표의 0방향 절삭이다(범위 밖·비유한은 거부). (2026-11-13 WB04-E355)
function truncate(value) {
    if (!Number.isFinite(value) || value < -2147483648 || value >= 2147483648) throw new RangeError('비행 좌표 ftol 정상 범위 오류');
    return Math.trunc(value);
}

// 선택된 원문 하위가 비어 있으면 앞선 쓰기를 유지하고 명시 거부한다. (2026-11-13 WB04-E355)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('비행 Frame 실제 하위 몸체 미연결');
    return hook;
}

// 두 좌표 쌍이 모두 같은지 본다(NaN 은 같지 않음). (2026-11-13 WB04-E355)
function samePoint(first, second) {
    return first[0] === second[0] && first[1] === second[1];
}

// 비행 제어 로직이다. (2026-11-13 WB04-E355)
export class RawFlyerFrame {
    #pool;
    #types;
    #details;
    #soundFrames;
    #globals;
    #hooks;

    // PATCH 와 188칸 TYPE/보충 표/장부를 확인하고 TYPE·보충 표는 만들 때 사본을 보관한다. (2026-11-13 WB04-E355)
    constructor(pool, types, details, soundFrames, globals, hooks) {
        if (pool?.edition !== 'patch1078' || types?.length !== TYPE_COUNT || details?.length !== TYPE_COUNT || soundFrames?.length !== TYPE_COUNT) {
            throw new TypeError('비행 Frame은 PATCH188의 같은 TYPE/장부만 사용합니다');
        }
        // 모든 전역은 처리 도중 바뀔 수 있어 값이 아니라 현재 값 조회 함수여야 한다.
        for (const name of ['server', 'boss', 'checking', 'now', 'frame', 'sunType', 'windType', 'rainType', 'sunAviaryProcess', 'soundProcess']) {
            if (typeof globals?.[name] !== 'function') throw new TypeError(`비행 Frame 현재 전역 조회기 누락: ${name}`);
        }
        this.#pool = pool;
        this.#types = types.map((type) => ({ footX: type.footX, footY: type.footY }));
        this.#details = details.map((detail) => ({ range: detail.range, sound: detail.sound }));
        this.#soundFrames = soundFrames;
        this.#globals = globals;
        this.#hooks = hooks ?? {};
    }

    // Host 가 같은 풀인지 확인할 수 있게 공개한다. (2026-11-13 WB04-E355)
    get pool() { return this.#pool; }

    // 기본 초기화와 맨 오 워 수명 초기화의 실제 쓰기 순서이다. (2026-11-13 WB04-E355)
    initialize(fields, rain) {
        if (rain) putTime(fields, LIFETIME, this.#globals.now() + 60);
        put(fields.bytes, ATTACK, 0);
        put(fields.bytes, LANDING, 0);
        put(fields.bytes, INITIALIZED, 1);
        putTime(fields, INITIAL_DELAY, this.#globals.now() + 20);
    }

    // 수명은 boss 일 때만 검사한다. server 가 아니어도 boss 이면 삭제한다. (2026-11-13 WB04-E355)
    frame(fields) {
        const globals = this.#globals;
        const rain = read(fields.bytes, 0) === RAIN_TABLE;
        if (rain) {
            if (!read(fields.bytes, INITIALIZED)) this.initialize(fields, true);
            if (globals.boss() && ((this.#pool.slot(fields.parent)[40] & 0x20) || time(fields, LIFETIME) <= globals.now())) {
                need(this.#hooks.destroy)(fields.parent, 0x200000);
                return;
            }
        }
        this.baseFrame(fields);
    }

    // 현재 TYPE 의 발자국과 좌표를 사용하는 004adea0 이다. (2026-11-13 WB04-E355)
    #center(sid) {
        const raw = this.#pool.slot(sid);
        const type = this.#types[raw[10]];
        if (!type) throw new RangeError('비행 Frame TYPE 번호가 표 밖입니다');
        return [Math.fround(float(raw, 14) - type.footX * 0.5), Math.fround(float(raw, 18) - type.footY * 0.5)];
    }

    // 현재 표적의 의존 해제가 재진입으로 필드를 바꾸면 그 뒤 WORD 를 다시 덮어쓴다. (2026-11-13 WB04-E355)
    clearTarget(fields) {
        if (!this.#globals.boss()) need(this.#hooks.assertion)(0x2ac);
        const target = targetOf(fields);
        if (!target) return;
        need(this.#hooks.removeDepend)(fields.form, target);
        fields.bytes[DIRTY] |= 4;
        put(fields.bytes, TARGET, 0, 2);
        this.notify(fields, 1, 0, 0);
        need(this.#hooks.refreshForm)(fields);
    }

    // 실제 메시지 1 등은 빈 몸체이다. 사건 2/server 일 때만 표적을 해제한다. 전용 훨리기그 표는 먼저 sunNotify 로 분배한다. (2026-11-13 WB04-E355)
    notify(fields, message, info1, info2) {
        if (read(fields.bytes, 0) === SUN_TABLE) {
            need(this.#hooks.sunNotify)(fields, message, info1, info2);
            return;
        }
        this.baseNotify(fields, message, info1, info2);
    }

    // 실제 공통 45b760 은 메시지 1 을 생략하며, 전용 45bd60 은 호출자가 먼저 분배한다. (2026-11-13 WB04-E355)
    baseNotify(fields, message, info1, info2) {
        if (message !== 2 || !this.#globals.server()) return;
        if ((info2 >>> 0) !== targetOf(fields)) need(this.#hooks.assertion)(0x2ba);
        this.clearTarget(fields);
    }

    // 더스트 데빌과 0/ffff 홈은 통지를 생략한다. 현재 홈이 실제 live 일 때만 가상 +34 이다. (2026-11-13 WB04-E355)
    parentDying(fields) {
        const pool = this.#pool;
        if (pool.slot(fields.parent)[10] === this.#globals.windType()) return;
        const home = fields.home;
        if (!home || home === 0xffff || home >= pool.capacity) return;
        if (!(pool.slot(home)[11] & 3)) need(this.#hooks.homeNotice)(home, 0, 0);
    }

    // 원본은 dead 종속을 허용하며 free/void 와 69 보다 큰 타입만 거부한다(004ad730). (2026-11-13 WB04-E355)
    checkForms(parent) {
        return checkRawObjectForms(this.#pool, parent);
    }

    // 착륙 대상 포인터는 이동 분기에서 선택한 홈 snapshot 이며 제거 인수는 현재 target/Form 이다. 계속하면 true, 종료 가상 호출 뒤 전체 Frame 반환이면 false 이다. (2026-11-13 WB04-E355)
    #returnHome(fields, parent, home) {
        const pool = this.#pool;
        const hooks = this.#hooks;
        const globals = this.#globals;
        if (read(pool.slot(home), 12, 2) !== fields.parent) {
            need(hooks.finish)(fields);
            return false;
        }
        const sun = need(hooks.sunProcess)(home, globals.sunAviaryProcess());
        if (!read(fields.bytes, LANDING)) {
            put(fields.bytes, LANDING, 1);
            sun.bytes[DIRTY] |= 4;
            put(sun.bytes, 24, 9);
            need(hooks.refreshSun)(sun);
            need(hooks.unpop)(home, 0);
            put(pool.allocated(home), 36, 27);
            need(hooks.repop)(home, 0);
            return true;
        }
        if (sun.stage === 9) return true;
        need(hooks.removeDepend)(fields.form, targetOf(fields));
        fields.bytes[DIRTY] |= 4;
        put(fields.bytes, TARGET, 0, 2);
        const delay = need(hooks.returnDelay)(fields);
        putTime(fields, RETURN, delay + globals.now());
        need(hooks.refreshForm)(fields);
        need(hooks.command)(parent, 2);
        put(fields.bytes, LANDING, 0);
        return true;
    }

    // 초기화/메모리/표적/이동/프레임 및 서버 후반을 원본 순서대로 실행한다. (2026-11-13 WB04-E355)
    baseFrame(fields) {
        const pool = this.#pool;
        const hooks = this.#hooks;
        const globals = this.#globals;
        if (!read(fields.bytes, INITIALIZED)) {
            if (read(fields.bytes, 0) === SUN_TABLE) need(hooks.sunInitialize)(fields);
            else this.initialize(fields, read(fields.bytes, 0) === RAIN_TABLE);
        }
        if (pool.slot(fields.parent)[11] & 2) return;
        if (pool.slot(fields.parent)[11] & 1) return;
        if ((globals.checking() >>> 0) && !need(hooks.robust)(fields)) need(hooks.assertion)(0x42c);
        if (fields.home && !need(hooks.sidRobust)(fields.home, false)) need(hooks.assertion)(0x431);
        const parent = fields.parent;
        if (!need(hooks.sidRobust)(parent, true)) need(hooks.assertion)(0x434);
        const raw = pool.slot(parent);
        if (raw[40] & 1) return;
        if (raw[40] & 0x20) {
            if (globals.boss()) need(hooks.destroy)(parent, 0x200000);
            return;
        }
        let point = position(raw);
        const elapsed = globals.now() - time(fields, LAST);
        // 원문은 비교 전 float snapshot 도 저장한다.
        const storedElapsed = Math.fround(elapsed);
        if (elapsed > 0) {
            if (!valid(point)) need(hooks.assertion)(0x447);
            if (globals.server() && time(fields, RETURN) !== 0 && time(fields, RETURN) <= globals.now()) {
                if (pool.slot(fields.parent)[10] !== globals.sunType()) need(hooks.assertion)(0x44b);
                if (targetOf(fields)) need(hooks.removeDepend)(fields.form, targetOf(fields));
                const home = fields.home;
                if (!home || home >= pool.capacity || (pool.slot(home)[11] & 1) || read(pool.slot(home), 12, 2) !== read(fields.bytes, PARENT_DWORD)) {
                    need(hooks.finish)(fields);
                    return;
                }
                fields.bytes[DIRTY] |= 4;
                put(fields.bytes, TARGET, home, 2);
                need(hooks.addDepend)(targetOf(fields), fields.form);
                putTime(fields, RETURN, 0);
                need(hooks.refreshForm)(fields);
                need(hooks.command)(parent, 3);
            }
            const target = targetOf(fields);
            if (!target || (pool.slot(target)[11] & 1)) point = position(pool.slot(parent));
            else {
                const targetPoint = this.#center(target);
                // 홈 여부는 offset 호출 전 snapshot 이다.
                const different = target !== fields.home;
                if (!different && pool.slot(fields.parent)[10] !== globals.sunType()) need(hooks.assertion)(0x46f);
                const offset = need(hooks.offset)(parent, !different);
                // 실제 저장된 float 벡터 덧셈이다.
                for (let axis = 0; axis < 2; axis += 1) targetPoint[axis] = Math.fround(targetPoint[axis] + offset[axis]);
                const orbit = need(hooks.scatter)(targetOf(fields), fields.parent);
                // 두 번째 벡터도 별도 float 저장이다.
                for (let axis = 0; axis < 2; axis += 1) targetPoint[axis] = Math.fround(targetPoint[axis] + orbit[axis]);
                clamp(targetPoint);
                if (!valid(targetPoint)) need(hooks.assertion)(0x487);
                // 거리 계산과 달리 이동 delta 는 float 이다.
                const delta = [Math.fround(targetPoint[0] - point[0]), Math.fround(targetPoint[1] - point[1])];
                // C++ std::max/std::min 의 비교 방향을 그대로 따른다(NaN 은 앞 인자를 유지).
                const measured = distance(targetPoint, point);
                const separation = measured < MIN_DISTANCE ? MIN_DISTANCE : measured;
                const scaled = (float(fields.bytes, SPEED) / separation) * storedElapsed;
                const ratio = scaled < 1 ? scaled : 1;
                // 원문의 두 축을 각각 float 로 기록한다.
                for (let axis = 0; axis < 2; axis += 1) point[axis] = Math.fround(ratio * delta[axis] + point[axis]);
                if (separation >= float(fields.bytes, RADIUS)) {
                    need(hooks.refreshTime)(parent);
                    if (read(fields.bytes, ATTACK)) {
                        this.notify(fields, 1, 0, 0);
                        put(fields.bytes, ATTACK, 0);
                        if (pool.slot(parent)[10] === globals.rainType()) need(hooks.removeSound)(parent, globals.soundProcess());
                    }
                } else if (different) {
                    if (!read(fields.bytes, ATTACK)) {
                        need(hooks.refreshTime)(parent);
                        put(fields.bytes, ATTACK, 1);
                        this.notify(fields, 1, 1, 0);
                    } else if (pool.slot(parent)[10] !== globals.windType()) need(hooks.hit)(parent, targetOf(fields));
                } else if (separation < 1 && globals.server() && !this.#returnHome(fields, parent, target)) return;
            }
        }
        const frame = read(fields.bytes, 0) === RAIN_TABLE ? this.rainFrame(fields) : need(hooks.otherFrame)(fields);
        if (valid(point)) {
            const current = pool.slot(parent);
            if (!samePoint(point, position(current)) || (frame >>> 0) !== read(current, 36)) {
                const old = position(current);
                need(hooks.unpop)(parent, 0);
                put(pool.allocated(parent), 36, frame >>> 0);
                need(hooks.pop)(parent, point[0], point[1], 0x40);
                if (truncate(old[0]) !== truncate(point[0]) || truncate(old[1]) !== truncate(point[1])) need(hooks.movedCell)(parent);
            }
        }
        fields.bytes[DIRTY] |= 0x40;
        putTime(fields, LAST, globals.now());
        if (!globals.server()) return;
        if (targetOf(fields)) {
            const current = fields.parent;
            if (!need(hooks.checkForms)(current)) need(hooks.assertion)(0x4f3);
            const range = this.#detail(pool.slot(current)[10]).range;
            const target = targetOf(fields);
            let keep = need(hooks.mayTarget)(fields.parent, targetOf(fields));
            if (keep) {
                const count = need(hooks.closerCount)(targetOf(fields), fields.parent);
                keep = count < need(hooks.capacity)(targetOf(fields));
            }
            if (keep || targetOf(fields) === fields.home) {
                const center = this.#center(target);
                const source = pool.slot(current);
                const delta = [Math.fround(center[0] - (read(source, 42) | 0)), Math.fround(center[1] - (read(source, 46) | 0))];
                const reach = Math.sqrt(delta[0] * delta[0] + delta[1] * delta[1]);
                if (!(reach < range)) this.clearTarget(fields);
            } else this.clearTarget(fields);
            if (targetOf(fields)) return;
        }
        if (time(fields, SEARCH) <= globals.now()) {
            need(hooks.findTarget)(fields);
            if (targetOf(fields)) {
                need(hooks.addDepend)(targetOf(fields), fields.form);
                need(hooks.refreshForm)(fields);
                return;
            }
            putTime(fields, SEARCH, globals.now() + 5);
        }
    }

    // 보충 표의 TYPE 번호별 값이다(표 밖은 거부). (2026-11-13 WB04-E355)
    #detail(type) {
        const detail = this.#details[type];
        if (!detail) throw new RangeError('비행 Frame 보충 표 번호가 표 밖입니다');
        return detail;
    }

    // 현재 frame 6/표적/공격 상태가 모두 맞을 때만 무기를 만들고 타입당 같은 tick 의 소리를 억제한다. (2026-11-13 WB04-E355)
    rainFrame(fields) {
        const pool = this.#pool;
        const hooks = this.#hooks;
        const globals = this.#globals;
        if (!(globals.now() >= time(fields, ANIMATION))) return read(pool.slot(fields.parent), 36) | 0;
        putTime(fields, ANIMATION, globals.now() + RAIN_DELAY);
        const parent = fields.parent;
        if (read(pool.slot(parent), 36) === 6 && targetOf(fields) && read(fields.bytes, ATTACK)) {
            need(hooks.launch)(fields.parent, targetOf(fields), 0);
            const type = pool.slot(parent)[10];
            if ((this.#soundFrames[type] >>> 0) !== (globals.frame() >>> 0)) {
                this.#soundFrames[pool.slot(parent)[10]] = globals.frame() >>> 0;
                const point = position(pool.slot(parent));
                need(hooks.sound)(point[0], point[1], this.#detail(pool.slot(parent)[10]).sound);
            }
        }
        const frame = read(pool.slot(fields.parent), 36) | 0;
        const count = need(hooks.runCount)(fields.parent);
        if (!count) throw new RangeError('원본 비행 프레임 signed 나머지0 경로');
        const next = (frame + 1) | 0;
        if (next === -2147483648 && count === -1) throw new RangeError('원본 비행 idiv 오버플로 경로');
        // C++ 정수 나머지는 피제수의 부호를 따르고 0 은 부호가 없다(JS 의 −0 은 0 으로 정규화).
        return (next % count) | 0;
    }
}
