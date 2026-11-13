// 커서 위치 세 몸체를 cpppj CursorPlacementTests와 원본 cursor-placement-x86.tsv 573행으로 대조한다. (2026-11-14 WB04-E401)
import './nan-exact.mjs';
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, floatToBits, bitsToFloat } from './raw-support.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { CursorDropState } from '../www/core/raw-cursor-drop.js';
import { CursorLifecycleState } from '../www/core/raw-cursor-lifecycle.js';
import { CursorPlacementState, RawCursorPlacement, bindCursorPlacement, bindCursorPlacementAssets, bindCursorPlacementWorld } from '../www/core/raw-cursor-placement.js';
import { GameAssets } from '../www/assets/game-assets.js';
import { Renderer } from '../www/render/renderer.js';

// 저장된 원본10.78 입력과23개 열 순서다.
const rows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/cursor-placement-x86.tsv'));
const [OP, PROFILE, FLAGS1, FLAGS2, FOOT_X, FOOT_Y, SPACING, DESCRIBE, TEXT, OLD_GUMP, NEW_GUMP, ACCEPT, PLACE, OBSTRUCTION,
    REGION, CURSOR_FLAGS, PLACEMENT_FLAGS, MANA, X, Y, MATCH, MUTATION, RECT] = Array.from({ length: 23 }, (_, index) => index);

// 명시 decoder 입력이다. 기대 결과는 관측 파일에서 읽는다.
function cells(profile) {
    if (profile === 2) return [];
    if (profile === 4) return [{ frame: 0, x: bitsToFloat(0x7fc00000), y: Math.fround(1.1) }];
    if (profile === 5) return [{ frame: 0, x: Math.fround(-1.9998), y: Math.fround(-0.9999) }, { frame: 1, x: Math.fround(254.0002), y: Math.fround(255.0002) }];
    return profile === 0 ? [{ frame: 0, x: 0, y: 0 }] : [{ frame: 0, x: 0, y: 0 }, { frame: 1, x: 1.25, y: -0.5 }, { frame: 2, x: -2.75, y: 3.5 }];
}

// 실제 SHP 공급자 경계에 주는 독립 signed WORD 입력이다.
function image(profile, frame) {
    if (profile === 3 && frame === 0) return { token: 1, width: -11, height: -17, hotspotX: 23, hotspotY: -7 };
    const [width, height, hotspotX, hotspotY] = [[31, 23, 8, 17], [19, 29, -3, 31], [47, 13, 7, -5]][frame];
    return { token: 1, width, height, hotspotX, hotspotY };
}

