// 지면 타일 선택 모듈이다: 본섬 칸 마스크(terrain-mask.js)의 칸마다 직선·대각선 이웃으로 방향 글자 두 개를 만들고, `isle` 타입의 프레임 코드 표에서 클러스터(프레임) 번호를 고른다.
// 복원 기준: cpppj `o/RawTerrainStamp.cpp`(첫 도장 FrameAt, 원본 Terrainbuilder 004c04b0)·`o/RawFrameChoose.cpp`(ChooseTerrainFrame, 원본 0049aa90)·`o/TerrainBuilder.cpp`(Orientation 표 0x52f8fc·Normalize 0041cd20)·
// `o/RawRegionPaint.cpp`(지역 도장 Select, 원소 갱신 0046fe40)·`o/RawGameRandomSeed.cpp`(원본 00455970 난수 시드)이며 근거 문서는 docs/exe/terrain-and-bridges.md 와 docs/exe/cpp-initial-terrain-stamp-audit-20261013.md 이다.
// 범위: 칸마다 프레임 번호(`isle` 타입의 클러스터 번호)까지이다. 절벽 fringe 는 terrain-fringe.js(첫 도장 훅 onStamp 로 이어 붙임), 소유자 색 변환은 owner-colors.js, 작은 받침은 island-supports.js 에 있고
// edgeFarm 과 그리기는 옮기지 않았다.
// 규칙 요약:
//  - 이웃 비트는 북부터 시계 방향 여덟 칸 `1,2,2,4,4,8,8,1`(짝수 번호는 직선 쪽 mask, 홀수 번호는 대각선 쪽 mask)이고, 이웃이 같은 영역일 때만 켠다. 두 mask 를 비트→글자 표 `PNOILJFBMHKEGDCA` 로 바꾼 것이 (방향 글자, 변형 글자)이다.
//  - 두 글자가 모두 'A'(여덟 이웃이 모두 같은 영역 = 원본 문자 'x' 안쪽 칸)이면 3 × 3 그림 네 변형 중 하나이다. 가장자리 칸('=')은 같은 글자의 클러스터 후보에서 고른다.
//  - 후보 검색(ChooseTerrainFrame): ① (방향, 변형) 정확히 일치 → ② 비어 있으면 Normalize 한 글자 → ③ 그래도 비면 (방향, 'A') → ④ 그래도 비면 방향만 일치 → 모두 비면 -1(도장은 0 으로 대체). 후보는 프레임 코드 저장 순서로 최대 256개이다.
//  - 후보에서 고르는 법: 원소 번호(theme)가 -1 이면 전체에서 난수 한 번, 0~3 이면 후보 수의 4분의 1(q)로 [q × theme + 1, q × (theme + 1)) 범위에서 난수 한 번(각 원소 묶음의 첫 프레임은 제외, 범위가 비면 오류).
//  - 첫 도장(RawTerrainStamp)은 원소 0(태양)로 후보를 고르고 안쪽 칸은 CRT `_rand` 99개 표를 쓴다. 신전을 세워 소유자가 정해진 영역은 지역 도장(RawRegionPaint)이 원소별로 다시 고르며 안쪽 칸은 결정적인 99개 표를 쓴다.
// 이 모듈은 DOM 을 쓰지 않아 Node 에서도 불러와 검사할 수 있다.
// (2026-11-02 WB03-X: 정적 맵 화면(M2) 3단계(b)로 추가)
import { EMPTY_TERRAIN, LocalRandom, NEIGHBOR_CELLS, WORLD_CELLS, connectionMask, orientationLetter } from './terrain-mask.js';
import { CrtRandom } from '../core/random.js';

// 지면 원소 이름이다. 번호 = 후보 4분의 1 구간의 순서이자 3 × 3 그림 묶음의 순서(해·천둥·바람·비)이다. (2026-11-02 WB03-X)
export const THEME_NAMES = Object.freeze(['sun', 'thunder', 'wind', 'rain']);

