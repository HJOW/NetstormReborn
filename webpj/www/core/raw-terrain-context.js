// 실제 초기 지형 constructor 의 지도·문자 대응·청크/칸 bounds 와 도장 앞뒤 문자 변환을 보관한 모듈이다(cpppj `o/RawTerrainContext.cpp/.h`, 패치 004c0390/004c09a0/004c0ad0, 10.78 전용).
// 사용: new RawTerrainContext('patch1078', owner, region, { left: 1, top: 1, right: 15, bottom: 15 }, new TerrainContextGlobals(), (assertion) => { ... }) 로 만든 뒤
//   map 이 돌려주는 지도(codes/regions/typeMapping/aliases/bounds/makePredictable/lastSid/excludedFlags)를 초기 stamp·성장이 같은 저장소로 읽고 쓴다.
//   markEdges(code, edge)는 여덟 이웃의 경계·alias·지역이 하나라도 다른 칸을 edge 문자로 바꾸고, normalizeAfterStamp()는 도장 뒤 `=`/`o`/`X` 문자를 `x` 로 바꾼다(지역 BYTE 는 보존).
// 지도는 원본 X 열 우선 65,536BYTE 두 배열이다(인덱스 = x * 256 + y). 렌더용 Y 행 자료는 호출자가 명시적으로 전치해야 한다. 이 모듈은 DOM 을 쓰지 않는다.
// 10.78 만 대상이므로 CD(10.72)의 "보고 없이 청크 값만 줄이는" 경계는 옮기지 않고 다른 판본 문자열은 거부한다. (2026-11-11 WB04-E290: 4-B RawTerrainContext 이식)
import { NEIGHBOR_CELLS } from '../assets/terrain-mask.js';

// 한 변 칸 수와 지도 한 배열의 BYTE 수, 한 청크의 칸 수 변환(왼쪽 4비트 이동), 청크 지도의 최대 크기이다. (2026-11-11 WB04-E290)
const WORLD_SIDE = 256;
const WORLD_BYTES = WORLD_SIDE * WORLD_SIDE;
const CELL_SHIFT = 4;
const MAX_CHUNK = 16;

// 지도에서 쓰는 특수 문자(ASCII)이다: 허공 '.', 본섬 'x', 경계 'X', 도장 중간 'o', 도장 중간 '='. (2026-11-11 WB04-E290)
const CODE_VOID = 0x2e;
const CODE_ISLAND = 0x78;
const CODE_EDGE = 0x58;
const CODE_MID_O = 0x6f;
const CODE_MID_EQ = 0x3d;

// 전투 화면에서 켜는 fringe 제외 flags(밝지 않은 쪽 0x10)와 소유자 0 일 때 쓰는 flags(0x18)이다. (2026-11-11 WB04-E290)
const EXCLUDE_BATTLE = 0x10;
const EXCLUDE_OWNER_ZERO = 0x18;

// 패치 bounds 저장 함수의 두 assertion 원인이다. 사용: diagnostic(TerrainContextAssertion.RightChunkLimit). (2026-11-11 WB04-E290)
export const TerrainContextAssertion = Object.freeze({
    // 입력 right 가 최대 청크 16 보다 크다.
    RightChunkLimit: 'RightChunkLimit',
    // 현재 저장한 bottom 이 최대 청크 16 보다 크다.
    BottomChunkLimit: 'BottomChunkLimit',
});

// int32 범위 정수인지 확인한다(암묵 형변환·반올림을 하지 않음). (2026-11-11 WB04-E290)
function isInt32(value) {
    return Number.isInteger(value) && value >= -0x80000000 && value <= 0x7fffffff;
}

// uint32 범위 정수인지 확인한다. (2026-11-11 WB04-E290)
function isUint32(value) {
    return Number.isInteger(value) && value >= 0 && value <= 0xffffffff;
}

