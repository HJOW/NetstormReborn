// 지역 투표 뒤의 실제 지면 flood 와 패치판 경계 프레임 도장을 같은 SID 풀에 적용한 모듈이다(cpppj `o/RawRegionPaint.cpp/.h`, 패치 00470900/0046fe40/004703b0, 10.78 전용).
// 사용: new RawRegionPaint(pool, frames, islands, terrainType, edgeFarmType, random, state, ownership, { setOwner, boundaryAt, regionAt }).paint({ x, y, owner, theme }) 는
//   ① 경계 후보 목록을 비우고 drawer 의 네 float 을 (257, 257, -1, -1)로 다시 쓴다 → ② 시작 칸의 표면이 지면 타입이면 요청 소유자/원소를 기록하고 북·동·남·서 flood 로 현재 요청과 다른 소유자의 지면에
//   소유자를 지정(가상 +0x74)하고 프레임 DWORD(+36)를 직접 쓴다 → ③ 빈 표면/월드 밖 후보와 지면 경계의 후보를 삽입 순서로 훑어 영향 지역 기록·edgeFarm 소유자/프레임 지정·이웃 후보 추가를 한다 → ④ 월드 다시 그리기 DWORD 를 하나 올린다.
// 프레임 선택은 본섬 안쪽(AA)은 독립 99 캐시 + 원소 36프레임 구간, 그 밖은 실제 후보 검색(`chooseTerrainFrame`, 게임 난수)이다. 이 모듈은 DOM 을 쓰지 않는다. CD(10.72)의 소유자 flood 전용 분기는 옮기지 않았다.
// (2026-11-11 WB04-E296: 4-B RawRegionPaint 이식)
import { chooseTerrainFrame, createPaintVariants } from '../assets/terrain-tiles.js';
import { connectionMask } from '../assets/terrain-mask.js';

// 지형 좌표(raw +14/+18)와 패치 판본의 프레임(+36)/소유자(+34)/TYPE(+10) 오프셋이다. (2026-11-11 WB04-E296)
const SLOT_X = 14;
const SLOT_Y = 18;
const SLOT_FRAME = 36;
const SLOT_OWNER = 34;
const SLOT_TYPE = 10;

// 원본 네 방향은 북·동·남·서이며 연결 비트는 1·2·4·8 이다. (2026-11-11 WB04-E296)
const DIRECTIONS = Object.freeze([Object.freeze({ x: 0, y: -1 }), Object.freeze({ x: 1, y: 0 }), Object.freeze({ x: 0, y: 1 }), Object.freeze({ x: -1, y: 0 })]);

// 지도 한 변 칸 수, 99 캐시 크기, 안쪽 프레임의 원소별 구간 크기(3×3 묶음 × 4 변형)와 변형 수, TYPE 수이다. (2026-11-11 WB04-E296)
const MAP_SIDE = 256;
const VARIANT_COUNT = 99;
const THEME_FRAMES = 36;
const CORE_VARIANTS = 4;
const TYPE_COUNT = 188;

// float 비트를 uint32 정수로 되돌릴 때 쓰는 보조 뷰이다(경계 후보의 정수 좌표를 float 비트로 재해석하는 영향 지역 기록용). (2026-11-11 WB04-E296)
const BITS = new DataView(new ArrayBuffer(4));

// 정수를 uint32 비트 그대로 float32 로 재해석한 값이다(원본 bit_cast). NaN 비트열은 숫자로 보존되지 않으므로 호출자가 따로 비트를 보관한다. (2026-11-11 WB04-E296)
function bitsToFloat(bits) {
    BITS.setUint32(0, bits >>> 0, true);
    return BITS.getFloat32(0, true);
}

// Region drawer 의 네 float·현재 요청·두 독립 변형 캐시와 좌표 후보 목록이다. bounds 는 생성자가 -10000 을 저장하며 각 Paint 호출이 (257, 257, -1, -1)로 다시 쓴다(float32 값).
// owner: 마지막 유효 지면에서 시작한 요청의 전체 DWORD 소유자, theme: 패치 요청의 원소 quartile(0/-1 은 기본, 1/2/그 밖은 다음 세 원소), revision: 독립 입력의 월드 다시 그리기 DWORD,
// boundary: 중복 없는 정수 좌표 후보(삽입 순서 `{ x, y }`), coreVariants/boundaryVariants: 본섬 안쪽/경계 도장의 서로 독립인 99 캐시(첫 사용에 만들고 재사용),
// sharedWorldRedraw: 같은 현재 DWORD 를 쓰는 `{ value }` 상자(주면 revision 대신 이 상자를 읽고 증가시킴)이다. (2026-11-11 WB04-E296)
export class RegionPaintState {
    bounds = [-10000, -10000, -10000, -10000];
    owner = 0;
    theme = 0;
    revision = 0;
    boundary = [];
    coreVariants = null;
    boundaryVariants = null;
    sharedWorldRedraw = null;