// 원소 종류의 수이다. (2026-11-02 WB03-X)
export const THEME_COUNT = THEME_NAMES.length;

// 이웃(북부터 시계 방향 여덟 칸)마다 켜는 비트이다(원본 RawTerrainStamp: 짝수 번호는 직선 mask, 홀수 번호는 대각선 mask 에 더한다; 대각선은 북서 1·북동 2·남동 4·남서 8). (2026-11-02 WB03-X)
const NEIGHBOR_BITS = Object.freeze([1, 2, 2, 4, 4, 8, 8, 1]);

// 후보 목록이 가질 수 있는 최대 개수이다(원본 ChooseTerrainFrame 의 256). (2026-11-02 WB03-X)
export const MAX_CANDIDATES = 256;

// 안쪽 칸 3 × 3 그림 묶음의 한 변 칸 수이다. (2026-11-02 WB03-X)
export const CORE_SIZE = 3;

// 원소 하나의 3 × 3 그림 변형 수이다. (2026-11-02 WB03-X)
export const CORE_VARIANTS = 4;

// 원소 하나가 차지하는 3 × 3 그림 클러스터 수이다(3 × 3 × 변형 4). (2026-11-02 WB03-X)
export const CORE_THEME_FRAMES = CORE_SIZE * CORE_SIZE * CORE_VARIANTS;

// 변형 번호를 고르는 난수표의 길이이다(원본 004c04b0 의 99개). (2026-11-02 WB03-X)
export const CORE_TABLE_SIZE = 99;

// 원본 CRT 시드 함수(00455970)가 난수 모드 0 에서 쓰는 고정 시드이다. (2026-11-02 WB03-X)
export const FIXED_CRT_SEED = 0x38d535;

// 원본 CRT 시드 함수가 시드 직후 버리는 `_rand` 호출 수이다. 이어서 세 값을 더 뽑아 곱한 값이 게임 난수의 시작 상태가 된다. (2026-11-02 WB03-X)
const CRT_WARMUP = 100;

// 방향 글자를 문자 코드로 바꾼다. 한 글자 문자열 또는 이미 코드(숫자)인 값을 받는다. 'A'~'P' 밖이면 오류이다. (2026-11-02 WB03-X)
function letterCode(letter) {
    const code = typeof letter === 'number' ? letter : String(letter).charCodeAt(0);
    if (!(code >= 0x41 && code <= 0x50)) throw new RangeError('프레임 글자는 A~P 만 있다');
    return code;
}

// 마스크의 (x, y) 칸 값이다. 월드 밖은 허공이다. (2026-11-02 WB03-X)
function cellAt(land, x, y) {
    return x >= 0 && y >= 0 && x < WORLD_CELLS && y < WORLD_CELLS ? land[y * WORLD_CELLS + x] : EMPTY_TERRAIN;
}

// 마스크의 (x, y) 칸이 지면이면 같은 영역 이웃의 직선·대각선 비트와 방향 글자를 돌려준다: { straight, diagonal, side, variant }.
// 이웃이 같은 영역 번호일 때만 비트를 켠다(월드 밖·허공·다른 영역은 끈다). 칸이 허공이면 null 이다.
// 사용: neighborLetters(mask, x, y). (2026-11-02 WB03-X)
export function neighborLetters(land, x, y) {
    const region = cellAt(land, x, y);
    if (region === EMPTY_TERRAIN) return null;
    let straight = 0;
    let diagonal = 0;
    // 북부터 시계 방향으로 여덟 이웃을 본다(짝수 번호는 직선, 홀수 번호는 대각선).
    for (let neighbor = 0; neighbor < NEIGHBOR_CELLS.length; neighbor += 1) {
        const [dx, dy] = NEIGHBOR_CELLS[neighbor];
        if (cellAt(land, x + dx, y + dy) !== region) continue;
        if ((neighbor & 1) === 0) straight |= NEIGHBOR_BITS[neighbor];
        else diagonal |= NEIGHBOR_BITS[neighbor];
    }
    return { straight, diagonal, side: orientationLetter(straight), variant: orientationLetter(diagonal) };
}

