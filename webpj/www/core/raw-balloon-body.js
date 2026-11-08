// 벌룬(42)·에어십·클라우드 플로터의 그림자 후처리(42ac90, 래퍼 42aed0)와 그림자 우선 삭제(42ae80), 클라우드 전용 Regular(42af60), 저장 검사(42ae60)를 같은 월드에 연결한 모듈이다
// (cpppj `o/RawBalloonBody.cpp/.h`, 10.78 전용). 그림자 갱신 중의 현재 부모 재독과 원본 float 저장 지점을 보존한다.
//  - handles(sid): 가상 표 0x5040c0(벌룬/에어십)과 0x504198(클라우드 플로터)만 처리한다. robust(sid): 최초 signed 프레임을 보관한 뒤 현재 TYPE 의 프레임 수와 비교(`frame < frames(TYPE)`)한다.
//  - validate(sid, flags): 상위 ValidateBase 가 공통 장부를 검사하므로 현재 원시 부모(할당·extra&9 없음·자산 TYPE)만 검사한다.
//  - postPop(sid, flags): 클라우드 플로터가 최초 flag 이면 먼저 추가 주기 Regular(event 3, 0.1f)를 할당 성공 때만 생성한다. 이어 최초 flag·현재 프레임이 TYPE 프레임 수 미만·TYPE flags1&0x400000 이면 그림자를 만들어(프레임 = 프레임 수 + 부모 프레임,
//    extra|=8, 부모 +8 WORD 에 연결, 부모 좌표로 Pop) 첫 그림자 주소를 보관하고, Unpop 후 `높이(+30)×0.0909…` 로 계산한 좌표(`height×0.5+x+0.1f`, `height+y−0.3f`; 중간 float 저장 없이 fstp 에서만 반올림)에 Pop 한다. Pop 효과 뒤 현재 그림자 WORD 를 다시 읽어
//    extra bit4 를 부모와 맞추고 표시 깊이를 정하며(높이 0 이면 현재 칸의 bit4 에 따라 −20/−21, 그림자 0xec) 최초 flag 가 없고 권한이 없으면 전용 check 를 부른 뒤 Carrier 공통 Damageable 후처리를 한 번 실행한다.
//  - preDestroy(sid, flags): 현재 그림자(+8 WORD)를 먼저 삭제하고 Carrier 기본 전처리를 실행한다. cloudRegular(...): 클라우드 플로터 전용으로 event 3 만 프레임을 진행(advance(sid, 1, 0))하고 payload 를 돌려주며 나머지는 기본 Carrier Regular(base)에 위임한다.
// hooks = { frames(TYPE)→프레임 수, create(TYPE, 2)→SID, pop(sid, x, y, flags), unpop(sid, flags), destroy(sid, flags), check(sid), allocateRegular()→bool, constructRegular(sid, 3, 0.1) }, spots 는 65536칸 지도 비트, boss 는 현재 권한 getter,
// post/pre 는 같은 풀의 RawCarrierPostPop/RawCarrierPreDestroy 이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E94: 벌룬 그림자 몸체 이식으로 추가)

// 원본 504194/5022ec/504190 의 float 상수(double 로 승격되어 쓰임), 가상 표, 슬롯 위치, 그림자 표시 깊이 값이다. (2026-11-08 WB04-E94)
const HEIGHT_TO_CELL = Math.fround(0.09090909361839294);
const SHADOW_X = Math.fround(0.10000000149011612);
const SHADOW_Y = Math.fround(0.30000001192092896);
const BALLOON_TABLE = 0x5040c0;
const CLOUD_TABLE = 0x504198;
const SHADOW_DEPTH = 0xe6;
const PARENT_DEPTH = 0xe2;
const SURFACE_DEPTH = 0xec;
const REGULAR_DELAY = Math.fround(0.1);
const FIRST_ASSET_TYPE = 70;
const SHADOW_FLAG = 0x400000;
const fround = Math.fround;
// 슬롯의 TYPE(+10)·상태(+11)·그림자 WORD(+8)·높이(+30)·깊이(+35)·프레임(+36)·extra(+40)·좌표(+14/+18) 위치이다. (2026-11-08 WB04-E94)
const TYPE = 10;
const STATE = 11;
const SHADOW = 8;
const HEIGHT = 30;
const DEPTH = 35;
const FRAME = 36;
const EXTRA = 40;
const X_AT = 14;
const Y_AT = 18;

