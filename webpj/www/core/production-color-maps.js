// 원본 0043c010 의 생산 아이콘 비활성·선행(밑그림) 두 색 변환표를 만드는 모듈이다(cpppj `client/ProductionColorMaps.cpp/.h`, 출력 556038/556338).
//  - buildProductionColorMaps(logical): 논리 팔레트 256색에서 { disabled, underlay } 두 Uint8Array(256) 표를 만든다.
//    underlay(선행 표)는 원본 memset 의 0 이고, disabled 는 소스 빨강만 2.1배(최대 1)로 키운 목표와 모든 후보의 blue→green→red 제곱 거리를 엄격 비교해 고른다
//    (최소 거리만 float32 로 저장). 후보 번호는 정렬하지 않고 팔레트 순서를 유지한다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E373: 5-A 색표 소단위 이식으로 추가)

// 정규화 double(1/255)·빨강 확대 계수·팔레트 크기·초기 최소 거리(float 1e6)이다. (2026-11-13 WB04-E373)
const NORMALIZE = 1.0 / 255.0;
const RED_SCALE = 2.1;
const COLORS = 256;
const INITIAL_DISTANCE = Math.fround(1e6);

// 논리 RGB 256색으로 비활성·선행 표를 만든다. 사용: const { disabled, underlay } = buildProductionColorMaps(logical). (2026-11-13 WB04-E373)
export function buildProductionColorMaps(logical) {
    if (!logical || logical.length !== COLORS) throw new RangeError('색표 논리 팔레트는 256색이어야 합니다.');
    const disabled = new Uint8Array(COLORS);
    const underlay = new Uint8Array(COLORS);
    const rgb = new Float64Array(COLORS * 3);
    // 원본 ColorSort 의 정규화 성분을 번호 순서로 만든다.
    for (let index = 0; index < COLORS; index += 1) {
        rgb[index * 3] = (logical[index] & 255) * NORMALIZE;
        rgb[index * 3 + 1] = ((logical[index] >>> 8) & 255) * NORMALIZE;
        rgb[index * 3 + 2] = ((logical[index] >>> 16) & 255) * NORMALIZE;
    }
    // 원본은 소스 빨강만 확대한 뒤 1 로 제한한다.
    for (let index = 0; index < COLORS; index += 1) {
        const target = Math.min(1.0, rgb[index * 3] * RED_SCALE);
        let minimum = INITIAL_DISTANCE;
        // blue→green→red 제곱 합을 float 최소값과 엄격 비교한다.
        for (let candidate = 0; candidate < COLORS; candidate += 1) {
            const blue = rgb[index * 3 + 2] - rgb[candidate * 3 + 2];
            const green = rgb[index * 3 + 1] - rgb[candidate * 3 + 1];
            const red = target - rgb[candidate * 3];
            const distance = blue * blue + green * green + red * red;
            if (distance < minimum) {
                minimum = Math.fround(distance);
                disabled[index] = candidate;
            }
        }
    }
    return Object.freeze({ disabled, underlay });
}
