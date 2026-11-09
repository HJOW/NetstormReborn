// AI 의 group 3 배치가 쓰는 별도 지상 표적 검색(0046b540 Find·0046b360 MayTarget·Depend 조회)을 같은 실제 Gun 월드에 연결한 모듈이다(cpppj `o/RawGunGroundTarget.cpp/.h`, 10.78 전용).
//  - new GunGroundTargetGlobals(): 원본이 조회 위치마다 다시 읽는 전역이다. excludedType(00541178 보행 운반체 검색 제외 TYPE, 72)·dependType(00541074 Depend Form TYPE, 1)·checking(005e4794 전체 DWORD, sharedChecking getter)·
//    metamap(00594fc8)·boss(00540bc4)·assertion(식, 줄)(원본 진단 장치, 반환하면 원래 순서로 계속). 호출자가 필드를 바꾸면 다음 조회 위치에서 바로 반영된다.
//  - new RawGunGroundTarget(pool, hash, types, targetTypes, targeting, state, globals): targeting 은 같은 풀의 RawGunTarget(`raw-gun-target.js`, 공통 HP·서버 번호·동맹·사제 수량·발자국 필터), state 는 그 Gun 전역
//    ({ friendlyFire, allyMasks[9], allyMaskRead?(소유자) }), types/targetTypes 는 188칸 TYPE 표와 Gun 검색 자료({ range, heightExtension })다.
//    · find(x, y, type, owner): 문맥(방향/프레임 0, 중심 = 좌표 − 발자국 절반 float) → 중심을 +0.99999f 뒤 절삭·float 저장·다시 절삭한 칸 ± trunc(사거리) 사각형으로 실제 일반 finder(flags 5)를 시작 →
//      첫 후보 준비 뒤 자기 동맹 진단(friendlyFire 꺼짐·자기 비트 없음이면 원본 assert 0x2fc) → 후보마다 HP 보유(flags1 0x10)와 mayTarget 을 통과한 것 중 반대쪽 경계 중점까지의 double 제곱거리가
//      float(사거리²)보다 엄격히 작은 최소를 고른다(같은 거리/NaN 은 먼저 고른 것을 유지, ground 가산 없음, 사정선 그림 없음).
//    · mayTarget(query, target): 비행/벌룬(flags2 0x120000)은 거부 → 보행 운반체(flags2 0x10000)는 문맥 TYPE 이 제외 TYPE 이거나 Depend 체인에 현재 dependType 인 자산 TYPE(<70) Form 이 있으면 거부 → 기존 RawGunTarget.mayTarget.
//    · Depend 조회: checking·비 metamap·dead 부모·비 boss 면 원본 진단 0x158 뒤 현재 head(+6)를 다시 읽고, next(+4)를 먼저 저장한 뒤 TYPE<70 이고 dependType 인 첫 Form 을 찾는다.
//      free/dead 자식도 원문대로 통과하며, 일치 없이 순환하거나 물리 범위 밖이면 명시 오류다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E211: 지상 표적 검색 이식으로 추가)
import { RawSquidFinder } from './raw-squid-finder.js';
import { RawSquidNeighbors } from './raw-squid-neighbors.js';

// 원본 00502310 의 float 0.99999(일반 ceil 0.9999 와 다름)이다. (2026-11-09 WB04-E211)
const SNAP_BIAS = Math.fround(0.99999);
// 후보 flags1 HP 보유 비트, flags2 의 비행/벌룬·보행 운반체 비트이다. (2026-11-09 WB04-E211)
const HIT_POINTS = 0x10;
const AIRBORNE = 0x120000;
const WALKER = 0x10000;
// 슬롯의 TYPE·상태·Depend 머리/다음 위치와 자산 TYPE 시작 번호, 최대 Player 번호, 일반 finder flags 이다. (2026-11-09 WB04-E211)
const TYPE_AT = 10;
const STATE_AT = 11;
const HEAD_AT = 6;
const NEXT_AT = 4;
const FIRST_ASSET_TYPE = 70;
const PLAYER_LAST = 8;
const FINDER_FLAGS = 5;
const TYPE_COUNT = 188;
// 두 원본 진단의 식과 줄 번호이다. (2026-11-09 WB04-E211)
const DEPEND_TEXT = 'inMetamapMode || !cont.isDead() || boss || yesIKnowThatIShouldntLookInsideMyselfDuringADestroy';
const DEPEND_LINE = 0x158;
const ALLY_TEXT = 'shootMyFriends || me->isAlly(me->getPlayerId())';
const ALLY_LINE = 0x2fc;
// int32 경계이다. (2026-11-09 WB04-E211)
const INT_MIN = -2147483648;
const INT_MAX = 2147483647;

