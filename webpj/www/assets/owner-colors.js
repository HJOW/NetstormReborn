// 소유자 색 변환표 모듈이다: 오브젝트의 소유자 색 번호(1~8)에 따라 그림의 색 번호 256개를 다른 색 번호로 바꾸는 9 × 256바이트 표를 만든다. 지면(`isle`, 타입 97)·작은 받침 윗면(`island`, 타입 94)의 테두리가
// 소유자 색으로 보이는 것이 이 표 때문이다. 0번 행(중립)은 항상 항등 표이다.
// 복원 기준: cpppj `client/OwnerColorMaps.cpp`(원본 43b950 항등 9표 · 43ba00 일반 띠 · 43bbe0/43b750 명도 전용 표)·`client/OwnerColorTableState.cpp`(원본 43bc40 배너 옵션 · 43be30 최초 등록)이다.
// 종류 두 가지:
//  - 일반 띠 표(`buildStandardOwnerColorMaps`): 색 번호 228~245 의 18칸만 바꾼다. 228~237 은 후보의 가장 밝은 색(밝기 0), 238~245 는 역순 7~0 의 여덟 명도이다. 다리 타입은 소유자 2·7 의 후반 명도를 고정한다.
//  - 명도 전용 표(`buildSpecialOwnerColorMaps`): 지정한 소스 색 번호마다 팔레트의 HSL 명도(= (최댓값 + 최솟값) ÷ 2)에 소유자별 보정값을 더한 목표에 가장 가까운 명도의 후보 색을 고른다(같은 거리는 앞 후보).
//    타입 97(`isle`)은 소유자별 보정값 8개, 141 은 −0.15, 94·111·137 은 0 이다.
// 소유자 색 후보표(72개)는 원본 실행 파일 005319e8~00531b07 의 DWORD 표이다: 소유자 색 번호 c(1~8)의 명도 후보 8개가 [c × 8, c × 8 + 8) 이고 낮은 바이트만 쓴다.
// 소유자 색 번호의 의미: 1 파랑 · 2 빨강 · 3 흰색 · 4 초록 · 5 보라 · 6 노랑 · 7 연파랑 · 8 주황(기본값은 소유자 번호 그대로, 미션 머리가 덮어씀).
// 이 모듈은 DOM 을 쓰지 않아 Node 에서도 불러와 검사할 수 있다.
// (2026-11-02 WB03-Y: 정적 맵 화면(M2) 3단계(c)로 추가)

// 소유자 색 번호의 개수이다(1~8, 0 은 중립). 변환표는 0~8 의 9행이다. (2026-11-02 WB03-Y)
export const OWNER_COLOR_COUNT = 8;

// 변환표 한 행의 길이이다(팔레트 색 번호 256개). (2026-11-02 WB03-Y)
export const OWNER_COLOR_ROW = 256;

// 소유자 색 후보표이다(원본 005319e8~00531b07 의 DWORD 72개). 색 0 의 8칸은 쓰이지 않는다. (2026-11-02 WB03-Y)
export const OWNER_COLOR_CANDIDATES = Object.freeze([
    0, 0, 0, 0, 0, 0, 0, 0, 81, 80, 76, 145, 146, 147, 4, 10, 94, 100, 170, 169, 204, 249, 217, 227,
    195, 93, 185, 35, 31, 149, 70, 15, 207, 131, 133, 128, 136, 2, 142, 143, 189, 190, 253, 150, 148, 70, 14, 10,
    113, 218, 219, 3, 52, 59, 67, 213, 9, 85, 155, 156, 6, 72, 144, 143, 220, 221, 222, 223, 224, 63, 227, 213,
]);

// 일반 띠 표가 바꾸는 색 번호 구간(228~245, 양 끝 포함)이다. 238 부터는 역순 명도이다. (2026-11-02 WB03-Y)
const BAND_FIRST = 228;
const BAND_LAST = 245;
const BAND_DARK_FIRST = 238;

// 다리 타입에서 소유자 2 와 7 의 후반 명도 번호이다(소유자 2 → 4, 소유자 7 → 3, 원본 특례). (2026-11-02 WB03-Y)
const BRIDGE_SHADE_OWNER_TWO = 4;
const BRIDGE_SHADE_OWNER_SEVEN = 3;

