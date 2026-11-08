// 원본 Colorsort.cpp 의 RGB 팔레트 색 띠와 Particle.cpp 의 색 표 병합을 옮긴 모듈이다(cpppj `o/ParticleGradient.cpp/.h`, 10.78 전용: 00439de0/0043ad20 색 띠·00487d20 색 표 병합·00457ea0 불꽃 꼬리 표·00458480 번개 표).
//  - buildParticleGradient(palette, from, to, colors, width = 2.4): RGBX DWORD 256개 팔레트에서 시작/끝 RGB(double 세 개)에 가장 가까운 색(B/G/R 제곱 순서의 합, 동률이면 먼저 나온 색)을 찾아 평행 이동 → 축(끝−시작) 투영(R/B/G 내적 ÷ 제곱 길이)과 B/G/R 거리 제곱 저장 →
//    원본 CRT 의 세 번 정렬(거리·원통 안팎/축 위치·선택 표식)과 원통 후보에서의 가장 가까운 위치 선택(중복 선택도 표식 1 유지)으로 색 띠를 만든다. colors 는 2..256, width 는 유한 양수이며 반올림 색 수가 256 미만이어야 하고 영길이 축은 거부한다.
//    결과 = { entries(256개 `{ translated[3], index, distance, position, selected }`, 원본 정렬 순서), from, to, count(요청 색 수), selected(원통 앞부분의 서로 다른 선택 수), threshold(원통 안팎을 나누는 제곱 거리, 0 이면 0.05) }.
//  - mergeParticleGradients(gradients): 최대 3개의 연속 색 띠에서 각 색 띠의 요청 색 수만큼 앞 항목 번호를 차례로 합친 DWORD 배열을 돌려준다(실제 선택 수가 아닌 요청 수 사용, 인접 할당 메모리는 포함하지 않음).
//  - buildFieryTailPalette(palette)·buildLightningPalette(palette): 실제 PE 의 RGB double 상수(불꽃: 어두운 빨강→주황 8색 + 주황→밝은색 3색, 번개: 파랑→흰색 8색 + 흰색→128/255 회색 3색, 폭 2.4)로 병합한 11개 색 번호이다.
// 원본 CRT 의 53비트 정밀도(027f) 연산/저장 순서를 쓰며 호출자의 팔레트를 변경하지 않는다. 이 모듈은 DOM 을 쓰지 않는다. (2026-11-09 WB04-E186: 입자 색 띠 이식으로 추가)

// 원본 팔레트 항목 수와 정렬 구간 스택 크기, 작은 구간 크기이다. (2026-11-09 WB04-E186)
const PALETTE_COLORS = 256;
const SMALL = 8;
const RANGES = 30;

// 비트(BigInt)를 double 로 바꾼다. (2026-11-09 WB04-E186)
function bitsDouble(bits) {
    const view = new DataView(new ArrayBuffer(8));
    view.setBigUint64(0, BigInt(bits), true);
    return view.getFloat64(0, true);
}

// 원본 RGB 정규화 double(1/255 의 이진 근사)과 최근접/선택 초기 거리, 0 임계값 대체, 마지막 선택 여유이다. (2026-11-09 WB04-E186)
const RGB_UNIT = bitsDouble(0x3f70101010101010n);
const NEAREST_LIMIT = 100000.0;
const SELECTION_LIMIT = 10000.0;
const FALLBACK_THRESHOLD = 0.05;
const SELECTION_EXTRA = 0.25;
// 실제 00509578/70/68/60 의 double 비트이다(두 색 띠가 시작 주황색을 공유). (2026-11-09 WB04-E186)
const FIERY_START = Object.freeze([1.0, bitsDouble(0x3fe8787878787878n), bitsDouble(0x3f98181818181818n)]);
const FIERY_DARK = Object.freeze([bitsDouble(0x3fe7373737373737n), 0.0, 0.0]);
const FIERY_LIGHT = Object.freeze([1.0, 1.0, bitsDouble(0x3fe5b5b5b5b5b5b6n)]);

