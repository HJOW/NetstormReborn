// 커서 수명의 실제 관측565행·취소 진단156행·TYPE/사정거리854행과 자산/공유 상태 계약을 검사한다(cpppj CursorLifecycleTests, 10.78).
// 실제 게임·브라우저 실행이 아니다. ClientGameplayGlobals의 실제 대여/반납 검사는 그 소유자를 연결할 때 한다. (2026-11-14 WB04-E400)
import './nan-exact.mjs';
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT, readData } from './helpers.mjs';
import { readRows, put, floatToBits, bitsToFloat } from './raw-support.mjs';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { CursorDropState } from '../www/core/raw-cursor-drop.js';
import { ClientAiDebugGlobals } from '../www/core/client-ai-debug-globals.js';
import { CursorLifecycleState, RawCursorLifecycle, bindCursorLifecycle, bindCursorLifecycleAssets, hasCursorRange, cursorPreviewSid } from '../www/core/raw-cursor-lifecycle.js';
import { GameAssets } from '../www/assets/game-assets.js';
import { FontSet, ALL_FONT_PATHS } from '../www/assets/font-set.js';

// 각 원본 fixture의 고유 입력이다. 판본 열 없이 10.78 관측만 들어 있다.
const rows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/cursor-lifecycle-x86.tsv'));
const checkingRows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/cursor-cancel-checking-x86.tsv'));
const rangeRows = readRows(path.join(REPO_ROOT, 'cpppj/tests/fixtures/cursor-type-range-x86.tsv'));
// 원본 실행기의17개 입력 열과 명시 측정 입력이다.
const [OP, FLAGS, OLD_WORD, ARGUMENT, TYPE_FLAGS, COST, LOCK, PREVIEW, DEAD, PARENT_FLAGS, HIGHLIGHT_CHANGE, PREPARE_CHANGE,
    MANA, RANGE, LEGAL_GUMP, MANA_GUMP, IMAGE] = Array.from({ length: 17 }, (_, index) => index);
const WIDTHS = [20, 33, -3, 120, 55]; const HEIGHTS = [10, 30, -1, 20, 17];
// DWORD 입력과 실제 raw 직렬화다.
const numbers = (text) => text.split(',').map(Number);
const hex = (raw) => Buffer.from(raw).toString('hex');

// 현재 풀/TYPE/내려놓기/목록을 공유하는 원본 격리 장면이다.
class CursorScene {
    // 단일 raw 풀과 같은 현재188TYPE, 두 공유 상태, 입력과 효과 기록/호출 횟수다.
    pool = new SidPool(24000, { server: false });
    types = Array.from({ length: 188 }, () => ({ flags2: 0, cost: 0, group: 0 }));
    drop = new CursorDropState(); state = new CursorLifecycleState(); input = []; trace = []; prepared = 0; measured = 0;

    // 원본 입력에 쓰는101번까지 실제 client 영역에서 확보한다.
    constructor() { for (let sid = 5; sid <= 101; sid += 1) assert.equal(this.pool.allocate(2), sid); }

    // fixture 입력만 복원한다. 이전 행의 출력은 이어받지 않는다.
    prepare(v = numbers(rows[0][0])) {
        this.input = v; this.drop = new CursorDropState(); this.state = new CursorLifecycleState(); this.trace = []; this.prepared = 0; this.measured = 0;
        // 죽음 표식도 독립 입력으로 직접 복원하므로 물리 보기만 시험에서 빌린다.
        for (const sid of [50, 51, 77, 100, 101]) {
            const raw = this.pool.physical(sid); raw.fill(0xa5); put(raw, 0, 1);
            raw[11] = sid === 50 || sid === 51 ? v[DEAD] : sid === 77 ? v[PARENT_FLAGS] : 0;
            put(raw, 12, v[ARGUMENT], 2); put(raw, 18, 100, 2); put(raw, 20, v[OLD_WORD], 2);
        }
        this.types[100].flags2 = v[TYPE_FLAGS]; this.types[100].cost = bitsToFloat(v[COST]);
        Object.assign(this.drop, { type: 100, lockDepth: v[LOCK], inputFlags: v[OLD_WORD], costFlags: v[FLAGS], preview: v[OP] === 1 ? v[PREVIEW] : 0, cursorGump: v[OP] === 1 ? 1 : 0 });
        Object.assign(this.state, { abstractType: 100, previewCount: v[OP] === 2 ? v[PREVIEW] | 0 : 2, warningFont: 17, costFont: 19 });
        this.state.previews[0] = 100; this.state.previews[1] = 101; return this;
    }

