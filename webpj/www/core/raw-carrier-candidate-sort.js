// 골렘(Carrier) 후보 평가기가 사용하는 원본 비교기(428020)와 CRT qsort(4e6010/4e5fa0)의 동률 순서를 옮긴 모듈이다(cpppj `o/RawCarrierCandidateSort.cpp/.h`, 10.78 전용).
// 표준 라이브러리 정렬의 동률 순서는 원본 CRT 와 다르므로 실제 제어 흐름(`goto` 를 구조화한 같은 분기)을 그대로 쓴다.
//  - compare(firstAddress, secondAddress): 428020 은 요소의 SID 값이 아니라 qsort 가 전달한 **요소 주소**를 SID 처럼 계산한다. 인자를 역참조하지 않고 `주소×50 + 풀 시작 주소`를 32비트로 감아 만든 두 주소가 풀의 [시작+50, 시작+용량×50) 안에 있는지 보고
//    (둘 다 밖이면 0, 첫째만 밖이면 1, 둘째만 밖이면 −1) 모두 안이면 현재 raw 의 +8 위치 unsigned WORD 차이를 돌려준다(슬롯 중간을 가리키는 감김 주소도 나눗셈으로 정렬하지 않고, 실제 확보 범위 밖 읽기는 거부).
//  - sort(candidates, onCompare): 폭 4 의 CRT qsort 이다. 8개 이하 구간은 엄격한 `>` 로 최대 위치를 고른 뒤 끝과 교환하는 선택 정렬(동률도 회전), 큰 구간은 3점 중앙 pivot·pivot 앞/뒤 전진 조건·pivot 교환 뒤 재배치·동률 구간 제외를 거쳐
//    30쌍 스택에 큰 구간을 두고 작은 구간부터 처리한다. 후보 값(SID)은 전혀 읽지 않는다.
// 웹에는 호스트 포인터가 없으므로 원본 32비트 주소는 명시 입력이다: poolBase = 풀 시작 주소(비교 때 사용), arrayBase = 정렬 배열 시작 주소(요소 i 의 주소는 arrayBase + 4i), onCompare(첫 주소, 둘째 주소)는 sort 의 두 번째 인자로 주는 비교 호출 관찰 훅(선택)이다.
// 이 모듈은 DOM 을 쓰지 않는다. (2026-11-08 WB04-E90: 운반 후보 비교기/정렬 이식으로 추가)

// 풀 슬롯 폭, 비교하는 WORD 위치, 보류 구간 스택의 크기, 정렬 가능한 최대 요소 수, 작은 구간 선택 정렬 한도이다. (2026-11-08 WB04-E90)
const SLOT = 50;
const WORD_AT = 8;
const STACK_SIZE = 30;
const MAX_ELEMENTS = 0x3fffffff;
const SMALL_RANGE = 8;

export class RawCarrierCandidateSort {
    // 매 비교마다 현재 물리 raw/용량을 읽는 동일 월드와 원본 32비트 주소 입력(풀 시작·정렬 배열 시작)이다.
    #pool;
    #poolBase;
    #arrayBase;

    // 10.78 의 50바이트 슬롯 산술만 허용한다. 주소는 32비트로 감긴다. (2026-11-08 WB04-E90)
    constructor(pool, { poolBase = 0x10000000, arrayBase = 0x20000000 } = {}) {
        if (pool?.edition !== 'patch1078') throw new TypeError('운반 후보 정렬의 판본 오류');
        this.#pool = pool;
        this.#poolBase = poolBase >>> 0;
        this.#arrayBase = arrayBase >>> 0;
    }

    // 풀의 물리 바이트 하나를 슬롯 경계를 넘어 읽는다(슬롯 번호 = offset / 50, 범위 밖은 거부). (2026-11-08 WB04-E90)
    #byte(offset) {
        const sid = Math.floor(offset / SLOT);
        if (sid >= this.#pool.capacity) throw new RangeError('운반 후보 비교기의 raw WORD 범위 오류');
        return this.#pool.physical(sid)[offset % SLOT];
    }

