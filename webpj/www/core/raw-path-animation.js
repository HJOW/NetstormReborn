// 보행 프로세스의 도착 프레임 접두(0048bd90)와 Squid 방향 조회(004ad680/004ad710)를 옮긴 모듈이다(cpppj `o/RawPathAnimation.cpp/.h`, 10.78 전용).
//  - side/direction: 프레임 코드의 side 를 signed char 로 읽는다(direction = side - 'A', 0..7 로 제한하지 않음). restFrame: 현재 글자 구간의 첫 물리 프레임 번호 + 1 이다(코드의 number=1 검색과 구별).
//  - stop: 같은 프레임이어도 Unpop(0) → 프레임 DWORD 쓰기 → Repop(0x40)을 한다. 경로 종료/프로세스 정리는 호출자의 몫이다.
// 프레임 표는 FrameCodeTable(side·variant·flags)이며 글자 구간(run)은 'A'..'P' 만 허용한다. 배열 밖 원본 메모리 읽기는 호스트에서 변경 전 오류로 거부한다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E62: 경로 도착 애니메이션 이식으로 추가)

// 슬롯의 TYPE/프레임 위치, 도착 재등록 flags, 자산 TYPE 시작 번호이다. (2026-11-08 WB04-E62)
const TYPE_OFFSET = 10;
const FRAME_OFFSET = 36;
const ARRIVAL_POP_FLAGS = 0x40;
const FIRST_ASSET_TYPE = 70;

// 프레임 코드의 side 를 signed char 로 읽는다. 배열 밖은 거부한다(004ad680). (2026-11-08 WB04-E62)
export function pathSide(frames, frame) {
    if (frame < 0 || frame >= frames.codes.length) throw new RangeError('보행 프레임 범위 오류');
    return (frames.codes[frame].side << 24) >> 24;
}

// side - 'A' 이다. L~P 와 음수 side 도 원본 signed 계산을 유지한다(004ad710). (2026-11-08 WB04-E62)
export function pathDirection(frames, frame) {
    return pathSide(frames, frame) - 0x41;
}

// 도착 프레임은 현재 글자 구간의 첫 물리 번호 + 1 이다. 글자가 'A'..'P' 밖이면 거부한다. (2026-11-08 WB04-E62)
export function pathRestFrame(frames, frame) {
    const side = pathSide(frames, frame);
    if (side < 0x41 || side > 0x50) throw new RangeError('보행 정지 글자 표 범위 오류');
    return frames.run(side).first + 1;
}

// pool 은 10.78 SidPool, frames 는 188칸 이하 FrameCodeTable 표(복사), hooks = { unpop(sid, flags), repop(sid, flags) } 이며 둘 다 필요하다. (2026-11-08 WB04-E62)
export class RawPathAnimation {
    // 객체 슬롯 풀, 복사한 프레임 표, 가상 Unpop/Repop 효과이다.
    #pool;
    #frames;
    #hooks;

    // 원본 가상 효과 두 개는 생략할 수 없다. 프레임 표의 수명은 내부 복사로 보장한다. (2026-11-08 WB04-E62)
    constructor(pool, frames, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('보행 정지는 10.78 풀만 받습니다.');
        if (typeof hooks?.unpop !== 'function' || typeof hooks.repop !== 'function') throw new TypeError('보행 정지 효과 누락');
        this.#pool = pool;
        this.#frames = [...frames];
        this.#hooks = hooks;
    }

    // 현재 프레임 DWORD 를 부호 있는 번호로 읽고 자산 TYPE/배열 범위를 확인한다. (2026-11-08 WB04-E62)
    #current(sid) {
        const raw = this.#pool.slot(sid);
        if (raw[TYPE_OFFSET] < FIRST_ASSET_TYPE || raw[TYPE_OFFSET] >= this.#frames.length) throw new RangeError('보행 정지 자산 타입 오류');
        return (raw[FRAME_OFFSET] | (raw[FRAME_OFFSET + 1] << 8) | (raw[FRAME_OFFSET + 2] << 16) | (raw[FRAME_OFFSET + 3] << 24)) | 0;
    }

    // 읽기와 글자 표 조회를 마친 뒤부터는 실제 원본 호출/쓰기 순서이다. free/예약 SID 와 타입/현재 프레임 오류는 가상 효과 전에 거부한다. (2026-11-08 WB04-E62)
    stop(sid) {
        this.#pool.allocated(sid);
        const current = this.#current(sid);
        const target = pathRestFrame(this.#frames[this.#pool.slot(sid)[TYPE_OFFSET]], current);
        this.#hooks.unpop(sid, 0);
        const raw = this.#pool.allocated(sid);
        // DWORD 전체를 쓰며 다른 필드와 위치는 보존한다.
        for (let index = 0; index < 4; index += 1) raw[FRAME_OFFSET + index] = (target >>> (8 * index)) & 0xff;
        this.#hooks.repop(sid, ARRIVAL_POP_FLAGS);
    }
}
