// AI 다리의 실제 모양·점유·끝점 평가와 호출당 8개 후보 검색을 원본 본체·프레임·spot·난수에 적용하는 모듈이다(cpppj `o/RawAiBridgeSearch.cpp/.h`, 10.78: 패치 0040f400 모양·0040f880 점유·0040f910 끝점 점수·004144b0 검색).
//  - new RawAiBridgeSearch(memory, frames, spots, bridge, random): memory 는 E164 `RawAiBrainMemory`(본체 0xb08 바이트), frames 는 현재 TYPE 번호 순서의 프레임 코드 배열 표(각 항목은 `{ side, variant, number, flags }` 배열이며 `CanonDecoder` 가 읽는 형태),
//    spots 는 현재 65536 BYTE 점유 지도(Uint8Array), bridge 는 005411a0 의 현재 전체 DWORD TYPE 번호를 돌려주는 getter, random 은 `GameRandom` 과 같은 `next(limit)` 를 가진 같은 게임 공용 난수이다. 별도 월드 사본이나 난수 스트림을 만들지 않는다.
//  - shape(pattern, direction, raw): 0040f400 — 실제 다리 패턴(26개)과 현재 bridge 프레임을 (1, 1) 기준으로 펼쳐 8×8 BYTE grid(+x*8+y)에 방향 글자를 쓰고, 같은 칸의 J(수직 북→남)/K(수평 동→서) 끝점만 12 BYTE(+0x40 + 개수*12: x, y, 방향)로 기록해
//    +0xb8 개수를 갱신한다. caller 의 grid/미사용 끝점 여백은 지우지 않는다(0xbc BYTE 전체를 받아 쓴다).
//  - clear(placement): 0040f880 — 실제 decoder 순서의 모든 칸에서 y 절삭 → x 절삭 → flat spot(y*256+x)이 0 인지 확인하고 첫 점유 칸에서 즉시 false, 빈 decoder 는 true 이다.
//  - score(placement, targetX, targetY, shape, excluded, scratch): 0040f910 — 연결에 쓴 끝점(excluded)을 제외하고 끝점마다 X 는 double 로 유지하고 Y 만 float 로 먼저 저장해 목표와의 두 축 절댓값 합(INT_MIN 감김)을 구하며,
//    방향이 열린 끝점과 다르면 금지(65536)/벌점(+4, 최소 6)을 적용하고 strict 최저점에서만 scratch(`{ x, y }`)를 float 로 갱신한다.
//  - search(token, scratch): 004144b0 — 호출당 후보×재고×회전 8개를 처리하고 예산이 소진되면 false(진행 필드 +0x6c4/+0x6c8/+0x6cc 유지), 네 회전이 끝나면 true 이다. 같은 모양의 모든 일치 끝점을 검사하고 strict 최저 또는 동점의 난수(100 중 50 미만)에서 +0x6a8..+0x6c0 결과를 갱신한다.
//    scratch 는 원본 caller 의 미초기화 임시 XY 입력/출력이며 NaN 의 비트 보존이 필요하면 `xBits`/`yBits`(float 원문 비트)를 함께 둔다(이 모듈이 새 값을 쓰면 지운다).
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E178: AI 다리 검색 이식으로 추가)
import { AI_BRAIN_BYTES } from './raw-ai-brain.js';
import { CanonDecoder } from '../assets/territory.js';
import { BRIDGE_PATTERNS } from '../assets/canon-type-patterns.js';

// 모양 버퍼 크기(8×8 grid + 끝점 10개×12 + 개수)와 끝점 배열 시작/개수 위치, 검색 호출 예산, 초기 최적 점수이다. (2026-11-09 WB04-E178)
export const SHAPE_BYTES = 0xbc;
const SHAPE_ENDPOINTS = 0x40;
const SHAPE_COUNT = 0xb8;
const SEARCH_BUDGET = 8;
const INITIAL_BEST = 65536;
const TWO_63 = 9223372036854775808;

// 0052f83c/0052f85c 의 원본 이웃 좌표이다(북, 북동, 동, 남동, 남, 남서, 서, 북서). (2026-11-09 WB04-E178)
const DX = Object.freeze([0, 1, 1, 1, 0, -1, -1, -1]);
const DY = Object.freeze([-1, -1, 0, 1, 1, 1, 0, -1]);
// 0052f8bc 의 반대 방향이며 -1 조회는 바로 앞의 float -1 비트(-1082130432)도 실제로 읽는다. (2026-11-09 WB04-E178)
const OPPOSITE = Object.freeze([-1082130432, 4, 5, 6, 7, 0, 1, 2, 3]);

