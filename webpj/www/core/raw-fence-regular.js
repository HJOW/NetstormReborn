// 썬 바리케이드(Fence)의 실제 Regular Handle(00457a50)에서 프레임/연결 변경 순서를 보존해 옮긴 모듈이다(cpppj `o/RawFenceRegular.cpp/.h`, 10.78 전용). 실제 가상 표 0x509470 인 부모만 분배한다.
//  - 보호 비트 20(extra&0x20)이면 사건 번호보다 먼저 공통 Damageable 처리기로 넘긴다. 사건 0(애니메이션): 현재 프레임의 글자 구간 크기로 `(프레임+1) % 크기`(구간 첫 번호를 더하지 않는 절대 나머지, 0 나눗셈/INT_MIN÷−1 거부)를 실제 프레임 지정기로 지정한다.
//    소유자 캐시(+42 DWORD)가 현재 소유자(+34)와 다르면 이웃 정리 → 하위 연결 수를 조회해 연결 캐시(+46 DWORD)와 다르면 **한 번 더** 조회해 기록하고 이웃 정리 → 0.1f 를 반환한다. 사건 1 은 이웃 스냅샷 갱신, 사건 2 는 재생성 위임 뒤 0 을 반환한다.
//    사건 3 은 소유자 캐시를 갱신하지 않는 이웃 정리 뒤 정의되지 않은 x87 반환(음수 indefinite NaN 0xffc00000)을 돌려준다. 그 밖의 사건은 Damageable 로 위임한다.
//  - 이웃 정리: 네 방향(0·2·4·6)을 먼저 모아 스냅샷으로 삼고(효과 뒤 다시 검색하지 않음), 사건 0 만 소유자 캐시를 쓴 뒤, 각 이웃마다 기존 표식 삭제 → 삭제 효과 뒤의 현재 좌표 네 float 로 연결선 삭제를 하고, 마지막에 사건 2(0.1f) 예약을 시도한다(할당 실패는 그 예약만 생략).
// hooks = { neighbor(sid, 방향), clearMarkers(sid, 이웃), clearLine([x1, y1, x2, y2]), connections(sid), reserveRegular(), addRegular(sid, 사건, payload), refreshNeighbors(sid), rebuild(sid), damageable(sid, 사건, 개수, payload) } 이며
// 선택된 하위의 누락은 앞선 쓰기를 보존한 채 거부한다. frames 는 TYPE별 글자 구간 표(188), setter 는 같은 풀의 SquidFrame 이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E89: 썬 바리케이드 Regular 이식으로 추가)
import { pathDirection } from './raw-path-animation.js';

// 10.78 썬 바리케이드의 실제 가상 표, 사건 3 의 정의되지 않은 x87 반환 비트(음수 indefinite NaN), 사건 2 예약 payload(0.1f)이다. (2026-11-08 WB04-E89)
const FENCE_HANDLE_TABLE = 0x00509470;
const EMPTY_X87_RESULT = 0xffc00000;
const REBUILD_PAYLOAD = Math.fround(0.1);
const INT_MIN = -2147483648;
// 슬롯의 TYPE(+10)·소유자(+34)·프레임(+36)·extra(+40)·소유자 캐시(+42)·연결 캐시(+46)·좌표(+14/+18) 위치와 보호 비트이다. (2026-11-08 WB04-E89)
const TYPE = 10;
const OWNER = 34;
const FRAME = 36;
const EXTRA = 40;
const OWNER_CACHE = 42;
const CONNECTION_CACHE = 46;
const X_AT = 14;
const Y_AT = 18;
const PROTECTED = 0x20;

// 비정렬 DWORD 의 실제 네 바이트만 읽고 쓴다. (2026-11-08 WB04-E89)
function read(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}
function write(raw, offset, value) {
    for (let index = 0; index < 4; index += 1) raw[offset + index] = (value >>> (index * 8)) & 0xff;
}

// 비정렬 float 좌표를 읽는다(NaN payload 보존). (2026-11-08 WB04-E89)
const view = new DataView(new ArrayBuffer(4));
function coordinate(raw, offset) {
    for (let index = 0; index < 4; index += 1) view.setUint8(index, raw[offset + index]);
    return view.getFloat32(0, true);
}
// 사건 3 이 돌려주는 NaN(부호/payload 보존)이다. (2026-11-08 WB04-E89)
const emptyResult = () => { view.setUint32(0, EMPTY_X87_RESULT, true); return view.getFloat32(0, true); };

// 미선택 효과는 요구하지 않으며 선택된 누락은 앞선 쓰기를 보존한 채 거부한다. (2026-11-08 WB04-E89)
function need(callback) {
    if (typeof callback !== 'function') throw new Error('썬 바리케이드 Handle의 실제 하위 미연결');
    return callback;
}

export class RawFenceRegular {
    // 현재 frame/owner/+2a/+2e 를 소유한 단일 raw 풀, TYPE 별 글자 구간 표, 같은 풀의 실제 프레임 지정기, 선택된 하위의 실제 소비자이다.
    #pool;
    #frames;
    #setter;
    #hooks;