// 방향 정규화이다(원본 0041cd20, cpppj TerrainBuilder::Normalize): 양쪽 직선 이웃이 받쳐 주는 대각선만 남기고 그 대각선에서 직선 연결을 다시 만든다.
// 대각선 비트 i 는 직선 비트 i 와 그 앞 직선 비트((i + 3) & 3)가 모두 있을 때만 유효하다. 반환: { side, variant }(글자). (2026-11-02 WB03-X)
export function normalizeOrientation(side, variant) {
    const cardinal = connectionMask(side);
    let diagonal = connectionMask(variant);
    let supported = 0;
    // 북서·북동·남동·남서마다 받쳐 주는 직선 쌍이 모두 있는지 본다.
    for (let i = 0; i < 4; i += 1) {
        const pair = (1 << i) | (1 << ((i + 3) & 3));
        if ((cardinal & pair) === pair) supported |= 1 << i;
    }
    diagonal &= supported;
    let result = 0;
    // 남은 대각선이 요구하는 직선 비트를 모은다.
    for (let i = 0; i < 4; i += 1) {
        if ((diagonal & (1 << i)) !== 0) result |= (1 << i) | (1 << ((i + 3) & 3));
    }
    return { side: orientationLetter(result), variant: orientationLetter(diagonal) };
}

// 타입의 프레임 코드 표이다(원본 타입의 +0x124 코드 배열): 클러스터가 선언된 순서대로 방향 글자(side)·변형 글자(variant)·플래그(flags)를 보관하고, 글자 구간과 후보 목록을 조회한다.
// 사용: const frames = FrameCodeTable.fromDefinition(typeTable.definitions[typeTable.find('isle') - 70]); frames.run('J'). (2026-11-02 WB03-X)
export class FrameCodeTable {
    // codes 는 [{ side, variant, flags }](글자는 문자 코드, 플래그는 부호 없는 바이트)이다. 배열을 복사해 얼린다.
    constructor(codes) {
        // 프레임 코드 목록이다(읽기 전용).
        // 원본 코드는 정확히 4바이트(방향·변형·번호·플래그)이다. 번호는 2026-11-10 WB04-E266 에서 FindNumber/FindMasked 를 위해 보관한다(미지정 입력은 0).
        this.codes = Object.freeze(codes.map((code) => Object.freeze({ side: code.side, variant: code.variant, number: code.number ?? 0, flags: code.flags })));
    }

    // 원본 0049a9a0(B)/CD 00444350: 방향·변형·번호가 모두 같은 첫 코드의 위치를 돌려준다(없으면 -1). 중복 프레임은 저장 순서의 첫 번째가 이긴다. 인자는 BYTE 이다.
    // (2026-11-10 WB04-E266: cpppj RiftTypeFrames::FindNumber)
    findNumber(side, variant, number) {
        // 첫 일치를 반환하므로 중복 프레임의 원본 순서가 유지된다.
        for (let i = 0; i < this.codes.length; i += 1) {
            const code = this.codes[i];
            if (code.side === (side & 0xff) && code.variant === (variant & 0xff) && code.number === (number & 0xff)) return i;
        }
        return -1;
    }

    // 원본 0049a9e0(B)/CD 00444410: 방향·변형·번호를 확인한 뒤 플래그를 signed 8비트에서 32비트로 부호 확장해 mask(DWORD)와 AND 한 값이 0 이 아닌 첫 코드의 위치이다.
    // (2026-11-10 WB04-E266: cpppj RiftTypeFrames::FindMasked)
    findMasked(side, variant, number, mask) {
        // 원본처럼 방향·변형·번호를 확인한 뒤 마스크를 검사한다.
        for (let i = 0; i < this.codes.length; i += 1) {
            const code = this.codes[i];
            if (code.side === (side & 0xff) && code.variant === (variant & 0xff) && code.number === (number & 0xff) && (((code.flags << 24) >> 24) & (mask >>> 0)) !== 0) return i;
        }
        return -1;
    }

