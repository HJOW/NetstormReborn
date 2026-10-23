// 원본 게임 팔레트(.COL)를 읽고, 원본 팔레트 로더가 계산하는 색 번호 검색·UI 명암 변환표를 복원하는 모듈이다.
// 원본은 8비트 색 번호 화면을 쓰므로 그림은 색 번호로 합성하고, 마지막에 이 팔레트의 RGB 로 바꿔 표시한다.
// 근거: 원본 004a4850(팔레트 파일 로더), 색 검색 몸체, 0043c010(GUI 명암 변환표 생성).
//       복원 기준은 cpppj 의 Screen.cpp(FindPaletteColor·GamePalette)·PaletteShade.cpp·GumpVisual.cpp 이다.
// (2026-10-23 WB03-M1: 메인 메뉴 화면 복원을 위해 추가)

// 팔레트의 색 개수이다. 원본 화면은 256색 인덱스 방식이다. (2026-10-23 WB03-M1)
export const PALETTE_COLORS = 256;

// Animator Pro .COL 파일의 크기(8바이트 머리 + 256×RGB)이다. 원본 로더는 머리 내용이 아니라 파일 크기로 형식을 고른다. (2026-10-23 WB03-M1)
const COL_FILE_SIZE = 0x308;

// Windows RGBQUAD(BGRX) 256개로 된 팔레트 파일의 크기이다. 원본 로더가 받는 두 번째 형식이다. (2026-10-23 WB03-M1)
const RGBQUAD_FILE_SIZE = 0x400;

// .COL 파일의 머리 크기(바이트)이다. RGB 표는 이 위치부터 시작한다. (2026-10-23 WB03-M1)
const COL_HEADER_SIZE = 8;

// 원본 색 검색이 처음 쓰는 float32 최소 거리 값이다. 실제 거리(최대 195,075)보다 크므로 첫 색이 항상 후보가 된다. (2026-10-23 WB03-M1)
const INITIAL_COLOR_DISTANCE = Math.fround(10000000);

// 원본 ColorSort 의 RGB 정규화 계수(double 1/255)이다. 명암 변환표 계산에서 각 채널에 곱한다. (2026-10-23 WB03-M1)
const SHADE_NORMALIZE = 1.0 / 255.0;

// 밝은 명암 목표 계수(각 채널 1.5배, 최대 1.0)이다. 돌 버튼의 밝은 테두리에 쓰는 표를 만든다. (2026-10-23 WB03-M1)
const SHADE_BRIGHT = 1.5;

// 어두운 명암 목표 계수이다. 원본의 float 0.6 을 double 로 승격한 값을 그대로 쓴다. (2026-10-23 WB03-M1)
const SHADE_DARK = 0.6000000238418579;

// 밝은 명암 후보에서 제외하는 색 번호 구간(229~245, 양 끝 포함)이다. 원본 생성 함수의 내부 검사와 같다. (2026-10-23 WB03-M1)
const BRIGHT_EXCLUDED_FIRST = 229;
const BRIGHT_EXCLUDED_LAST = 245;

// 명암 후보 검색의 float32 초기 최소 거리이다. 정규화 거리의 최댓값 3.0 보다 크다. (2026-10-23 WB03-M1)
const SHADE_INITIAL_DISTANCE = Math.fround(1e6);

// 돌 버튼 외곽선 색으로 찾는 원본 목표 RGB(55,51,54)이다. 원본 전역 005bec38 에 저장되는 색이다. (2026-10-23 WB03-M1)
const BUTTON_EDGE_RGB = Object.freeze([55, 51, 54]);

// 원본 게임 팔레트 한 벌이다. rgb 는 번호 순서의 R,G,B 바이트 768개, logical 은 원본 PALETTEENTRY 와 같은
// R | G<<8 | B<<16 형태의 DWORD 256개다. parse 로 파일에서 만들고, 색 검색·명암 표·화면 출력에 넘긴다.
// (2026-10-23 WB03-M1)
export class GamePalette {
    // RGB 바이트 768개로 팔레트를 만든다. 보통은 GamePalette.parse(bytes) 를 쓴다.
    constructor(rgb) {
        if (!(rgb instanceof Uint8Array) || rgb.length !== PALETTE_COLORS * 3) throw new TypeError('GamePalette: RGB 768바이트가 필요합니다.');
        // 번호 순서의 R,G,B 바이트(768개)다.
        this.rgb = rgb;
        // 원본 논리 팔레트(PALETTEENTRY)와 같은 R | G<<8 | B<<16 DWORD 256개다. 색 검색이 읽는다.
        this.logical = new Uint32Array(PALETTE_COLORS);
        // 각 색 번호의 DWORD 를 만든다.
        for (let i = 0; i < PALETTE_COLORS; i += 1) {
            this.logical[i] = (rgb[i * 3] | (rgb[i * 3 + 1] << 8) | (rgb[i * 3 + 2] << 16)) >>> 0;
        }
    }

