// 신전 Ascend 의 반경별 적 자산 삭제 후보 판정(004db680)과 이벤트 0 본체(004db8e0), 외부 소유 섬 수(0046f480)를 옮긴 모듈이다(cpppj `o/RawVortexAscend.cpp/.h` 의 섬 배열 경로, 10.78 전용).
//  - candidate(sid, island, owner): 후보 TYPE 의 genus 가 제외 묶음(0x128b302)이면 거절, 현재 섬 번호가 다르면 거절, 후보 좌표(절삭)의 spot 비트 2 가 있으면 표면 0단계 지지 자산이 있고 현재 noIsland 타입이 아니어야 하며,
//    없으면 후보의 현재 TYPE flags1 의 0x200000 이 있어야 한다. 마지막으로 후보의 소유자가 owner 와 달라야 한다. 섬 조회 훅 뒤 spot/타입/소유자를 원본 순서로 다시 읽는다.
//  - tick(temple, count): 반경은 count(signed)이며 32 초과는 64 이다. 신전 섬 번호를 조회하고, 반경 finder 생성자처럼 신전 좌표에 float 0.99999 를 더해 절삭한 뒤 반경으로 사각형을 만들어 일반 finder 를 시작하고,
//    후보 버퍼를 확보한 뒤(실패는 첫 후보에서 거부) 최대 20개 후보를 모으며 20개가 되면 난수 소비 없이 중단한다. 후보가 있으면 게임 난수(limit=후보 수) 한 번으로 하나를 flags 0x100000 으로 삭제한다.
//    반환은 후보가 있었거나 반경이 64 가 아니면 true(최종 64 의 빈 반경이면 false)이다. 버퍼 해제는 예외에도 실행한다.
//  - countForeignOwnedIslands(owner): 존재하며 소유자가 0 도 owner 도 아닌 섬의 수이다.
// 섬 목록은 두 가지로 줄 수 있다: ① 배열 항목 { exists, owner }(원본 IslandList 의 존재 DWORD/+0xc 소유자; 기존 독립 입력 경로) ② 실제 월드의 RawIslandRuntime(cpppj 두 번째 생성자 대응) — 이 경우 복사 없이
// 실제 3204바이트 기록의 현재 count(signed)·존재·owner 를 반복마다 다시 읽고(삭제/owner 변경이 곧바로 보임) 다른 풀의 목록은 거부한다. 훅 { islandNumber(x,y)→int32, reserveCandidateBuffer()→bool, releaseCandidateBuffer(), destroy(sid, flags) } 는 모두 필수이다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E120: RawVortexAscend 이식으로 추가, 2026-11-13 WB04-E370: 실제 섬 런타임 직접 읽기 생성자 추가)
import { RawIslandRuntime } from './raw-island-runtime.js';
import { RawSquidFinder } from './raw-squid-finder.js';

// 원본 Ascend 에서 제외하는 표면/내용물/유닛 genus 묶음, 후보 버퍼의 DWORD 후보 수, 반경 finder 의 좌표 편향(float 0.99999)이다. (2026-11-08 WB04-E120)
const EXCLUDED_GENUS = 0x128b302;
const CANDIDATES = 20;
const RADIUS_BIAS = Math.fround(0.99999);
// 판본의 TYPE 수, 소유자 위치, 표시 지도 크기, spot 비트 2(지지 표면), 후속 후보의 flags1 비트이다. (2026-11-08 WB04-E120)
const TYPE_COUNT = 188;
const OWNER_AT = 34;
const SPOT_COUNT = 65536;
const SPOT_SUPPORT = 2;
const REQUIRES_FLAG1 = 0x200000;
const DESTROY_FLAGS = 0x100000;

// float32 를 읽기 위한 변환 버퍼이다. (2026-11-08 WB04-E120)
const floatView = new DataView(new ArrayBuffer(4));

// 원본 ftol 의 low DWORD 를 signed 로 읽는다. 실제 지도 밖 읽기는 별도 계약 위반으로 진단한다. (2026-11-08 WB04-E120)
function truncate(value) {
    if (!Number.isFinite(value) || value < -9223372036854775808 || value >= 9223372036854775808) throw new RangeError('Ascend 좌표 CRT 범위 오류');
    return Number(BigInt.asIntN(32, BigInt(Math.trunc(value))));
}

