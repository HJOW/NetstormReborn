// Damageable 삭제 후처리(첫 드롭 → 섬 정리 → 주변 HP 피해 예약 → 주변 다리 약화/삭제 → 공통 post)를 raw SID 풀 위에서 옮긴 모듈이다(cpppj `o/RawDamageablePostDestroy.cpp/.h`, 10.78 전용; 원본 0044b9e0·드롭 0044b830·다리 약화 00421db0).
// postDestroy(sid, flags) 순서:
//   1) flags 에 0x300000(붕괴|폭발) 비트가 있으면 현재 genus 로 drops(sid, genus, true).
//   2) 일반 객체(extra&9 없음)이고 TYPE flags1 의 0x400 이면 현재 좌표 한 칸에서 잔해 genus(0x1000000)를 찾고, 자기 genus 에 0x10000000 이 없고, (flags 0x200000 이거나 현재 TYPE 이 조용한 표면 TYPE 이 아니면)
//      현재 좌표·flags·현재 소유자 BYTE 로 섬 정리(dropIsland)를 요청한다.
//   3) 전투이고 HP 를 가진 일반 객체이면: 공격 소유자 = (flags>>16)&15.
//      - flags 0x200000 이 없고 (group 이 0·1 이거나 genus 에 볼텍스 0x200): 피해 = 최대 HP/2+1(볼텍스는 400). flags 0x400000 이 없으면 발자국 중심(+0.99999 절삭)에서 반경 footX/2+1 의 일반 탐색(표면 단계 생략)으로
//        HP 를 가진·dead 가 아닌·abstract 가 아닌 후보마다 Regular(현재 피해 사건, payload 0, count = 소유자×65536+피해 — WORD 로 자르지 않는 DWORD 덧셈)를 예약한다.
//      - genus 에 0x130000 이 없으면 반경 2 의 일반 탐색에서 다리 genus(4) 후보마다: 현재 프레임 flags 에 금(0x20)이 없거나 낙하가 이미 예약돼 있으면 단단함(0x40)이 아닐 때 약화(weaken), 아니면 현재 권한(boss)일 때 가상 삭제(destroy, 0).
//   4) 언제나 마지막에 공통 postDestroy(base).
// 모든 효과 뒤에 raw(TYPE·상태·extra·좌표·프레임·소유자)와 전역을 다시 읽는다. 드롭 반복 수만 처음 계산한 값을 유지한다.
//  - RawDamageablePostDestroy(pool, hash, types, frames, hp, footprint, state, hooks): types 는 188칸 TYPE 표(참조; flags1·flags2·group·footX), frames 는 TYPE 별 프레임 코드 배열 [{ side, variant, number, flags }],
//    hp 는 같은 풀의 SquidReward(hasHitPoints·maxHitPoints), footprint 는 같은 풀·해시의 RawSquidNeighbors(center), state 는 DamageablePostDestroyState,
//    hooks = { dropDude(sid, index, random), dropIsland(x, y, flags, owner), regular(sid, event, payload, count), hasBridgeFall(sid)→bool, notifySurface(sid), displayUpdate(sid), destroy(sid, flags), base(sid, flags) } 가 모두 필수.
//    drops(sid, genus, random): 0044b830. SID 0·abstract·드롭 genus(0x174200) 없음·제외 TYPE 다섯 개면 없음. 수량 1(genus 0x4200 이면 5, 0x20000 이면 2, 현재 특별 TYPE 이면 3), 현재 Dude 수가 20 보다 크면 절반.
//    genusAt(x, y, genus)→SID: 좌표를 그대로 절삭한 한 칸의 일반 탐색(genus 에 6 비트가 없으면 표면 단계 생략)에서 genus 가 겹치는 첫 후보. free/dead 를 따로 거르지 않는다(Carrier 도착·이동·얼음 탑이 공유).
//    weaken(sid): 00421db0. 수명 비트(WORD 의 0x78)를 (금 수명−1)로 쓰고 표면 통지 뒤 현재 프레임의 금 간 프레임을 찾아(단단한 프레임은 번호 1 로 봄) 있으면 프레임을 쓰고 표시 갱신을 부른다.
//  - DamageablePostDestroyState: battle(00594fbc)·boss(00540bc4)·dudeCount(0055a4f8)·silentSurfaceType(005412a4, 147)·damageEvent(6699)·bridgeCrackLife(0052f960, 5)·excludedDropTypes(133·156·144·138·187)·specialDropType(135).
//    실제 Client 는 bind*({ value }) 로 현재 전역 소유자를 빌린다(Dude 수는 readCount() 를 가진 장기 목록). 빌린 값은 각 소비 위치에서 다시 읽으며 독립 필드는 그대로 둔다.
// 이 모듈은 DOM 을 쓰지 않는다. CD 10.72 분기(제외 TYPE 4개·프레임 BYTE·extra 35)는 옮기지 않았다. (2026-11-10 WB04-E253: 3-C RawDamageablePostDestroy 이식으로 추가)
import { bridgeCrackedFrame } from './bridge-decay.js';
import { RawSquidFinder } from './raw-squid-finder.js';