// 세 번의 qsort 가 사용하는 서로 다른 비교 키이다. (2026-11-09 WB04-E186)
const SORT = Object.freeze({ distance: 0, cylinder: 1, selected: 2 });

// 원본은 차이를 double 로 구한 뒤 부호를 비교한다(NaN/동률은 0). (2026-11-09 WB04-E186)
const sign = (value) => (value < 0 ? -1 : value > 0 ? 1 : 0);

// 0043a0c0/0043a300/0043a3d0: 동률 처리와 선택되지 않은 항목의 0 비교를 보존한다. (2026-11-09 WB04-E186)
function compareEntries(a, b, key, threshold) {
    if (key === SORT.distance) return sign(a.distance - b.distance);
    if (key === SORT.cylinder) {
        if (a.distance >= threshold) return b.distance < threshold ? 1 : 0;
        if (b.distance >= threshold) return -1;
        return sign(a.position - b.position);
    }
    if (!a.selected) return b.selected ? 1 : 0;
    if (!b.selected) return -1;
    return sign(a.position - b.position);
}

// 004e6010/004e5fa0 의 피벗/교환 순서이다(80바이트 항목의 색/표식도 함께 교환). (2026-11-09 WB04-E186)
function sortEntries(entries, key, threshold) {
    const stack = [];
    let range = { low: 0, high: entries.length - 1 };
    // 주소 대신 현재 인덱스의 원소를 비교한다.
    const compare = (a, b) => compareEntries(entries[a], entries[b], key, threshold);
    // 원본 byte swap 과 같은 항목 전체 교환이다.
    const swap = (a, b) => { if (a !== b) { const held = entries[a]; entries[a] = entries[b]; entries[b] = held; } };
    // 큰 구간을 고정 스택에 보관하며 다음에 작은 구간을 처리한다.
    const push = (next) => { if (stack.length === RANGES) throw new Error('입자 색 정렬 스택 넘침'); stack.push(next); };
    // 현재 구간과 보관했던 구간이 모두 끝날 때까지 원본 분기 순서를 따른다.
    for (;;) {
        let proceed = false;
        if (range.high - range.low + 1 <= SMALL) {
            // small sort 는 최대 항목을 마지막으로 보내는 선택 정렬이다.
            while (range.low < range.high) {
                let maximum = range.low;
                // 엄격히 큰 항목만 최대 위치로 바꾼다.
                for (let current = range.low + 1; current <= range.high; current += 1) if (compare(current, maximum) > 0) maximum = current;
                swap(maximum, range.high);
                range.high -= 1;
            }
        } else {
            let middle = range.low + Math.trunc((range.high - range.low + 1) / 2);
            if (compare(range.low, middle) > 0) swap(range.low, middle);
            if (compare(range.low, range.high) > 0) swap(range.low, range.high);
            if (compare(middle, range.high) > 0) swap(middle, range.high);
            let left = range.low;
            let right = range.high;
            // 실제 CRT 처럼 피벗의 자기 비교를 건너뛰고 교차할 때까지 분할한다.
            for (;;) {
                if (left < middle) {
                    // 피벗 앞의 작은/동률 항목을 지난다.
                    do { left += 1; } while (left < middle && compare(left, middle) <= 0);
                }
                if (left >= middle) {
                    // 피벗 뒤의 작은/동률 항목을 끝까지 검사한다.
                    do { left += 1; } while (left <= range.high && compare(left, middle) <= 0);
                }
                // 오른쪽은 엄격히 큰 항목만 지난다.
                do { right -= 1; } while (right > middle && compare(right, middle) > 0);
                if (left > right) break;
                swap(left, right);
                if (middle === right) middle = left;
            }
            right += 1;
            if (right > middle) {
                // 피벗 오른쪽의 실제 0 비교 항목도 제거한다.
                do { right -= 1; } while (right > middle && compare(right, middle) === 0);
            }
            if (right <= middle) {
                // 피벗을 건너뛰고 왼쪽 끝까지 동률 구간을 제거한다.
                do { right -= 1; } while (right > range.low && compare(right, middle) === 0);
            }
            if (right - range.low < range.high - left) {
                if (left < range.high) push({ low: left, high: range.high });
                range.high = right;
                if (range.low < range.high) proceed = true;
            } else {
                if (range.low < right) push({ low: range.low, high: right });
                range.low = left;
                if (range.low < range.high) proceed = true;
            }
        }
        if (proceed) continue;
        if (!stack.length) return;
        range = stack.pop();
    }
}

