// 경로(Path) 프로세스의 실제 저장 이미지 100080바이트(0x186f0)·생성자·시각 계산(Prepare)·한 꼭짓점씩 진행하는 Step·비행 Init 을 옮긴 모듈이다(cpppj `o/RawPathRuntime.cpp/.h`, 10.78 전용).
// 호스트 포인터와 원본 vtable 주소를 혼동하지 않고, 원본 메모리 이미지의 뜻 있는 필드를 같은 위치/폭으로 읽고 쓴다(미초기화 영역은 생성자의 heapFill 을 그대로 보존).
//  - 이미지 배치(10.78): +0 vtable·+4/+8 Form/부모·+0x10 꼭짓점 개수·+0x14 꼭짓점 좌표 float 쌍(8바이트 간격, 최대 5000)·+0x9c58 도착 시각 double 배열·+0x13898 방향 DWORD 배열·+0x186b8 다음 프레임 예약 시각(QWORD)·
//    +0x186c0 현재 단계·+0x186c4 각도 float·+0x186c8 완료·+0x186cc/+0x186d0 도착 대상 두 개·+0x186d4 helper(+4 가 이동 회피 깊이)·+0x186dc/+0x186e0 패치 전용 목적지 float·+0x186e4/+0x186e8 명령 꼬리·+0x186ec 사망 감시.
//  - new RawPathRuntime({ priest, heapFill, flying, worm, balloon, dude, bulf, walker }): 실제 생성자처럼 vtable(기본 텔레포트 0x50e284, 사제 0x50e458, 파생 종류는 지정 표)과 개수·시각 배열 전체·지정 필드만 0 으로 만들고 꼭짓점/방향 배열은 heapFill 로 남긴다. 파생 종류는 서로 배타적이다.
//  - setPath(points, directions, times): 경로 탐색/수신부가 만든 꼭짓점·방향·시각을 실제 append 와 같은 인덱스로 쓴다(직선 경로를 추측해 만들지 않음). prepare(now, startTime, speed, turningSpeed, firstDirection): 늦은 시작(2초 초과)이면 startTime 을,
//    아니면 now 를 첫 시각으로 쓰고 꼭짓점마다 `회전 역수×회전량 + 거리×속도 역수 + 이전 시각`(역수는 float 저장)으로 누적한다. step(now, parent, observation, stepSound): 한 호출에 최대 한 꼭짓점만 넘으며 짝수 전환에서 발소리를 요청한다.
//    initializeFlight(parent, destination, cruiseHeight, currentHeight): 비행 Init — 생성자의 기존 개수를 초기화하지 않고 이륙/수평/착륙 꼭짓점을 추가한 뒤 현재 단계를 0 으로 만든다.
//  - 필드 접근: count/current/angle/finished·targetA/B·helper·moveAwayDepth·deathWatchToken·nextFrame 등 getter 와 setter 는 원본 저장 폭만 쓰고 옆의 값을 보존한다. 범위 밖 읽기/쓰기는 호스트 메모리 접근 대신 오류로 진단한다.
// 10.72(CD) 분기(배열 600·float 중간 저장 차이·관찰 시각)는 옮기지 않았다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E98: 경로 런타임 이식으로 추가)
import { RawSquidNeighbors } from './raw-squid-neighbors.js';

// 10.78 경로 이미지의 배열 길이와 필드 위치이다. (2026-11-08 WB04-E98)
export const PATH_LAYOUT = Object.freeze({
    capacity: 5000, size: 0x186f0, times: 0x9c58, directions: 0x13898, nextFrame: 0x186b8, current: 0x186c0, angle: 0x186c4, finished: 0x186c8,
    targetA: 0x186cc, targetB: 0x186d0, helper: 0x186d4, deadFlag: 0x186ec, baseVtable: 0x50df64, teleportVtable: 0x50e284, priestVtable: 0x50e458
});
// 꼭짓점 좌표 시작 위치(x/y 가 8바이트 간격)와 개수 위치, 패치 전용 목적지 float 위치, 명령 꼬리 위치이다. (2026-11-08 WB04-E98)
const COUNT_AT = 0x10;
const POINT_X_AT = 0x14;
const POINT_Y_AT = 0x18;
const DESTINATION_X_AT = 0x186dc;
const DESTINATION_Y_AT = 0x186e0;
const TAIL_AT = 0x186e4;
// 파생 종류별 vtable 주소이다(fly39·worm40·balloon42·dude24·bulf41·walker38). (2026-11-08 WB04-E98)
const VTABLE = Object.freeze({ flying: 0x50e2ec, worm: 0x50e4c4, balloon: 0x50e2b8, dude: 0x50e3d8, bulf: 0x50e354, walker: 0x50e320 });
// 방향 한 칸의 각도, 한 바퀴, 늦은 시작 허용 간격(초), 고도→칸 배율(단정도 ROM 504194)이다. (2026-11-08 WB04-E98)
const DIRECTION_DEGREES = 45;
const FULL_TURN = 360;
const LATE_START = 2;
const HEIGHT_TO_CELL = Math.fround(0.09090909361839294);
const fround = Math.fround;

