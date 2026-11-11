// 실제 일반 초기 지형의 통로·청크별 성장·빈틈 보정과 군도 root 가 공유하는 성장/제거 helper 를 현재 owned context 에 연결한 모듈이다(cpppj `o/RawTerrainGrowth.cpp/.h`, 패치 004c10a0, 10.78 전용).
// 사용: new RawTerrainGrowth(context, chunks, globals, { checking: () => 0, assertion: (expression, file, line) => {} }).runNormal() 은 Y 외부/X 내부 통로 → 현재 전체 청크 성장 → 두 번 빈틈 보정을
//   `context.codes`/`context.regions`(X 열 우선 65,536BYTE)에 직접 수행한다. 게임 객체 생성은 뒤의 RawTerrainStamp 몫이다.
// 시드는 `{ value }`(uint32) 상자이다(원본 caller 로컬 시드 참조). 게임 전역 난수를 소비하지 않고 청크 지역 난수(`LocalRandom` 과 같은 식)만 쓴다.
// 진단: 첫 시드/청크 getter 의 원문 assertion 만 `diagnostics.assertion(표현식, 원본 파일, 행)` 으로 보고한 뒤 정상 복귀한다. 정상 입력은 진단 장치를 요구하지 않는다.
//   `diagnostics.checking` 은 원문 005e4794 의 현재 DWORD 를 호출마다 읽는 함수이다(없으면 0). CD(10.72) 의 noRegion 진입 캐시와 무진단 경계는 옮기지 않는다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-11 WB04-E292: 4-B RawTerrainGrowth 이식)
import { ChunkMap } from '../assets/territory.js';
import { LocalRandom, NEIGHBOR_CELLS, connectionMask } from '../assets/terrain-mask.js';
import { RawTerrainContext, TerrainContextGlobals } from './raw-terrain-context.js';

// 청크 방문 번호가 시드에 더해지는 증가량과 성장 연속 실패 한계이다(원본 004c117e·004c0800). (2026-11-11 WB04-E292)
const CHUNK_SEED_STEP = 0x10e3;
const GROWTH_FAILURES = 1000;

// 한 변 칸 수(지도 저장 범위), 한 청크의 칸 수, 청크 지도 한 변, 섬 방문 배열 크기이다. (2026-11-11 WB04-E292)
const WORLD_SIDE = 256;
const CHUNK_SIDE = 16;
const ISLAND_LIMIT = 100;

// 지도에서 쓰는 문자: 본섬 'x', 허공 '.'. (2026-11-11 WB04-E292)
const CODE_ISLAND = 0x78;
const CODE_VOID = 0x2e;

// 첫 시드가 실패했을 때의 고정 반환 값과 두 진단의 원문 위치이다. (2026-11-11 WB04-E292)
const SEED_FALLBACK = 0x2333;
const CHUNKMAP_FILE = '\\Ns\\O\\Chunkmap.cpp';
const TERRAINBUILDER_FILE = '\\Ns\\O\\Terrainbuilder.cpp';

// BYTE 를 원본 signed char 로 읽는다. (2026-11-11 WB04-E292)
function signed(value) {
    return (value << 24) >> 24;
}

// int32 정수인지 확인한다(암묵 형변환을 하지 않음). (2026-11-11 WB04-E292)
function isInt32(value) {
    return Number.isInteger(value) && value >= -0x80000000 && value <= 0x7fffffff;
}

// 초기 지형 성장 한 벌이다. context/chunks/globals 는 이 객체보다 오래 살아야 하며 지도 사본을 만들지 않는다. (2026-11-11 WB04-E292)
export class RawTerrainGrowth {
    // 현재 code/region/bounds 를 쓰는 owned context, 원본 12바이트 항목을 읽는 현재 청크 지도, 선택 지역/빈 청크 비교가 읽는 현재 noRegion 전역, 선택 진단이다.
    #context;
    #chunks;
    #globals;
    #diagnostics;

