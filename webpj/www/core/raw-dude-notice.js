// 충돌 주변 Dude 의 반응(현재 task/Regular·Burning·가까운 거주 구역 대피)을 같은 raw 월드에서 실행하는 모듈이다(cpppj `o/RawDudeNotice.cpp/.h`, 10.78 전용).
//  - createDudeNoticeState(clock, options): 같은 Host 시계(SquidProcessState, 현재 double 게임 시각)와 원본 분류 전역
//    { burningProcessType 14, pathProcessType 24, bridgeType 82, raisedGenusMask 0x50444200(005424b8) } 이다.
//  - new RawDudeNotice(pool, hash, types, frames, speeds, residences, state, hooks): types 는 188칸 TYPE 레코드(flags2·footX·footY), frames 는 FrameCodeTable 배열,
//    speeds 는 TYPE 별 { speed }(TYPE+e0 의 현재 이동 속도, 경로 요청 동안 2배), residences 는 거주 구역 active 목록(SquidPostPopList)이다. 모두 처리기보다 오래 살아야 한다.
//    · around(x, y)(00451470): (x−1, y−1)을 float 0.9999 로 거의 올림한 칸 기준 ±4 사각형을 실제 RawSquidFinder 로 훑어 flags2&0x400(Dude genus) 후보마다 notice 한다.
//    · notice(sid)(00450db0): dead(상태 비트 2)·작업 0x5107/0x5105 면 반응하지 않는다. 아니면 현재 Regular(마스크 fff0/값 5100)를 찾거나 만들고 0x5107·현재 시각으로 재설정한 뒤
//      task WORD 를 0x5107 로 쓰고, 현재 TYPE 이 Bridge 면 표면 통지, 종속 Burning(14)이 없으면 생성하고, 같은 소유자의 가장 가까운 거주 구역 진입점으로 빠르게 이동(move)한다.
//      대피 표적이 해제/dead 이면 원본 assert(오류)다.
//    · nearestResidence(sid)(0048fd30): 현재 소유자가 같은 거주 구역 중 float 저장 거리(초기 9999)보다 엄격히 가까운 첫 후보(없으면 0)이다.
//    · residencePoint(sid)(004505a0): x − footX×0.5 를 절삭한 뒤 +0.5, y − footY 에 genus 마스크에 따라 −0.5/+1 을 더한 표적 발자국 진입점이다.
//    · move(sid, destination, quickly)(현재 경로 24 의 끝점이 거의 올림 목적지와 같으면 성공. 아니면 속도를 정수로 백업하고 2배로 써서 경로를 요청한 뒤 정수 백업으로 복원하고,
//      성공했고 현재 프레임 코드가 'I' 단계면 현재 TYPE 의 'A' 첫 프레임을 쓴다.)
//    hooks = { taskRegular, addTaskRegular, createBurning, pathEndpoint, startPath, setFrame, notifySurface } 이며 도달한 장치만 요구한다(미연결은 그 위치에서 거부).
//  - makeDudeNoticeHostHooks(pool, host, hooks): 기존 Regular 조회/생성만 같은 Host 에 연결한다(payload 5·event 0x5107·flags 0x50). Burning/Path 생성 경계는 호출자가 공급한다.
// CD 10.72 분기(인라인 notice·BYTE 프레임·소유자 +32)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E207: Dude 충돌 반응 이식으로 추가)
import { RawSquidFinder } from './raw-squid-finder.js';

// 원본 Dude task WORD(반응·낙하)와 가까운 거주 구역 최초 거리, 거의 올림 float 바이어스이다. (2026-11-09 WB04-E207)
const NOTICE_TASK = 0x5107;
const FALL_TASK = 0x5105;
const FAR_DISTANCE = Math.fround(9999);
const SNAP_BIAS = Math.fround(0.9999);

// 슬롯의 next·종속 머리·TYPE·상태·task·좌표·소유자·프레임 위치이다. (2026-11-09 WB04-E207)
const NEXT_AT = 4;
const CHILD_AT = 6;
const TYPE_AT = 10;
const STATE_AT = 11;
const TASK_AT = 12;
const X_AT = 14;
const Y_AT = 18;
const OWNER_AT = 34;
const FRAME_AT = 36;

