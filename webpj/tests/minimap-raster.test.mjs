// 미니맵 셀 그림(004808d0)의 실제 raw 월드 입력과 전체 128×128 bitmap 을 원본 x86 fixture 로 대조한다(cpppj MinimapRasterTests, 10.78).
//  - minimap-cell-x86.tsv 4722행(두 x87 정밀도 관측): 셀·모드·색 억제·프레임·색 DWORD 12개·슬롯 입력(sid:type:owner:flags2:xBits:yBits:next:state:extra)·해시 머리(level:x:y:head)
//    입력과, 원문이 바꾼 위치의 관측 픽셀(offset:BYTE). 미수정 주변(0xA5)까지 포함한 전체 bitmap 을 비교한다.
//  - 각 행은 입력만 같은 SidPool/SquidHash/TYPE flags2 에 설치하고 모듈이 그린다. 기대 픽셀은 비교에만 쓴다. 거부 경로는 웹 계약 검사다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E204: 미니맵 셀 그림 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { RawMinimapRaster, createMinimapRasterState } from '../www/core/raw-minimap-raster.js';

// 원본 x86 fixture 이다(머리 주석 행 제외). (2026-11-09 WB04-E204)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'minimap-cell-x86.tsv')) ?? [];

// 원문 실행기에 공급한 미수정 bitmap·슬롯 표식과 bitmap 크기이다. (2026-11-09 WB04-E204)
const INITIAL = 0xa5;
const BITMAP_SIZE = 128 * 128;

// 예외가 지정 종류로 나는지 본다. (2026-11-09 WB04-E204)
function throws(action, kind = Error) {
    try { action(); } catch (error) { return error instanceof kind; }
    return false;
}

// 현재 실제 Pool/Hash/TYPE 를 갖되 기대 픽셀은 계산하지 않는 검사 장면이다(cpppj RasterScene). (2026-11-09 WB04-E204)
function scene(row) {
    const pool = new SidPool(24000, { server: false });
    const hash = new SquidHash();
    const types = Array.from({ length: 188 }, () => ({ flags2: 0 }));
    const colors = row[6].split(':').map(Number);
    assert.equal(colors.length, 12);
    const state = createMinimapRasterState({
        challenge: Number(row[3]), suppressColors: Number(row[4]), frame: Number(row[5]),
        black: colors[0], neutralIsle: colors[9], neutralNoIsland: colors[10], challengeGreen: colors[11], owners: colors.slice(1, 9),
    });
    // sid:type:owner:flags2:xBits:yBits:next:state:extra 입력이다.
    if (row[7] !== '-') for (const item of row[7].split(';')) {
        const fields = item.split(':').map(Number);
        assert.equal(fields.length, 9);
        const [sid, kind] = fields;
        assert.ok(sid < pool.capacity && kind < types.length);
        // 물리 슬롯 입력이다(원문이 SID 1 같은 예약 슬롯도 읽는다).
        const raw = pool.physical(sid);
        raw.fill(INITIAL);
        put(raw, 4, fields[6], 2);
        raw[10] = kind;
        raw[11] = fields[7];
        put(raw, 14, fields[4]);
        put(raw, 18, fields[5]);
        raw[34] = fields[2];
        raw[40] = fields[8];
        types[kind].flags2 = fields[3];
    }
    // level:x:y:head 의 명시 배열 입력이다.
    if (row[8] !== '-') for (const item of row[8].split(';')) {
        const fields = item.split(':').map(Number);
        assert.equal(fields.length, 4);
        hash.setCell(fields[0], fields[1], fields[2], fields[3]);
    }
    return { pool, hash, types, state, bitmap: new Uint8Array(BITMAP_SIZE).fill(INITIAL) };
}

// 원문이 바꾼 위치의 관측만 펼쳐 미수정 주변까지 포함한 기대 bitmap 을 만든다. (2026-11-09 WB04-E204)
function observed(pixels) {
    const expected = new Uint8Array(BITMAP_SIZE).fill(INITIAL);
    // offset:BYTE 는 원문 관측이며 타입/색 선택을 재계산하지 않는다.
    if (pixels !== '-') for (const item of pixels.split(';')) {
        const [offset, value] = item.split(':').map(Number);
        assert.ok(offset < BITMAP_SIZE && value <= 255);
        expected[offset] = value;
    }
    return expected;
}