// 방향 번호를 제한하지 않는 원본 산술이다. 유효 0..7 입력에서는 최단 signed 회전(양의 180도는 4방향 차이에서 선택)이다. (2026-11-08 WB04-E98)
function directionDelta(previous, next) {
    let forward = next - previous;
    let backward = previous - next;
    if (next < previous) forward += 8;
    if (previous < next) backward += 8;
    return (forward <= backward ? forward : -backward) * DIRECTION_DEGREES;
}

// 경로 프로세스의 원시 이미지이다. (2026-11-08 WB04-E98)
export class RawPathRuntime {
    // 100080바이트 원시 이미지이다. (2026-11-08 WB04-E98)
    #bytes;
    // 리틀 엔디언 필드 접근용 뷰이다. (2026-11-08 WB04-E98)
    #view;

    // 실제 생성자처럼 시각 배열과 지정 필드만 초기화한다. heapFill 은 독립 생성자 대조의 미초기화 입력이다. (2026-11-08 WB04-E98)
    constructor({ priest = false, heapFill = 0, flying = false, worm = false, balloon = false, dude = false, bulf = false, walker = false } = {}) {
        this.#bytes = new Uint8Array(PATH_LAYOUT.size).fill(heapFill & 0xff);
        this.#view = new DataView(this.#bytes.buffer);
        this.#write(0, priest ? PATH_LAYOUT.priestVtable : PATH_LAYOUT.teleportVtable);
        this.#write(4, 0);
        this.#write(8, 0);
        this.#write(COUNT_AT, 0);
        const kinds = [['flying', flying], ['worm', worm], ['balloon', balloon], ['dude', dude], ['bulf', bulf], ['walker', walker]];
        let chosen = false;
        // 파생 종류는 사제와 서로 다른 종류 모두에 배타적이다.
        for (const [name, enabled] of kinds) {
            if (!enabled) continue;
            if (priest || chosen) throw new Error(`실제 ${name} 생성자 종류 오류`);
            this.#write(0, VTABLE[name]);
            chosen = true;
        }
        // 원본 생성자는 시각 배열 전체만 0 으로 만들고 꼭짓점/방향 배열은 보존한다.
        for (let index = 0; index < PATH_LAYOUT.capacity; index += 1) this.#view.setBigUint64(PATH_LAYOUT.times + index * 8, 0n, true);
        this.#view.setBigUint64(PATH_LAYOUT.nextFrame, 0n, true);
        // current/angle/finished/두 대상/helper/helper+4/사망 감시는 모두 0 이다.
        for (const offset of [PATH_LAYOUT.current, PATH_LAYOUT.angle, PATH_LAYOUT.finished, PATH_LAYOUT.targetA, PATH_LAYOUT.targetB, PATH_LAYOUT.helper, PATH_LAYOUT.helper + 4, PATH_LAYOUT.deadFlag]) this.#write(offset, 0);
        this.#write(TAIL_AT, 0);
        this.#write(TAIL_AT + 4, 0);
    }

    // 원시 이미지이다. 호출자는 읽기 전용으로 쓴다(독립 입력 장치만 쓰기에 사용). (2026-11-08 WB04-E98)
    get bytes() { return this.#bytes; }

    // 배열 밖의 메모리를 읽거나 쓰기 전에 진단한다. (2026-11-08 WB04-E98)
    #range(offset, width) {
        if (!Number.isInteger(offset) || offset < 0 || offset + width > this.#bytes.length) throw new RangeError('경로 raw 범위');
    }

    // 원본 DWORD 를 부호 없이 읽는다. (2026-11-08 WB04-E98)
    #read(offset) { this.#range(offset, 4); return this.#view.getUint32(offset, true); }

    // 원본 DWORD 만 쓰며 옆의 값을 보존한다. (2026-11-08 WB04-E98)
    #write(offset, value) { this.#range(offset, 4); this.#view.setUint32(offset, value >>> 0, true); }

    // 꼭짓점 조회의 원본 assert 범위를 호스트 예외로 진단한다. (2026-11-08 WB04-E98)
    #check(index) {
        if (index < 0 || index >= this.count) throw new RangeError('경로 꼭짓점 범위');
    }