// 비정렬 필드를 실제 BYTE 폭으로 조립/기록한다. (2026-11-08 WB04-E94)
function read(raw, offset, width = 4) {
    let value = 0;
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value >>> 0;
}
function put(raw, offset, value, width = 4) {
    for (let index = 0; index < width; index += 1) raw[offset + index] = (value >>> (index * 8)) & 0xff;
}

// 비정렬 float 좌표를 읽는다(NaN payload 보존). (2026-11-08 WB04-E94)
const view = new DataView(new ArrayBuffer(4));
function coordinate(raw, offset) {
    for (let index = 0; index < 4; index += 1) view.setUint8(index, raw[offset + index]);
    return view.getFloat32(0, true);
}

export class RawBalloonBody {
    // 부모와 그림자의 단일 원시 저장소, 최초 TYPE 주소를 효과 뒤에도 보존하는 TYPE 표, 고도 0 의 깊이를 정하는 현재 칸 비트, 마지막 check 와 Carrier 전처리가 읽는 현재 권한, 같은 공통 깊이를 끝내는 후처리/전처리, 큰 가상 월드 경계이다.
    #pool;
    #types;
    #spots;
    #boss;
    #post;
    #pre;
    #hooks;

    // 같은 PATCH 풀/TYPE/공통 모듈과 물리 지도만 허용한다. (2026-11-08 WB04-E94)
    constructor(pool, types, spots, boss, post, pre, hooks) {
        if (pool?.edition !== 'patch1078' || types.length !== 188 || spots.length !== 65536 || typeof boss !== 'function' || post?.pool !== pool || pre?.pool !== pool) throw new TypeError('벌룬의 실제 원본 월드 연결 오류');
        this.#pool = pool;
        this.#types = types;
        this.#spots = spots;
        this.#boss = boss;
        this.#post = post;
        this.#pre = pre;
        this.#hooks = hooks ?? {};
    }