    // 원본 0049aa30(A)/CD 00444460(A): 방향·변형과 부호 확장한 플래그가 정확히 같은 첫 코드의 위치이다. 번호는 이 검색의 조건이 아니다.
    // (2026-11-10 WB04-E266: cpppj RiftTypeFrames::FindFlags)
    findFlags(side, variant, flags) {
        // 정확한 플래그 값까지 일치하는 첫 코드를 찾는다.
        for (let i = 0; i < this.codes.length; i += 1) {
            const code = this.codes[i];
            if (code.side === (side & 0xff) && code.variant === (variant & 0xff) && ((code.flags << 24) >> 24) === (flags | 0)) return i;
        }
        return -1;
    }

    // 원본이 글자 16개('A'~'P')를 차례로 보며 첫 번호가 처음 정해질 때마다 올리는 +0x12c 값, 곧 구간이 하나라도 있는 글자의 수이다. (2026-11-10 WB04-E266: cpppj RiftTypeFrames::LetterKinds)
    letterKinds() {
        let kinds = 0;
        // 글자 'A'~'P' 를 하나씩 확인한다.
        for (let letter = 0x41; letter <= 0x50; letter += 1) if (this.run(String.fromCharCode(letter)).first !== -1) kinds += 1;
        return kinds;
    }

    // 타입 정의(parseTypeDefinition 의 결과)의 클러스터 코드에서 표를 만든다.
    static fromDefinition(definition) {
        return new FrameCodeTable(definition.clusters.map((cluster) => cluster.code));
    }

    // 프레임 개수이다.
    get length() {
        return this.codes.length;
    }

    // 방향 글자 하나가 차지하는 프레임 구간 { first(처음 프레임 번호, 없으면 -1), count }이다(원본 0049b060). 'A'~'P' 밖의 글자는 오류이다.
    run(letter) {
        const code = letterCode(letter);
        let first = -1;
        let count = 0;
        // 배열 전체를 한 번 훑어 같은 글자를 센다.
        for (let i = 0; i < this.codes.length; i += 1) {
            if (this.codes[i].side !== code) continue;
            if (first === -1) first = i;
            count += 1;
        }
        return { first, count };
    }

    // 방향 글자가 맞고(variant 가 null 이 아니면 변형 글자도 맞는) 프레임 번호를 저장 순서대로 최대 256개 모은다(원본 ChooseTerrainFrame 의 검색).
    candidates(side, variant) {
        const sideCode = letterCode(side);
        const variantCode = variant === null ? null : letterCode(variant);
        const found = [];
        // 코드 배열을 앞에서부터 훑는다.
        for (let i = 0; i < this.codes.length && found.length < MAX_CANDIDATES; i += 1) {
            if (this.codes[i].side === sideCode && (variantCode === null || this.codes[i].variant === variantCode)) found.push(i);
        }
        return found;
    }
}

// 표(FrameCodeTable)마다 글자 쌍별 후보 검색 결과를 기억해 둔다(표는 만든 뒤 바뀌지 않아 결과가 항상 같고, 한 맵에서 같은 쌍을 수만 번 찾기 때문이다). (2026-11-02 WB03-X)
const CANDIDATE_CACHE = new WeakMap();

