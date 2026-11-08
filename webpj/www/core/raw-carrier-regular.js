// 골렘(Carrier)의 Regular Handle(42a750)을 옮긴 모듈이다(cpppj `o/RawCarrierRegular.cpp/.h`, 10.78 전용). 내용물 표시/시전 기한/회피/낙하 사건과 Damageable 위임이다.
//  - 사건 0x493(시전 기한): 현재 내용물(find 1)의 종류 TYPE 의 casttime×10(32비트 감김)이 현재 횟수(signed)보다 작으면 0 을 반환한다(빈 결과도 원본처럼 슬롯 0 에서 종류를 읽음).
//  - 사건 0x491(내용물 교대 표시): 기한 `1.5f/payload`(float 저장)와 횟수(float)를 비교하며 mask 0x20 → 1 순서로 내용물을 조회한다(조회마다 기한을 먼저 검사). 내용물 종류가 허용 flags2(0x208100)가 아니면 0, 현재 횟수 홀짝에 맞는 쪽이면
//    새 미리보기를 만들어(기본 프레임 +36 직접 쓰기, +35 = 부모−1, extra|=8, 단어/소유자 지정, 부모 좌표를 [1, 255] clamp 하고 y 에서 0.8/1.2 를 뺀 위치로 Pop, 타이머 할당 성공 때만 타이머 생성) 둘 다 없으면 0, 아니면 부모 표시 갱신(refresh)을 부른다.
//  - 사건 0x492(회피, boss 만): 현재 부모 좌표가 다리 칸/코드 비트 판정(needsEscape)에 걸리고 이동 프로세스가 없으면 회피(1.0, 0xffffffff)를 실행한다.
//  - 사건 0x5105 는 낙하(falling) 반환값, signed 0x494 미만의 나머지와 그 밖의 사건은 실제 Damageable 기본 처리기로 넘어간다.
// 큰 하위 몸체만 훅이다: find(parent, mask, kind, index)→SID, create(type, 2)→SID, setWord(sid, 값), setOwner(sid, 소유자), pop(sid, x, y, flags), allocateTimer()→bool, constructTimer(sid, payload), refresh(sid), needsEscape(x, y)→bool,
// movementType(type)→처리 타입, hasPath(sid, type)→bool, escape(sid, 지연, 사건), damageable(sid, 사건, 개수, payload), falling(sid, 개수). details[n] = { defaultFrame, castTime }, boss 는 현재 권한을 돌려주는 getter 이다.
// 실제 가상 표(+5c = 42a750)인 여섯 VT 만 직접 받으며 파생은 caller 분배를 보존한다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E91: Carrier Regular 이식으로 추가)

// 보호 PE 의 실제 +5c=42a750 여섯 VT 와 사건 번호, 허용 내용물 flags2, 깊이 보정(y 에서 빼는 값)이다. (2026-11-08 WB04-E91)
const CARRIER_REGULAR_TABLES = Object.freeze([0x503d80, 0x503e50, 0x503f20, 0x503ff0, 0x5040c0, 0x50e568]);
const EVENT_CAST = 0x493;
const EVENT_SHOW = 0x491;
const EVENT_ESCAPE = 0x492;
const EVENT_FALL = 0x5105;
const EVENT_LIMIT = 0x494;
const ALLOWED_CONTENT = 0x208100;
const SHOW_DEPTH = [Math.fround(0.8), Math.fround(1.2)];
const fround = Math.fround;
// 슬롯의 종류 WORD(+18)·단어 WORD(+12)·TYPE(+10)·소유자(+34)·깊이(+35)·프레임(+36)·extra(+40)·좌표(+14/+18)이다. (2026-11-08 WB04-E91)
const TYPE = 10;
const WORD = 12;
const KIND = 18;
const OWNER = 34;
const DEPTH = 35;
const FRAME = 36;
const EXTRA = 40;
const X_AT = 14;
const Y_AT = 18;

// 비정렬 원문 WORD/DWORD 를 낮은 바이트부터 합친다. (2026-11-08 WB04-E91)
function read(raw, at, width = 4) {
    let value = 0;
    for (let index = 0; index < width; index += 1) value += raw[at + index] * 2 ** (8 * index);
    return value >>> 0;
}

// 직접 프레임 쓰기는 SquidFrame 의 추가 표시 효과를 호출하지 않는다. (2026-11-08 WB04-E91)
function put(raw, at, value) {
    for (let index = 0; index < 4; index += 1) raw[at + index] = (value >>> (index * 8)) & 0xff;
}

// 비정렬 float 좌표를 읽는다(NaN payload 보존). (2026-11-08 WB04-E91)
const view = new DataView(new ArrayBuffer(4));
function coordinate(raw, offset) {
    for (let index = 0; index < 4; index += 1) view.setUint8(index, raw[offset + index]);
    return view.getFloat32(0, true);
}

// 선택한 외부 효과만 필수다. 조기 반환 경로에 없는 효과를 미리 요구하지 않는다. (2026-11-08 WB04-E91)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('Carrier 실제 Regular 하위 몸체 미연결');
    return hook;
}

// 실제 41d0d0 의 unordered 분기는 NaN 을 보존한다. 유한 좌표만 1..255 로 제한한다. (2026-11-08 WB04-E91)
function clamp(value) {
    const lower = value < 1 ? 1 : value;
    return 255 < lower ? 255 : lower;
}

