// 초기 terrain frame 선택과 실제 stamp 를 두 난수/현재 지도/같은 SID 풀에서 원본 순서로 실행하는 모듈이다(cpppj `o/RawTerrainStamp.cpp/.h`, 패치 004c04b0/004c0640/004bfe50, 10.78 전용).
// 사용: new RawTerrainStamp(pool, types, frames, context.map, gameRandom, crtBox, state, hooks).stamp(0x78) 은 지도의 Y 외부/X 내부 순서로 문자의 alias 가 요청 문자와 같은 칸마다
//   frameAt(x, y)로 프레임을 고르고 hooks.create → setOwner(0) → 프레임 직접 쓰기 → Pop → (지면 genus 면) 지역 설정과 fringe 생성의 원본 순서를 수행한다.
// 게임 객체 생성/Pop/지역 setter 는 월드의 실제 몸체를 hooks 로 받는다(create(type, predictable) → sid, setOwner(sid, owner), pop(sid, x, y, flags), setRegion(sid, region), chunkRegion(chunkX, chunkY),
//   assertion(종류)). 난수는 비-AA 칸과 fringe 검색이 쓰는 GameRandom 과 첫 프레임 요청에서 99번 소비하는 독립 CRT holdrand 상자(`{ value }`)이다.
// TerrainStampState.variants 는 전역(Client 수명) 99 캐시이다: 첫 frameAt 에서 한 번만 만들고 월드 교체 뒤에도 유지한다. 이 모듈은 DOM 을 쓰지 않는다. CD(10.72)의 BYTE frame/전체 후보 선택은 옮기지 않았다.
// (2026-11-11 WB04-E295: 4-B RawTerrainStamp 이식)
import { chooseTerrainFrame } from '../assets/terrain-tiles.js';
import { NEIGHBOR_CELLS, orientationLetter } from '../assets/terrain-mask.js';

// 원본 factory 결과의 Pop flags 이며 위치 0 입력/프로세스 생성 의미를 보존한다. (2026-11-11 WB04-E295)
const POP_FLAGS = 0x02000061;

// 실제 _rand 의 holdrand 곱셈/증가 값과 15비트 반환 mask 이다. (2026-11-11 WB04-E295)
const CRT_MULTIPLIER = 0x343fd;
const CRT_INCREMENT = 0x269ec3;
const CRT_MASK = 0x7fff;

// 북부터 시계방향 bit 표이다. 대각선은 북서 1/북동 2/남동 4/남서 8 이다. (2026-11-11 WB04-E295)
const NEIGHBOR_BITS = Object.freeze([1, 2, 2, 4, 4, 8, 8, 1]);

// 패치 판본의 TYPE 수, 99 캐시 크기, 한 변 칸 수, 안쪽 칸의 3×3 묶음 크기/변형 수이다. (2026-11-11 WB04-E295)
const TYPE_COUNT = 188;
const VARIANT_COUNT = 99;
const WORLD_SIDE = 256;
const CORE_SIZE = 3;
const CORE_VARIANTS = 4;

// raw 슬롯의 TYPE 위치, 프레임 DWORD 위치(패치 +36)이다. (2026-11-11 WB04-E295)
const SLOT_TYPE = 10;
const SLOT_FRAME = 36;

// 지도에서 쓰는 문자: 본섬 'x', 경계 'X', 중간 '='. (2026-11-11 WB04-E295)
const CODE_ISLAND = 0x78;
const CODE_EDGE = 0x58;
const CODE_MID_EQ = 0x3d;

// 원본의 두 assertion 지점이다. 사용: hooks.assertion(TerrainStampAssertion.MissingRegion). (2026-11-11 WB04-E295)
export const TerrainStampAssertion = Object.freeze({
    // 패치의 섬 genus 지면이 지역 127(noRegion)이면 현재 ChunkMap 조회 전에 보고한다.
    MissingRegion: 'MissingRegion',
    // fringe 생성 직전 makePredictable 이 0 이면 보고한다.
    UnpredictableFringe: 'UnpredictableFringe',
});

