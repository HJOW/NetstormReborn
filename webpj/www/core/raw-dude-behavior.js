// Dude(일반 보행 유닛)의 이동 정지 450e60·지면 판정 450e80·추락 450f50·도착 450960·population 삭제 후처리 450920을 옮긴 모듈이다(cpppj `o/RawDudeBehavior.cpp/.h`, 10.78 전용).
// 현재 슬롯(raw 70번 타입·가상 표 0x508cc8)과 256×256 지도를 읽어 부모의 이동 상태를 정하며, 큰 효과(task·이동 삭제·부모 삭제·공통 후처리·진단)는 훅이다.
//  - handles(sid): 타입 70 이고 가상 표가 0x508cc8 인 슬롯만 분배한다(같은 이름의 다른 파생 자산은 포함하지 않는다).
//  - halt(sid): dead 이거나 공간 전환(checkSpace)이면 12, 계속 보행이면 10 이다. 공간 효과가 현재 Path 를 지웠을 때도 원본 12 를 그대로 반환한다.
//  - checkSpace(sid): dead 면 false, 현재 task 가 추락(0x5105)이면 true, 아니면 현재 좌표를 거의 올림(+0.9999 절삭)한 칸의 지도 비트를 읽어 비트 6(표면)이 없으면 fall(task 0x5105 + 이동 삭제) 하고 true,
//    비트 8(건물 점유)이면 task 0x5106 + 이동 삭제 후 true, 아니면 false 이다. 지도 밖은 RangeError.
//  - fall(sid): 현재 상태에 추가 관문 없이 task 0x5105 를 걸고 이동 삭제를 수행한 뒤 항상 true.
//  - arrival(sid): free/dead 면 1001, task 별로 대기(0x5101)는 진단 모드면 진단 0x153, 낙하(0x5105)는 진단 0x157 을 부른 뒤(복귀하면 계속) 부모 삭제하고 1001, 복귀(0x5103)/투하(0x5104)/알림(0x5107)도 삭제 후 1001,
//    보행(0x5102)은 삭제 없이 1000, 알 수 없는 task 는 진단 0x15b 뒤 1000 이다.
//  - postDestroy(sid, flags): 슬롯의 +40 비트 9 가 없을 때 population 에서 같은 항목을 모두 압축 제거하고(남은 항목 순서와 count 뒤 stale 꼬리 보존) 공통 후처리를 부른다.
// state = { checking(진단 모드: 불리언 또는 호출 시점에 읽는 함수), population: SquidPostPopList }, 훅 { task(sid, event, delay), movementType(type)→종류, find(sid, 종류, 0, true), destroy(sid, flags), basePostDestroy(sid, flags), assertion(line) } 는 도달한 지점에서만 요구한다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E122: RawDudeBehavior 이식으로 추가)

// 원본 task WORD 값(대기/보행/복귀/투하/추락/막힘/알림)이다. (2026-11-08 WB04-E122)
const IDLE = 0x5101;
const WALK = 0x5102;
const RETURN = 0x5103;
const DROP = 0x5104;
const FALL = 0x5105;
const BLOCKED = 0x5106;
const NOTICE = 0x5107;
// 이 몸체가 다루는 타입·가상 표, 지도 크기, 거의 올림 편향(실제 500edc 의 float, double 정밀도로 더함)이다. (2026-11-08 WB04-E122)
const DUDE_TYPE = 70;
const DUDE_TABLE = 0x508cc8;
const SPOT_COUNT = 65536;
const CELL_BIAS = Math.fround(0.9999);
const floatView = new DataView(new ArrayBuffer(4));

// 지정 폭의 리틀엔디언 필드만 읽는다. (2026-11-08 WB04-E122)
function read(raw, offset, width = 4) {
    let value = 0;
    // 원본 비정렬 BYTE/WORD/DWORD 를 낮은 바이트부터 합친다.
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value;
}

// 원본 지도 접근의 정상 범위만 허용하며 거의 올림 이전의 float 저장 비트를 사용한다. (2026-11-08 WB04-E122)
function cell(bits) {
    floatView.setUint32(0, bits >>> 0, true);
    const value = floatView.getFloat32(0, true) + CELL_BIAS;
    if (!(value >= 0 && value < 256)) throw new RangeError('Dude 지면 판정 지도 범위 오류');
    return Math.trunc(value);
}

// 실제 선택된 효과가 미연결이면 그 지점에서 중단한다. (2026-11-08 WB04-E122)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('Dude 부모의 실제 하위 미연결');
    return hook;
}

// Dude 부모 행동 논리이다. 사용: new RawDudeBehavior(pool, spots, state, hooks). (2026-11-08 WB04-E122)
export class RawDudeBehavior {
    // 현재 raw 부모와 종속 체인이 공유하는 저장소, 동일 월드의 점유/표면 비트 지도, 진단 모드/population 장기 참조, 실제 도달한 효과 훅이다.
    #pool;
    #spots;
    #state;
    #hooks;