    // 진단은 `{ checking?, assertion? }` 이다. constructor 는 지도/난수를 바꾸지 않는다. (2026-11-11 WB04-E292)
    constructor(context, chunks, globals, diagnostics = {}) {
        if (!(context instanceof RawTerrainContext) || !(chunks instanceof ChunkMap) || !(globals instanceof TerrainContextGlobals)
            || diagnostics === null || typeof diagnostics !== 'object'
            || (diagnostics.checking !== undefined && typeof diagnostics.checking !== 'function')
            || (diagnostics.assertion !== undefined && typeof diagnostics.assertion !== 'function')) {
            throw new TypeError('초기 지형 성장의 context/청크 지도/전역/진단 종류 오류');
        }
        this.#context = context;
        this.#chunks = chunks;
        this.#globals = globals;
        this.#diagnostics = diagnostics;
    }

    // 원문 전체 DWORD 를 호출 시점마다 읽는 checking 관문이다(함수가 없으면 독립 입력의 0). (2026-11-11 WB04-E292)
    #checking() {
        return this.#diagnostics.checking ? (this.#diagnostics.checking() | 0) !== 0 : false;
    }

    // 첫 시드/청크 getter 경계에서만 원문 전체 진단 인자를 전달한다. 장치가 없으면 무진단 성공으로 숨기지 않고 거부한다. (2026-11-11 WB04-E292)
    #assert(expression, file, line) {
        if (!this.#diagnostics.assertion) throw new Error('초기 지형 시드 진단 장치 누락');
        this.#diagnostics.assertion(expression, file, line);
    }

    // 패치 getter 의 mode/범위 진단 뒤 같은 현재 청크를 당겨서 조회한다. 첫 진단의 효과로 mode/청크가 바뀌어도 뒤의 검사와 getter 는 현재 저장소를 다시 읽는다. (2026-11-11 WB04-E292)
    #seedChunkAt(x, y) {
        if (!this.#chunks.mode) this.#assert('isValid()', CHUNKMAP_FILE, 0xb7);
        if (x < 0 || y < 0 || x >= this.#chunks.xLength || y >= this.#chunks.yLength) {
            this.#assert('cc.x>=0 && cc.y>=0 && cc.x<xLen && cc.y<yLen', CHUNKMAP_FILE, 0xb8);
        }
        return this.#chunks.at(x, y);
    }

    // 지도 저장 범위를 검사하고 X 열 우선 주소를 얻는다. 손상 좌표를 가장자리로 당기지 않는다. (2026-11-11 WB04-E292)
    static #index(x, y) {
        if (!Number.isInteger(x) || !Number.isInteger(y) || x < 0 || y < 0 || x >= WORLD_SIDE || y >= WORLD_SIDE) throw new RangeError('초기 성장 지도 범위 오류');
        return x * WORLD_SIDE + y;
    }

    // 원본 청크 BYTE 를 signed 로 읽는다(진단 없음, 가장자리 당김 있음). 현재 섬 목록의 존재 조회와 별개이다. (2026-11-11 WB04-E292)
    #chunkRegion(x, y) {
        return signed(this.#chunks.at(x, y).island);
    }

    // 004c0e20: 첫 비 noRegion 청크의 raw 항목 +5(record[3])를 반환하고, 모두 비어 있으면(checking 켜짐에서 진단 뒤) 0x2333 을 반환한다.
    // 패치는 각 getter 의 효과 뒤 현재 전체 noRegion DWORD 와 비교한다. (2026-11-11 WB04-E292)
    firstChunkSeed() {
        // 원본 청크 범위는 Y 행이 외부이며, 진단 복귀 뒤 right/bottom 을 다시 읽는다.
        for (let y = this.#context.chunks().top; y < this.#context.chunks().bottom; y += 1) {
            // 현재 행의 left 에서 시작하고 첫 비어 있지 않은 raw 청크만 반환한다.
            for (let x = this.#context.chunks().left; x < this.#context.chunks().right; x += 1) {
                const chunk = this.#seedChunkAt(x, y);
                if (signed(chunk.island) !== this.#globals.noRegion) return chunk.record[3];
            }
        }
        if (this.#checking()) this.#assert('0', TERRAINBUILDER_FILE, 0x25f);
        return SEED_FALLBACK;
    }

    // 004bca20: 상태가 0 이면 0x0bad0bad 로 바꾸고 `상태 × 0x10003 + 3` 의 상위 16비트를 limit 로 나눈 나머지를 돌려준다. 시드 상자의 uint32 가 갱신된다. (2026-11-11 WB04-E292)
    #next(seed, limit) {
        const random = new LocalRandom(seed.value);
        const result = random.next(limit);
        seed.value = random.state;
        return result;
    }

    // limit<=0 에서 시드도 그대로인 원문 range helper 이다: width = high - low (uint32 감김 뒤 int32) 가 1 미만이면 low 를 그대로 돌려주고 시드를 소비하지 않는다.
    // 군도 root 도 같은 로컬 시드와 비양수 폭 계약으로 두 번 밀도를 추출한다. (2026-11-11 WB04-E292)
    nextRange(seed, low, high) {
        RawTerrainGrowth.#assertSeed(seed);
        if (!isInt32(low) || !isInt32(high)) throw new TypeError('nextRange 의 low/high 는 int32 정수여야 합니다.');
        const width = (high - low) | 0;
        if (width < 1) return low;
        return (low + this.#next(seed, width)) | 0;
    }

    // 패치 float32 → ftol 하위 DWORD 의 0 방향 나눗셈으로 칸 좌표의 청크 번호를 얻는다(큰 값은 float32 정밀도로 먼저 줄어든다). (2026-11-11 WB04-E292)
    static #floatChunk(value) {
        return Math.trunc((Math.trunc(Math.fround(value)) | 0) / CHUNK_SIDE) | 0;
    }

    // 시드 상자 종류를 검사한다. (2026-11-11 WB04-E292)
    static #assertSeed(seed) {
        if (!seed || typeof seed.value !== 'number') throw new TypeError('시드는 { value } 상자여야 합니다.');
    }

    // 004c13f0: 첫 시드 × 0x5d 의 로컬 난수로 count 개의 성공을 흩뿌린다(`x`, 지역 0). 일반 RunNormal 에는 호출하지 않는 군도 상위의 별도 몸체이며, 거부된 칸은 count 를 줄이지 않는다.
    // 원문에 없는 재시도 제한이나 중복 제거는 넣지 않는다(중복도 성공). 음수 count 는 원문에서 약 40억 회 도는 입력이라 거부한다. (2026-11-11 WB04-E292)
    scatter(count) {
        if (!isInt32(count) || count < 0) throw new RangeError('scatter 의 count 는 0 이상 int32 정수여야 합니다.');
        const seed = { value: Math.imul(this.firstChunkSeed(), 0x5d) >>> 0 };
        let remaining = count;
        // 거부된 칸은 count 를 줄이지 않는다.
        while (remaining !== 0) {
            const bounds = this.#context.map.bounds;
            const x = this.nextRange(seed, (bounds[0] + 2) | 0, (bounds[2] - 2) | 0);
            const y = this.nextRange(seed, (bounds[1] + 2) | 0, (bounds[3] - 2) | 0);
            const chunkX = RawTerrainGrowth.#floatChunk(x);
            const chunkY = RawTerrainGrowth.#floatChunk(y);
            const hasIsland = signed(this.#seedChunkAt(chunkX, chunkY).island) !== this.#globals.noRegion;
            if (hasIsland && this.#seedChunkAt(chunkX, chunkY).island !== 0) continue;
            const index = RawTerrainGrowth.#index(x, y);
            this.#context.codes[index] = CODE_ISLAND;
            this.#context.regions[index] = 0;
            remaining -= 1;
        }
    }

    // 004c0930: 호출자가 지정한 사각형을 X 열 먼저 반열린 순서로 채우며 이미 x 인 칸의 지역은 유지한다. 새로 채운 칸 수를 돌려준다. (2026-11-11 WB04-E292)
    #fill(left, top, right, bottom, region) {
        let added = 0;
        // 원본은 X 열을 먼저 진행한다.
        for (let x = left; x < right; x += 1) {
            // 각 X 열의 Y 를 현재 끝까지 진행한다.
            for (let y = top; y < bottom; y += 1) {
                const index = RawTerrainGrowth.#index(x, y);
                if (this.#context.codes[index] === CODE_ISLAND) continue;
                this.#context.codes[index] = CODE_ISLAND;
                this.#context.regions[index] = region;
                added += 1;
            }
        }
        return added;
    }

    // 004c0780: 양 끝을 포함한 현재 bounds 안의 비 x 칸을 추가한다. 같은 현재 군도 DWORD 가 비영일 때만 청크 지역 조회를 건너뛴다. (2026-11-11 WB04-E292)
    #put(x, y, region) {
        const bounds = this.#context.map.bounds;
        if (x < bounds[0] || y < bounds[1] || x > bounds[2] || y > bounds[3]) return false;
        const index = RawTerrainGrowth.#index(x, y);
        if (this.#context.codes[index] === CODE_ISLAND) return false;
        if (!this.#context.sharedArchipelagoMode().value && this.#chunkRegion(Math.trunc(x / CHUNK_SIDE), Math.trunc(y / CHUNK_SIDE)) !== signed(region)) return false;
        this.#context.regions[index] = region;
        this.#context.codes[index] = CODE_ISLAND;
        return true;
    }

    // 004c0800: 한 청크 칸 단위 성장이다(좌표를 청크 번호로 받아 1×1 사각형을 성장시킨다). (2026-11-11 WB04-E292)
    #grow(island, x, y, target, seed) {
        this.growRectangle(island, { left: x, top: y, right: x + 1, bottom: y + 1 }, target, seed);
    }

    // 004c0800: 청크 사각형의 X 폭을 두 좌표 난수에 사용하고 caller 의 시드를 갱신한다(원문 bottom 인자는 난수 폭 계산에 쓰지 않는다). 성공이 없는 연속 1000회 또는 목표 소진에서 끝난다.
    // 칸마다 X/Y 난수와 limit 1 호출을 모두 소비한 뒤 현재 x/지역을 읽고, 맞으면 Y 행 먼저 3×3 을 Put 한다. island 는 int32, target 도 int32 이다. (2026-11-11 WB04-E292)
    growRectangle(island, bounds, target, seed) {
        RawTerrainGrowth.#assertSeed(seed);
        if (!isInt32(island) || !isInt32(target) || !bounds || !isInt32(bounds.left) || !isInt32(bounds.top) || !isInt32(bounds.right) || !isInt32(bounds.bottom)) {
            throw new TypeError('growRectangle 의 island/target/bounds 는 int32 정수여야 합니다.');
        }
        // 원본 signed DWORD 왼쪽 shift 의 하위 32비트를 보존한다.
        const cell = (value) => (value << 4) | 0;
        const width = (Math.imul((bounds.right - bounds.left) | 0, 16) - 1) | 0;
        const region = island & 0xff;
        let remaining = target;
        let failures = GROWTH_FAILURES;
        // 성공이 없는 연속 1000회 또는 목표 추가량 소진에서 종료한다.
        while (remaining > 0 && failures !== 0) {
            failures -= 1;
            const cx = (cell(bounds.left) + this.nextRange(seed, 0, width)) | 0;
            const cy = (cell(bounds.top) + this.nextRange(seed, 0, width)) | 0;
            this.#next(seed, 1);
            const index = RawTerrainGrowth.#index(cx, cy);
            if (this.#context.codes[index] !== CODE_ISLAND || (!this.#context.sharedArchipelagoMode().value && this.#context.regions[index] !== region)) continue;
            const before = remaining;
            // 현재 원본 3×3 은 Y 행을 먼저 순회한다.
            for (let ny = cy - 1; ny <= cy + 1; ny += 1) {
                // 각 행의 X 를 처음부터 다시 읽는다.
                for (let nx = cx - 1; nx <= cx + 1; nx += 1) {
                    if (this.#put(nx, ny, region)) remaining -= 1;
                }
            }
            if (remaining < before) failures = GROWTH_FAILURES;
        }
    }

    // 004c0b30: 현재 칸이 x 일 때만 여덟 이웃을 읽고(직교 2·대각 1 가중), 합이 threshold 이하이면 허공으로 제거한다. 지역 BYTE 의 일치는 점수 조건이 아니다. 제거했는지 돌려준다. (2026-11-11 WB04-E292)
    removeSparseCell(x, y, threshold) {
        if (!isInt32(threshold)) throw new TypeError('removeSparseCell 의 threshold 는 int32 정수여야 합니다.');
        const index = RawTerrainGrowth.#index(x, y);
        if (this.#context.codes[index] !== CODE_ISLAND) return false;
        let weight = 0;
        // 북부터 시계방향으로 읽으며 짝수 방향의 직교 이웃에만 추가 가중치를 준다.
        for (let direction = 0; direction < NEIGHBOR_CELLS.length; direction += 1) {
            const [dx, dy] = NEIGHBOR_CELLS[direction];
            if (this.#context.codes[RawTerrainGrowth.#index(x + dx, y + dy)] === CODE_ISLAND) weight += (direction & 1) ? 1 : 2;
        }
        if (weight > threshold) return false;
        this.#context.regions[index] = this.#globals.noRegion & 0xff;
        this.#context.codes[index] = CODE_VOID;
        return true;
    }

    // 004c0db0: 내부 칸을 Y 외부/X 내부로 즉시 제거하며 변화가 없을 때까지 반복한다(제거된 앞 칸을 같은 pass 의 뒤 칸이 즉시 읽는다). 임계값은 4, 순회 끝은 bottom-1/right-1 직전이다. (2026-11-11 WB04-E292)
    removeSparse() {
        let changed;
        // 한 pass 에서 하나라도 제거했으면 처음부터 다시 훑는다.
        do {
            changed = false;
            const bounds = this.#context.map.bounds;
            // 원문 내부 Y 행 끝은 bottom-1 직전이다.
            for (let y = (bounds[1] + 1) | 0; y < ((bounds[3] - 1) | 0); y += 1) {
                // 현재 right 를 각 칸 뒤 다시 읽고 임계값 4 로 지운다.
                for (let x = (bounds[0] + 1) | 0; x < ((bounds[2] - 1) | 0); x += 1) {
                    changed = this.removeSparseCell(x, y, 4) || changed;
                }
            }
        } while (changed);
    }

    // 004c0fe0: 플레이어 청크 사각형을 float32 칸 경계로 바꿔 code/지역을 지운다. 첫 좌표는 float32 저장 뒤 ftol 하위 DWORD 이며 반복 비교는 정확한 정수와 float32 끝값 사이에서 한다.
    // 원문 context 의 현재 bounds 로 잘라내지 않고 지도 밖 접근은 거부한다. (2026-11-11 WB04-E292)
    clearPlacement(bounds) {
        if (!bounds || !isInt32(bounds.left) || !isInt32(bounds.top) || !isInt32(bounds.right) || !isInt32(bounds.bottom)) {
            throw new TypeError('clearPlacement 의 bounds 는 int32 청크 네 값이어야 합니다.');
        }
        // 원본 32비트 shift 후 float32 저장 지점을 보존한다.
        const cell = (value) => Math.fround((value << 4) | 0);
        // float32 값(정수값)의 ftol 하위 DWORD 만 쓴다.
        const first = (value) => Math.trunc(value) | 0;
        const left = cell(bounds.left);
        const top = cell(bounds.top);
        const right = cell(bounds.right);
        const bottom = cell(bounds.bottom);
        // Y 행을 먼저 진행하며 원문 context 의 현재 bounds 로 잘라내지 않는다.
        for (let y = first(top); y < bottom; y += 1) {
            // 각 행의 시작 X 는 변환된 같은 left 이며 오른쪽 경계는 포함하지 않는다.
            for (let x = first(left); x < right; x += 1) {
                const index = RawTerrainGrowth.#index(x, y);
                this.#context.regions[index] = this.#globals.noRegion & 0xff;
                this.#context.codes[index] = CODE_VOID;
            }
        }
    }

    // 004c0c30: 원래 이웃 13개(방향 0..12 를 8방향으로 순환)를 읽어 같은 지역이 연속 5개 이상이고 홀수(대각선) 방향에 닿으면 빈 칸을 그 지역의 x 로 메운다. 변경된 앞선 칸도 현재 배열에서 읽는다. (2026-11-11 WB04-E292)
    #fixCell(x, y) {
        let run = 0;
        let region = this.#globals.noRegion;
        // 북부터 13번의 원본 조회 순서를 유지한다.
        for (let direction = 0; direction <= 12; direction += 1) {
            const [dx, dy] = NEIGHBOR_CELLS[direction & 7];
            const index = RawTerrainGrowth.#index(x + dx, y + dy);
            if (this.#context.codes[index] !== CODE_ISLAND) {
                run = 0;
                continue;
            }
            const current = signed(this.#context.regions[index]);
            if (!run) region = current;
            if (region !== current) {
                run = 0;
                continue;
            }
            run += 1;
            if (run > 4 && (direction & 1)) {
                const target = RawTerrainGrowth.#index(x, y);
                this.#context.regions[target] = region & 0xff;
                this.#context.codes[target] = CODE_ISLAND;
                return;
            }
        }
    }

    // 004c0cf0: 군도 root 의 한 pass 와 일반 성장의 두 pass 를 같은 몸체로 실행한다. 원본의 bottom 은 포함 직전까지, right 는 한 칸 더 제외한다. 각 pass 의 문자 변경은 즉시 다음 칸에 반영된다. (2026-11-11 WB04-E292)
    fixGaps(passes) {
        if (!isInt32(passes)) throw new TypeError('fixGaps 의 passes 는 int32 정수여야 합니다.');
        // 각 pass 의 현재 문자 변경을 즉시 다음 칸에 반영한다.
        for (let pass = 0; pass < passes; pass += 1) {
            const bounds = this.#context.map.bounds;
            // Y 는 현재 top+1 부터 bottom 직전까지다.
            for (let y = (bounds[1] + 1) | 0; y < bounds[3]; y += 1) {
                // X 는 현재 left+1 부터 right-1 직전까지다.
                for (let x = (bounds[0] + 1) | 0; x < ((bounds[2] - 1) | 0); x += 1) {
                    if (this.#context.codes[RawTerrainGrowth.#index(x, y)] !== CODE_VOID) continue;
                    if (this.#context.codes[RawTerrainGrowth.#index(x - 1, y)] === CODE_ISLAND || this.#context.codes[RawTerrainGrowth.#index(x + 1, y)] === CODE_ISLAND
                        || this.#context.codes[RawTerrainGrowth.#index(x, y - 1)] === CODE_ISLAND || this.#context.codes[RawTerrainGrowth.#index(x, y + 1)] === CODE_ISLAND) this.#fixCell(x, y);
                }
            }
        }
    }

    // 004c10a0: 일반 분기이다. 전체 통로를 Y 청크 순서로 먼저 구성(섬별 시드 = 방문 수 × 0x10e3 + record[3], 중앙 4×4·연결 방향 통로·두 번째 방문의 중앙 10×10 채움, 밀도 = max(20, 난수30 + 50 - 채운 칸×100/256) × 256/100)한 뒤,
    // 모든 비 noRegion 청크의 실제 성장 helper 를 호출하고 빈틈 보정을 두 번 한다. 로컬 targets/visits 만 임시이며 결과 code/region 은 실제 context 배열이다. (2026-11-11 WB04-E292)
    runNormal() {
        const targets = new Int32Array(CHUNK_SIDE * CHUNK_SIDE);
        const visits = new Uint32Array(ISLAND_LIMIT);
        // 전체 통로를 Y 청크 순서로 먼저 구성한다.
        for (let y = this.#context.chunks().top; y < this.#context.chunks().bottom; y += 1) {
            // 각 Y 청크 행의 X 청크를 현재 bounds 에서 다시 읽는다.
            for (let x = this.#context.chunks().left; x < this.#context.chunks().right; x += 1) {
                const chunk = this.#chunks.at(x, y);
                const island = this.#chunkRegion(x, y);
                if (island === this.#globals.noRegion || (this.#context.region !== this.#globals.noRegion && island !== this.#context.region)) continue;
                if (island < 0 || island >= ISLAND_LIMIT) throw new RangeError('초기 성장 섬 방문 배열 범위 오류');
                if (x < 0 || y < 0 || x >= CHUNK_SIDE || y >= CHUNK_SIDE) throw new RangeError('초기 성장 청크 목표 배열 범위 오류');
                const seed = { value: (Math.imul(visits[island], CHUNK_SEED_STEP) + chunk.record[3]) >>> 0 };
                visits[island] += 1;
                const left = x * CHUNK_SIDE;
                const top = y * CHUNK_SIDE;
                const cx = left + 4 + (chunk.record[3] & 7);
                const cy = top + 4 + ((9999 - chunk.record[3]) & 7);
                // 청크 항목의 방향은 타입 모델이 글자 문자열, 실제 Builder(RawIslandBuilder)가 BYTE 코드 번호로 쓴다: 둘 다 같은 글자로 읽는다. (2026-11-11 WB04-E303: 정상 지형 월드 연결 시험이 찾음)
                const mask = connectionMask(typeof chunk.side === 'number' ? String.fromCharCode(chunk.side) : chunk.side);
                const region = island & 0xff;
                let added = this.#fill(cx - 1, cy - 1, cx + 3, cy + 3, region);
                added += this.#fill(mask & 8 ? left : cx, mask & 1 ? top : cy, mask & 2 ? left + 16 : cx + 2, mask & 4 ? top + 16 : cy + 2, region);
                if (visits[island] === 2) added += this.#fill(left + 3, top + 3, left + 13, top + 13, region);
                const density = Math.max(20, this.#next(seed, 30) + 50 - Math.trunc(added * 100 / 256));
                targets[x * CHUNK_SIDE + y] = Math.trunc(density * 256 / 100);
            }
        }
        // 통로가 모두 만들어진 뒤 모든 비 noRegion 청크의 실제 성장 helper 를 호출한다.
        for (let y = this.#context.chunks().top; y < this.#context.chunks().bottom; y += 1) {
            // 현재 청크를 같은 X 순서로 읽고 방문 수 증가를 원본처럼 유지한다.
            for (let x = this.#context.chunks().left; x < this.#context.chunks().right; x += 1) {
                const chunk = this.#chunks.at(x, y);
                const island = this.#chunkRegion(x, y);
                if (island === this.#globals.noRegion) continue;
                if (island < 0 || island >= ISLAND_LIMIT) throw new RangeError('초기 성장 현재 섬 배열 범위 오류');
                if (x < 0 || y < 0 || x >= CHUNK_SIDE || y >= CHUNK_SIDE) throw new RangeError('초기 성장 청크 목표 배열 범위 오류');
                const seed = { value: (Math.imul(visits[island], CHUNK_SEED_STEP) + chunk.record[3]) >>> 0 };
                visits[island] += 1;
                this.#grow(island, x, y, targets[x * CHUNK_SIDE + y], seed);
            }
        }
        this.fixGaps(2);
    }
}