    // signed 입력도 원본 DWORD 십진 표기로 기록한다.
    event(letter, ...values) { this.trace.push([letter, ...values.map((value) => value >>> 0)].join(':')); }

    // 계산을 대신하지 않고 관측 실행기와 같은 외부 입력/효과만 제공한다.
    hooks() {
        const d = this.drop; const s = this.state; const v = this.input; const event = (letter, ...values) => this.event(letter, ...values);
        return {
            highlight: (value) => { event('H', value); if (v[HIGHLIGHT_CHANGE] === 1) d.lockDepth = 3; if (v[HIGHLIGHT_CHANGE] === 2) d.preview = 51; },
            prepareCursor: () => { event('P'); if (v[PREPARE_CHANGE] && ++this.prepared === 1) s.previewCount = 1; },
            selectCursor: (value) => event('C', value), closePopups: () => event('X'), source: (sid) => { event('U', sid); return 77; },
            sourceNotice: (source, preview, flags) => event('N', source, preview, flags), detach: (sid, flags) => event('D', sid, flags),
            unpop: (sid, flags) => event('O', sid, flags), destroy: (sid, flags) => event('K', sid, flags), removeProcess: (process, flags) => event('R', process, flags),
            sound: (name) => event(`S:${name}`, 0, 0, 0, 0, 0), addLocalSp: (delta) => event('A', floatToBits(delta)),
            defaultArgument: (type) => { assert.equal(type, 100); return 11; },
            createPreviews: () => { event('B'); s.previews[0] = 100; s.previews[1] = 101; s.previewCount = 2; d.legalGump = v[LEGAL_GUMP] ? 44 : 0; },
            createMana: (first, flags) => { event('M', first, flags); s.currentGump = v[MANA_GUMP] ? 45 : 0; return v[MANA] ? 1 : 0; },
            createRange: (first, flags) => { event('G', first, flags); return 2; }, processEvent: (process, code, first, second) => event('E', process, code, first, second),
            typeNumber: (type) => { event('F', type); return 1; }, hasRange: (type) => { event('L', type); return v[RANGE] !== 0; },
            queryCursor: () => { event('Q'); s.currentGump = 46; }, translate: (text) => text,
            measure: (text, context) => {
                event(`T:${text}:${Array.from(context).join(',')}`, 0xffffffff); const index = this.measured++;
                return index < 5 ? [WIDTHS[index], HEIGHTS[index]] : [37, v[IMAGE] ? -9 : 12];
            },
            costImage: (type, frame) => { assert.deepEqual([type, frame], [92, 3]); return { token: 1, width: v[IMAGE] ? -11 : 23, height: v[IMAGE] ? -7 : 19, hotspotX: -3, hotspotY: 5 }; },
            mouse: () => ({ x: -3, y: 47 }), updatePosition: (x, y) => event('Z', x, y),
        };
    }

    // 현재 전역78DWORD를 순서 그대로 꺼낸다. 미사용 목록 칸도 전부 포함한다.
    snapshot() {
        const d = this.drop; const s = this.state;
        return [d.lockDepth, d.type, d.argument, d.direction, s.directionArgument, d.preview, d.inputFlags, d.costFlags, d.cursorGump, d.legalGump, s.currentGump,
            s.previewCount, s.warningWidth, s.warningHeight, s.costTextWidth, s.costHeight, s.costHotspotX, s.costHotspotY, d.cursorWidth, s.imageHeight,
            ...s.warningWidths, ...s.warningHeights, ...s.warningContext, ...s.costContext, ...s.previews, s.abstractType, s.costImage].map((value) => value >>> 0).join(',');
    }

