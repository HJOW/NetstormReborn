// Factory 와 전체 지식 창이 공유하는 원본 70바이트 행의 의미 필드 비교와 CRT 정렬을 옮긴다(원문 00453f70/004e6010, cpppj client/KnowledgeSort.cpp/.h, 기준선 1f00a25a, 10.78 전용).
// 행은 { type(+0 TYPE 번호), available(+8), inProduction(+c), mana(+14 문자열), description(+1e 문자열) } 이며 포인터나 원문 메모리 배치는 노출하지 않는다. 문자열은 바이트 문자열이다.
// 비교는 첫 공통 우선 flags2 분류(한쪽에만 있으면 즉시 결정, 둘 다 있으면 뒤 플래그를 보지 않음) 뒤 설명을 C locale ASCII(대문자만 소문자로 접음)로 비교한다. 정렬은 JS Array.sort 가 아니라 원본 CRT qsort 의 피벗·동률 교환 순서를 그대로 따른다
// (같은 분류/설명의 동률 항목의 상대 순서가 원본과 같아야 하기 때문이다). 전용 PE fixture 는 없다(cpppj 에도 직접 시험이 없다). 기준선 뒤 변경도 없다.
// 사용: sortKnowledgeRecords(rows, typeRecords)  // typeRecords 는 buildTypeRecords 의 레코드 배열(flags2). compareKnowledgeRecords(a, b, typeRecords) → 음수/0/양수.
// (2026-11-14 WB04-E448: RawMenuControls 선행 지식 정렬 이식)

// 453f70 이 첫 공통 플래그에서 멈추는 우선 순서(flags2 비트)다.
const PRIORITY = [0x800, 0x8000000, 0x10000, 0x4000000, 0x20000000, 0x40000, 0x20000];
// 원본 CRT 가 shortsort 를 선택하는 포함 구간 길이와 구간 스택의 최대 쌍 개수다.
const SMALL_RANGE = 8; const MAX_RANGES = 30;

// 설명 문자열의 index 번째 바이트(범위 밖은 NUL=0)다. 대문자 A..Z 만 소문자로 접는다.
function foldedByte(text, index) {
    let value = index < text.length ? text.charCodeAt(index) & 0xff : 0;
    if (value >= 65 && value <= 90) value += 32;
    return value;
}

// 00453f70: 두 행의 TYPE(범위 밖이면 RangeError)의 flags2 를 우선 순서대로 보며 한쪽에만 켜진 첫 비트에서 결정하고(앞쪽에만 있으면 -1, 뒤쪽에만 있으면 1, 둘 다 있으면 분류를 끝내고 설명 비교로 간다),
// 같은 분류는 첫 NUL 까지의 부호 없는 ASCII 차이(대문자만 소문자로)를 돌려준다. 두 설명이 NUL 까지 같으면 0 이다.
export function compareKnowledgeRecords(first, second, types) {
    if (first.type >= types.length || second.type >= types.length) throw new RangeError('지식 정렬 TYPE 범위');
    const a = types[first.type].flags2 >>> 0; const b = types[second.type].flags2 >>> 0;
    // 한쪽에만 우선 플래그가 있으면 즉시 반환한다. 둘 다 있으면 뒤 플래그를 보지 않는다.
    for (const bit of PRIORITY) { if (a & bit) { if (!(b & bit)) return -1; break; } if (b & bit) return 1; }
    // 첫 NUL 까지의 부호 없는 ASCII 차이를 반환한다.
    for (let index = 0; ; index += 1) {
        const x = foldedByte(first.description, index); const y = foldedByte(second.description, index);
        if (x !== y) return x - y;
        if (!x) return 0;
    }
}

// 004e6010: 원본 CRT 의 작은 구간 선택(최대 항목을 끝으로 보내는 선택 정렬)/중앙 피벗/동률 제거/큰 구간 보관을 같은 순서로 실행한다. 비교는 항상 현재 인덱스의 원소로 한다. 구간 스택이 30쌍을 넘으면 Error 다(C++ logic_error).
export function sortKnowledgeRecords(entries, types) {
    const stack = []; let low = 0; let high = entries.length - 1;
    // 주소 대신 현재 인덱스의 원소를 비교한다.
    const compare = (a, b) => compareKnowledgeRecords(entries[a], entries[b], types);
    // 원본 byte swap 과 같은 항목 전체 교환이다.
    const swap = (a, b) => { if (a !== b) [entries[a], entries[b]] = [entries[b], entries[a]]; };
    // 큰 구간을 고정 스택에 보관하며 다음에 작은 구간을 처리한다.
    const push = (rangeLow, rangeHigh) => { if (stack.length === MAX_RANGES) throw new Error('Factory 지식 정렬 스택 넘침'); stack.push([rangeLow, rangeHigh]); };
    // 현재 구간과 보관했던 구간이 모두 끝날 때까지 원본 분기 순서를 따른다.
    for (;;) {
        if (high - low + 1 <= SMALL_RANGE) {
            // small sort 는 최대 항목을 마지막으로 보내는 선택 정렬이다.
            while (low < high) {
                let maximum = low;
                // 엄격히 큰 항목만 최대 위치로 바꾼다.
                for (let current = low + 1; current <= high; current += 1) if (compare(current, maximum) > 0) maximum = current;
                swap(maximum, high); high -= 1;
            }
        } else {
            let middle = low + Math.floor((high - low + 1) / 2);
            if (compare(low, middle) > 0) swap(low, middle);
            if (compare(low, high) > 0) swap(low, high);
            if (compare(middle, high) > 0) swap(middle, high);
            let left = low; let right = high;
            // 실제 CRT 처럼 피벗의 자기 비교를 건너뛰고 교차할 때까지 분할한다.
            for (;;) {
                if (left < middle) {
                    // 피벗 앞의 작은/동률 항목을 지난다.
                    do { left += 1; } while (left < middle && compare(left, middle) <= 0);
                }
                if (left >= middle) {
                    // 피벗 뒤의 작은/동률 항목을 끝까지 검사한다.
                    do { left += 1; } while (left <= high && compare(left, middle) <= 0);
                }
                // 오른쪽은 엄격히 큰 항목만 지난다.
                do { right -= 1; } while (right > middle && compare(right, middle) > 0);
                if (left > right) break;
                swap(left, right); if (middle === right) middle = left;
            }
            right += 1;
            if (right > middle) {
                // 피벗 오른쪽의 실제 0 비교 항목도 제거한다.
                do { right -= 1; } while (right > middle && compare(right, middle) === 0);
            }
            if (right <= middle) {
                // 피벗을 건너뛰고 왼쪽 끝까지 동률 구간을 제거한다.
                do { right -= 1; } while (right > low && compare(right, middle) === 0);
            }
            if (right - low < high - left) {
                if (left < high) push(left, high);
                high = right;
                if (low < high) continue;
            } else {
                if (low < right) push(low, right);
                low = left;
                if (low < high) continue;
            }
        }
        if (stack.length === 0) return;
        [low, high] = stack.pop();
    }
}
