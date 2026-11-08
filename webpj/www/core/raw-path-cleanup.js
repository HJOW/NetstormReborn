// PathProcess 삭제 통지의 공간 접두 뒤 실제 예약·운반 nugget·신전 의식·건설 helper 정리를 옮긴 모듈이다(cpppj `o/RawPathCleanup.cpp/.h`, 10.78). 부모 공간 접두는 기존 경로 프로세스의 OnFormDestroy 가 수행하고
// 이 객체는 그 다음 몸체(패치 0048aa00 의 공간 접두 이후)만 실행한다.
//  - createPathCleanupState(): { boss(권한), checking(패치 assert 켜짐 DWORD), priestType(158), daisType(165), outpostType(126), nuggetType(154), helperEvent(0x1a2c), sharedNuggetType(실제 005412c0 의 같은 번호를 빌리는 getter 또는 null) }.
//    권한/타입/이벤트는 하위 효과 뒤에도 현재 값을 다시 읽는다. currentNuggetType(state)는 공유 getter 가 있으면 그 값을 쓴다.
//  - new RawPathCleanup(pool, hash, types, state, hooks): hooks = { reserveShared() → bool(원본 new(0x28)의 성공/실패), addShared(sid, event, payload), contained(sid, mask, kind, index), graphFor(sid), ritual(temple, parent, flags),
//    refund(type, owner), destroy(sid, flags) } 일곱 모두 필수이다(조회는 기존 실제 모듈, 의식/삭제/할당은 실행 경계).
//  - afterSpatial(runtime, parent): ① helper 가 현재 유효한 건설 보조 부류(0x404200)이면 부모 좌표(거의올림)±2 사각형에서 찾아 예약(실패해도) 하고 helper 필드를 0 으로 지움(첫 일치에서 끝)
//    ② 10.78: 부모가 죽지 않았으면 귀환 장소(radius 1)와 운반 nugget 이 있고 소유자가 같으며 두 Graph 가 같고 권한이 있으면 nugget 삭제 ③ 부모 타입이 사제이면 거리 2 의 신전을 찾아 의식 요청(abstract 신전 제외)
//    ④ 권한이 있고 남은 helper 가 있으면 정리(이미 죽은 것은 생략, 보조 부류가 아니거나 abstract 가 아니면 checking 일 때 assert 로 거부하고 끝, 아니면 환불 뒤 삭제).
//  - returnPlaceAt(x, y, radius = 1): 좌표±radius 를 절삭한 사각형에서 abstract/죽음을 제외한 귀환 장소(genus 0x200 또는 outpost)의 첫 일치이다.
//    nearbyType(x, y, radius, type): 거의올림 좌표 ± radius 사각형에서 현재 타입의 genus 로 finder 시작 단계를 고른 첫 같은 타입이다.
//  - makePathCleanupHooks(pool, carrier, anchor, host, place, hooks): 기존 contained 조회·Graph·공유 예약·환불을 같은 풀에 연결한다(의식/삭제/예약 성공 여부는 호출자 몫).
// 10.72(CD)의 귀환 nugget 분기는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E159: 경로 정리 이식으로 추가)
import { PATH_LAYOUT } from './raw-path-runtime.js';
import { RawSquidFinder } from './raw-squid-finder.js';

// 원본 helper 부류·귀환 장소 부류·contained mask, 거의올림 bias(float 0.9999899864196777), CRT 불확정 signed64 경계, 슬롯 필드 위치, 타입 표 크기이다. (2026-11-09 WB04-E159)
const HELPER_GENUS = 0x404200;
const RETURN_PLACE_GENUS = 0x200;
const NUGGET_MASK = 0x20;
const FINDER_BIAS = Math.fround(0.9999899864196777);
const FTOL_LIMIT = 9223372036854775808;
const TYPE = 10;
const STATE = 11;
const X = 14;
const Y = 18;
const OWNER = 34;
const EXTRA = 40;
const TYPE_COUNT = 188;
const HOOK_NAMES = ['reserveShared', 'addShared', 'contained', 'graphFor', 'ritual', 'refund', 'destroy'];

// 경로 정리가 읽는 전역을 원본 초기값으로 만든다. (2026-11-09 WB04-E159)
export function createPathCleanupState() {
    return { boss: true, checking: 0, priestType: 158, daisType: 165, outpostType: 126, nuggetType: 154, helperEvent: 0x1a2c, sharedNuggetType: null };
}

