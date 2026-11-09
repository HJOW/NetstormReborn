// ColorSort primary(439de0)의 raw 생성자 실제 바이트·저장 순서·부분 효과와 두 원문 정밀도 차이를 원본 x86 fixture 로 대조한다(cpppj ColorSortPrimaryTests).
//  - colorsort-primary-x86.tsv 80행: 제어어 027f(53비트) 40행·037f(64비트) 40행. 각 행은 논리 RGBX·초기 할당·기대 raw·실행 뒤 논리 입력·완료 여부·store 관측열을 담는다.
//  - 027f 40행만 모듈과 대조한다(alias 행은 같은 raw 저장소를 입력으로 빌리고, 중단 행은 지정 store 수에서 관측 예외로 멈춘다). 037f 행은 원문 차이 보존만 감사한다.
//  - 초기 할당 계약·재생성·getter 범위는 cpppj 손검사를 옮겼다. 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E205: ColorSort primary 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { RawColorSortPrimary, COLOR_SORT_PRIMARY_BYTES } from '../www/core/raw-color-sort-primary.js';

// 원본 x86 fixture 이다(주석 행 제외, 14칸). (2026-11-09 WB04-E205)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'colorsort-primary-x86.tsv')) ?? [];

// 예외가 지정 종류로 나는지 본다. (2026-11-09 WB04-E205)
function throws(action, kind = Error) {
    try { action(); } catch (error) { return error instanceof kind; }
    return false;
}

// 16진 문자열의 바이트 배열이다. (2026-11-09 WB04-E205)
const bytesOf = (text) => Uint8Array.from(Buffer.from(text, 'hex'));

// 바이트 배열의 소문자 16진 문자열이다. (2026-11-09 WB04-E205)
const hexOf = (bytes) => Buffer.from(bytes.buffer, bytes.byteOffset, bytes.byteLength).toString('hex');

// 지정 store 수 뒤 멈추는 외부 관측 중단 표식이다. (2026-11-09 WB04-E205)
class ObservationStop extends Error {}

test('ColorSortPrimary: 027f 40행의 raw 전체·실제 alias·모든 store 관측·명시 중단 위치가 원본과 같다', { skip: rows.length === 0 && 'colorsort-primary-x86.tsv 없음' }, () => {
    let count = 0;
    let normal = 0;
    let stops = 0;
    let lightness = 0;
    // 최초 80행 중 027f 40행만 53비트 모듈과 비교한다.
    for (const row of rows) {
        assert.equal(row.length, 14);
        if (row[1] !== '027f') continue;
        const logical = bytesOf(row[4]);
        const expected = bytesOf(row[6]);
        const logicalAfter = bytesOf(row[7]);
        const owner = new RawColorSortPrimary(bytesOf(row[5]));
        const same = owner.bytes;
        const alias = Number(row[2]);
        const stop = Number(row[3]);
        // 외부 입력이 기본이며 alias 행은 같은 raw 를 빌린다.
        const input = alias >= 0 ? owner.bytes.subarray(alias, alias + 1024) : logical;
        const trace = [];
        let writes = 0;
        let stopped = false;
        const hooks = {
            checkpoint(completed, offset, size) {
                if (completed) trace.push(`${offset}:${size}:${hexOf(owner.bytes.subarray(offset, offset + size))}`);
                writes = completed;
                if (stop >= 0 && completed === stop) throw new ObservationStop();
            },
        };
        try { owner.construct(input, hooks); } catch (error) {
            if (!(error instanceof ObservationStop)) throw error;
            stopped = true;
        }
        assert.equal(owner.bytes, same);
        assert.equal(hexOf(owner.bytes), hexOf(expected), `raw ${row[0]}`);
        assert.equal(hexOf(input), hexOf(logicalAfter), `입력 ${row[0]}`);
        assert.equal(trace.join(';'), row[12], `관측 ${row[0]}`);
        assert.equal(stopped, row[8] === '0');
        assert.equal(owner.constructed, row[8] === '1');
        if (row[8] === '1') {
            normal += 1;
            assert.equal(writes, 1792);
            const view = new DataView(expected.buffer);
            // getter 는 원문 +8 의 비트를 그대로 돌려줘야 한다.
            for (let index = 0; index < 256; index += 1) {
                const value = owner.lightness(index);
                const bits = new DataView(new ArrayBuffer(8));
                bits.setFloat64(0, value, true);
                assert.equal(bits.getBigUint64(0, true), view.getBigUint64(index * 56 + 8, true));
                lightness += 1;
            }
        } else {
            stops += 1;
            assert.equal(writes, stop);
        }
        count += 1;
    }
    assert.deepEqual([rows.length, count, normal, stops, lightness], [80, 40, 18, 22, 4608]);
});

