// 실제 edgeFarm 군집의 지역 시드·archipelago·중심/주변 순회와 기존 단일 배치를 같은 월드에서 실행하는 모듈이다(cpppj `o/RawFarmCluster.cpp/.h`, 원본 0040e560 + 지역 record 시드 helper, 10.78 전용, 기준선 뒤 현재 edgeFarm TYPE 접근자 변경 반영).
//  - place(area, region, quantity)(0040e560): 지역 record 의 시드(청크를 y 바깥/x 안쪽으로 훑어 처음 만나는 island 번호가 region 인 청크의 record BYTE+3 × 77)로 별도 local 난수를 만들고, 목표 = 수량 × 20 성공이다.
//    archipelago 이고 지역 0 이면 사각형을 섬 확장 단위(span×16)만큼 넓히고 목표를 (목표/9, signed 나눗셈) × (span×2+2)² 로 바꾸며 force = 1, 반지름 2 이다. 높이→폭 순서의 local 난수로 중심 좌표를 고르고
//    (limit < 1 이면 난수를 소비하지 않고 0) 단일 배치(mode 1, 현재 edgeFarm TYPE)를 시도한다. 성공하면 남은 수량을 줄이고 실패 한도(30000)를 재설정한 뒤 중심 주변을 x 바깥/y 안쪽으로 (중심 포함) 시도하며 성공할 때만 줄이고 0 이면 멈춘다.
//    최초 중심은 수량이 0 이어도 시도한다. 실패 한도를 다 쓰면 끝낸다. 요청값은 snapshot 이지만 edgeFarm TYPE 은 각 시도에서 현재 값을 읽는다.
//  - 지역 record 시드는 현재 지역 owner 가 아니라 청크의 원래 Territory record 를 읽는다. CD 전용 진단(존재 DWORD/record 누락)은 10.78 에는 없다.
// state = { archipelago, archipelagoSpan, sharedArchipelago?(()→DWORD), sharedArchipelagoSpan?(()→signed) } 이며 호출 때마다 현재 값을 읽는다. 사용: new RawFarmCluster(pool, chunks, islands, placementState, state, { place }).place({left,top,right,bottom}, region, quantity).
// chunks = { xLength, yLength, at(x,y) → { island(BYTE), record[6] } }(ChunkMap 은 partial 이라 최소 계약만 쓴다). 이 모듈은 DOM 을 쓰지 않는다. (2026-11-11 WB04-E285: 4-B Fort 군집 배치 이식, 목록 순서를 앞당겨 단일 배치와 함께 대조)
import { GameRandom } from './random.js';

// 원본 시드 배율/수량 배율과 중심 실패 한도이다. (2026-11-11 WB04-E285)
const SEED_FACTOR = 77;
const QUANTITY_FACTOR = 20;
const FAILURE_LIMIT = 30000;

// 군집 진입의 현재 archipelago/섬 확장 단위 전역이다. shared* 함수가 있으면 호출마다 그 실제 전역(Login.bb4 와 다른 bb0, 군도 root 와 같은 00542654)을 읽는다. (2026-11-11 WB04-E285)
export class FarmClusterState {
    archipelago = false;
    archipelagoSpan = 1;
    sharedArchipelago = null;
    sharedArchipelagoSpan = null;

    // bool 의 물리 폭으로 대체하지 않고 현재 DWORD 가 0 이 아닌지로 판정한다. (2026-11-11 WB04-E285)
    isArchipelago() { return this.sharedArchipelago ? (this.sharedArchipelago() >>> 0) !== 0 : Boolean(this.archipelago); }

    // caller 가 값을 저장해도 현재 owner 의 같은 DWORD 를 읽도록 매번 조회한다. (2026-11-11 WB04-E285)
    currentSpan() { return (this.sharedArchipelagoSpan ? this.sharedArchipelagoSpan() : this.archipelagoSpan) | 0; }
}

// 두 판본의 local 난수 helper 가 쓰는 CRT 64비트 절삭의 하위 DWORD(signed)이다: 비유한/±2^63 이상은 0. (2026-11-11 WB04-E285)
function truncate(value) {
    if (!Number.isFinite(value) || Math.abs(value) >= 9223372036854775808) return 0;
    return Math.trunc(value) | 0;
}

// limit 이 1 보다 작으면 원본 localRandom 을 소비하지 않고 0 을 반환한다. (2026-11-11 WB04-E285)
function next(random, limit) {
    return limit < 1 ? 0 : random.next(limit >>> 0);
}

