// Carrier 의 위험한 다리 판정(4225b0)·가속 낙하(426ef0)·포획 사제 해방(426c80)·내부 좌표 검사(40e800)를 같은 현재 상태에서 처리하는 모듈이다(cpppj `o/RawCarrierFalling.cpp/.h`, 10.78 전용).
//  - needsEscape(x, y): 좌표에 float32 0.9999 를 더한 `_ftol64` 하위 signed 칸이 0..255 안이고 그 칸의 표면 머리가 있으며 현재 TYPE genus 에 비트 4(다리)가 있을 때 현재 프레임 코드의 flags & 0x20 을 반환한다(아니면 0).
//  - fall(부모, 개수): signed 개수가 100 을 넘으면 권한자가 아닐 때 1, 권한자는 포획 사제(mask 0x20, 현재 사제 종류)를 찾으면 해방한 뒤 삭제하고 -1. 개수가 10 이면 +35 BYTE 를 20 으로 쓰고, 그 밖에는 가속 낙하 y 를 254 로 제한해 Unpop → Pop(flags 0x40) 후 0.05.
//  - releasePriest(부모): 진단 전역이 켜져 있는데 권한자가 아니면 원문 진단(Carrier.cpp 3bf)을 요구하고 권한자가 아니면 끝. 권한자는 포획 내용물을 찾고 현재 사제 종류로 생성해 WORD 를 옮기고 소유자 0 을 지정한 뒤 이전 소유자의 Player 기록 +0xa8/+0xac 복귀 좌표가 내부 좌표가 아니면 진단(3d0)을 요구하고 Pop → 낙하 가상 → 포획 내용물 삭제한다.
//  - validPoint(x, y): 엄격한 내부 좌표 비교(0 < x < 현재 지도 제한 float, 0 < y < 같은 제한)이다.
// 캡처 시점과 현재 가상 표 재조회를 보존하며 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E73: Carrier 낙하 이식으로 추가)

// 슬롯의 TYPE/좌표/프레임/낙하 표식/원래 소유자 위치, 지도 크기, 다리 genus 비트와 프레임 flags 비트, 해방/낙하 상수, Player 기록 폭과 복귀 좌표 위치이다. (2026-11-08 WB04-E73)
const TYPE_OFFSET = 10;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const FRAME_OFFSET = 36;
const FALL_MARK_OFFSET = 35;
const FALL_MARK = 20;
const OWNER_WORD_OFFSET = 12;
const OWNER_MASK = 0x7f;
const WORLD_CELLS = 256;
const BRIDGE_GENUS = 4;
const ESCAPE_FLAG = 0x20;
const FALL_LIMIT = 100;
const FALL_MARK_COUNT = 10;
const FALL_FLOOR = 254.0;
const FALL_DELAY = Math.fround(0.05);
const POP_FLAGS = 0x40;
const CAPTURED_MASK = 0x20;
const PLAYER_STRIDE = 0xb4;
const RETURN_X = 0xa8;
const RETURN_Y = 0xac;
const GROUND_BIAS = Math.fround(0.9999);
const TYPE_COUNT = 188;
const SURFACE_COUNT = 65536;

// 비정렬 원본 DWORD/WORD 를 리틀엔디언으로 읽으며 물리 범위를 확인한다. (2026-11-08 WB04-E73)
function read(raw, offset, width = 4) {
    if (offset > raw.length || width > raw.length - offset) throw new RangeError('캐리어 낙하 원본 기록 범위 오류');
    let value = 0;
    // 필요한 하위 바이트만 결합한다.
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value;
}

// 비트를 단정도 float 로 해석한다. (2026-11-08 WB04-E73)
function floatOf(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits, true);
    return view.getFloat32(0, true);
}

// _ftol64 의 무효 정수도 실제 사용한 하위 DWORD(signed)로 관찰한다. (2026-11-08 WB04-E73)
function cell(value) {
    const adjusted = value + GROUND_BIAS;
    if (!Number.isFinite(adjusted) || adjusted < -9223372036854775808 || adjusted >= 9223372036854775808) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(adjusted))));
}

// 선택된 하위만 요구한다. 권한 없는 조기 반환에는 효과가 필요 없다. (2026-11-08 WB04-E73)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('캐리어 낙하 실제 하위 미연결');
    return hook;
}

// pool 은 10.78 SidPool, types 는 188칸 TYPE 표(flags2), frames 는 188칸 FrameCodeTable 표, surface 는 65536칸 WORD 표면 머리, players 는 Player 기록(폭 0xb4 의 배수), boss/checking/priestType/mapLimit 는 현재 전역을 읽는 getter,
//   hooks = { find(부모, mask, 종류, 순번)→SID, findKind(부모, 종류)→SID, create(타입, flags)→SID, setWord(sid, word), setOwner(sid, 소유자), destroy(sid, flags), unpop(sid, flags), pop(sid, x, y, flags), fallWalker(sid)→bool, assertion(줄) } 이다. (2026-11-08 WB04-E73)
export class RawCarrierFalling {
    // 캐리어/내용물/새 사제의 동일 물리 풀, 현재 TYPE 의 genus, 현재 다리 프레임 코드, 실제 hash0 표면 머리, Player 기록, 현재 전역 getter, 큰 하위 효과이다.
    #pool;
    #types;
    #frames;
    #surface;
    #players;
    #boss;
    #checking;
    #priestType;
    #mapLimit;
    #hooks;