    // 현재 월드 다시 그리기 DWORD 이다(빌린 상자가 있으면 그 값). (2026-11-11 WB04-E296)
    get worldRedraw() {
        return this.sharedWorldRedraw ? this.sharedWorldRedraw.value >>> 0 : this.revision >>> 0;
    }

    // 현재 DWORD 를 직접 하나 올린다(DWORD 감김). 진입 시점의 값을 캐시하지 않고 호출 시점의 현재 값에 더한다. (2026-11-11 WB04-E296)
    incrementWorldRedraw() {
        if (this.sharedWorldRedraw) this.sharedWorldRedraw.value = (this.sharedWorldRedraw.value + 1) >>> 0;
        else this.revision = (this.revision + 1) >>> 0;
    }
}

// raw float 좌표에 원본 _ftol 의 0 방향 절삭을 적용한다. 비유한/정수 범위 밖은 거부한다. (2026-11-11 WB04-E296)
function coordinate(raw, offset) {
    BITS.setUint8(0, raw[offset]);
    BITS.setUint8(1, raw[offset + 1]);
    BITS.setUint8(2, raw[offset + 2]);
    BITS.setUint8(3, raw[offset + 3]);
    const value = Math.trunc(BITS.getFloat32(0, true));
    if (!Number.isFinite(value) || value < -0x80000000 || value > 0x7fffffff) throw new RangeError('지형 도장 좌표 절삭 범위 오류');
    return value;
}

