// 일반 타입 배치의 후보 무시·로컬 발자국 표시·충돌 거부를 실제 사각형 탐색기에 연결한 모듈이다(cpppj `o/RawCanonPlacementCollision.cpp/.h`, 10.78).
//  - createCanonPlacementCollisionState(): { clientMode(00540bc0, 예측 가능한 클라이언트 SID 를 충돌에서 제외), ignoredGenus(005325b4, 배치 타입과 겹칠 때 후보 마스크를 반환하는 전역) } 를 원본 초기값(0)으로 만든다.
//  - placementIgnoreValue(placing, candidate, mode, ignoredGenus): 0049ade0 의 임의 타입 무시 함수이다. 반환은 비트값(DWORD)이며 bool 로 축소하지 않는다. 조기 분기 순서는 ① mode 0 이고 배치 genus 0x20000 → 1 ② 후보 genus 0x02000000 → 1
//    ③ 0x120000 → 1 ④ 배치가 다리(4)이고 후보 0x200000 → 1 ⑤ 후보 섬(2)이고 배치 타입 flags1 에 표면(0x800)이 없음 → 1 ⑥ 후보 0x30006 이고 배치 0x208100 → 1 ⑦ 전역이 배치 genus 와 겹치면 후보 & 0x218000 ⑧ mode 0 이고 배치 0x30000 → 1 ⑨ 아니면 0.
//  - new RawCanonPlacementCollision(pool, hash, types, state, preview, hooks): hooks = { inspectRegions(query, scan), inspectCandidateTerrain(query, sid) } 둘 다 필수이다. inspectRegions 는 모양 순회/지형/지역/관계 경계이며 모양마다 scan(area)로 충돌 검사를 요청하고
//    scan 이 거짓이면 즉시 중단해야 한다. inspectCandidateTerrain 은 충돌을 무시한 후보도 호출한 뒤 finder 의 미리 저장한 next 로 진행한다.
//  - inspectCandidate(query, localOwner, sid): 0049b9c4..0049bb1a 한 후보 — 현재 raw 타입을 먼저 읽고 클라이언트 SID 조건(예측 가능한 SID 5..14999)·무시 함수·로컬 발자국 표시(미리보기 배열 [dx×12+dy+13]=1, 후보 아래쪽 y 바깥·x 안쪽 순서, 넓은 차를 CRT 처럼 절삭)·
//    extra 의 비트 0 과 mode 0 순서로 거부(거짓)를 정한다. inspectArea(query, localOwner, area): 실제 finder 의 begin/next 를 쓰고 첫 거부 뒤에는 후속 후보를 읽지 않는다.
//    inspect(query, localOwner): 지역 경계가 거부 이후 scan 을 다시 요청하거나 성공을 덮어쓰지 못하도록 거부 상태를 유지한다.
//  - makeCanonPlacementCollisionHooks(pool, collision, hooks): 미리보기의 inspectCollisions 만 연결한다. makeCanonGeometryCollisionHooks(pool, geometry, regions, hooks): 요청의 소유자/모드를 캡처한 지역 정책(regions(query))을 만들고 E146 기하의 패턴 순회/finder 범위를 연결한다.
// 10.72(CD)의 bridge 비트/전역 마스크/extra 위치 차이는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E153: 일반 배치 충돌 이식으로 추가)
import { RawSquidFinder } from './raw-squid-finder.js';

// 슬롯 필드 위치(10.78): 타입(+10)·x(+14)·y(+18)·extra(+40), 예측 가능한 SID 범위의 시작(5)과 서버 영역 시작(15000), 미리보기 한 변(12)과 내부 시작 오프셋(13), 배열 크기, 타입 표 크기이다. (2026-11-09 WB04-E153)
const TYPE = 10;
const X = 14;
const Y = 18;
const EXTRA = 40;
const FIRST_CLIENT = 5;
const FIRST_SERVER = 15000;
const SIDE = 12;
const INTERIOR = 13;
const BLOCKED_SIZE = 144;
const TYPE_COUNT = 188;

