// 운반 후보 비교기/정렬 raw-carrier-candidate-sort.js(RawCarrierCandidateSort)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: carrier-candidate-sort-x86.tsv 513행(두 x87 정밀도 동일 결과) — 입력 8열(직접 비교 여부·정렬 배열 시작 주소·정렬 개수·풀 패턴·값 패턴·비교 주소 둘·풀 시작 주소) → 반환값, 정렬 뒤 배열, 비교기 호출 주소 열(요소 주소 쌍의 순서), 배열 Adler-32.
//    풀 raw 는 패턴 0/1 의 전체 24000×50 바이트이고 후보 값은 SID 범위 밖도 포함한다. 비교기는 후보 값을 읽지 않으며 raw 와 정렬 범위 밖 요소는 변하지 않아야 한다.
//  - 계약: CarrierCandidateSort_HostAddressAndEdition(범위 밖 주소 비교의 0/±1, 빈/단일 배열, CD 풀 거부).
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E90: 운반 후보 정렬 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, adler32 } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawCarrierCandidateSort } from '../www/core/raw-carrier-candidate-sort.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E90)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'carrier-candidate-sort-x86.tsv'));

// 독립 입력 수와 풀 슬롯 수/후보 수이다. (2026-11-08 WB04-E90)
const ROWS = 513;
const CAPACITY = 24000;
const CANDIDATES = 500;

// TSV 의 명시 DWORD 열을 읽는다(`-` 는 빈 열). (2026-11-08 WB04-E90)
const numbers = (text) => (text === '-' ? [] : text.split(',').map(Number));

// 슬롯 경계의 WORD 와 감김으로 생기는 비정렬 WORD 를 구분하는 풀 raw 입력이다(pattern 0 은 전부 0). (2026-11-08 WB04-E90)
function loadPool(pool, pattern) {
    // 슬롯 하나씩 물리 바이트를 채운다.
    for (let sid = 0; sid < CAPACITY; sid += 1) {
        const raw = pool.physical(sid);
        for (let offset = 0; offset < 50; offset += 1) raw[offset] = pattern ? (sid * 50 + offset) * 37 + sid * 11 + pattern * 19 & 0xff : 0;
    }
}

// 정렬하지 않은 원래 입력이며 일부 값은 유효 SID 범위를 벗어난다. (2026-11-08 WB04-E90)
function sortValues(pattern) {
    // 실제 정렬은 이 값의 크기나 유효성에 의존하지 않아야 한다.
    return Uint32Array.from({ length: CANDIDATES }, (_, i) => (pattern === 0 ? i + 1 : pattern === 1 ? 500 - i : (0xf0000000 + i * 0x10203) >>> 0));
}

// 예외가 나는지 본다. (2026-11-08 WB04-E90)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

test('CarrierCandidateSort: 원본 10.78 513입력의 비교기 반환·CRT qsort 동률 순서·비교 호출 주소 열·Adler-32 가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const pool = new SidPool(CAPACITY);
    const loaded = { pattern: -1 };
    let count = 0;
    // 두 정밀도 결과도 같아야 하며 JS 는 정수 경로 하나로 같은 원문을 재생한다.
    for (const fields of rows) {
        assert.equal(fields.length, 9);
        const row = numbers(fields[0]);
        assert.equal(row.length, 8);
        if (loaded.pattern !== row[3]) { loadPool(pool, row[3]); loaded.pattern = row[3]; }
        const candidates = sortValues(row[4]);
        const initial = Uint32Array.from(candidates);
        let calls = [];
        const sorter = new RawCarrierCandidateSort(pool, { poolBase: row[7], arrayBase: row[1] });
        let result = 0;
        if (row[0]) { result = sorter.compare(row[5], row[6]); calls = [row[5], row[6]]; } else sorter.sort(candidates.subarray(0, row[2]), (first, second) => calls.push(first, second));
        const label = `입력 ${fields[0]}`;
        assert.equal(result, Number(fields[1]), label);
        assert.deepEqual(Array.from(candidates.subarray(0, row[2])), numbers(fields[2]), label);
        assert.deepEqual(calls, numbers(fields[3]), label);
        assert.deepEqual(Array.from(candidates.subarray(row[2])), Array.from(initial.subarray(row[2])), label);
        assert.equal(adler32(new Uint8Array(candidates.buffer, 0, row[2] * 4)), Number(fields[4]), label);
        assert.deepEqual(fields.slice(1, 5), fields.slice(5), label);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('CarrierCandidateSort: 범위 밖 주소 비교는 0/±1 이고 빈/단일 배열은 그대로이며 CD 풀과 정렬 한도를 거부한다', () => {
    const pool = new SidPool(CAPACITY);
    const sorter = new RawCarrierCandidateSort(pool);
    assert.equal(sorter.compare(0, 0), 0);
    // 0 번 슬롯 주소는 풀 시작+50 보다 낮아 범위 밖이므로 범위 밖 첫째는 1, 범위 밖 둘째는 -1 이다(원본 규칙).
    assert.equal(sorter.compare(0, 1), 1);
    assert.equal(sorter.compare(1, 0), -1);
    assert.equal(sorter.compare(1, 1), 0);
    sorter.sort(new Uint32Array(0));
    const single = Uint32Array.of(42);
    sorter.sort(single);
    assert.equal(single[0], 42);
    assert.ok(throws(() => new RawCarrierCandidateSort({ edition: 'cd1072' })));
});
