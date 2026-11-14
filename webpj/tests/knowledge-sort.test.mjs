// 지식 행 비교·CRT 정렬(knowledge-sort)을 기준선 소스 계약과 손으로 모의 실행한 동률 순서·성질 검사로 검증한다.
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 동률 항목의 상대 순서는 원본 CRT 알고리즘의 결과(작은 구간은 선택 정렬로 뒤집힘, 9개 동률은 분할 뒤 그대로)를 손으로 따라가 확인했다. (2026-11-14 WB04-E448)
import test from 'node:test';
import assert from 'node:assert/strict';
import { compareKnowledgeRecords, sortKnowledgeRecords } from '../www/ui/knowledge-sort.js';

// 우선 순서의 flags2 비트(원문 453f70 의 순서)다.
const BITS = [0x800, 0x8000000, 0x10000, 0x4000000, 0x20000000, 0x40000, 0x20000];
// 행 만들기: type 번호로 flags2 표를 가리키고 id 는 항목 식별용이다.
const row = (type, description, id = 0) => ({ type, description, id, available: true, inProduction: false, mana: '' });
// 여러 TYPE 의 flags2 표다.
const flagsOf = (...values) => values.map((flags2) => ({ flags2 }));

test('compare 우선 분류: 우선 비트가 한쪽에만 있으면 즉시 결정하고(앞쪽만 -1, 뒤쪽만 1) 앞선 비트가 뒤 비트보다 우선하며 둘 다 있으면 설명으로 간다', () => {
    for (const bit of BITS) {
        const types = flagsOf(bit, 0); assert.equal(compareKnowledgeRecords(row(0, 'zzz'), row(1, 'aaa'), types), -1, bit.toString(16)); assert.equal(compareKnowledgeRecords(row(1, 'aaa'), row(0, 'zzz'), types), 1, bit.toString(16));
    }
    // 둘 다 같은 우선 비트(0x800)를 가지면 뒤 비트(0x8000000)를 보지 않고 설명 비교로 간다.
    const both = flagsOf(0x800 | 0x8000000, 0x800); assert.ok(compareKnowledgeRecords(row(0, 'a'), row(1, 'b'), both) < 0); assert.ok(compareKnowledgeRecords(row(0, 'b'), row(1, 'a'), both) > 0);
    // a 는 뒤(0x8000000)에, b 는 앞(0x800)에 있으면 앞선 비트의 b 가 우선(1)이다.
    assert.equal(compareKnowledgeRecords(row(0, 'a'), row(1, 'a'), flagsOf(0x8000000, 0x800)), 1); assert.equal(compareKnowledgeRecords(row(0, 'a'), row(1, 'a'), flagsOf(0x800, 0x8000000)), -1);
    // 우선 비트가 아닌 flags2 는 분류에 영향이 없다.
    assert.equal(compareKnowledgeRecords(row(0, 'a'), row(1, 'a'), flagsOf(0x1, 0x2)), 0);
});

test('compare 설명: 부호 없는 ASCII 차이를 돌려주고 대문자 A..Z 만 소문자로 접으며 NUL(문자열 끝/내장 NUL) 이전이 같으면 0 이다', () => {
    const types = flagsOf(0); const c = (a, b) => compareKnowledgeRecords(row(0, a), row(0, b), types);
    assert.equal(c('abc', 'abd'), -1); assert.equal(c('abd', 'abc'), 1); assert.equal(c('b', 'a'), 1); assert.equal(c('ABC', 'abc'), 0); assert.equal(c('Zed', 'zeD'), 0);
    assert.equal(c('@', '`'), 64 - 96); assert.equal(c('[', '{'), 91 - 123); assert.equal(c('A', 'a'), 0); assert.equal(c('Z', 'z'), 0); assert.equal(c('Y', 'z'), 121 - 122);
    assert.equal(c('ab', 'ac'), -1); assert.equal(c('xay', 'xby'), -1); assert.equal(c('abc', 'ab'), 99); assert.equal(c('ab', 'abc'), -99); assert.equal(c('', 'a'), -97); assert.equal(c('', ''), 0);
    // 높은 바이트는 부호 없이 비교한다(0xe9 > 'A').
    assert.equal(c('\xe9', 'A'), 0xe9 - 97); assert.equal(c('\xe9', '\xe9'), 0);
    // 내장 NUL 은 문자열 끝처럼 같으면 0 이고, 한쪽만 NUL 이면 차이다.
    const nul = String.fromCharCode(0); assert.equal(c('ab' + nul + 'x', 'ab' + nul + 'y'), 0); assert.equal(c('ab' + nul + 'x', 'abc'), -99);
});

