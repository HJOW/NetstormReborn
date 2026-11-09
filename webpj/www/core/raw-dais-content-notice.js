// 다이스에 포획 내용물이 부착될 때 실제 제단 O 프레임/원래 owner WORD 와 cue 를 갱신하는 모듈이다(cpppj `o/RawDaisContentNotice.cpp/.h`, 10.78 전용: Dais 가상 +24 의 내용물 부모 알림).
// ContentFormHost 의 reason 1(부착)/2(제거)를 실제 Dais 가상 표(0x00507950)의 몸체로 전달한다. reason 1 이고 내용물의 kind(+18 WORD)가 현재 사제 kind(005412d0)이면:
//   ① 같은 typed finder(`RawRunePostPop.altarAt`)로 다이스 곁의 제단을 찾고, 있으면 제단 가상 Unpop(flags 0) → 제단 현재 TYPE 의 첫 'O' 프레임을 프레임 DWORD(+36)에 직접 쓰고(setter/표시 효과 없음)
//   → 내용물의 원래 owner WORD(+12, 하위 7비트)를 공통 WORD setter 로 쓰며(현재 다리 TYPE 이면 표면 통지) → 제단 가상 Repop(flags 0) 한다.
//   ② 다이스 owner(+34)가 현재 로컬 owner(00540c70)와 같으면 포획 사제 알림 cue(00542a50)를 중지한다(희생 완료의 voiceCue 와 다른 cue).
// reason 2 제거는 자연 반환이다. 생성자 VT 에 없는 부모 알림을 일반 body 로 대신하지 않는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-10 WB04-E219: 다이스 내용물 알림 이식으로 추가)

import { setRawSquidWord } from './raw-squid-word.js';

// 실제 165 생성자의 최종 가상 표(Dais)와 제단 TYPE 수이다. (2026-11-10 WB04-E219)
export const PATCH_DAIS_VTABLE = 0x00507950;
const TYPE_COUNT = 188;
const TYPE_OFFSET = 10;
const KIND_OFFSET = 18;
const WORD_OFFSET = 12;
const FRAME_OFFSET = 36;
const OWNER_OFFSET = 34;

// 선택된 callable 을 요구하고, 호출 동안 쓸 사본을 돌려준다. (2026-11-10 WB04-E219)
function need(effect) {
    if (typeof effect !== 'function') throw new Error('내용물 부모 알림의 선택된 실제 장치 미연결');
    return effect;
}

// 정렬되지 않은 원본 BYTE/WORD/DWORD 를 지정한 폭만 읽는다. (2026-11-10 WB04-E219)
function read(raw, offset, width) {
    let value = 0;
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value;
}

// 다이스 내용물 알림이다. pool 은 10.78 SidPool, frames 는 타입별 FrameCodeTable 188칸, rune 은 같은 풀의 RawRunePostPop(altarAt), state = { priestType(), localOwner(), cue(), bridgeType() } 현재 전역 getter,
// hooks = { unpop(sid, flags), repop(sid, flags), stopVoice(cue, zero, flags), notifySurface(sid) } 이다. 공유 전역/프레임/typedAltar 는 더 오래 살아야 한다. PATCH 장치는 도달한 위치에서 요구한다.
// 사용: new RawDaisContentNotice(pool, frames, rune, state, hooks).notice(dais, form, 1). (2026-11-10 WB04-E219)
export class RawDaisContentNotice {
    // 단일 현재 raw 저장소, 가상 Unpop 뒤 현재 제단 타입의 O 첫 프레임을 읽는 표, 같은 typedAltar finder, 현재 사제/로컬/cue/다리 타입 전역, 제단 가상 공간·cue·표면 효과이다.
    #pool;
    #frames;
    #rune;
    #state;
    #hooks;

    // 다른 풀/프레임 물리를 먼저 거부하며 PATCH 의 장치는 실제 도달한 위치에서 요구한다. (2026-11-10 WB04-E219)
    constructor(pool, frames, rune, state, hooks = {}) {
        if (pool?.edition !== 'patch1078' || rune?.pool !== pool || !Array.isArray(frames) || frames.length !== TYPE_COUNT) throw new TypeError('다이스 내용물 알림의 동일 풀/필수 효과 누락');
        this.#pool = pool;
        this.#frames = frames;
        this.#rune = rune;
        this.#state = state;
        this.#hooks = hooks ?? {};
    }

    // 실제 다이스 생성자 VT 를 검사하며 원본 body 에 없는 타입 필터를 넣지 않는다. (2026-11-10 WB04-E219)
    handles(dais) { return read(this.#pool.slot(dais), 0, 4) === PATCH_DAIS_VTABLE; }

    // 첫 프레임의 원본 signed DWORD 를 표시 setter 를 호출하지 않고 그대로 쓴다. (2026-11-10 WB04-E219)
    #writeFrame(altar, frame) {
        const raw = this.#pool.physical(altar);
        const value = frame >>> 0;
        for (let index = 0; index < 4; index += 1) raw[FRAME_OFFSET + index] = (value >>> (8 * index)) & 0xff;
    }

    // 004ac220: 공통 WORD 쓰기 뒤 현재 다리 타입만 통지한다. (2026-11-10 WB04-E219)
    #writeWord(altar, word) {
        setRawSquidWord(this.#pool, altar, word, this.#state.bridgeType(), this.#hooks.notifySurface);
    }

    // reason 2(제거)는 자연 반환이며 kind 검사는 현재 전역 사제 타입과 비교한다. reason 1 이면 제단 재등록과 로컬 cue 중지를 수행한다. (2026-11-10 WB04-E219)
    notice(dais, form, reason) {
        if (!this.handles(dais)) throw new TypeError('다이스 내용물 알림의 실제 부모 VT 오류');
        const state = this.#state;
        const hooks = this.#hooks;
        const pool = this.#pool;
        if (!(reason & 1) || read(pool.slot(form), KIND_OFFSET, 2) !== (state.priestType() >>> 0)) return;
        const altar = this.#rune.altarAt(dais);
        if (altar) {
            need(hooks.unpop)(altar, 0);
            const type = pool.slot(altar)[TYPE_OFFSET];
            if (type >= this.#frames.length) throw new RangeError('다이스 알림의 현재 제단 타입 범위');
            this.#writeFrame(altar, this.#frames[type].run('O').first);
            this.#writeWord(altar, read(pool.slot(form), WORD_OFFSET, 2) & 127);
            need(hooks.repop)(altar, 0);
        }
        if (pool.slot(dais)[OWNER_OFFSET] === (state.localOwner() >>> 0)) need(hooks.stopVoice)(state.cue(), 0, 0);
    }

    // 실제 ContentFormHost/가상 공간의 동일 풀을 확인한다. (2026-11-10 WB04-E219)
    get pool() { return this.#pool; }
}
