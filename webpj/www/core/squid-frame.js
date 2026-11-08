// Squid 프레임 지정(004acee0)과 해시 단계 저장(004ace40)을 raw SID 풀 위에서 옮긴 모듈이다(cpppj `o/SquidFrame.cpp/.h`, 10.78 전용).
// 표시 갱신·Unpop·Pop 은 가상 함수이므로 호출자가 훅으로 연결하고 이 모듈은 순서·인자만 원본대로 호출한다.
//  - set: 현재 프레임으로 구한 해시 단계를 단계 바이트(+0x21)에 쓰고, 그것이 쓰기 전의 단계 바이트와 같으면 표시 갱신 → 프레임 쓰기 → 표시 갱신(새 프레임이 현재와 같아도 한다),
//    다르면 새 프레임이 현재와 다를 때만 Unpop(flags) → 프레임 쓰기 → 현재 위치에 Pop(flags | 0x50)을 한다. 새 프레임의 범위는 원본도 검사하지 않는다.
//  - storeLevel: island·bridge genus 는 0, 그 밖은 현재 프레임 크기의 큰 쪽이 2 이하면 1, 4 이하면 2, 넘으면 3 을 단계 바이트에 써서 돌려준다.
// 근거: 세 실제 PE 의 제한 x86 기대값(setframe-x86.tsv, 웹은 10.78 `originals` 행). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E59: 객체 프레임 지정 이식으로 추가)

import { SquidHash } from './squid-hash.js';

// 프레임 지정이 받는 flags 이다. 0 은 +0x88, 0x2000 은 +0x8c 가상 표시 갱신을 쓴다. 그 밖의 값은 원본 assert 이다. (2026-11-08 WB04-E59)
export const FRAME_UPDATE_ALTERNATE = 0x2000;

// 해시 단계가 맞지 않아 다시 등록할 때 Pop 에 더하는 flags 이다(원본 `or edi, 0x50`). (2026-11-08 WB04-E59)
export const FRAME_REPOP_FLAGS = 0x50;

// 슬롯의 TYPE 번호·위치 float x/y·프레임(DWORD)·해시 단계 바이트 위치와 섬/다리 genus 비트(flags2 0x2|0x4)이다. (2026-11-08 WB04-E59)
const TYPE_OFFSET = 10;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const FRAME_OFFSET = 0x24;
const LEVEL_OFFSET = 0x21;
const ISLAND_BRIDGE = 0x2 | 0x4;

// 리틀엔디언 DWORD 를 호스트 정렬과 무관하게 읽는다. (2026-11-08 WB04-E59)
function readDword(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 단정도 float 비트를 수로 바꾼다. (2026-11-08 WB04-E59)
function floatOf(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// 객체 프레임 변경을 물리 프레임·통지·갱신 요청에 반영하는 처리기이다. pool 은 SidPool, types 는 { flags2 } 표(생성 때 복사),
//   hooks = { frameSize(sid, frame) → [폭, 높이], update(sid, flags), unpop(sid, flags), pop(sid, x, y, flags) } 이며 모두 연결되어야 한다. (2026-11-08 WB04-E59)
export class SquidFrame {
    // 객체 슬롯의 생성·조회·반납에 쓰는 SID 풀, 복사한 타입 표, 외부 효과 묶음이다.
    #pool;
    #types;
    #hooks;

    // 외부 효과는 빠짐없이 연결되어야 한다. (2026-11-08 WB04-E59)
    constructor(pool, types, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('프레임 지정은 10.78 SidPool 만 받습니다.');
        if (typeof hooks?.frameSize !== 'function' || typeof hooks.update !== 'function' || typeof hooks.unpop !== 'function' || typeof hooks.pop !== 'function') {
            throw new TypeError('프레임 지정 훅이 모두 연결되어야 한다');
        }
        this.#pool = pool;
        this.#types = types.map((record) => ({ ...record }));
        this.#hooks = hooks;
    }

    // 다른 풀의 객체를 프레임 진행기에 섞지 않도록 읽기 전용 풀 참조를 공개한다. (2026-11-08 WB04-E59)
    get pool() {
        return this.#pool;
    }

    // 현재 프레임 필드(+0x24 DWORD, 부호 있는 번호)를 읽는다. (2026-11-08 WB04-E59)
    #frame(sid) {
        return readDword(this.#pool.slot(sid), FRAME_OFFSET) | 0;
    }

    // 004ace40: 객체의 현재 프레임으로 해시 단계를 구해 단계 바이트(+0x21)에 쓰고 돌려준다. island·bridge 는 프레임을 읽지 않는다. (2026-11-08 WB04-E59)
    storeLevel(sid) {
        const number = this.#pool.slot(sid)[TYPE_OFFSET];
        if (number >= this.#types.length) throw new RangeError('프레임 지정 타입 범위 오류');
        const genus = this.#types[number].flags2 >>> 0;
        let level = 0;
        if ((genus & ISLAND_BRIDGE) === 0) {
            const size = this.#hooks.frameSize(sid, this.#frame(sid));
            level = SquidHash.objectLevel(genus, size[0], size[1]);
        }
        this.#pool.allocated(sid)[LEVEL_OFFSET] = level;
        return level;
    }

    // 004acee0: 프레임을 지정한다. flags 는 0 또는 0x2000 만 허용하며 그 밖의 값은 상태를 바꾸기 전에 거부한다. (2026-11-08 WB04-E59)
    set(sid, frame, flags) {
        if (flags !== 0 && flags !== FRAME_UPDATE_ALTERNATE) throw new RangeError('프레임 지정 flags(원본 assert)');
        const newFrame = frame | 0;
        // 단계 바이트는 부호 있는 바이트로 읽는다. 단계 계산이 그 자리를 덮어쓰기 전에 읽어 둔다.
        const stored = (this.#pool.slot(sid)[LEVEL_OFFSET] << 24) >> 24;
        const current = this.#frame(sid);
        const level = this.storeLevel(sid);
        // 패치는 프레임 DWORD 전체를 낮은 바이트부터 쓴다(원본 mov dword).
        const write = () => {
            const raw = this.#pool.allocated(sid);
            for (let index = 0; index < 4; index += 1) raw[FRAME_OFFSET + index] = (newFrame >>> (8 * index)) & 0xff;
        };
        if (level === stored) {
            // 같은 단계: 옛 그림 영역과 새 그림 영역을 한 번씩 갱신한다. 다시 등록하지 않는다.
            this.#hooks.update(sid, flags);
            write();
            this.#hooks.update(sid, flags);
            return;
        }
        // 단계가 맞지 않는 객체: 프레임이 그대로면 단계 바이트만 고친 채 끝난다.
        if (newFrame === current) return;
        this.#hooks.unpop(sid, flags);
        write();
        // +0x4c 의 넘김 함수는 Unpop 뒤의 위치를 읽어 가상 Pop 에 넘긴다.
        const raw = this.#pool.slot(sid);
        this.#hooks.pop(sid, floatOf(readDword(raw, X_OFFSET)), floatOf(readDword(raw, Y_OFFSET)), (flags | FRAME_REPOP_FLAGS) >>> 0);
    }
}
