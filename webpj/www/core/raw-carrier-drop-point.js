// 운반물(Carrier)이 내용물을 내려놓을 위치(426a70)와 발자국 탐색 helper(49bfe0), 좌표 snap(41d7a0)을 옮긴 모듈이다(cpppj `client/RawCarrierDropPoint.cpp/.h`, 10.78 전용).
// 좌표 선택·반복 순서·유효성은 이 모듈이 계산하고, 전체 배치 검사(0049b510)와 실제 발자국 중심(004adea0)만 외부 훅이다.
//  - find(carrier, type): 운반물 슬롯의 현재 좌표(+14/+18 float)를 snap 한 점을 만든다. TYPE flags2 에 0x100 이 있으면 외부 중심(center)을 snap 해 바로 반환한다(배치 검사를 부르지 않음).
//    아니면 현재 점이 소유자(+34 BYTE)로 허용되는지(mayPlace) 검사해 허용이면 그 점을 그대로 반환한다(유효성 재검사 없음). 아니면 짝수 방향 네 개(0·2·4·6)를 먼저, 홀수 방향 네 개(1·3·5·7)를 이어
//    기준점에 방향 오프셋을 더한 점으로 findFootprint 를 부르고, 반환점이 두 축 모두 0 초과·현재 지도 한계 미만이면 채택한다. 어떤 방향도 맞지 않으면 현재 실패점을 반환한다.
//    소유자는 각 findFootprint 호출 직전에 슬롯에서 다시 읽는다(배치 검사가 슬롯을 바꾸면 이후 방향에 반영).
//  - findFootprint(type, argument, x, y, flags, owner): 대상 TYPE 의 현재 footX/footY 를 x 바깥/y 안쪽 순서로 순회하며 mayPlace({type, argument, x+열, y+행, flags, owner, mode 0})가 처음 허용하는 점을 반환한다.
//    반복 조건마다 TYPE 의 footX/footY 를 다시 읽고(검사가 바꾸면 반영), 실패한 열을 건너뛰지 않으며, 끝까지 없으면 현재 실패점을 반환한다.
//  - RawCarrierDropPoint.snap(point): 각 좌표에 실제 단정도 bias(0.99999f)를 53비트 x87 정밀도(JS 배정도)로 더하고 CRT signed64 절삭의 하위 DWORD 를 signed32 로 해석해 float 에 저장한다.
//    비유한·signed64 범위 밖 값은 CRT 불확정 정수 0 이다.
// state = { mapLimit(signed32, 현재 지도 한계), invalidPoint([x, y], 발자국/주변 탐색 실패 때 그대로 복사하는 현재 전역) }, types[type] = { flags2, footX(signed32), footY(signed32) } 이며 모두 호출마다 현재 값을 읽는다.
// hooks = { mayPlace({ type, argument, x, y, flags, owner, mode })→bool, center(sid)→[x, y] } 이며 도달한 경로에서 없으면 오류이다. 판본은 PATCH(10.78)만 받는다. 이 모듈은 DOM 을 쓰지 않는다.
// (2026-11-08 WB04-E95: 운반물 내려놓기 위치 이식으로 추가. 실제 운반물 메뉴(RawCarrierMenu)는 아직 이식 전이라 메뉴 연결(BindCarrierMenuDropPoint)과 전체 배치 파이프라인 훅 조립(MakeCarrierDropPointHooks)은 해당 단위에서 한다)

// 단정도 snap bias(0.99999f)와 8방향 오프셋 표(북부터 시계 방향), 슬롯의 TYPE 비교용 flags2 중심 비트와 좌표·소유자 위치이다. (2026-11-08 WB04-E95)
const SNAP_BIAS = Math.fround(0.99999);
const NEIGHBOR_CELLS = Object.freeze([[0, -1], [1, -1], [1, 0], [1, 1], [0, 1], [-1, 1], [-1, 0], [-1, -1]].map((cell) => Object.freeze(cell)));
const CENTER_FLAG = 0x100;
const X_AT = 14;
const Y_AT = 18;
const OWNER_AT = 34;
const fround = Math.fround;

// 원본 ftol 처럼 signed64 로 절삭한 하위 DWORD 를 signed32 로 해석해 float 에 저장한다. 비유한·범위 밖은 0 이다. (2026-11-08 WB04-E95)
function snapCoordinate(coordinate) {
    const value = coordinate + SNAP_BIAS;
    let integer = 0n;
    if (Number.isFinite(value) && value >= -(2 ** 63) && value < 2 ** 63) integer = BigInt(Math.trunc(value));
    return fround(Number(BigInt.asIntN(32, integer)));
}

// 좌표에 signed32 오프셋을 더한 뒤 한 번만 단정도로 저장한다. (2026-11-08 WB04-E95)
const offsetBy = (coordinate, offset) => fround(coordinate + offset);

// 실제 도달한 필수 훅 누락을 조용한 실패점으로 바꾸지 않는다. (2026-11-08 WB04-E95)
function required(hook) {
    if (typeof hook !== 'function') throw new Error('운반물 위치의 실제 경계 미연결');
    return hook;
}

