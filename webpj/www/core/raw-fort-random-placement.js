// Fort 후처리의 단일 랜덤 배치(AddRandom 한 번)를 현재 raw 공간·지역·생성/Pop 에 연결한 모듈이다(cpppj `o/RawFortRandomPlacement.cpp/.h`, 원본 0040db90, 10.78 전용, 기준선 뒤 현재 TYPE 전역 접근자 변경 반영).
//  - place(localRandom, request)(0040db90): request = { type, x, y, region, mode, force }. 반올림 spot(x+0.5, y+0.5의 CRT 절삭)이 256×256 안인지 확인하고 spot 지도 하위 6비트(0x3f)가 0 이면 0 반환 → 같은 좌표에 edgeFarm 타입 객체가 이미 있으면 0 →
//    공간 검사(mayPlace) 거절이면 0 → force 가 아니면 청크의 지역 번호(signed BYTE)가 요청 지역과 다를 때 0 → 요청 TYPE 이 지면 프레임(0x200000)이면 지면 객체(isle genus)를 찾아(없으면 assert 거부)
//    mode 가 켜져 있으면 모서리 검사(NW→NE→SE→SW 단락 평가)에 실패할 때 1(새 SID 가 아님)을 반환하고, 통과하면 Unpop(0) → Retype(edgeFarm) → HP → Repop(64) 로 그 지면 객체를 교체해 지면 SID 를 돌려준다.
//    mode 가 꺼져 있으면 지면의 현재 프레임을 가져온다. 임의 프레임(0x100000)이면 고정 P/P·wildcard 후보(최대 256)에서 전역 난수 1회로 고른다. 프레임 −1(0xffffffff)은 assert 거부 →
//    factory 생성(type, 0) → 프레임 직접 쓰기(가상 SetFrame 없음) → HP → owner(force 면 0, 아니면 지역 owner, ownerOverride 가 있으면 그 값) → Pop(flags 0) → 새 SID.
//    localRandom 인자는 원본 단일 몸체에서 쓰이지 않는다(프레임은 전역 난수를 소비한다).
//  - mayReplace(x, y): 실제 isle WORD 지도/타입 검사와 네 모서리의 단락 평가(`!Island(대각) && Island(위/아래) && Island(좌/우)` 이면 교체 불가).
//  - retype(sid, type): 새 객체가 void 상태이고 두 TYPE 의 foot 이 같을 때만 raw 타입 BYTE 를 바꾸고 현재 생성자만 재실행한다(postCreate 는 부르지 않음).
// 현재 지면/edgeFarm TYPE 전역은 사용 위치마다 다시 읽는다(Native 는 실제 전역의 같은 주소를 빌림). 모든 자료와 경계는 같은 월드이며 이 객체보다 오래 살아야 한다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-11 WB04-E285: 4-B Fort 단일 랜덤 배치 이식)
import { RawSquidFinder } from './raw-squid-finder.js';

// 원본 raw 슬롯의 타입/상태/HP/프레임 위치와 지도 크기·특별 플래그·void 상태, 지면 genus(isle) 비트이다. (2026-11-11 WB04-E285)
const TYPE = 10;
const STATE = 11;
const HP = 26;
const FRAME = 36;
const WORLD = 256;
const TERRAIN_FRAME = 0x200000;
const RANDOM_FRAME = 0x100000;
const SPOT_MASK = 0x3f;
const VOID = 4;
const ISLAND_GENUS = 2;
const CANDIDATES = 256;
const SIDE_P = 80;
const VARIANT_A = 65;
const TYPE_COUNT = 188;
const SPOT_BYTES = 65536;

// 현재 지면(isle)·edgeFarm TYPE 번호와 편집 소유자 전역이다. 값 필드는 독립 입력의 기본값이고, shared* 함수가 있으면 호출 때마다 그 실제 전역을 읽는다. (2026-11-11 WB04-E285)
// 사용: const state = new FortRandomPlacementState(); state.ownerOverride = 3; state.sharedEdgeFarmType = () => 121. 원본 초기값은 isle 97, edgeFarm 121 이다.
export class FortRandomPlacementState {
    islandType = 97;
    edgeFarmType = 121;
    // 00544ce0: 0 이면 지역 owner/force 0 규칙을 유지한다.
    ownerOverride = 0;
    // 실제 TYPE 전역을 빌리는 선택형 함수이다.
    sharedIslandType = null;
    sharedEdgeFarmType = null;