// signed 거리 더하기를 원본 DWORD 넘침 규칙으로 수행한다. (2026-11-08 WB04-E120)
function add(value, delta) { return (value + delta) | 0; }

// 0041d7a0 은 x87 double 합을 절삭한 뒤 좌표 float 로 저장한다. 보드의 정수 범위에서 그대로 반환한다. (2026-11-08 WB04-E120)
function radiusCoordinate(value) {
    const biased = value + RADIUS_BIAS;
    if (!Number.isFinite(biased) || biased < -2147483648 || biased >= 2147483648) throw new RangeError('Ascend 반경 좌표 범위 오류');
    return Math.trunc(biased);
}

// 신전 Ascend 판정 논리이다. 사용: new RawVortexAscend(pool, hash, spots, types, islands, random, state, hooks). (2026-11-08 WB04-E120)
export class RawVortexAscend {
    // 후보/부모를 조회하는 단일 raw 풀, 일반 반경 finder 와 표면 0단계가 함께 읽는 실제 해시, 발자국 spot 비트 지도(65536),
    // genus/flags1 이 저장된 공유 타입 표, 섬 존재/소유자 목록(독립 배열), 실제 월드의 섬 런타임(없으면 null), 삭제 후보 선택이 한 번 소비하는 게임 난수, 현재 noIsland 타입 전역, 외부 효과 훅이다.
    #pool;
    #hash;
    #spots;
    #types;
    #islands;
    #liveIslands = null;
    #random;
    #state;
    #hooks;

    // 필요한 효과와 표/지도의 판본 크기를 처음 구성할 때 확인한다. islands 는 { exists, owner } 배열이거나 같은 풀의 RawIslandRuntime 이다. state = { noIslandType, sharedNoIslandType?: { value } }.
    // (2026-11-08 WB04-E120, 2026-11-13 WB04-E370: RawIslandRuntime 을 직접 읽는 생성자 경로 추가)
    constructor(pool, hash, spots, types, islands, random, state, hooks) {
        if (pool?.edition !== 'patch1078' || !spots || spots.length !== SPOT_COUNT || !types || types.length !== TYPE_COUNT || !hooks
            || typeof hooks.islandNumber !== 'function' || typeof hooks.reserveCandidateBuffer !== 'function'
            || typeof hooks.releaseCandidateBuffer !== 'function' || typeof hooks.destroy !== 'function') throw new Error('Ascend 공유 지도/타입 표/하위 효과 누락');
        // 실제 섬 런타임은 복사 없이 같은 풀의 것만 받는다(배열 경로는 그대로 둔다).
        if (islands instanceof RawIslandRuntime && islands.pool !== pool) throw new TypeError('Ascend 섬 목록과 raw 풀 불일치');
        this.#pool = pool;
        this.#hash = hash;
        this.#spots = spots;
        this.#types = types;
        this.#islands = islands;
        this.#liveIslands = islands instanceof RawIslandRuntime ? islands : null;
        this.#random = random;
        this.#state = state;
        this.#hooks = hooks;
    }

    // Regular 어댑터가 같은 월드 풀에 연결돼 있는지 확인한다. (2026-11-08 WB04-E120)
    get pool() { return this.#pool; }

    // 현재 noIsland 타입 전역이다(후보마다 당시 현재 번호를 읽는다). (2026-11-08 WB04-E120)
    #noIslandType() {
        const shared = this.#state.sharedNoIslandType;
        return shared ? shared.value : this.#state.noIslandType;
    }

    // 현재 raw 좌표를 정렬과 무관하게 float 로 읽는다. (2026-11-08 WB04-E120)
    #coordinate(raw, offset) {
        floatView.setUint32(0, (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0, true);
        return floatView.getFloat32(0, true);
    }