// 방향·변형 글자에 대한 후보 목록과 어느 단계에서 찾았는지를 돌려준다: { stage, candidates }(둘 다 얼린 값).
// stage 는 'exact'(정확히 일치) / 'normalized'(정규화한 글자) / 'sideA'((방향, 'A')) / 'side'(방향만) / 'none'(후보 없음)이다. 난수는 쓰지 않는다.
// 사용: terrainFrameCandidates(frames, 'B', 'F'). (2026-11-02 WB03-X)
export function terrainFrameCandidates(frames, side, variant) {
    const key = `${side}${variant}`;
    let cache = CANDIDATE_CACHE.get(frames);
    if (!cache) {
        cache = new Map();
        CANDIDATE_CACHE.set(frames, cache);
    }
    let result = cache.get(key);
    if (!result) {
        result = Object.freeze(findCandidates(frames, side, variant));
        cache.set(key, result);
    }
    return result;
}

// 후보 검색 본체이다(캐시 없음): 정확히 → 정규화 → (방향, 'A') → 방향만 순서로 처음 비지 않은 단계의 후보를 돌려준다. (2026-11-02 WB03-X)
function findCandidates(frames, side, variant) {
    let candidates = frames.candidates(side, variant);
    if (candidates.length > 0) return { stage: 'exact', candidates: Object.freeze(candidates) };
    const normalized = normalizeOrientation(side, variant);
    candidates = frames.candidates(normalized.side, normalized.variant);
    if (candidates.length > 0) return { stage: 'normalized', candidates: Object.freeze(candidates) };
    // 정규화 뒤에도 후보가 없으면 정규화하기 전의 방향 글자로 (방향, 'A'), 방향만 순서로 찾는다.
    candidates = frames.candidates(side, 'A');
    if (candidates.length > 0) return { stage: 'sideA', candidates: Object.freeze(candidates) };
    candidates = frames.candidates(side, null);
    if (candidates.length > 0) return { stage: 'side', candidates: Object.freeze(candidates) };
    return { stage: 'none', candidates: Object.freeze([]) };
}

// 원소 theme(0~3)에 대한 후보 인덱스 범위 { low, high }(반열린)이다. q = 후보 수 ÷ 4(내림), low = q × theme + 1, high = q × (theme + 1)이다.
// 범위가 비면(high ≤ low) 원본의 범위 검사 오류처럼 RangeError 이다. 사용: themeRange(8, 1) → { low: 3, high: 4 }. (2026-11-02 WB03-X)
export function themeRange(candidateCount, theme) {
    const quarter = Math.trunc(candidateCount / 4);
    const low = quarter * theme + 1;
    const high = quarter * (theme + 1);
    if (high <= low) throw new RangeError('지역 프레임 quartile 난수 범위 오류');
    return { low, high };
}

// 방향·변형 글자와 원소로 클러스터 하나를 고른다(원본 ChooseTerrainFrame). theme 가 -1 이면 후보 전체에서, 0~3 이면 themeRange 의 범위에서 random.next 를 한 번 소비해 고른다.
// 후보가 없으면 -1 이다(난수를 쓰지 않는다). random 은 next(limit)(0 이상 limit 미만의 정수)를 가진 객체이다. (2026-11-02 WB03-X)
export function chooseTerrainFrame(frames, side, variant, theme, random) {
    const { candidates } = terrainFrameCandidates(frames, side, variant);
    if (candidates.length === 0) return -1;
    if (theme === -1) return candidates[random.next(candidates.length)];
    const { low, high } = themeRange(candidates.length, theme);
    const index = low + random.next(high - low);
    if (index < 0 || index >= candidates.length) throw new RangeError('지역 프레임 후보 범위 오류');
    return candidates[index];
}

// CRT(MSVC `rand`) 난수는 세션 장기 상태와 같은 구현을 쓰도록 core/random.js 로 옮겼다. 기존 소비자를 위해 이 모듈에서도 다시 내보낸다.
// 사용: const crt = new CrtRandom(FIXED_CRT_SEED); crt.next(). (2026-11-02 WB03-X, 2026-11-05 WB04-C1: core 의 같은 구현으로 통합)
export { CrtRandom };