    // 일반 Dude 의 현재 저장소를 공유하고 복원하지 않은 판본은 명시 거부한다. (2026-11-08 WB04-E122)
    constructor(pool, spots, state, hooks = {}) {
        if (pool?.edition !== 'patch1078' || !spots || spots.length !== SPOT_COUNT || !state?.population) throw new Error('Dude 부모 판본/지도 연결 오류');
        this.#pool = pool;
        this.#spots = spots;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 실제 Session/경로 어댑터의 풀 혼용을 검사한다. (2026-11-08 WB04-E122)
    get pool() { return this.#pool; }

    // 현재 생성/최초 Pop/삭제와 같은 30칸 목록을 읽기 전용으로 제공한다. (2026-11-08 WB04-E122)
    get population() { return this.#state.population; }

    // 현재 진단 모드(005e4794)가 비영인지 호출 시점에 읽는다. (2026-11-08 WB04-E122)
    #checking() {
        const checking = this.#state.checking;
        return typeof checking === 'function' ? checking() : checking;
    }

    // 같은 이름의 다른 파생 자산은 포함하지 않는다. (2026-11-08 WB04-E122)
    handles(sid) {
        const raw = this.#pool.slot(sid);
        return raw[10] === DUDE_TYPE && read(raw, 0) === DUDE_TABLE;
    }

    // 원본 도착은 free/dead 에서도 반환하므로 이 getter 에 생존 관문을 추가하지 않는다(타입/가상 표만 확인). (2026-11-08 WB04-E122)
    #raw(sid) {
        if (!this.handles(sid)) throw new Error('Dude 부모 타입/가상 표 오류');
        return this.#pool.slot(sid);
    }

    // 원본은 task 설정 이후의 현재 타입으로 이동 종류를 다시 선택해 같은 조회/삭제를 실행한다. (2026-11-08 WB04-E122)
    #stopMovement(sid) {
        const type = need(this.#hooks.movementType)(this.#raw(sid)[10]);
        need(this.#hooks.find)(sid, type, 0, true);
    }

    // 450f50 의 반환 1 은 실제 이동 객체가 있었는지와 무관하다. 사용: fall(sid) → true. (2026-11-08 WB04-E122)
    fall(sid) {
        this.#raw(sid);
        need(this.#hooks.task)(sid, FALL, 0);
        this.#stopMovement(sid);
        return true;
    }

    // 450e80: bit 6 은 표면, bit 8 은 건물 점유이다. y/x 모두 원본 almostceil 을 거친다. 사용: checkSpace(sid) → 공간 전환 여부. (2026-11-08 WB04-E122)
    checkSpace(sid) {
        const raw = this.#raw(sid);
        if (raw[11] & 2) return false;
        if (read(raw, 12, 2) === FALL) return true;
        const x = cell(read(raw, 14));
        const y = cell(read(raw, 18));
        const spot = this.#spots[y * 256 + x];
        if (!(spot & 6) && this.fall(sid)) return true;
        if (spot & 8) {
            need(this.#hooks.task)(sid, BLOCKED, 0);
            this.#stopMovement(sid);
            return true;
        }
        return false;
    }

    // 450e60: 공간 효과가 현재 Path 를 지웠을 때도 원본 12 를 그대로 반환한다. 사용: halt(sid) → 12 또는 10. (2026-11-08 WB04-E122)
    halt(sid) { return (this.#raw(sid)[11] & 2) || this.checkSpace(sid) ? 12 : 10; }

    // 450960: 원본의 진단 후 fallthrough 와 삭제 뒤 고정 반환을 보존한다. 사용: arrival(sid) → 1000(계속 보행) 또는 1001(삭제됨). (2026-11-08 WB04-E122)
    arrival(sid) {
        const raw = this.#raw(sid);
        if (raw[11] & 3) return 1001;
        switch (read(raw, 12, 2)) {
        case IDLE:
            if (this.#checking()) need(this.#hooks.assertion)(0x153);
            need(this.#hooks.destroy)(sid, 0);
            return 1001;
        case DROP:
        case NOTICE:
            need(this.#hooks.destroy)(sid, 0);
            return 1001;
        case WALK:
            return 1000;
        case FALL:
            if (this.#checking()) need(this.#hooks.assertion)(0x157);
            need(this.#hooks.destroy)(sid, 0);
            return 1001;
        case RETURN:
            need(this.#hooks.destroy)(sid, 0);
            return 1001;
        default:
            need(this.#hooks.assertion)(0x15b);
            return 1000;
        }
    }

    // 450920(40ea00): 원본은 모든 일치 항목을 제거하고 남은 항목 순서와 물리 꼬리 바이트를 보존한다. 사용: postDestroy(sid, flags). (2026-11-08 WB04-E122)
    postDestroy(sid, flags) {
        if (!(this.#raw(sid)[40] & 9)) this.#state.population.removeAll(sid);
        need(this.#hooks.basePostDestroy)(sid, flags);
    }
}