// 충돌 계층이 읽는 전역을 원본 초기값으로 만든다. (2026-11-09 WB04-E153)
export function createCanonPlacementCollisionState() {
    return { clientMode: 0, ignoredGenus: 0 };
}

// 슬롯의 float 좌표를 비정렬 리틀엔디언 비트로 읽는다. (2026-11-09 WB04-E153)
function floatAt(raw, offset) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0, true);
    return view.getFloat32(0, true);
}

// 조기 분기의 순서와 10.78 의 bridge/전역 마스크를 그대로 보존한다. 반환은 비트값(DWORD)이다. (2026-11-09 WB04-E153)
export function placementIgnoreValue(placing, candidate, mode, ignoredGenus) {
    const own = placing.flags2 >>> 0;
    const other = candidate.flags2 >>> 0;
    if (mode === 0 && (own & 0x20000) !== 0) return 1;
    if ((other & 0x02000000) !== 0) return 1;
    if ((other & 0x120000) !== 0) return 1;
    if ((own & 4) !== 0 && (other & 0x200000) !== 0) return 1;
    if ((other & 2) !== 0 && (placing.flags1 & 0x800) === 0) return 1;
    if ((other & 0x30006) !== 0 && (own & 0x208100) !== 0) return 1;
    if (((ignoredGenus >>> 0) & own) !== 0) return (other & 0x218000) >>> 0;
    if (mode === 0 && (own & 0x30000) !== 0) return 1;
    return 0;
}

// 일반 타입 배치 충돌 계층이다. 실제 탐색기는 같은 풀/해시/현재 타입 표로 이 객체 안에서 만든다. 미리보기 상태는 직접 공유해 후보 검사 이전의 표시를 유지한다. (2026-11-09 WB04-E153)
export class RawCanonPlacementCollision {
    #pool;
    #hash;
    #types;
    #state;
    #preview;
    #hooks;

    // 타입 표 크기와 필수 지역 경계를 확인한다. (2026-11-09 WB04-E153)
    constructor(pool, hash, types, state, preview, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('일반 타입 충돌은 10.78 SidPool 만 받습니다.');
        if (!Array.isArray(types) || types.length !== TYPE_COUNT || !hash || !state || !(preview?.blocked instanceof Uint8Array) || preview.blocked.length !== BLOCKED_SIZE
            || typeof hooks?.inspectRegions !== 'function' || typeof hooks.inspectCandidateTerrain !== 'function') {
            throw new TypeError('일반 타입 충돌 자료/지역 경계 누락');
        }
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
        this.#state = state;
        this.#preview = preview;
        this.#hooks = { inspectRegions: hooks.inspectRegions, inspectCandidateTerrain: hooks.inspectCandidateTerrain };
    }