    // 모서리 조회 위치마다 현재 지면 TYPE 을 읽는다. (2026-11-11 WB04-E285)
    currentIslandType() { return (this.sharedIslandType ? this.sharedIslandType() : this.islandType) >>> 0; }

    // 앞선 효과 뒤 Retype 에서도 현재 edgeFarm TYPE 을 다시 읽는다. (2026-11-11 WB04-E285)
    currentEdgeFarmType() { return (this.sharedEdgeFarmType ? this.sharedEdgeFarmType() : this.edgeFarmType) >>> 0; }
}

// x87 CRT 절삭은 하위 DWORD 를 쓴다. 원본이 주소를 벗어나는 비정상 입력(비유한/±9.2e18 이상)은 거부한다. (2026-11-11 WB04-E285)
function ftol(value) {
    if (!Number.isFinite(value) || Math.abs(value) >= 9.2e18) throw new RangeError('Fort 배치 CRT 좌표 범위 오류');
    return Math.trunc(value) | 0;
}

// 판본 좁은 필드를 읽는다. (2026-11-11 WB04-E285)
function readField(raw, offset, width) {
    let result = 0;
    // 지정 폭만 little endian 으로 조합한다.
    for (let byte = 0; byte < width; byte += 1) result |= raw[offset + byte] << (8 * byte);
    return result >>> 0;
}

// 지정한 폭만 원본 raw 슬롯에 기록한다(옆 payload 보존). (2026-11-11 WB04-E285)
function writeField(raw, offset, value, width) {
    for (let byte = 0; byte < width; byte += 1) raw[offset + byte] = (value >>> (8 * byte)) & 255;
}

// 사용: new RawFortRandomPlacement(pool, hash, spots, types, frames, globalRandom, state, hooks).place(local, { type, x, y, region, mode, force }) → SID/0/1.
// hooks = { mayPlace(query{type,argument,x,y,flags,owner,mode})→bool, chunkRegion(cx,cy)→signed BYTE, regionOwner(region)→owner, create(type,flags)→SID, construct(sid,type), hasHitPoints(sid)→bool,
//          typeHitPoints(type)→HP, setOwner(sid,owner), unpop(sid,flags), repop(sid,flags), pop(sid,x,y,flags) } 모두 필수이다. (2026-11-11 WB04-E285)
export class RawFortRandomPlacement {
    // 자료/모든 필수 효과·실제 hash0 을 검사한 뒤 빌린 현재 저장소를 유지한다. spots 는 65536BYTE 지도, hash 는 SquidHash(레벨 0 이 65536칸)이다.
    constructor(pool, hash, spots, types, frames, globalRandom, state, hooks) {
        const required = ['mayPlace', 'chunkRegion', 'regionOwner', 'create', 'construct', 'hasHitPoints', 'typeHitPoints', 'setOwner', 'unpop', 'repop', 'pop'];
        if (pool?.edition !== 'patch1078' || types?.length !== TYPE_COUNT || frames?.length !== types.length || spots?.length !== SPOT_BYTES
            || hash?.entries(0).length !== SPOT_BYTES || !globalRandom || typeof globalRandom.next !== 'function' || !state
            || required.some((name) => typeof hooks?.[name] !== 'function')) throw new TypeError('Fort 단일 배치 판본/자료/실제 효과 오류');
        this.pool = pool;
        this.hash = hash;
        this.spots = spots;
        this.types = types;
        this.frames = frames;
        this.random = globalRandom;
        this.state = state;
        this.hooks = hooks;
    }

    // 요청/현재 raw 타입을 실제 자료 범위 안에서 읽는다. (2026-11-11 WB04-E285)
    type(number) {
        if (!(number >= 0 && number < this.types.length)) throw new RangeError('Fort 단일 배치 타입 범위 오류');
        return this.types[number];
    }

