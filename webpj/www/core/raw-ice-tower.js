// 아이스 타워(147)의 파괴(48edf0)와 성장형(150)의 예약(48ead0)·성장 이벤트(48ebe0)·삭제(48eb80)를 원본 순서대로 옮긴 모듈이다(cpppj `o/RawIceTower.cpp/.h`, 10.78 전용).
// 원본은 150 에 공통 Damageable 처리를 추가하지 않으며 삭제 전후의 현재 값을 읽는다. 공간·소유자·생성·삭제·재배치·받침 정리·공통 후처리·프레임 표시 갱신은 훅이다.
//  - postPopGrowingIce(pool, frames, parent, flags, reserve, regular, base): 최초 flags bit0 이고 예약(`reserve`)이 성공하면 현재 TYPE 프레임 수로 `40/프레임 수`(float)초 주기의 이벤트 0x3a6 을 부착(regular)하고, 어느 경우든 마지막에 공통 후처리(base)를 한다.
//    예약이 실패해도 공통 후처리는 막지 않으며 extra/editor 관문을 추가하지 않는다.
//  - RawIceTower.postPop(parent, flags): 위 후처리를 훅으로 연결한다. postDestroy(parent, flags): 완성형(147)이 extra&9 없음·삭제 flags 0x200000 없음·로딩 깊이 0 이면 표면 칸(grid)을 조회해 받침 없는 섬 TYPE 이고 flags 0x8000000 이면
//    받침 정리(dropIsland) 뒤 공통 후처리로 끝낸다. 아니면 battle 일 때 공간 해제(unpop)→성장형(150) 생성→현재 WORD(+12)·zOrder(+35)·소유자(+34) 복사→프레임 0→현재 좌표로 재배치(pop)하고, 마지막에 공통 후처리(damageablePost)이다.
//  - growingPostDestroy(parent, flags): 성장형은 extra&9 없음이고 flags 에 0x8200000 비트가 있으면 현재 좌표의 일반 finder 조회(genusAt, genus 0x1000000)가 있을 때 받침 정리를 하고 마지막에 공통 후처리(basePost)이다.
//  - handle(parent, event, count, payload): 이벤트 0x3a6 만 처리한다. extra bit5 가 없으면 프레임 DWORD+1 을 signed 로 `프레임 수−1` 과 비교해 작으면 프레임 표시 갱신(setFrame)이고, 아니면 boss 일 때 완성형(147)을 생성해 소유자를 넘기고 자신을 삭제한 뒤
//    현재 좌표에 재배치하며 -1(반복 종료)을 반환한다. 그 밖에는 다음 주기 `40/프레임 수`(float)를 반환한다. count 는 쓰지 않는다.
// types = { ice(147), growing(150), noIsland(157) } 의 live 값, battle/boss 는 getter 또는 `{ value }`, loadingDepth 는 getter 또는 `{ value }`, frames[type] 은 { codes } 모양의 188칸 프레임 표이다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E109: 아이스 타워 이식으로 추가)

// 전체 성장 시간(초), 성장 이벤트 번호, 슬롯의 TYPE/상태/WORD/좌표/소유자/zOrder/extra/프레임 위치, 판본의 TYPE 수이다. (2026-11-08 WB04-E109)
const GROWTH_SECONDS = 40;
const GROWTH_EVENT = 0x3a6;
const TYPE_AT = 10;
const WORD_AT = 12;
const X_AT = 14;
const Y_AT = 18;
const OWNER_AT = 34;
const ZORDER_AT = 35;
const EXTRA_AT = 40;
const FRAME_AT = 36;
const TYPE_COUNT = 188;
const fround = Math.fround;

// 비정렬 raw 의 지정 바이트폭을 주변 값과 섞지 않고 읽는다(원본 little endian). (2026-11-08 WB04-E109)
function read(raw, at, width = 4) {
    let value = 0;
    // 지정 폭의 바이트만 낮은 순서부터 조합한다.
    for (let index = 0; index < width; index += 1) value += raw[at + index] * 2 ** (8 * index);
    return value;
}