// 명도 전용 표의 소스 색 번호 목록이다(원본 가변 인자 순서, 종료 표시 ff 는 뺐다). 타입 94·97 은 같은 지면 소스 33개를 쓴다. (2026-11-02 WB03-Y)
const SURFACE_SOURCES = Object.freeze([44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 3, 177, 200, 203, 224, 220, 221, 222, 223, 114, 201, 226]);
const SOURCES_111 = Object.freeze([86, 188, 85, 247, 84, 31, 34, 75, 82, 74, 83, 156, 153, 70, 72, 71, 73, 152, 13, 144]);
const SOURCES_141 = Object.freeze([87, 88, 85, 86, 84, 156, 75, 82, 153, 74, 73, 83, 155, 72, 71, 44, 152, 154, 70, 13, 144]);
const SOURCES_137 = Object.freeze([218, 219, 220, 52, 53, 54, 55, 56, 57, 58, 59, 66, 161, 210, 19, 21, 214, 141, 18]);

// 타입 97(`isle`)의 소유자 1~8 명도 보정값이다(원본 43b750). (2026-11-02 WB03-Y)
const ISLE_OFFSETS = Object.freeze([-0.1, -0.17, 0.15, 0, 0, 0.05, -0.2, 0]);

// 타입 141 의 명도 보정값이다(float 로 저장된 −0.15 를 double 로 승격한 값, 원본 그대로). (2026-11-02 WB03-Y)
const OFFSET_141 = -0.15000000596046448;

// RGB 정규화 계수(double 1/255)이다(원본 ColorSort 와 같다). (2026-11-02 WB03-Y)
const NORMALIZE = 1.0 / 255.0;

// 명도 비교에서 첫 후보의 거리 상한이다(원본 `min(99, 거리)`). (2026-11-02 WB03-Y)
const DISTANCE_LIMIT = 99.0;

// 소유자 색표가 만들어지는 타입 번호(10.78 패치판, 원본 현재 TYPE 전역)이다. banner 는 배너 옵션을 켜면 반납되고 끄면 마지막 둘(97 `isle`·94 `island`)만 만들어지는 타입,
// standard 는 최초 등록 때 일반 띠 표를 만드는 12개, special 은 명도 전용 표를 만드는 3개이다. (2026-11-02 WB03-Y)
export const OWNER_COLOR_TYPES = Object.freeze({
    banner: Object.freeze([85, 71, 128, 97, 94]),
    standard: Object.freeze([82, 155, 158, 164, 135, 133, 156, 73, 170, 146, 144, 126]),
    special: Object.freeze([111, 141, 137]),
});

// 지면 타입(`isle`)의 번호이다. (2026-11-02 WB03-Y)
export const ISLE_TYPE = 97;

// 작은 받침 윗면 타입(`island`)의 번호이다. (2026-11-02 WB03-Y)
export const ISLAND_TYPE = 94;

// 다리 타입의 번호이다(일반 띠 표의 소유자 2·7 특례 대상). (2026-11-02 WB03-Y)
export const BRIDGE_TYPE = 82;

// 항등 변환표 9행을 만든다. 행 i 의 색 번호 c 는 c 로 간다. (2026-11-02 WB03-Y)
function identityMaps() {
    return Array.from({ length: OWNER_COLOR_COUNT + 1 }, () => Uint8Array.from({ length: OWNER_COLOR_ROW }, (_, color) => color));
}

// 명도 전용 표의 소스 색 번호 목록을 돌려준다. 94·97 → 지면 33개, 111·141·137 → 각 전용 목록이며 그 밖의 타입은 RangeError 이다.
// 사용: specialOwnerColorSources(97). (2026-11-02 WB03-Y)
export function specialOwnerColorSources(type) {
    if (type === 111) return SOURCES_111;
    if (type === 141) return SOURCES_141;
    if (type === 137) return SOURCES_137;
    if (type === ISLE_TYPE || type === ISLAND_TYPE) return SURFACE_SOURCES;
    throw new RangeError('전용 소유자 색표 타입이 아니다');
}

