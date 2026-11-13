// 원본 일반 자산 출력의 추상(밝음)·건설(어두움)·그림자 대상 색 변환표를 만드는 모듈이다(cpppj `client/SpriteColorMaps.cpp/.h`, 원본 전체 ColorSort 0043c010의 해당 세 출력 556238/556438/557738).
//  - buildSpriteColorMaps(logical): 논리 팔레트 256색(R/G/B/예약 순서 DWORD)에서 { bright, dead, shadow } 세 Uint8Array(256) 표를 만든다.
//    bright·dead 는 GUI 명암 표(assets/palette.js 의 buildShadeMaps: 밝음 1.5배·최대 1, 어두움 float 0.6배)를 그대로 쓰고,
//    shadow 는 소스 RGB 에 float 0.6(0.6000000238418579)을 곱한 목표와 모든 후보의 blue→green→red 제곱 거리를 엄격 비교해 고른다(최소 거리만 float32 로 저장).
//    원본 0043c010 끝에서 228..245 를 항등으로 덮고 254·255 를 명시 대입한다.
// 세 표는 같은 파일 팔레트에서 만들며 일시적인 날씨 RGB 변경에는 다시 계산하지 않는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E373: 5-A 색표 소단위 이식으로 추가)
import { buildShadeMaps } from '../assets/palette.js';

// 원본 정규화 double(1/255)과 float 0.6 을 double 로 승격한 그림자 계수, 팔레트 크기, 초기 최소 거리(float 1e6)이다. (2026-11-13 WB04-E373)
const NORMALIZE = 1.0 / 255.0;
const SHADOW = 0.6000000238418579;
const COLORS = 256;
const INITIAL_DISTANCE = Math.fround(1e6);
// 항등으로 덮는 예약 구간(228..245)과 마지막 두 명시 번호이다. (2026-11-13 WB04-E373)
const IDENTITY_FIRST = 228;
const IDENTITY_LAST = 245;

// 논리 RGB 256색으로 밝음·건설·그림자 세 표를 만든다. 사용: const { bright, dead, shadow } = buildSpriteColorMaps(logical). (2026-11-13 WB04-E373)
export function buildSpriteColorMaps(logical) {
    if (!logical || logical.length !== COLORS) throw new RangeError('색표 논리 팔레트는 256색이어야 합니다.');
    const shade = buildShadeMaps(logical);
    const shadow = new Uint8Array(COLORS);
    const rgb = new Float64Array(COLORS * 3);
    // ColorSort 가 저장한 성분별 double 을 예약 BYTE 를 제외하고 번호 순서로 만든다.
    for (let index = 0; index < COLORS; index += 1) {
        rgb[index * 3] = (logical[index] & 255) * NORMALIZE;
        rgb[index * 3 + 1] = ((logical[index] >>> 8) & 255) * NORMALIZE;
        rgb[index * 3 + 2] = ((logical[index] >>> 16) & 255) * NORMALIZE;
    }
    // 각 소스 RGB 를 어둡게 만든 목표와 모든 후보를 원본 순서로 비교한다.
    for (let index = 0; index < COLORS; index += 1) {
        const targetRed = rgb[index * 3] * SHADOW;
        const targetGreen = rgb[index * 3 + 1] * SHADOW;
        const targetBlue = rgb[index * 3 + 2] * SHADOW;
        let minimum = INITIAL_DISTANCE;
        // 원본 blue→green→red 합과 엄격한 비교 뒤 float32 저장을 유지한다.
        for (let candidate = 0; candidate < COLORS; candidate += 1) {
            const blue = targetBlue - rgb[candidate * 3 + 2];
            const green = targetGreen - rgb[candidate * 3 + 1];
            const red = targetRed - rgb[candidate * 3];
            const distance = blue * blue + green * green + red * red;
            if (distance < minimum) {
                minimum = Math.fround(distance);
                shadow[index] = candidate;
            }
        }
    }
    // 원본 0043c010 끝에서 18회에 걸쳐 228..245 를 항등으로 덮고 마지막 두 예약 번호도 명시 대입한다.
    for (let index = IDENTITY_FIRST; index <= IDENTITY_LAST; index += 1) shadow[index] = index;
    shadow[254] = 254;
    shadow[255] = 255;
    return Object.freeze({ bright: shade.bright, dead: shade.dark, shadow });
}