    // 같은 현재 상태를 빌린 실제 제품 수명 몸체다.
    body(hooks = this.hooks()) { return new RawCursorLifecycle(this.pool, this.types, this.drop, this.state, hooks); }
}

// 관측의 사건/78DWORD/비용 문자열과 원문150바이트를 직접 비교한다.
function checkScene(scene, row, label = '') {
    assert.equal(scene.trace.join(';') || '-', row[1], label); assert.equal(scene.snapshot(), row[2], label); assert.equal(scene.state.costText, row[3], label);
    // 각 실제 슬롯의 전체 바이트를 따로 대조한다.
    for (const [index, sid] of [[4, 50], [5, 51], [6, 77]]) assert.equal(hex(scene.pool.slot(sid)), row[index], `${label} SID${sid}`);
}

test('CursorLifecycle_NativeTakeCancelAndActivate565: 원본565행의 취득/취소/정리·78DWORD·raw150바이트·측정/효과가 같다', { skip: rows === null }, () => {
    assert.equal(rows.length, 565); const scene = new CursorScene();
    // 17개 입력만 공급하고 기대값은 저장 관측에서 읽는다.
    for (const row of rows) {
        assert.equal(row.length, 7); const v = numbers(row[0]); assert.equal(v.length, 17); scene.prepare(v); const body = scene.body();
        if (v[OP] === 0) body.take(50, v[FLAGS]); else if (v[OP] === 1) body.cancel(); else body.activate(v[LOCK]);
        checkScene(scene, row, row[0]);
    }
});

// 명시 진단 중단을 구별하는 검사 예외다.
class AssertionStop extends Error {}

test('CursorLifecycle_NativeCancelCurrentCheckingAndDiagnosticReturn156: 반환음/진단 뒤 현재 checking·TYPE·음수0 비용과 중단 prefix가 같다', { skip: checkingRows === null }, () => {
    assert.equal(checkingRows.length, 156); const scene = new CursorScene();
    // 각 관측의 최초 checking과 두 명시 효과만 공급한다.
    for (const row of checkingRows) {
        assert.equal(row.length, 9); const v = numbers(row[0]); assert.equal(v.length, 20); scene.prepare(v.slice(3));
        const checking = { value: v[0] }; scene.state.checking = checking;
        scene.types[101].cost = bitsToFloat(0xc0700000); scene.types[102].cost = bitsToFloat(0x80000000); const hooks = scene.hooks();
        hooks.sound = (name) => { scene.event(`S:${name}`, 0, 0, 0, 0, 0); if (v[1]) { checking.value = v[1] === 1 ? 0 : 0x80000000; scene.state.abstractType = 101; } };
        hooks.assertion = (expression, file, line) => {
            assert.deepEqual([expression, file, line], ['false', '.\\Cursor.cpp', 0x3f1]); scene.event('D:66616c7365:2e5c437572736f722e637070:1009');
            if (v[2] === 2) throw new AssertionStop(); if (v[2] === 1) { checking.value = 0; scene.state.abstractType = 102; }
        };
        let outcome = 'return'; try { scene.body(hooks).cancel(); } catch (error) { if (!(error instanceof AssertionStop)) throw error; outcome = 'assertion'; }
        checkScene(scene, row, row[0]); assert.equal(outcome, row[7]); assert.equal(checking.value >>> 0, Number(row[8]));
    }
});