    // 팔레트 파일 바이트를 읽는다. 0x308 바이트는 .COL(RGB), 0x400 바이트는 RGBQUAD(BGRX)이며 그 밖의 크기는 오류다.
    // 사용: GamePalette.parse(await fetchBytes('d/GIFCLOUD.COL'))
    static parse(bytes) {
        const rgb = new Uint8Array(PALETTE_COLORS * 3);
        if (bytes.length === COL_FILE_SIZE) {
            rgb.set(bytes.subarray(COL_HEADER_SIZE, COL_HEADER_SIZE + PALETTE_COLORS * 3));
        } else if (bytes.length === RGBQUAD_FILE_SIZE) {
            // BGRX 순서의 네 바이트를 RGB 세 바이트로 옮긴다.
            for (let i = 0; i < PALETTE_COLORS; i += 1) {
                rgb[i * 3] = bytes[i * 4 + 2];
                rgb[i * 3 + 1] = bytes[i * 4 + 1];
                rgb[i * 3 + 2] = bytes[i * 4];
            }
        } else {
            throw new Error(`지원하지 않는 팔레트 파일 크기입니다: ${bytes.length}`);
        }
        return new GamePalette(rgb);
    }
}

// 원본 화면 팔레트 적용(004a2640, 파일 로더 004a4850 이 (0,256,0,1)로 호출)이 고정하는 0번(검정)·255번(흰색)을 반영한
// 화면 팔레트를 만든다. 원본은 논리 팔레트와 DIB 색 표 모두에서 이 두 칸을 덮어쓰므로, 화면 표시·색 검색·명암 표는
// 파일 값이 아니라 이 팔레트를 써야 한다(GIFCLOUD.COL 의 255번은 청록색이지만 화면에서는 흰색이다).
// 이 규칙을 넣은 명암 표는 원본 캐시 d/!color.dat 의 밝음·어두움 표 512바이트와 일치한다.
// 사용: const palette = buildScreenPalette(GamePalette.parse(bytes)) (2026-10-23 WB03-M1)
export function buildScreenPalette(filePalette) {
    const rgb = filePalette.rgb.slice();
    rgb.set([0, 0, 0], 0);
    rgb.set([255, 255, 255], (PALETTE_COLORS - 1) * 3);
    return new GamePalette(rgb);
}

// 논리 팔레트에서 목표 RGB 에 가장 가까운 색 번호를 찾는다(원본 색 검색 몸체).
// 거리는 각 채널 차이 제곱의 합이고, 번호 0부터 차례로 보며 더 작을 때만 바꾸므로 동률이면 앞 번호가 남는다.
// 사용: 버튼 외곽선처럼 원본이 RGB 로 지정한 UI 색의 번호를 구할 때 쓴다. (2026-10-23 WB03-M1)
export function findPaletteColor(logical, red, green, blue) {
    let best = INITIAL_COLOR_DISTANCE;
    let chosen = 0;
    // 256색을 번호 순으로 검사한다. 예약 바이트는 거리 계산에 쓰지 않는다.
    for (let i = 0; i < logical.length; i += 1) {
        const color = logical[i];
        const dr = (color & 255) - red;
        const dg = ((color >>> 8) & 255) - green;
        const db = ((color >>> 16) & 255) - blue;
        const distance = dr * dr + dg * dg + db * db;
        if (distance < best) {
            best = Math.fround(distance);
            chosen = i;
        }
    }
    return chosen;
}

// 돌 버튼이 쓰는 세 색 번호(검정 글자 그림자·흰 글자·외곽선)를 찾는다.
// 원본 팔레트 로더가 기본 검정(0,0,0)·흰색(255,255,255)·외곽선 RGB(55,51,54)를 검색해 전역에 저장한 값과 같다.
// 사용: buttonTextContexts·planButtonDraw 에 넘긴다. (2026-10-23 WB03-M1)
export function buildButtonPalette(logical) {
    return Object.freeze({
        black: findPaletteColor(logical, 0, 0, 0),
        white: findPaletteColor(logical, 255, 255, 255),
        edge: findPaletteColor(logical, BUTTON_EDGE_RGB[0], BUTTON_EDGE_RGB[1], BUTTON_EDGE_RGB[2]),
    });
}