// 정상 지도/사거리의 signed 절삭이다. 범위 밖 입력은 자동 clamp 하지 않고 거부한다. (2026-11-09 WB04-E211)
function integer(value) {
    if (!Number.isFinite(value) || value < INT_MIN || value > INT_MAX) throw new RangeError('지상 표적 정수 좌표 범위');
    return Math.trunc(value) | 0;
}

// 00422230: 0041d7a0 로 보정해 float 로 저장한 뒤 다시 절삭한다. (2026-11-09 WB04-E211)
function cell(value) {
    return integer(Math.fround(integer(value + SNAP_BIAS)));
}

// 범위 조합이 signed 넘침으로 다른 주소가 되지 않게 거부한다. (2026-11-09 WB04-E211)
function bound(center, distance, maximum) {
    const value = maximum ? center + distance : center - distance;
    if (value < INT_MIN || value > INT_MAX) throw new RangeError('지상 표적 검색 사각형 범위');
    return value;
}

// 원본 전역의 현재 값 묶음이다. 소비 위치마다 필드를 다시 읽는다. (2026-11-09 WB04-E211)
export class GunGroundTargetGlobals {
    excludedType = 72;
    dependType = 1;
    checking = 0;
    sharedChecking = null;
    metamap = false;
    boss = true;
    assertion = null;

    // 진단 전역을 비영으로만 판정한다. (2026-11-09 WB04-E211)
    isChecking() { return (this.sharedChecking ? this.sharedChecking() >>> 0 : Number(this.checking) >>> 0) !== 0; }
}

// 별도 지상 검색의 실제 후보 순서/Depend 필터/좌표 보정과 float 저장 위치를 보존한다. (2026-11-09 WB04-E211)
export class RawGunGroundTarget {
    // 같은 풀·해시·TYPE 표·Gun 검색 자료·공통 Gun 필터·Gun 전역·지상 전역이다.
    #pool;
    #hash;
    #types;
    #targetTypes;
    #targeting;
    #state;
    #globals;

    // 같은 PATCH 풀과 현재 전체 TYPE 표를 빌린다. (2026-11-09 WB04-E211)
    constructor(pool, hash, types, targetTypes, targeting, state, globals) {
        if (pool?.edition !== 'patch1078' || targeting?.pool !== pool || !Array.isArray(types) || types.length !== TYPE_COUNT || targetTypes?.length !== types.length || !(globals instanceof GunGroundTargetGlobals)) {
            throw new TypeError('지상 표적 같은 PATCH 풀/TYPE 누락');
        }
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#targetTypes = targetTypes;
        this.#targeting = targeting;
        this.#state = state;
        this.#globals = globals;
    }

    // 같은 실제 월드 연결 확인용 풀이다. (2026-11-09 WB04-E211)
    get pool() { return this.#pool; }

    // 원본 진단 장치를 부른다. 없으면 묵시 통과 대신 오류다. (2026-11-09 WB04-E211)
    #assert(text, line, missing) {
        const assertion = this.#globals.assertion;
        if (typeof assertion !== 'function') throw new Error(missing);
        assertion(text, line);
    }

