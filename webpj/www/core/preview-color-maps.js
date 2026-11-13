// 원본 0043c010 의 커서 미리보기 파괴/불법/글자 그림자 세 색 변환표를 만드는 모듈이다(cpppj `client/PreviewColorMaps.cpp/.h`).
//  - buildPreviewColorMaps(logical, red): 논리 팔레트 256색과 현재 기본 빨강 번호(하위 BYTE 만 쓴다)로 { destroy, illegal, labelShadow } 세 Uint8Array(256) 표를 만든다.
//    destroy 는 모든 소스 번호를 red 의 하위 BYTE 로, labelShadow 는 모두 0 으로 채운다(원본 그대로). illegal 은 소스 빨강만 2.1배(최대 1)로 키운 목표와 모든 후보의
//    blue→green→red 제곱 거리를 엄격 비교해 고르며 최소 거리만 float32 로 저장한다(동률 선택을 단순화하지 않는다).
// 원본 ColorSort 와 같은 double 정규 RGB 및 float 최소 거리 저장을 쓴다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-13 WB04-E373: 5-A 색표 소단위 이식으로 추가)

// 정규화 double(1/255)·불법 표의 빨강 확대 계수(506708)·팔레트 크기·초기 최소 거리(float 1e6)이다. (2026-11-13 WB04-E373)
const NORMALIZE = 1.0 / 255.0;
const ILLEGAL_RED = 2.1;
const COLORS = 256;
const INITIAL_DISTANCE = Math.fround(1e6);

// 논리 RGB 256색과 기본 빨강으로 파괴·불법·글자 그림자 표를 만든다. 사용: buildPreviewColorMaps(logical, 189). (2026-11-13 WB04-E373)
export function buildPreviewColorMaps(logical, red) {
    if (!logical || logical.length !== COLORS) throw new RangeError('색표 논리 팔레트는 256색이어야 합니다.');
    const destroy = new Uint8Array(COLORS).fill(red & 0xff);
    const illegal = new Uint8Array(COLORS);
    const labelShadow = new Uint8Array(COLORS);
    const rgb = new Float64Array(COLORS * 3);
    // 예약 BYTE 를 제외하고 각 성분을 double 로 저장한다.
    for (let index = 0; index < COLORS; index += 1) {
        rgb[index * 3] = (logical[index] & 255) * NORMALIZE;
        rgb[index * 3 + 1] = ((logical[index] >>> 8) & 255) * NORMALIZE;
        rgb[index * 3 + 2] = ((logical[index] >>> 16) & 255) * NORMALIZE;
    }
    // 각 원본 색의 빨강만 확대하고 나머지 두 성분은 유지한다.
    for (let index = 0; index < COLORS; index += 1) {
        const target = Math.min(1.0, rgb[index * 3] * ILLEGAL_RED);
        let minimum = INITIAL_DISTANCE;
        // 엄격한 double 비교 뒤 최소 거리만 float32 로 좁힌다.
        for (let candidate = 0; candidate < COLORS; candidate += 1) {
            const blue = rgb[index * 3 + 2] - rgb[candidate * 3 + 2];
            const green = rgb[index * 3 + 1] - rgb[candidate * 3 + 1];
            const difference = target - rgb[candidate * 3];
            const distance = blue * blue + green * green + difference * difference;
            if (distance < minimum) {
                minimum = Math.fround(distance);
                illegal[index] = candidate;
            }
        }
    }
    return Object.freeze({ destroy, illegal, labelShadow });
}
