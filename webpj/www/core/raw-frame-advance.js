// 공통 프레임 진행(004afc90)의 한 번 감싸기와 실제 프레임 지정(SquidFrame.set) 호출을 옮긴 모듈이다(cpppj `o/RawFrameAdvance.cpp/.h`, 10.78 전용).
// 현재 프레임 DWORD 의 side 글자 구간(첫 번호 first, 개수 count)에서 `relative = current + delta - first` 를 DWORD 로 계산하고, signed 로 count 이상이면 count 를 빼고 음수이면 count 를 더한다(보정은 한 번뿐, modulo/반복 없음).
//   그 뒤 `first + relative` 를 flags 와 함께 실제 `SquidFrame.set` 에 넘기고 보정 여부를 반환한다. 32비트 감김은 `| 0`/`>>> 0` 으로 보존한다. 구간 밖 메모리 읽기는 호스트에서 변경 전 오류로 진단한다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E65: 프레임 진행 이식으로 추가)

import { pathSide } from './raw-path-animation.js';

// 슬롯의 TYPE/프레임 위치이다. (2026-11-08 WB04-E65)
const TYPE_OFFSET = 10;
const FRAME_OFFSET = 36;

// pool 은 10.78 SidPool, frames 는 타입별 FrameCodeTable 표(참조: 교체는 다음 호출에 반영), setter 는 같은 풀의 SquidFrame 이다. (2026-11-08 WB04-E65)
export class RawFrameAdvance {
    // 객체 슬롯 풀, 현재 프레임 표, 실제 프레임 지정기이다.
    #pool;
    #frames;
    #setter;

    // 불일치한 풀 연결은 실제 프레임/해시를 바꾸기 전에 거부한다. (2026-11-08 WB04-E65)
    constructor(pool, frames, setter) {
        if (pool?.edition !== 'patch1078' || setter?.pool !== pool || !Array.isArray(frames) || frames.length === 0) throw new TypeError('프레임 진행 연결 오류');
        this.#pool = pool;
        this.#frames = frames;
        this.#setter = setter;
    }

    // 낙하 이벤트 어댑터가 같은 풀에 연결됐는지 확인하는 풀이다. (2026-11-08 WB04-E65)
    get pool() {
        return this.#pool;
    }

    // 직접 프레임 지정과 진행이 같은 효과 경로를 쓰는지 확인하는 지정기이다. (2026-11-08 WB04-E65)
    get setter() {
        return this.#setter;
    }

    // signed 증분을 DWORD 연산으로 더해 음수면 길이를 더하고 길이 이상이면 뺀다. 보정은 한 번뿐이다. flags 는 실제 SquidFrame.set 에 전달하고 보정 발생 여부를 반환한다. (2026-11-08 WB04-E65)
    advance(sid, delta, flags = 0) {
        const raw = this.#pool.slot(sid);
        if (raw[TYPE_OFFSET] >= this.#frames.length) throw new RangeError('프레임 진행 타입 오류');
        const current = (raw[FRAME_OFFSET] | (raw[FRAME_OFFSET + 1] << 8) | (raw[FRAME_OFFSET + 2] << 16) | (raw[FRAME_OFFSET + 3] << 24)) | 0;
        const frames = this.#frames[raw[TYPE_OFFSET]];
        const side = pathSide(frames, current);
        const run = frames.run(side);
        const first = run.first >>> 0;
        const count = run.count >>> 0;
        let relative = ((current >>> 0) + (delta >>> 0) - first) >>> 0;
        let wrapped = false;
        if ((relative | 0) >= run.count) {
            relative = (relative - count) >>> 0;
            wrapped = true;
        } else if ((relative | 0) < 0) {
            relative = (relative + count) >>> 0;
            wrapped = true;
        }
        this.#setter.set(sid, (first + relative) | 0, flags);
        return wrapped;
    }
}
