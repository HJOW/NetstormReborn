// 일반 배치 미리보기 raw-canon-placement-preview.js(RawCanonPlacementPreview·훅 어댑터)를 원본 10.78 실제 PE 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: canonpreview-x86.tsv 의 `originals` 1620행 — 실제 일반 타입/패턴 배치 접두와 미리보기(12×12 차단 배열·표면 지도·소유 관계)를 사각형 12종×지도 5종×전역/소유자 9종×조회 중 변화 3종으로 실행해
//    허용 결과, 표시 차단 전역, 편집기/동맹/관계 표 전역, 표면 타입 genus, 144바이트 차단 배열(부분 쓰기 포함), 표면 슬롯 raw, spot·표면 지도 Adler-32, 모양/충돌 경계 호출 기록이 같다. 접두를 거쳐 재생한다.
//    `originalCD`·`original1037` 행은 다른 판본이라 사용하지 않는다.
//  - 계약: 실제 E146 기하 모듈의 모양 범위를 연결하고 미리보기 표시와 후반 거부가 독립이며 비로컬 요청은 배열을 건드리지 않음, 필수 하위 경계·지도 크기·산술 범위·관계 표 범위·다른 풀 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E152: 일반 배치 미리보기 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, bitsToFloat, floatToBits, adler32 } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { RawCanonPlacementGeometry } from '../www/core/raw-canon-placement-geometry.js';
import { RawCanonPlacement, createPlacementState, makeCanonShapePlacementHooks } from '../www/core/raw-canon-placement.js';
import { RawCanonPlacementPreview, createCanonPlacementPreviewState, makeCanonGeometryPreviewHooks, makeCanonPlacementPreviewHooks } from '../www/core/raw-canon-placement-preview.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-09 WB04-E152)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'canonpreview-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[0] === 'originals');

// 판본 행 수, 풀 크기, 지도 칸 수, 타입 수이다. 표면 네 개는 실제 실행기에 넣은 SID/타입/genus/소유자 값이다. (2026-11-09 WB04-E152)
const ROWS = 1620;
const CAPACITY = 24000;
const CELLS = 65536;
const TYPE_COUNT = 188;
const SURFACES = [
    { sid: 50, type: 83, genus: 2, owner: 1 },
    { sid: 51, type: 84, genus: 2, owner: 2 },
    { sid: 52, type: 85, genus: 4, owner: 0 },
    { sid: 53, type: 86, genus: 2, owner: 8 }
];

// 예외가 나는지 본다. (2026-11-09 WB04-E152)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 16진 문자열로 바꾼다. (2026-11-09 WB04-E152)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 새 타입 표와 프레임 표이다. (2026-11-09 WB04-E152)
const makeTypes = () => Array.from({ length: TYPE_COUNT }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0, group: 0, maxHitPoints: 0 }));
const makeFrames = () => Array.from({ length: TYPE_COUNT }, () => ({ frames: [], defaultFrame: 0 }));

// 다섯 지도 입력을 만든다. 원본 실행기에 공급한 주기와 같고 비대칭 주기로 x/y 전치와 경계 칸을 구별한다. (2026-11-09 WB04-E152)
const maps = (() => {
    const result = [];
    // 다섯 지도 변형을 차례로 만든다.
    for (let variant = 0; variant < 5; variant += 1) {
        const spots = new Uint8Array(CELLS);
        const surface = new Uint16Array(CELLS);
        // 모든 칸의 SID 와 spot 비트를 변형 규칙에 따라 정한다.
        for (let index = 0; index < CELLS; index += 1) {
            const x = index % 256;
            const y = Math.floor(index / 256);
            const choice = (x + 2 * y) % 5;
            let sid = 0;
            let spot = 0;
            if (variant === 1) { sid = 50; spot = 6; }
            if (variant === 2 || variant === 4) { sid = [0, 50, 51, 52, 53][choice]; spot = variant === 4 && (3 * x + y) % 7 === 0 ? 16 : 6; }
            if (variant === 3) { sid = choice !== 3 ? 0xffff : 52; spot = choice !== 3 ? 16 : 6; }
            spots[index] = spot;
            surface[index] = sid;
        }
        result.push({ spots, surface });
    }
    return result;
})();