    // 현재 소유자 동맹 마스크이다(실제 세션 getter 우선). (2026-11-09 WB04-E211)
    #allyMask(owner) {
        const state = this.#state;
        return (state.allyMaskRead ? state.allyMaskRead(owner) : state.allyMasks[owner]) >>> 0;
    }

    // 004b22d0/004b1b80/004b1bd0: 진단 뒤 현재 head 를 읽고 next 를 먼저 저장하며 TYPE<70·현재 dependType 첫 일치를 찾는다. (2026-11-09 WB04-E211)
    #hasDependency(parent) {
        const globals = this.#globals;
        const raw = this.#pool.slot(parent);
        if (globals.isChecking() && !globals.metamap && (raw[STATE_AT] & 2) && !globals.boss) this.#assert(DEPEND_TEXT, DEPEND_LINE, '지상 표적 Depend 진단 미연결');
        const head = this.#pool.slot(parent);
        let current = head[HEAD_AT] | (head[HEAD_AT + 1] << 8);
        const visited = new Set();
        // next 는 후보를 판정하기 전에 저장하며 자산 TYPE 70 이상은 건너뛴다.
        while (current) {
            if (current >= this.#pool.capacity || visited.has(current)) throw new Error('지상 표적 Depend 체인 손상');
            visited.add(current);
            const candidate = this.#pool.slot(current);
            const next = candidate[NEXT_AT] | (candidate[NEXT_AT + 1] << 8);
            if (candidate[TYPE_AT] < FIRST_ASSET_TYPE && candidate[TYPE_AT] === globals.dependType) return true;
            current = next;
        }
        return false;
    }

    // 0046b360: 비행/벌룬 제외 → 보행 운반체의 현재 TYPE/Depend → 기존 실제 mayTarget 이다. (2026-11-09 WB04-E211)
    mayTarget(query, target) {
        const raw = this.#pool.slot(target);
        if (raw[TYPE_AT] >= this.#types.length) throw new RangeError('지상 표적 현재 TYPE 범위');
        if (this.#types[raw[TYPE_AT]].flags2 & AIRBORNE) return false;
        if (this.#types[this.#pool.slot(target)[TYPE_AT]].flags2 & WALKER) {
            if (query.type === this.#globals.excludedType || this.#hasDependency(target)) return false;
        }
        return this.#targeting.mayTarget(query, target);
    }

    // 00469440: 확장 발자국에서 반대 방향 경계 중점까지의 double 제곱거리이다. 비교 전에 float 로 줄이지 않는다. (2026-11-09 WB04-E211)
    #distance(query, target) {
        const rect = this.#targeting.footprint(target);
        const midX = Math.fround((rect.left + rect.right) * 0.5);
        const midY = Math.fround((rect.top + rect.bottom) * 0.5);
        const direction = (RawSquidNeighbors.direction(query.centerX, query.centerY, midX, midY) + 4) & 7;
        let x;
        let y;
        if (direction === 0 || direction === 4) {
            x = Math.fround((rect.right - rect.left) * 0.5 + rect.left);
            y = direction === 0 ? rect.top : rect.bottom;
        } else {
            x = direction === 2 ? rect.right : rect.left;
            y = Math.fround((rect.bottom - rect.top) * 0.5 + rect.top);
        }
        const dx = query.centerX - x;
        const dy = query.centerY - y;
        return dx * dx + dy * dy;
    }

    // 0046b540: 최초 후보 준비 뒤 자기 동맹을 검사하며 등거리/NaN 후보는 기존 최초 선택을 유지한다. 반환은 표적 SID(없으면 0)다. (2026-11-09 WB04-E211)
    find(x, y, type, owner) {
        if (!(type >= 0 && type < this.#types.length) || !(owner >= 0 && owner <= PLAYER_LAST) || !Number.isFinite(x) || !Number.isFinite(y)) throw new RangeError('지상 표적 검색 문맥 범위');
        const source = this.#types[type];
        const query = { x, y, centerX: Math.fround(x - source.footX * 0.5), centerY: Math.fround(y - source.footY * 0.5), type, direction: 0, owner, frame: 0 };
        const range = this.#targetTypes[type].range;
        const distance = integer(range);
        const cx = cell(query.centerX);
        const cy = cell(query.centerY);
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types, this.#targetTypes.map((targetType) => targetType.heightExtension));
        const first = finder.begin({ left: bound(cx, distance, false), top: bound(cy, distance, false), right: bound(cx, distance, true), bottom: bound(cy, distance, true) }, FINDER_FLAGS);
        let best = Math.fround(range * range);
        let result = 0;
        if (!this.#state.friendlyFire && !(this.#allyMask(owner) & (1 << owner))) this.#assert(ALLY_TEXT, ALLY_LINE, '지상 표적 자기 동맹 진단 미연결');
        // 후보 반환 때 저장된 next 와 매 후보의 현재 raw 를 쓴다. 추가 ground 가산은 없다.
        for (let sid = first; sid; sid = finder.next()) {
            const raw = this.#pool.slot(sid);
            if (raw[TYPE_AT] >= this.#types.length) throw new RangeError('지상 표적 후보 TYPE 범위');
            if (!(this.#types[raw[TYPE_AT]].flags1 & HIT_POINTS) || !this.mayTarget(query, sid)) continue;
            const score = this.#distance(query, sid);
            if (score < best) {
                result = sid;
                best = Math.fround(score);
            }
        }
        return result;
    }
}