// 공통 raw 필드 위치(TYPE·상태·원래 소유자 WORD·좌표·소유자·프레임·extra)와 TYPE 표 길이이다. (2026-11-10 WB04-E253)
const TYPE_OFFSET = 10;
const STATE_OFFSET = 11;
const WORD_OFFSET = 12;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const OWNER_OFFSET = 34;
const FRAME_OFFSET = 36;
const EXTRA_OFFSET = 40;
const TYPE_COUNT = 188;

// 드롭 genus 마스크와 수량 분기(5개·2개), 볼텍스·다리·잔해 genus, 섬 정리 제외 genus, 다리 후처리 제외 genus 이다. (2026-11-10 WB04-E253)
const DROP_MASK = 0x174200;
const DROP_FIVE = 0x4200;
const DROP_TWO = 0x20000;
const VORTEX = 0x200;
const BRIDGE = 4;
const RUBBLE = 0x1000000;
const NO_ISLAND_GENUS = 0x10000000;
const NO_BRIDGE_GENUS = 0x130000;

// 삭제 flags: 붕괴|폭발(첫 드롭), 붕괴(섬 정리 허용·주변 피해 생략), 주변 피해 탐색 생략이다. TYPE flags1 의 섬 정리 대상 비트이다. (2026-11-10 WB04-E253)
const DROP_FLAGS = 0x300000;
const COLLAPSE_FLAG = 0x200000;
const NO_SPLASH_FLAG = 0x400000;
const ISLAND_CLEANUP = 0x400;

// 다리 수명 비트(WORD 의 0x78)와 프레임 flags 의 금(0x20)·단단함(0x40), 제외 TYPE 개수, Dude 절반 경계이다. (2026-11-10 WB04-E253)
const LIFE_MASK = 0x78;
const CRACKED = 0x20;
const HARD = 0x40;
const EXCLUDED_COUNT = 5;
const DUDE_LIMIT = 20;

// finder 반경 중심의 거의 올림 상수(단정도 0.99999, 00502310)이다. 중간 float 저장 없이 double 로 더한다. (2026-11-10 WB04-E253)
const AREA_BIAS = Math.fround(0.99999);

// signed64 CRT 절삭의 표현 범위(±2^63)이다. (2026-11-10 WB04-E253)
const CRT_LIMIT = 9223372036854775808;

const REQUIRED_HOOKS = Object.freeze(['dropDude', 'dropIsland', 'regular', 'hasBridgeFall', 'notifySurface', 'displayUpdate', 'destroy', 'base']);

// 정렬되지 않은 raw 리틀엔디언 값을 읽는다(부호 없음). (2026-11-10 WB04-E253)
function read(raw, offset, width = 4) {
    let value = 0;
    // 원본 필드 폭만큼 낮은 바이트부터 합친다.
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value >>> 0;
}

