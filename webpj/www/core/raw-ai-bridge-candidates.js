// 실제 AI 다리 탐색의 나선 24칸과 연결점 후보 6개를 같은 현재 월드에서 준비하고 시작점을 고르는 모듈이다(cpppj `o/RawAiBridgeCandidates.cpp/.h`, 10.78: 패치 0040fe90 후보 준비·0040faa0 열린 이웃·00410b10 시작점).
//  - new RawAiBridgeCandidates(memory, pool, types, frames, surfaces, spots, globals): memory 는 E164 `RawAiBrainMemory`, pool 은 같은 PATCH SidPool(실제 50바이트 표면 슬롯), types 는 현재 188 TYPE({ flags1 })의 같은 배열(graph getter 의 표면 flag),
//    frames 는 현재 188 프레임 코드 표(`FrameCodeTable`, 현재 raw 프레임의 방향 코드), surfaces 는 같은 hash0 의 65536 WORD 지도(Uint16Array), spots 는 같은 현재 점유 65536 BYTE, globals = { bridge()(005411a0 의 전체 DWORD 로 다리 여부 비교),
//    fencemark()(005411c0 의 낮은 BYTE, graph getter 의 예외 타입), windArcher()(00541288 의 낮은 BYTE, 두 번째 예외 타입) } 는 매 읽기 위치에서 현재 값을 읽는다.
//  - prepare(token): 0040fe90 — 한 호출에 나선 24칸을 처리하고 예산이 소진되면 false(상태 유지), 경계(10..246 밖)/나선 24단계/단계 8 이상 + 후보 3개 이상이면 true 이다. 현재 변의 각 칸에서 같은 graph/owner 표면이고 4방향 이웃 중 빈 칸이 있으면
//    북/동/남/서의 연결비트(원본 0052f910 A~P 표)가 다리 여부와 일치하고 그 이웃이 비어 있는 방향마다 최대 6개 후보(x, y, 방향 12바이트, +0x648)를 저장한다(count 증가 선행·DWORD 주소 감김). 나선 회전은 signed 나머지, 변 길이는 signed 산술 시프트이다.
//  - open(surface, x, y): 0040faa0 — 현재 코드의 연결/다리 구분에 맞는 빈 이웃이 있으면 같은 SID, 없으면 0 이다(지면의 owner/graph 는 호출자가 검사). start(token, target, bounds): 00410b10 — 현재 사각형을 X 바깥/Y 안쪽 순서로 훑어 현재 본체 graph/owner 가 같고
//    열린 표면 중 목표까지 거리(두 축 절댓값 합, INT_MIN 감김 포함)가 엄격히 최소인 첫 칸을 고르며 없으면 `{ -10000, -10000 }` 이다. 지도 경계를 clamp 하지 않는다.
//  - 협력자 접근(원본의 friend `RawAiBridgePlanning`): pool·surfaces·frames·bridgeType·field/setField/body/spot/graph/connections 로 같은 월드를 공유하며 별도 사본을 만들지 않는다.
// 10.72(CD)는 범위 밖이다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E176: AI 다리 후보 이식으로 추가)
import { AI_BRAIN_BYTES } from './raw-ai-brain.js';
import { NEIGHBOR_CELLS, connectionMask } from '../assets/terrain-mask.js';

// 원본 호출 예산·후보 용량·Brain+5f0+58 의 실제 후보 배열 시작, 부족한 시작점의 거리 초기값이다. (2026-11-09 WB04-E176)
const BUDGET = 24;
const CAPACITY = 6;
const CANDIDATES = 0x648;
const INITIAL_BEST = 65536;
const UNSET = -10000;
const TWO_63 = 9223372036854775808;

// DWORD 의 signed 해석이다. (2026-11-09 WB04-E176)
const signed = (value) => value | 0;