    // 파생 종류/사제 여부와 무관한 판본 상수 배열 길이이다. (2026-11-08 WB04-E98)
    static get layout() { return PATH_LAYOUT; }

    // 0041cd60: 4방향 차이에서는 양의 180도를 고른다. (2026-11-08 WB04-E98)
    static directionDelta(previous, next) { return directionDelta(previous, next); }

    // 경로 탐색/네트워크 복원부가 실제 원본 순서로 만든 꼭짓점·방향·시각을 받는다. 직선 경로를 만들어 주지 않는다. (2026-11-08 WB04-E98)
    setPath(points, directions, times = []) {
        if (points.length !== directions.length || points.length >= PATH_LAYOUT.capacity || (times.length !== 0 && times.length !== points.length)) {
            throw new RangeError('경로 입력 크기');
        }
        this.#write(COUNT_AT, points.length);
        // 실제 append 와 같은 꼭짓점/방향/시각 배열의 인덱스를 사용한다.
        for (let index = 0; index < points.length; index += 1) {
            this.#view.setFloat32(POINT_X_AT + index * 8, points[index].x, true);
            this.#view.setFloat32(POINT_Y_AT + index * 8, points[index].y, true);
            this.#view.setInt32(PATH_LAYOUT.directions + index * 4, directions[index], true);
            this.#view.setFloat64(PATH_LAYOUT.times + index * 8, times.length === 0 ? 0 : times[index], true);
        }
    }

    // 실제 비행 Init: 생성자의 기존 count 를 초기화하지 않고 꼭짓점을 추가한다(착륙 직전 고도는 단정도 배율로 한 번 저장). (2026-11-08 WB04-E98)
    initializeFlight(parent, destination, cruiseHeight = 0, currentHeight = 0) {
        // 4882e0 의 좌표/방향/시각 쓰기와 마지막 count 증가 순서이다.
        const append = (point, direction) => {
            const index = this.count;
            if (index < 0 || index + 1 >= PATH_LAYOUT.capacity) throw new RangeError('비행 경로 append 범위 오류');
            this.#view.setFloat32(POINT_X_AT + index * 8, point.x, true);
            this.#view.setFloat32(POINT_Y_AT + index * 8, point.y, true);
            this.#view.setBigUint64(PATH_LAYOUT.times + index * 8, 0n, true);
            this.#view.setInt32(PATH_LAYOUT.directions + index * 4, direction, true);
            this.#write(COUNT_AT, index + 1);
        };
        append(parent, 0);
        const rise = fround(cruiseHeight * HEIGHT_TO_CELL);
        if ((currentHeight & 0xff) < ((cruiseHeight - 1) | 0)) append({ x: parent.x, y: fround(parent.y - (rise - (currentHeight & 0xff) * HEIGHT_TO_CELL)) }, 0);
        if (cruiseHeight) append({ x: destination.x, y: fround(destination.y - rise) }, 0);
        append(destination, RawSquidNeighbors.direction(parent.x, parent.y, destination.x, destination.y));
        this.resetCurrent();
    }

    // 48ad60 은 Step 완료 뒤 count 슬롯과 최초 단계의 -1 인접 8바이트도 읽는다. 물리 범위 안의 원문을 보존한다. (2026-11-08 WB04-E98)
    physicalTime(index) {
        if (index < -1 || index >= PATH_LAYOUT.capacity) throw new RangeError('벌룬 고도 시각의 물리 배열 범위');
        return this.#view.getFloat64(PATH_LAYOUT.times + index * 8, true);
    }

