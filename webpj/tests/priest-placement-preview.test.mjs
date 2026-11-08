// 사제 배치 미리보기 raw-priest-placement-preview.js(RawPriestPlacementPreview·접두 훅 어댑터)를 원본 10.78 실제 PE 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: priestpreview-x86.tsv 의 `originals` 1620행 — 실제 사제 배치 접두와 미리보기(12×12 차단 배열·표면 지도·소유 관계)를 사각형 12종×지도 5종×전역/소유자 9종×조회 중 변화 3종으로 실행해
//    허용 결과, 표시 차단 전역, 편집기/동맹/관계 표 전역, 표면 타입 genus, 144바이트 차단 배열(부분 쓰기 포함), 표면 슬롯 raw, spot·표면 지도 Adler-32, 모양/충돌 경계 호출 기록이 같다. 접두를 거쳐 재생한다.
//    `originalCD`·`original1037` 행은 다른 판본이라 사용하지 않는다.
//  - 계약: 필수 하위 경계·지도 크기·산술 범위·관계 표 범위·다른 풀 거부, 미리보기의 차단 표시가 실제 배치 허용 결과와 독립(사제 생성까지 연결).
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E157: 사제 배치 미리보기 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, bitsToFloat, floatToBits, adler32, get } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { createPlacementState } from '../www/core/raw-canon-placement.js';
import { createCanonPlacementPreviewState } from '../www/core/raw-canon-placement-preview.js';
import { RawPriestPlacement, makePriestSpawnPlacementHooks } from '../www/core/raw-priest-placement.js';
import { RawPriestPlacementPreview, makePriestPlacementPreviewHooks } from '../www/core/raw-priest-placement-preview.js';
import { PriestSpawnState, RawPriestSpawn } from '../www/core/raw-priest-spawn.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-09 WB04-E157)
const allRows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'priestpreview-x86.tsv')) ?? [];
const rows = allRows.filter((row) => row[0] === 'originals');

// 판본 행 수, 풀 크기, 지도 칸 수, 타입 수, 사제 타입 번호이다. 표면 네 개는 실제 실행기에 넣은 SID/타입/genus/소유자 값이다. (2026-11-09 WB04-E157)
const ROWS = 1620;
const CAPACITY = 24000;
const CELLS = 65536;
const TYPE_COUNT = 188;
const PRIEST = 158;
const SURFACES = [
    { sid: 50, type: 83, genus: 2, owner: 1 },
    { sid: 51, type: 84, genus: 2, owner: 2 },
    { sid: 52, type: 85, genus: 4, owner: 0 },
    { sid: 53, type: 86, genus: 2, owner: 8 }
];

// 예외가 나는지 본다. (2026-11-09 WB04-E157)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 16진 문자열로 바꾼다. (2026-11-09 WB04-E157)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 새 타입 표이다. (2026-11-09 WB04-E157)
const makeTypes = () => Array.from({ length: TYPE_COUNT }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0, group: 0, maxHitPoints: 0 }));

// 다섯 지도 입력을 만든다. 원본 실행기에 공급한 주기와 같고 비대칭 주기로 x/y 전치와 경계 칸을 구별한다. (2026-11-09 WB04-E157)
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