// 운반물 내려놓기 위치 계산기이다. (2026-11-08 WB04-E95)
export class RawCarrierDropPoint {
    // 운반물 슬롯(+14/+18 좌표·+34 소유자)을 읽는 실제 SID 풀이다. (2026-11-08 WB04-E95)
    #pool;
    // 현재 flags2/footX/footY 를 제공하는 같은 TYPE 표이다. (2026-11-08 WB04-E95)
    #types;
    // 현재 지도 한계와 실패점 전역이다. (2026-11-08 WB04-E95)
    #state;
    // 실제 배치 검사와 발자국 중심의 외부 연결이다. (2026-11-08 WB04-E95)
    #hooks;

    // 같은 실제 풀·TYPE 표·현재 전역과 외부 훅을 받는다. 모든 참조는 이 객체보다 오래 살아야 한다. PATCH 전용이다. (2026-11-08 WB04-E95)
    constructor(pool, types, state, hooks = {}) {
        if (pool?.edition !== 'patch1078' || !types || types.length === 0) throw new Error('운반물 위치 Pool/TYPE 판본 오류');
        this.#pool = pool;
        this.#types = types;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 같은 실제 세션의 풀을 돌려준다(메뉴/배치 검사 연결 여부 확인용). (2026-11-08 WB04-E95)
    get pool() { return this.#pool; }

    // 각 좌표를 snap 한 새 점이다. (2026-11-08 WB04-E95)
    static snap(point) { return [snapCoordinate(point[0]), snapCoordinate(point[1])]; }

    // 타입 범위 오류를 호스트 메모리 접근으로 진행하지 않는다. (2026-11-08 WB04-E95)
    #type(type) {
        if (!Number.isInteger(type) || type < 0 || type >= this.#types.length) throw new RangeError('운반물 위치 TYPE');
        return this.#types[type];
    }

    // 소유자는 BYTE 를 부호 확장하지 않는다. 주변 탐색 진입마다 현재 슬롯에서 다시 읽는다. (2026-11-08 WB04-E95)
    #owner(carrier) { return this.#pool.slot(carrier)[OWNER_AT]; }

    // 후보의 두 축이 모두 0 초과이며 현재 지도 한계(signed32 → float) 미만인 경우만 채택한다(NaN 은 거부). (2026-11-08 WB04-E95)
    #valid(point) {
        const limit = fround(this.#state.mapLimit | 0);
        return 0 < point[0] && point[0] < limit && 0 < point[1] && point[1] < limit;
    }

    // 현재 실패점 전역의 복사본이다. (2026-11-08 WB04-E95)
    #invalid() { return [this.#state.invalidPoint[0], this.#state.invalidPoint[1]]; }

    // 발자국 탐색: 외부 배치 판정 뒤 바뀐 현재 footX/footY 를 반복 조건에서 다시 읽는다. (2026-11-08 WB04-E95)
    findFootprint(type, argument, x, y, flags, owner) {
        const record = this.#type(type);
        // 각 열의 첫 행부터 검사하며 실패한 열을 건너뛰지 않는다.
        for (let column = 0; column < (record.footX | 0); column += 1) {
            // 원본 helper 는 소유자 인자를 이 호출 동안 유지하고 mode 는 항상 0 으로 전달한다.
            for (let row = 0; row < (record.footY | 0); row += 1) {
                const candidateX = offsetBy(x, column);
                const candidateY = offsetBy(y, row);
                if (required(this.#hooks.mayPlace)({ type, argument, x: candidateX, y: candidateY, flags: flags >>> 0, owner: owner >>> 0, mode: 0 })) {
                    return [offsetBy(x, column), offsetBy(y, row)];
                }
            }
        }
        return this.#invalid();
    }

    // 위치 선택: 현재 점 즉시 허용 → 짝수 방향 → 홀수 방향 순서이다. 즉시 허용점의 유효성은 다시 검사하지 않는다. (2026-11-08 WB04-E95)
    find(carrier, type) {
        const raw = this.#pool.slot(carrier);
        const view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
        const point = RawCarrierDropPoint.snap([view.getFloat32(X_AT, true), view.getFloat32(Y_AT, true)]);
        const record = this.#type(type);
        if ((record.flags2 >>> 0) & CENTER_FLAG) return RawCarrierDropPoint.snap(required(this.#hooks.center)(carrier));
        if (required(this.#hooks.mayPlace)({ type, argument: 0, x: point[0], y: point[1], flags: 0, owner: this.#owner(carrier), mode: 0 })) return point;
        // 짝수 방향 네 개를 먼저 검사하고 홀수 방향 네 개를 이어 검사한다.
        for (let phase = 0; phase < 2; phase += 1) {
            // 각 방향의 helper 호출 직전에 현재 소유자를 읽고 같은 위치 기준점을 유지한다.
            for (let index = 0; index < 4; index += 1) {
                const [dx, dy] = NEIGHBOR_CELLS[phase + index * 2];
                const candidate = this.findFootprint(type, 0, offsetBy(point[0], dx), offsetBy(point[1], dy), 0, this.#owner(carrier));
                if (this.#valid(candidate)) return candidate;
            }
        }
        return this.#invalid();
    }
}