export class RawCarrierRegular {
    // Carrier/내용물/새 미리보기의 동일 raw 풀, 현재 내용물 genus 를 읽는 188 TYPE, 같은 번호의 기본 프레임/casttime, 현재 권한 getter, 실제 큰 생성/부착/가상 효과와 기본 Handle 이다.
    #pool;
    #types;
    #details;
    #boss;
    #hooks;

    // 같은 PATCH 188 TYPE 와 권한 getter 를 확인한다. (2026-11-08 WB04-E91)
    constructor(pool, types, details, boss, hooks) {
        if (pool?.edition !== 'patch1078' || types.length !== 188 || details.length !== types.length || typeof boss !== 'function') throw new TypeError('Carrier Regular는 같은 PATCH188 TYPE가 필요합니다');
        this.#pool = pool;
        this.#types = types;
        this.#details = details;
        this.#boss = boss;
        this.#hooks = hooks ?? {};
    }

    // 같은 Session/Host 연결을 확인하는 풀이다. (2026-11-08 WB04-E91)
    get pool() { return this.#pool; }

    // 실제 +5c 가 42a750 인 여섯 VT 만 직접 받는다(genus/타입 번호로 파생 범위를 넓히지 않음). (2026-11-08 WB04-E91)
    handles(parent) {
        return CARRIER_REGULAR_TABLES.includes(read(this.#pool.slot(parent), 0));
    }

    // 사제 등 파생 부모가 실제 기본 호출로도 사용할 수 있다. (2026-11-08 WB04-E91)
    handle(parent, event, count, payload) {
        const pool = this.#pool;
        const hooks = this.#hooks;
        event >>>= 0;
        count >>>= 0;
        if ((event | 0) < EVENT_LIMIT) {
            if (event === EVENT_CAST) {
                // 원본은 빈 결과도 물리 0 번 슬롯에서 kind 를 읽는다.
                const form = need(hooks.find)(parent, 1, 0, 0);
                const kind = read(pool.slot(form), KIND, 2);
                if (kind >= this.#details.length) throw new RangeError('Carrier 시전 내용물 TYPE 범위 오류');
                const limit = Math.imul(this.#details[kind].castTime, 10);
                if (limit < (count | 0)) return 0;
            } else if (event === EVENT_SHOW) {
                // 두 값 모두 실제 float 저장 경계를 거친다.
                const limit = fround(1.5 / payload);
                const elapsed = fround(count | 0);
                let found = 0;
                // mask 0x20/1 순서로 조회하며 각각 조회 뒤 기한을 검사한다.
                for (let index = 0; index < 2; index += 1) {
                    const form = need(hooks.find)(parent, index === 0 ? 0x20 : 1, 0, 0);
                    if (limit < elapsed) return 0;
                    if (!form) continue;
                    const kind = read(pool.slot(form), KIND, 2);
                    if (kind >= this.#types.length) throw new RangeError('Carrier 표시 내용물 TYPE 범위 오류');
                    if (!(this.#types[kind].flags2 & ALLOWED_CONTENT)) return 0;
                    found += 1;
                    if ((index === 0 && (count & 1)) || (index === 1 && !(count & 1))) {
                        const ghost = need(hooks.create)(kind, 2);
                        const raw = pool.allocated(ghost);
                        if (raw[TYPE] >= this.#details.length) throw new RangeError('Carrier 새 표시 TYPE 범위 오류');
                        put(raw, FRAME, this.#details[raw[TYPE]].defaultFrame >>> 0);
                        raw[DEPTH] = (pool.slot(parent)[DEPTH] - 1) & 0xff;
                        raw[EXTRA] |= 8;
                        need(hooks.setWord)(ghost, read(pool.slot(form), WORD, 2));
                        need(hooks.setOwner)(ghost, pool.slot(parent)[OWNER]);
                        const current = pool.slot(parent);
                        const x = coordinate(current, X_AT);
                        const y = fround(coordinate(current, Y_AT) - SHOW_DEPTH[index]);
                        need(hooks.pop)(ghost, clamp(x), clamp(y), 0);
                        if (need(hooks.allocateTimer)()) need(hooks.constructTimer)(ghost, payload);
                    }
                }
                if (!found) return 0;
                need(hooks.refresh)(parent);
            } else if (event === EVENT_ESCAPE && this.#boss()) {
                const raw = pool.slot(parent);
                if (need(hooks.needsEscape)(coordinate(raw, X_AT), coordinate(raw, Y_AT))) {
                    const type = need(hooks.movementType)(pool.slot(parent)[TYPE]);
                    if (!need(hooks.hasPath)(parent, type)) need(hooks.escape)(parent, 1.0, 0xffffffff);
                }
            }
        } else if (event === EVENT_FALL) return need(hooks.falling)(parent, count);
        return need(hooks.damageable)(parent, event, count, payload);
    }

    // 여섯 직접 VT 이외의 파생은 caller 분배를 보존한다. (2026-11-08 WB04-E91)
    handler(fallback = null) {
        return (parent, event, count, payload) => {
            if (this.handles(parent)) return this.handle(parent, event, count, payload);
            if (typeof fallback !== 'function') throw new Error('다른 실제 Carrier 파생 Regular 미연결');
            return fallback(parent, event, count, payload);
        };
    }
}
