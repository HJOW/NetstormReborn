// 투사체 기하 표 구성 raw-projectile-geometry-types.js(buildProjectileGeometryTypes)를 합성 정의와 웹 준비 자료의 실제 TYPE·SHP 헤더로 검사한다.
//  - 합성 계약: 정의 블록 b → TYPE 70+b 칸, 기본 프레임·클러스터 수·orientations 방향 수·물리 프레임 포구·height 절삭·range float, 속성 이름 대소문자 무시·마지막 값 우선·문자열은 0,
//    x87 정수 불확정값, 정의 개수/SHP 표 누락 거부, 정의가 없는 TYPE 은 독립된 빈 칸.
//  - 실제 자료(netstorm.tarc·d/_shapes.shp): 모든 자산 TYPE(70..185)의 포구 쌍이 SHP 추가 헤더 +0x20/+0x22 의 signed WORD 를 직접 다시 읽은 값과 같고, 기본 프레임·클러스터 수·방향 수·사거리가
//    독립 경로(buildTypeRecords)와 같으며, 이 표를 쓴 RawProjectileGeometry 가 실제 썬 디스크 발사기(71)의 포구를 SHP 헤더 두 값의 합으로 계산한다.
// cpppj 의 같은 구성(client 세션·CampaignInstallSupport)에는 독립 관측 fixture 가 없다. 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E362: 7-P2 RawProjectileGeometry 의 실제 SHP 헤더 연결과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import { floatToBits, bitsToFloat, put } from './raw-support.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { buildTypeRecords } from '../www/assets/type-flags.js';
import { SidPool } from '../www/core/sid-pool.js';
import { RawProjectileGeometry } from '../www/core/raw-projectile-geometry.js';
import { buildProjectileGeometryTypes } from '../www/core/raw-projectile-geometry-types.js';

// 합성 정의 하나이다: 속성 목록과 클러스터 수, 기본 프레임. (2026-11-13 WB04-E362)
const definition = (properties, clusterCount = 1, defaultFrame = 0) => ({
    name: 'x', properties: properties.map(([name, value]) => ({ name, value })), clusters: Array.from({ length: clusterCount }, () => ({})), defaultFrame,
});

// 합성 SHP 표이다: 길이 188 의 칸에서 지정한 TYPE 만 프레임 포구를 갖는다. (2026-11-13 WB04-E362)
function syntheticShapes(entries) {
    const shapes = Array.from({ length: 188 }, () => ({ frameCount: 0, loaded: false, frames: [] }));
    for (const [type, offsets] of entries) shapes[type] = { frameCount: offsets.length, loaded: true, frames: offsets.map(([muzzleX, muzzleY]) => ({ muzzleX, muzzleY })) };
    return shapes;
}

test('투사체 기하 표: 정의 블록 b 는 TYPE 70+b 칸에 기본 프레임·클러스터 수·방향 수·물리 프레임 포구·height·range 를 채우고 나머지 칸은 독립된 빈 칸이다', () => {
    const geometry = buildProjectileGeometryTypes(
        [definition([['range', 20], ['height', 2], ['orientations', 20]], 3, 5), definition([['range', 9.5]], 2, 1)],
        syntheticShapes([[70, [[-7, 11], [32767, -32768], [0, 0]]], [71, [[4, -5], [6, 7]]]]));
    assert.equal(geometry.length, 188);
    assert.deepEqual(geometry[70], { defaultFrame: 5, frameCount: 3, directions: 20, shotOffsets: [[-7, 11], [32767, -32768], [0, 0]], heightExtension: 2, range: 20 });
    assert.deepEqual(geometry[71], { defaultFrame: 1, frameCount: 2, directions: 0, shotOffsets: [[4, -5], [6, 7]], heightExtension: 0, range: 9.5 });
    // 정의가 없는 TYPE(0~69 와 뒤쪽)은 모두 0 인 빈 칸이고, 서로 다른 객체이다.
    const empty = { defaultFrame: 0, frameCount: 0, directions: 0, shotOffsets: [], heightExtension: 0, range: 0 };
    for (const type of [0, 69, 72, 187]) assert.deepEqual(geometry[type], empty, `TYPE ${type}`);
    assert.notEqual(geometry[0], geometry[1]);
    assert.notEqual(geometry[0].shotOffsets, geometry[1].shotOffsets);
    // 길이는 지정한 TYPE 개수를 따른다.
    assert.equal(buildProjectileGeometryTypes([definition([])], syntheticShapes([[70, []]]), 188).length, 188);
});