test('compare TYPE 범위: 두 행의 TYPE 이 표 길이 이상이면 RangeError 다(표 길이 -1 은 허용)', () => {
    const types = flagsOf(0, 0); assert.doesNotThrow(() => compareKnowledgeRecords(row(1, 'a'), row(0, 'a'), types));
    assert.throws(() => compareKnowledgeRecords(row(2, 'a'), row(0, 'a'), types), { name: 'RangeError', message: /TYPE 범위/ }); assert.throws(() => compareKnowledgeRecords(row(0, 'a'), row(2, 'a'), types), RangeError);
});

test('정렬 작은 구간: 동률 두 개는 선택 정렬로 뒤집히고, 동률 세 개는 [x2,x3,x1] 이 되며, 서로 다른 항목은 오름차순이다', () => {
    const types = flagsOf(0); const ids = (entries) => entries.map((entry) => entry.id);
    const two = [row(0, 'a', 1), row(0, 'a', 2)]; assert.equal(sortKnowledgeRecords(two, types), undefined); assert.deepEqual(ids(two), [2, 1]);
    const three = [row(0, 'a', 1), row(0, 'a', 2), row(0, 'a', 3)]; sortKnowledgeRecords(three, types); assert.deepEqual(ids(three), [2, 3, 1]);
    const mixed = [row(0, 'c', 1), row(0, 'a', 2), row(0, 'b', 3)]; sortKnowledgeRecords(mixed, types); assert.deepEqual(ids(mixed), [2, 3, 1]);
    const empty = []; sortKnowledgeRecords(empty, types); assert.deepEqual(empty, []); const single = [row(0, 'x', 7)]; sortKnowledgeRecords(single, types); assert.deepEqual(ids(single), [7]);
    // 우선 분류가 설명보다 먼저다.
    const classes = flagsOf(0, 0x800); const priority = [row(0, 'a', 1), row(1, 'z', 2)]; sortKnowledgeRecords(priority, classes); assert.deepEqual(ids(priority), [2, 1]);
});

test('정렬 큰 구간: 9개 동률은 분할 뒤 그대로이고(손 모의 실행), 8개 동률은 작은 구간 선택 정렬로 [x2..x8,x1] 이 된다', () => {
    const types = flagsOf(0); const ids = (entries) => entries.map((entry) => entry.id);
    const nine = Array.from({ length: 9 }, (_, index) => row(0, 'same', index + 1)); sortKnowledgeRecords(nine, types); assert.deepEqual(ids(nine), [1, 2, 3, 4, 5, 6, 7, 8, 9]);
    const eight = Array.from({ length: 8 }, (_, index) => row(0, 'same', index + 1)); sortKnowledgeRecords(eight, types); assert.deepEqual(ids(eight), [2, 3, 4, 5, 6, 7, 8, 1]);
});

test('정렬 성질: 임의 길이/동률 입력의 결과는 비교 기준 오름차순이고 같은 항목의 순열이며 항목 객체를 복사하지 않는다', () => {
    const types = flagsOf(0, 0x800, 0x8000000, 0x20000); let seed = 12345; const random = () => { seed = (Math.imul(seed, 1103515245) + 12345) >>> 0; return seed / 2 ** 32; };
    for (const size of [0, 1, 2, 7, 8, 9, 10, 16, 17, 31, 64, 129, 400]) {
        const entries = Array.from({ length: size }, (_, index) => row(Math.floor(random() * 4), ['Ann', 'ann', 'Bob', 'Cat', 'cat', 'zed', 'A', ''][Math.floor(random() * 8)], index));
        const before = [...entries]; sortKnowledgeRecords(entries, types);
        assert.deepEqual(entries.map((entry) => entry.id).sort((a, b) => a - b), before.map((entry) => entry.id).sort((a, b) => a - b), `permutation ${size}`); entries.forEach((entry) => assert.ok(before.includes(entry)));
        for (let index = 1; index < entries.length; index += 1) assert.ok(compareKnowledgeRecords(entries[index - 1], entries[index], types) <= 0, `order ${size}@${index}`);
        const again = [...before]; sortKnowledgeRecords(again, types); assert.deepEqual(again.map((entry) => entry.id), entries.map((entry) => entry.id), `deterministic ${size}`); // 같은 입력은 같은 결과(동률 순서 포함)다.
    }
});