// 실제 __ftol 의 signed64 절삭 하위 DWORD(uint32)이며 indefinite 의 하위는 0 이다. (2026-11-09 WB04-E176)
function truncate(value) {
    if (!Number.isFinite(value) || value < -TWO_63 || value >= TWO_63) return 0;
    return Number(BigInt.asUintN(32, BigInt(Math.trunc(value))));
}

// 원본 xor/sub 의 INT_MIN 감김도 signed 최소 거리 비교에 그대로 전달한다. (2026-11-09 WB04-E176)
function absolute(value) {
    return signed(value) < 0 ? (0 - value) >>> 0 : value >>> 0;
}

// raw 슬롯/본체의 비정렬 DWORD 를 원본 little endian 으로 읽는다. (2026-11-09 WB04-E176)
function read(raw, offset) {
    if (offset < 0 || offset > raw.length || raw.length - offset < 4) throw new RangeError('AI 다리 후보 DWORD 읽기 범위');
    return (raw[offset] | (raw[offset + 1] << 8) | (raw[offset + 2] << 16) | (raw[offset + 3] << 24)) >>> 0;
}

// 다리 후보 준비/시작점 처리기이다. 실제 hash0/spot/프레임 코드만 읽고 후보 상태를 같은 본체에 갱신한다. (2026-11-09 WB04-E176)
export class RawAiBridgeCandidates {
    #memory;
    #pool;
    #types;
    #frames;
    #surfaces;
    #spots;
    #globals;

    // 원본 준비는 다른 판본의 슬롯/TYPE 배치에서 실행하지 않는다. (2026-11-09 WB04-E176)
    constructor(memory, pool, types, frames, surfaces, spots, globals) {
        if (!memory || typeof memory.bytes !== 'function') throw new TypeError('AI 다리 후보 본체 소유자 누락');
        if (pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length !== 188 || !Array.isArray(frames) || frames.length !== 188
            || !(surfaces instanceof Uint16Array) || surfaces.length !== 65536 || !(spots instanceof Uint8Array) || spots.length !== 65536) {
            throw new TypeError('AI 다리 후보의 실제 PATCH188/지도 저장소 누락');
        }
        for (const name of ['bridge', 'fencemark', 'windArcher']) {
            if (typeof globals?.[name] !== 'function') throw new TypeError(`AI 다리 후보 현재 전역 조회기 누락: ${name}`);
        }
        this.#memory = memory;
        this.#pool = pool;
        this.#types = types;
        this.#frames = frames;
        this.#surfaces = surfaces;
        this.#spots = spots;
        this.#globals = globals;
    }

