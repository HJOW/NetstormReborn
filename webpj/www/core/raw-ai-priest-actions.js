// 사제 AI 의 의식/건설/탐색/회수/수집·귀환 전체 행동(패치 00411cd0)과 그 헬퍼를 옮긴 모듈이다(cpppj `o/RawAiPriestActions.cpp/.h`, 10.78).
//  - snapAiPriestPoint(point): 0041d7a0 — 각 축에 float 0.99999 를 더한 뒤(중간 float 저장 없이) signed ftol 하위 DWORD 를 float 으로 저장한다.
//  - aiPriestPointValid(point, mapLimit): 0040e800 — 두 float 모두 0 보다 크고 현재 mapLimit 을 float 에 저장한 값보다 작아야 한다(NaN 은 false).
//  - aiPriestInRitual(pool, frames, priest): 00491b30 — 현재 물리 frame(+36)의 프레임 코드 side 를 signed char 로 읽은 방향(side − 'A')이 10..14 인지 본다(상태 BYTE 로 대신하지 않는다).
//  - aiAltarContainsEnemy(pool, dais, enemy, altarType, find): 00448a90 — 다이스 위치의 실제 제단(find(x, y, type), 찾은 SID 0 도 읽음) argument WORD 를 적 owner 전체 DWORD 와 비교한다.
//  - new RawAiPriestActions(memory, pool, globals, hooks): memory 는 E164 `RawAiBrainMemory`, pool 은 같은 PATCH SidPool, globals = { now()(0055b4d0 현재 게임 시각, 각 기한 쓰기 위치에서 재조회), mapLimit()(00531928 현재 signed 지도 크기), daisType()(005412ec 현재 다이스 TYPE) },
//    hooks = { scan(token, budget), vulnerable(sid), busy(sid), path(sid)→SID, maxHitPoints(sid)→int32, ritual(sid), altarEnemy(enemy, sid), target(token, actor, target, sacrifice), distance(point, sid), select(token)→TYPE,
//    home(token, sid), collect(token, sid), deploy(token, type, point), begin(token, request)(0040f660, request = { type, bounds, avoid, owner } 값 snapshot), point(token, actor, point), enemyLowFunds(token), salvage(sid) }.
//  - act(token, priest): 00411cd0 전체 — 탐색 접두(진행 중이면 scan(0) 만) → 사제 0/생각 기한(+0x5a8, unordered 미도달)/상처/예약 → 경로가 있으면 최대 HP 조회만 → 건설 기한(+0x5c0) → 다이스/제단(+0x2a4 와 능력 0x400)의 의식/적 제단/거리 2 초과 이동 →
//    다음 건설 TYPE 선택 → 지갑 400 미만 저자금 귀환/수집 → 목표 유효성/TYPE 0 → 현재 위치 보정 좌표가 목표와 같으면 20초 기한 뒤 배치 → 탐색 기한(+0x5c8) → 같은 TYPE 완료 탐색의 최적 좌표 소비·보정·이동 또는 새 탐색 시작 →
//    다이스 회수(건물 중 signed HP 최대 첫 SID 또는 신전, 회수 기한 +0x5e8 이 0 일 때만 대상 이동·회수 요청·10초 기한·재탐색·scan(1)) → 탐색 기한 3초. 각 관문 뒤 본체 필드를 다시 읽고 선택된 하위 누락은 앞선 효과를 보존하며 명시 실패한다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E171: AI 사제 행동 이식으로 추가)
import { AI_BRAIN_BYTES } from './raw-ai-brain.js';
import { pathDirection } from './raw-path-animation.js';

// 0041d7a0 의 float 0.99999 보정과 다이스 거리 경계(PE 00501444 의 float 2.0), 원본 double 기한(PE 005012d8/00501448/00501450)과 목표 미정 −10000.0f 비트이다. (2026-11-09 WB04-E171)
const SNAP_BIAS = Math.fround(0.99999);
const DAIS_DISTANCE = 2;
const BUILD_DELAY = 20;
const SEARCH_DELAY = 3;
const SALVAGE_DELAY = 10;
const UNSET = 0xc61c4000;
const TWO_63 = 9223372036854775808;

// 비정렬 DWORD 를 물리 범위 안에서 읽는다. (2026-11-09 WB04-E171)
function read(raw, at) {
    if (at < 0 || at > raw.length || raw.length - at < 4) throw new RangeError('AI 사제 행동 DWORD 범위');
    return (raw[at] | (raw[at + 1] << 8) | (raw[at + 2] << 16) | (raw[at + 3] << 24)) >>> 0;
}