test('투사체 기하 표: 속성은 이름 대소문자를 무시하고 마지막 값이 이기며 문자열은 숫자로 바꾸지 않고, height 는 0 쪽 절삭 int32·range 는 float 이다', () => {
    const cases = [
        // [속성 목록, 기대 height, 기대 range]
        [[['RANGE', 20.1], ['Height', 2.9]], 2, Math.fround(20.1)],
        [[['height', -2.9]], -2, 0],
        [[['range', 5], ['range', 7], ['height', 3], ['HEIGHT', 4]], 4, 7],
        [[['range', 'far'], ['height', 'tall']], 0, 0],
        [[['range', 5], ['range', 'later string wins']], 0, 0],
        [[['height', 2147483647.5]], 2147483647, 0],
        [[['height', -2147483648.5]], -2147483648, 0],
        [[['height', 2147483648]], -2147483648, 0],
        [[['height', -2147483649]], -2147483648, 0],
        [[['height', Infinity], ['range', Infinity]], -2147483648, Infinity],
        [[['height', 0.9], ['range', 1e-50]], 0, 0],
        // 비유한 height 는 정수 변환의 불확정값이다(NaN 은 비교가 모두 거짓이라 별도로 거른다).
        [[['height', NaN]], -2147483648, 0],
        [[['height', -Infinity]], -2147483648, 0],
    ];
    for (const [properties, height, range] of cases) {
        const geometry = buildProjectileGeometryTypes([definition(properties)], syntheticShapes([[70, []]]));
        assert.equal(geometry[70].heightExtension, height, JSON.stringify(properties));
        assert.equal(geometry[70].range, range, JSON.stringify(properties));
    }
});

test('투사체 기하 표: 방향 수는 orientations 숫자만(float 폭·0 쪽 절삭) 읽고 프레임 수는 클러스터 수이며 SHP 프레임 수와 독립이고 잘못된 입력을 거부한다', () => {
    const geometry = buildProjectileGeometryTypes([definition([['orientations', 20.9]], 4), definition([], 0)], syntheticShapes([[70, [[1, 2]]], [71, [[3, 4], [5, 6], [7, 8]]]]));
    assert.deepEqual([geometry[70].directions, geometry[70].frameCount, geometry[70].shotOffsets.length], [20, 4, 1]);
    assert.deepEqual([geometry[71].directions, geometry[71].frameCount, geometry[71].shotOffsets.length], [0, 0, 3]);
    // orientations 가 숫자가 아니면 아처 자료 읽기가 거부한다.
    assert.throws(() => buildProjectileGeometryTypes([definition([['orientations', 'many']])], syntheticShapes([[70, []]])), TypeError);
    // 정의가 TYPE 표 끝(188)을 넘기거나 대응하는 SHP 칸이 없으면 거부한다.
    assert.throws(() => buildProjectileGeometryTypes(Array.from({ length: 119 }, () => definition([])), syntheticShapes([])), (error) => error instanceof RangeError && /TYPE 표/.test(error.message));
    // 정의가 TYPE 표를 꽉 채우는 118개(70..187)는 허용되고 마지막 TYPE 187 의 칸은 그 정의(클러스터 1개)로 채워진다.
    assert.equal(buildProjectileGeometryTypes(Array.from({ length: 118 }, () => definition([])), syntheticShapes([]))[187].frameCount, 1);
    assert.throws(() => buildProjectileGeometryTypes([definition([])], []), (error) => error instanceof TypeError && /TYPE 70/.test(error.message));
    // SHP 칸은 있어도 프레임 배열이 없으면 거부한다.
    const noFrames = [];
    noFrames[70] = {};
    assert.throws(() => buildProjectileGeometryTypes([definition([])], noFrames), (error) => error instanceof TypeError && /TYPE 70/.test(error.message));
    noFrames[70] = { frames: 'x' };
    assert.throws(() => buildProjectileGeometryTypes([definition([])], noFrames), TypeError);
    assert.equal(buildProjectileGeometryTypes([definition([])], syntheticShapes([[70, []]]), 71)[70].shotOffsets.length, 0);
    assert.throws(() => buildProjectileGeometryTypes([definition([]), definition([])], syntheticShapes([[70, []], [71, []]]), 71), RangeError);
});