// 원본 WORD/프레임의 폭만 써 인접 필드를 보존한다. (2026-11-10 WB04-E253)
function write(raw, offset, value, width) {
    // 낮은 바이트부터 지정 폭만 대입한다.
    for (let index = 0; index < width; index += 1) raw[offset + index] = (value >>> (8 * index)) & 0xff;
}

// raw 좌표 float 를 비트 그대로 읽는다. (2026-11-10 WB04-E253)
const floatView = new DataView(new ArrayBuffer(4));
function coordinate(raw, offset) {
    floatView.setUint32(0, read(raw, offset), true);
    return floatView.getFloat32(0, true);
}

// _ftol 의 signed64 절삭 뒤 하위 DWORD 를 signed 로 읽는다. 비유한·표현 밖 입력은 계약 밖이라 거부한다. (2026-11-10 WB04-E253)
function truncate(value) {
    if (!Number.isFinite(value) || value < -CRT_LIMIT || value >= CRT_LIMIT) throw new RangeError('Damageable post 좌표 CRT 범위 오류');
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// signed add/sub 의 원본 DWORD 감김이다. (2026-11-10 WB04-E253)
const add = (value, delta) => (value + delta) | 0;

// { value } 공유 소유자인지 확인한다(null 은 해제). (2026-11-10 WB04-E253)
function owner(box, name) {
    if (box !== null && (typeof box !== 'object' || !('value' in box))) throw new TypeError(`Damageable post ${name} 공유 소유자는 { value } 여야 합니다.`);
    return box;
}

// 실제 PostDestroy 와 드롭 helper 가 매 호출 읽는 원본 전역이다. 독립 입력은 필드를 직접 쓰고 실제 Client 는 bind* 로 현재 전역의 소유자를 빌린다. (2026-11-10 WB04-E253)
export class DamageablePostDestroyState {
    // 00594fbc: 주변 HP 피해와 다리 후처리를 허용한다.
    battle = false;
    // 00540bc4: 금 간 다리의 실제 가상 삭제 권한이다.
    boss = true;
    // 0055a4f8: 20 보다 크면 생성 수를 절반으로 줄인다(signed).
    dudeCount = 0;
    // 005412a4(rainBlocker): 일반 삭제의 섬 정리에서 제외한다.
    silentSurfaceType = 147;
    // 원본 Regular event 이며 packed 피해/owner 는 별도 count 에 쓴다.
    damageEvent = 6699;
    // 0052f960: 약화가 쓰는 life 는 이 값−1 이다.
    bridgeCrackLife = 5;
    // 드롭에서 제외하는 현재 TYPE 전역 다섯 개(원문 단락 비교 순서)이다.
    excludedDropTypes = [133, 156, 144, 138, 187];
    // 현재 이 TYPE 이면 일반 수량 대신 3개를 생성한다(벌프).
    specialDropType = 135;
    #boss = null;
    #silent = null;
    #excluded = [null, null, null, null, null];
    #special = null;
    #battle = null;
    #population = null;
    #event = null;

    // 원문 다리 후처리가 읽는 같은 Client boss 를 빌린다.
    bindBoss(box) { this.#boss = owner(box, 'boss'); }
    // 보상과 같은 장기 Client TYPE 번호(005412a4)를 빌린다.
    bindSilentSurfaceType(box) { this.#silent = owner(box, '조용한 표면 TYPE'); }
    // 제외 TYPE 다섯 주소 중 index 번째를 빌린다.
    bindExcludedDropType(index, box) {
        if (!Number.isInteger(index) || index < 0 || index >= EXCLUDED_COUNT) throw new RangeError('Damageable post 제외 TYPE 번호 범위 오류');
        this.#excluded[index] = owner(box, '제외 TYPE');
    }
    // 벌프의 현재 TYPE DWORD 를 빌린다.
    bindSpecialDropType(box) { this.#special = owner(box, '특별 드롭 TYPE'); }
    // 원문의 같은 00594fbc 현재 bool 을 빌린다.
    bindBattle(box) { this.#battle = owner(box, 'battle'); }
    // 0055a4f8 의 현재 개수를 같은 장기 목록(readCount() 를 가진 SquidPostPopList)에서 읽는다.
    bindDudePopulation(list) {
        if (list !== null && typeof list?.readCount !== 'function') throw new TypeError('Damageable post Dude 목록은 readCount() 를 가져야 합니다.');
        this.#population = list;
    }
    // 같은 Client 의 실제 피해 Regular DWORD 를 빌린다.
    bindDamageEvent(box) { this.#event = owner(box, '피해 사건'); }

    // 드롭·섬·주변 피해 효과 뒤 현재 권한이다.
    get currentBoss() { return Boolean(this.#boss ? this.#boss.value : this.boss); }
    // 앞선 드롭/finder 효과 뒤 현재 조용한 표면 TYPE DWORD 이다.
    get currentSilentSurfaceType() { return (this.#silent ? this.#silent.value : this.silentSurfaceType) >>> 0; }
    // 각 실제 비교 시점의 제외 TYPE 전체 DWORD 이다.
    excludedDropType(index) { return (this.#excluded[index] ? this.#excluded[index].value : this.excludedDropTypes[index]) >>> 0; }
    // 제외 비교를 통과한 뒤 읽는 현재 벌프 TYPE DWORD 이다.
    get currentSpecialDropType() { return (this.#special ? this.#special.value : this.specialDropType) >>> 0; }
    // 섬/드롭 효과 뒤 현재 전투 모드이다.
    get currentBattle() { return Boolean(this.#battle ? this.#battle.value : this.battle); }
    // 수량 계산 시점의 현재 population 을 같은 signed 비트로 읽는다.
    get currentDudeCount() { return (this.#population ? this.#population.readCount() : this.dudeCount) | 0; }
    // 각 현재 예약의 실제 사건 번호이다.
    get currentDamageEvent() { return (this.#event ? this.#event.value : this.damageEvent) >>> 0; }
}

export class RawDamageablePostDestroy {
    // 단일 SID 풀, 일반 finder 의 해시, TYPE 표, 프레임 코드 표, HP 몸체, 발자국 중심 몸체, 현재 전역, 하위 효과이다.
    #pool;
    #hash;
    #types;
    #frames;
    #hp;
    #footprint;
    #state;
    #hooks;

    // pool/hash/HP/중심/효과는 같은 월드에 속하며 이 객체보다 오래 살아야 한다. 같은 실제 자료와 모든 하위 효과를 구성 때 확인한다. (2026-11-10 WB04-E253)
    constructor(pool, hash, types, frames, hp, footprint, state, hooks) {
        if (pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length !== TYPE_COUNT || frames?.length !== types.length
            || hp?.pool !== pool || footprint?.pool !== pool || footprint?.hash !== hash || REQUIRED_HOOKS.some((name) => typeof hooks?.[name] !== 'function')) {
            throw new TypeError('Damageable post 실제 자료/효과 연결 오류');
        }
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#frames = frames;
        this.#hp = hp;
        this.#footprint = footprint;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 실제 Dais/Altar 어댑터가 같은 풀인지 확인할 때 쓰는 참조이다. (2026-11-10 WB04-E253)
    get pool() { return this.#pool; }

    // 삭제 뒤 raw 의 dead 상태도 원본처럼 읽으며 명시 SID/TYPE 범위만 제한한다. (2026-11-10 WB04-E253)
    #slot(sid) {
        if (!sid || sid >= this.#pool.capacity) throw new RangeError('Damageable post SID 오류');
        const raw = this.#pool.slot(sid);
        if (raw[TYPE_OFFSET] >= this.#types.length) throw new RangeError('Damageable post 타입 오류');
        return raw;
    }

    // 콜백 뒤 새 raw TYPE 을 다시 조회한다. (2026-11-10 WB04-E253)
    #type(sid) { return this.#types[this.#slot(sid)[TYPE_OFFSET]]; }

    // 현재 원본 genus(TYPE+0xec)이다. (2026-11-10 WB04-E253)
    #genus(sid) { return this.#type(sid).flags2 >>> 0; }

    // 현재 extra 의 abstract/buried 만 검사한다. free/dead 는 별도 원본 분기에서 읽는다. (2026-11-10 WB04-E253)
    #ordinary(sid) { return (this.#slot(sid)[EXTRA_OFFSET] & 9) === 0; }

    // 부모/후보의 현재 raw 좌표이다. (2026-11-10 WB04-E253)
    #position(sid) {
        const raw = this.#slot(sid);
        return [coordinate(raw, X_OFFSET), coordinate(raw, Y_OFFSET)];
    }

    // 일반 탐색기로 한 칸 genus 조회를 한다(좌표는 그대로 절삭, genus 에 6 비트가 없으면 표면 단계 생략). 첫 통과 후보만 돌려주며 free/dead 를 따로 거르지 않는다. (2026-11-10 WB04-E253)
    genusAt(x, y, genus) {
        const ix = truncate(x);
        const iy = truncate(y);
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
        // 원본의 첫 통과 후보만 반환한다.
        for (let current = finder.begin({ left: ix, top: iy, right: ix, bottom: iy }, (genus & 6) === 0 ? 1 : 0); current; current = finder.next()) {
            if (this.#genus(current) & genus) return current;
        }
        return 0;
    }

    // 중심 float 저장 뒤 원본 거의 올림을 적용하고 반경 덧셈은 DWORD 감김으로 보존한다. (2026-11-10 WB04-E253)
    #area(sid, radius) {
        const center = this.#footprint.center(sid);
        const x = truncate(center[0] + AREA_BIAS);
        const y = truncate(center[1] + AREA_BIAS);
        return { left: add(x, -radius), top: add(y, -radius), right: add(x, radius), bottom: add(y, radius) };
    }

    // 현재 다리 프레임의 코드 flags 바이트이다. 프레임이 표 밖이면 거부한다. (2026-11-10 WB04-E253)
    #frameFlags(sid) {
        const raw = this.#slot(sid);
        const frame = read(raw, FRAME_OFFSET);
        const codes = this.#frames[raw[TYPE_OFFSET]];
        if (frame >= codes.length) throw new RangeError('Damageable post 다리 프레임 오류');
        return codes[frame].flags & 0xff;
    }

    // 00421db0: 원본 WORD 쓰기 → 공간 통지 → 그 뒤 현재 TYPE·프레임 재조회 순서를 유지한다. (2026-11-10 WB04-E253)
    weaken(sid) {
        const word = read(this.#slot(sid), WORD_OFFSET, 2);
        const next = (word & ~LIFE_MASK) | ((((this.#state.bridgeCrackLife >>> 0) - 1) * 8) & LIFE_MASK);
        write(this.#pool.allocated(sid), WORD_OFFSET, next, 2);
        this.#hooks.notifySurface(sid);
        const raw = this.#slot(sid);
        const frame = bridgeCrackedFrame(this.#frames[raw[TYPE_OFFSET]], read(raw, FRAME_OFFSET) | 0, true);
        if (frame !== -1) {
            write(this.#pool.allocated(sid), FRAME_OFFSET, frame, 4);
            this.#hooks.displayUpdate(sid);
        }
    }

    // 0044b830: 수량 결정 뒤 매 반복 실제 하위 생성을 요청한다. 생성 뒤 부모/전역이 변해도 처음 계산한 반복 수는 다시 계산하지 않는다. (2026-11-10 WB04-E253)
    drops(sid, genus, random) {
        if (!sid) return;
        const raw = this.#slot(sid);
        if ((raw[EXTRA_OFFSET] & 1) || !(genus & DROP_MASK)) return;
        let count = 1;
        if (genus & DROP_FIVE) count = 5;
        if (genus & DROP_TWO) count = 2;
        const type = raw[TYPE_OFFSET];
        const state = this.#state;
        // 제외 TYPE 과 현재 특별 TYPE 을 원본 우선순서로 검사한다(각 비교 시점의 전체 DWORD).
        for (let index = 0; index < EXCLUDED_COUNT; index += 1) if (type === state.excludedDropType(index)) return;
        if (type === state.currentSpecialDropType) count = 3;
        if (state.currentDudeCount > DUDE_LIMIT) count = Math.floor(count / 2);
        // 처음 계산한 수만큼 요청한다.
        for (let index = 0; index < count; index += 1) this.#hooks.dropDude(sid, index, random);
    }

    // 0044b9e0: 첫 드롭/섬 정리 뒤 부모 상태를 현재 읽고 HP 피해 예약과 다리 순회를 수행한 뒤 공통 post 를 부른다. (2026-11-10 WB04-E253)
    postDestroy(sid, flags) {
        const hooks = this.#hooks;
        const state = this.#state;
        const hp = this.#hp;
        flags >>>= 0;
        this.#slot(sid);
        if (flags & DROP_FLAGS) this.drops(sid, this.#genus(sid), true);
        if (this.#ordinary(sid) && (this.#type(sid).flags1 & ISLAND_CLEANUP)) {
            const position = this.#position(sid);
            if (this.genusAt(position[0], position[1], RUBBLE) && !(this.#genus(sid) & NO_ISLAND_GENUS)
                && ((flags & COLLAPSE_FLAG) || this.#slot(sid)[TYPE_OFFSET] !== state.currentSilentSurfaceType)) {
                const current = this.#position(sid);
                hooks.dropIsland(current[0], current[1], flags, this.#slot(sid)[OWNER_OFFSET]);
            }
        }
        if (state.currentBattle && hp.hasHitPoints(sid) && this.#ordinary(sid)) {
            const attackerOwner = (flags >>> 16) & 15;
            if (!(flags & COLLAPSE_FLAG) && (this.#type(sid).group === 0 || this.#type(sid).group === 1 || (this.#genus(sid) & VORTEX))) {
                let damage = add(Math.trunc(hp.maxHitPoints(sid) / 2), 1);
                if (this.#genus(sid) & VORTEX) damage = 400;
                if (!(flags & NO_SPLASH_FLAG)) {
                    const radius = add(Math.trunc(this.#type(sid).footX / 2), 1);
                    const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
                    // 첫 후보부터 현재 일반 finder 의 표면 단계 생략 순서로 HP 대상을 처리한다.
                    for (let current = finder.begin(this.#area(sid, radius), 1); current; current = finder.next()) {
                        if (hp.hasHitPoints(current) && !(this.#slot(current)[STATE_OFFSET] & 2) && !(this.#slot(current)[EXTRA_OFFSET] & 1)) {
                            hooks.regular(current, state.currentDamageEvent, 0, (attackerOwner * 65536 + (damage >>> 0)) >>> 0);
                        }
                    }
                }
            }
            if (!(this.#genus(sid) & NO_BRIDGE_GENUS)) {
                const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
                // 순회가 저장한 next 를 쓰며 각 후보의 프레임/상태는 효과 뒤 새로 읽는다.
                for (let current = finder.begin(this.#area(sid, 2)); current; current = finder.next()) {
                    if (!(this.#genus(current) & BRIDGE)) continue;
                    if (!(this.#frameFlags(current) & CRACKED) || hooks.hasBridgeFall(current)) {
                        if (!(this.#frameFlags(current) & HARD)) this.weaken(current);
                    } else if (state.currentBoss) hooks.destroy(current, 0);
                }
            }
        }
        hooks.base(sid, flags);
    }
}