// 필요한 효과 경계가 없으면 빈 성공으로 대체하지 않는다. (2026-11-08 WB04-E109)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('아이스 타워 실제 하위 몸체 미연결');
    return hook;
}

// 현재 DWORD 프레임만 쓰며 이 대입에서는 표시 갱신을 추가하지 않는다(네 바이트 전체 대입). (2026-11-08 WB04-E109)
function writeFrame(pool, sid, value) {
    const raw = pool.allocated(sid);
    for (let index = 0; index < 4; index += 1) raw[FRAME_AT + index] = (value >>> (index * 8)) & 0xff;
}

// 원본 TYPE+114 의 현재 프레임 수이며 부분 코드표나 다른 TYPE 을 섞지 않는다. (2026-11-08 WB04-E109)
function frameCount(pool, frames, sid) {
    const type = pool.slot(sid)[TYPE_AT];
    if (type >= frames.length) throw new RangeError('아이스 현재 프레임 TYPE 오류');
    return frames[type].codes.length;
}

// 현재 값을 읽는 getter 또는 `{ value }` 소유자의 값이다. (2026-11-08 WB04-E109)
const live = (source) => (typeof source === 'function' ? source() : source.value);

// 성장형 아이스의 최초 예약과 공통 후처리이다. (2026-11-08 WB04-E109)
export function postPopGrowingIce(pool, frames, parent, flags, reserve, regular, base) {
    if (pool?.edition !== 'patch1078' || frames.length !== TYPE_COUNT) throw new Error('성장형 아이스 후처리 판본/프레임 오류');
    if ((flags & 1) && need(reserve)()) {
        // 확보 뒤 현재 TYPE/프레임을 먼저 읽고 그 다음 생성자 경계를 요구한다.
        const period = fround(GROWTH_SECONDS / fround(frameCount(pool, frames, parent)));
        need(regular)(parent, GROWTH_EVENT, period);
    }
    need(base)(parent, flags);
}

// 아이스 타워 완성형/성장형 몸체이다. (2026-11-08 WB04-E109)
export class RawIceTower {
    // 같은 raw 50바이트를 읽고 QA/프레임을 쓰는 풀이다. (2026-11-08 WB04-E109)
    #pool;
    // 현재 TYPE numFrames 의 원본 코드표이다. (2026-11-08 WB04-E109)
    #frames;
    // 효과 뒤에도 읽는 현재 타입 전역이다. (2026-11-08 WB04-E109)
    #types;
    // 재성장 생성의 전투 관문(getter 또는 { value })이다. (2026-11-08 WB04-E109)
    #battle;
    // 최종 타워 생성의 권한 관문(getter 또는 { value })이다. (2026-11-08 WB04-E109)
    #boss;
    // 완성형 파괴의 최초 로딩 관문(getter 또는 { value })이다. (2026-11-08 WB04-E109)
    #loadingDepth;
    // 같은 월드의 실제 하위 몸체이다. (2026-11-08 WB04-E109)
    #hooks;

    // 소유자/공간/프레임과 전투상태를 현재 세션에 연결한다(10.78 의 188 타입 프레임). (2026-11-08 WB04-E109)
    constructor(pool, frames, types, battle, boss, loadingDepth, hooks = {}) {
        if (pool?.edition !== 'patch1078' || frames.length !== TYPE_COUNT) throw new Error('아이스 타워 판본/프레임 오류');
        this.#pool = pool;
        this.#frames = frames;
        this.#types = types;
        this.#battle = battle;
        this.#boss = boss;
        this.#loadingDepth = loadingDepth;
        this.#hooks = hooks;
    }