// constructor 가 읽는 원본 mutable 타입/모드 전역이다. 사용: new TerrainContextGlobals({ islandType: 97, edgeType: 98, noRegion: 127, battle: false }).
// islandType/edgeType 는 `x`·`=`/`X` 문자의 초기 지면 타입 번호, noRegion 은 초기 지역 BYTE 배열을 채우는 현재 지역 없음 값(하위 BYTE 만 사용), battle 은 fringe 제외 flags 0x10 을 켜는 현재 전투 화면 상태이다. (2026-11-11 WB04-E290)
export class TerrainContextGlobals {
    islandType;
    edgeType;
    noRegion;
    battle;

    constructor({ islandType = 97, edgeType = 98, noRegion = 127, battle = false } = {}) {
        if (!isUint32(islandType) || !isUint32(edgeType) || !isInt32(noRegion) || typeof battle !== 'boolean') {
            throw new TypeError('초기 지형 context 전역은 uint32 타입 두 개와 int32 noRegion 과 boolean battle 이어야 합니다.');
        }
        this.islandType = islandType;
        this.edgeType = edgeType;
        this.noRegion = noRegion;
        this.battle = battle;
    }
}

// 초기 지형의 code/region 지도와 문자 대응·bounds·이웃 분류를 원본 순서대로 구성한다. 지도/대응표는 이 객체가 소유하며 map 에 담긴 배열의 수명은 이 객체와 같다. (2026-11-11 WB04-E290)
export class RawTerrainContext {
    // 생성 요청의 현재 플레이어·대상 지역이다(후속 wrapper 는 현재 값을 읽는다).
    #owner;
    #region;
    // 제한 후 청크 범위이며 map.bounds 의 칸 범위와 합치지 않는다.
    #chunks;
    // 군도 root 가 켜는 원본 DWORD 를 `{ value }` 참조로 공유한다(생성 시 0).
    #archipelago;
    // 원본 X 열 우선 두 65,536BYTE 지도와 문자별 256 DWORD 대응표이다.
    #codes = new Uint8Array(WORLD_BYTES);
    #regions = new Uint8Array(WORLD_BYTES);
    #mapping = new Int32Array(256);
    #aliases = new Int32Array(256);
    // 위 실제 저장소를 초기 stamp·성장과 공유하는 context 이다.
    #map;
    // 패치가 bounds 오류를 보고한 뒤 계속 진행할 때 호출하는 필수 경계이다.
    #diagnostic;