// 일반 띠 표(원본 43ba00)를 만든다: 항등 9행에서 소유자 1~8 행의 228~245 만 후보 색으로 바꾼다.
// 228~237 은 밝기 0, 238~245 는 245 − 색 번호(7~0)이며 type 이 bridgeType 이면 소유자 2 는 밝기 4, 소유자 7 은 밝기 3 으로 238~245 를 고정한다.
// 반환: Uint8Array(256) 9개의 배열. 사용: buildStandardOwnerColorMaps(164, 82). (2026-11-02 WB03-Y)
export function buildStandardOwnerColorMaps(type, bridgeType = BRIDGE_TYPE, candidates = OWNER_COLOR_CANDIDATES) {
    if (candidates.length < (OWNER_COLOR_COUNT + 1) * 8) throw new RangeError('소유자 색 후보는 72개여야 한다');
    const maps = identityMaps();
    // 소유자 색 번호 1~8 마다 띠를 바꾼다.
    for (let owner = 1; owner <= OWNER_COLOR_COUNT; owner += 1) {
        // 228~245 의 색 번호를 하나씩 바꾼다.
        for (let color = BAND_FIRST; color <= BAND_LAST; color += 1) {
            let shade = color < BAND_DARK_FIRST ? 0 : BAND_LAST - color;
            if (color >= BAND_DARK_FIRST && type === bridgeType && (owner === 2 || owner === 7)) shade = owner === 2 ? BRIDGE_SHADE_OWNER_TWO : BRIDGE_SHADE_OWNER_SEVEN;
            // 후보의 낮은 바이트만 쓴다(원본 DWORD 의 높은 비트는 색 번호가 아니다).
            maps[owner][color] = candidates[owner * 8 + shade] & 0xff;
        }
    }
    return maps;
}

// 논리 팔레트(R | G << 8 | B << 16 DWORD 256개)의 색 번호별 HSL 명도(double)를 구한다: 채널을 1/255 배한 값의 (최댓값 + 최솟값) × 0.5. (2026-11-02 WB03-Y)
export function paletteLightness(logical) {
    if (logical.length !== OWNER_COLOR_ROW) throw new RangeError('논리 팔레트는 256색이어야 한다');
    const light = new Float64Array(OWNER_COLOR_ROW);
    // 색 번호마다 정규화한 RGB 에서 명도를 계산한다.
    for (let i = 0; i < OWNER_COLOR_ROW; i += 1) {
        const red = (logical[i] & 255) * NORMALIZE;
        const green = ((logical[i] >>> 8) & 255) * NORMALIZE;
        const blue = ((logical[i] >>> 16) & 255) * NORMALIZE;
        light[i] = (Math.max(red, green, blue) + Math.min(red, green, blue)) * 0.5;
    }
    return light;
}

// 명도 전용 표(원본 43bbe0/43b750)를 만든다: 항등 9행에서 소유자 1~8 행의 소스 색 번호마다 `팔레트 명도 + 보정값` 에 가장 가까운 명도의 후보 색(소유자의 후보 8개 중)을 고른다.
// 거리는 double 로 비교하고 더 작을 때만 바꾸므로 같은 거리는 앞 후보이다(첫 후보의 거리는 99 로 자른다). 보정값은 타입 97 이 소유자별 `ISLE_OFFSETS`, 141 이 −0.15, 그 밖은 0 이다.
// logical 은 논리 팔레트 DWORD 256개, type 은 94·97·111·141·137 이다. 사용: buildSpecialOwnerColorMaps(palette.logical, ISLE_TYPE). (2026-11-02 WB03-Y)
export function buildSpecialOwnerColorMaps(logical, type, candidates = OWNER_COLOR_CANDIDATES) {
    if (candidates.length < (OWNER_COLOR_COUNT + 1) * 8) throw new RangeError('소유자 색 후보는 72개여야 한다');
    specialOwnerColorSources(type);
    return buildSpecialOwnerColorMapsFromLightness(paletteLightness(logical), type, candidates);
}

