// 사제 배치의 후보 무시·로컬 발자국 표시·충돌 거부를 실제 사각형 탐색기에 연결한 모듈이다(cpppj `o/RawPriestPlacementCollision.cpp/.h`, 10.78). E153 RawCanonPlacementCollision 의 사제용 어댑터이다.
//  - 사제와 일반 타입은 같은 클라이언트/무시 전역(createCanonPlacementCollisionState)과 같은 무시 함수(placementIgnoreValue, 0049ade0)를 공유한다.
//  - new RawPriestPlacementCollision(pool, hash, types, state, preview, hooks): hooks = { inspectRegions(priestQuery, scan), inspectCandidateTerrain(priestQuery, sid) } 둘 다 필수이다.
//    inspectRegions 는 모양 순회·지형/지역/관계 경계이며 모양마다 scan(area)로 충돌 검사를 요청한다(scan 이 거짓이면 즉시 중단). 충돌을 무시한 후보도 inspectCandidateTerrain 을 호출한 뒤 finder 의 미리 저장한 next 로 진행한다.
//  - inspectCandidate(query, localOwner, sid): 매 후보마다 현재 사제 genus(0x200000)·유한 좌표 계약을 확인하고 공통 무시/표시/거부 본문을 쓴다(첫 인자는 타입 번호). inspectArea(query, localOwner, area): 실제 finder 의 begin/next 를 쓰고
//    첫 거부 뒤에는 후속 후보를 읽지 않는다. inspect(query, localOwner): 지역 경계가 거부 이후 scan 을 다시 요청하거나 성공을 덮어쓰지 못하도록 거부 상태를 유지한다.
//  - makePriestPlacementCollisionHooks(pool, collision, hooks): 미리보기의 inspectCollisions 만 실제 후보 검사/탐색기로 연결한다. 모양/지형 경계는 호출자가 제공한다.
// 10.72(CD)의 bridge 비트/전역 마스크/extra 위치 차이는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E157: 사제 배치 충돌 이식으로 추가)
import { RawSquidFinder } from './raw-squid-finder.js';
import { RawCanonPlacementCollision } from './raw-canon-placement-collision.js';

// 사제 genus 비트와 타입 표 크기이다. (2026-11-09 WB04-E157)
const GENUS_PRIEST = 0x200000;
const TYPE_COUNT = 188;

// 공통 요청에서 사제 API 의 원래 여섯 값 인자를 복구한다. (2026-11-09 WB04-E157)
function priestQuery(query) {
    return { type: query.type, x: query.x, y: query.y, flags: query.flags, owner: query.owner, mode: query.mode };
}

// 사제 배치 충돌 계층이다. 실제 탐색기는 같은 풀/해시/현재 타입 표로 이 객체 안에서 구성한다. (2026-11-09 WB04-E157)
export class RawPriestPlacementCollision {
    #pool;
    #hash;
    #types;
    #hooks;
    #collision;

    // 기존 필수 경계를 진단한 뒤 일반 충돌 계산기에 사제 콜백을 공급한다. 미리보기 상태는 직접 공유해 후보 검사 이전의 표시를 유지한다. (2026-11-09 WB04-E157)
    constructor(pool, hash, types, state, preview, hooks) {
        if (typeof hooks?.inspectRegions !== 'function' || typeof hooks.inspectCandidateTerrain !== 'function') throw new TypeError('사제 충돌 지역 경계 누락');
        this.#hooks = { inspectRegions: hooks.inspectRegions, inspectCandidateTerrain: hooks.inspectCandidateTerrain };
        const { inspectRegions, inspectCandidateTerrain } = this.#hooks;
        this.#collision = new RawCanonPlacementCollision(pool, hash, types, state, preview, {
            inspectRegions: (query, scan) => inspectRegions(priestQuery(query), scan),
            inspectCandidateTerrain: (query, sid) => { inspectCandidateTerrain(priestQuery(query), sid); }
        });
        if (!Array.isArray(types) || types.length !== TYPE_COUNT) throw new TypeError('사제 충돌 자료/지역 경계 누락');
        this.#pool = pool;
        this.#hash = hash;
        this.#types = types;
    }

    // 미리보기와 같은 풀/판본인지 연결 시 확인할 수 있도록 원래 풀을 제공한다. (2026-11-09 WB04-E157)
    get pool() { return this.#pool; }

    // 사제 전용 경로를 일반 자산 전체 배치 판정으로 오해하지 않도록 진입 계약(사제 genus·유한 좌표)을 확인한다. (2026-11-09 WB04-E157)
    #placing(query) {
        if (!Number.isInteger(query.type) || query.type < 0 || query.type >= this.#types.length || (this.#types[query.type].flags2 & GENUS_PRIEST) === 0 || !Number.isFinite(query.x) || !Number.isFinite(query.y)) {
            throw new TypeError('사제 충돌 요청 타입/좌표 오류');
        }
    }

    // 0049b9c4..0049bb1a: 매 후보의 현재 사제 genus 계약을 확인하고 공통 무시/표시/거부 본문을 사용한다. (2026-11-09 WB04-E157)
    inspectCandidate(query, localOwner, candidate) {
        this.#placing(query);
        return this.#collision.inspectCandidate({ type: query.type, argument: query.type, x: query.x, y: query.y, flags: query.flags, owner: query.owner, mode: query.mode }, localOwner, candidate);
    }

    // 일반 finder 의 기본 가상 필터와 flag 0 을 사용하므로 buried 만 finder 내부에서 제외한다. 현재 후보가 거부하면 Next 를 호출하기 전에 종료한다. (2026-11-09 WB04-E157)
    inspectArea(query, localOwner, area) {
        this.#placing(query);
        const finder = new RawSquidFinder(this.#pool, this.#hash, this.#types);
        for (let sid = finder.begin(area); sid !== 0; sid = finder.next()) {
            if (!this.inspectCandidate(query, localOwner, sid)) return false;
            this.#hooks.inspectCandidateTerrain({ ...query }, sid);
        }
        return true;
    }

    // 지역 경계가 거부 이후 scan 을 다시 요청하거나 성공을 덮어쓰지 못하도록 거부 상태를 유지한다. 요청은 값으로 복사한다. (2026-11-09 WB04-E157)
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

// 미리보기의 inspectCollisions 만 실제 후보 검사/탐색기로 연결한다. 기존 미리보기의 모양 범위 조회는 그대로 둔다. (2026-11-09 WB04-E157)
export function makePriestPlacementCollisionHooks(pool, collision, hooks = {}) {
    if (pool !== collision.pool) throw new TypeError('사제 충돌/미리보기의 SID 풀이 다릅니다');
    return { ...hooks, inspectCollisions: (query, localOwner) => collision.inspect(query, localOwner) };
}
