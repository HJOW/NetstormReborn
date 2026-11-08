// 사제 배치 기하 raw-priest-placement-geometry.js(RawPriestPlacementGeometry·훅 어댑터)를 원본 10.78 실제 PE 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: priestgeometry-x86.tsv 의 `originals` 2304행 — 실제 비패턴 decoder(프레임 코드 개수·기본 프레임·인자·부호 DWORD 회전(홀수 포함)·좌표·명시 프레임·발자국)의 생성 직후 상태(프레임·유효·좌표 비트·미리보기 범위)와
//    모양별 finder 사각형, 한 칸 진행 뒤 상태(프레임·유효·좌표 비트·끝 범위)가 같다. `originalCD`·`original1037` 행은 다른 판본이라 사용하지 않는다.
//  - 계약: 지역 경계 호출 순서(begin → scan → end → finish)와 현재 발자국/기본 프레임 변경에 대한 값 캡처, 빈 모양의 begin/finish, 첫 거부 뒤 후속 생략, 비패턴 genus·패턴 전역·비유한 좌표·누락 자료·다른 풀·0 발자국 거부.
//  - 이월: C++ 의 비패턴 모양/미리보기 범위 → finder → 사제 생성자 통합 시험(`priest_geometry_preview_collision_spawn_factory`)은 사제 미리보기/충돌/지형 이식 뒤 통합 단위에서 다룬다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E156: 사제 배치 기하 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { decodeCanonType } from '../www/core/raw-canon-type.js';
import { RawPriestPlacementGeometry, makePriestGeometryCollisionHooks, makePriestGeometryPreviewHooks } from '../www/core/raw-priest-placement-geometry.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-09 WB04-E156)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'priestgeometry-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[0] === 'originals');

// 판본 행 수, 전체 행 수(세 PE), 풀 크기, 타입 수, 실제 사제 타입 번호이다. (2026-11-09 WB04-E156)
const ROWS = 2304;
const TOTAL = 4352;
const CAPACITY = 24000;
const TYPE_COUNT = 188;
const PRIEST = 158;

// 예외가 나는지 본다. (2026-11-09 WB04-E156)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 네 정수(배열 또는 사각형)를 원본 관찰의 순서(왼쪽:위:오른쪽:아래)로 직렬화한다. (2026-11-09 WB04-E156)
const area = (value) => (Array.isArray(value) ? value : [value.left, value.top, value.right, value.bottom]).join(':');

// 새 타입 표와 프레임 표이다. (2026-11-09 WB04-E156)
const makeTypes = () => Array.from({ length: TYPE_COUNT }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0, group: 0, maxHitPoints: 0 }));
const makeFrames = () => Array.from({ length: TYPE_COUNT }, () => ({ frames: [], defaultFrame: 0 }));

// 지역 몸체를 명시적으로 허용하는 입력 경계이다. 실제 지형 구현으로 해석하지 않는다. (2026-11-09 WB04-E156)
const allowRegions = () => ({ beginRegions: () => {}, endShape: () => true, finishRegions: () => true });

test('사제 배치 기하: 원본 10.78 관측 2304행의 비패턴 decoder 상태·범위·finder 사각형이 같다', { skip: rows.length === 0 }, () => {
    assert.equal(allRows.length, TOTAL);
    assert.equal(rows.length, ROWS);
    let validRows = 0;
    // 각 행에는 독립적인 프레임 메타/좌표/발자국 입력만 공급한다.
    for (const row of rows) {
        assert.equal(row.length, 22);
        const x = bitsToFloat(Number(row[4]));
        const y = bitsToFloat(Number(row[5]));
        const footX = Number(row[9]) | 0;
        const footY = Number(row[10]) | 0;
        const codes = Array.from({ length: Number(row[7]) }, () => ({ side: 0, variant: 0, number: 0, flags: 0 }));
        const decoder = decodeCanonType(codes, Number(row[8]) | 0, Array(8).fill(0), { type: PRIEST, argument: Number(row[2]) | 0, direction: Number(row[3]) | 0, x, y, explicitFrame: Number(row[6]) !== 0 });
        const where = row.slice(0, 11).join(' ');
        assert.equal(decoder.frame, Number(row[11]) | 0, where);
        assert.equal(decoder.valid, Number(row[12]) !== 0, where);
        assert.equal(floatToBits(decoder.x), Number(row[13]), where);
        assert.equal(floatToBits(decoder.y), Number(row[14]), where);
        assert.equal(area(decoder.bounds(footX, footY)), row[15], where);
        const rect = decoder.valid ? area(RawPriestPlacementGeometry.collisionArea(decoder.x, decoder.y, footX, footY)) : 'none';
        assert.equal(rect, row[16], where);
        if (decoder.valid) validRows += 1;
        decoder.advance();
        assert.equal(decoder.frame, Number(row[17]) | 0, where);
        assert.equal(decoder.valid, Number(row[18]) !== 0, where);
        assert.equal(floatToBits(decoder.x), Number(row[19]), where);
        assert.equal(floatToBits(decoder.y), Number(row[20]), where);
        assert.equal(area(decoder.bounds(footX, footY)), row[21], where);
    }
    // 유효 칸과 빈 칸이 모두 실제로 관찰되었는지 확인한다.
    assert.ok(validRows > 0 && validRows < ROWS, String(validRows));
});