// SHP 추가 헤더의 비정렬 little endian signed WORD 를 직접 읽는다(빌더와 다른 경로). (2026-11-13 WB04-E362)
function signedWord(header, offset) {
    const value = header[offset] | (header[offset + 1] << 8);
    return value >= 0x8000 ? value - 0x10000 : value;
}

test('투사체 기하 표(실제 자료): 모든 자산 TYPE 의 포구 쌍이 SHP 추가 헤더 +0x20/+0x22 를 직접 다시 읽은 값과 같고 나머지 자료가 독립 TYPE 레코드와 같다', () => {
    const assets = realCaptureAssets();
    const definitions = assets.table.definitions;
    const geometry = buildProjectileGeometryTypes(definitions, assets.shapes);
    const records = buildTypeRecords(definitions);
    let offsets = 0;
    // 자산 TYPE 마다 SHP 헤더의 마지막 두 WORD 와 TYPE 자료를 대조한다.
    for (let block = 0; block < definitions.length; block += 1) {
        const type = 70 + block;
        const entry = geometry[type];
        assert.equal(entry.shotOffsets.length, assets.shapes[type].frames.length, `TYPE ${type} 프레임 수`);
        // 모든 물리 프레임의 포구 쌍은 원본 SHP 헤더 36바이트의 마지막 두 signed WORD 이다.
        for (let frame = 0; frame < entry.shotOffsets.length; frame += 1) {
            const header = assets.database.squidHeader(block, frame);
            assert.deepEqual(entry.shotOffsets[frame], [signedWord(header, 32), signedWord(header, 34)], `TYPE ${type} 프레임 ${frame}`);
            offsets += 1;
        }
        assert.deepEqual([entry.defaultFrame, entry.frameCount, entry.directions, entry.range], [records[type].defaultFrame, records[type].frameCount, records[type].directions, Math.fround(records[type].range)], `TYPE ${type} 자료`);
    }
    assert.ok(offsets > 1000);
    // 알려진 자산: 썬 캐논(128) 사거리 20·포구 (−23, −11), 썬 디스크 발사기(71) 사거리 9.
    assert.deepEqual([geometry[128].range, geometry[128].shotOffsets[0], geometry[71].range, geometry[140].directions, geometry[131].heightExtension], [20, [-23, -11], 9, 20, 2]);
});

test('투사체 기하 표(실제 자료): 이 표를 쓴 RawProjectileGeometry 는 실제 썬 디스크 발사기(71)의 포구를 기본 프레임과 현재 프레임의 SHP 헤더 두 쌍의 합으로 계산한다', () => {
    const assets = realCaptureAssets();
    const geometry = buildProjectileGeometryTypes(assets.table.definitions, assets.shapes);
    const pool = new SidPool(24000, { server: false });
    const sid = pool.allocate(2);
    const types = assets.types;
    const logic = new RawProjectileGeometry(pool, types, geometry);
    const first = geometry[71].defaultFrame;
    // 몇 개의 현재 프레임에서 독립 계산(헤더 직접 읽기)과 float 비트까지 비교한다.
    for (const frame of [0, 1, 7, 20, 34]) {
        const raw = pool.allocated(sid);
        raw.fill(0);
        raw[10] = 71;
        put(raw, 14, floatToBits(12.25));
        put(raw, 18, floatToBits(33.5));
        put(raw, 36, frame);
        const a = assets.database.squidHeader(1, first);
        const b = assets.database.squidHeader(1, frame);
        const expected = [
            Math.fround((signedWord(a, 32) + signedWord(b, 32)) / 16 + 12.25),
            Math.fround((signedWord(a, 34) + signedWord(b, 34)) / 11 + 33.5),
        ];
        const actual = logic.archerMuzzle(sid);
        assert.deepEqual(actual.map(floatToBits), expected.map(floatToBits), `프레임 ${frame}`);
    }
    // 128/129/131/140 의 가상 포구는 현재 raw 좌표를 그대로 돌려준다(SHP 값과 무관).
    for (const type of [128, 129, 131, 140]) {
        const raw = pool.allocated(sid);
        raw.fill(0);
        raw[10] = type;
        put(raw, 14, floatToBits(7.5));
        put(raw, 18, floatToBits(8.25));
        assert.deepEqual(logic.muzzle(sid).map(floatToBits), [floatToBits(7.5), floatToBits(8.25)]);
    }
    assert.equal(bitsToFloat(floatToBits(1.5)), 1.5);
});