    // 각 효과 뒤의 현재 x/y 를 원본 float 비트로 읽는다. (2026-11-08 WB04-E109)
    #position(sid) {
        const raw = this.#pool.slot(sid);
        const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
        return [view.getFloat32(X_AT, true), view.getFloat32(Y_AT, true)];
    }

    // 성장형 48ead0 의 실제 예약과 공통 후처리이다. (2026-11-08 WB04-E109)
    postPop(parent, flags) {
        postPopGrowingIce(this.#pool, this.#frames, parent, flags, this.#hooks.reserveRegular, this.#hooks.regular, this.#hooks.basePop);
    }

    // 48edf0 의 완성형 파괴 후 성장형 생성이다. 공간을 먼저 비우고 현재 WORD/zOrder/소유자/좌표 순서로 그대로 전달한다. (2026-11-08 WB04-E109)
    postDestroy(parent, flags) {
        const hooks = this.#hooks;
        const types = this.#types;
        const pool = this.#pool;
        const raw = pool.slot(parent);
        if (raw[TYPE_AT] === types.ice && !(raw[EXTRA_AT] & 9) && !(flags & 0x200000) && !live(this.#loadingDepth)) {
            const point = this.#position(parent);
            const surface = need(hooks.grid)(point[0], point[1]);
            if (surface && pool.slot(surface)[TYPE_AT] === types.noIsland && (flags & 0x8000000)) {
                const at = this.#position(parent);
                need(hooks.dropIsland)(at[0], at[1], flags, pool.slot(parent)[OWNER_AT]);
                need(hooks.damageablePost)(parent, flags);
                return;
            }
            if (live(this.#battle)) {
                need(hooks.unpop)(parent, 0);
                const born = need(hooks.create)(types.growing, 0);
                need(hooks.word)(born, read(pool.slot(parent), WORD_AT, 2));
                pool.allocated(born)[ZORDER_AT] = pool.slot(parent)[ZORDER_AT];
                need(hooks.owner)(born, pool.slot(parent)[OWNER_AT]);
                writeFrame(pool, born, 0);
                const at = this.#position(parent);
                need(hooks.pop)(born, at[0], at[1], 0);
            }
        }
        need(hooks.damageablePost)(parent, flags);
    }

    // 48eb80: 성장형은 extra 9 와 두 삭제 비트만 보고 현재 섬의 존재를 조회한다. (2026-11-08 WB04-E109)
    growingPostDestroy(parent, flags) {
        const hooks = this.#hooks;
        const pool = this.#pool;
        if (!(pool.slot(parent)[EXTRA_AT] & 9) && (flags & 0x8200000)) {
            const point = this.#position(parent);
            if (need(hooks.genusAt)(point[0], point[1], 0x1000000)) {
                const at = this.#position(parent);
                need(hooks.dropIsland)(at[0], at[1], flags, pool.slot(parent)[OWNER_AT]);
            }
        }
        need(hooks.basePost)(parent, flags);
    }

    // 48ebe0: count 는 사용하지 않고 원래 프레임 DWORD 의 감김 증가와 signed 비교만 수행한다. (2026-11-08 WB04-E109)
    handle(parent, event, count, payload) {
        void count;
        const hooks = this.#hooks;
        const pool = this.#pool;
        if (event !== GROWTH_EVENT) return payload;
        if (!(pool.slot(parent)[EXTRA_AT] & 0x20)) {
            const next = (read(pool.slot(parent), FRAME_AT) + 1) >>> 0;
            const last = (frameCount(pool, this.#frames, parent) - 1) >>> 0;
            if ((next | 0) < (last | 0)) {
                need(hooks.setFrame)(parent, next | 0, 0);
            } else if (live(this.#boss)) {
                const born = need(hooks.create)(this.#types.ice, 0);
                need(hooks.owner)(born, pool.slot(parent)[OWNER_AT]);
                const point = this.#position(parent);
                need(hooks.destroy)(parent, 0);
                need(hooks.pop)(born, point[0], point[1], 0);
                return -1;
            }
        }
        return fround(GROWTH_SECONDS / frameCount(pool, this.#frames, parent));
    }
}