// 단일 float32 값(JS 수)이다. 사각형/좌표 계산 결과를 원본처럼 float 로 저장한다. (2026-11-11 WB04-E285)
const f32 = Math.fround;

// 사용: hooks = { place(random, request)→비0 성공 }. 자료와 필수 효과가 서로 다른 월드에 연결되지 않게 구성 때 검사한다. (2026-11-11 WB04-E285)
export class RawFarmCluster {
    constructor(pool, chunks, islands, placementState, state, hooks) {
        if (!pool || pool !== islands?.pool || typeof hooks?.place !== 'function' || !chunks || typeof chunks.at !== 'function' || !placementState || !state) throw new TypeError('FarmCluster 같은 월드/필수 효과 누락');
        this.pool = pool;
        this.chunks = chunks;
        this.islands = islands;
        this.placementState = placementState;
        this.state = state;
        this.hooks = hooks;
    }

    // 현재 청크를 y 바깥/x 안쪽으로 찾은 첫 지역 record 의 BYTE+3 을 원본 시드로 읽는다. 없으면 0. (2026-11-11 WB04-E285)
    #regionSeed(region) {
        // 원본 선형 검색은 x 가 먼저 증가하고 끝에서 y 가 증가한다.
        for (let y = 0; y < this.chunks.yLength; y += 1) {
            for (let x = 0; x < this.chunks.xLength; x += 1) {
                const entry = this.chunks.at(x, y);
                if (((entry.island << 24) >> 24) === region) return Math.imul(entry.record[3], SEED_FACTOR) >>> 0;
            }
        }
        return 0;
    }

    // 수량 × 20 성공을 목표로 중심과 주변을 배치한다. area 는 float 사각형, region 은 signed, quantity 는 DWORD 이다. (2026-11-11 WB04-E285)
    place(area, region, quantity) {
        const random = new GameRandom(this.#regionSeed(region));
        let remaining = Math.imul(quantity >>> 0, QUANTITY_FACTOR) >>> 0;
        let force = 0;
        let radius = 1;
        const rect = { left: area.left, top: area.top, right: area.right, bottom: area.bottom };
        if (this.state.isArchipelago() && region === 0) {
            const span = this.state.currentSpan() >>> 0;
            const extension = Math.imul(span, 16) >>> 0;
            const start = (extension - 16) | 0;
            const end = extension | 0;
            rect.left = f32(rect.left - start);
            rect.top = f32(rect.top - start);
            rect.right = f32(rect.right + end);
            rect.bottom = f32(rect.bottom + end);
            const width = (Math.imul(span, 2) + 2) >>> 0;
            remaining = Math.imul(Math.imul(Math.trunc((remaining | 0) / 9) >>> 0, width), width) >>> 0;
            force = 1;
            radius = 2;
        }
        const height = truncate(rect.bottom - rect.top);
        const width = truncate(rect.right - rect.left);
        let failures = FAILURE_LIMIT;
        // 원본은 최초 중심을 항상 시도하며 성공 중심마다 실패 한도를 재설정한다.
        do {
            const dy = next(random, height);
            const dx = next(random, width);
            const x = f32(dx + rect.left);
            const y = f32(dy + rect.top);
            if (this.hooks.place(random, { type: this.placementState.currentEdgeFarmType(), x, y, region, mode: 1, force })) {
                remaining = (remaining - 1) >>> 0;
                failures = FAILURE_LIMIT;
                // 원본 주변 순회는 x 바깥/y 안쪽이며 중심 좌표도 다시 시도한다.
                for (let ox = -radius; ox <= radius && remaining; ox += 1) {
                    // 성공할 때만 remaining 을 감소시키고 0 이면 남은 주변 시도를 멈춘다.
                    for (let oy = -radius; oy <= radius && remaining; oy += 1) {
                        const nx = f32(x + ox);
                        const ny = f32(y + oy);
                        if (this.hooks.place(random, { type: this.placementState.currentEdgeFarmType(), x: nx, y: ny, region, mode: 1, force })) remaining = (remaining - 1) >>> 0;
                    }
                }
            }
            if (!remaining) break;
            failures -= 1;
        } while (failures);
    }
}

// 같은 pool 의 기존 RawFortRandomPlacement.place 를 필수 군집 효과에 연결한다. 사용: makeFarmClusterPlacementHook(pool, placement, hooks). (2026-11-11 WB04-E285)
export function makeFarmClusterPlacementHook(pool, placement, hooks) {
    if (pool !== placement.pool) throw new TypeError('FarmCluster 단일배치 pool 불일치');
    return { ...hooks, place: (random, request) => placement.place(random, request) };
}