    // 후보 값/슬롯 정렬 의도로 고치지 않고 실제 unsigned 주소 비교와 WORD 차이를 보존한다(428020 전체). (2026-11-08 WB04-E90)
    compare(firstAddress, secondAddress) {
        const base = this.#poolBase;
        const first = (Math.imul(firstAddress >>> 0, SLOT) + base) >>> 0;
        const second = (Math.imul(secondAddress >>> 0, SLOT) + base) >>> 0;
        const low = (base + SLOT) >>> 0;
        const high = (base + this.#pool.capacity * SLOT) >>> 0;
        const validFirst = first >= low && first < high;
        const validSecond = second >= low && second < high;
        if (!validFirst && !validSecond) return 0;
        if (!validFirst) return 1;
        if (!validSecond) return -1;
        // 감김으로 슬롯 중간을 가리키는 주소도 나눗셈으로 정렬하지 않는다.
        const word = (address) => {
            const offset = ((address - base) >>> 0) + WORD_AT;
            return this.#byte(offset) | (this.#byte(offset + 1) << 8);
        };
        const secondWord = word(second);
        return word(first) - secondWord;
    }

    // 4e6010/4e5fa0 의 폭 4 정렬이다. 같은 키의 교환·pivot 이동·작은 구간 처리도 원본 순서이다. candidates 는 Uint32Array 이다. (2026-11-08 WB04-E90)
    sort(candidates, onCompare = null) {
        if (candidates.length < 2) return;
        if (candidates.length > MAX_ELEMENTS) throw new RangeError('운반 후보 정렬의 32비트 폭 초과');
        const arrayBase = this.#arrayBase;
        // 비교 인자는 배열 요소의 주소이다. 평가 순서를 명시해 관측용 훅도 원본 호출 순서로 받는다.
        const compare = (first, second) => {
            const firstAddress = (arrayBase + 4 * first) >>> 0;
            const secondAddress = (arrayBase + 4 * second) >>> 0;
            if (onCompare) onCompare(firstAddress, secondAddress);
            return this.compare(firstAddress, secondAddress);
        };
        // 폭 4 의 바이트 교환과 같은 DWORD 교환이며 비교기는 교환 뒤 현재 위치를 다시 받는다.
        const swap = (first, second) => {
            if (first !== second) { const held = candidates[first]; candidates[first] = candidates[second]; candidates[second] = held; }
        };
        // 원본 두 30항목 스택의 경계와 아직 처리하지 않은 큰 구간의 수, 현재 구간의 포함 양끝이다.
        const pending = [];
        let low = 0;
        let high = candidates.length - 1;
        // 정렬이 끝난 구간은 스택에서 다음 구간을 꺼내고, 두 구간이면 작은 쪽부터 반복한다.
        for (;;) {
            if (high - low + 1 <= SMALL_RANGE) {
                // 4e5fa0 은 엄격한 > 로 최대 위치를 고른 뒤 끝과 교환하므로 동률도 회전한다.
                for (let last = high; last > low; last -= 1) {
                    let maximum = low;
                    // 현재 마지막 위치까지 비교하며 SID 값은 전혀 읽지 않는다.
                    for (let probe = low + 1; probe <= last; probe += 1) if (compare(probe, maximum) > 0) maximum = probe;
                    swap(maximum, last);
                }
            } else {
                // 세 위치의 중앙 pivot 이며 교환 뒤 위치가 바뀔 수 있다.
                let middle = low + Math.trunc((high - low + 1) / 2);
                if (compare(low, middle) > 0) swap(low, middle);
                if (compare(low, high) > 0) swap(low, high);
                if (compare(middle, high) > 0) swap(middle, high);
                let left = low;
                let right = high;
                // 4e6140: pivot 앞/뒤의 전진 조건과 pivot 교환 뒤 재배치를 보존한다.
                for (;;) {
                    let skipForward = false;
                    if (middle > left) {
                        // pivot 전까지는 <= 인 요소를 지나간다.
                        do { left += 1; } while (left < middle && compare(left, middle) <= 0);
                        if (middle > left) skipForward = true;
                    }
                    // pivot 을 만났거나 지났으면 상한까지 같은 조건으로 전진한다.
                    if (!skipForward) do { left += 1; } while (left <= high && compare(left, middle) <= 0);
                    // 오른쪽은 pivot 위치에서 멈추며 엄격한 > 인 위치만 지나간다.
                    do { right -= 1; } while (right > middle && compare(right, middle) > 0);
                    if (left > right) break;
                    swap(left, right);
                    if (middle === right) middle = left;
                }
                right += 1;
                let done = false;
                if (middle < right) {
                    // pivot 오른쪽 동률 구간의 끝을 제외한다.
                    do { right -= 1; } while (right > middle && compare(right, middle) === 0);
                    if (middle < right) done = true;
                }
                // pivot 왼쪽도 같은 키를 제외하되 low 는 다시 비교하지 않는다.
                if (!done) do { right -= 1; } while (right > low && compare(right, middle) === 0);
                // 현재 큰 구간만 저장한다. 원본의 signed 길이 비교를 요소 인덱스로 표현한다.
                const push = (first, last) => {
                    if (pending.length === STACK_SIZE) throw new RangeError('운반 후보 정렬의 구간 스택 초과');
                    pending.push([first, last]);
                };
                if (right - low >= high - left) {
                    if (low < right) push(low, right);
                    if (left < high) { low = left; continue; }
                } else {
                    if (left < high) push(left, high);
                    if (low < right) { high = right; continue; }
                }
            }
            if (!pending.length) break;
            [low, high] = pending.pop();
        }
    }
}