// 원본 난수 시드 함수(00455970)를 옮긴 것이다: CRT 상태를 시드로 정하고 100개를 버린 뒤 세 값을 더 뽑아 곱한 32비트 값을 게임 난수의 시작 상태로 쓴다.
// seed 는 난수 모드 0 이면 고정 시드 0x38d535, 아니면 현재 시각(Unix 초)이다. 반환: { crt(103개를 소비한 CRT 난수), game(LocalRandom: 게임 난수와 같은 생성기), gameSeed }.
// 사용: const { crt, game } = seedGameRandoms(); . 이 시작 상태가 실제 지형 도장 때의 상태와 같다는 보장은 없다(그 사이에 다른 소비가 있다). (2026-11-02 WB03-X)
export function seedGameRandoms(seed = FIXED_CRT_SEED) {
    const crt = new CrtRandom(seed);
    // 시드 직후 100개를 버린다.
    for (let i = 0; i < CRT_WARMUP; i += 1) crt.next();
    const first = crt.next();
    const second = crt.next();
    const third = crt.next();
    const gameSeed = Math.imul(Math.imul(first, second), third) >>> 0;
    return { crt, game: new LocalRandom(gameSeed), gameSeed };
}

// 첫 도장의 변형 난수표 99개이다(원본 004c04b0 의 전역 표): CRT 난수를 99번 이어 뽑아 채우되 앞 항목과 하위 2비트가 같으면 1 올린다. crt 의 상태를 99번 전진시킨다.
// 사용: createStampVariants(seedGameRandoms().crt). (2026-11-02 WB03-X)
export function createStampVariants(crt) {
    const values = new Array(CORE_TABLE_SIZE);
    // 99개를 앞에서부터 채운다.
    for (let i = 0; i < CORE_TABLE_SIZE; i += 1) {
        let value = crt.next();
        if (i > 0 && values[i - 1] % 4 === value % 4) value += 1;
        values[i] = value;
    }
    return values;
}

// 지역 도장의 변형 난수표 99개이다(원본 RawRegionPaint 의 캐시): 항목마다 시드 0 의 지역 난수 `Next(65536)`을 새로 뽑으므로 모든 항목이 같은 값 v 이고, 앞 항목과 하위 2비트가 같으면 1 올려 v, v + 1 이 번갈아 나온다.
// 게임 난수를 쓰지 않으므로 항상 같다. (2026-11-02 WB03-X)
export function createPaintVariants() {
    const values = new Array(CORE_TABLE_SIZE);
    // 항목마다 시드 0 의 난수를 새로 만든다.
    for (let i = 0; i < CORE_TABLE_SIZE; i += 1) {
        let value = new LocalRandom(0).next(65536);
        if (i > 0 && values[i - 1] % 4 === value % 4) value += 1;
        values[i] = value;
    }
    return values;
}

// 3 × 3 그림 묶음의 시작 프레임이다(`JJ00` 자리표시자의 다음 프레임). 'J' 방향 글자가 없는 타입이면 오류이다. (2026-11-02 WB03-X)
function coreStart(frames) {
    const { first } = frames.run('J');
    if (first === -1) throw new RangeError('3 × 3 그림 자리표시자(JJ00)가 없다');
    return first + 1;
}

// 첫 도장에서 안쪽 칸(x, y)의 프레임이다(원본 FrameAt): 시작 + (y % 3 + 변형 × 3) × 3 + x % 3, 변형 = (표[(표[(x ÷ 3) % 99] + 표[(y ÷ 3) % 99]) % 99] + y ÷ 3) % 4. 원소 항이 없으므로 항상 태양 묶음이다.
// variants 는 createStampVariants 의 표이다. 사용: stampCoreFrame(frames, variants, 120, 130). (2026-11-02 WB03-X)
export function stampCoreFrame(frames, variants, x, y) {
    const blockX = Math.trunc(x / CORE_SIZE);
    const blockY = Math.trunc(y / CORE_SIZE);
    const sum = (variants[blockX % CORE_TABLE_SIZE] + variants[blockY % CORE_TABLE_SIZE]) % CORE_TABLE_SIZE;
    const chosen = (variants[sum] + blockY) % CORE_VARIANTS;
    return coreStart(frames) + ((y % CORE_SIZE) + chosen * CORE_SIZE) * CORE_SIZE + (x % CORE_SIZE);
}