// 초기 stamp 만 사용하는 전역 99 캐시와 현재 특수 타입/원본 입력이다. 같은 월드의 RegionPaint 캐시와 섞지 않는다.
// variants: 첫 frameAt 에서 현재 CRT 상태를 99회 연속 소비해 만들고 월드 교체 뒤에도 보존(null 이면 미생성), islandType/edgeType/fringeType: 원본 기본 본섬(97)/경계(98)/장식(102) 타입,
// noRegion: 현재 ChunkMap 조회가 필요한 지역 없음 값(127), fringePrefixFlags: 검색 실패 -1 뒤 원본이 읽는 코드 배열 바로 앞 BYTE 의 명시 입력이다. (2026-11-11 WB04-E295)
export class TerrainStampState {
    variants = null;
    islandType = 97;
    edgeType = 98;
    fringeType = 102;
    noRegion = 127;
    fringePrefixFlags = 0;
}

// 프레임 flags BYTE 를 MSVC signed char → DWORD 로 부호 확장한다. (2026-11-11 WB04-E295)
function signedFlags(flags) {
    return ((flags << 24) >> 24) >>> 0;
}

// 실제 문자의 하위 BYTE 를 부호 있는 값으로 읽는다. 지도에 ASCII 밖 문자가 있으면 별도 raw 범위가 필요하므로 거부한다. (2026-11-11 WB04-E295)
function character(code) {
    if (code >= 0x80) throw new RangeError('초기stamp signed 문자mapping 범위 오류');
    return code;
}

// 지도 접근은 X 열 우선이고 유효 좌표를 밖에서 clamp 하지 않는다. (2026-11-11 WB04-E295)
function index(x, y) {
    if (!Number.isInteger(x) || !Number.isInteger(y) || x < 0 || x >= WORLD_SIDE || y < 0 || y >= WORLD_SIDE) throw new RangeError('초기stamp 지도 범위 오류');
    return x * WORLD_SIDE + y;
}

// 초기 terrain 의 프레임 선택과 stamp 실행기이다. pool/types/frames/map/random/crt/state 는 이 객체보다 오래 살고 같은 월드의 실제 저장소여야 한다. (2026-11-11 WB04-E295)
export class RawTerrainStamp {
    // 생성/owner/프레임/Pop 이 공유하는 raw 슬롯 풀, 현재 TYPE 표(flags2 필요), 188개 프레임 코드 표(FrameCodeTable), 현재 code/지역/mapping/bounds context 이다.
    #pool;
    #types;
    #frames;
    #map;
    // 비-AA 와 fringe 검색이 소비하는 실제 GameRandom, 초기 99 캐시가 소비하는 공유 CRT holdrand DWORD 상자(GameRandom 과 독립), 전역 수명의 캐시/특수 타입 상태, 필수 하위 효과이다.
    #random;
    #crt;
    #state;
    #hooks;

    // 모든 필수 연결(SID 풀 10.78·TYPE/프레임 188칸·지도 배열 크기·여섯 하위 효과)을 먼저 확인한다. (2026-11-11 WB04-E295)
    constructor(pool, types, frames, map, random, crt, state, hooks) {
        if (pool?.edition !== 'patch1078' || !Array.isArray(types) || types.length !== TYPE_COUNT || !Array.isArray(frames) || frames.length !== TYPE_COUNT
            || !map || map.codes?.length !== 65536 || map.regions?.length !== 65536 || map.typeMapping?.length !== 256 || map.aliases?.length !== 256 || !Array.isArray(map.bounds)
            || !random || typeof random.next !== 'function' || typeof crt?.value !== 'number' || !(state instanceof TerrainStampState)
            || ['create', 'setOwner', 'pop', 'setRegion', 'chunkRegion', 'assertion'].some((name) => typeof hooks?.[name] !== 'function')) {
            throw new TypeError('초기stamp 지도/타입/필수 효과 누락');
        }
        this.#pool = pool;
        this.#types = types;
        this.#frames = frames;
        this.#map = map;
        this.#random = random;
        this.#crt = crt;
        this.#state = state;
        this.#hooks = hooks;
    }