// 내용물 검색 위치의 현재 nugget 타입이다. 공유 getter 가 있으면 그 값을 쓴다. (2026-11-09 WB04-E159)
export function currentNuggetType(state) {
    return (typeof state.sharedNuggetType === 'function' ? state.sharedNuggetType() : state.nuggetType) >>> 0;
}

// 원본 CRT 의 signed64 절삭 뒤 하위 DWORD 만 int32 로 돌려준다. 비유한/범위 밖은 불확정 값 0 이다. (2026-11-09 WB04-E159)
function truncate(value) {
    if (!Number.isFinite(value) || value >= FTOL_LIMIT || value < -FTOL_LIMIT) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 원본 x87 덧셈/절삭 뒤 하위 DWORD 만 읽는다. 좁은 좌표 사각형을 임의로 반올림하지 않는다. (2026-11-09 WB04-E159)
function cell(coordinate, biased) {
    return truncate(Math.fround(coordinate) + (biased ? FINDER_BIAS : 0));
}

// 정렬되지 않은 float 좌표의 바이트를 순서대로 조합한다. (2026-11-09 WB04-E159)
function coordinate(raw, offset) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0, true);
    return view.getFloat32(0, true);
}

// 경로 프로세스 삭제 뒤의 정리 계층이다. 풀/hash/타입/상태/연결 대상은 처리기보다 오래 살아야 한다. (2026-11-09 WB04-E159)
export class RawPathCleanup {
    #pool;
    #hash;
    #types;
    #state;
    #hooks;

    // 현재 자료 폭과 필수 실제 하위 경계를 첫 효과 전에 확인한다. (2026-11-09 WB04-E159)
    constructor(pool, hash, types, state, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('경로 정리는 10.78 SidPool 만 받습니다.');
        if (!Array.isArray(types) || types.length !== TYPE_COUNT || !hash || !state || HOOK_NAMES.some((name) => typeof hooks?.[name] !== 'function')) throw new TypeError('Path cleanup 실제 하위 연결 누락');
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#state = state;
        this.#hooks = Object.fromEntries(HOOK_NAMES.map((name) => [name, hooks[name]]));
    }