// 비정렬 little endian DWORD 를 읽는다. (2026-11-11 WB04-E296)
function dword(raw, offset) {
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 소유자 가상 호출·빈 표면의 edgeFarm finder·현재 지역 getter 를 연결하는 도장 실행기이다. 풀/프레임 표/섬 지도/난수/상태/영향 기록은 월드와 같은 저장소이고 이 객체보다 오래 살아야 한다. (2026-11-11 WB04-E296)
export class RawRegionPaint {
    // 소유자와 직접 프레임 대입의 실제 raw 슬롯 저장소, 현재 타입별 코드·J 시작 번호의 조회 표(188칸 FrameCodeTable), 실제 hash0 표면 머리 WORD 지도(65,536칸)이다.
    #pool;
    #frames;
    #islands;
    // flood 가 처리하는 실제 isle 타입 번호, 빈 표면의 일반 finder 에서 찾는 지형 경계 타입 번호, 비-AA 후보 선택이 소비하는 게임 난수이다.
    #terrainType;
    #edgeFarmType;
    #random;
    // drawer 요청과 원본 전역의 도장 캐시/후보/revision 상태, 투표 wrapper 와 공유하는 중복 없는 영향 지역 기록, 실제 소유자/일반 finder/지역 조회 경계이다.
    #state;
    #ownership;
    #hooks;

    // 모든 필수 연결(10.78 풀·표 크기·세 훅)을 확인한다. (2026-11-11 WB04-E296)
    constructor(pool, frames, islands, terrainType, edgeFarmType, random, state, ownership, hooks) {
        if (pool?.edition !== 'patch1078' || !Array.isArray(frames) || frames.length !== TYPE_COUNT || !(islands instanceof Uint16Array) || islands.length !== MAP_SIDE * MAP_SIDE
            || !Number.isInteger(terrainType) || terrainType < 0 || terrainType >= frames.length || !Number.isInteger(edgeFarmType) || edgeFarmType < 0 || edgeFarmType >= frames.length
            || !random || typeof random.next !== 'function' || !(state instanceof RegionPaintState) || !Array.isArray(ownership?.affected)
            || ['setOwner', 'boundaryAt', 'regionAt'].some((name) => typeof hooks?.[name] !== 'function')) {
            throw new TypeError('지역 도장 지도/프레임/필수 효과 연결 오류');
        }
        this.#pool = pool;
        this.#frames = frames;
        this.#islands = islands;
        this.#terrainType = terrainType;
        this.#edgeFarmType = edgeFarmType;
        this.#random = random;
        this.#state = state;
        this.#ownership = ownership;
        this.#hooks = hooks;
    }

    // 외부 연결의 SID 저장소 일치 검사에 사용한다. (2026-11-11 WB04-E296)
    get pool() {
        return this.#pool;
    }

    // 월드 밖은 SID 0 이다. 별도의 snap 이나 좌표 clamp 를 넣지 않는다. (2026-11-11 WB04-E296)
    #surface(x, y) {
        return x >= 0 && y >= 0 && x < MAP_SIDE && y < MAP_SIDE ? this.#islands[y * MAP_SIDE + x] : 0;
    }

    // 소유자 가상 호출 뒤 현재 프레임을 지면 코드 배열에서 읽는다(drawer 가 캡처한 지면 타입의 코드 표). (2026-11-11 WB04-E296)
    #code(sid) {
        const index = dword(this.#pool.slot(sid), SLOT_FRAME);
        const codes = this.#frames[this.#terrainType].codes;
        if (index >= codes.length) throw new RangeError('지역 도장 현재 프레임 코드 범위 오류');
        return codes[index];
    }

    // 캐시는 각 항목마다 새 시드 0 의 첫 지역 난수를 쓰며 직전 항목과 mod 4 가 같으면 1 을 더한다(게임 난수를 소비하지 않음). (2026-11-11 WB04-E296)
    static #variants(current) {
        return current ?? createPaintVariants();
    }

    // AA 는 원본 좌표/99 캐시/36프레임 원소 구간을 쓰고 다른 코드는 실제 후보 검색을 사용한다. 두 캐시 좌표 항은 요청 x/y 이고 픽셀 residue 는 현재 raw 좌표를 절삭한다. (2026-11-11 WB04-E296)
    #select(sid, x, y, boundary) {
        const code = this.#code(sid);
        const state = this.#state;
        if (boundary) state.boundaryVariants = RawRegionPaint.#variants(state.boundaryVariants);
        else state.coreVariants = RawRegionPaint.#variants(state.coreVariants);
        const cache = boundary ? state.boundaryVariants : state.coreVariants;
        const type = this.#pool.slot(sid)[SLOT_TYPE];
        if (type >= this.#frames.length) throw new RangeError('지역 도장 타입 번호 오류');
        if (code.side !== 0x41 || code.variant !== 0x41) return chooseTerrainFrame(this.#frames[type], String.fromCharCode(code.side), String.fromCharCode(code.variant), state.theme, this.#random);
        const raw = this.#pool.slot(sid);
        const px = coordinate(raw, SLOT_X);
        const py = coordinate(raw, SLOT_Y);
        const bx = Math.trunc(x / 3) % VARIANT_COUNT;
        const by = Math.trunc(y / 3) % VARIANT_COUNT;
        if (bx < 0 || by < 0) throw new RangeError('지역 도장 변형 캐시 좌표 오류');
        const sum = (cache[bx] + cache[by]) % VARIANT_COUNT;
        const variant = (Math.trunc(py / 3) + cache[sum]) % CORE_VARIANTS;
        const theme = state.theme === 0 || state.theme === -1 ? 0 : state.theme === 1 ? 1 : state.theme === 2 ? 2 : 3;
        return this.#frames[type].run('J').first + 1 + theme * THEME_FRAMES + (variant * 3 + (py % 3)) * 3 + (px % 3);
    }

    // 원본은 SetFrame 가상 함수를 부르지 않고 프레임 DWORD 만 직접 저장한다. (2026-11-11 WB04-E296)
    #writeFrame(sid, frame) {
        const target = this.#pool.allocated(sid);
        const bits = frame >>> 0;
        // 실제 필드 폭 4바이트를 little endian 으로 쓴다.
        for (let byte = 0; byte < 4; byte += 1) target[SLOT_FRAME + byte] = (bits >>> (byte * 8)) & 0xff;
    }

    // 좌표별 중복 제거는 값 전체가 같을 때만 적용한다. (2026-11-11 WB04-E296)
    #addBoundary(x, y) {
        const list = this.#state.boundary;
        if (!list.some((point) => point.x === x && point.y === y)) list.push({ x, y });
    }

    // 표면 0/범위 밖 후보도 원본은 목록에 넣고 다음 stamp 에서 일반 finder 를 수행한다. 북·동·남·서 호출 순서를 보존한다. (2026-11-11 WB04-E296)
    #addNeighbors(x, y, connections) {
        // 원본 코드의 북·동·남·서 호출 순서를 보존한다.
        for (let direction = 0; direction < DIRECTIONS.length; direction += 1) {
            if ((connections & (1 << direction)) !== 0) this.#addBoundary(x + DIRECTIONS[direction].x, y + DIRECTIONS[direction].y);
        }
    }

    // 재귀의 가상 호출 순서를 유지하며 큰 실제 섬에서도 호스트 스택을 넘지 않게 한다: 북·동·남 재귀와 서쪽 tail loop 를 역순 push 로 재현한다. (2026-11-11 WB04-E296)
    #flood(x, y) {
        const state = this.#state;
        const work = [{ x, y }];
        // 먼저 호출할 이웃이 마지막 push 가 되도록 서·남·동·북 순서로 준비한다.
        while (work.length > 0) {
            const point = work.pop();
            const sid = this.#surface(point.x, point.y);
            if (sid === 0) {
                this.#addBoundary(point.x, point.y);
                continue;
            }
            const raw = this.#pool.slot(sid);
            if (raw[SLOT_TYPE] !== this.#terrainType || raw[SLOT_OWNER] === state.owner) continue;
            this.#hooks.setOwner(sid, state.owner);
            const code = this.#code(sid);
            const connections = connectionMask(String.fromCharCode(code.side));
            this.#writeFrame(sid, this.#select(sid, point.x, point.y, false));
            for (let direction = DIRECTIONS.length; direction > 0; direction -= 1) {
                if ((connections & (1 << (direction - 1))) !== 0) work.push({ x: point.x + DIRECTIONS[direction - 1].x, y: point.y + DIRECTIONS[direction - 1].y });
            }
        }
    }

    // 빈 표면의 edgeFarm fallback·영향 지역·추가 후보를 수행한다: 지역 기록에는 정수 좌표의 비트를 float 로 재해석한 값(xBits/yBits 는 원본 비트)과 지역 번호를 중복 지역 없이 넣는다. (2026-11-11 WB04-E296)
    #boundary(x, y) {
        const hooks = this.#hooks;
        let sid = this.#surface(x, y);
        if (sid === 0) {
            sid = hooks.boundaryAt(Math.fround(x), Math.fround(y), this.#edgeFarmType);
            if (sid === 0) return;
        }
        const region = hooks.regionAt(Math.fround(x), Math.fround(y));
        if (!this.#ownership.affected.some((value) => value.region === region)) {
            this.#ownership.affected.push({ x: bitsToFloat(x >>> 0), y: bitsToFloat(y >>> 0), xBits: x >>> 0, yBits: y >>> 0, region });
        }
        const raw = this.#pool.slot(sid);
        const type = raw[SLOT_TYPE];
        if (type === this.#terrainType && raw[SLOT_OWNER] === this.#state.owner) return;
        if (type !== this.#terrainType && type !== this.#edgeFarmType) return;
        hooks.setOwner(sid, this.#state.owner);
        const code = this.#code(sid);
        const connections = connectionMask(String.fromCharCode(code.side));
        this.#writeFrame(sid, this.#select(sid, x, y, true));
        // 경계도 직접 프레임 쓰기 뒤에 이웃 후보를 추가한다.
        this.#addNeighbors(x, y, connections);
    }

    // 실패한 시작 좌표에서도 원본 전역 월드 다시 그리기 증가를 생략하지 않는다. 요청은 `{ x, y, owner, theme }` 이다. (2026-11-11 WB04-E296)
    paint(request) {
        if (!request || !Number.isInteger(request.x) || !Number.isInteger(request.y) || !Number.isInteger(request.owner) || request.owner < 0 || request.owner > 0xffffffff
            || !Number.isInteger(request.theme) || request.theme < -0x80000000 || request.theme > 0xffffffff) {
            throw new TypeError('도장 요청은 정수 x/y/owner/theme 이어야 합니다.');
        }
        const state = this.#state;
        state.boundary.length = 0;
        state.bounds = [257, 257, -1, -1];
        const sid = this.#surface(request.x, request.y);
        if (sid !== 0 && this.#pool.slot(sid)[SLOT_TYPE] === this.#terrainType) {
            state.owner = request.owner >>> 0;
            state.theme = request.theme | 0;
            this.#flood(request.x, request.y);
        }
        // stamp 중 추가된 후보도 같은 호출에서 뒤이어 처리한다.
        for (let index = 0; index < state.boundary.length; index += 1) {
            const point = state.boundary[index];
            this.#boundary(point.x, point.y);
        }
        state.incrementWorldRedraw();
    }
}

// 기존 투표의 필수 도장 경계를 실제 몸체에 연결하고 같은 풀 여부를 확인한다. 반환 객체는 입력 훅 묶음에 `paint` 만 더한 사본이다. (2026-11-11 WB04-E296)
export function makeRegionPaintOwnershipHooks(pool, paint, hooks) {
    if (pool !== paint?.pool) throw new TypeError('지역 투표/도장 SID 풀이 다릅니다');
    return { ...hooks, paint: (request) => paint.paint(request) };
}