// GUI 질감 명암에 쓰는 밝은/어두운 색 번호 변환표 두 개를 만든다(원본 0043c010).
// 각 색의 목표 RGB(밝음 1.5배·최대 1, 어두움 0.6배)를 double 로 계산하고, 채널 차이 절댓값의 합이 가장 작은 번호를 고른다.
// 최소 거리만 float32 로 저장하는 원본 비교 방식을 그대로 따르며, 밝은 표는 229~245 번을 후보에서 뺀다.
// 반환: { bright: Uint8Array(256), dark: Uint8Array(256) }. 원본 기본 팔레트의 결과는 원본 캐시 !color.dat 과 같다.
// (2026-10-23 WB03-M1)
export function buildShadeMaps(logical) {
    const rgb = new Float64Array(PALETTE_COLORS * 3);
    // 예약 바이트를 뺀 R/G/B 를 원본과 같은 double 값으로 정규화해 둔다.
    for (let i = 0; i < PALETTE_COLORS; i += 1) {
        rgb[i * 3] = (logical[i] & 255) * SHADE_NORMALIZE;
        rgb[i * 3 + 1] = ((logical[i] >>> 8) & 255) * SHADE_NORMALIZE;
        rgb[i * 3 + 2] = ((logical[i] >>> 16) & 255) * SHADE_NORMALIZE;
    }
    const bright = new Uint8Array(PALETTE_COLORS);
    const dark = new Uint8Array(PALETTE_COLORS);
    // 소스 색 번호마다 두 목표 색을 각각 검색한다.
    for (let i = 0; i < PALETTE_COLORS; i += 1) {
        const br = Math.min(1.0, rgb[i * 3] * SHADE_BRIGHT);
        const bg = Math.min(1.0, rgb[i * 3 + 1] * SHADE_BRIGHT);
        const bb = Math.min(1.0, rgb[i * 3 + 2] * SHADE_BRIGHT);
        const dr = rgb[i * 3] * SHADE_DARK;
        const dg = rgb[i * 3 + 1] * SHADE_DARK;
        const db = rgb[i * 3 + 2] * SHADE_DARK;
        let brightMinimum = SHADE_INITIAL_DISTANCE;
        let darkMinimum = SHADE_INITIAL_DISTANCE;
        // 후보 번호를 0부터 차례로 비교한다. 거리는 double 로 비교하고 최소 거리만 float32 로 좁혀 저장한다.
        for (let candidate = 0; candidate < PALETTE_COLORS; candidate += 1) {
            const cr = rgb[candidate * 3];
            const cg = rgb[candidate * 3 + 1];
            const cb = rgb[candidate * 3 + 2];
            const brightDistance = Math.abs(br - cr) + Math.abs(bg - cg) + Math.abs(bb - cb);
            const darkDistance = Math.abs(dr - cr) + Math.abs(dg - cg) + Math.abs(db - cb);
            if (brightDistance < brightMinimum && (candidate < BRIGHT_EXCLUDED_FIRST || candidate > BRIGHT_EXCLUDED_LAST)) {
                brightMinimum = Math.fround(brightDistance);
                bright[i] = candidate;
            }
            if (darkDistance < darkMinimum) {
                darkMinimum = Math.fround(darkDistance);
                dark[i] = candidate;
            }
        }
    }
    return Object.freeze({ bright, dark });
}

// 실행 환경이 리틀 엔디언인지 검사한다. ImageData 를 Uint32 로 쓸 때 바이트 순서를 맞추기 위해 쓴다. (2026-10-23 WB03-M1)
function isLittleEndian() {
    return new Uint8Array(new Uint32Array([1]).buffer)[0] === 1;
}

// 팔레트 RGB 를 ImageData 의 Uint32 보기에 바로 쓸 수 있는 불투명 RGBA 값 256개로 바꾼다.
// 사용: 색 번호 화면을 표시할 때 rgba[색번호] 를 Uint32Array(imageData.data.buffer) 에 쓴다. (2026-10-23 WB03-M1)
export function buildRgbaLookup(rgb) {
    const table = new Uint32Array(PALETTE_COLORS);
    const little = isLittleEndian();
    // 색 번호마다 R,G,B,A(255) 네 바이트를 실행 환경의 바이트 순서에 맞춰 묶는다.
    for (let i = 0; i < PALETTE_COLORS; i += 1) {
        const r = rgb[i * 3];
        const g = rgb[i * 3 + 1];
        const b = rgb[i * 3 + 2];
        table[i] = little ? ((255 << 24) | (b << 16) | (g << 8) | r) >>> 0 : ((r << 24) | (g << 16) | (b << 8) | 255) >>> 0;
    }
    return table;
}