    // 현재 일반 hash/finder 의 exact type/genus 검색이다. 초기 spot 의 반올림 좌표와 구별한다. 못 찾으면 0. (2026-11-11 WB04-E285)
    find(x, y, value, genus) {
        const ix = ftol(x);
        const iy = ftol(y);
        const finder = new RawSquidFinder(this.pool, this.hash, this.types);
        const flags = !genus && !(this.type(value).flags2 & 6) ? 1 : 0; // exact type 의 surface/bridge 부류가 아니면 hash0 을 건너뛴다.
        // 원본 finder 의 void/footprint/후행 next 규칙과 처음 통과 순서를 유지한다.
        for (let sid = finder.begin({ left: ix, top: iy, right: ix, bottom: iy }, flags); sid; sid = finder.next()) {
            const type = this.pool.slot(sid)[TYPE];
            if (genus ? (this.type(type).flags2 & value) !== 0 : type === value) return sid;
        }
        return 0;
    }

    // WORD 표면 번호/type 만 검사한다. 게임 상태 필터를 새로 넣지 않는다. (2026-11-11 WB04-E285)
    island(x, y) {
        if (x < 0 || y < 0 || x >= WORLD || y >= WORLD) return false;
        const sid = this.hash.view(0)[y * WORLD + x];
        return sid !== 0 && this.pool.slot(sid)[TYPE] === this.state.currentIslandType();
    }

    // NW→NE→SE→SW 의 원본 단락 평가를 유지한다. 좌표가 보드 밖이면 거부한다. (2026-11-11 WB04-E285)
    mayReplace(x, y) {
        if (x < 0 || y < 0 || x >= WORLD || y >= WORLD) throw new RangeError('Fort 교체 좌표 범위 오류');
        if (!this.island(x - 1, y - 1) && this.island(x, y - 1) && this.island(x - 1, y)) return false;
        if (!this.island(x + 1, y - 1) && this.island(x, y - 1) && this.island(x + 1, y)) return false;
        if (!this.island(x + 1, y + 1) && this.island(x, y + 1) && this.island(x + 1, y)) return false;
        if (!this.island(x - 1, y + 1) && this.island(x, y + 1) && this.island(x - 1, y)) return false;
        return true;
    }

    // 원본 Retype 은 void 와 두 foot 동일성만 검사하고 현재 생성자만 재실행한다(004b1620). (2026-11-11 WB04-E285)
    retype(sid, number) {
        const raw = this.pool.allocated(sid);
        const before = this.type(raw[TYPE]);
        const after = this.type(number);
        if (!(raw[STATE] & VOID) || before.footX !== after.footX || before.footY !== after.footY) throw new Error('Fort Retype 원본 void/동일 foot assert');
        raw[TYPE] = number;
        this.hooks.construct(sid, raw[TYPE]);
    }