// 원본 최근접 탐색은 B/G/R 의 제곱을 그 순서로 더한다. (2026-11-09 WB04-E186)
function distance(a, b) {
    const red = a[0] - b[0];
    const green = a[1] - b[1];
    const blue = a[2] - b[2];
    const first = blue * blue + green * green;
    return first + red * red;
}

// 정규화 → 가장 가까운 시작/끝 → 평행 이동 → 축 투영 → 원본 정렬/선택 순서이다. 사용: buildParticleGradient(palette, [0, 0, 1], [1, 1, 1], 8). (2026-11-09 WB04-E186)
export function buildParticleGradient(palette, from, to, colors, width = 2.4) {
    if (colors < 2 || colors > 256 || !Number.isFinite(width) || width <= 0) throw new RangeError('입자 색 띠 개수/폭 오류');
    if (!(palette.length >= PALETTE_COLORS)) throw new RangeError('입자 색 입력 팔레트 크기 오류');
    // 대상 RGB 는 실제 함수와 같이 최근접 색을 고르며 보드/색 범위로 임의 clamp 하지 않는다.
    for (let channel = 0; channel < 3; channel += 1) if (!Number.isFinite(from[channel]) || !Number.isFinite(to[channel])) throw new RangeError('입자 대상 RGB 비유한 값');
    const wanted = colors * width;
    if (!Number.isFinite(wanted) || wanted < 1 || wanted >= 255.5) throw new RangeError('입자 색 원통 후보 수 범위 오류');
    const rounded = Math.trunc(Math.fround(wanted) + 0.5);
    const cylinder = Math.trunc(wanted);
    if (rounded >= 256) throw new RangeError('입자 색 임계 항목 범위 오류');
    const result = { entries: [], from: -1, to: -1, count: colors, selected: 0, threshold: 0 };
    let bestFrom = NEAREST_LIMIT;
    let bestTo = NEAREST_LIMIT;
    // 원래 팔레트 번호와 RGB double 을 원본 순서로 만든다. 동일 거리에서는 먼저 나온 색을 유지한다.
    for (let index = 0; index < PALETTE_COLORS; index += 1) {
        const entry = { translated: [0, 0, 0], index, distance: 0, position: 0, selected: 0 };
        // 플래그 BYTE 는 RGB 계산에 참여하지 않는다.
        for (let channel = 0; channel < 3; channel += 1) entry.translated[channel] = ((palette[index] >>> (8 * channel)) & 255) * RGB_UNIT;
        const nearFrom = distance(entry.translated, from);
        const nearTo = distance(entry.translated, to);
        if (nearFrom < bestFrom) { bestFrom = nearFrom; result.from = index; }
        if (nearTo < bestTo) { bestTo = nearTo; result.to = index; }
        result.entries.push(entry);
    }
    if (result.from < 0 || result.to < 0) throw new RangeError('입자 시작/끝 최근접 색 없음');
    const origin = result.entries[result.from].translated.slice();
    // 각 RGB 차이를 double 로 저장한 뒤에만 축 벡터를 읽는다(원래 팔레트 순서로 뺀다).
    for (const entry of result.entries) for (let channel = 0; channel < 3; channel += 1) entry.translated[channel] -= origin[channel];
    const axis = result.entries[result.to].translated.slice();
    const firstNorm = axis[2] * axis[2] + axis[1] * axis[1];
    const norm = firstNorm + axis[0] * axis[0];
    if (!(norm > 0)) throw new Error('입자 색 띠 영길이 축 미복원');
    // 0043a660 의 R/B/G 내적과 B/G/R 제곱 거리 저장 순서이다.
    for (const entry of result.entries) {
        const color = entry.translated;
        const firstDot = axis[0] * color[0] + axis[2] * color[2];
        const dot = firstDot + axis[1] * color[1];
        entry.position = dot / norm;
        entry.distance = distance(color, [entry.position * axis[0], entry.position * axis[1], entry.position * axis[2]]);
    }
    sortEntries(result.entries, SORT.distance, 0);
    result.threshold = result.entries[rounded].distance;
    if (result.threshold === 0) result.threshold = FALLBACK_THRESHOLD;
    sortEntries(result.entries, SORT.cylinder, result.threshold);
    const step = 1.0 / (colors - 1);
    const stop = step * SELECTION_EXTRA + 1.0;
    // 같은 색이 여러 위치에 가장 가까울 수 있다. 선택 표식을 중복 가산하지 않는다.
    for (let position = 0; position < stop; position += step) {
        let best = SELECTION_LIMIT;
        let chosen = -1;
        // 동일 오차이면 앞 항목을 유지하는 원본 엄격한 비교이다.
        for (let index = 0; index < cylinder; index += 1) {
            const difference = Math.abs(position - result.entries[index].position);
            if (difference < best) { best = difference; chosen = index; }
        }
        if (chosen < 0) throw new Error('입자 색 띠 후보 선택 실패');
        result.entries[chosen].selected = 1;
    }
    // 실제 선택 수는 원통 후보 앞부분에서만 센다.
    for (let index = 0; index < cylinder; index += 1) result.selected += result.entries[index].selected !== 0 ? 1 : 0;
    sortEntries(result.entries, SORT.selected, result.threshold);
    result.entries[result.selected - 1].index = result.to;
    return result;
}

