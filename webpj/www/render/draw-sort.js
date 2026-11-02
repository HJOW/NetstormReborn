// 10.78의 표시 비교(00497900)와 CRT qsort/shortsort(004e6010/004e5fa0)를 옮긴다.
// JS Array.sort는 완전 동률을 1로 반환하는 원본의 교환 순서를 보존하지 않는다.
// 사용: sortDrawEntries([{ x, y, depth, ...그림자료 }]); 항목 전체를 제자리 교환한다. (2026-11-02 WB03-Z)

// 원본은 8항목 이하에서 선택 정렬을 쓴다.
const SMALL_RANGE = 8;
// 원본 signed 바이트 거리와 30칸 스택의 최대 범위를 유지한다.
const MAX_ENTRIES = Math.floor(0x7fffffff / 16);
const MAX_STACK = 30;

// signed WORD 깊이 내림차순, float32 y/x 오름차순이다. 완전 동률도 1이다.
export function compareDrawOrder(a, b) {
    const depth = b.depth - a.depth;
    if (depth !== 0) return depth;
    const delta = Math.fround(a.y - b.y);
    return (delta === 0 ? Math.fround(a.x - b.x) : delta) < 0 ? -1 : 1;
}

// 두 위치의 객체 참조 전체를 옮겨 프레임·색표·종류·tail을 같은 항목에 남긴다.
function swap(entries, a, b) {
    if (a !== b) [entries[a], entries[b]] = [entries[b], entries[a]];
}

// 양 끝 포함 구간에서 최대 항목을 뒤에 확정한다. 같은 키의 비교도 생략하지 않는다.
function smallSort(entries, low, high) {
    // 마지막 항목을 확정할 때마다 오른쪽 끝을 줄인다.
    while (low < high) {
        let maximum = low;
        // 엄격히 양수인 비교만 최대 위치를 바꾼다.
        for (let current = low + 1; current <= high; current += 1) {
            if (compareDrawOrder(entries[current], entries[maximum]) > 0) maximum = current;
        }
        swap(entries, maximum, high);
        high -= 1;
    }
}

// 작은 구간부터 처리하도록 큰 구간을 원본 고정 스택에 보관한다.
function push(stack, low, high) {
    if (stack.length === MAX_STACK) throw new RangeError('표시 정렬 구간 스택 넘침');
    stack.push([low, high]);
}

// 10.78 정렬이다. 계약 오류는 교환 전에 거부한다. x/y는 저장된 float32, depth는 signed WORD여야 한다.
// CD판·다른 항목 폭·비유한 좌표의 정렬은 지원하지 않는다.
export function sortDrawEntries(entries) {
    if (entries.length > MAX_ENTRIES) throw new RangeError('표시 정렬 항목 수 범위 오류');
    // 잘못된 키가 뒤에 있어도 앞 항목을 먼저 바꾸지 않는다.
    for (const entry of entries) {
        if (!Number.isFinite(entry.x) || !Number.isFinite(entry.y) ||
            entry.x !== Math.fround(entry.x) || entry.y !== Math.fround(entry.y) ||
            !Number.isInteger(entry.depth) || entry.depth < -32768 || entry.depth > 32767) {
            throw new RangeError('표시 정렬 키 범위 오류');
        }
    }
    if (entries.length < 2) return entries;
    // 원본의 미처리 구간 스택과 현재 양 끝이다.
    const stack = [];
    let low = 0;
    let high = entries.length - 1;
    // 현재 구간 또는 저장 구간이 남아 있는 동안 반복한다.
    for (;;) {
        if (high - low + 1 <= SMALL_RANGE) smallSort(entries, low, high);
        else {
            let middle = low + Math.floor((high - low + 1) / 2);
            if (compareDrawOrder(entries[low], entries[middle]) > 0) swap(entries, low, middle);
            if (compareDrawOrder(entries[low], entries[high]) > 0) swap(entries, low, high);
            if (compareDrawOrder(entries[middle], entries[high]) > 0) swap(entries, middle, high);
            let left = low;
            let right = high;
            // 중앙값 피벗의 자기 비교를 생략하며 양쪽을 분할한다.
            for (;;) {
                if (left < middle) {
                    // 피벗 앞의 작은 항목을 지난다.
                    do { left += 1; } while (left < middle && compareDrawOrder(entries[left], entries[middle]) <= 0);
                }
                if (left >= middle) {
                    // 피벗 칸은 건너뛰고 끝 다음 칸에서는 비교하지 않는다.
                    do { left += 1; } while (left <= high && compareDrawOrder(entries[left], entries[middle]) <= 0);
                }
                // 오른쪽 커서는 피벗에서 멈춘다.
                do { right -= 1; } while (right > middle && compareDrawOrder(entries[right], entries[middle]) > 0);
                if (left > right) break;
                swap(entries, left, right);
                if (middle === right) middle = left;
            }
            right += 1;
            if (right > middle) {
                // 실제 0 비교만 제거한다. 표시 키의 완전 동률은 1이다.
                do { right -= 1; } while (right > middle && compareDrawOrder(entries[right], entries[middle]) === 0);
            }
            if (right <= middle) {
                // 피벗 칸을 생략하고 왼쪽 끝을 정한다.
                do { right -= 1; } while (right > low && compareDrawOrder(entries[right], entries[middle]) === 0);
            }
            if (right - low < high - left) {
                if (left < high) push(stack, left, high);
                high = right;
                if (low < high) continue;
            } else {
                if (low < right) push(stack, low, right);
                low = left;
                if (low < high) continue;
            }
        }
        if (stack.length === 0) return entries;
        [low, high] = stack.pop();
    }
}