    // 패치 004c0390 의 실제 기본 배열·특수 mapping/alias 와 bounds 를 만든다. edition 은 'patch1078' 만 허용한다.
    // owner 는 uint32, region 과 bounds 네 값은 int32 이고 diagnostic 은 필수 함수이다. (2026-11-11 WB04-E290)
    constructor(edition, owner, region, bounds, globals, diagnostic) {
        if (edition !== 'patch1078') throw new TypeError('초기 지형 context 는 10.78(patch1078) 판본만 지원합니다.');
        if (typeof diagnostic !== 'function') throw new TypeError('초기 지형 context 의 bounds 진단 연결 누락');
        if (!isUint32(owner) || !isInt32(region) || !(globals instanceof TerrainContextGlobals)) {
            throw new TypeError('초기 지형 context 의 owner(uint32)/region(int32)/전역 종류 오류');
        }
        this.#owner = owner;
        this.#region = region;
        this.#diagnostic = diagnostic;
        this.#archipelago = RawTerrainContext.#makeArchipelagoBox();
        this.#codes.fill(CODE_VOID);
        this.#regions.fill(globals.noRegion & 0xff);
        this.#mapping[CODE_VOID] = 0;
        this.#mapping[CODE_ISLAND] = globals.islandType | 0;
        this.#mapping[CODE_EDGE] = globals.edgeType | 0;
        this.#mapping[CODE_MID_O] = 0;
        this.#mapping[CODE_MID_EQ] = globals.islandType | 0;
        this.#aliases[CODE_ISLAND] = CODE_ISLAND;
        this.#aliases[CODE_EDGE] = CODE_ISLAND;
        this.#aliases[CODE_MID_O] = CODE_ISLAND;
        this.#aliases[CODE_MID_EQ] = CODE_ISLAND;
        this.#map = {
            codes: this.#codes,
            regions: this.#regions,
            typeMapping: this.#mapping,
            aliases: this.#aliases,
            bounds: [0, 0, 0, 0],
            makePredictable: 1,
            lastSid: { value: 0 },
            excludedFlags: globals.battle ? EXCLUDE_BATTLE : 0,
        };
        if (!owner) this.#map.excludedFlags = EXCLUDE_OWNER_ZERO;
        this.setBounds(bounds);
    }

    // 군도 DWORD 상자이다: 값은 항상 uint32 로 감겨 저장되고 읽기/쓰기가 같은 상자에 전파된다. (2026-11-11 WB04-E290)
    static #makeArchipelagoBox() {
        let stored = 0;
        return Object.freeze({
            get value() { return stored; },
            set value(next) {
                if (typeof next !== 'number') throw new TypeError('군도 DWORD 는 수여야 합니다.');
                stored = next >>> 0;
            },
        });
    }

    // 칸 bounds 를 먼저 16 배 저장(int32 감김)한 뒤 패치 진단과 청크 right/bottom 의 최대 16 제한을 적용한다.
    // bounds 는 `{ left, top, right, bottom }` 청크 정수이며 map.bounds 는 같은 배열 객체를 제자리에서 갱신한다. (2026-11-11 WB04-E290)
    setBounds(bounds) {
        if (!bounds || !isInt32(bounds.left) || !isInt32(bounds.top) || !isInt32(bounds.right) || !isInt32(bounds.bottom)) {
            throw new TypeError('초기 지형 context bounds 는 int32 청크 네 값이어야 합니다.');
        }
        this.#chunks = { left: bounds.left, top: bounds.top, right: bounds.right, bottom: bounds.bottom };
        const cellBounds = this.#map.bounds;
        cellBounds[0] = (bounds.left << CELL_SHIFT) | 0;
        cellBounds[1] = (bounds.top << CELL_SHIFT) | 0;
        cellBounds[2] = (bounds.right << CELL_SHIFT) | 0;
        cellBounds[3] = (bounds.bottom << CELL_SHIFT) | 0;
        if (bounds.right > MAX_CHUNK) this.#diagnostic(TerrainContextAssertion.RightChunkLimit);
        if (this.#chunks.bottom > MAX_CHUNK) this.#diagnostic(TerrainContextAssertion.BottomChunkLimit);
        if (this.#chunks.right > MAX_CHUNK) this.#chunks.right = MAX_CHUNK;
        if (this.#chunks.bottom > MAX_CHUNK) this.#chunks.bottom = MAX_CHUNK;
    }

    // 저장 범위를 넘으면 원본 자료 손상으로 거부한다(가장자리로 끌어당기지 않음). (2026-11-11 WB04-E290)
    static #index(x, y) {
        if (!Number.isInteger(x) || !Number.isInteger(y) || x < 0 || x >= WORLD_SIDE || y < 0 || y >= WORLD_SIDE) {
            throw new RangeError('초기 지형 context 지도 범위 오류');
        }
        return x * WORLD_SIDE + y;
    }

    // signed char 음수 주소는 현재 ASCII 대응표의 정상 입력이 아니므로 거부한다. (2026-11-11 WB04-E290)
    static #character(value) {
        if (value >= 0x80) throw new RangeError('초기 지형 context signed 문자 주소 범위 오류');
        return value;
    }

    // 원본 signed char 로 읽은 BYTE 값이다(0x80 이상은 음수). (2026-11-11 WB04-E290)
    static #signed(value) {
        return (value << 24) >> 24;
    }

    // 004c09a0: 여덟 이웃의 bounds·alias·지역 중 하나라도 다르면 현재 code 를 edge 로 바꾼다. 이웃 판정은 반열린 순회 bounds 와 달리 right/bottom 칸도 포함한다.
    // code 는 비교할 signed 문자 정수, edge 는 0..255 BYTE 이다. (2026-11-11 WB04-E290)
    markEdges(code, edge) {
        if (!isInt32(code) || !Number.isInteger(edge) || edge < 0 || edge > 0xff) throw new TypeError('markEdges 의 code(int32)/edge(BYTE) 오류');
        const bounds = this.#map.bounds;
        // 원본의 Y 외부/X 내부 순서로 앞에서 바뀐 문자도 alias 를 통해 후행 판정에 반영한다.
        for (let y = bounds[1]; y < bounds[3]; y += 1) {
            // 각 행의 현재 X 범위를 읽는다.
            for (let x = bounds[0]; x < bounds[2]; x += 1) {
                const index = RawTerrainContext.#index(x, y);
                if (RawTerrainContext.#signed(this.#codes[index]) !== code) continue;
                // 북부터 시계방향 실제 여덟 이웃에서 첫 불일치만 처리한다.
                for (const [dx, dy] of NEIGHBOR_CELLS) {
                    const nx = x + dx;
                    const ny = y + dy;
                    if (nx < bounds[0] || ny < bounds[1] || nx > bounds[2] || ny > bounds[3]) {
                        this.#codes[index] = edge;
                        break;
                    }
                    const neighbor = RawTerrainContext.#index(nx, ny);
                    if (this.#aliases[RawTerrainContext.#character(this.#codes[neighbor])] !== RawTerrainContext.#signed(this.#codes[index])
                        || this.#regions[neighbor] !== this.#regions[index]) {
                        this.#codes[index] = edge;
                        break;
                    }
                }
            }
        }
    }

    // 004c0ad0: 도장 뒤 현재 `=`/`o`/`X` 문자를 `x` 로 바꾸며 지역 BYTE 와 다른 문자는 변경하지 않는다. (2026-11-11 WB04-E290)
    normalizeAfterStamp() {
        const bounds = this.#map.bounds;
        // 도장의 같은 반열린 Y 외부/X 내부 bounds 를 순회한다.
        for (let y = bounds[1]; y < bounds[3]; y += 1) {
            // 원본의 현재 X 범위를 각 행에서 다시 읽는다.
            for (let x = bounds[0]; x < bounds[2]; x += 1) {
                const index = RawTerrainContext.#index(x, y);
                const code = this.#codes[index];
                if (code === CODE_MID_EQ || code === CODE_MID_O || code === CODE_EDGE) this.#codes[index] = CODE_ISLAND;
            }
        }
    }

    // 복사한 지도 대신 현재 도장 context 의 참조를 반환한다(초기 지면 성장/초기 stamp 가 같은 배열을 쓰게 함). (2026-11-11 WB04-E290)
    get map() { return this.#map; }

    // 성장/경계 처리가 같은 X 열 우선 배열에 쓰게 하는 문자/지역 배열이다. (2026-11-11 WB04-E290)
    get codes() { return this.#codes; }

    get regions() { return this.#regions; }

    // 문자 대응표를 별도 사본으로 만들지 않는다(초기화/편집 helper 가 같은 DWORD 를 갱신). (2026-11-11 WB04-E290)
    get aliases() { return this.#aliases; }

    // 칸 bounds 와 별도로 보존하는 제한 후 청크 범위의 사본이다(호출자가 바꿔도 내부 값은 그대로). (2026-11-11 WB04-E290)
    chunks() {
        return { ...this.#chunks };
    }

    // 원본 context 의 생성 요청 소유자/대상 지역이다. 후속 wrapper 는 현재 값을 읽어야 한다. (2026-11-11 WB04-E290)
    get owner() { return this.#owner; }

    get region() { return this.#region; }

    // 청크 getter/첫 시드의 패치 진단을 고르는 판본이다(웹은 항상 patch1078). (2026-11-11 WB04-E290)
    get edition() { return 'patch1078'; }

    // 군도 root 가 켜는 원본 DWORD 를 성장과 Put 이 같은 현재 값으로 읽도록 공유하는 `{ value }` 상자이다(3.6절 ③ 의 기준선 ArchipelagoMode() 접근자). (2026-11-11 WB04-E290)
    sharedArchipelagoMode() {
        return this.#archipelago;
    }
}