    // 현재 예약 시각(fly39 의 186b8)이다. (2026-11-08 WB04-E98)
    get nextFrame() { return this.#view.getFloat64(PATH_LAYOUT.nextFrame, true); }

    // 실제 프레임이 now+0.02 를 저장하는 같은 QWORD 이다. (2026-11-08 WB04-E98)
    setNextFrame(time) { this.#view.setFloat64(PATH_LAYOUT.nextFrame, time, true); }

    // 경로 꼭짓점 개수(signed)이다. (2026-11-08 WB04-E98)
    get count() { return this.#view.getInt32(COUNT_AT, true); }

    // 현재 단계(signed)이다. (2026-11-08 WB04-E98)
    get current() { return this.#view.getInt32(PATH_LAYOUT.current, true); }

    // 보간에서 마지막으로 저장한 각도(float)이다. (2026-11-08 WB04-E98)
    get angle() { return this.#view.getFloat32(PATH_LAYOUT.angle, true); }

    // 도착 가상 호출 직전에 켜는 완료 필드이다. (2026-11-08 WB04-E98)
    get finished() { return this.#read(PATH_LAYOUT.finished) !== 0; }

    // Init 은 현재 단계만 0 으로 만든다(finished/angle 은 초기화하지 않는다). (2026-11-08 WB04-E98)
    resetCurrent() { this.#write(PATH_LAYOUT.current, 0); }

    // 도착 1002 는 finished 를 다시 0 으로 만든다. (2026-11-08 WB04-E98)
    setFinished(value) { this.#write(PATH_LAYOUT.finished, value ? 1 : 0); }

    // 현재 deathWatch DWORD 의 관찰 토큰이다(원문 32비트 지역 주소 자체와 구별). (2026-11-08 WB04-E98)
    get deathWatchToken() { return this.#read(PATH_LAYOUT.deadFlag); }

    // 같은 필드에 비주소 토큰 1/해제 0 을 쓴다. 실제 삭제 통지는 shared watch 가 담당한다. (2026-11-08 WB04-E98)
    setDeathWatchToken(token) { this.#write(PATH_LAYOUT.deadFlag, token); }

    // 원본 raw 상태를 독립 입력/수신 어댑터에서 이관한다. 이동 계산이나 경로 탐색 결과를 새로 만들지 않는다. (2026-11-08 WB04-E98)
    restoreStep(current, angle, finished = false) {
        if (current < 0 || current > this.count || !Number.isFinite(angle)) throw new RangeError('경로 상태 이관');
        this.#write(PATH_LAYOUT.current, current);
        this.#view.setFloat32(PATH_LAYOUT.angle, angle, true);
        this.setFinished(finished);
    }

    // 꼭짓점 방향(signed)이다. (2026-11-08 WB04-E98)
    direction(index) { this.#check(index); return this.#view.getInt32(PATH_LAYOUT.directions + index * 4, true); }

    // 꼭짓점 도착 시각(double)이다. (2026-11-08 WB04-E98)
    time(index) { this.#check(index); return this.#view.getFloat64(PATH_LAYOUT.times + index * 8, true); }

    // 꼭짓점 좌표(float 쌍)이다. (2026-11-08 WB04-E98)
    point(index) {
        this.#check(index);
        return { x: this.#view.getFloat32(POINT_X_AT + index * 8, true), y: this.#view.getFloat32(POINT_Y_AT + index * 8, true) };
    }

    // 부모 가상 도착에 넘기는 두 raw 대상 번호이다. 종류/소유자를 새로 해석하지 않는다. (2026-11-08 WB04-E98)
    get targetA() { return this.#read(PATH_LAYOUT.targetA); }
    get targetB() { return this.#read(PATH_LAYOUT.targetB); }

    // 두 번호를 원본 DWORD 로 저장한다. (2026-11-08 WB04-E98)
    setTargets(first, second) {
        this.#write(PATH_LAYOUT.targetA, first);
        this.#write(PATH_LAYOUT.targetB, second);
    }

    // 시작 상위 몸체의 명령 DWORD 세 개와 패치 전용 목적지 두 float 를 실제 끝 필드에 쓴다(tailByte 인자도 DWORD 전체를 기록). (2026-11-08 WB04-E98)
    setCommandFields(helper, tailWord, tailByte, destination) {
        this.#write(PATH_LAYOUT.helper, helper);
        this.#write(TAIL_AT, tailWord);
        this.#write(TAIL_AT + 4, tailByte);
        this.#range(DESTINATION_X_AT, 8);
        this.#view.setFloat32(DESTINATION_X_AT, destination.x, true);
        this.#view.setFloat32(DESTINATION_Y_AT, destination.y, true);
    }

    // 시작의 연속 꼭짓점 좌표만 덮어쓴다. 시각·방향·개수는 그대로 보존한다. (2026-11-08 WB04-E98)
    replacePoint(index, point) {
        this.#check(index);
        this.#view.setFloat32(POINT_X_AT + index * 8, point.x, true);
        this.#view.setFloat32(POINT_Y_AT + index * 8, point.y, true);
    }

    // cleanup 이 읽는 helper DWORD 이다. 도착 명령 대상 두 개와 혼동하지 않는다. (2026-11-08 WB04-E98)
    get helper() { return this.#read(PATH_LAYOUT.helper); }

    // SharedRegular 예약 시 helper 필드 하나만 0 으로 쓴다. 명령 꼬리·목적지·경로는 보존한다. (2026-11-08 WB04-E98)
    clearHelper() { this.#write(PATH_LAYOUT.helper, 0); }

    // Dais 다음 목표 이동이 읽는 signed DWORD 깊이(helper+4)이다. 명령 꼬리와 다르다. (2026-11-08 WB04-E98)
    get moveAwayDepth() { return this.#view.getInt32(PATH_LAYOUT.helper + 4, true); }

    // 같은 필드 DWORD 만 쓴다. 원본 BridgeCommand 가 기존 깊이+1 을 보관하며 다른 필드는 그대로다. (2026-11-08 WB04-E98)
    setMoveAwayDepth(depth) { this.#view.setInt32(PATH_LAYOUT.helper + 4, depth | 0, true); }

    // ProcessForm 부착 뒤의 실제 번호를 이미지에 기록한다(+4 Form, +8 부모). (2026-11-08 WB04-E98)
    setAttachment(form, parent) {
        this.#write(4, form);
        this.#write(8, parent);
    }

    // 패치 00488e20: 늦은 startTime 선택·float 중간 저장(역수)·회전/거리 시간을 계산해 각 꼭짓점 시각을 double 로 저장한다. (2026-11-08 WB04-E98)
    prepare(now, startTime, speed, turningSpeed, firstDirection) {
        if (this.count < 1 || speed === 0 || turningSpeed === 0) throw new RangeError('경로 시각 입력');
        let time = now - startTime > LATE_START ? startTime : now;
        this.#view.setFloat64(PATH_LAYOUT.times, time, true);
        this.#view.setInt32(PATH_LAYOUT.directions, firstDirection | 0, true);
        const inverseSpeed = fround(1 / speed);
        const inverseTurning = fround(1 / turningSpeed);
        // 원본 현재 시각은 꼭짓점마다 double 에 저장되고 다음 꼭짓점에서 다시 읽힌다.
        for (let index = 1; index < this.count; index += 1) {
            const from = this.point(index - 1);
            const to = this.point(index);
            const dx = to.x - from.x;
            const dy = to.y - from.y;
            const distance = Math.sqrt(dy * dy + dx * dx);
            const delta = Math.abs(directionDelta(this.direction(index - 1), this.direction(index)));
            time = inverseTurning * fround(delta) + distance * inverseSpeed + time;
            this.#view.setFloat64(PATH_LAYOUT.times + index * 8, time, true);
        }
    }

    // 0048a330: 도착 시각보다 빠를 때만 보간하며 원본 currentPathVert 를 한 번씩만 전진한다. 짝수 전환에서 현재 부모 위치의 발소리를 요청한다. (2026-11-08 WB04-E98)
    step(now, parent, observation, stepSound) {
        const result = { position: { x: parent.x, y: parent.y }, angle: this.angle, finished: false };
        const current = this.current;
        if (current >= this.count) {
            if (current < 1 || current !== this.count) throw new RangeError('경로 종료 단계');
            return { position: this.point(current - 1), angle: fround(fround(this.direction(current - 1)) * DIRECTION_DEGREES), finished: true };
        }
        observation.flags = (observation.flags | 1) >>> 0;
        observation.stamp = 0;
        const target = this.point(current);
        const targetTime = this.time(current);
        if (now < targetTime) {
            if (current !== 0) {
                const previousTime = this.time(current - 1);
                const previous = this.point(current - 1);
                if (previousTime <= now) {
                    const scale = fround((now - previousTime) / (targetTime - previousTime));
                    if (!(scale >= 0 && scale <= 1)) throw new RangeError('경로 보간 비율');
                    const base = fround(fround(this.direction(current - 1)) * DIRECTION_DEGREES);
                    const change = fround(directionDelta(this.direction(current - 1), this.direction(current)) * scale);
                    let angle = base + change;
                    if (angle < 0) angle += FULL_TURN;
                    else if (angle >= FULL_TURN) angle -= FULL_TURN;
                    result.angle = fround(angle);
                    this.#view.setFloat32(PATH_LAYOUT.angle, result.angle, true);
                    result.position = {
                        x: fround((target.x - previous.x) * scale + previous.x),
                        y: fround((target.y - previous.y) * scale + previous.y)
                    };
                }
            }
        } else {
            result.position = target;
            result.angle = fround(fround(this.direction(current)) * DIRECTION_DEGREES);
            this.#write(PATH_LAYOUT.current, current + 1);
            result.finished = current + 1 >= this.count;
            if (!result.finished && ((current + 1) & 1) === 0) {
                if (typeof stepSound !== 'function') throw new Error('경로 발소리 효과 누락');
                stepSound();
            }
        }
        return result;
    }
}