// 지역 도장에서 안쪽 칸(x, y)의 프레임이다(원본 RegionPaint::Select): 시작 + 원소 × 36 + (변형 × 3 + y % 3) × 3 + x % 3, 변형 = (y ÷ 3 + 표[(표[(x ÷ 3) % 99] + 표[(y ÷ 3) % 99]) % 99]) % 4.
// theme 는 0~3 이다(-1 은 태양으로 본다). variants 는 createPaintVariants 의 표이다. 사용: paintCoreFrame(frames, variants, 3, 120, 130). (2026-11-02 WB03-X)
export function paintCoreFrame(frames, variants, theme, x, y) {
    const blockX = Math.trunc(x / CORE_SIZE);
    const blockY = Math.trunc(y / CORE_SIZE);
    const sum = (variants[blockX % CORE_TABLE_SIZE] + variants[blockY % CORE_TABLE_SIZE]) % CORE_TABLE_SIZE;
    const variant = (blockY + variants[sum]) % CORE_VARIANTS;
    const element = theme === -1 ? 0 : theme;
    return coreStart(frames) + element * CORE_THEME_FRAMES + (variant * CORE_SIZE + (y % CORE_SIZE)) * CORE_SIZE + (x % CORE_SIZE);
}

// 마스크의 모든 지면 칸에 방향 글자를 붙인다. 반환: { sides, variants }(각각 길이 65536 의 Uint8Array, 인덱스 = y × 256 + x, 값 = 글자의 문자 코드, 지면이 아닌 칸은 0). (2026-11-02 WB03-X)
export function classifyCells(land) {
    const sides = new Uint8Array(land.length);
    const variants = new Uint8Array(land.length);
    // 월드를 y 바깥·x 안쪽 순서로 훑는다.
    for (let y = 0; y < WORLD_CELLS; y += 1) {
        // 같은 행의 칸을 왼쪽부터 본다.
        for (let x = 0; x < WORLD_CELLS; x += 1) {
            if (land[y * WORLD_CELLS + x] === EMPTY_TERRAIN) continue;
            const letters = neighborLetters(land, x, y);
            sides[y * WORLD_CELLS + x] = letters.side.charCodeAt(0);
            variants[y * WORLD_CELLS + x] = letters.variant.charCodeAt(0);
        }
    }
    return { sides, variants };
}

