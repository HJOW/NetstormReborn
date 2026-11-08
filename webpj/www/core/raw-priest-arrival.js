// 사제 실제 +3c(도착)의 중립 회복/내용물 부착·삭제와 공통 Carrier 도착 위임을 옮긴 모듈이다(cpppj `o/RawPriestArrival.cpp/.h`, 10.78: 패치 00491f70).
//  - createPriestArrivalState(): { boss(00540bc4, 기본 참), loading(005c89b8), suppressNeutralReturn(00594fa4) } — 권한/로딩/중립 재배치 억제는 하위 효과 뒤에도 매번 다시 읽는다.
//  - new RawPriestArrival(pool, priest, hitPoints, terrain, islands, footprint, create, carrier, state, hooks): 모든 참조는 같은 월드(풀)의 기존 모듈이다 — priest(RawPriestState: 현재 signed HP), hitPoints(SquidReward: 최대 HP),
//    terrain(RawCanonPlacementTerrain: 현재 지면 WORD/지역 getter), islands(RawIslandRuntime: 섬 Temple), footprint(RawSquidNeighbors: float 발자국), create(RawContainedCreate: 내용물 부착), carrier(RawCarrierArrival: 일반 부모 도착).
//    hooks = { destroy(sid, flags)(실제 사제 가상 +10 의 flags 0 자기 삭제), neutralReturn(sid)(00491db0 속도/귀환/임의 위치 이동 몸체) } 둘 다 필수이며 빈 효과로 완료 처리하지 않는다.
//  - arrival(sid, targetA, targetB): 00491f70 전체 부모 wrapper 이다. 권한이 있고 현재 HP 가 최대 HP 미만인 중립(loading/억제 아님) 사제가 현재 지면의 섬 Temple 근처(발자국 거리 1.0 이하)이면서 Temple 의 소유자가 같으면
//    Temple 에 같은 타입/WORD 의 내용물을 만들고(mask 32) 자기 삭제한 뒤 1001 을 돌려준다. 아니고 권한이 있는 중립 사제의 HP 가 최대 HP 의 절반 이상이면 귀환 몸체를 실행하고 1001 이다. 그 밖은 일반 Carrier 도착으로 위임한다.
//  - templeAt(x, y): 0046f950 — 현재 plain 좌표의 지역(sentinel 127 이면 Temple 0)→현재 섬 Temple DWORD 를 읽는다. 풀 용량 밖이면 거부한다.
// 10.72(CD)의 float 거리 저장 차이는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E162: 사제 도착 이식으로 추가)

// 사제 자기 삭제/회피 상태(1001), 내용물 부착 mask, 사제 지역 없음 sentinel, 원본 float 1.0 거리 경계이다. (2026-11-09 WB04-E162)
const REPLACED = 1001;
const CAPTURE_MASK = 32;
const PRIEST_NO_ISLAND = 127;
const NEAR_LIMIT = 1;
const OWNER = 34;

// 사제 도착 처리가 읽는 전역을 원본 초기값으로 만든다. (2026-11-09 WB04-E162)
export function createPriestArrivalState() { return { boss: true, loading: false, suppressNeutralReturn: false }; }

// 원본 비정렬 raw 값을 지정 폭으로 읽는다(지정 필드 밖의 stale 바이트는 섞지 않는다). (2026-11-09 WB04-E162)
function read(raw, offset, width) {
    let value = 0;
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value;
}

// DWORD 비트를 단정도 float 로 해석한다. (2026-11-09 WB04-E162)
function floatAt(raw, offset) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, read(raw, offset, 4) >>> 0, true);
    return view.getFloat32(0, true);
}

// 현재 plain 좌표를 원본 0 방향 절삭의 정상 signed32 범위에서 정수로 바꾼다. (2026-11-09 WB04-E162)
function coordinate(value) {
    if (!Number.isFinite(value) || value < -2147483648 || value >= 2147483648) throw new RangeError('사제 도착 현재 좌표 범위');
    return Math.trunc(value);
}

// 사제 도착 wrapper 이다. 참조된 월드/metadata/상태/하위 효과는 이 객체보다 오래 살아야 한다. (2026-11-09 WB04-E162)
export class RawPriestArrival {
    #pool;
    #priest;
    #hitPoints;
    #terrain;
    #islands;
    #footprint;
    #create;
    #carrier;
    #state;
    #hooks;