test('정렬 오류: TYPE 범위 오류는 비교 시점에 RangeError 로 전달된다', () => {
    assert.throws(() => sortKnowledgeRecords([row(5, 'a'), row(0, 'b')], flagsOf(0)), { name: 'RangeError', message: /TYPE 범위/ });
});

// 독립 기준: MS CRT qsort 를 인덱스 형태의 재귀로 다시 쓴 기준 구현이다(작은 구간 선택 정렬·중앙 피벗 3점 정렬·동률 제거·두 부분 재귀). 구간 처리 순서는 결과 배열에 영향이 없다.
function crtReference(entries, types) {
    const compare = (a, b) => compareKnowledgeRecords(entries[a], entries[b], types); const swap = (a, b) => { if (a !== b) [entries[a], entries[b]] = [entries[b], entries[a]]; };
    const recurse = (lowStart, high) => {
        let low = lowStart;
        for (;;) {
            const size = high - low + 1;
            if (size <= 8) { let top = high; while (top > low) { let maximum = low; for (let p = low + 1; p <= top; p += 1) if (compare(p, maximum) > 0) maximum = p; swap(maximum, top); top -= 1; } return; }
            let middle = low + Math.floor(size / 2);
            if (compare(low, middle) > 0) swap(low, middle); if (compare(low, high) > 0) swap(low, high); if (compare(middle, high) > 0) swap(middle, high);
            let loval = low; let hival = high;
            for (;;) {
                if (middle > loval) { do { loval += 1; } while (loval < middle && compare(loval, middle) <= 0); }
                if (middle <= loval) { do { loval += 1; } while (loval <= high && compare(loval, middle) <= 0); }
                do { hival -= 1; } while (hival > middle && compare(hival, middle) > 0);
                if (hival < loval) break;
                swap(loval, hival); if (middle === hival) middle = loval;
            }
            hival += 1;
            if (middle < hival) { do { hival -= 1; } while (hival > middle && compare(hival, middle) === 0); }
            if (middle >= hival) { do { hival -= 1; } while (hival > low && compare(hival, middle) === 0); }
            if (low < hival) recurse(low, hival);
            if (loval < high) low = loval; else return;
        }
    };
    recurse(0, entries.length - 1);
}

test('정렬 기준 대조: 동률이 많은 임의 입력에서 구현 결과(동률 순서 포함)가 독립 기준 구현과 항목마다 같다', () => {
    const types = flagsOf(0, 0x800, 0x20000); let seed = 987654321; const random = () => { seed = (Math.imul(seed, 1664525) + 1013904223) >>> 0; return seed / 2 ** 32; };
    for (let trial = 0; trial < 600; trial += 1) {
        const size = Math.floor(random() * 120); const kinds = 1 + Math.floor(random() * 6);
        const entries = Array.from({ length: size }, (_, index) => row(Math.floor(random() * 3), String.fromCharCode(65 + Math.floor(random() * kinds)), index));
        const actual = [...entries]; sortKnowledgeRecords(actual, types); const expected = [...entries]; crtReference(expected, types);
        assert.deepEqual(actual.map((entry) => entry.id), expected.map((entry) => entry.id), `trial ${trial} size ${size}`);
    }
    // 정렬된/역순/전부 같은 입력(분할이 한쪽으로 치우치는 경우)도 같다.
    for (const make of [(i) => i, (i) => -i, () => 0, (i) => i % 3]) {
        const entries = Array.from({ length: 300 }, (_, index) => row(0, String.fromCharCode(48 + ((make(index) % 40) + 40) % 40), index)); const actual = [...entries]; sortKnowledgeRecords(actual, types); const expected = [...entries]; crtReference(expected, types);
        assert.deepEqual(actual.map((entry) => entry.id), expected.map((entry) => entry.id));
    }
});