// 마스크의 모든 지면 칸에 프레임(타입 `isle` 의 클러스터 번호)을 정한다. 반환: Int32Array(길이 65536, 인덱스 = y × 256 + x, 지면이 없으면 -1).
// 1단계(첫 도장): 월드 y 바깥·x 안쪽 순서로 안쪽 칸(두 글자가 모두 'A')은 stampCoreFrame, 가장자리 칸은 원소 0 의 chooseTerrainFrame(후보가 없으면 프레임 0)이다.
// 2단계(지역 도장): options.themeOf(영역 번호)가 0~3 을 돌려주는 영역은 그 영역의 칸을 다시 y·x 순서로 고른다. 원본은 지금 프레임의 코드(첫 도장이 고른 클러스터의 글자)로 판단하고 안쪽 코드면 paintCoreFrame, 아니면 그 원소의 chooseTerrainFrame 이다.
// themeOf 가 null/undefined 를 돌려주면(소유자가 없는 영역) 첫 도장 그대로 둔다. 원본의 지역 도장은 신전에서 번지는 순서와 경계 처리가 있어 난수를 쓰는 순서가 다르다(후보 집합과 안쪽 칸 규칙은 같다).
// options: { random(기본 seedGameRandoms().game), crt(기본 seedGameRandoms().crt), themeOf(기본 모든 영역이 소유자 없음),
//   stampVariants(이미 만들어 둔 첫 도장 난수표 99개, 기본은 첫 지면 칸에서 crt 로 새로 만듦 — 원본은 이 표를 전역에 한 번만 만들어 월드마다 다시 쓴다), onStamp(x, y, frame, random)(첫 도장에서 칸의 프레임을 고른 직후 부르는 훅,
//   원본이 지면 하나를 만든 뒤 절벽 fringe 를 이어서 만드는 순서를 같은 난수로 이어 가기 위한 것이다 — terrain-fringe.js 의 createFringeStamper 가 쓴다) }.
// 사용: buildTerrainFrames(mask, FrameCodeTable.fromDefinition(def), { themeOf: (territory) => themes.get(territory) }). (2026-11-02 WB03-X; stampVariants·onStamp 훅은 2026-11-02 WB03-Y 에서 절벽 fringe 연결을 위해 추가)
export function buildTerrainFrames(land, frames, options = {}) {
    const seeded = options.random && options.crt ? null : seedGameRandoms();
    const random = options.random ?? seeded.game;
    const crt = options.crt ?? seeded.crt;
    const themeOf = options.themeOf ?? (() => null);
    const result = new Int32Array(land.length).fill(-1);
    // 첫 도장 난수표이다. 지면 칸이 하나도 없으면 원본처럼 crt 를 소비하지 않도록 첫 칸에서 만든다(원본은 첫 프레임 요청에서 표를 만들고 이후 재사용한다). (2026-11-02 WB03-Y)
    let stampVariants = options.stampVariants ?? null;
    // 1단계: 모든 지면 칸을 첫 도장 규칙으로 고른다.
    for (let y = 0; y < WORLD_CELLS; y += 1) {
        // 같은 행의 칸을 왼쪽부터 첫 도장 규칙으로 고른다.
        for (let x = 0; x < WORLD_CELLS; x += 1) {
            const index = y * WORLD_CELLS + x;
            if (land[index] === EMPTY_TERRAIN) continue;
            stampVariants ??= createStampVariants(crt);
            const letters = neighborLetters(land, x, y);
            const interior = letters.side === 'A' && letters.variant === 'A';
            const frame = interior ? stampCoreFrame(frames, stampVariants, x, y) : chooseTerrainFrame(frames, letters.side, letters.variant, 0, random);
            result[index] = frame === -1 ? 0 : frame;
            options.onStamp?.(x, y, result[index], random);
        }
    }
    // 2단계: 원소가 정해진 영역만 지역 도장 규칙으로 다시 고른다.
    const paintVariants = createPaintVariants();
    // 영역마다 한 번만 원소를 묻는다.
    const themes = new Map();
    // 월드를 y 바깥·x 안쪽 순서로 훑는다.
    for (let y = 0; y < WORLD_CELLS; y += 1) {
        // 같은 행의 칸을 왼쪽부터 본다.
        for (let x = 0; x < WORLD_CELLS; x += 1) {
            const index = y * WORLD_CELLS + x;
            const territory = land[index];
            if (territory === EMPTY_TERRAIN) continue;
            if (!themes.has(territory)) themes.set(territory, themeOf(territory) ?? null);
            const theme = themes.get(territory);
            if (theme === null) continue;
            const code = frames.codes[result[index]];
            const interior = code.side === 0x41 && code.variant === 0x41;
            const side = String.fromCharCode(code.side);
            const variant = String.fromCharCode(code.variant);
            const frame = interior ? paintCoreFrame(frames, paintVariants, theme, x, y) : chooseTerrainFrame(frames, side, variant, theme, random);
            // 현재 프레임 자신이 같은 글자의 후보라 -1 이 될 수 없다(후보 검색은 비지 않는다).
            result[index] = frame;
        }
    }
    return result;
}