test('일반 배치 미리보기: 원본 10.78 관측 1620행의 허용·차단 배열·표면 raw·지도 checksum·사건 기록이 같다', { skip: rows.length === 0 }, () => {
    assert.equal(allRows.length, 3 * ROWS);
    assert.equal(rows.length, ROWS);
    const pool = new SidPool(CAPACITY, { server: false });
    const types = makeTypes();
    const counts = { allowed: 0, rejected: 0, partial: 0 };
    // 기대 mask 를 구현으로 계산하지 않고 실제 PE 가 저장한 전체 배열과 비교한다.
    for (const row of rows) {
        assert.equal(row.length, 37);
        const events = [];
        const record = (event) => events.push(event);
        const query = { type: Number(row[1]), argument: Number(row[2]), x: bitsToFloat(Number(row[3])), y: bitsToFloat(Number(row[4])), flags: Number(row[6]), owner: Number(row[5]), mode: Number(row[7]) };
        types[query.type].flags2 = Number(row[8]) >>> 0;
        const placementState = createPlacementState();
        placementState.forcePlacement = Number(row[9]) >>> 0;
        placementState.localPlayer = Number(row[10]) | 0;
        placementState.blockedRelation = 0xffffffff;
        const rect = { left: bitsToFloat(Number(row[11])), top: bitsToFloat(Number(row[12])), right: bitsToFloat(Number(row[13])), bottom: bitsToFloat(Number(row[14])) };
        const lower = Number(row[15]);
        assert.equal(Number(row[16]), 0);
        const bounds = { left: Number(row[17]) | 0, top: Number(row[18]) | 0, right: Number(row[19]) | 0, bottom: Number(row[20]) | 0 };
        const variant = Number(row[21]);
        const spots = maps[variant].spots.slice();
        const surfaceMap = maps[variant].surface.slice();
        const previewState = createCanonPlacementPreviewState();
        previewState.blocked.fill(0xa5);
        previewState.editor = Number(row[22]) >>> 0;
        previewState.useAlliances = Number(row[23]) >>> 0;
        previewState.alliances[19] = Number(row[24]) >>> 0;
        const mutation = Number(row[25]);
        if ((query.owner & 255) === 255) previewState.alliances[8] = Number(row[24]) >>> 0;
        // 같은 raw/타입 입력을 매 행 복구한다. 실제 구현에서 free/dead 필터를 추가하면 결과가 달라진다.
        for (const surface of SURFACES) {
            const raw = pool.physical(surface.sid);
            raw.fill(0xcd);
            raw[10] = surface.type;
            raw[11] = (variant + surface.sid) % 16;
            raw[34] = surface.owner;
            types[surface.type].flags2 = surface.genus;
        }
        // 원본 실행기와 같은 조회 경계의 입력 변화를 공급한다.
        const mutate = () => {
            if (mutation === 1) {
                const index = (bounds.bottom + 1) * 256 + bounds.right + 1;
                if (index >= 0 && index < spots.length) spots[index] = 16;
            }
            if (mutation === 2) {
                previewState.editor = 0;
                previewState.useAlliances = 1;
                previewState.alliances[19] = 0;
                types[83].flags2 = 4;
                pool.physical(50)[34] = 8;
            }
        };
        const preview = new RawCanonPlacementPreview(pool, types, spots, surfaceMap, previewState, {
            bounds: (request) => {
                assert.ok(request.type === query.type && request.argument === query.argument && request.x === query.x && request.y === query.y && request.flags === query.flags);
                assert.ok(previewState.blocked.every((value) => value === 0));
                record(`B:${request.type}:${request.argument}:${floatToBits(request.x)}:${floatToBits(request.y)}:${request.flags}`);
                record(`R:${bounds.left}:${bounds.top}:${bounds.right}:${bounds.bottom}`);
                mutate();
                return bounds;
            },
            inspectCollisions: (request, localOwner) => {
                assert.ok(request.type === query.type && request.argument === query.argument && request.x === query.x && request.y === query.y);
                record(`C:${localOwner ? 1 : 0}:${request.owner}:${request.flags}:${request.mode}:${lower}`);
                return lower !== 0;
            }
        });
        const placement = new RawCanonPlacement(pool, types, placementState, makeCanonPlacementPreviewHooks(pool, preview, {
            shape: (type, argument, flags) => {
                assert.ok(type === query.type && argument === query.argument && flags === query.flags);
                record(`H:${type}:${argument}:${flags}`);
                return rect;
            }
        }));
        const allowed = placement.mayPlace(query);
        const where = row.slice(0, 26).join(' ');
        assert.equal(allowed, Number(row[26]) !== 0, where);
        assert.equal(placementState.blockedRelation >>> 0, Number(row[27]), where);
        assert.equal(previewState.editor >>> 0, Number(row[28]), where);
        assert.equal(previewState.useAlliances >>> 0, Number(row[29]), where);
        assert.equal(previewState.alliances[19], Number(row[30]), where);
        assert.equal(types[83].flags2 >>> 0, Number(row[31]), where);
        assert.equal(hex(previewState.blocked), row[32], where);
        assert.equal(hex(pool.slot(50)), row[33], where);
        assert.equal(adler32(spots), Number(row[34]), where);
        assert.equal(adler32(new Uint8Array(surfaceMap.buffer, surfaceMap.byteOffset, surfaceMap.byteLength)), Number(row[35]), where);
        assert.equal(events.join(';'), row[36], where);
        if (allowed) counts.allowed += 1; else counts.rejected += 1;
        if (previewState.blocked.some((value) => value === 0xa5)) counts.partial += 1;
    }
    // 허용과 거부, 지도 가장자리의 부분 배열이 모두 실제로 관찰되었는지 확인한다.
    assert.ok(counts.allowed > 0 && counts.rejected > 0 && counts.partial > 0, JSON.stringify(counts));
});