    // 최초 조회 전에 실제 같은 월드/필수 가상 경계를 확인한다. (2026-11-09 WB04-E162)
    constructor(pool, priest, hitPoints, terrain, islands, footprint, create, carrier, state, hooks) {
        if (pool?.edition !== 'patch1078') throw new TypeError('사제 도착은 10.78 SidPool 만 받습니다.');
        if (priest?.pool !== pool || hitPoints?.pool !== pool || terrain?.pool !== pool || islands?.pool !== pool || footprint?.pool !== pool || create?.pool !== pool || carrier?.pool !== pool
            || !state || typeof hooks?.destroy !== 'function' || typeof hooks.neutralReturn !== 'function') {
            throw new TypeError('사제 도착 실제 월드/하위 효과 연결 누락');
        }
        this.#pool = pool;
        this.#priest = priest;
        this.#hitPoints = hitPoints;
        this.#terrain = terrain;
        this.#islands = islands;
        this.#footprint = footprint;
        this.#create = create;
        this.#carrier = carrier;
        this.#state = state;
        this.#hooks = { destroy: hooks.destroy, neutralReturn: hooks.neutralReturn };
    }

    // 실제 부모 path 가 같은 풀의 사제 wrapper 인지 검증할 때 사용하도록 원래 풀을 제공한다. (2026-11-09 WB04-E162)
    get pool() { return this.#pool; }

    // 현재 raw float 두 개와 owner BYTE 만 읽는다(효과 뒤 다시 읽는다). (2026-11-09 WB04-E162)
    #position(sid) { const raw = this.#pool.slot(sid); return { x: floatAt(raw, 14), y: floatAt(raw, 18) }; }
    #owner(sid) { return this.#pool.slot(sid)[OWNER]; }

    // 실제 loading 부터 검사하며 패치 wrapper 도 같은 세 입력(loading·중립 owner 0·억제)을 읽는다. (2026-11-09 WB04-E162)
    #neutralEligible(sid) { return !this.#state.loading && this.#owner(sid) === 0 && !this.#state.suppressNeutralReturn; }

    // 0046f950: 현재 지면이 없으면 sentinel 127 → Temple 0 이며 실제 섬 getter 를 재사용한다. (2026-11-09 WB04-E162)
    templeAt(x, y) {
        const island = this.#terrain.regionAt(coordinate(x), coordinate(y));
        if (island === PRIEST_NO_ISLAND) return 0;
        const temple = this.#islands.temple(island | 0);
        if (temple >= this.#pool.capacity) throw new RangeError('사제 도착 현재 섬 Temple SID');
        return temple;
    }

    // 0041d840: 원본 거리 helper 는 발자국 안에서도 음수 거리를 그대로 유지하고 Euclid 길이를 쓰지 않는다(축별 signed 거리의 큰 쪽과 1.0 비교). (2026-11-09 WB04-E162)
    #nearTemple(sid, temple) {
        const point = this.#position(sid);
        const bounds = this.#footprint.footprint(temple);
        const x = Math.fround(point.x < bounds.left ? bounds.left - point.x : point.x - bounds.right);
        const y = point.y < bounds.top ? bounds.top - point.y : point.y - bounds.bottom;
        return (y <= x ? x : y) <= NEAR_LIMIT;
    }

    // 저 HP 중립의 현재 섬 복귀/내용물 생성→자기 삭제, 절반 이상 중립 귀환, 일반 Carrier 위임 순서이다. (2026-11-09 WB04-E162)
    arrival(sid, targetA, targetB) {
        const state = this.#state;
        if (state.boss) {
            if (this.#priest.currentHitPoints(sid) < this.#hitPoints.maxHitPoints(sid) && this.#neutralEligible(sid)) {
                const point = this.#position(sid);
                const temple = this.templeAt(point.x, point.y);
                if (temple && this.#owner(temple) === this.#owner(sid) && this.#nearTemple(sid, temple)) {
                    const raw = this.#pool.slot(sid);
                    this.#create.create(temple, raw[10], read(raw, 12, 2), CAPTURE_MASK, 0, 0);
                    this.#hooks.destroy(sid, 0);
                    return REPLACED;
                }
            }
        }
        if (state.boss && this.#neutralEligible(sid) && this.#priest.currentHitPoints(sid) >= Math.trunc(this.#hitPoints.maxHitPoints(sid) / 2)) {
            this.#hooks.neutralReturn(sid);
            return REPLACED;
        }
        return this.#carrier.arrival(sid, targetA, targetB);
    }
}