// 판본 TYPE 수·Dude genus·높이 보정 genus·원본 int32 범위·글자 코드이다. (2026-11-09 WB04-E207)
const TYPE_COUNT = 188;
const DUDE_GENUS = 0x400;
const LOWER_GENUS = 0x10040000;
const INT32_MIN = -0x80000000;
const INT32_MAX = 0x7fffffff;
const LETTER_A = 0x41;
const LETTER_I = 0x49;

// Host Regular 조회 마스크/값과 생성 payload/flags 이다. (2026-11-09 WB04-E207)
const TASK_MASK = 0xfff0;
const TASK_VALUE = 0x5100;
const TASK_PAYLOAD = 5;
const TASK_FLAGS = 0x50;

// float 비트 변환용 공용 보기이다. (2026-11-09 WB04-E207)
const FLOAT_VIEW = new DataView(new ArrayBuffer(4));

// 원본 비정렬 little endian 값을 부호 없이 읽는다. (2026-11-09 WB04-E207)
function read(raw, offset, width = 4) {
    let value = 0;
    // 지정 폭의 낮은 바이트만 조합한다.
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (index * 8);
    return value;
}

// 현재 좌표 float 의 원본 비트를 읽는다. (2026-11-09 WB04-E207)
function coordinate(raw, offset) {
    FLOAT_VIEW.setUint32(0, read(raw, offset), true);
    return FLOAT_VIEW.getFloat32(0, true);
}

// 도달한 장치만 요구한다. (2026-11-09 WB04-E207)
function need(device, message) {
    if (typeof device !== 'function') throw new Error(message);
    return device;
}

// 같은 Host 시계와 원본 프로세스/표면 분류 전역(cpppj DudeNoticeState)이다. (2026-11-09 WB04-E207)
export function createDudeNoticeState(clock, { burningProcessType = 14, pathProcessType = 24, bridgeType = 82, raisedGenusMask = 0x50444200 } = {}) {
    return { clock, burningProcessType, pathProcessType, bridgeType, raisedGenusMask };
}

// 충돌 주변 Dude 반응 처리기이다. 사용: new RawDudeNotice(pool, hash, types, frames, speeds, residences, state, hooks).around(x, y). (2026-11-09 WB04-E207)
export class RawDudeNotice {
    // 현재 객체/종속 Form 이 공유하는 풀·공간 해시·TYPE/프레임 표·속도 저장소·거주 구역 목록·전역·하위 연결이다.
    #pool;
    #hash;
    #types;
    #frames;
    #speeds;
    #residences;
    #state;
    #hooks;

    // 현재 타입/프레임/속도 배열의 판본 크기와 거주 구역 count 를 검사한다. (2026-11-09 WB04-E207)
    constructor(pool, hash, types, frames, speeds, residences, state, hooks = {}) {
        if (pool?.edition !== 'patch1078' || types.length !== TYPE_COUNT || frames.length !== TYPE_COUNT || speeds.length !== TYPE_COUNT) throw new TypeError('Dude notice 타입/판본 배열 오류');
        residences.items();
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#frames = frames;
        this.#speeds = speeds;
        this.#residences = residences;
        this.#state = state;
        this.#hooks = hooks ?? {};
    }

    // 같은 Host/공간 어댑터의 SID 풀 혼용을 막는 데 쓰는 풀이다. (2026-11-09 WB04-E207)
    get pool() {
        return this.#pool;
    }