test('사제 배치 기하: 지역 경계 호출 순서·현재 발자국/기본 프레임 변경·빈 모양·첫 거부 뒤 후속 생략을 지킨다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const types = makeTypes();
    const frames = makeFrames();
    const state = { patternTypes: Array(8).fill(0), decoderDiagnostics: null };
    types[PRIEST].flags2 = 0x210000;
    types[PRIEST].footX = types[PRIEST].footY = 1;
    frames[PRIEST].defaultFrame = 4;
    frames[PRIEST].frames = Array.from({ length: 5 }, () => ({ side: 0, variant: 0, number: 0, flags: 0 }));
    const query = { type: PRIEST, x: 20.5, y: 21.5, flags: 0, owner: 1, mode: 0 };
    const events = [];
    const geometry = new RawPriestPlacementGeometry(pool, types, frames, state, {
        beginRegions: (request) => {
            assert.equal(request.x, 20.5);
            events.push('begin');
            query.x = 99;
            types[PRIEST].footX = 3;
            types[PRIEST].footY = 2;
            frames[PRIEST].defaultFrame = -1;
        },
        endShape: (request, frame, x, y) => { assert.ok(request.x === 20.5 && frame === 4 && x === 20 && y === 21); events.push('end'); return true; },
        finishRegions: (request) => { assert.equal(request.x, 20.5); events.push('finish'); return true; }
    });
    assert.equal(geometry.inspect(query, (rect) => { events.push('scan'); assert.equal(area(rect), '18:20:21:22'); return true; }), true);
    assert.deepEqual(events, ['begin', 'scan', 'end', 'finish']);
    // 기본 프레임이 −1 이면 빈 모양이므로 시작과 마지막 판정만 불린다. 범위는 현재 발자국으로 계산한다.
    events.length = 0;
    query.x = 20.5;
    assert.equal(area(geometry.bounds(query)), '17:19:20:21');
    assert.equal(geometry.inspect(query, () => assert.fail('칸이 없다')), true);
    assert.deepEqual(events, ['begin', 'finish']);
    // 첫 거부는 후처리와 최종 판정을 생략한다.
    frames[PRIEST].defaultFrame = 4;
    query.x = 20.5;
    events.length = 0;
    assert.equal(geometry.inspect(query, () => false), false);
    assert.deepEqual(events, ['begin']);
});

test('사제 배치 기하: 선택 경계(beginShape)는 양수 발자국에서만 불리고 0 발자국은 거부한다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const types = makeTypes();
    const frames = makeFrames();
    types[PRIEST].flags2 = 0x210000;
    types[PRIEST].footX = types[PRIEST].footY = 1;
    frames[PRIEST].frames = [{ side: 0, variant: 0, number: 0, flags: 0 }];
    const shapes = [];
    const geometry = new RawPriestPlacementGeometry(pool, types, frames, { patternTypes: Array(8).fill(0), decoderDiagnostics: null }, {
        ...allowRegions(),
        beginShape: (request, frame, x, y) => shapes.push([request.type, frame, x, y])
    });
    assert.equal(geometry.inspect({ type: PRIEST, x: 20.5, y: 21.5, flags: 0, owner: 1, mode: 0 }, () => true), true);
    assert.deepEqual(shapes, [[PRIEST, 0, 20, 21]]);
    types[PRIEST].footX = 0;
    assert.ok(throws(() => geometry.inspect({ type: PRIEST, x: 20.5, y: 21.5, flags: 0, owner: 1, mode: 0 }, () => true)));
});

test('사제 배치 기하: 필수 경계·비사제/패턴 타입·비유한 좌표·다른 풀·0 발자국을 거부한다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const other = new SidPool(CAPACITY, { server: true });
    const types = makeTypes();
    const metadata = makeFrames();
    const state = { patternTypes: Array(8).fill(0), decoderDiagnostics: null };
    assert.ok(throws(() => new RawPriestPlacementGeometry(pool, types, metadata, state, {})));
    assert.ok(throws(() => new RawPriestPlacementGeometry(pool, types, metadata.slice(0, 1), state, allowRegions())));
    const geometry = new RawPriestPlacementGeometry(pool, types, metadata, state, allowRegions());
    const query = { type: PRIEST, x: 20, y: 21, flags: 0, owner: 1, mode: 0 };
    // 사제 genus 가 없으면 거부한다.
    assert.ok(throws(() => geometry.bounds(query)));
    types[PRIEST].flags2 = 0x200000;
    // 현재 패턴 전역이면 거부한다.
    state.patternTypes[4] = PRIEST;
    assert.ok(throws(() => geometry.bounds(query)));
    state.patternTypes[4] = 0;
    assert.ok(throws(() => geometry.bounds({ ...query, x: Infinity })));
    assert.ok(throws(() => geometry.inspect(query, null)));
    assert.ok(throws(() => makePriestGeometryPreviewHooks(other, geometry, {})));
    assert.ok(throws(() => makePriestGeometryCollisionHooks(other, geometry, {})));
    assert.ok(throws(() => RawPriestPlacementGeometry.collisionArea(20, 21, 0, 1)));
    // 어댑터는 기존 훅을 유지하고 연결한 경계만 바꾼다.
    const preview = makePriestGeometryPreviewHooks(pool, geometry, { keep: 1 });
    const collision = makePriestGeometryCollisionHooks(pool, geometry, { keep: 2 });
    assert.ok(preview.keep === 1 && typeof preview.bounds === 'function' && collision.keep === 2 && typeof collision.inspectRegions === 'function');
});