test('ColorSortPrimary: 037f 원문 출력은 53비트로 바꾸지 않고 두 정밀도의 필드 차이를 보존한다', { skip: rows.length === 0 && 'colorsort-primary-x86.tsv 없음' }, () => {
    const pairs = new Map();
    const differences = new Array(8).fill(0);
    let extended = 0;
    let normal = 0;
    let stopped = 0;
    // 전체 원문 80행은 보존/형식 감사만 한다.
    for (const row of rows) {
        assert.ok(row[1] === '027f' || row[1] === '037f');
        if (row[1] === '037f') extended += 1;
        if (row[8] === '1') {
            normal += 1;
            if (!pairs.has(row[0])) pairs.set(row[0], [null, null]);
            pairs.get(row[0])[row[1] === '027f' ? 0 : 1] = bytesOf(row[6]);
        } else stopped += 1;
    }
    const offsets = [0, 8, 16, 24, 32, 40, 48, 52];
    const sizes = [8, 8, 8, 8, 8, 8, 4, 4];
    // 같은 실제 입력의 두 원문 결과만 비교한다.
    for (const [first, second] of pairs.values()) {
        assert.equal(first.length, COLOR_SORT_PRIMARY_BYTES);
        assert.equal(second.length, COLOR_SORT_PRIMARY_BYTES);
        // 전체 항목의 모든 기록 필드를 본다.
        for (let index = 0; index < 256; index += 1) {
            // 원문 padding 도 따로 센다.
            for (let field = 0; field < offsets.length; field += 1) {
                const offset = index * 56 + offsets[field];
                if (hexOf(first.subarray(offset, offset + sizes[field])) !== hexOf(second.subarray(offset, offset + sizes[field]))) differences[field] += 1;
            }
        }
    }
    assert.deepEqual([rows.length, extended, normal, stopped, pairs.size], [80, 40, 36, 44, 18]);
    assert.deepEqual(differences, [409, 0, 451, 0, 0, 0, 0, 0]);
});

test('ColorSortPrimary: 초기 할당 크기를 요구하고 같은 저장소 재생성·getter 범위·미작성 padding 을 지킨다', () => {
    const initial = new Uint8Array(COLOR_SORT_PRIMARY_BYTES).fill(0xa5);
    const logical = new Uint8Array(1024);
    const owner = new RawColorSortPrimary(initial);
    assert.ok(throws(() => owner.lightness(0)));
    assert.equal(owner.constructed, false);
    owner.construct(logical);
    const same = owner.bytes;
    owner.construct(logical);
    assert.equal(owner.bytes, same);
    assert.equal(owner.constructed, true);
    assert.equal(owner.lightness(0), 0);
    assert.ok(throws(() => new RawColorSortPrimary(initial.subarray(0, 8)), TypeError));
    assert.ok(throws(() => owner.lightness(256), RangeError));
    assert.ok(throws(() => owner.construct(new Uint8Array(1023)), TypeError));
    // 재생성도 미작성 padding(+52..+55)을 유지한다.
    for (let index = 0; index < 256; index += 1) assert.ok(owner.bytes.subarray(index * 56 + 52, index * 56 + 56).every((value) => value === 0xa5));
});
