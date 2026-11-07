// SquidFinder(객체 번호, flags)의 연결 이웃을 실제 raw 풀·일반 해시에서 찾는 모듈이다(cpppj `o/RawSquidNeighbors.cpp/.h`, 10.78 전용; 원본 004b23e0/004b1d70/004b1e80/004b1e70).
// 끝 칸 변환(flag 0)과 다리/섬 연결(flag 4)의 일반 해시 순회와 flag 8 의 표면 지도(단계 0) 순회를 구별한다. 풀·해시·spot 은 조회 때마다 다시 읽으며 이 객체보다 오래 살아야 한다.
//  - RawSquidNeighbors(pool, hash, spots, types, frames): types = 타입 번호별 { flags1, flags2, footX, footY }, frames = 타입 번호별 프레임 코드 배열 [{ side, variant, number, flags }]. spots 는 256×256 바이트이다.
//    source(sid): 삭제 호출 전 포착하는 값 { x, y, type, frame }(raw 입력을 값으로 포착해 진단 복귀 뒤 원본 인자가 바뀌지 않게 함). footprint(sid)/center(sid): float 발자국 사각형(왼쪽/위는 1..255 보정, 원래 두 좌표가 (0,256) 안일 때만 이전 점 보존)과 중심(단정도 저장값).
//    first(sid): 연결 이웃이 없으면 0 인 첫 이웃(생성 직후 +0x34 값). static direction(fromX, fromY, toX, toY): from 에서 to 를 보는 네 방향 0 북/2 동/4 남/6 서(세로 차이는 단정도로 저장한 값, 가로는 배정도, 크기가 같으면 세로).
//  - RawSquidNeighborWalk(neighbors, sidOrSource, flags, lazyFrame): 원천(발자국·타입·프레임·중심)은 생성 때 한 번 읽고 후보 필드와 해시 체인은 Next 마다 다시 읽는다. 원천 발자국이 내부(spot 비트 8 의 AND)이면 시작하지 않는다(current 0).
//    flags 비트: 1 프레임 접합 생략, 2 표면 아닌 후보 허용, 4 abstract(extra 비트 1) 제외, 8 표면 지도 y/x 순회(004b1c20: 지도 밖 셀은 건너뜀, 자기 발자국 안 기준점 제외, signed short 중복 캐시). 그 밖의 flags 는 RangeError.
//    accept: 표면 타입 → 가로/세로 확장 교차의 XOR(flag 8 은 생략) → 중심 방향 접합(bridgeConnects) → dead → abstract → 후보 기준점의 spot(좌표 + 0.9999 를 0 쪽으로 절삭한 칸, 배열 밖만 거부)이 내부가 아닐 것 이다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E39: raw 풀 연결 이웃 탐색 이식으로 추가)
import { WORLD_CELLS } from '../assets/terrain-mask.js';
import { bridgeConnects, TYPE_FLAG1_SURFACE } from './surface-finder.js';
import { RawSquidFinder } from './raw-squid-finder.js';

// raw 필드 위치(타입·상태·좌표·프레임·extra)와 상태/extra 비트, 원본 발자국 보정이 쓰는 지도 좌표 양 끝, 후보 기준점 spot 칸에 더하는 값이다. (2026-11-08 WB04-E39)
const TYPE_OFFSET = 10;
const STATE_OFFSET = 11;
const X_OFFSET = 14;
const Y_OFFSET = 18;
const FRAME_OFFSET = 36;
const EXTRA_OFFSET = 40;
const DEAD = 2;
const ABSTRACT = 1;
const INTERIOR = 8;
const FIRST_CELL = 1;
const LAST_CELL = 255;
const SPOT_BIAS = Math.fround(0.9999);
const FIRST_ASSET_TYPE = 70;

// 탐색기 flags 비트이다. (2026-11-08 WB04-E39)
export const NEIGHBOR_FLAG = Object.freeze({ noJoint: 1, anyType: 2, skipAbstract: 4, surfaceMap: 8, supported: 15 });

// 정렬되지 않은 리틀엔디언 필드와 float 비트 읽기이다. (2026-11-08 WB04-E39)
function read(raw, offset, width = 4) {
    let value = 0;
    // 필드 폭 안에서만 낮은 바이트부터 합친다.
    for (let index = 0; index < width; index += 1) value += raw[offset + index] * 2 ** (8 * index);
    return value >>> 0;
}
const floatView = new DataView(new ArrayBuffer(4));
function floatOf(raw, offset) {
    floatView.setUint32(0, read(raw, offset), true);
    return floatView.getFloat32(0, true);
}

