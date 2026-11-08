// 투사체 포구/치수 기하 raw-projectile-geometry.js(RawProjectileGeometry)를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: projectile-geom-x86.tsv 10.78(originals) 120행 — 썬 디스크 발사기 포구(M)/치수(D)/표적 상자(B)의 입력(프레임·extra·픽셀 배율·좌표 비트·발자국·확장 높이) → 결과 float 전체 비트(허용오차 없음, x87=027f),
//    cannon-rayend-x86.tsv 4040행 — 썬 캐논(TYPE 128)의 프레임·출발/도착 좌표·사정거리 → 사정선 끝 두 DWORD. CD 10.72 행은 웹 범위 밖이다.
//  - 계약: 썬 캐논 가상 포구가 비정렬 raw x/y 를 변형 없이 반환하고, 기본 0..3·발사 24..27 프레임의 네 방향 종점이 축 정렬이다. 아이스/썬더는 같은 현재 방향 getter 를 요구한다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E85: 투사체 포구/치수 기하 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits, bitsToFloat } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawProjectileGeometry } from '../www/core/raw-projectile-geometry.js';

// 원본 관측 10.78 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E85)
const fixtures = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const rows = readRows(path.join(fixtures, 'projectile-geom-x86.tsv'))?.filter((row) => row[0] === 'originals') ?? null;
const rayRows = readRows(path.join(fixtures, 'cannon-rayend-x86.tsv'));

// 독립 입력 수이다. (2026-11-08 WB04-E85)
const ROWS = 120;
const RAY_ROWS = 4040;

// 예외가 나는지 본다. (2026-11-08 WB04-E85)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 새 TYPE 표/기하 입력(188칸)이다. (2026-11-08 WB04-E85)
const freshTypes = () => Array.from({ length: 188 }, () => ({ footX: 0, footY: 0 }));
const freshGeometry = () => Array.from({ length: 188 }, () => ({ defaultFrame: 0, frameCount: 0, directions: 0, shotOffsets: [], heightExtension: 0, range: 0 }));

test('ProjectileGeometry: 원본 10.78 120입력의 썬 디스크 포구/치수/충돌 상자가 float 전체 비트까지 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const pool = new SidPool(24000, { server: false });
    const object = pool.allocate(2);
    const types = freshTypes();
    const geometry = freshGeometry();
    geometry[71].defaultFrame = 1;
    geometry[71].frameCount = 4;
    geometry[71].shotOffsets = [[-35, 91], [20, -7], [32767, -32768], [-127, 43]];
    let count = 0;
    // 현재 행의 입력만 읽어 같은 조회기의 모든 결과 DWORD 를 원본과 비교한다.
    for (const row of rows) {
        const raw = pool.allocated(object);
        raw[10] = 71;
        put(raw, 14, Number(row[6]));
        put(raw, 18, Number(row[7]));
        put(raw, 36, Number(row[2]));
        raw[40] = Number(row[3]) & 0xff;
        types[71].footX = Number(row[8]);
        types[71].footY = Number(row[9]);
        geometry[71].heightExtension = Number(row[10]);
        const logic = new RawProjectileGeometry(pool, types, geometry, Number(row[4]), Number(row[5]));
        let actual;
        if (row[1] === 'M') actual = logic.archerMuzzle(object).map(floatToBits);
        else if (row[1] === 'D') actual = logic.dimensions(object).map((value) => value >>> 0);
        else actual = logic.targetBounds(object, [bitsToFloat(Number(row[6])), bitsToFloat(Number(row[7]))]).map(floatToBits);
        const expected = row[11].split(',').map((value) => parseInt(value, 16));
        assert.deepEqual(actual, expected, `행 ${count}: ${row.slice(1, 11).join(' ')}`);
        count += 1;
    }
    assert.equal(count, ROWS);
});

test('ProjectileGeometry: 썬 캐논 가상 포구가 비정렬 raw x/y 를 변형 없이 반환하고 기본/발사 프레임의 네 방향 종점이 축 정렬이다', () => {
    const pool = new SidPool(24000, { server: false });
    const object = pool.allocate(2);
    const logic = new RawProjectileGeometry(pool, freshTypes(), freshGeometry());
    const raw = pool.allocated(object);
    raw[10] = 128;
    put(raw, 14, 0x422a0000);
    put(raw, 18, 0x41a40000);
    const point = logic.muzzle(object);
    assert.ok(floatToBits(point[0]) === 0x422a0000 && floatToBits(point[1]) === 0x41a40000);
    // 프레임 0..3 과 24..27 은 같은 네 cardinal 방향이다.
    const cases = [[0, [11, 30], [10, 30]], [1, [11, 10], [10, 10]], [2, [1, 19], [1, 20]], [3, [30, 21], [30, 20]]];
    for (const [frame, to, expected] of cases) {
        for (const base of [0, 24]) {
            put(raw, 36, frame + base);
            assert.deepEqual(logic.cannonRayEnd([10, 20], to, object), expected, `프레임 ${frame + base}`);
        }
    }
    put(raw, 36, 5);
    assert.ok(throws(() => logic.cannonRayEnd([10, 20], [11, 30], object)));
    raw[10] = 129;
    assert.ok(throws(() => logic.cannonRayEnd([10, 20], [11, 30], object)));
    assert.ok(throws(() => new RawProjectileGeometry({ edition: 'cd1072' }, freshTypes(), freshGeometry())));
    assert.ok(throws(() => new RawProjectileGeometry(pool, freshTypes(), freshGeometry(), 0, 11)));
    assert.ok(throws(() => new RawProjectileGeometry(pool, freshTypes(), freshGeometry(), 16, 11, { pool: new SidPool(24000), get: () => 0 })));
});

test('ProjectileGeometry: 원본 사정선 끝 4040입력의 방향 불일치 사정거리 보정이 두 DWORD 까지 같다', { skip: !rayRows && '원본 fixture 없음' }, () => {
    const pool = new SidPool(24000, { server: false });
    const object = pool.allocate(2);
    const geometry = freshGeometry();
    const logic = new RawProjectileGeometry(pool, freshTypes(), geometry);
    const raw = pool.allocated(object);
    raw[10] = 128;
    let count = 0;
    // 기대 좌표는 재계산하지 않고 실제 PE 의 반환 DWORD 를 그대로 읽는다.
    for (const row of rayRows) {
        assert.equal(row.length, 8);
        put(raw, 36, Number(row[0]));
        geometry[128].range = bitsToFloat(Number(row[5]));
        const point = logic.cannonRayEnd([bitsToFloat(Number(row[1])), bitsToFloat(Number(row[2]))], [bitsToFloat(Number(row[3])), bitsToFloat(Number(row[4]))], object);
        assert.equal(floatToBits(point[0]), Number(row[6]), `행 ${count}`);
        assert.equal(floatToBits(point[1]), Number(row[7]), `행 ${count}`);
        count += 1;
    }
    assert.equal(count, RAY_ROWS);
});
