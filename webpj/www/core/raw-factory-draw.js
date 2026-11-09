// 원본 Factory 의 기본 프레임 가상 +78 과 추가 그림 가상 +a4 를 옮긴 모듈이다(cpppj `o/RawFactoryDraw.cpp/.h`, 10.78 전용).
//  - new RawFactoryDraw(pool, frames): pool 은 10.78 SidPool(`core/sid-pool.js`), frames 는 188칸 FrameCodeTable 표(`assets/terrain-tiles.js`, run(글자) → { first, count })이다.
//    현재 슬롯/TYPE 프레임 표를 매번 다시 읽으며, 두 객체는 이 처리기보다 오래 살아야 한다. 판본/표 크기가 다르면 생성 때 거부한다.
//  - handles(sid): 실제 +78/+a4/+80 이 같은 두 원본 가상 표(Factory 0x509148·아웃포스트 0x509210)인지 본다. 이름/TYPE 번호가 같아도 미복원 파생 표는 거부한다.
//  - mainFrame(sid): 00454cd0. 현재 상태 BYTE(+11) 상위 3비트 Level 의 TYPE 'A'..'H' 구간 첫 번호다. 없는 글자는 -1 이다.
//  - additional(sid, x, y, draw): 00454cf0. extra(+40)&9 또는 dead(+11 비트 2)이면 생략하고, 현재 프레임(signed)이 기본 프레임과 다를 때만 draw(x, y, type, frame)(0044c180 소비자)에 한 번 낸다.
//  - ShapeToBuffer(`render/raw-shape-to-buffer.js`)의 state.mainFrame 에는 mainFrame 을, state.additional 에는 additional 위에서 DrawShape 하는 함수를 연결한다.
// 해제 여부는 새로 거부하지 않는다(원본 추가 그림은 자신의 dead 조건을 따로 본다). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E202: 그림·캡처 묶음의 Factory 그림 이식으로 추가)

// 10.78 Factory 와 아웃포스트 wrapper 가상 표이다(두 표의 +80 은 모두 004ad0f0). (2026-11-09 WB04-E202)
const FACTORY_VTABLE = 0x509148;
const OUTPOST_VTABLE = 0x509210;

// 슬롯의 TYPE·상태·현재 프레임·extra 위치와 생략 비트, 판본 TYPE 수이다. (2026-11-09 WB04-E202)
const TYPE_AT = 10;
const STATE_AT = 11;
const FRAME_AT = 36;
const EXTRA_AT = 40;
const ABSTRACT_BURIED = 9;
const DEAD_BIT = 2;
const TYPE_COUNT = 188;

// 비정렬 원본 DWORD 를 낮은 바이트부터 부호 없이 읽는다. (2026-11-09 WB04-E202)
function readDword(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 현재 슬롯/TYPE 프레임 표를 읽어 Factory 기본·추가 그림을 정하는 처리기이다. 사용: const draw = new RawFactoryDraw(pool, frames); draw.mainFrame(sid). (2026-11-09 WB04-E202)
export class RawFactoryDraw {
    // 같은 실제 50바이트 슬롯 풀과 현재 TYPE 의 A..P 첫 프레임 표이다.
    #pool;
    #frames;

    // TYPE 표와 판본이 다른 저장소는 실제 슬롯을 읽기 전에 거부한다. (2026-11-09 WB04-E202)
    constructor(pool, frames) {
        if (pool?.edition !== 'patch1078' || !Array.isArray(frames) || frames.length !== TYPE_COUNT) throw new TypeError('Factory 그림의 10.78 실제 풀/프레임 표 필요');
        this.#pool = pool;
        this.#frames = frames;
    }

    // 분배기의 동일 풀 계약에 쓰는 풀이다. (2026-11-09 WB04-E202)
    get pool() {
        return this.#pool;
    }

    // 두 원본 가상 표만 받는다. (2026-11-09 WB04-E202)
    handles(sid) {
        const table = readDword(this.#pool.slot(sid), 0);
        return table === FACTORY_VTABLE || table === OUTPOST_VTABLE;
    }

    // 실제 가상 표/프레임 표 범위를 사용 시마다 새 슬롯 사본으로 검사한다. 실패 시 실제 대상(SID·TYPE·VT)을 남긴다. (2026-11-09 WB04-E202)
    #raw(sid) {
        const raw = this.#pool.slot(sid);
        if (!this.handles(sid) || raw[TYPE_AT] >= this.#frames.length) {
            throw new Error(`Factory 그림의 실제 가상 표/TYPE 미복원 SID=${sid} TYPE=${raw[TYPE_AT]} VT=${readDword(raw, 0)}`);
        }
        return raw;
    }

    // 기존 hasLevel 의 OR 조건 때문에 정상 TYPE 에서는 항상 상태 BYTE 상위 3비트를 읽는다. (2026-11-09 WB04-E202)
    mainFrame(sid) {
        const raw = this.#raw(sid);
        return this.#frames[raw[TYPE_AT]].run(0x41 + (raw[STATE_AT] >> 5)).first;
    }

    // getter 뒤에도 현재 슬롯의 실제 프레임과 TYPE 을 다시 읽는다. abstract/dead 의 추가 그림 생략은 원본 몸체의 분기다. (2026-11-09 WB04-E202)
    additional(sid, x, y, draw) {
        const raw = this.#raw(sid);
        if ((raw[EXTRA_AT] & ABSTRACT_BURIED) || (raw[STATE_AT] & DEAD_BIT)) return;
        const base = this.mainFrame(sid);
        const current = readDword(this.#raw(sid), FRAME_AT) | 0;
        if (current === base) return;
        if (typeof draw !== 'function') throw new Error('Factory 추가 그림 0044c180 실제 소비자 미연결');
        draw(x, y, this.#raw(sid)[TYPE_AT], current);
    }
}