    // 미리보기와 같은 풀인지 연결 시 확인할 수 있도록 원래 풀을 제공한다. (2026-11-09 WB04-E153)
    get pool() { return this.#pool; }

    // 일반 타입은 genus 를 제한하지 않으며 타입 번호와 비유한 좌표를 진단한다. (2026-11-09 WB04-E153)
    #placing(query) {
        if (!Number.isInteger(query.type) || query.type < 0 || query.type >= this.#types.length || !Number.isFinite(query.x) || !Number.isFinite(query.y)) throw new TypeError('일반 타입 충돌 요청 타입/좌표 오류');
        return this.#types[query.type];
    }

    // 현재 raw 타입을 먼저 읽고 클라이언트 SID 조건, 현재 전역 무시 조건, 표시, extra/mode 순서로 검사한다. 거짓이면 충돌 거부이다. (2026-11-09 WB04-E153)
    inspectCandidate(query, localOwner, candidate) {
        const placing = this.#placing(query);
        const raw = this.#pool.slot(candidate);
        const type = raw[TYPE];
        if (type >= this.#types.length) throw new RangeError('일반 타입 충돌 후보 타입 오류');
        const other = this.#types[type];
        const x = floatAt(raw, X);
        const y = floatAt(raw, Y);
        const state = this.#state;
        if (state.clientMode && candidate >= FIRST_CLIENT && candidate < FIRST_SERVER) return true;
        if (placementIgnoreValue(placing, other, query.mode >>> 0, state.ignoredGenus) !== 0) return true;
        if (localOwner) {
            if (!Number.isFinite(x) || !Number.isFinite(y) || other.footX < 0 || other.footY < 0 || other.footX > 256 || other.footY > 256) throw new RangeError('일반 타입 충돌 후보 발자국/좌표 오류');
            const blocked = this.#preview.blocked;
            // 후보의 아래쪽부터 y 를 늘리고 각 행에서 x 를 늘린다. float 저장 없이 넓은 차를 CRT 처럼 절삭한다.
            for (let cy = 0; cy < other.footY; cy += 1) {
                // 빈 가로 축은 원본처럼 좌표 계산/표시를 수행하지 않는다.
                for (let cx = 0; cx < other.footX; cx += 1) {
                    const dx = Math.trunc(Math.fround(query.x) - (x - cx));
                    const dy = Math.trunc(Math.fround(query.y) - (y - cy));
                    if (dx < 0 || dy < 0 || dx >= placing.footX || dy >= placing.footY) continue;
                    const index = dx * SIDE + dy + INTERIOR;
                    if (index >= blocked.length) throw new RangeError('일반 타입 충돌 미리보기 배열 오류');
                    blocked[index] = 1;
                }
            }
        }
        return (raw[EXTRA] & 1) !== 0 && (query.mode >>> 0) === 0;
    }

    // 일반 finder 의 기본 가상 필터와 flag 0 을 사용하므로 buried 만 finder 내부에서 제외한다. 현재 후보가 거부하면 Next 를 호출하기 전에 종료한다. (2026-11-09 WB04-E153)
    inspectArea(query, localOwner, area) {
        this.#placing(query);
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
        for (let sid = finder.begin(area); sid !== 0; sid = finder.next()) {
            if (!this.inspectCandidate(query, localOwner, sid)) return false;
            this.#hooks.inspectCandidateTerrain({ ...query }, sid);
        }
        return true;
    }

    // 지역 경계가 거부 이후 scan 을 다시 요청하거나 성공을 덮어쓰지 못하도록 거부 상태를 유지한다. (2026-11-09 WB04-E153)
    inspect(query, localOwner) {
        query = { ...query };
        this.#placing(query);
        let rejected = false;
        const allowed = this.#hooks.inspectRegions({ ...query }, (area) => {
            if (rejected) return false;
            rejected = !this.inspectArea(query, localOwner, area);
            return !rejected;
        });
        return !!allowed && !rejected;
    }
}

// 미리보기의 inspectCollisions 만 실제 후보 검사/탐색기로 연결한다. 모양/지형 경계는 호출자가 제공한다. (2026-11-09 WB04-E153)
export function makeCanonPlacementCollisionHooks(pool, collision, hooks = {}) {
    if (pool !== collision.pool) throw new TypeError('일반 타입 충돌/미리보기의 SID 풀이 다릅니다');
    return { ...hooks, inspectCollisions: (query, localOwner) => collision.inspect(query, localOwner) };
}

// 정책은 검사마다 원래 요청으로 만들며 거부 뒤의 모양/최종 효과는 실제 기하 모듈이 생략한다. 요청의 소유자/모드를 캡처한 지역 정책과 실제 패턴 순회/finder 범위를 연결한다. (2026-11-09 WB04-E153)
export function makeCanonGeometryCollisionHooks(pool, geometry, regions, hooks = {}) {
    if (pool !== geometry.pool || typeof regions !== 'function') throw new TypeError('일반 모양/충돌의 SID 풀 또는 지역 정책 오류');
    return {
        ...hooks,
        inspectRegions: (query, scan) => {
            const policy = regions({ ...query });
            return geometry.inspect({ type: query.type, argument: query.argument | 0, direction: query.flags | 0, x: query.x, y: query.y, explicitFrame: false }, (cell) => scan(cell.area), policy);
        }
    };
}