test('사제 배치 미리보기: 원본 10.78 관측 1620행의 허용·차단 배열·표면 raw·지도 checksum·사건 기록이 같다', { skip: rows.length === 0 }, () => {
    assert.equal(allRows.length, 3 * ROWS);
    assert.equal(rows.length, ROWS);
    const pool = new SidPool(CAPACITY, { server: false });
    const types = makeTypes();
    const counts = { allowed: 0, rejected: 0, partial: 0 };
    // 기대 mask 를 구현으로 계산하지 않고 실제 PE 가 저장한 전체 배열과 비교한다.
    for (const row of rows) {
        assert.equal(row.length, 36);
        const events = [];
        const record = (event) => events.push(event);
        const query = { type: Number(row[1]), x: bitsToFloat(Number(row[2])), y: bitsToFloat(Number(row[3])), flags: Number(row[5]), owner: Number(row[4]), mode: Number(row[6]) };
        types[query.type].flags2 = Number(row[7]) >>> 0;
        const placementState = createPlacementState();
        placementState.forcePlacement = Number(row[8]) >>> 0;
        placementState.localPlayer = Number(row[9]) | 0;
        placementState.blockedRelation = 0xffffffff;
        const rect = { left: bitsToFloat(Number(row[10])), top: bitsToFloat(Number(row[11])), right: bitsToFloat(Number(row[12])), bottom: bitsToFloat(Number(row[13])) };
        const lower = Number(row[14]);
        assert.equal(Number(row[15]), 0);
        const bounds = { left: Number(row[16]) | 0, top: Number(row[17]) | 0, right: Number(row[18]) | 0, bottom: Number(row[19]) | 0 };
        const variant = Number(row[20]);
        const spots = maps[variant].spots.slice();
        const surfaceMap = maps[variant].surface.slice();
        const previewState = createCanonPlacementPreviewState();
        previewState.blocked.fill(0xa5);
        previewState.editor = Number(row[21]) >>> 0;
        previewState.useAlliances = Number(row[22]) >>> 0;
        previewState.alliances[19] = Number(row[23]) >>> 0;
        const mutation = Number(row[24]);
        if ((query.owner & 255) === 255) previewState.alliances[8] = Number(row[23]) >>> 0;
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
        const preview = new RawPriestPlacementPreview(pool, types, spots, surfaceMap, previewState, {
            bounds: (request) => {
                assert.ok(request.type === query.type && request.x === query.x && request.y === query.y && request.flags === query.flags);
                assert.ok(previewState.blocked.every((value) => value === 0));
                record(`B:${request.type}:${floatToBits(request.x)}:${floatToBits(request.y)}:${request.flags}`);
                record(`R:${bounds.left}:${bounds.top}:${bounds.right}:${bounds.bottom}`);
                mutate();
                return bounds;
            },
            inspectCollisions: (request, localOwner) => {
                assert.ok(request.type === query.type && request.x === query.x && request.y === query.y);
                record(`C:${localOwner ? 1 : 0}:${request.owner}:${request.flags}:${request.mode}:${lower}`);
                return lower !== 0;
            }
        });
        const placement = new RawPriestPlacement(pool, types, placementState, makePriestPlacementPreviewHooks(pool, preview, {
            shape: (type, argument, flags) => {
                assert.ok(type === query.type && argument === query.type && flags === query.flags);
                record(`H:${type}:${argument}:${flags}`);
                return rect;
            }
        }));
        const allowed = placement.mayPlace(query);
        const where = row.slice(0, 25).join(' ');
        assert.equal(allowed, Number(row[25]) !== 0, where);
        assert.equal(placementState.blockedRelation >>> 0, Number(row[26]), where);
        assert.equal(previewState.editor >>> 0, Number(row[27]), where);
        assert.equal(previewState.useAlliances >>> 0, Number(row[28]), where);
        assert.equal(previewState.alliances[19], Number(row[29]), where);
        assert.equal(types[83].flags2 >>> 0, Number(row[30]), where);
        assert.equal(hex(previewState.blocked), row[31], where);
        assert.equal(hex(pool.slot(50)), row[32], where);
        assert.equal(adler32(spots), Number(row[33]), where);
        assert.equal(adler32(new Uint8Array(surfaceMap.buffer, surfaceMap.byteOffset, surfaceMap.byteLength)), Number(row[34]), where);
        assert.equal(events.join(';'), row[35], where);
        if (allowed) counts.allowed += 1; else counts.rejected += 1;
        if (previewState.blocked.some((value) => value === 0xa5)) counts.partial += 1;
    }
    // 허용과 거부, 지도 가장자리의 부분 배열이 모두 실제로 관찰되었는지 확인한다.
    assert.ok(counts.allowed > 0 && counts.rejected > 0 && counts.partial > 0, JSON.stringify(counts));
});