// 실제 Particle 팔레트 병합은 선택 수가 아닌 각 색 띠의 요청 수를 복사한다. (2026-11-09 WB04-E186)
export function mergeParticleGradients(gradients) {
    if (!gradients.length || gradients.length > 3) throw new RangeError('입자 색 띠 병합 개수 오류');
    const result = [];
    // null 구멍이 없는 연속 색 띠를 원본 순서대로 병합한다.
    for (const gradient of gradients) {
        if (gradient.count < 0 || gradient.count > 256) throw new RangeError('입자 색 띠 병합 길이 오류');
        // 요청 수가 실제 선택 수보다 커도 나머지 정렬 항목의 번호를 그대로 읽는다.
        for (let index = 0; index < gradient.count; index += 1) result.push(gradient.entries[index].index);
    }
    return result;
}

// 실제 생성자는 어두운 빨강 → 주황 8색, 주황 → 밝은색 3색을 순서대로 요청한다. (2026-11-09 WB04-E186)
export function buildFieryTailPalette(palette) {
    return mergeParticleGradients([buildParticleGradient(palette, FIERY_DARK, FIERY_START, 8), buildParticleGradient(palette, FIERY_START, FIERY_LIGHT, 3)]);
}

// 실제 빔의 두 색 띠는 0.5 가 아닌 PE binary64 의 128/255 회색을 사용한다(파랑 → 흰색 8색, 흰색 → 회색 3색, 원문 순서). (2026-11-09 WB04-E186)
export function buildLightningPalette(palette) {
    const blue = [0, 0, 1];
    const white = [1, 1, 1];
    const gray = [128.0 / 255.0, 128.0 / 255.0, 128.0 / 255.0];
    return mergeParticleGradients([buildParticleGradient(palette, blue, white, 8), buildParticleGradient(palette, white, gray, 3)]);
}