// 매 행에서 같은 실제 풀과 세 커서 상태를 사용하는 원본 장면이다.
class PlacementScene {
    // 풀·현재 TYPE·표면 지도·공유 커서 상태와 명시 입력/사건·투영 경계다.
    pool = new SidPool(24000, { server: false }); types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0 }));
    surface = new Uint16Array(65536); drop = new CursorDropState(); life = new CursorLifecycleState(); state = new CursorPlacementState(); input = []; trace = []; projected = null;

    // 실제 client 번호를100번까지 순서대로 확보한다.
    constructor() { for (let sid = 5; sid <= 100; sid += 1) assert.equal(this.pool.allocate(2), sid); }

    // 원본 입력만 초기화하고 이전 행의 결과를 재사용하지 않는다.
    prepare(v = rows[0][0].split(',').map(Number)) {
        this.input = v; this.drop = new CursorDropState(); this.life = new CursorLifecycleState(); this.state = new CursorPlacementState(); this.trace = []; this.surface.fill(0);
        this.surface[0] = 80; this.surface[22 * 256 + 21] = 81; this.surface[21 * 256 + 20] = 82; this.surface[255 * 256 + 255] = 83;
        Object.assign(this.drop, { legal: 5, legalGump: 2, type: 100, argument: 7, direction: 3, preview: 50, costFlags: v[CURSOR_FLAGS], cursorGump: v[MANA] ? 1 : 0,
            x: bitsToFloat(v[X]), y: bitsToFloat(v[Y]), localPlayer: 0x123401, screenX: -3, screenY: 47 });
        Object.assign(this.life, { currentGump: v[OLD_GUMP] ? 1 : 0, abstractType: 100 }); this.life.previews[0] = 100;
        Object.assign(this.state, { placementAllowed: v[PLACE], descriptionAllowed: v[DESCRIBE], spaceAllowed: (1 - v[OBSTRUCTION]) >>> 0, sourceAllowed: v[MATCH],
            hotspotX: 44, hotspotY: 45, imageWidth: 91, imageHeight: 93, placementFlags: v[PLACEMENT_FLAGS], labelSpacing: v[SPACING] | 0,
            obstruction: v[OBSTRUCTION], acceptEvent: 0x123456, screenOrigin: { x: -17, y: 31 }, description: 'old' });
        this.state.bounds.set([3, 7, 43, 51]);
        Object.assign(this.types[100], { footX: v[FOOT_X] | 0, footY: v[FOOT_Y] | 0, flags1: v[FLAGS1], flags2: v[FLAGS2] });
        Object.assign(this.types[101], { footX: 7, footY: 9, flags1: v[FLAGS1], flags2: 4 });
        const size = [[35, 17, 49, 23], [-3, -7, -11, -13], [0x7fffffff, 0x80000001 | 0, 0x7ffffffd, 0x80000003 | 0]][v[RECT]];
        [this.life.warningWidth, this.life.warningHeight, this.drop.cursorWidth, this.life.costHeight] = size;
        const [x, y] = [[101, 203], [-3, -7], [0x7ffffffe, 0x80000002 | 0]][v[RECT]]; this.projected = { x, y };
        put(this.pool.allocated(100), 22, 0xfff9, 2); put(this.pool.allocated(100), 24, 0x8001, 2); return this;
    }

    // 원문 인수를 signed 비트 손실 없이 DWORD 십진수로 기록한다.
    event(letter, ...values) { this.trace.push([letter, ...values.map((value) => value >>> 0)].join(':')); }

    // 판정을 대신하지 않는 decoder/그림/공간/GUI/출력 경계다.
    hooks() {
        const v = this.input; const d = this.drop; const l = this.life; const s = this.state; const event = (letter, ...values) => this.event(letter, ...values);
        return {
            describe: (text, x, y, type, owner) => { event('D', floatToBits(x), floatToBits(y), type, owner); text.value = v[TEXT] === 0 ? 'AB' : v[TEXT] === 1 ? '' : 'ABCDE'; if (v[MUTATION] === 1) l.abstractType = 101; return v[DESCRIBE]; },
            walk: (query, visit) => {
                event('W', query.type, query.argument, query.direction, floatToBits(query.x), floatToBits(query.y), query.explicitFrame ? 1 : 0);
                // false이면 원본처럼 다음 칸 진행 사건을 생략한다.
                for (const cell of cells(v[PROFILE])) { cell.x = Math.fround(cell.x + query.x); cell.y = Math.fround(cell.y + query.y); if (!visit(cell)) return; event('N'); }
            },
            image: (type, frame) => { event('I', type, 0, frame); return image(v[PROFILE], frame); },
            gumpAt: (x, y) => { event('G', x, y, l.currentGump, d.legalGump); return v[NEW_GUMP] ? 1 : 0; },
            accept: (gump, code, first, second, third) => { event('A', gump, code, first, second, third); return v[ACCEPT] ? 2 : 0; },
            parent: (gump) => { assert.equal(gump, 2); return 77; },
            mayPlace: (query) => { event('P', query.type, query.argument, floatToBits(query.x), floatToBits(query.y), query.flags, query.owner, query.mode); if (v[MUTATION] === 2) s.obstruction = 1; return v[PLACE]; },
            source: (sid) => { event('U', sid); return 77; }, region: (sid) => { event('R', sid); return (sid === 77 || v[MATCH] ? v[REGION] : v[REGION] + 1) | 0; },
            processEvent: (process, code, first, second) => event('E', process, code, first, second), project: (x, y) => { event('J', floatToBits(x), floatToBits(y)); return this.projected; },
            invalidate: (rect, flags) => event('V', rect.left, rect.top, rect.right, rect.bottom, flags),
        };
    }

    // 원본17DWORD의 현재 상태를 그대로 직렬화한다.
    snapshot(result) {
        const d = this.drop; const l = this.life; const s = this.state;
        return [d.legal, s.placementAllowed, s.descriptionAllowed, s.spaceAllowed, s.sourceAllowed, l.currentGump, d.legalGump, ...s.bounds,
            s.hotspotX, s.hotspotY, s.imageWidth, s.imageHeight, l.abstractType, result].map((value) => value >>> 0).join(',');
    }

    // 같은 상태를 빌리는 실제 제품 처리기다.
    body(hooks = this.hooks(), surface = this.surface) { return new RawCursorPlacement(this.pool, this.types, surface, this.drop, this.life, this.state, hooks); }
}

test('CursorPlacement_OriginalBodiesMatchIndependentX86: 원본573행의 Query/Prepare/SourceAllowed·상태17DWORD·사건/설명이 같다', { skip: rows === null }, () => {
    assert.equal(rows.length, 573); const scene = new PlacementScene();
    // 각 원문23개 입력만 공급하고 기대 출력은 재계산하지 않는다.
    for (const row of rows) {
        assert.equal(row.length, 4); const input = row[0].split(',').map(Number); assert.equal(input.length, 23); scene.prepare(input); const body = scene.body(); let result = 0;
        if (input[OP] === 0) body.query(); else if (input[OP] === 1) body.prepare(); else result = body.sourceAllowed();
        assert.equal(scene.snapshot(result), row[2], row[0]); assert.equal(scene.trace.join(';') || '-', row[1], row[0]); assert.equal(scene.state.description, row[3], row[0]);
    }
});