// std::clamp / std::min / std::max 와 같은 비교 순서이다. (2026-11-08 WB04-E39)
const clamp = (value, low, high) => (value < low ? low : high < value ? high : value);
const minOf = (a, b) => (b < a ? b : a);
const maxOf = (a, b) => (a < b ? b : a);

// x87 덧셈 뒤 _ftol 과 같이 중간값을 float 로 좁히지 않고 0 방향으로 자른 칸 번호이다. (2026-11-08 WB04-E39)
const biasedCell = (value) => Math.trunc(value + SPOT_BIAS);

export class RawSquidNeighbors {
    #pool;
    #hash;
    #spots;
    #types;
    #currentTypes = null;
    #frames;

    // 손상된 표/지도 연결은 조회를 시작하기 전에 거부한다.
    constructor(pool, hash, spots, types, frames) {
        if (spots.length !== WORLD_CELLS * WORLD_CELLS || types.length === 0 || types.length !== frames.length) throw new TypeError('raw 이웃 타입/프레임/지도 크기 오류');
        this.#pool = pool;
        this.#hash = hash;
        this.#spots = spots;
        this.#types = Array.from(types);
        this.#frames = Array.from(frames);
    }

    get pool() { return this.#pool; }

    get hash() { return this.#hash; }

    get spots() { return this.#spots; }

    // 실제 World 의 현재 대여 표 또는 기존 독립 소유 사본이다.
    types() { return this.#currentTypes ?? this.#types; }

    // 실제 PATCH World 의 현재 TYPE 를 빌리고 null 은 독립 소유 사본으로 돌린다. 풀/해시/코드를 교체하지 않는다.
    bindCurrentTypes(types) {
        if (types !== null && types.length !== 188) throw new TypeError('이웃 현재 TYPE 대여의 길이 오류');
        this.#currentTypes = types;
    }

    // 원본 타입 조회의 범위 밖 주소 산술을 예외로 바꾼다.
    frames(type) {
        if (type >= this.#frames.length) throw new RangeError('raw 이웃 프레임 타입 범위');
        return this.#frames[type];
    }

    // 원문의 선택된 현재 레코드만 반환한다.
    currentType(type) {
        const types = this.types();
        if (type >= types.length) throw new RangeError('이웃 현재 TYPE 범위 오류');
        return types[type];
    }

    // 실제 슬롯의 정보만 포착하며 프레임 코드나 후보를 아직 읽지 않는다.
    source(sid) {
        if (!sid || sid >= this.#pool.capacity) throw new RangeError('raw 이웃 SID 오류');
        const raw = this.#pool.slot(sid);
        return { x: floatOf(raw, X_OFFSET), y: floatOf(raw, Y_OFFSET), type: raw[TYPE_OFFSET], frame: read(raw, FRAME_OFFSET) };
    }

    // 값 원천의 보정 발자국/중심을 만들고 필요한 프레임만 판독한다. 후보의 같은 타입 자료는 바꾸지 않는다.
    readObject(source, readFrame, lazyFrame = false) {
        const number = source.type;
        if (number < FIRST_ASSET_TYPE || number >= this.types().length) throw new RangeError('raw 이웃 자산 타입 오류');
        const type = this.types()[number];
        const { x, y } = source;
        if (!Number.isFinite(x) || !Number.isFinite(y) || type.footX < 1 || type.footY < 1 || type.footX > WORLD_CELLS || type.footY > WORLD_CELLS) {
            throw new RangeError(`raw 이웃 좌표/발자국 오류 TYPE=${number} x=${x} y=${y} foot=${type.footX},${type.footY}`);
        }
        // 실제 49ae80 의 모서리 확장은 원래 두 좌표가 모두 (0,256) 안일 때만 이전 점을 보존한다.
        const left = clamp(Math.fround(x - Math.fround(type.footX - 1)), FIRST_CELL, LAST_CELL);
        const top = clamp(Math.fround(y - Math.fround(type.footY - 1)), FIRST_CELL, LAST_CELL);
        const valid = x > 0 && x < WORLD_CELLS && y > 0 && y < WORLD_CELLS;
        const object = {
            left: valid ? minOf(x, left) : left, top: valid ? minOf(y, top) : top,
            right: valid ? maxOf(x, left) : left, bottom: valid ? maxOf(y, top) : top,
            centerX: Math.fround(x - Math.fround(type.footX * 0.5)), centerY: Math.fround(y - Math.fround(type.footY * 0.5)),
            type: number, frame: { side: 0, variant: 0, number: 0, flags: 0 },
        };
        if (readFrame) {
            const codes = this.frames(number);
            if (source.frame >= codes.length) {
                if (!lazyFrame) throw new RangeError('raw 이웃 프레임 오류');
            } else object.frame = codes[source.frame];
        }
        return object;
    }

    // 발자국만 필요한 호출자에게 사각형을 그대로 준다.
    footprint(sid) {
        const object = this.readObject(this.source(sid), false);
        return { left: object.left, top: object.top, right: object.right, bottom: object.bottom };
    }

    // 중심은 x87 에서 계산한 뒤 단정도로 저장한 값이다.
    center(sid) {
        const object = this.readObject(this.source(sid), false);
        return [object.centerX, object.centerY];
    }

    // 0041ce90 의 flag 1 분기: 가로 차이는 배정도, 세로 차이는 단정도로 저장했다가 다시 읽는다. |세로| < |가로| 가 아니면(같음·NaN 포함) 세로 방향이다.
    static direction(fromX, fromY, toX, toY) {
        const dx = toX - fromX;
        const dy = Math.fround(toY - fromY);
        if (!(Math.abs(dy) < Math.abs(dx))) return dy > 0 ? 4 : 0;
        return dx > 0 ? 2 : 6;
    }

    // 004ab880: 발자국 전체의 spot 비트를 AND 한다(네 정수 모서리를 각각 1..255 로 보정).
    interior(source) {
        let bits = 0xff;
        const left = clamp(Math.trunc(source.left), 1, 255);
        const right = clamp(Math.trunc(source.right), 1, 255);
        const top = clamp(Math.trunc(source.top), 1, 255);
        const bottom = clamp(Math.trunc(source.bottom), 1, 255);
        // 정수 절삭/보정 후 원본 y/x 순서로 발자국을 읽는다.
        for (let y = top; y <= bottom; y += 1) for (let x = left; x <= right; x += 1) bits &= this.#spots[y * WORLD_CELLS + x];
        return (bits & INTERIOR) !== 0;
    }

    // 004b1e80: 일반 finder 의 기하 교차와 flags 8 지도 후보를 구별하고 현재 프레임/상태/spot 을 검사한다.
    accept(candidate, source, flags) {
        const raw = this.#pool.slot(candidate);
        const number = raw[TYPE_OFFSET];
        if ((this.currentType(number).flags1 & TYPE_FLAG1_SURFACE) === 0 && (flags & NEIGHBOR_FLAG.anyType) === 0) return false;
        const object = this.readObject(this.source(candidate), false);
        // 0041d9b0: 후보 사각형을 축별로 한 칸 넓혀 source 와 교차하며 정확히 한 축만 겹쳐야 한다.
        const intersects = (left, top, right, bottom) => {
            const x1 = maxOf(source.left, left);
            const y1 = maxOf(source.top, top);
            const x2 = minOf(source.right, right);
            const y2 = minOf(source.bottom, bottom);
            return x1 >= FIRST_CELL && y1 >= FIRST_CELL && x2 < WORLD_CELLS && y2 < WORLD_CELLS && x1 <= x2 && y1 <= y2;
        };
        if ((flags & NEIGHBOR_FLAG.surfaceMap) === 0 &&
            intersects(Math.fround(object.left - 1), object.top, Math.fround(object.right + 1), object.bottom) ===
            intersects(object.left, Math.fround(object.top - 1), object.right, Math.fround(object.bottom + 1))) return false;
        if ((flags & NEIGHBOR_FLAG.noJoint) === 0) {
            const framed = this.readObject(this.source(candidate), true);
            // 후보 중심에서 source 중심을 보는 방향으로 두 프레임의 접합을 검사한다.
            const direction = RawSquidNeighbors.direction(framed.centerX, framed.centerY, source.centerX, source.centerY);
            if (!bridgeConnects(framed.frame, this.types()[number].flags2, source.frame, this.types()[source.type].flags2, direction)) return false;
        }
        if ((raw[STATE_OFFSET] & DEAD) !== 0) return false;
        if ((flags & NEIGHBOR_FLAG.skipAbstract) !== 0 && (raw[EXTRA_OFFSET] & ABSTRACT) !== 0) return false;
        // 후보 기준점의 spot 은 좌표에 0.9999 를 더해 자른 칸에서 읽는다. 원본은 y*256+x 의 선형 주소를 읽으므로 배열 밖만 거부한다.
        const index = biasedCell(floatOf(raw, Y_OFFSET)) * WORLD_CELLS + biasedCell(floatOf(raw, X_OFFSET));
        if (index < 0 || index >= this.#spots.length) throw new RangeError('raw 이웃 spot 칸 범위 오류');
        return (this.#spots[index] & INTERIOR) === 0;
    }

    // 첫 반환 뒤의 버킷/슬롯은 읽지 않는다. 별도 일반 finder 를 사용하므로 다른 삭제 탐색 커서를 덮지 않는다.
    first(sid) {
        return new RawSquidNeighborWalk(this, sid, 0).current;
    }
}

// 004b23e0 으로 만든 탐색기의 순회이다. 생성 직후 current 가 첫 이웃(원본 +0x34)이다. (2026-11-08 WB04-E39)
export class RawSquidNeighborWalk {
    #finder;
    #surfaceNext = null;
    #current = 0;

    // neighbors 는 이 객체보다 오래 살아야 한다. sidOrSource 는 SID 또는 { x, y, type, frame } 값 원천이다.
    constructor(neighbors, sidOrSource, flags, lazyFrame = false) {
        this.#finder = new RawSquidFinder(neighbors.pool, neighbors.hash, neighbors.types());
        if ((flags & ~NEIGHBOR_FLAG.supported) !== 0) throw new RangeError('raw 이웃 flags 범위 오류');
        const source = typeof sidOrSource === 'number' ? neighbors.source(sidOrSource) : sidOrSource;
        const object = neighbors.readObject(source, true, lazyFrame);
        if (neighbors.interior(object)) return;
        if ((flags & NEIGHBOR_FLAG.surfaceMap) !== 0) {
            // 원문 _ftol 로 저장하는 확장 사각형의 정수 네 모서리이다. 지도 밖 셀은 Next 에서 0 으로 취급한다.
            const left = Math.trunc(Math.fround(object.left - 1));
            const top = Math.trunc(Math.fround(object.top - 1));
            const right = Math.trunc(Math.fround(object.right + 1));
            const bottom = Math.trunc(Math.fround(object.bottom + 1));
            let x = left;
            let y = top;
            // 원천/flags/사각형은 생성 때 보관한다. 커서와 SHORT 캐시는 각 순회가 독립적으로 소유한다.
            const seen = [];
            this.#surfaceNext = () => {
                // 원문 004b1c20 의 선증가와 모서리 생략을 유지하며 현재 셀/후보를 매 Next 마다 읽는다.
                for (;;) {
                    x += 1;
                    if (y === top || y === bottom) {
                        if (x >= right) { y += 1; if (y > bottom) return 0; x = left; }
                    } else if (x > right) { y += 1; x = left + (y === bottom ? 1 : 0); }
                    if (x < 0 || y < 0 || x >= WORLD_CELLS || y >= WORLD_CELLS) continue;
                    const candidate = neighbors.hash.cell(0, x, y);
                    if (!candidate) continue;
                    const raw = neighbors.pool.slot(candidate);
                    const cx = floatOf(raw, X_OFFSET);
                    const cy = floatOf(raw, Y_OFFSET);
                    if (object.left <= cx && cx <= object.right && object.top <= cy && cy <= object.bottom) continue;
                    if (!neighbors.accept(candidate, object, flags)) continue;
                    // 원문 캐시는 signed short 를 int 로 확장해 DWORD SID 와 비교한다. 별도 자기 번호 제외는 없다.
                    if (seen.some((prior) => prior === candidate)) continue;
                    seen.push((candidate << 16) >> 16);
                    return candidate;
                }
            };
            this.#current = this.#surfaceNext();
            return;
        }
        this.#current = this.#finder.begin({
            left: Math.trunc(Math.fround(object.left - 1)), top: Math.trunc(Math.fround(object.top - 1)),
            right: Math.trunc(Math.fround(object.right + 1)), bottom: Math.trunc(Math.fround(object.bottom + 1)),
        }, 0, (candidate) => neighbors.accept(candidate, object, flags));
    }

    // 현재 이웃 번호를 읽기만 한다. 0 이면 끝이다.
    get current() { return this.#current; }

    // 끝난 뒤(또는 내부 발자국이라 시작하지 않은 경우)에는 일반 탐색을 다시 부르지 않는다.
    next() {
        if (this.#current !== 0) this.#current = this.#surfaceNext ? this.#surfaceNext() : this.#finder.next();
        return this.#current;
    }
}