    // free/contained 는 정상 Notice 의 자산 계약 밖이다(dead 는 반응 없이 반환하려고 조회한다). (2026-11-09 WB04-E207)
    #slot(sid) {
        if (!Number.isInteger(sid) || sid < 5 || sid >= this.#pool.capacity) throw new RangeError('Dude notice SID 오류');
        const raw = this.#pool.slot(sid);
        if ((raw[STATE_AT] & 9) || raw[TYPE_AT] >= this.#types.length) throw new Error('Dude notice 현재 자산 상태/타입 오류');
        return raw;
    }

    // 최신 종속 Form 부터 실제 type/dead 를 검사하며 원본 next 순서로 찾는다(없으면 0). (2026-11-09 WB04-E207)
    #process(sid, type) {
        let current = read(this.#slot(sid), CHILD_AT, 2);
        let visited = 0;
        // 손상된 체인의 무한 반복만 거부하고 정상 원본 순서는 유지한다.
        while (current) {
            visited += 1;
            if (current >= this.#pool.capacity || visited > this.#pool.capacity) throw new Error('Dude notice 종속 체인 오류');
            const raw = this.#pool.slot(current);
            if (raw[TYPE_AT] === type && !(raw[STATE_AT] & 2)) return current;
            current = read(raw, NEXT_AT, 2);
        }
        return 0;
    }

    // 거의 올림: float 입력 + float 0.9999 를 double 로 절삭한 뒤 float 에 저장한다. 원본 CRT 정상 int32 범위만 받는다. (2026-11-09 WB04-E207)
    static #snap(point) {
        // 두 현재 좌표를 같은 규칙으로 절삭한다.
        return point.map((value) => {
            const wide = value + SNAP_BIAS;
            if (!Number.isFinite(wide) || wide < INT32_MIN || wide > INT32_MAX) throw new RangeError('Dude notice 스냅 좌표 오류');
            return Math.fround(Math.trunc(wide) | 0);
        });
    }

    // 실제 영역 탐색기와 타입 genus 를 쓰며 buried/범위 필터를 손으로 대체하지 않는다. 후보 next 는 각 Notice 가 바꾼 raw 이후에도 finder 커서로 전진한다. (2026-11-09 WB04-E207)
    around(x, y) {
        const [px, py] = RawDudeNotice.#snap([Math.fround(x - 1), Math.fround(y - 1)]);
        const cx = px | 0;
        const cy = py | 0;
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
        // 사각형 안 후보를 원본 순서로 방문한다.
        for (let sid = finder.begin({ left: cx - 4, top: cy - 4, right: cx + 4, bottom: cy + 4 }); sid; sid = finder.next()) {
            const raw = this.#pool.slot(sid);
            if (raw[TYPE_AT] >= this.#types.length) throw new RangeError('Dude notice 후보 타입 오류');
            if (this.#types[raw[TYPE_AT]].flags2 & DUDE_GENUS) this.notice(sid);
        }
    }

    // 현재 소유자/좌표를 읽고 후보마다 best float 저장을 유지한다. 같은 거리면 앞선 후보가 남는다. (2026-11-09 WB04-E207)
    nearestResidence(sid) {
        const raw = this.#slot(sid);
        const owner = raw[OWNER_AT];
        const x = coordinate(raw, X_AT);
        const y = coordinate(raw, Y_AT);
        let best = FAR_DISTANCE;
        let result = 0;
        // 실제 거주 구역 active DWORD 목록을 현재 순서대로만 조회한다.
        for (const number of this.#residences.items()) {
            if (!number || number >= this.#pool.capacity) throw new RangeError('Dude notice 거주 구역 SID 오류');
            const candidate = this.#pool.slot(number);
            if (candidate[OWNER_AT] !== owner) continue;
            const dx = x - coordinate(candidate, X_AT);
            const dy = y - coordinate(candidate, Y_AT);
            const distance = Math.sqrt(dx * dx + dy * dy);
            if (distance < best) {
                best = Math.fround(distance);
                result = number;
            }
        }
        return result;
    }

    // 일반 Gun 중심과 달리 x − footX×0.5 절삭 + 0.5 와 타입 높이/현재 genus 로 y 를 보정한 진입점 [x, y](float)이다. (2026-11-09 WB04-E207)
    residencePoint(sid) {
        const raw = this.#slot(sid);
        const type = this.#types[raw[TYPE_AT]];
        const x = coordinate(raw, X_AT);
        if (!Number.isFinite(x) || x < 0 || x > 256) throw new RangeError('Dude notice 거주 구역 좌표 오류');
        let y = Math.fround(coordinate(raw, Y_AT) - Math.fround(type.footY));
        const entranceX = x - type.footX * 0.5;
        if (!(type.flags2 & this.#state.raisedGenusMask)) y = Math.fround(y - 0.5);
        else if (type.flags2 & LOWER_GENUS) y = Math.fround(y + 1);
        return [Math.fround(Math.fround(Math.trunc(entranceX) | 0) + 0.5), y];
    }

    // 기존 목적지를 비교한 뒤 속도 정수 백업을 만들고 새 경로 요청 앞에서만 2배로 쓴다. 반환은 경로 성공 여부다. (2026-11-09 WB04-E207)
    move(sid, destination, quickly = true) {
        this.#slot(sid);
        const hooks = this.#hooks;
        if (this.#process(sid, this.#state.pathProcessType)) {
            const endpoint = need(hooks.pathEndpoint, 'Dude notice 실제 기존 경로 endpoint 미연결')(sid);
            if (!endpoint) throw new Error('Dude notice 현재 경로 객체 조회 오류');
            const snapped = RawDudeNotice.#snap([Math.fround(destination[0]), Math.fround(destination[1])]);
            if (endpoint[0] === snapped[0] && endpoint[1] === snapped[1]) return true;
        }
        const startPath = need(hooks.startPath, 'Dude notice 실제 DudePath24 생성 미복원');
        const type = this.#slot(sid)[TYPE_AT];
        const speed = this.#speeds[type].speed;
        if (!Number.isFinite(speed) || speed < INT32_MIN || speed > INT32_MAX) throw new RangeError('Dude notice 속도 CRT 정상 범위 오류');
        const saved = Math.trunc(speed) | 0;
        if (quickly) this.#speeds[type].speed = Math.fround(speed + speed);
        const made = Boolean(startPath(sid, destination[0], destination[1]));
        this.#speeds[type].speed = Math.fround(saved);
        if (made) {
            const raw = this.#slot(sid);
            const frame = read(raw, FRAME_AT);
            const codes = this.#frames[raw[TYPE_AT]].codes;
            if (frame >= codes.length) throw new RangeError('Dude notice 현재 단계 frame 오류');
            if (codes[frame].side === LETTER_I) {
                const setFrame = need(hooks.setFrame, 'Dude notice 실제 A 프레임 쓰기 미연결');
                setFrame(sid, this.#frames[this.#slot(sid)[TYPE_AT]].run(LETTER_A).first, 0);
            }
        }
        return made;
    }

    // 상태/Regular → Burning → 같은 소유자의 가장 가까운 거주 구역 → 원본 대피 요청 순서를 유지한다. (2026-11-09 WB04-E207)
    notice(sid) {
        const raw = this.#slot(sid);
        const task = read(raw, TASK_AT, 2);
        if ((raw[STATE_AT] & 2) || task === NOTICE_TASK || task === FALL_TASK) return;
        const hooks = this.#hooks;
        let regular = need(hooks.taskRegular, 'Dude notice 현재 Regular 조회 미연결')(sid);
        if (!regular) {
            regular = need(hooks.addTaskRegular, 'Dude notice 실제 Regular 생성 미연결')(sid);
            if (!regular) throw new Error('Dude notice 실제 Regular 생성 실패');
        }
        regular.resetTask(NOTICE_TASK, this.#state.clock.now);
        const bytes = this.#pool.allocated(sid);
        bytes[TASK_AT] = NOTICE_TASK & 0xff;
        bytes[TASK_AT + 1] = NOTICE_TASK >>> 8;
        if (this.#pool.slot(sid)[TYPE_AT] === this.#state.bridgeType) need(hooks.notifySurface, 'Dude task 실제 표면 통지 미연결')(sid);
        if (!this.#process(sid, this.#state.burningProcessType)) need(hooks.createBurning, 'Dude notice 실제 Burning14 생성 미복원')(sid);
        const destination = this.nearestResidence(sid);
        if (!destination) return;
        if (this.#pool.slot(destination)[STATE_AT] & 3) throw new Error('Dude notice 대피 표적 alive 원본 assert');
        this.move(sid, this.residencePoint(destination), true);
    }
}

// 현재 Regular 는 기존 Host 객체 그대로이며 ResetTask 는 공유 좁은 API 를 쓴다. Burning/Path 생성 경계는 그대로 둔다. (2026-11-09 WB04-E207)
export function makeDudeNoticeHostHooks(pool, host, hooks = {}) {
    if (pool !== host?.pool) throw new TypeError('Dude notice/Host SID 풀 불일치');
    return {
        ...hooks,
        taskRegular: (sid) => host.findEventMasked(sid, TASK_MASK, TASK_VALUE),
        addTaskRegular: (sid) => host.addRegular(sid, NOTICE_TASK, TASK_PAYLOAD, TASK_FLAGS),
    };
}
