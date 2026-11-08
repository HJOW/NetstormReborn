// 네 다리 전략의 실제 연결 판정·무작위 시작점·빈 사각형·재시도 기한을 옮긴 모듈이다(cpppj `o/RawAiBridgePlanning.cpp/.h`, 10.78: 패치 00410d40 연결·00411310 재시도 기한·00411470 무작위 시작점·004119b0 빈 사각형).
//  - new RawAiBridgePlanning(candidates, random, now, graphFor): candidates 는 E176 `RawAiBridgeCandidates`(같은 실제 본체/풀/TYPE/코드/지도 getter 의 유일한 소유 모듈), random 은 원본 공용 `GameRandom`(00532710),
//    now() 는 기한을 쓸 때 읽는 현재 0055b4d0 QWORD 시각, graphFor(sid) 는 같은 현재 00462bc0 이며 필요한 연결 판정에서만 호출한다(E57 `RawPlayerPlacementAnchor.graphFor`).
//  - connected(token, geyser): 00410d40 — 현재 목표 간헐천 graph 가 본체 +0x298 과 같을 때만(graph 하위 뒤 본체와 실제 목표 좌표를 다시 읽음) 좌표(0.9999 보정 없이 ftol)의 가장자리 12칸에서 자기 소유 J(북/남 3칸씩)/K(서/동 3칸씩) 다리가 있는지 확인한다.
//  - delayRetry(token): 00411310 — 현재 +0x350 count 전체 DWORD 가 0 이면 40초, 그 밖은 120초를 현재 QWORD 시각에 더해 +0x5d0 에 쓴다(다른 본문 바이트 보존).
//  - randomStart(token, bounds): 00411470 — 같은 graph/owner(본체 +0x298/+0x260)로 고정하고 경계를 bottom → top → right → left 순서로 절삭한 뒤 최대 20번 y → x 공용 난수를 소비해(상한 제외) 같은 graph/owner 의 첫 열린 지면을 선택한다. 없으면 `{ -10000, -10000 }`.
//  - rectangleFree(left, top, right, bottom): 004119b0 — inclusive 사각형의 flat spot BYTE 가 모두 0 인지 X 바깥/Y 안쪽 순서로 검사한다(반전 폭은 빈 사각형이며 물리 지도 밖은 spot getter 가 거부).
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E176: AI 다리 계획 이식으로 추가)

// 원본 연결 판정 방향 글자(J 북/남, K 서/동)와 재시도 기한(초), 미정 좌표, 시도 횟수이다. (2026-11-09 WB04-E176)
const SIDE_J = 'J'.charCodeAt(0);
const SIDE_K = 'K'.charCodeAt(0);
const EMPTY_DELAY = 40;
const FULL_DELAY = 120;
const UNSET = -10000;
const ATTEMPTS = 20;
const TWO_63 = 9223372036854775808;

// 원본 DWORD 의 signed 해석이다. (2026-11-09 WB04-E176)
const signed = (value) => value | 0;

// 실제 __ftol 의 signed64 하위 DWORD(uint32)이며 indefinite 하위는 0 이다. (2026-11-09 WB04-E176)
function truncate(value) {
    if (!Number.isFinite(value) || value < -TWO_63 || value >= TWO_63) return 0;
    return Number(BigInt.asUintN(32, BigInt(Math.trunc(value))));
}

// 단정도 float 비트를 수로 바꾼다. (2026-11-09 WB04-E176)
function bitsFloat(bits) {
    const view = new DataView(new ArrayBuffer(4));
    view.setUint32(0, bits >>> 0, true);
    return view.getFloat32(0, true);
}

// 실제 슬롯의 비정렬 DWORD 만 읽는다. (2026-11-09 WB04-E176)
function read(raw, at) {
    if (at < 0 || at > raw.length || raw.length - at < 4) throw new RangeError('AI 다리 계획 DWORD 범위');
    return (raw[at] | (raw[at + 1] << 8) | (raw[at + 2] << 16) | (raw[at + 3] << 24)) >>> 0;
}

// 다리 전략 계획 처리기이다. 생성 시 지도/본체/난수 효과는 없다. (2026-11-09 WB04-E176)
export class RawAiBridgePlanning {
    #candidates;
    #random;
    #now;
    #graphFor;

    // 후보 모듈/난수/현재 시각 조회기를 빌린다(graphFor 는 필요한 연결 판정에서만 요구한다). (2026-11-09 WB04-E176)
    constructor(candidates, random, now, graphFor) {
        if (!candidates || typeof candidates.field !== 'function' || typeof random?.next !== 'function' || typeof now !== 'function') throw new TypeError('AI 다리 계획 후보/난수/현재 시각 누락');
        this.#candidates = candidates;
        this.#random = random;
        this.#now = now;
        this.#graphFor = graphFor;
    }

