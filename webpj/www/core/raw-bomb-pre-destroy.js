// Point Blast 계열 Bomb 의 가상 전처리(41e960)를 공통 Squid 삭제와 구분해 옮긴 모듈이다(cpppj `o/RawBombPreDestroy.cpp/.h`, 10.78 전용).
//  - handles(sid): 현재 슬롯 첫 DWORD(가상 표)가 실제 502370 인 것만 처리한다(다른 Bomb 파생이나 입력 TYPE 이름으로 표를 넓히지 않는다).
//  - preDestroy(sid, flags): 상태 비트 9(extra) 가 없고 flags 비트 0x100000(폭발 삭제)이 있을 때만 폭발(4605d0) → 효과 뒤 현재 좌표로 explosion.wav 소리(반복 0)를 낸 다음, 항상 비가상 공통 전처리(4b0950)를 부른다.
// 폭발·소리·공통 전처리는 훅 { explosion(sid), sound(x, y), base(sid, flags) } 이며 도달한 지점에서만 요구한다(없으면 TypeError). 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E51: RawBombPreDestroy 이식으로 추가)

// 슬롯 필드 위치(상태 비트·x·y)와 가상 표, 폭발 삭제 플래그, extra 의 살아 있음 검사 마스크이다. (2026-11-08 WB04-E51)
const FLAG_OFFSET = 40;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const BOMB_PREDESTROY_VTABLE = 0x502370;
const EXPLOSION_FLAG = 0x100000;
const POPPED_MASK = 9;

// 정렬되지 않은 DWORD 한 칸과 그 float 해석이다(주변 필드는 읽지 않는다). (2026-11-08 WB04-E51)
const floatView = new DataView(new ArrayBuffer(4));
function read(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}
function floatOf(raw, offset) {
    floatView.setUint32(0, read(raw, offset), true);
    return floatView.getFloat32(0, true);
}

// 선택되지 않은 효과는 요구하지 않으며 빈 성공은 공급하지 않는다. (2026-11-08 WB04-E51)
function need(hook) {
    if (typeof hook !== 'function') throw new TypeError('Bomb 실제 전처리 경계 미연결');
    return hook;
}

// Bomb 가상 전처리 실행기이다. 사용: new RawBombPreDestroy(pool, { explosion, sound, base }). (2026-11-08 WB04-E51)
export class RawBombPreDestroy {
    #pool;
    #hooks;

    // CD 의 다른 전처리와 같은 것으로 추정하지 않도록 PATCH 풀만 받는다. (2026-11-08 WB04-E51)
    constructor(pool, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('Bomb 전처리 PATCH 판본 오류');
        this.#pool = pool;
        this.#hooks = hooks ?? {};
    }

    // 현재 슬롯의 가상 표가 실제 41e960 몸체(502370)인지 확인한다. (2026-11-08 WB04-E51)
    handles(sid) {
        return read(this.#pool.slot(sid), 0) === BOMB_PREDESTROY_VTABLE;
    }

    // 최초 extra/flags 관문은 한 번만 검사하고 폭발 효과 뒤에는 현재 raw 좌표를 다시 읽는다. (2026-11-08 WB04-E51)
    preDestroy(sid, flags) {
        if (!this.handles(sid)) throw new TypeError('Bomb 고유 전처리 VT 불일치');
        const hooks = this.#hooks;
        if (!(this.#pool.slot(sid)[FLAG_OFFSET] & POPPED_MASK) && (flags & EXPLOSION_FLAG)) {
            need(hooks.explosion)(sid);
            const raw = this.#pool.slot(sid);
            need(hooks.sound)(floatOf(raw, X_OFFSET), floatOf(raw, Y_OFFSET));
        }
        need(hooks.base)(sid, flags);
    }
}