    // 원본 판본과 현재 프레임 지정기의 단일 저장소를 확인한다. (2026-11-08 WB04-E89)
    constructor(pool, frames, setter, hooks) {
        if (pool?.edition !== 'patch1078' || frames?.length !== 188 || setter?.pool !== pool) throw new TypeError('썬 바리케이드 Handle 판본/프레임/풀 불일치');
        this.#pool = pool;
        this.#frames = frames;
        this.#setter = setter;
        this.#hooks = hooks ?? {};
    }

    // TYPE 이름만 같은 다른 부모를 이 처리기로 호출하지 않는다(실제 00509470 가상 표 기록값만 분배). (2026-11-08 WB04-E89)
    handles(sid) {
        return read(this.#pool.slot(sid), 0) === FENCE_HANDLE_TABLE;
    }

    // 네 이웃을 먼저 모으며 사건 0 의 owner 캐시만 그 뒤에 쓴다. 사건 3 은 캐시를 갱신하지 않는다. (2026-11-08 WB04-E89)
    #clearNeighbors(sid, updateOwner = true) {
        const pool = this.#pool;
        const hooks = this.#hooks;
        const neighbors = [0, 0, 0, 0];
        // 원본 0/2/4/6 순서이며 원본 local_1c 의 스냅샷이라 효과 뒤 다시 검색하지 않는다.
        for (let direction = 0; direction < 8; direction += 2) neighbors[direction / 2] = need(hooks.neighbor)(sid, direction);
        if (updateOwner) write(pool.allocated(sid), OWNER_CACHE, pool.slot(sid)[OWNER]);
        // 먼저 모은 네 SID 를 같은 순서로 처리한다.
        for (const neighbor of neighbors) {
            if (!neighbor) continue;
            need(hooks.clearMarkers)(sid, neighbor);
            // mark 삭제 효과 뒤의 현재 좌표를 읽는다.
            const a = pool.slot(sid);
            const b = pool.slot(neighbor);
            need(hooks.clearLine)([coordinate(a, X_AT), coordinate(a, Y_AT), coordinate(b, X_AT), coordinate(b, Y_AT)]);
        }
        this.#schedule(sid);
    }

    // 확보 실패 이후에도 상위 Handle 의 다음 연결 수 조회는 계속된다. (2026-11-08 WB04-E89)
    #schedule(sid) {
        if (need(this.#hooks.reserveRegular)()) need(this.#hooks.addRegular)(sid, 2, REBUILD_PAYLOAD);
    }

    // 보호 bit20 은 사건 번호보다 먼저 검사한다. 사건 0 의 두 변경 분기는 독립이다. (2026-11-08 WB04-E89)
    handle(sid, event, count, payload) {
        const pool = this.#pool;
        const hooks = this.#hooks;
        if (!this.handles(sid)) throw new TypeError('썬 바리케이드 Handle 실제 가상 표 불일치');
        if (pool.slot(sid)[EXTRA] & PROTECTED) return need(hooks.damageable)(sid, event, count, payload);
        if (event === 0) {
            const raw = pool.slot(sid);
            const type = raw[TYPE];
            if (type >= this.#frames.length) throw new RangeError('썬 바리케이드 Handle TYPE 범위 오류');
            const current = read(raw, FRAME) | 0;
            const direction = pathDirection(this.#frames[type], current);
            if (direction < 0 || direction >= 16) throw new RangeError('썬 바리케이드 Handle 글자 범위 오류');
            const size = this.#frames[type].run(String.fromCharCode(0x41 + direction)).count;
            const next = (current + 1) | 0;
            if (!size || (next === INT_MIN && size === -1)) throw new RangeError('썬 바리케이드 Handle 원본 idiv 범위 오류');
            // 원본은 구간의 첫 번호를 더하지 않고 절대 나머지를 지정한다.
            this.#setter.set(sid, (next % size) | 0, 0);
            if (read(pool.slot(sid), OWNER_CACHE) !== pool.slot(sid)[OWNER]) this.#clearNeighbors(sid);
            // 원본은 하위 호출을 마친 뒤 캐시 DWORD 를 읽는다.
            const observed = need(hooks.connections)(sid);
            if (observed !== read(pool.slot(sid), CONNECTION_CACHE)) {
                // 원본은 비교 때와 기록 때 두 번 현재 값을 조회한다.
                const changed = need(hooks.connections)(sid);
                write(pool.allocated(sid), CONNECTION_CACHE, changed);
                this.#clearNeighbors(sid);
            }
            return REBUILD_PAYLOAD;
        }
        if (event === 1) { need(hooks.refreshNeighbors)(sid); return 0; }
        if (event === 2) { need(hooks.rebuild)(sid); return 0; }
        if (event === 3) {
            this.#clearNeighbors(sid, false);
            // 원문은 float 를 정의하지 않고 반환한다. 정상 ABI 의 빈 x87 을 실제 496cb0 이 FST 한 결과를 표현한다.
            return emptyResult();
        }
        return need(hooks.damageable)(sid, event, count, payload);
    }

    // 이미 복원한 다른 부모는 순서와 예외를 유지한 채 fallback 으로 넘긴다. (2026-11-08 WB04-E89)
    handler(fallback = null) {
        return (sid, event, count, payload) => {
            if (this.handles(sid)) return this.handle(sid, event, count, payload);
            if (typeof fallback !== 'function') throw new Error('썬 바리케이드 외 Regular 부모 미연결');
            return fallback(sid, event, count, payload);
        };
    }
}