test('일반 배치 미리보기: 실제 E146 기하 모듈의 범위를 연결하고 표시와 후반 거부는 독립이며 비로컬 요청은 배열을 건드리지 않는다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const types = makeTypes();
    const frames = makeFrames();
    const geometryState = { patternTypes: [107, 82, 94, 157, 131, 129, 140, 142], decoderDiagnostics: null };
    types[157].footX = types[157].footY = 1;
    const codes = [];
    // 받침 패턴 0 은 방향 A..I 의 변형 번호 1 을 조회한다.
    for (let side = 65; side <= 73; side += 1) codes.push({ side, variant: 80, number: 1, flags: 0 });
    frames[157].frames = codes;
    const geometry = new RawCanonPlacementGeometry(pool, types, frames, geometryState);
    const spots = new Uint8Array(CELLS).fill(16);
    const surface = new Uint16Array(CELLS);
    const state = createCanonPlacementPreviewState();
    const placementState = createPlacementState();
    placementState.localPlayer = 1;
    let collisions = 0;
    const query = { type: 157, argument: 0, x: 20.5, y: 21.5, flags: 0, owner: 1, mode: 0 };
    const preview = new RawCanonPlacementPreview(pool, types, spots, surface, state, makeCanonGeometryPreviewHooks(pool, geometry, {
        inspectCollisions: (request, local) => {
            assert.ok(request.type === 157 && request.argument === 0 && local === (request.owner === 1));
            collisions += 1;
            return false;
        }
    }));
    const placement = new RawCanonPlacement(pool, types, placementState, makeCanonPlacementPreviewHooks(pool, preview, {
        shape: (type, argument) => {
            assert.ok(type === 157 && argument === 0);
            return { left: 0, top: 0, right: 48, bottom: 33 };
        }
    }));
    assert.ok(!placement.mayPlace(query) && collisions === 1);
    const area = geometry.bounds({ type: 157, argument: 0, direction: 0, x: 20.5, y: 21.5, explicitFrame: false });
    assert.deepEqual([area.left, area.top, area.right, area.bottom], [17, 18, 20, 21]);
    assert.equal(state.blocked.filter((value) => value === 1).length, 36);
    // 비로컬 요청은 실제 범위를 다시 조회하거나 미리보기를 초기화하지 않는다.
    state.blocked.fill(0xa5);
    query.owner = 2;
    assert.ok(!preview.inspect(query, false) && collisions === 2);
    assert.ok(state.blocked.every((value) => value === 0xa5));
    assert.ok(throws(() => makeCanonGeometryPreviewHooks(new SidPool(CAPACITY, { server: true }), geometry, {})));
    assert.ok(throws(() => makeCanonPlacementPreviewHooks(new SidPool(CAPACITY, { server: true }), preview, {})));
});

test('일반 배치 미리보기: 필수 경계·지도 크기·산술 범위·관계 표 범위를 거부한다', () => {
    const pool = new SidPool(CAPACITY, { server: false });
    const types = makeTypes();
    const spots = new Uint8Array(CELLS);
    const surfaceMap = new Uint16Array(CELLS);
    const state = createCanonPlacementPreviewState();
    let bounds = { left: 20, top: 21, right: 20, bottom: 21 };
    const hooks = { bounds: () => bounds, inspectCollisions: () => true };
    assert.ok(throws(() => new RawCanonPlacementPreview(pool, types, spots, surfaceMap, state, { inspectCollisions: hooks.inspectCollisions })));
    assert.ok(throws(() => new RawCanonPlacementPreview(pool, types, spots, new Uint16Array(1), state, hooks)));
    assert.ok(throws(() => new RawCanonPlacementPreview(pool, types, spots, surfaceMap, state, { ...hooks, inspectCollisions: null })));
    const preview = new RawCanonPlacementPreview(pool, types, spots, surfaceMap, state, hooks);
    const query = { type: 82, argument: 0, x: 20, y: 21, flags: 0, owner: 1, mode: 0 };
    // 열이 12칸을 넘으면 배열 범위 오류, 오른쪽이 최대 정수이면 산술 범위 오류이다.
    bounds = { left: 1, top: 1, right: 11, bottom: 1 };
    assert.ok(throws(() => preview.inspect(query, true)));
    bounds = { left: 1, top: 1, right: 2147483647, bottom: 1 };
    assert.ok(throws(() => preview.inspect(query, true)));
    // 10.78 은 y=254 이상이면 배열을 부분만 쓴 채 거짓이다.
    bounds = { left: 20, top: 255, right: 20, bottom: 255 };
    assert.equal(preview.inspect(query, true), false);
    // 방향 동맹 표의 인덱스가 음수가 되는 소유자는 진단한다(표면 소유자 1×9 + 요청자 −128).
    bounds = { left: 20, top: 21, right: 20, bottom: 21 };
    pool.physical(50)[10] = 83;
    pool.physical(50)[34] = 1;
    types[83].flags2 = 2;
    surfaceMap.fill(50);
    state.editor = 0;
    state.useAlliances = 1;
    assert.ok(throws(() => preview.inspect({ ...query, owner: 128 }, true)));
});