    // 실제 Session 조립 시 다른 월드가 섞이지 않게 같은 풀을 확인할 수 있도록 제공한다. (2026-11-09 WB04-E176)
    get pool() { return this.#pool; }

    // 협력자(연결 계층)가 같은 hash0 지도/프레임 표/현재 다리 TYPE 를 읽도록 제공한다(사본이 아니다). (2026-11-09 WB04-E176)
    get surfaces() { return this.#surfaces; }
    get frames() { return this.#frames; }
    get bridgeType() { return this.#globals.bridge() >>> 0; }

    // 크기 없는 가짜 본체나 반납된 핸들을 처리하지 않는다. (2026-11-09 WB04-E176)
    body(token) {
        const raw = this.#memory.bytes(token);
        if (raw.length !== AI_BRAIN_BYTES) throw new RangeError('AI 다리 후보의 실제0xb08 본체 누락');
        return raw;
    }

    // 원본이 각 위치에서 다시 읽는 현재 필드를 캐시하지 않는다. (2026-11-09 WB04-E176)
    field(token, offset) { return read(this.body(token), offset >>> 0); }

    // 음수 후보 번호도 원본 DWORD 주소 감김 뒤 실제 본체 안이면 그 바이트를 쓴다(물리 본체 밖은 거부, 주변 필드 보존). (2026-11-09 WB04-E176)
    setField(token, offset, value) {
        const raw = this.body(token);
        offset >>>= 0;
        if (offset > raw.length || raw.length - offset < 4) throw new RangeError('AI 다리 후보 DWORD 쓰기 범위');
        // 지정 네 바이트만 쓴다.
        for (let index = 0; index < 4; index += 1) raw[offset + index] = ((value >>> 0) >>> (index * 8)) & 0xff;
    }

    // x255 의 오른쪽처럼 같은 flat 지도 안의 인접 주소는 원본대로 다음 행을 읽는다(65536 BYTE 밖은 거부). (2026-11-09 WB04-E176)
    spot(x, y) {
        const index = y * 256 + x;
        if (index < 0 || index >= this.#spots.length) throw new RangeError('AI 다리 후보 spot 물리 범위');
        return this.#spots[index];
    }

    // free/dead/void 나 소유자 필터를 getter 에 추가하지 않고 실제 4ad390 의 타입 조건과 raw graph BYTE 를 읽는다. (2026-11-09 WB04-E176)
    graph(sid) {
        const raw = this.#pool.slot(sid);
        const type = raw[10];
        if (type >= this.#types.length || (!((this.#types[type].flags1 >>> 0) & 0x800) && type !== (this.#globals.fencemark() & 0xff) && type !== (this.#globals.windArcher() & 0xff))) throw new Error('AI 다리 후보 graph getter 타입 오류');
        return raw[30];
    }

    // 현재 코드의 A~P 연결표는 지면 생성과 같은 원본 0052f910 을 공유한다(실제 4ad680). (2026-11-09 WB04-E176)
    connections(sid) {
        const raw = this.#pool.slot(sid);
        const type = raw[10];
        const frame = read(raw, 36);
        if (type < 70 || type >= this.#frames.length || frame >= this.#frames[type].codes.length) throw new RangeError('AI 다리 후보의 현재 프레임 범위');
        return connectionMask(String.fromCharCode(this.#frames[type].codes[frame].side));
    }

    // 0040faa0: 북/동/남/서 첫 열린 끝점만 확인하며 지면의 owner/graph 는 caller 가 검사한다(다리 TYPE 는 BYTE 로 축소하지 않는다). (2026-11-09 WB04-E176)
    open(surface, x, y) {
        const bridge = this.#pool.slot(surface)[10] === this.bridgeType;
        const connections = this.connections(surface);
        // 현재 코드의 각 cardinal 비트와 빈 이웃을 같은 순서로 검사한다.
        for (let direction = 0; direction < 8; direction += 2) {
            const [dx, dy] = NEIGHBOR_CELLS[direction];
            if (((connections & (1 << (direction / 2))) !== 0) === bridge && !this.spot(x + dx, y + dy)) return surface;
        }
        return 0;
    }

    // 00410b10: 현재 맵 경계를 clamp 하지 않으며 같은 거리에서는 먼저 발견한 X/Y 를 유지한다. (2026-11-09 WB04-E176)
    start(token, target, bounds) {
        this.body(token);
        const targetX = truncate(target.x);
        const targetY = truncate(target.y);
        let best = INITIAL_BEST;
        let result = { x: UNSET, y: UNSET };
        // 원본의 fild 비교는 float 저장 전 정수값이며 X 가 바깥 순회이다.
        for (let x = truncate(bounds.left); signed(x) < bounds.right; x = (x + 1) >>> 0) {
            // 각 X 마다 현재 위 경계를 다시 절삭하고 Y 를 올린다.
            for (let y = truncate(bounds.top); signed(y) < bounds.bottom; y = (y + 1) >>> 0) {
                const graph = this.field(token, 0x298);
                const owner = this.field(token, 0x260);
                if (signed(x) < 0 || signed(y) < 0 || x >= 256 || y >= 256) continue;
                const surface = this.#surfaces[y * 256 + x];
                if (!surface || this.graph(surface) !== graph || this.#pool.slot(surface)[34] !== owner
                    || (this.spot(signed(x) - 1, signed(y)) && this.spot(signed(x) + 1, signed(y)) && this.spot(signed(x), signed(y) - 1) && this.spot(signed(x), signed(y) + 1))) continue;
                const score = signed((absolute((targetX - x) >>> 0) + absolute((targetY - y) >>> 0)) >>> 0);
                if (score >= best || !this.open(surface, signed(x), signed(y))) continue;
                result = { x: signed(x), y: signed(y) };
                best = score;
            }
        }
        return result;
    }

    // helper 효과가 없는 실제 읽기 순서와 후보 기록 뒤 나선 진행을 보존한다. (2026-11-09 WB04-E176)
    prepare(token) {
        const graph = this.field(token, 0x5f8);
        const owner = this.field(token, 0x5fc);
        let budget = BUDGET;
        // 한 나선 변이 끝날 때만 바깥 종료 조건을 다시 검사한다.
        for (;;) {
            const x = signed(this.field(token, 0x6a0));
            const y = signed(this.field(token, 0x6a4));
            const stage = signed(this.field(token, 0x698));
            if (x <= 10 || y <= 10 || x >= 246 || y >= 246 || stage >= 24 || (stage >= 8 && signed(this.field(token, 0x690)) >= 3)) return true;
            // 현재 변의 나머지는 24칸 예산이 소진될 때까지 이어간다.
            while (this.field(token, 0x69c) !== 0) {
                if (!budget) return false;
                budget -= 1;
                const px = signed(this.field(token, 0x6a0));
                const py = signed(this.field(token, 0x6a4));
                let number = 0;
                if (px >= 0 && py >= 0 && px < 256 && py < 256) number = this.#surfaces[py * 256 + px];
                if (number) {
                    if (this.graph(number) === graph && this.#pool.slot(number)[34] === owner && (!this.spot(px - 1, py) || !this.spot(px + 1, py) || !this.spot(px, py - 1) || !this.spot(px, py + 1))) {
                        const bridge = this.#pool.slot(number)[10] === this.bridgeType;
                        const connections = this.connections(number);
                        // 원본은 북/동/남/서만 이 순서로 최대 6개 저장한다.
                        for (let direction = 0; direction < 8; direction += 2) {
                            const count = this.field(token, 0x690);
                            if (signed(count) >= CAPACITY) break;
                            const [dx, dy] = NEIGHBOR_CELLS[direction];
                            if (((connections & (1 << (direction / 2))) !== 0) === bridge && !this.spot(px + dx, py + dy)) {
                                this.setField(token, 0x690, (count + 1) >>> 0);
                                const at = (CANDIDATES + Math.imul(count, 12)) >>> 0;
                                this.setField(token, at, this.field(token, 0x6a0));
                                this.setField(token, (at + 4) >>> 0, this.field(token, 0x6a4));
                                this.setField(token, (at + 8) >>> 0, direction);
                            }
                        }
                    }
                }
                const direction = this.field(token, 0x694);
                if (direction >= NEIGHBOR_CELLS.length) throw new RangeError('AI 다리 후보 나선 방향 범위');
                const [dx, dy] = NEIGHBOR_CELLS[direction];
                this.setField(token, 0x6a0, (this.field(token, 0x6a0) + dx) >>> 0);
                this.setField(token, 0x6a4, (this.field(token, 0x6a4) + dy) >>> 0);
                this.setField(token, 0x69c, (this.field(token, 0x69c) - 1) >>> 0);
            }
            const next = (signed((this.field(token, 0x694) + 2) >>> 0) % 8) >>> 0;
            const stageBits = (this.field(token, 0x698) + 1) >>> 0;
            this.setField(token, 0x694, next);
            this.setField(token, 0x698, stageBits);
            this.setField(token, 0x69c, (signed(stageBits) >> 1) >>> 0);
        }
    }
}