    // 원본은 free/dead 필터나 graph 재비교를 가장자리 판정에 추가하지 않는다(현재 map 좌표 0..255 와 전체 bridge/owner/현재 첫 코드만 판정). (2026-11-09 WB04-E176)
    #edge(token, x, y, side) {
        const candidates = this.#candidates;
        if (x >= 256 || y >= 256) return false;
        const surface = candidates.surfaces[y * 256 + x];
        if (!surface) return false;
        const raw = candidates.pool.slot(surface);
        const type = raw[10];
        if (type !== candidates.bridgeType || raw[34] !== candidates.field(token, 0x260)) return false;
        const frame = read(raw, 36);
        if (type < 70 || type >= candidates.frames.length || frame >= candidates.frames[type].codes.length) throw new RangeError('AI 다리 연결 현재 코드 범위');
        return candidates.frames[type].codes[frame].side === side;
    }

    // graph 하위 뒤 현재 본체와 실제 목표 좌표를 다시 읽으며 J12/K12 로 범위를 넓히지 않는다. (2026-11-09 WB04-E176)
    connected(token, geyser) {
        const candidates = this.#candidates;
        candidates.body(token);
        if (typeof this.#graphFor !== 'function') throw new Error('AI 다리 연결 현재 graph 미연결');
        const graph = this.#graphFor(geyser);
        if (graph !== candidates.field(token, 0x298)) return false;
        const raw = candidates.pool.slot(geyser);
        const x = truncate(bitsFloat(read(raw, 14)));
        const y = truncate(bitsFloat(read(raw, 18)));
        // x-2..x 에서 북/남의 J 를 이 순서로 검사한다.
        for (let offset = 0; offset < 3; offset += 1) {
            if (this.#edge(token, (x - 2 + offset) >>> 0, (y - 3) >>> 0, SIDE_J) || this.#edge(token, (x - 2 + offset) >>> 0, (y + 1) >>> 0, SIDE_J)) return true;
        }
        // y-2..y 에서 서/동의 K 를 이 순서로 검사한다.
        for (let offset = 0; offset < 3; offset += 1) {
            if (this.#edge(token, (x - 3) >>> 0, (y - 2 + offset) >>> 0, SIDE_K) || this.#edge(token, (x + 1) >>> 0, (y - 2 + offset) >>> 0, SIDE_K)) return true;
        }
        return false;
    }

    // 실제 count 전체 DWORD 가 0 인지만 검사하고 다른 본문 바이트를 보존한다(40/120초를 현재 QWORD 시각에 더함). (2026-11-09 WB04-E176)
    delayRetry(token) {
        const candidates = this.#candidates;
        const delay = candidates.field(token, 0x350) === 0 ? EMPTY_DELAY : FULL_DELAY;
        const view = new DataView(new ArrayBuffer(8));
        view.setFloat64(0, this.#now() + delay, true);
        candidates.setField(token, 0x5d0, view.getUint32(0, true));
        candidates.setField(token, 0x5d4, view.getUint32(4, true));
    }

    // 고정 graph/owner 와 절삭한 사각형을 보관하지만 현재 지도/코드는 각 후보마다 읽는다. (2026-11-09 WB04-E176)
    randomStart(token, bounds) {
        const candidates = this.#candidates;
        const graph = candidates.field(token, 0x298);
        const owner = candidates.field(token, 0x260);
        // 원본의 bottom → top → right → left CRT 순서이다.
        const bottom = truncate(bounds.bottom);
        const top = truncate(bounds.top);
        const right = truncate(bounds.right);
        const left = truncate(bounds.left);
        // 상한 제외 y → x 두 난수를 항상 소비하고 최대 20번 시도한다.
        for (let remaining = ATTEMPTS; remaining > 0; remaining -= 1) {
            const y = (this.#random.next((bottom - top) >>> 0) + top) >>> 0;
            const x = (this.#random.next((right - left) >>> 0) + left) >>> 0;
            if (x >= 256 || y >= 256) continue;
            const surface = candidates.surfaces[y * 256 + x];
            if (surface && candidates.graph(surface) === graph && candidates.pool.slot(surface)[34] === owner
                && (!candidates.spot(signed(x) - 1, signed(y)) || !candidates.spot(signed(x) + 1, signed(y)) || !candidates.spot(signed(x), signed(y) - 1) || !candidates.spot(signed(x), signed(y) + 1))
                && candidates.open(surface, signed(x), signed(y))) return { x: signed(x), y: signed(y) };
        }
        return { x: UNSET, y: UNSET };
    }

    // 행 경계의 실제 flat 주소도 보존하며 물리 지도 바깥은 기존 spot getter 가 거부한다. (2026-11-09 WB04-E176)
    rectangleFree(left, top, right, bottom) {
        const candidates = this.#candidates;
        // 원본의 inclusive X 순회이며 반전 폭은 빈 사각형이다.
        for (let x = left >>> 0; signed(x) <= right; x = (x + 1) >>> 0) {
            // 각 열의 inclusive Y 순서에서 첫 점유 즉시 실패한다.
            for (let y = top >>> 0; signed(y) <= bottom; y = (y + 1) >>> 0) if (candidates.spot(signed(x), signed(y)) !== 0) return false;
        }
        return true;
    }
}