// 저장된 명도(색 번호 256개의 double)에서 바로 명도 전용 표를 만든다: 원본 실제 Client 의 입력은 ColorSort primary(RawColorSortPrimary)가 저장한 +8 double 이며 RGB 에서 다시 계산하지 않는다
// (cpppj `BuildSpecialOwnerColorMaps(const RawColorSortPrimary&, …)` 대응). light 는 길이 256 의 숫자 배열/Float64Array 이고 나머지는 위 함수와 같다.
// 사용: buildSpecialOwnerColorMapsFromLightness(Float64Array.from({ length: 256 }, (_, i) => primary.lightness(i)), 97). (2026-11-13 WB04-E374: 5-A OwnerColorTableState/ClientColorSortInput 이식에서 추가)
export function buildSpecialOwnerColorMapsFromLightness(light, type, candidates = OWNER_COLOR_CANDIDATES) {
    if (candidates.length < (OWNER_COLOR_COUNT + 1) * 8) throw new RangeError('소유자 색 후보는 72개여야 한다');
    if (!light || light.length !== OWNER_COLOR_ROW) throw new RangeError('저장 명도는 256색이어야 한다');
    const sources = specialOwnerColorSources(type);
    const maps = identityMaps();
    // 소유자 색 번호 1~8 마다 소스 색을 바꾼다.
    for (let owner = 1; owner <= OWNER_COLOR_COUNT; owner += 1) {
        const offset = type === ISLE_TYPE ? ISLE_OFFSETS[owner - 1] : type === 141 ? OFFSET_141 : 0.0;
        // 소스 색 번호를 원본 순서대로 처리한다.
        for (const source of sources) {
            const target = light[source] + offset;
            let selected = 0;
            let minimum = Math.min(DISTANCE_LIMIT, Math.abs(target - light[candidates[owner * 8]]));
            // 후보 1~7 은 엄격히 더 가까울 때만 바꾼다.
            for (let shade = 1; shade < 8; shade += 1) {
                const distance = Math.abs(target - light[candidates[owner * 8 + shade]]);
                if (distance < minimum) {
                    minimum = distance;
                    selected = shade;
                }
            }
            maps[owner][source] = candidates[owner * 8 + selected] & 0xff;
        }
    }
    return maps;
}

// 최초 등록(원본 43be30)과 배너 옵션(원본 43bc40) 뒤의 타입별 소유자 색표 모음을 만든다. 반환: Map(타입 번호 → Uint8Array(256) 9개).
//  - 일반 띠 표: OWNER_COLOR_TYPES.standard 12개 타입(skipFirstTwo 가 참이면 앞의 두 개는 만들지 않는다, 원본 59ab28 이 0 이 아닐 때).
//  - 명도 전용 표: 111·141·137.
//  - 배너 옵션 useBanners 가 0 이면(섬 소유자 색을 보임) 97(`isle`)·94(`island`) 의 명도 전용 표도 만든다. 0 이 아니면(F7 'Hide Island Ownership') 두 표가 없어 지면은 소유자 색으로 바뀌지 않는다.
// logical 은 논리 팔레트 DWORD 256개이다. 사용: createOwnerColorTables(palette.logical, { useBanners: 0 }).get(97)[owner]. (2026-11-02 WB03-Y)
export function createOwnerColorTables(logical, { useBanners = 0, bridgeType = BRIDGE_TYPE, skipFirstTwo = false } = {}) {
    const tables = new Map();
    // 일반 띠 표 12개를 원본 등록 순서대로 만든다.
    OWNER_COLOR_TYPES.standard.forEach((type, index) => {
        if (skipFirstTwo && index < 2) return;
        tables.set(type, buildStandardOwnerColorMaps(type, bridgeType));
    });
    // 명도 전용 표 3개를 만든다.
    for (const type of OWNER_COLOR_TYPES.special) tables.set(type, buildSpecialOwnerColorMaps(logical, type));
    // 배너 옵션이 꺼져 있으면 지면과 받침 윗면의 표도 만든다.
    if (useBanners === 0) {
        tables.set(ISLE_TYPE, buildSpecialOwnerColorMaps(logical, ISLE_TYPE));
        tables.set(ISLAND_TYPE, buildSpecialOwnerColorMaps(logical, ISLAND_TYPE));
    }
    return tables;
}

// 소유자 색 번호 color(0~8)로 색 번호 한 줄(Uint8Array)을 변환한 새 배열을 돌려준다. maps 는 위 함수들의 9행 표이다. 사용: applyOwnerColor(maps, 1, pixels). (2026-11-02 WB03-Y)
export function applyOwnerColor(maps, color, pixels) {
    if (!Number.isInteger(color) || color < 0 || color > OWNER_COLOR_COUNT) throw new RangeError('소유자 색 번호는 0~8 이어야 한다');
    const row = maps[color];
    const out = new Uint8Array(pixels.length);
    // 색 번호를 한 칸씩 바꾼다.
    for (let i = 0; i < pixels.length; i += 1) out[i] = row[pixels[i]];
    return out;
}
