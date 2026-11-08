// 10.78 사제의 604 의식 애니메이션(00494160 과 실제 도달하는 내부 49417a)을 실제 K..O 코드표와 현재 호스트/프레임에 연결해 옮긴 모듈이다(cpppj `o/RawPriestRitualFrame.cpp/.h`, 10.78 전용).
// 순서: 현재 프레임의 방향(side - 'A')이 K..O(10..14)가 아니면 아무 효과 없이 0 으로 종료 → 그 글자 구간의 첫 번호 first 와 개수 count 를 읽어 다음 프레임 = 현재 + 1(DWORD 감김 보존)을 계산하고
//   `다음 - first` 가 signed 로 count 이상이면 `first + 4` 로 되돌린다 → 다음 프레임이 `first + 14` 이고 현재 605 사건이 없으면 효과 뒤의 현재 좌표로 beamOver03.wav 를 한 번 요청 → 다음 프레임을 지정하고 0.08초를 반환한다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E70: 사제 의식 프레임 이식으로 추가)

import { pathDirection } from './raw-path-animation.js';

// 의식 반복 시작/소리 프레임 오프셋, 소리를 억제하는 현재 사건 번호, 프레임 지연(float32 0.08), 슬롯의 TYPE/프레임/좌표 위치, 의식 방향 범위(K..O)이다. (2026-11-08 WB04-E70)
const RESTART_OFFSET = 4;
const SOUND_OFFSET = 14;
const SUPPRESS_EVENT = 605;
const FRAME_DELAY = Math.fround(0.08);
const TYPE_OFFSET = 10;
const FRAME_OFFSET = 36;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const FIRST_RITUAL_DIRECTION = 10;
const LAST_RITUAL_DIRECTION = 14;
const TYPE_COUNT = 188;
const LETTER_A = 0x41;
const SOUND_FILE = 'beamOver03.wav';

// 정렬되지 않은 DWORD 를 리틀엔디언으로 읽는다. (2026-11-08 WB04-E70)
function dwordAt(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 비트를 단정도 float 로 해석한다. (2026-11-08 WB04-E70)
function floatOf(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits, true);
    return view.getFloat32(0, true);
}

// pool 은 10.78 SidPool, frames 는 188칸 FrameCodeTable 표(참조), hooks = { setFrame(sid, 프레임, flags), eventExists(sid, 사건)→bool, playAt(x, y, 파일, flags) } 이며 모두 필요하다. (2026-11-08 WB04-E70)
export class RawPriestRitualFrame {
    // 효과 뒤 현재 타입/좌표를 다시 읽는 풀, 같은 현재 코드표, 세 필수 하위 효과이다.
    #pool;
    #frames;
    #hooks;

    // 같은 10.78 타입 범위와 필수 효과를 첫 상태 변경 전에 확인한다. (2026-11-08 WB04-E70)
    constructor(pool, frames, hooks) {
        if (pool?.edition !== 'patch1078' || !Array.isArray(frames) || frames.length !== TYPE_COUNT || ['setFrame', 'eventExists', 'playAt'].some((name) => typeof hooks?.[name] !== 'function')) {
            throw new TypeError('사제604 판본/현재표/필수 효과 오류');
        }
        this.#pool = pool;
        this.#frames = frames;
        this.#hooks = hooks;
    }

    // 다음 프레임은 효과 전에 계산하고 605 조회 뒤의 실제 좌표로 소리를 요청한다. 반환은 다음 예약 지연(초; 의식이 아니면 0)이다. (2026-11-08 WB04-E70)
    tick(priest) {
        const raw = this.#pool.slot(priest);
        const type = raw[TYPE_OFFSET];
        if (type >= this.#frames.length) throw new RangeError('사제604 현재 TYPE 범위');
        // 원본 current frame DWORD 이다.
        const current = dwordAt(raw, FRAME_OFFSET);
        const direction = pathDirection(this.#frames[type], current | 0);
        if (direction < FIRST_RITUAL_DIRECTION || direction > LAST_RITUAL_DIRECTION) return 0;
        const run = this.#frames[type].run(LETTER_A + direction);
        // first 가 -1 이어도 원본 DWORD 감김을 그대로 따른다.
        const first = run.first >>> 0;
        let next = (current + 1) >>> 0;
        if (((next - first) | 0) >= run.count) next = (first + RESTART_OFFSET) >>> 0;
        if (next === ((first + SOUND_OFFSET) >>> 0) && !this.#hooks.eventExists(priest, SUPPRESS_EVENT)) {
            const after = this.#pool.slot(priest);
            this.#hooks.playAt(floatOf(dwordAt(after, X_OFFSET)), floatOf(dwordAt(after, Y_OFFSET)), SOUND_FILE, 0);
        }
        this.#hooks.setFrame(priest, next | 0, 0);
        return FRAME_DELAY;
    }
}