// 지정 DWORD 만 쓰고 주변 원문을 보존한다. (2026-11-09 WB04-E171)
function put(raw, at, value) {
    if (at < 0 || at > raw.length || raw.length - at < 4) throw new RangeError('AI 사제 행동 쓰기 범위');
    for (let index = 0; index < 4; index += 1) raw[at + index] = (value >>> (index * 8)) & 0xff;
}

// 원본 DWORD 의 signed 해석이다. (2026-11-09 WB04-E171)
const signed = (value) => value | 0;

// 원본 ftol 의 signed64 절삭 하위 DWORD(signed)이며 무효 FISTP 의 하위값은 0 이다. (2026-11-09 WB04-E171)
function ftol(value) {
    if (!Number.isFinite(value) || value < -TWO_63 || value >= TWO_63) return 0;
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// 단정도 float 와 비트의 상호 변환이다. (2026-11-09 WB04-E171)
function floatBits(value) {
    const view = new DataView(new ArrayBuffer(4));
    view.setFloat32(0, value, true);
    return view.getUint32(0, true);
}
function bitsFloat(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// 선택된 실제 하위가 없으면 앞선 효과를 보존하고 명시 실패한다. (2026-11-09 WB04-E171)
function need(hook) {
    if (typeof hook !== 'function') throw new Error('AI 사제 행동 실제 하위 미연결');
    return hook;
}

// 0041d7a0: float 0.99999 더하기 뒤 중간 float 축소 없이 ftol 하며 하위 signed DWORD 를 float 로 저장한다. (2026-11-09 WB04-E171)
export function snapAiPriestPoint(point) {
    return { x: Math.fround(ftol(point.x + SNAP_BIAS)), y: Math.fround(ftol(point.y + SNAP_BIAS)) };
}

// 0040e800: 원본은 mapLimit 을 float 에 한 번 저장해 두 축을 검사한다(NaN 은 false). (2026-11-09 WB04-E171)
export function aiPriestPointValid(point, mapLimit) {
    const limit = Math.fround(mapLimit);
    return point.x > 0 && point.x < limit && point.y > 0 && point.y < limit;
}

// 00491b30: 상태 BYTE/물리 frame 번호 자체로 의식 진행을 대신하지 않고 현재 프레임 코드 방향이 10..14 인지 본다. (2026-11-09 WB04-E171)
export function aiPriestInRitual(pool, frames, priest) {
    const raw = pool.slot(priest);
    if (raw[10] >= frames.length) throw new RangeError('AI 사제 의식 현재TYPE 범위');
    const phase = pathDirection(frames[raw[10]], signed(read(raw, 36)));
    return phase > 9 && phase < 15;
}

// 00448a90: 제단 조회 후 현재 argument WORD 를 읽으며 적 owner 를 WORD 로 잘라 비교하지 않는다(찾은 SID 0 도 읽는다). (2026-11-09 WB04-E171)
export function aiAltarContainsEnemy(pool, dais, enemy, altarType, find) {
    const raw = pool.slot(dais);
    const altar = need(find)(bitsFloat(read(raw, 14)), bitsFloat(read(raw, 18)), altarType);
    const found = pool.slot(altar);
    const argument = found[12] | (found[13] << 8);
    return argument === (enemy >>> 0);
}

// 사제 행동 처리기이다. 같은 0xb08/실제 pool 과 선택된 행동 하위에서 수행하며 자동 프레임 순서는 별도이다. (2026-11-09 WB04-E171)
export class RawAiPriestActions {
    #memory;
    #pool;
    #globals;
    #hooks;

    // 같은 PATCH 풀/실제 본체와 현재 전역 조회기를 빌린다. 생성은 게임 행동/장치를 호출하지 않는다. (2026-11-09 WB04-E171)
    constructor(memory, pool, globals, hooks) {
        if (!memory || typeof memory.bytes !== 'function') throw new TypeError('AI 사제 행동 본체 소유자 누락');
        if (pool?.edition !== 'patch1078') throw new TypeError('AI 사제 행동 같은PATCH풀 누락');
        for (const name of ['now', 'mapLimit', 'daisType']) {
            if (typeof globals?.[name] !== 'function') throw new TypeError(`AI 사제 행동 현재 전역 조회기 누락: ${name}`);
        }
        this.#memory = memory;
        this.#pool = pool;
        this.#globals = globals;
        this.#hooks = hooks ?? {};
    }

    // 같은 실제 월드의 풀인지 확인할 수 있도록 제공한다. (2026-11-09 WB04-E171)
    get pool() { return this.#pool; }

    // 해제/가짜 본체를 거부하고 현재 살아 있는 0xb08 본체를 빌린다. (2026-11-09 WB04-E171)
    #body(token) {
        const raw = this.#memory.bytes(token);
        if (raw.length !== AI_BRAIN_BYTES) throw new RangeError('AI 사제 행동 실제0xb08본체 누락');
        return raw;
    }

    // 하위 효과 뒤 현재 DWORD 를 다시 읽는다. (2026-11-09 WB04-E171)
    #field(token, at) { return read(this.#body(token), at); }

    // 지정 필드만 갱신한다. (2026-11-09 WB04-E171)
    #set(token, at, value) { put(this.#body(token), at, value >>> 0); }

    // 두 DWORD 를 원본 QWORD(double)로 읽는다. (2026-11-09 WB04-E171)
    #time(token, at) {
        const view = new DataView(new ArrayBuffer(8));
        view.setUint32(0, this.#field(token, at), true);
        view.setUint32(4, this.#field(token, at + 4), true);
        return view.getFloat64(0, true);
    }

    // 현재 시각은 각 기한 쓰기 위치에서 새로 읽어 now + delay(double)를 두 DWORD 로 쓴다. (2026-11-09 WB04-E171)
    #deadline(token, at, delay) {
        const view = new DataView(new ArrayBuffer(8));
        view.setFloat64(0, this.#globals.now() + delay, true);
        this.#set(token, at, view.getUint32(0, true));
        this.#set(token, at + 4, view.getUint32(4, true));
    }

    // native now → due 관문과 같이 less/unordered 를 모두 거부한다. (2026-11-09 WB04-E171)
    #ready(token, at) { return this.#globals.now() >= this.#time(token, at); }

    // 잘못된 DWORD 를 다른 정상 SID 로 잘라 쓰지 않는다(0 은 물리 슬롯이다). (2026-11-09 WB04-E171)
    #object(value) {
        if (value >= this.#pool.capacity || value > 65535) throw new RangeError('AI 사제 행동 실제SID 범위');
        return value;
    }

    // 현재 pool 의 raw 두 float 이다. (2026-11-09 WB04-E171)
    #position(actor) {
        const raw = this.#pool.slot(actor);
        return { x: bitsFloat(read(raw, 14)), y: bitsFloat(read(raw, 18)) };
    }

    // 현재 목표(+0xad0/+0xad4)를 읽는다. (2026-11-09 WB04-E171)
    #target(token) { return { x: bitsFloat(this.#field(token, 0xad0)), y: bitsFloat(this.#field(token, 0xad4)) }; }

    // 같은 목표 원문의 두 성분만 쓴다. (2026-11-09 WB04-E171)
    #setTarget(token, point) {
        this.#set(token, 0xad0, floatBits(point.x));
        this.#set(token, 0xad4, floatBits(point.y));
    }

    // 좌표 미정값은 원본처럼 y → x 순서로 지운다. (2026-11-09 WB04-E171)
    #clearTarget(token) {
        this.#set(token, 0xad4, UNSET);
        this.#set(token, 0xad0, UNSET);
    }

    // 각 초기화 호출 시 현재 여덟 인자를 같은 본체에서 읽고 하위에 값으로 넘긴다(NaN 회피 좌표는 원본 비트도 함께 전달). (2026-11-09 WB04-E171)
    #beginSearch(token, type) {
        const request = {
            type,
            bounds: { left: bitsFloat(this.#field(token, 0x270)), top: bitsFloat(this.#field(token, 0x274)), right: bitsFloat(this.#field(token, 0x278)), bottom: bitsFloat(this.#field(token, 0x27c)) },
            avoid: { x: bitsFloat(this.#field(token, 0x2b0)), y: bitsFloat(this.#field(token, 0x2b4)), xBits: this.#field(token, 0x2b0), yBits: this.#field(token, 0x2b4) },
            owner: this.#field(token, 0x260)
        };
        need(this.#hooks.begin)(token, request);
    }

    // HP 는 최대 HP getter 전에 저장하고 기능 800h 는 그 효과 뒤 현재 값을 읽는다. (2026-11-09 WB04-E171)
    #fallback(token, priest) {
        const hooks = this.#hooks;
        if (this.#field(token, 0x2cc)) {
            need(hooks.home)(token, priest);
            return;
        }
        const hp = signed(read(this.#pool.slot(priest), 26));
        const maximum = need(hooks.maxHitPoints)(priest) | 0;
        if (maximum <= hp && (this.#field(token, 0x248) & 0x800)) need(hooks.collect)(token, priest);
        else need(hooks.home)(token, priest);
    }

    // 원본 최초 count/pointer 를 사용하고 같은 HP 이면 앞선 SID 를 유지한다(추가 생존 필터 없음). (2026-11-09 WB04-E171)
    #bestBuilding(token) {
        const count = signed(this.#field(token, 0x308));
        let selected = 0;
        if (count > 0) {
            const list = this.#memory.bytes(this.#field(token, 0x300));
            // 처음 목록을 정순으로 비교해 signed HP 가 더 큰 첫 SID 만 바꾼다.
            for (let index = 0; index < count; index += 1) {
                const value = read(list, index * 4);
                if (!selected || signed(read(this.#pool.slot(this.#object(selected)), 26)) < signed(read(this.#pool.slot(this.#object(value)), 26))) selected = value;
            }
        }
        return this.#object(selected);
    }

    // 전체 prefix/각 관문/현재 재조회와 원래 요청 순서를 보존한다. (2026-11-09 WB04-E171)
    act(token, priest) {
        const hooks = this.#hooks;
        const globals = this.#globals;
        if (signed(this.#field(token, 0x5a4)) < signed(this.#field(token, 0x5a0))) {
            need(hooks.scan)(token, 0);
            return;
        }
        if (!priest || !this.#ready(token, 0x5a8) || need(hooks.vulnerable)(priest) || need(hooks.busy)(priest)) return;
        if (need(hooks.path)(priest)) {
            need(hooks.maxHitPoints)(priest);
            return;
        }
        if (!this.#ready(token, 0x5c0)) return;
        if (this.#field(token, 0x2a4) && (this.#field(token, 0x248) & 0x400)) {
            if (need(hooks.ritual)(this.#object(this.#field(token, 0x29c)))) return;
            if (need(hooks.altarEnemy)(this.#field(token, 0x264), this.#object(this.#field(token, 0x2a4)))) {
                const dais = this.#object(this.#field(token, 0x2a4));
                const self = this.#object(this.#field(token, 0x29c));
                need(hooks.target)(token, self, dais, 1);
                return;
            }
            const distance = need(hooks.distance)(this.#position(priest), this.#object(this.#field(token, 0x2a4)));
            if (distance > DAIS_DISTANCE) {
                need(hooks.target)(token, priest, this.#object(this.#field(token, 0x2a4)), 0);
                return;
            }
        }
        const type = need(hooks.select)(token) >>> 0;
        if (this.#field(token, 0x294) && !this.#field(token, 0x2cc) && signed(this.#field(token, 0x2d8)) < 400) {
            this.#fallback(token, priest);
            return;
        }
        const target = this.#target(token);
        if (!aiPriestPointValid(target, globals.mapLimit())) {
            if (!type) {
                this.#fallback(token, priest);
                return;
            }
        } else if (!type) {
            this.#clearTarget(token);
            return;
        }
        const snapped = snapAiPriestPoint(this.#position(priest));
        const current = this.#target(token);
        if (snapped.x === current.x && snapped.y === current.y) {
            this.#deadline(token, 0x5c0, BUILD_DELAY);
            need(hooks.deploy)(token, type, this.#position(priest));
            this.#clearTarget(token);
            return;
        }
        if (!this.#ready(token, 0x5c8)) return;
        if (type === this.#field(token, 0x574)) {
            if (signed(this.#field(token, 0x5a0)) <= signed(this.#field(token, 0x5a4))) {
                this.#setTarget(token, { x: bitsFloat(this.#field(token, 0x57c)), y: bitsFloat(this.#field(token, 0x580)) });
                this.#set(token, 0x574, 0);
                this.#setTarget(token, snapAiPriestPoint(this.#target(token)));
                if (aiPriestPointValid(this.#target(token), globals.mapLimit())) {
                    need(hooks.point)(token, priest, this.#target(token));
                    return;
                }
            }
        } else this.#beginSearch(token, type);
        if (type === (globals.daisType() >>> 0) && signed(this.#field(token, 0x5a4)) >= signed(this.#field(token, 0x5a0)) && !aiPriestPointValid(this.#target(token), globals.mapLimit()) && this.#time(token, 0x5e8) === 0) {
            let selected = this.#bestBuilding(token);
            if (!selected) {
                if (!need(hooks.enemyLowFunds)(token)) {
                    this.#deadline(token, 0x5c8, SEARCH_DELAY);
                    return;
                }
                selected = this.#object(this.#field(token, 0x294));
                if (!selected) {
                    this.#deadline(token, 0x5c8, SEARCH_DELAY);
                    return;
                }
            }
            need(hooks.target)(token, priest, selected, 0);
            need(hooks.salvage)(selected);
            this.#deadline(token, 0x5e8, SALVAGE_DELAY);
            this.#beginSearch(token, type);
            need(hooks.scan)(token, 1);
        }
        this.#deadline(token, 0x5c8, SEARCH_DELAY);
    }
}