    // 동일 월드 연결 검사용 풀이다. (2026-11-08 WB04-E94)
    get pool() { return this.#pool; }

    // 가상 표만 확인하며 원본의 두 동일 생성자 타입을 임의로 합성하지 않는다. (2026-11-08 WB04-E94)
    handles(sid) {
        const table = read(this.#pool.slot(sid), 0);
        return table === BALLOON_TABLE || table === CLOUD_TABLE;
    }

    // 현재 VT 가 벌룬 경로일 때만 실행하며 TYPE 조회 전에 frame DWORD 를 보관한다(0042ae60). (2026-11-08 WB04-E94)
    robust(sid) {
        if (!this.handles(sid)) throw new TypeError('벌룬 Robust의 다른 가상 표');
        const frame = read(this.#pool.slot(sid), FRAME) | 0;
        const kind = this.#pool.slot(sid)[TYPE];
        if (kind >= this.#types.length || typeof this.#hooks.frames !== 'function') throw new Error('벌룬 Robust의 현재 TYPE 프레임 조회 누락');
        return frame < this.#hooks.frames(kind);
    }

    // 공간 변경 전 현재 부모만 검사하며 상위 공통 ValidateBase 와 효과를 중복 실행하지 않는다. (2026-11-08 WB04-E94)
    validate(sid, flags) {
        void flags;
        const raw = this.#pool.allocated(sid);
        if (!this.handles(sid) || (raw[STATE] & 9) || raw[TYPE] < FIRST_ASSET_TYPE || raw[TYPE] >= this.#types.length) throw new Error('벌룬 후처리의 현재 원시 부모 오류');
    }

    // 그림자의 동일 TYPE 내 두 번째 그림 묶음을 원본 signed DWORD 감김으로 계산한다. (2026-11-08 WB04-E94)
    #shadowFrame(parent, count) { return ((count >>> 0) + read(this.#pool.slot(parent), FRAME)) >>> 0; }

    // 최초 flag 는 그림자 좌표 임시값과 별개다. 디컴파일의 같은 임시변수명으로 합치지 않는다. (2026-11-08 WB04-E94)
    postPop(sid, flags) {
        const pool = this.#pool;
        const hooks = this.#hooks;
        flags >>>= 0;
        // 실제 42aed0 은 추가 주기를 먼저 만든 뒤 현재 부모로 42ac90 을 호출한다.
        if (read(pool.slot(sid), 0) === CLOUD_TABLE && (flags & 1)) {
            if (typeof hooks.allocateRegular !== 'function') throw new Error('클라우드 플로터 Regular 실제 할당 미연결');
            if (hooks.allocateRegular()) {
                if (typeof hooks.constructRegular !== 'function') throw new Error('클라우드 플로터 Regular 실제 생성 미연결');
                hooks.constructRegular(sid, 3, REGULAR_DELAY);
            }
        }
        const kind = pool.slot(sid)[TYPE];
        if (kind >= this.#types.length || typeof hooks.frames !== 'function') throw new Error('벌룬의 현재 TYPE 프레임 미연결');
        // 최초 TYPE 주소와 별도 보존한 최초 flag 이다.
        const type = this.#types[kind];
        const first = (flags & 1) !== 0;
        if (first && (read(pool.slot(sid), FRAME) | 0) < hooks.frames(kind) && (type.flags1 & SHADOW_FLAG)) {
            const shadow = hooks.create(pool.slot(sid)[TYPE], 2);
            put(pool.allocated(shadow), FRAME, this.#shadowFrame(sid, hooks.frames(kind)));
            pool.allocated(shadow)[EXTRA] |= 8;
            put(pool.allocated(sid), SHADOW, shadow, 2);
            hooks.pop(shadow, coordinate(pool.slot(sid), X_AT), coordinate(pool.slot(sid), Y_AT), 0);
        }
        // Unpop 중 부모 WORD 가 바뀌어도 이 첫 그림자 주소를 유지한다.
        let shadow = read(pool.slot(sid), SHADOW, 2);
        if (shadow) {
            hooks.unpop(shadow, 0);
            put(pool.allocated(shadow), FRAME, this.#shadowFrame(sid, hooks.frames(kind)));
            const x = coordinate(pool.slot(sid), X_AT);
            const y = coordinate(pool.slot(sid), Y_AT);
            const height = pool.slot(sid)[HEIGHT] * HEIGHT_TO_CELL;
            pool.allocated(shadow)[DEPTH] = SHADOW_DEPTH;
            // 42ad64~42ad9d 는 중간 float 저장 없이 두 좌표의 fstp 에서만 반올림한다.
            hooks.pop(shadow, fround(height * 0.5 + x + SHADOW_X), fround((height + y) - SHADOW_Y), 0);
        }
        // 첫 가상 Pop 효과 뒤 현재 그림자 WORD 를 다시 읽는다.
        shadow = read(pool.slot(sid), SHADOW, 2);
        if (shadow) {
            const raw = pool.allocated(shadow);
            raw[EXTRA] ^= (pool.slot(sid)[EXTRA] ^ raw[EXTRA]) & 0x10;
            raw[DEPTH] = SHADOW_DEPTH;
            pool.allocated(sid)[DEPTH] = PARENT_DEPTH;
            if (!pool.slot(sid)[HEIGHT]) {
                const x = coordinate(pool.slot(sid), X_AT);
                const y = coordinate(pool.slot(sid), Y_AT);
                if (!Number.isFinite(x) || !Number.isFinite(y) || x < 0 || x >= 256 || y < 0 || y >= 256) throw new RangeError('벌룬 표시 깊이의 현재 칸 범위');
                pool.allocated(sid)[DEPTH] = (-20 - ((this.#spots[Math.trunc(y) * 256 + Math.trunc(x)] & 0x10) !== 0 ? 1 : 0)) & 0xff;
                raw[DEPTH] = SURFACE_DEPTH;
            }
        }
        if (!first && !this.#boss()) hooks.check(sid);
        this.#post.damageablePostPop(sid, flags);
    }

    // 원본 42af60 은 count 와 payload 를 변경하지 않고 event 3 의 반환도 입력 payload 이다. (2026-11-08 WB04-E94)
    cloudRegular(sid, event, count, payload, advance, base) {
        if (read(this.#pool.slot(sid), 0) !== CLOUD_TABLE) throw new TypeError('클라우드 플로터 전용 Regular의 다른 가상 표');
        if (event === 3) {
            if (typeof advance !== 'function') throw new Error('클라우드 플로터 실제 프레임 진행 미연결');
            advance(sid, 1, 0);
            return payload;
        }
        if (typeof base !== 'function') throw new Error('클라우드 플로터 기본 Carrier Regular 미연결');
        return base(sid, event, count, payload);
    }

    // 42ae80 은 그림자 삭제 뒤 426890 과 동일한 기본 효과 순서를 갖는다. (2026-11-08 WB04-E94)
    preDestroy(sid, flags) {
        const shadow = read(this.#pool.slot(sid), SHADOW, 2);
        if (shadow) this.#hooks.destroy(shadow, 0);
        this.#pre.preDestroy(sid, flags);
    }
}