test('CursorPlacement_BindsSharedLifecycleAndRequiresRealDependencies: 같은 준비/판정 바인딩과 선택된 의존/표면 지도/TYPE/부모 오류를 구별한다', () => {
    const scene = new PlacementScene().prepare([1, 0, 0, 0, 2, 3, 16, 1, 0, 0, 0, 0, 1, 0, 4, 0, 0, 0, 0, 0, 1, 0, 0]);
    const body = scene.body(); const lifecycle = {}; const drop = {}; bindCursorPlacement(lifecycle, drop, body);
    lifecycle.prepareCursor(); const first = scene.trace.slice(); scene.trace = []; drop.prepareCursor(); assert.deepEqual(scene.trace, first); assert.equal(body.pool, scene.pool);
    assert.equal(lifecycle.queryCursor, drop.queryCursor); assert.equal(lifecycle.prepareCursor, drop.prepareCursor);
    const missing = scene.body({}, []); assert.throws(() => missing.query(), /의존 동작/); scene.drop.inputFlags = 0x20; scene.drop.costFlags = 0; assert.equal(missing.sourceAllowed(), 1);
    scene.drop.costFlags = 0x20; assert.throws(() => missing.sourceAllowed(), /의존 동작/);
    assert.throws(() => new RawCursorPlacement({ edition: 'cd1072' }, [], [], scene.drop, scene.life, scene.state), /판본 계약/);
    scene.prepare(); scene.drop.costFlags = 0x20; scene.drop.x = 0; scene.drop.y = 0; assert.throws(() => scene.body(scene.hooks(), []).sourceAllowed(), /표면 지도 누락/);
    scene.prepare(); scene.life.abstractType = 188; assert.throws(() => scene.body().query(), /abstract TYPE/);
    scene.prepare(); scene.input[NEW_GUMP] = 1; scene.input[ACCEPT] = 1; const hooks = scene.hooks(); hooks.parent = () => 0; assert.throws(() => scene.body(hooks).query(), /parentSid/);
});

test('CursorPlacement.Prepare: 첫 출력이 바꾼 카메라 원점과 경고/비용 상태를 뒤 사각형에서 읽는다', () => {
    const scene = new PlacementScene().prepare(); scene.drop.costFlags = 0x40; scene.state.descriptionAllowed = 1; scene.state.spaceAllowed = 1; scene.state.placementAllowed = 1;
    scene.state.sharedScreenOrigin = new Int32Array([10, 20]); const rectangles = []; const hooks = scene.hooks();
    hooks.invalidate = (rect, flags) => { rectangles.push([rect, flags]); if (rectangles.length === 1) scene.state.sharedScreenOrigin.set([101, -203]); };
    scene.body(hooks).prepare(); assert.equal(rectangles.length, 2);
    assert.deepEqual(rectangles[1], [{ left: -148, top: -32564, right: -97, bottom: -32539 }, 0]);
});

test('커서 위치 자산/월드 바인딩: 실제 Canon·SHP·Renderer와 현재 TYPE·배치 경계만 연결하고 기존 GUI를 보존한다', () => {
    const { table, database, types } = realCaptureAssets(); const assets = new GameAssets(table, database); const geometry = { patternTypes: [107, 82, 94, 157, 131, 129, 140, 142], decoderDiagnostics: null };
    const hooks = {}; bindCursorPlacementAssets(hooks, assets, types, geometry); const cells = [];
    hooks.walk({ type: 92, argument: 0, direction: 0, x: 20, y: 21, explicitFrame: false }, (cell) => { cells.push(cell); return true; });
    assert.deepEqual(cells, [{ frame: table.definitions[22].defaultFrame, x: 20, y: 21 }]);
    const metrics = database.squidMetrics(22, 3); assert.deepEqual(hooks.image(92, 3), { token: 23, width: metrics.width, height: metrics.height, hotspotX: metrics.hotspotX, hotspotY: metrics.hotspotY });
    assert.throws(() => hooks.image(69, 0), /자산 TYPE/); assert.throws(() => hooks.image(92, -1), /frameCheck/); assert.throws(() => hooks.image(92, 10000), /frameCheck/);
    const pool = new SidPool(24000); const renderer = new Renderer(100, 80); renderer.clearDirty(); const calls = [];
    const placement = { pool, mayPlace: (query) => { calls.push(query); return 7; } }; const gui = () => 17; const connected = { gumpAt: gui };
    bindCursorPlacementWorld(connected, pool, assets, types, geometry, placement, renderer); const query = { type: 92, x: 1, y: 2 };
    assert.equal(connected.mayPlace(query), 1); assert.equal(calls[0], query); assert.equal(connected.gumpAt, gui);
    connected.invalidate({ left: 2, top: 3, right: 7, bottom: 9 }, 2); assert.deepEqual(renderer.cameraDirtyEntries(), [{ rect: { left: 2, top: 3, right: 7, bottom: 9 }, flags: 2 }]);
    assert.throws(() => bindCursorPlacementWorld({}, {}, assets, types, geometry, placement, renderer), /월드 계약/);
});
