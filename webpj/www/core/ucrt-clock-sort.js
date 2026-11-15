// cpppj 제품 1313의 시간 보정이 사용하는 Windows UCRT 10.0.19041.7802 qsort의 표본 순서를 재현한다.
// 사용: sortClockSamples([{ offset: 1, roundTrip: 0.5 }, ...]); 입력 배열을 제자리에서 바꾼다.
// 왕복 시간이 같은 항목은 안정 정렬하면 다른 절반이 선택될 수 있으므로 실제 native 순열과 대조한다.
// (2026-11-15 WB04-E516: RawClockSynchronization Poll의 최대 70표본 정렬용)

// 한 번에 보정할 수 있는 원본 표본의 최대 개수와 작은 구간의 정렬 기준이다. (2026-11-15 WB04-E516)
const MAX_CLOCK_SAMPLES = 70;
const SHORT_RANGE_LIMIT = 8;

// 원본 비교자는 두 ordered 비교가 모두 거짓이면 동률을 반환한다. (2026-11-15 WB04-E516)
function compareRoundTrips(first, second) {
    return first.roundTrip < second.roundTrip ? -1 : second.roundTrip < first.roundTrip ? 1 : 0;
}

// 배열의 두 자리를 맞바꾸며 같은 자리이면 아무것도 바꾸지 않는다. (2026-11-15 WB04-E516)
function swapSamples(samples, first, second) {
    if (first !== second) [samples[first], samples[second]] = [samples[second], samples[first]];
}

// 짧은 구간은 앞에서 처음 만난 최댓값을 뒤쪽으로 옮겨 정렬한다. (2026-11-15 WB04-E516)
function sortShortRange(samples, start, end) {
    // 오른쪽 끝의 위치를 하나씩 확정한다.
    for (let last = end; last > start; last -= 1) {
        let largest = start;
        // 같은 값은 앞서 발견한 항목을 유지한다.
        for (let index = start + 1; index <= last; index += 1) {
            if (compareRoundTrips(samples[index], samples[largest]) > 0) largest = index;
        }
        swapSamples(samples, largest, last);
    }
}

// 중간값 분할과 동률 구간을 같은 위치에 두고 양쪽 미정 구간을 정렬한다. (2026-11-15 WB04-E516)
function sortRange(samples, start, end) {
    if (end <= start) return;
    const count = end - start + 1;
    if (count <= SHORT_RANGE_LIMIT) {
        sortShortRange(samples, start, end);
        return;
    }
    let pivot = start + Math.floor(count / 2);
    if (compareRoundTrips(samples[start], samples[pivot]) > 0) swapSamples(samples, start, pivot);
    if (compareRoundTrips(samples[start], samples[end]) > 0) swapSamples(samples, start, end);
    if (compareRoundTrips(samples[pivot], samples[end]) > 0) swapSamples(samples, pivot, end);
    let left = start;
    let right = end;
    // 기준값보다 큰 왼쪽과 크지 않은 오른쪽을 교환한다.
    for (;;) {
        if (pivot > left) {
            do { left += 1; } while (left < pivot && compareRoundTrips(samples[left], samples[pivot]) <= 0);
        }
        if (pivot <= left) {
            do { left += 1; } while (left <= end && compareRoundTrips(samples[left], samples[pivot]) <= 0);
        }
        do { right -= 1; } while (right > pivot && compareRoundTrips(samples[right], samples[pivot]) > 0);
        if (right < left) break;
        swapSamples(samples, left, right);
        if (pivot === right) pivot = left;
    }
    right += 1;
    // 분할 기준과 같은 구간은 재정렬에서 제외한다.
    if (pivot < right) {
        do { right -= 1; } while (right > pivot && compareRoundTrips(samples[right], samples[pivot]) === 0);
    }
    if (pivot >= right) {
        do { right -= 1; } while (right > start && compareRoundTrips(samples[right], samples[pivot]) === 0);
    }
    sortRange(samples, start, right);
    sortRange(samples, left, end);
}

// 최대 70개의 offset·roundTrip 쌍을 native CRT와 같은 순서로 정렬한다. (2026-11-15 WB04-E516)
export function sortClockSamples(samples) {
    if (!Array.isArray(samples) || samples.length > MAX_CLOCK_SAMPLES) throw new RangeError('시간 표본의 실제 70개 범위');
    sortRange(samples, 0, samples.length - 1);
}
