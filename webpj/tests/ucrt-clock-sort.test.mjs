// 제품 1313 시간 표본 정렬을 Windows UCRT 합성 입력 350건과 대조한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { sortClockSamples } from '../www/core/ucrt-clock-sort.js';

// Windows UCRT qsort가 기록한 크기 1~70과 여러 동률 패턴을 재생한다.
test('시간 표본 정렬: Windows UCRT 순열 350건', () => {
    const fixture = JSON.parse(fs.readFileSync(new URL('./fixtures/ucrt-clock-sort.json', import.meta.url), 'utf8'));
    assert.equal(fixture.format, 'netstorm-ucrt-clock-sort-v1');
    assert.equal(fixture.cases.length, 350);
    // 각 행에서 같은 객체를 정렬하고 저장된 native 순서와 비교한다.
    for (const [index, entry] of fixture.cases.entries()) {
        const samples = entry.values.map((roundTrip, offset) => ({ offset, roundTrip }));
        sortClockSamples(samples);
        assert.deepEqual(samples.map((sample) => sample.offset), entry.order, `입력 ${index}`);
    }
});

// 같은 시간 두 개는 안정 정렬과 다르며 빈 입력과 70개 제한을 처리한다.
test('시간 표본 정렬: 동률·빈 입력·표본 범위', () => {
    const samples = [{ offset: 1, roundTrip: 0.5 }, { offset: 2, roundTrip: 0.5 }];
    const original = [...samples];
    assert.equal(sortClockSamples(samples), undefined);
    assert.deepEqual(samples, [original[1], original[0]]);
    assert.doesNotThrow(() => sortClockSamples([]));
    const infinite = [0, -Infinity, 0, -Infinity, 0].map((roundTrip, offset) => ({ roundTrip, offset }));
    sortClockSamples(infinite);
    assert.deepEqual(infinite.map((sample) => sample.offset), [1, 3, 2, 4, 0]);
    const tooMany = Array.from({ length: 71 }, (_, offset) => ({ offset, roundTrip: offset }));
    const before = [...tooMany];
    assert.throws(() => sortClockSamples(tooMany), RangeError);
    assert.deepEqual(tooMany, before);
    assert.throws(() => sortClockSamples(null), RangeError);
});