    // 최초 프레임은 가상 SetFrame 없이 패치 DWORD 로 직접 쓴다. (2026-11-11 WB04-E285)
    #frame(sid, value) {
        writeField(this.pool.allocated(sid), FRAME, value, 4);
    }

    // HasHitPoints 호출 뒤 현재 타입 HP 를 읽고 공통 HP 필드에 직접 쓴다. (2026-11-11 WB04-E285)
    #hitPoints(sid) {
        if (this.hooks.hasHitPoints(sid)) {
            const hp = this.hooks.typeHitPoints(this.pool.slot(sid)[TYPE]);
            writeField(this.pool.allocated(sid), HP, hp >>> 0, 4);
        }
    }

    // 실제 고정 P/P fallback·wildcard·후보 최대 256 과 global PRNG 한 번 소비이다. 후보가 없으면 −1. (2026-11-11 WB04-E285)
    #randomFrame(type) {
        this.type(type);
        const codes = this.frames[type].codes;
        const candidates = [];
        // P/P → 변환 P/P(P 방향 mask 0) → P/A → P/임의 변형의 첫 비어 있지 않은 집합이다.
        for (let pass = 0; pass < 4 && candidates.length === 0; pass += 1) {
            // 앞 256개의 일치 인덱스만 저장하며 나머지 코드도 원본처럼 계속 읽는다.
            for (let index = 0; index < codes.length; index += 1) {
                if (codes[index].side === SIDE_P && (pass === 3 || codes[index].variant === (pass === 2 ? VARIANT_A : SIDE_P)) && candidates.length < CANDIDATES) candidates.push(index);
            }
        }
        return candidates.length ? candidates[this.random.next(candidates.length)] : -1;
    }

    // 실제 단일 몸체의 조기 반환/지면 교체/새 객체 효과를 순서대로 실행한다. localRandom 은 원본처럼 쓰이지 않는다. 반환 1 은 모서리 거부이며 새 SID 로 해석하면 안 된다. (2026-11-11 WB04-E285)
    place(localRandom, request) {
        const hooks = this.hooks;
        const sx = ftol(request.x + 0.5);
        const sy = ftol(request.y + 0.5);
        if (sx < 0 || sy < 0 || sx >= WORLD || sy >= WORLD) throw new RangeError('Fort 반올림 spot 주소 범위 오류');
        if (!(this.spots[sy * WORLD + sx] & SPOT_MASK)) return 0;
        if (this.find(request.x, request.y, this.state.currentEdgeFarmType(), false)) return 0;
        const query = { type: request.type, argument: 0, x: request.x, y: request.y, flags: 0, owner: 0, mode: 0 };
        if (!hooks.mayPlace(query)) return 0;
        if (!request.force && (hooks.chunkRegion(Math.trunc(ftol(request.x) / 16), Math.trunc(ftol(request.y) / 16)) | 0) !== (request.region | 0)) return 0;
        const requested = this.type(request.type);
        let frame = 0;
        if (requested.flags1 & TERRAIN_FRAME) {
            const ground = this.find(request.x, request.y, ISLAND_GENUS, true);
            if (!ground) throw new Error('Fort terrain-frame 원본 지면 assert');
            if (request.mode) {
                if (!this.mayReplace(ftol(request.x), ftol(request.y))) return 1;
                hooks.unpop(ground, 0);
                this.retype(ground, this.state.currentEdgeFarmType());
                this.#hitPoints(ground);
                hooks.repop(ground, 64);
                return ground;
            }
            frame = readField(this.pool.slot(ground), FRAME, 4);
        }
        if (requested.flags1 & RANDOM_FRAME) frame = this.#randomFrame(request.type) >>> 0;
        if (frame === 0xffffffff) throw new Error('Fort 단일 배치 프레임 -1 원본 assert');
        const sid = hooks.create(request.type, 0);
        this.#frame(sid, frame);
        this.#hitPoints(sid);
        let owner = request.force ? 0 : hooks.regionOwner(request.region) >>> 0;
        if (this.state.ownerOverride) owner = this.state.ownerOverride >>> 0;
        hooks.setOwner(sid, owner);
        hooks.pop(sid, request.x, request.y, 0);
        return sid;
    }
}

// 같은 pool 의 완전한 배치 pipeline/factory/타입 HP 를 연결한다. 가상 공간·지역·owner 는 필수로 남긴다(hooks 로 받은 나머지를 보존). 사용: makeFortRandomPlacementHooks(pool, pipeline, factory, reward, { chunkRegion, regionOwner, setOwner, unpop, repop, pop }). (2026-11-11 WB04-E285)
export function makeFortRandomPlacementHooks(pool, placement, factory, reward, hooks) {
    if (pool !== placement.pool || pool !== factory.pool || pool !== reward.pool) throw new TypeError('Fort placement 어댑터 pool 불일치');
    return {
        ...hooks,
        mayPlace: (query) => placement.mayPlace(query),
        create: (type, flags) => factory.create(type, flags),
        construct: (sid, type) => factory.construct(type, sid),
        hasHitPoints: (sid) => reward.hasHitPoints(sid),
        typeHitPoints: (type) => reward.typeHitPoints(type),
    };
}

// FinishOwner 의 필수 단일 placement 를 실제 몸체에 연결한다: local PRNG 를 그대로 place 에 전달한다. 사용: makeFortSinglePlacementHook(pool, placement, finishHooks). (2026-11-11 WB04-E285)
export function makeFortSinglePlacementHook(pool, placement, hooks) {
    if (pool !== placement.pool) throw new TypeError('Fort finish/단일 placement pool 불일치');
    return { ...hooks, place: (random, request) => placement.place(random, request) };
}