test('CursorLifecycle_NativeTypeNumberAndRange: 원본854행의 현재 그룹/flags2 사정거리 판정과 번호 범위가 같다', { skip: rangeRows === null }, () => {
    const types = Array.from({ length: 188 }, () => ({ group: 0, flags2: 0 })); assert.equal(rangeRows.length, 854);
    // 매번 같은 TYPE 칸을 덮어 이전 판정 캐시가 쓰이지 않도록 한다.
    for (const row of rangeRows) { const v = row.map(Number); types[v[0]].group = v[1] | 0; types[v[0]].flags2 = v[2]; assert.equal(v[0], v[3]); assert.equal(hasCursorRange(types, v[0]), v[4] !== 0); }
    assert.throws(() => hasCursorRange(types, 188), /TYPE 범위/); assert.throws(() => hasCursorRange([], 0), /TYPE 범위/);
});

test('CursorLifecycle_SharedMenuDropBindingsAndGuards: 같은 상태의 취득/취소·목록과 선택 경계/판본/큰 DWORD를 보존한다', () => {
    const scene = new CursorScene().prepare(); const body = scene.body(); const priest = {}; const drop = {}; bindCursorLifecycle(priest, drop, body);
    priest.takePreview(50, 0x160); assert.deepEqual([scene.drop.preview, scene.drop.lockDepth, scene.drop.costFlags, scene.state.previewCount], [50, 0, 0x160, 2]); assert.equal(body.pool, scene.pool);
    drop.activatePreview(0); assert.equal(scene.state.previewCount, 2); drop.invalidGui(); assert.deepEqual([scene.drop.preview, scene.drop.lockDepth, scene.state.previewCount], [0, 1, 0]);
    scene.prepare(); assert.throws(() => scene.body({}).take(50, 0x160), /경계 미연결/);
    assert.throws(() => new RawCursorLifecycle({ edition: 'cd1072' }, scene.types, scene.drop, scene.state, {}), /계약 오류/);
    assert.throws(() => new RawCursorLifecycle(scene.pool, scene.types.slice(1), scene.drop, scene.state, {}), /계약 오류/);
    scene.state.previewCount = 31; scene.state.previews.fill(100); assert.throws(() => scene.body().activate(1), /30칸/);
    assert.equal(cursorPreviewSid(65535), 65535); assert.throws(() => cursorPreviewSid(0x10032), /호스트SID범위/);
    scene.prepare(); scene.state.previews[0] = 0x10064; assert.throws(() => scene.body().activate(1), /호스트SID범위/); assert.equal(scene.state.previews[0], 0x10064);
});

test('CursorLifecycle 현재 진단: 실제 AiDebug 상자를 빌리고 선택된 누락 진단은 반환음 뒤에서만 실패한다', () => {
    const scene = new CursorScene().prepare([1, 0x80, 0x80, 0, 0, 0x437ac000, 0, 50, 0, 0, 0, 0, 1, 0, 0, 0, 0]);
    const ai = new ClientAiDebugGlobals(); scene.state.checking = ai.checking(); ai.checking().value = -1; assert.equal(scene.state.checkingValue(), 0xffffffff);
    assert.throws(() => scene.body().cancel(), /경계 미연결/); assert.equal(scene.trace.at(-1), 'S:teleporterActive.wav:0:0:0:0:0');
    assert.deepEqual([scene.drop.preview, scene.drop.cursorGump, scene.state.previewCount], [50, 1, 2]);
    scene.prepare([1, 0x80, 0x80, 0, 0, 0x437ac000, 0, 50, 0, 0, 0, 0, 1, 0, 0, 0, 0]); scene.state.checking = () => ai.checking().value;
    ai.checking().value = 0; scene.body().cancel(); assert.deepEqual([scene.drop.preview, scene.drop.lockDepth, scene.drop.cursorGump], [0, 1, 0]);
});