    // 이웃은 bounds 양 끝을 포함하고 같은 alias DWORD/지역 BYTE 일 때만 mask 에 들어간다. (2026-11-11 WB04-E295)
    #same(x, y, alias, region) {
        const bounds = this.#map.bounds;
        if (x < bounds[0] || y < bounds[1] || x > bounds[2] || y > bounds[3]) return false;
        const at = index(x, y);
        return this.#map.aliases[character(this.#map.codes[at])] === alias && this.#map.regions[at] === region;
    }

    // 단일 전역 캐시는 첫 프레임 요청에서만 현재 CRT 상태를 전진시킨다: 99개 연속 _rand 이고, 앞 원소와 signed mod 4 가 같으면 1 을 더한다. (2026-11-11 WB04-E295)
    #initialize() {
        if (this.#state.variants) return;
        const variants = new Int32Array(VARIANT_COUNT);
        this.#state.variants = variants;
        // 원본 99개 연속 _rand 와 앞 원소의 mod 4 비교 순서를 그대로 수행한다.
        for (let at = 0; at < VARIANT_COUNT; at += 1) {
            this.#crt.value = (Math.imul(this.#crt.value, CRT_MULTIPLIER) + CRT_INCREMENT) >>> 0;
            let value = (this.#crt.value >>> 16) & CRT_MASK;
            if (at && variants[at - 1] % 4 === value % 4) value += 1;
            variants[at] = value;
        }
    }

    // 직선/대각선 이웃은 북부터 시계방향 네 비트이고 캐시는 비-AA 에서도 초기화한다. 안쪽 칸('x' 이고 두 방향이 모두 'A')은 캐시의 3×3 묶음 프레임, 그 밖은 원소 0 의 후보 선택(후보 없음 -1 → 0)이다. (2026-11-11 WB04-E295)
    frameAt(x, y) {
        const at = index(x, y);
        const code = this.#map.codes[at];
        const char = character(code);
        const alias = (this.#map.aliases[char] << 24) >> 24;
        const region = this.#map.regions[at];
        let straight = 0;
        let diagonal = 0;
        // 짝수는 직선/홀수는 대각선이며 실제 이웃별 bit 표로 두 mask 를 각각 누적한다.
        for (let neighbor = 0; neighbor < NEIGHBOR_CELLS.length; neighbor += 1) {
            const [dx, dy] = NEIGHBOR_CELLS[neighbor];
            if (this.#same(x + dx, y + dy, alias, region)) {
                if (neighbor % 2) diagonal |= NEIGHBOR_BITS[neighbor];
                else straight |= NEIGHBOR_BITS[neighbor];
            }
        }
        const type = this.#map.typeMapping[char];
        if (type < 0 || type >= this.#frames.length) throw new RangeError('초기stamp frame 타입 범위 오류');
        const side = orientationLetter(straight);
        const variant = orientationLetter(diagonal);
        this.#initialize();
        let frame;
        if (code === CODE_ISLAND && side === 'A' && variant === 'A') {
            const values = this.#state.variants;
            const sum = (values[Math.trunc(x / CORE_SIZE) % VARIANT_COUNT] + values[Math.trunc(y / CORE_SIZE) % VARIANT_COUNT]) % VARIANT_COUNT;
            if (sum < 0) throw new RangeError('초기stamp cache 합계 범위 오류');
            const chosen = (values[sum] + Math.trunc(y / CORE_SIZE)) % CORE_VARIANTS;
            frame = this.#frames[type].run('J').first + 1 + ((y % CORE_SIZE) + chosen * CORE_SIZE) * CORE_SIZE + (x % CORE_SIZE);
        } else {
            frame = chooseTerrainFrame(this.#frames[type], side, variant, 0, this.#random);
        }
        return frame === -1 ? 0 : frame;
    }

    // 원본 직접 대입 폭(패치 DWORD)이며 SetFrame 부수 효과를 만들지 않는다. (2026-11-11 WB04-E295)
    #writeFrame(sid, frame) {
        const raw = this.#pool.allocated(sid);
        const bits = frame >>> 0;
        // 실제 필드 폭의 바이트만 little endian 으로 쓴다.
        for (let byte = 0; byte < 4; byte += 1) raw[SLOT_FRAME + byte] = (bits >>> (byte * 8)) & 0xff;
    }

    // factory 뒤 현재 타입을 캡처하고 Pop 이후 현재 지역/frame flag 를 조회해 fringe 를 원본 순서로 만든다. (2026-11-11 WB04-E295)
    #create(x, y, type, frame, region) {
        const hooks = this.#hooks;
        const sid = hooks.create(type, this.#map.makePredictable);
        this.#map.lastSid.value = sid;
        hooks.setOwner(sid, 0);
        this.#writeFrame(sid, frame);
        const actualType = this.#pool.slot(sid)[SLOT_TYPE];
        if (actualType >= this.#types.length) throw new RangeError('초기stamp 생성 타입 범위 오류');
        hooks.pop(sid, Math.fround(x), Math.fround(y), POP_FLAGS);
        if (!(this.#types[actualType].flags2 & 2)) return;
        let current = region;
        if (current === this.#state.noRegion) {
            hooks.assertion(TerrainStampAssertion.MissingRegion);
            current = hooks.chunkRegion(Math.trunc(x / 16), Math.trunc(y / 16));
        }
        hooks.setRegion(sid, current);
        const codes = this.#frames[actualType].codes;
        if (frame < 0 || frame >= codes.length) throw new RangeError('초기stamp 생성 프레임 범위 오류');
        const code = codes[frame];
        if (!(code.flags & 1)) return;
        if (this.#state.fringeType >= this.#frames.length) throw new RangeError('초기stamp fringe 타입 범위 오류');
        const fringeFrames = this.#frames[this.#state.fringeType];
        let selected;
        // 원본은 후보 flags 를 -1 판정 전에 읽으며 제외 flags 가 겹치면 난수를 다시 소비한다.
        for (;;) {
            selected = chooseTerrainFrame(fringeFrames, String.fromCharCode(code.side), String.fromCharCode(code.variant), -1, this.#random);
            const flags = selected === -1 ? this.#state.fringePrefixFlags : fringeFrames.codes[selected].flags;
            if (!(this.#map.excludedFlags & signedFlags(flags))) break;
        }
        if (selected === -1) return;
        if (!this.#map.makePredictable) hooks.assertion(TerrainStampAssertion.UnpredictableFringe);
        const fringe = hooks.create(this.#state.fringeType, this.#map.makePredictable);
        this.#writeFrame(fringe, selected);
        hooks.pop(fringe, Math.fround(x), Math.fround(Math.fround(y) + 4), POP_FLAGS);
    }

    // 입력 문자별 alias 를 현재 읽고 실제 mapping 세 항목을 생성 직전에 다시 지정한다. Y 외부/X 내부 반열린 순서가 난수와 factory/Pop 호출 순서를 결정한다. (2026-11-11 WB04-E295)
    stamp(code) {
        if (!Number.isInteger(code) || code < -128 || code > 127) throw new TypeError('stamp 의 code 는 signed 문자 정수여야 합니다.');
        const map = this.#map;
        // Y 외부/X 내부 반열린 순서이다.
        for (let y = map.bounds[1]; y < map.bounds[3]; y += 1) {
            // 각 행의 left/right 도 현재 context 에서 다시 읽는다.
            for (let x = map.bounds[0]; x < map.bounds[2]; x += 1) {
                const at = index(x, y);
                const char = character(map.codes[at]);
                if (x < map.bounds[0] || y < map.bounds[1] || x > map.bounds[2] || y > map.bounds[3] || map.aliases[char] !== code) continue;
                map.typeMapping[CODE_ISLAND] = this.#state.islandType | 0;
                map.typeMapping[CODE_EDGE] = this.#state.edgeType | 0;
                map.typeMapping[CODE_MID_EQ] = this.#state.islandType | 0;
                const type = map.typeMapping[char];
                if (!type) continue;
                const region = (map.regions[at] << 24) >> 24;
                const frame = this.frameAt(x, y);
                this.#create(x, y, type >>> 0, frame, region);
            }
        }
    }

    // 연결하려는 월드의 같은 pool 인지 확인할 때 현재 저장소를 반환한다. (2026-11-11 WB04-E295)
    get pool() {
        return this.#pool;
    }
}