    // 실제 월드 연결이 같은 풀인지 확인할 수 있도록 원래 풀을 제공한다. (2026-11-09 WB04-E159)
    get pool() { return this.#pool; }

    // 현재 raw 타입으로 현재 타입 표의 flags2 를 조회한다(helper 의 free 상태를 자동으로 걸러내지 않는다). (2026-11-09 WB04-E159)
    #genus(sid) {
        const type = this.#pool.slot(sid)[TYPE];
        if (type >= this.#types.length) throw new RangeError('cleanup 현재 raw 타입 범위');
        return this.#types[type].flags2;
    }

    // 소유자 BYTE(+34)와 abstract/매장 extra BYTE(+40)이다. (2026-11-09 WB04-E159)
    #owner(sid) { return this.#pool.slot(sid)[OWNER]; }
    #extra(sid) { return this.#pool.slot(sid)[EXTRA]; }

    // 004263a0: 좌표±정수 radius 를 절삭한 사각형에서 abstract/죽음을 제외한 genus 0x200 또는 outpost 를 찾는다. 실제 기본 finder 는 0단계를 포함한다. (2026-11-09 WB04-E159)
    returnPlaceAt(x, y, radius = 1) {
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
        const dx = Math.fround(x);
        const dy = Math.fround(y);
        const dr = radius | 0;
        for (let sid = finder.begin({ left: truncate(dx - dr), top: truncate(dy - dr), right: truncate(dx + dr), bottom: truncate(dy + dr) }); sid !== 0; sid = finder.next()) {
            const raw = this.#pool.slot(sid);
            if ((this.#extra(sid) & 1) === 0 && (raw[STATE] & 2) === 0 && ((this.#genus(sid) & RETURN_PLACE_GENUS) !== 0 || raw[TYPE] === this.#state.outpostType)) return sid;
        }
        return 0;
    }

    // 004b21d0: 원본 생성자가 거의올림 좌표를 먼저 만들고 radius 만큼 사각형을 넓힌다. 현재 타입의 genus 로 finder 시작 단계를 고르고 첫 같은 타입을 돌려준다. (2026-11-09 WB04-E159)
    nearbyType(x, y, radius, type) {
        if (!Number.isInteger(type) || type < 0 || type >= this.#types.length) throw new RangeError('cleanup 근처 타입 범위');
        const cx = cell(x, true);
        const cy = cell(y, true);
        const distance = radius | 0;
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
        for (let sid = finder.begin({ left: (cx - distance) | 0, top: (cy - distance) | 0, right: (cx + distance) | 0, bottom: (cy + distance) | 0 }, (this.#types[type].flags2 & 6) === 0 ? 1 : 0); sid !== 0; sid = finder.next()) {
            if (this.#pool.slot(sid)[TYPE] === type) return sid;
        }
        return 0;
    }

    // 공간 접두 뒤에도 원래 부모를 보존하고, 하위 효과 뒤 helper/권한/타입은 현재 상태로 다시 읽는다. (2026-11-09 WB04-E159)
    afterSpatial(runtime, parent) {
        if (runtime.bytes.length !== PATH_LAYOUT.size) throw new TypeError('cleanup 경로 판본 폭 불일치');
        const pool = this.#pool;
        const hooks = this.#hooks;
        const state = this.#state;
        const helper = runtime.helper;
        if (helper !== 0 && helper < pool.capacity && (this.#genus(helper) & HELPER_GENUS) !== 0) {
            const raw = pool.slot(parent);
            const x = cell(coordinate(raw, X), true);
            const y = cell(coordinate(raw, Y), true);
            const finder = new RawSquidFinder(pool, this.#hash, this.#types);
            // helper 가 실제 현재 사각형 안에 있을 때만 예약한다. 원본처럼 첫 일치에서 종료한다.
            for (let sid = finder.begin({ left: x - 2, top: y - 2, right: x + 2, bottom: y + 2 }); sid !== 0; sid = finder.next()) {
                if (sid !== helper) continue;
                if (hooks.reserveShared()) hooks.addShared(runtime.helper, state.helperEvent, 1);
                runtime.clearHelper();
                break;
            }
        }
        if ((pool.slot(parent)[STATE] & 2) === 0) {
            const raw = pool.slot(parent);
            const place = this.returnPlaceAt(coordinate(raw, X), coordinate(raw, Y));
            const nugget = hooks.contained(parent, NUGGET_MASK, currentNuggetType(state), 0);
            if (place !== 0 && nugget !== 0 && this.#owner(place) === this.#owner(parent)) {
                const parentGraph = hooks.graphFor(parent);
                const placeGraph = hooks.graphFor(place);
                if (placeGraph === parentGraph && state.boss) hooks.destroy(nugget, 0);
            }
        }
        if (pool.slot(parent)[TYPE] === state.priestType) {
            const raw = pool.slot(parent);
            const temple = this.nearbyType(coordinate(raw, X), coordinate(raw, Y), 2, state.daisType);
            if (temple !== 0 && (this.#extra(temple) & 8) === 0) hooks.ritual(temple, parent, 0);
        }
        const remaining = runtime.helper;
        if (state.boss && remaining !== 0) {
            if (remaining >= pool.capacity) throw new RangeError('cleanup 남은 helper SID 범위');
            const target = remaining;
            if ((pool.slot(target)[STATE] & 1) === 0) {
                if ((this.#extra(target) & 1) === 0 || (this.#genus(target) & HELPER_GENUS) === 0) {
                    if (state.checking || (typeof state.sharedChecking === 'function' && state.sharedChecking())) throw new Error('cleanup shouldDestroy 원본 assert');
                    return;
                }
                const raw = pool.slot(target);
                hooks.refund(raw[TYPE], this.#owner(target));
                hooks.destroy(target, 0);
            }
        }
    }
}

// 기존 모듈 몸체를 다시 구현하지 않고 정확한 인자/순서의 훅에 연결한다(contained·Graph·공유 예약·환불). 의식/삭제/예약 성공 여부는 호출자 hooks 가 공급한다. (2026-11-09 WB04-E159)
export function makePathCleanupHooks(pool, carrier, anchor, host, place, hooks = {}) {
    if (carrier?.pool !== pool || anchor?.pool !== pool || host?.pool !== pool || place?.pool !== pool) throw new TypeError('cleanup 하위 모듈 SID 풀 불일치');
    return {
        ...hooks,
        contained: (sid, mask, kind, index) => carrier.findContained(sid, mask, kind, index),
        graphFor: (sid) => anchor.graphFor(sid),
        addShared: (sid, event, payload) => { host.addSharedRegular(sid, event, payload); },
        refund: (type, owner) => { place.refund(type, owner); }
    };
}