    // 004db680: 현재 genus 와 섬을 확인한 뒤 현재 spot/타입/owner 를 원본 순서로 다시 읽는다. 사용: candidate(sid, island, owner) → 삭제 후보인지. (2026-11-08 WB04-E120)
    candidate(sid, island, owner) {
        const first = this.#pool.slot(sid);
        const type = first[10];
        if (type >= this.#types.length) throw new RangeError('Ascend 후보 타입 오류');
        const x = this.#coordinate(first, 14);
        const y = this.#coordinate(first, 18);
        if (this.#types[type].flags2 & EXCLUDED_GENUS) return false;
        if (this.#hooks.islandNumber(x, y) !== island) return false;
        const cellX = truncate(x);
        const cellY = truncate(y);
        if (cellX < 0 || cellX >= 256 || cellY < 0 || cellY >= 256) throw new RangeError('Ascend spot 지도 범위 오류');
        const index = cellY * 256 + cellX;
        if (this.#spots[index] & SPOT_SUPPORT) {
            const support = this.#hash.cell(0, cellX, cellY);
            if (!support || this.#pool.slot(support)[10] === this.#noIslandType()) return false;
        } else {
            const currentType = this.#pool.slot(sid)[10];
            if (currentType >= this.#types.length) throw new RangeError('Ascend 후속 후보 타입 오류');
            if (!(this.#types[currentType].flags1 & REQUIRES_FLAG1)) return false;
        }
        return this.#pool.slot(sid)[OWNER_AT] !== owner;
    }

    // 004db8e0: finder 최초 결과를 얻은 뒤 버퍼를 확보하며 첫 20개까지만 후보를 모으고 다음 1회 난수로 삭제한다. 사용: tick(temple, count) → 계속 진행할지. (2026-11-08 WB04-E120)
    tick(temple, count) {
        let radius = count | 0;
        if (radius > 32) radius = 64;
        const raw = this.#pool.slot(temple);
        const templeX = this.#coordinate(raw, 14);
        const templeY = this.#coordinate(raw, 18);
        const island = this.#hooks.islandNumber(templeX, templeY);
        // 반경 finder 생성자는 두 좌표에 0.99999f 편향을 더해 절삭한 뒤 부호 있는 반경을 더하고 뺀다.
        // 섬 조회 뒤 신전 좌표를 다시 읽는다.
        const x = radiusCoordinate(this.#coordinate(this.#pool.slot(temple), 14));
        const y = radiusCoordinate(this.#coordinate(this.#pool.slot(temple), 18));
        const negativeRadius = (0 - radius) | 0;
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
        let sid = finder.begin({ left: add(x, negativeRadius), top: add(y, negativeRadius), right: add(x, radius), bottom: add(y, radius) });
        const allocated = this.#hooks.reserveCandidateBuffer();
        const candidates = [];
        try {
            // 현재 후보 owner 는 매번 현재 신전 raw 에서 읽으며 20개가 되면 Next 없이 중단한다.
            while (sid) {
                if (this.candidate(sid, island, this.#pool.slot(temple)[OWNER_AT])) {
                    if (!allocated) throw new Error('Ascend 후보 버퍼 확보 실패');
                    candidates.push(sid);
                    if (candidates.length === CANDIDATES) break;
                }
                sid = finder.next();
            }
            if (candidates.length) this.#hooks.destroy(candidates[this.#random.next(candidates.length)], DESTROY_FLAGS);
        } catch (error) {
            this.#hooks.releaseCandidateBuffer();
            throw error;
        }
        this.#hooks.releaseCandidateBuffer();
        return candidates.length !== 0 || radius !== 64;
    }

    // 0046f480: 원본은 목록 길이를 현재 반복마다 다시 읽으며 존재/nonzero owner/다른 owner 의 항목만 센다. 사용: countForeignOwnedIslands(owner).
    // 실제 섬 런타임이면 반복마다 signed count 와 현재 기록을 다시 읽고 owner 를 두 번 조회한다(원본 호출 횟수 보존). (2026-11-08 WB04-E120, 2026-11-13 WB04-E370: 실제 목록 경로 추가)
    countForeignOwnedIslands(owner) {
        const live = this.#liveIslands;
        if (live) {
            let currentCount = 0;
            // 원본 두 판본처럼 반복마다 signed count 와 현재 32바이트 기록을 조회한다.
            for (let index = 0; index < (live.count | 0); index += 1) {
                if (live.exists(index) && live.owner(index) !== (owner >>> 0) && live.owner(index) !== 0) currentCount += 1;
            }
            return currentCount;
        }
        let count = 0;
        // 공유 IslandList 항목의 존재/소유자를 읽는다. 빈 번호는 개수에 더하지 않는다.
        for (const island of this.#islands) if (island.exists && island.owner && island.owner !== (owner >>> 0)) count += 1;
        return count;
    }
}