test('CursorLifecycle_ProtectedAssetsAndActualText·ActualAssetTypeRangeDuringTake: 실제 비용 SHP·글꼴 측정과 현재 TYPE를 취득에 연결한다', () => {
    const { table, database, types } = realCaptureAssets(); const assets = new GameAssets(table, database);
    const fonts = new FontSet(new Map(ALL_FONT_PATHS.map((name) => [name, readData(name)]))); const hooks = {}; bindCursorLifecycleAssets(hooks, assets, types, fonts);
    assert.equal(hooks.defaultArgument(123), table.definitions[53].defaultFrame >>> 0);
    const image = hooks.costImage(92, 3); const metrics = database.squidMetrics(22, 3);
    assert.deepEqual(image, { token: 23, width: metrics.width, height: metrics.height, hotspotX: metrics.hotspotX, hotspotY: metrics.hotspotY });
    assert.deepEqual(hooks.measure('123', new CursorLifecycleState().costContext), Object.values(fonts.measure(fonts.encode('123'), { flags: 2, font: 3, color: 0, style: 0, extra: 0, shadowColor: 0, shadowX: 1, shadowY: 1 })));
    assert.throws(() => hooks.defaultArgument(69), /자산 TYPE/); assert.throws(() => hooks.typeNumber(188), /TYPE 번호/); assert.throws(() => hooks.costImage(92, 10000), /frameCheck/);
    // 내장 타입의 번호 변환은 SHP 블록을 요구하지 않는다.
    for (let number = 0; number < 188; number += 1) assert.equal(hooks.typeNumber(number), number);
    assert.ok(hooks.hasRange(128)); assert.ok(!hooks.hasRange(92));
    const scene = new CursorScene().prepare(); scene.types = types; put(scene.pool.allocated(50), 18, 128, 2);
    const current = scene.hooks(); Object.assign(current, { defaultArgument: hooks.defaultArgument, typeNumber: hooks.typeNumber, hasRange: hooks.hasRange });
    scene.body(current).take(50, 0x160); assert.deepEqual([scene.state.abstractType, scene.drop.type, scene.drop.cursorGump], [128, 128, 2]); assert.ok(scene.trace.includes('G:100:0'));
    // 자산만 묶으면 기존 측정 함수는 유지한다.
    const oldMeasure = () => [1, 2]; const withoutText = { measure: oldMeasure }; bindCursorLifecycleAssets(withoutText, assets, types); assert.equal(withoutText.measure, oldMeasure);
    assert.throws(() => bindCursorLifecycleAssets({}, assets, types.slice(1)), /판본 계약/);
    const previous = types[92].flags2; types[92].flags2 |= 0x100; assert.ok(hooks.hasRange(92)); types[92].flags2 = previous; assert.ok(!hooks.hasRange(92));
    assert.deepEqual(hooks.measure('123\0ignored', new CursorLifecycleState().costContext), hooks.measure('123', new CursorLifecycleState().costContext));
});

test('CursorLifecycle.activate: 삭제 효과 뒤 현재 개수와 다음 DWORD를 읽고 실패 시 목록을 지우지 않는다', () => {
    const scene = new CursorScene().prepare(); let destroyed = [];
    const hooks = scene.hooks(); hooks.destroy = (sid) => { destroyed.push(sid); scene.state.previewCount = 1; };
    scene.body(hooks).activate(1); assert.deepEqual(destroyed, [100]); assert.equal(scene.state.previewCount, 0); assert.ok(scene.state.previews.every((sid) => sid === 0));
    scene.prepare(); destroyed = []; const changed = scene.hooks(); changed.destroy = (sid) => { destroyed.push(sid); scene.state.previews[1] = 0x10065; };
    assert.throws(() => scene.body(changed).activate(1), /호스트SID범위/);
    assert.deepEqual(destroyed, [100]); assert.deepEqual([scene.state.previewCount, ...scene.state.previews.slice(0, 2)], [2, 100, 0x10065]);
});

test('CursorLifecycle.take: 기본 인자 조회가 Form WORD를 바꾸면 뒤 입력 플래그는 같은 살아 있는 raw에서 읽는다', () => {
    const scene = new CursorScene().prepare(); const hooks = scene.hooks();
    hooks.defaultArgument = () => { put(scene.pool.allocated(50), 20, 0x3456, 2); return 11; };
    scene.body(hooks).take(50, 0);
    assert.equal(scene.drop.inputFlags, 0x3456);
    assert.equal(scene.pool.slot(50)[21], 0x35);
});