// 비정렬 DWORD 를 주변 바이트와 무관하게 little endian 으로 읽는다. (2026-11-09 WB04-E178)
function read(raw, offset) {
    if (offset < 0 || offset > raw.length || raw.length - offset < 4) throw new RangeError('AI 다리 검색 DWORD 읽기 범위');
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 지정한 네 BYTE 만 낮은 바이트부터 쓴다. (2026-11-09 WB04-E178)
function write(raw, offset, value) {
    if (offset < 0 || offset > raw.length || raw.length - offset < 4) throw new RangeError('AI 다리 검색 DWORD 쓰기 범위');
    for (let index = 0; index < 4; index += 1) raw[offset + index] = (value >>> (index * 8)) & 0xff;
}

// 원문 DWORD 의 signed 비교를 보존한다. (2026-11-09 WB04-E178)
const signed = (value) => value | 0;

// 원본 __ftol signed64 절삭의 낮은 DWORD 이며 indefinite(비유한/범위 밖)의 하위는 0 이다. (2026-11-09 WB04-E178)
function truncate(value) {
    if (!Number.isFinite(value) || value < -TWO_63 || value >= TWO_63) return 0;
    return Number(BigInt.asUintN(32, BigInt(Math.trunc(value))));
}

// INT_MIN 도 원본 xor/sub 의 32비트 감김으로 처리한 signed 절댓값이다. (2026-11-09 WB04-E178)
function absolute(value) {
    return signed(signed(value) < 0 ? (0 - value) >>> 0 : value);
}

// 전체 방향 번호를 검사한 뒤 원본 이웃 표를 조회할 번호를 돌려준다. (2026-11-09 WB04-E178)
function direction(value) {
    if (value >= 8) throw new RangeError('AI 다리 검색 방향 표 범위');
    return value;
}

// 단정도 float 와 비트의 상호 변환이다. (2026-11-09 WB04-E178)
function floatBits(value) {
    const view = new DataView(new ArrayBuffer(4));
    view.setFloat32(0, value, true);
    return view.getUint32(0, true);
}

// 임시 XY 의 한 좌표 원문 비트이다(NaN 비트 보존용 xBits/yBits 가 있으면 그 값). (2026-11-09 WB04-E178)
function coordinateBits(point, axis) {
    return axis === 'x' ? (point.xBits ?? floatBits(point.x)) >>> 0 : (point.yBits ?? floatBits(point.y)) >>> 0;
}

// 다리 검색기이다. 생성자에서는 모양/검색 효과를 실행하지 않는다. (2026-11-09 WB04-E178)
export class RawAiBridgeSearch {
    #memory;
    #frames;
    #spots;
    #bridge;
    #random;

    // 현재 bridge 전역/프레임과 공용 난수를 빌린다. (2026-11-09 WB04-E178)
    constructor(memory, frames, spots, bridge, random) {
        if (!memory || typeof memory.bytes !== 'function') throw new TypeError('AI 다리 검색 본체 소유자 누락');
        if (!Array.isArray(frames)) throw new TypeError('AI 다리 검색 현재 프레임 표 누락');
        if (!(spots instanceof Uint8Array) || spots.length !== 65536) throw new RangeError('AI 다리 검색 실제 spot 65536 누락');
        if (typeof bridge !== 'function') throw new TypeError('AI 다리 검색 현재 다리 TYPE 조회기 누락');
        if (typeof random?.next !== 'function') throw new TypeError('AI 다리 검색 공용 난수 누락');
        this.#memory = memory;
        this.#frames = frames;
        this.#spots = spots;
        this.#bridge = bridge;
        this.#random = random;
    }

    // 매 호출마다 현재 소유 블록을 다시 조회해 정확한 0xb08 본체만 허용한다. (2026-11-09 WB04-E178)
    #body(token) {
        const raw = this.#memory.bytes(token);
        if (raw.length !== AI_BRAIN_BYTES) throw new RangeError('AI 다리 검색 실제0xb08 본체 누락');
        return raw;
    }

    // 원본 생성자의 bridge 경로이며 패턴 번호에 임의 마스크를 추가하지 않는다. (2026-11-09 WB04-E178)
    #decoder(point, pattern, directionValue) {
        const type = this.#bridge() >>> 0;
        if (type >= this.#frames.length || pattern >= BRIDGE_PATTERNS.length) throw new RangeError('AI 다리 검색 현재TYPE/패턴 범위');
        return new CanonDecoder(this.#frames[type], BRIDGE_PATTERNS[pattern], signed(directionValue), point.x, point.y);
    }

    // 0040f400: caller 가 이미 비운 64 BYTE grid 를 다시 비우지 않고 실제 칸과 끝점 개수만 갱신한다. raw 는 0xbc BYTE 전체이다. (2026-11-09 WB04-E178)
    shape(pattern, directionValue, raw) {
        if (raw.length !== SHAPE_BYTES) throw new RangeError('AI 다리 모양 실제0xbc 크기');
        const decoder = this.#decoder({ x: 1, y: 1 }, pattern >>> 0, directionValue >>> 0);
        const cells = [];
        // 실제 decoder 가 내는 칸 순서를 끝점 검사에도 유지한다.
        for (; decoder.valid; decoder.advance()) {
            const x = signed(truncate(decoder.x));
            const y = signed(truncate(decoder.y));
            if (x < 1 || x > 6 || y < 1 || y > 6) throw new RangeError('AI 다리 모양1..6 좌표');
            raw[x * 8 + y] = decoder.side & 0xff;
            cells.push([x, y]);
        }
        write(raw, SHAPE_COUNT, 0);
        // 같은 칸의 북→남 또는 동→서 순서로만 열린 끝점을 기록한다.
        for (const [x, y] of cells) {
            const side = raw[x * 8 + y];
            if (side !== 0x4a && side !== 0x4b) continue;
            // J 는 수직(북 0/남 4), K 는 수평(동 2/서 6) 두 끝만 검사한다.
            for (const edge of (side === 0x4a ? [0, 4] : [2, 6])) {
                if (raw[(x + DX[edge]) * 8 + y + DY[edge]] !== 0) continue;
                const count = read(raw, SHAPE_COUNT);
                const offset = SHAPE_ENDPOINTS + count * 12;
                write(raw, SHAPE_COUNT, count + 1);
                write(raw, offset, x >>> 0);
                write(raw, offset + 4, y >>> 0);
                write(raw, offset + 8, edge);
            }
        }
    }

    // 0040f880: y 절삭 → x 절삭 → flat spot 조회 순서이며 첫 점유 칸에서 즉시 false, 빈 decoder 는 true 이다. (2026-11-09 WB04-E178)
    clear(placement) {
        const decoder = this.#decoder(placement.point, placement.pattern >>> 0, placement.direction >>> 0);
        // 첫 점유 칸에서 즉시 중단한다.
        for (; decoder.valid; decoder.advance()) {
            const y = truncate(decoder.y);
            const x = truncate(decoder.x);
            const index = ((y << 8) + x) >>> 0;
            if (index >= this.#spots.length) throw new RangeError('AI 다리 검색spot 물리 범위');
            if (this.#spots[index] !== 0) return false;
        }
        return true;
    }

    // 0040f910: X 절대점은 x87 에 유지하고 Y 절대점만 float 에 먼저 저장하며 strict 최저점에서만 scratch 를 갱신한다. (2026-11-09 WB04-E178)
    score(placement, targetX, targetY, shape, excluded, scratch) {
        if (shape.length !== SHAPE_BYTES) throw new RangeError('AI 다리 끝점 실제0xbc 크기');
        let best = INITIAL_BEST;
        // 원본 순회와 strict 최소 비교이므로 끝점 자체 동률은 바꾸지 않는다.
        for (let index = 0; signed(index) < signed(read(shape, SHAPE_COUNT)); index += 1) {
            if (index === (excluded >>> 0)) continue;
            const offset = SHAPE_ENDPOINTS + index * 12;
            const x = signed(read(shape, offset) - 1) + placement.point.x;
            const y = Math.fround(signed(read(shape, offset + 4) - 1) + placement.point.y);
            const dx = truncate(targetX - x);
            const dy = truncate(targetY - y);
            const absX = absolute(dx);
            const absY = absolute(dy);
            let score = signed((absX >>> 0) + (absY >>> 0));
            let wanted;
            if (absX === absY) wanted = -1;
            else if (absX > absY || absX === 2) wanted = signed(dx) <= 0 ? 6 : 2;
            else wanted = signed(dy) <= 0 ? 0 : 4;
            const edge = signed(read(shape, offset + 8));
            if (edge !== wanted) {
                if (edge === OPPOSITE[wanted + 1] || absX === 2 || absY === 2) score = INITIAL_BEST;
                else {
                    score = signed((score >>> 0) + 4);
                    if (score <= 6) score = 6;
                }
            }
            if (score < best) {
                best = score;
                scratch.x = Math.fround(x);
                scratch.y = y;
                delete scratch.xBits;
                delete scratch.yBits;
            }
        }
        return best;
    }

    // 004144b0: 본체의 +0x6c4 회전 → +0x6c8 역순 재고 → +0x6cc 정순 연결점 상태를 그대로 재개하고, 8개 예산이 소진되면 false 이다. (2026-11-09 WB04-E178)
    search(token, scratch) {
        const raw = this.#body(token);
        let budget = SEARCH_BUDGET;
        if (signed(read(raw, 0x6c8)) >= signed(read(raw, 0x628))) write(raw, 0x6c8, (read(raw, 0x628) - 1) >>> 0);
        // 네 회전이 끝날 때만 true 이며 예산 중단은 현재 진행 필드를 유지한다.
        while (signed(read(raw, 0x6c4)) <= 7) {
            // 재고 마지막 조각부터 첫 조각으로 내려간다.
            while (signed(read(raw, 0x6c8)) >= 0) {
                // 같은 회전/조각에 각 연결점을 정순으로 대입한다.
                while (signed(read(raw, 0x6cc)) < signed(read(raw, 0x690))) {
                    if (budget < 1) return false;
                    const candidate = (0x648 + Math.imul(read(raw, 0x6cc), 12)) >>> 0;
                    const edge = direction(read(raw, (candidate + 8) >>> 0));
                    const desired = OPPOSITE[edge + 1];
                    const shape = new Uint8Array(SHAPE_BYTES);
                    const pattern = read(raw, (0x608 + Math.imul(read(raw, 0x6c8), 4)) >>> 0);
                    const rotation = read(raw, 0x6c4);
                    budget -= 1;
                    this.shape(pattern, rotation, shape);
                    // 8개 예산은 끝점 수와 별개이며 같은 모양의 모든 일치 끝점을 검사한다.
                    for (let endpoint = 0; signed(endpoint) < signed(read(shape, SHAPE_COUNT)); endpoint += 1) {
                        const offset = SHAPE_ENDPOINTS + endpoint * 12;
                        if (signed(read(shape, offset + 8)) !== desired) continue;
                        const placement = {
                            point: {
                                x: Math.fround(signed(read(raw, candidate) + 1 + (DX[edge] >>> 0) - read(shape, offset))),
                                y: Math.fround(signed(read(raw, (candidate + 4) >>> 0) + 1 + (DY[edge] >>> 0) - read(shape, offset + 4)))
                            },
                            pattern,
                            direction: rotation
                        };
                        if (!this.clear(placement)) continue;
                        const score = this.score(placement, signed(read(raw, 0x640)), signed(read(raw, 0x644)), shape, endpoint, scratch);
                        const best = signed(read(raw, 0x6a8));
                        if (score > best || (score === best && this.#random.next(100) >= 50)) continue;
                        write(raw, 0x6ac, floatBits(placement.point.x));
                        write(raw, 0x6b0, floatBits(placement.point.y));
                        write(raw, 0x6b4, pattern);
                        write(raw, 0x6b8, rotation);
                        write(raw, 0x6a8, score >>> 0);
                        write(raw, 0x6bc, coordinateBits(scratch, 'x'));
                        write(raw, 0x6c0, coordinateBits(scratch, 'y'));
                    }
                    write(raw, 0x6cc, (read(raw, 0x6cc) + 1) >>> 0);
                }
                write(raw, 0x6cc, 0);
                write(raw, 0x6c8, (read(raw, 0x6c8) - 1) >>> 0);
            }
            write(raw, 0x6c8, (read(raw, 0x628) - 1) >>> 0);
            write(raw, 0x6c4, (read(raw, 0x6c4) + 2) >>> 0);
        }
        return true;
    }
}