test('사제 배치 미리보기: 필수 경계·지도 크기·산술 범위·관계 표 범위·다른 풀을 거부한다', () => {
    const pool = new SidPool(CAPACITY, { server: false });
    const types = makeTypes();
    const spots = new Uint8Array(CELLS);
    const surfaceMap = new Uint16Array(CELLS);
    const state = createCanonPlacementPreviewState();
    let bounds = { left: 20, top: 21, right: 20, bottom: 21 };
    const hooks = { bounds: () => bounds, inspectCollisions: () => true };
    const query = { type: PRIEST, x: 20, y: 21, flags: 0, owner: 1, mode: 0 };
    assert.ok(throws(() => new RawPriestPlacementPreview(pool, types, spots, surfaceMap, state, { inspectCollisions: hooks.inspectCollisions })));
    assert.ok(throws(() => new RawPriestPlacementPreview(pool, types, spots, new Uint16Array(1), state, hooks)));
    const preview = new RawPriestPlacementPreview(pool, types, spots, surfaceMap, state, hooks);
    bounds = { left: 1, top: 1, right: 11, bottom: 1 };
    assert.ok(throws(() => preview.inspect(query, true)));
    bounds = { left: 1, top: 1, right: 2147483647, bottom: 1 };
    assert.ok(throws(() => preview.inspect(query, true)));
    bounds = { left: 20, top: 255, right: 20, bottom: 255 };
    assert.equal(preview.inspect(query, true), false);
    bounds = { left: 20, top: 21, right: 20, bottom: 21 };
    pool.physical(50)[10] = 83;
    pool.physical(50)[34] = 1;
    types[83].flags2 = 2;
    surfaceMap.fill(50);
    state.editor = 0;
    state.useAlliances = 1;
    assert.ok(throws(() => preview.inspect({ ...query, owner: 128 }, true)));
    assert.ok(throws(() => makePriestPlacementPreviewHooks(new SidPool(CAPACITY, { server: false }), preview, {})));
});

test('사제 배치 미리보기: 차단 표시는 실제 배치 허용 결과와 독립이며 사제 생성까지 연결된다', () => {
    const pool = new SidPool(CAPACITY, { server: true });
    const types = makeTypes();
    types[PRIEST].flags2 = 0x210000;
    const spots = new Uint8Array(CELLS).fill(6);
    const surfaceMap = new Uint16Array(CELLS);
    const previewState = createCanonPlacementPreviewState();
    const placementState = createPlacementState();
    placementState.localPlayer = 1;
    placementState.blockedRelation = 0xffffffff;
    let collisions = 0;
    let pops = 0;
    let born = 0;
    const preview = new RawPriestPlacementPreview(pool, types, spots, surfaceMap, previewState, {
        bounds: (query) => { const x = Math.trunc(query.x); const y = Math.trunc(query.y); return { left: x, top: y, right: x, bottom: y }; },
        inspectCollisions: (_query, local) => { assert.ok(local); collisions += 1; return true; }
    });
    const placement = new RawPriestPlacement(pool, types, placementState, makePriestPlacementPreviewHooks(pool, preview, { shape: () => ({ left: 0, top: 0, right: 16, bottom: 11 }) }));
    const spawn = new RawPriestSpawn(pool, types, spots, new PriestSpawnState(), makePriestSpawnPlacementHooks(pool, placement, {
        create: (type) => { born = pool.allocate(); pool.allocated(born)[10] = type; return born; },
        setOwner: () => {},
        pop: (sid, x, y, flags) => { assert.ok(sid === born && x === 21 && y === 22 && flags === 0); pops += 1; },
        notifySurface: () => assert.fail('다리 타입이 아니므로 표면 알림은 없다'),
        checkCarrier: (sid) => { assert.equal(sid, born); }
    }));
    spawn.spawn(20.75, 21.9, PRIEST, 0x8101);
    assert.ok(born === 15000 && collisions === 1 && pops === 1 && get(pool.slot(born), 12, 2) === 0x8101);
    assert.equal(previewState.blocked.filter((value) => value === 1).length, 9);
});