test('MinimapRaster: 원본 4722행의 지면/해시 순서와 전체 128×128 bitmap 이 같다', { skip: rows.length === 0 && 'minimap-cell-x86.tsv 없음' }, () => {
    let count = 0;
    // 두 정밀도별 원문 관측을 빠뜨리지 않고 같은 본체에 재생한다.
    for (const row of rows) {
        assert.equal(row.length, 16);
        assert.equal(row[9], '1');
        assert.equal(row[13], '0');
        assert.ok(Number(row[14]) > 0);
        assert.equal(row[15].length, 64);
        const world = scene(row);
        new RawMinimapRaster(world.pool, world.hash, world.types, world.state).drawCell(world.bitmap, Number(row[1]), Number(row[2]));
        assert.deepEqual(world.bitmap, observed(row[10]), `행 ${count} ${row[0]}`);
        count += 1;
    }
    assert.equal(count, 4722);
});

test('MinimapRaster: 판본·bitmap/셀 범위·픽셀 범위·next 순환을 거부하고 현재 전역을 매번 읽는다(웹 계약 검사)', () => {
    const pool = new SidPool(24000, { server: false });
    const hash = new SquidHash();
    const types = Array.from({ length: 188 }, () => ({ flags2: 0 }));
    const state = createMinimapRasterState({ black: 3, owners: [11, 12, 13, 14, 15, 16, 17, 18] });
    assert.ok(throws(() => new RawMinimapRaster({ edition: 'cd1072' }, hash, types, state), TypeError));
    assert.ok(throws(() => new RawMinimapRaster(pool, hash, [], state), TypeError));
    const raster = new RawMinimapRaster(pool, hash, types, state);
    assert.equal(raster.pool, pool);
    const bitmap = new Uint8Array(BITMAP_SIZE);
    assert.ok(throws(() => raster.drawCell(new Uint8Array(BITMAP_SIZE - 1), 0, 0), RangeError));
    assert.ok(throws(() => raster.drawCell(bitmap, 64, 0), RangeError));
    assert.ok(throws(() => raster.drawCell(bitmap, 0, -1), RangeError));
    // 일반 점: flags2&4 인 객체를 소유자 색으로 그리고, 색 억제 전역은 다음 호출에서 현재 프레임 하위 BYTE 로 바뀐다.
    const raw = pool.physical(300);
    raw.fill(0);
    raw[10] = 120;
    raw[34] = 2;
    put(raw, 14, floatToBits(5.5));
    put(raw, 18, floatToBits(6.25));
    types[120].flags2 = 4;
    hash.setCell(0, 5, 6, 300);
    raster.drawCell(bitmap, 1, 1);
    assert.deepEqual([bitmap[2 * 128 + 2], bitmap[3 * 128 + 2], bitmap[3 * 128 + 3]], [3, 12, 3]);
    state.suppressColors = 1;
    state.frame = 0x1234;
    raster.drawCell(bitmap, 1, 1);
    assert.equal(bitmap[3 * 128 + 2], 0x34);
    // 좌표가 월드 밖으로 절삭되면 픽셀 범위 오류이고, 비유한 좌표는 0 으로 절삭된다.
    put(raw, 14, floatToBits(400));
    assert.ok(throws(() => raster.drawCell(bitmap, 1, 1), RangeError));
    put(raw, 14, floatToBits(Number.NaN));
    raster.drawCell(bitmap, 1, 1);
    assert.equal(bitmap[3 * 128 + 0], 0x34);
    // 손상된 next 순환과 TYPE 범위 밖은 거부한다.
    put(raw, 14, floatToBits(5.5));
    put(raw, 4, 300, 2);
    assert.ok(throws(() => raster.drawCell(bitmap, 1, 1)));
    put(raw, 4, 0, 2);
    const short = new RawMinimapRaster(pool, hash, types.slice(0, 100), state);
    assert.ok(throws(() => short.drawCell(bitmap, 1, 1), RangeError));
});