    // 판본과 자료 크기를 구성 때 확인한다. (2026-11-08 WB04-E73)
    constructor(pool, types, frames, surface, players, boss, checking, priestType, mapLimit, hooks) {
        if (pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length !== TYPE_COUNT || !Array.isArray(frames) || frames.length !== TYPE_COUNT
            || !(surface instanceof Uint16Array) || surface.length !== SURFACE_COUNT || !(players instanceof Uint8Array) || players.length % PLAYER_STRIDE !== 0
            || [boss, checking, priestType, mapLimit].some((getter) => typeof getter !== 'function')) {
            throw new TypeError('캐리어 낙하 PATCH 자료 크기 오류');
        }
        this.#pool = pool;
        this.#types = types;
        this.#frames = frames;
        this.#surface = surface;
        this.#players = players;
        this.#boss = boss;
        this.#checking = checking;
        this.#priestType = priestType;
        this.#mapLimit = mapLimit;
        this.#hooks = hooks ?? {};
    }

    // 다른 월드의 풀 혼합을 검사하는 풀이다. (2026-11-08 WB04-E73)
    get pool() {
        return this.#pool;
    }

    // 4225b0: 거의 올림의 signed 좌표 범위 뒤 현재 다리 프레임 코드의 flags & 0x20 을 반환한다. (2026-11-08 WB04-E73)
    needsEscape(x, y) {
        const ix = cell(x);
        const iy = cell(y);
        if (ix < 0 || iy < 0 || ix >= WORLD_CELLS || iy >= WORLD_CELLS) return 0;
        const sid = this.#surface[iy * WORLD_CELLS + ix];
        if (!sid) return 0;
        const raw = this.#pool.slot(sid);
        const type = raw[TYPE_OFFSET];
        if (type >= this.#types.length) throw new RangeError('캐리어 다리 TYPE 오류');
        if (!(this.#types[type].flags2 & BRIDGE_GENUS)) return 0;
        const frame = read(raw, FRAME_OFFSET);
        const codes = this.#frames[type].codes;
        if (frame >= codes.length) throw new RangeError('캐리어 다리 프레임 코드 범위 오류');
        return codes[frame].flags & ESCAPE_FLAG;
    }

    // 40e800: 엄격한 내부 좌표/단정도 지도 제한 비교이다. (2026-11-08 WB04-E73)
    validPoint(x, y) {
        if (!(x > 0)) return false;
        const limit = Math.fround(this.#mapLimit() | 0);
        return x < limit && y > 0 && y < limit;
    }

    // 426ef0: signed 개수와 권한에 따라 낙하/사제 해방/삭제를 실행한다. 반환은 다음 예약 지연(-1 은 종료)이다. (2026-11-08 WB04-E73)
    fall(parent, count) {
        const hooks = this.#hooks;
        const signedCount = count | 0;
        if (signedCount > FALL_LIMIT) {
            if (!this.#boss()) return 1;
            if (need(hooks.find)(parent, CAPTURED_MASK, this.#priestType() >>> 0, 0)) this.releasePriest(parent);
            need(hooks.destroy)(parent, 0);
            return -1;
        }
        if (signedCount === FALL_MARK_COUNT) this.#pool.allocated(parent)[FALL_MARK_OFFSET] = FALL_MARK;
        const raw = this.#pool.slot(parent);
        const x = floatOf(read(raw, X_OFFSET));
        let y = Math.fround((signedCount * 0.5 + 1.0) * 0.0625 + floatOf(read(raw, Y_OFFSET)));
        // unordered 분기는 NaN 을 보존한다.
        if (FALL_FLOOR < y) y = FALL_FLOOR;
        need(hooks.unpop)(parent, 0);
        need(hooks.pop)(parent, x, y, POP_FLAGS);
        return FALL_DELAY;
    }

    // 426c80: 현재 내용물 WORD 와 이전 소유자의 사제 위치로 실제 사제를 복원한다. (2026-11-08 WB04-E73)
    releasePriest(parent) {
        const hooks = this.#hooks;
        if (this.#checking() && !this.#boss()) need(hooks.assertion)(0x3bf);
        if (!this.#boss()) return;
        const source = need(hooks.findKind)(parent, this.#priestType() >>> 0);
        const priest = need(hooks.create)(this.#priestType() >>> 0, 0);
        need(hooks.setWord)(priest, read(this.#pool.slot(source), OWNER_WORD_OFFSET, 2));
        need(hooks.setOwner)(priest, 0);
        const owner = this.#pool.slot(source)[OWNER_WORD_OFFSET] & OWNER_MASK;
        const at = owner * PLAYER_STRIDE;
        const x = floatOf(read(this.#players, at + RETURN_X));
        const y = floatOf(read(this.#players, at + RETURN_Y));
        if (!this.validPoint(x, y)) need(hooks.assertion)(0x3d0);
        need(hooks.pop)(priest, x, y, 0);
        need(hooks.fallWalker)(priest);
        need(hooks.destroy)(source, 0);
    }
}
