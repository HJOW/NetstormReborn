// 공통 표시 갱신 squid-display.js(SquidDisplay)·SHP 프레임 접근 raw-rift-shape-access.js·변경 표 dirty-regions.js 를 원본 10.78 관측과 cpppj 계약으로 대조한다.
//  - 원본 관측: display-x86.tsv 의 10.78(originals) 구간(Update 200·Bounds·Pop 240·Unpop 240: 표시 억제·카메라·확대율·선택 표식·그림자·변경 표 병합/넘침과 raw 슬롯 전체)과
//    geyser-partial-display-x86.tsv 432행(간헐천 부분 8c: 현재 기본 프레임·무효 raw 프레임·WORD/extra·확대율/클리핑의 실제 출력). CD 10.72 구간은 웹 범위(10.78) 밖이다.
//  - 계약: SquidDisplayLive_*(진단 복귀 뒤 현재 TYPE/좌표 재조회, 상세 writer → 진단 순서와 고정 프레임, 억제와 부모 순서)·buildSquidFrameAssertionDetail·SetView 거부.
//    픽셀을 그리는 Renderer 어댑터·SHP 헤더 판독(ShapeDatabase)·실제 World 저장소 연결·마나/볼트/신전 rune 부분 표시(RawCursorMana 등)는 그 파일의 이식 단위 몫이다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-08 WB04-E59: 객체 표시 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, get, bitsToFloat, floatToBits } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { SquidHash } from '../www/core/squid-hash.js';
import { SquidPop, RAW_POP_RESULT } from '../www/core/squid-pop.js';
import { SquidUnpop } from '../www/core/squid-unpop.js';
import { SquidFactory } from '../www/core/squid-factory.js';
import { SquidDisplay, buildSquidFrameAssertionDetail } from '../www/core/squid-display.js';
import { readRiftShapeFrame, RIFT_SHAPE_ASSERTION } from '../www/core/raw-rift-shape-access.js';
import { DirtyRegions, DIRTY_CAPACITY } from '../www/render/dirty-regions.js';

// 원본 관측 파일이다. 없으면 해당 검사를 건너뛴다. (2026-11-08 WB04-E59)
const FIXTURES = path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures');
const displayRows = readRows(path.join(FIXTURES, 'display-x86.tsv'));
const geyserRows = readRows(path.join(FIXTURES, 'geyser-partial-display-x86.tsv'));

// 예외가 나는지 본다. (2026-11-08 WB04-E59)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// 바이트 배열을 소문자 hex 로 만든다. (2026-11-08 WB04-E59)
const hex = (bytes) => Buffer.from(bytes).toString('hex');

// 변경 표 정수 목록(쉼표/세미콜론 구분, "-" 는 빈 목록)을 읽는다. (2026-11-08 WB04-E59)
const integers = (text) => text.replace(/[,;]/g, ' ').split(/\s+/).filter((part) => part !== '' && part !== '-').map(Number);

// 변경 표(원본 순서)를 fixture 형식 숫자 목록과 비교한다: 전체 갱신 여부와 항목마다 [left, top, right, bottom, flags]. (2026-11-08 WB04-E59)
function checkDirty(sink, full, text) {
    assert.equal(sink.suppressed(), Number(full) !== 0);
    const expected = integers(text);
    assert.equal(sink.dirty.entries.length * 5, expected.length);
    // 기대 표와 실제 표 크기가 달라도 범위 밖을 읽지 않는다.
    sink.dirty.entries.forEach((entry, index) => {
        const n = index * 5;
        assert.deepEqual([entry.rect.left, entry.rect.top, entry.rect.right, entry.rect.bottom, entry.flags], expected.slice(n, n + 5));
    });
}

// 원본 표시 억제(첫 화면 force + 변경 표 100항목 넘침)를 독립 변경 표에 연결한 출력 장치이다. (2026-11-08 WB04-E59)
function dirtySink(width, height, forced) {
    const dirty = new DirtyRegions(width, height);
    return {
        dirty,
        suppressed: () => forced || dirty.fullRedraw,
        invalidate: (rect, flags) => dirty.add(rect, flags)
    };
}

// 선택한 raw 타입 번호에 대응하는 최소 타입 표(188칸)이다: walker 일반 공간 경로는 지도에 spot 을 쓰지 않는다. (2026-11-08 WB04-E59)
function displayTypes() {
    return Array.from({ length: 188 }, () => ({ footX: 1, footY: 1, flags1: 0, flags2: 0x10000, maxHitPoints: 91, zOrder: 7, defaultFrame: 0, name: '' }));
}

test('SquidDisplay: 원본 10.78 활성 표시의 변경 표·경계·Pop/Unpop 과 raw 슬롯이 같다', { skip: !displayRows && '원본 fixture 없음' }, () => {
    let pool = null;
    let hash = null;
    let factory = null;
    let display = null;
    let pop = null;
    let unpop = null;
    let sink = null;
    let types = null;
    let shapes = null;
    let spots = null;
    let sid = 0;
    let vtable = 0;
    let patch = false;
    const counts = { begins: 0, updates: 0, bounds: 0, pops: 0, unpops: 0 };
    // Begin/Type/View/Slot 은 원본 로더를 완료했다고 주장하지 않는 명시적 준비 입력이다.
    for (const row of displayRows) {
        if (row[0] === 'Begin') {
            patch = row[1] === 'originals';
            if (!patch) continue;
            types = displayTypes();
            shapes = types.map(() => ({ frameCount: 0, loaded: false, frames: [] }));
            spots = new Uint8Array(65536);
            pool = new SidPool(32768, { server: false });
            hash = new SquidHash();
            factory = new SquidFactory(pool, types);
            sid = factory.create(74, 2);
            assert.equal(sid, Number(row[3]));
            const slot = pool.slot(sid);
            vtable = (slot[0] | (slot[1] << 8) | (slot[2] << 16) | (slot[3] << 24)) >>> 0;
            counts.begins += 1;
            continue;
        }
        if (!patch) continue;
        if (row[0] === 'Type') {
            types[74].flags1 = Number(row[1]) >>> 0;
            shapes[74].frameCount = Number(row[2]);
            shapes[74].loaded = Number(row[3]) !== 0;
            types[74].maxHitPoints = Number(row[4]) | 0;
            const values = integers(row[5]);
            assert.equal(values.length % 4, 0);
            shapes[74].frames = [];
            // 각 물리 프레임의 signed short 헤더를 순서대로 입력한다.
            for (let index = 0; index + 3 < values.length; index += 4) {
                shapes[74].frames.push({ width: values[index], height: values[index + 1], hotspotX: values[index + 2], hotspotY: values[index + 3], cellWidth: 0, cellHeight: 0 });
            }
        } else if (row[0] === 'View') {
            sink = dirtySink(Number(row[1]), Number(row[2]), Number(row[7]) !== 0);
            const v = integers(row[6]);
            assert.equal(v.length, 4);
            display = new SquidDisplay(types, shapes, sink, { cameraX: Number(row[3]), cameraY: Number(row[4]), zoom: Number(row[5]), viewport: { left: v[0], top: v[1], right: v[2], bottom: v[3] } });
            pop = new SquidPop(pool, hash, spots, { display });
            unpop = new SquidUnpop(pool, hash, spots, display);
        } else if (row[0] === 'Slot') {
            hash.reset();
            const raw = pool.allocated(sid);
            raw.fill(0);
            put(raw, 0, vtable);
            raw[10] = 74;
            raw[11] = Number(row[3]);
            put(raw, 36, Number(row[1]) >>> 0);
            raw[40] = Number(row[2]);
            put(raw, 14, Number(row[4]));
            put(raw, 18, Number(row[5]));
        } else if (row[0] === 'Update') {
            const before = hex(pool.slot(sid));
            display.update(pool.slot(sid), Number(row[1]));
            assert.equal(hex(pool.slot(sid)), before);
            checkDirty(sink, row[2], row[3]);
            counts.updates += 1;
        } else if (row[0] === 'Bounds') {
            const r = display.bounds(74, Number(row[1]), bitsToFloat(row[2]), bitsToFloat(row[3]));
            assert.deepEqual([r.left, r.top, r.right, r.bottom], [4, 5, 6, 7].map((index) => Number(row[index])));
            counts.bounds += 1;
        } else if (row[0] === 'Pop') {
            assert.equal(pop.pop(sid, types[74], 1, 1, bitsToFloat(row[2]), bitsToFloat(row[3]), Number(row[1])), RAW_POP_RESULT.registered);
            assert.equal(hex(pool.slot(sid)), row[4]);
            checkDirty(sink, row[5], row[6]);
            counts.pops += 1;
        } else if (row[0] === 'Unpop') {
            unpop.unpop(sid, types[74], Number(row[1]));
            assert.equal(hex(pool.slot(sid)), row[2]);
            checkDirty(sink, row[3], row[4]);
            counts.unpops += 1;
        } else assert.fail(`알 수 없는 행 ${row[0]}`);
    }
    assert.equal(counts.begins, 2);
    assert.ok(counts.updates === 200 && counts.pops === 240 && counts.unpops === 240 && counts.bounds > 150, JSON.stringify(counts));
});

test('SquidDisplay: 원본 10.78 간헐천 부분 8c 432행의 변경 표와 raw 불변이 같다', { skip: !geyserRows && '원본 fixture 없음' }, () => {
    const types = Array.from({ length: 188 }, () => ({ footX: 0, footY: 0, flags1: 0, flags2: 0, maxHitPoints: 0, defaultFrame: 0, name: '' }));
    const shapes = types.map(() => ({ frameCount: 0, loaded: false, frames: [] }));
    // 원본 SHP 입력의 signed 픽셀/핫스폿과 별도 해시 칸 크기를 그대로 만든다.
    const geyserShape = () => ({
        frameCount: 4,
        loaded: true,
        frames: [[100, 180, -3, 5], [130, 60, 7, -9], [50, 260, 2, 3], [80, 90, 4, 6]].map(([width, height, hotspotX, hotspotY]) => ({ width, height, hotspotX, hotspotY, cellWidth: 1, cellHeight: 1 }))
    });
    shapes[74] = geyserShape();
    shapes[75] = geyserShape();
    types[74].maxHitPoints = 91;
    types[75].maxHitPoints = 91;
    const sink = { entries: [], flag: false, suppressed: () => sink.flag, invalidate: (rect, flags) => sink.entries.push([rect, flags]) };
    const display = new SquidDisplay(types, shapes, sink, { cameraX: 7, cameraY: 9, zoom: 65536, viewport: { left: 12, top: 9, right: 620, bottom: 450 } });
    // 실제 요청 순서의 사각형/flags 를 원문 형식 문자열로 직렬화한다.
    const packed = () => (sink.entries.length === 0 ? '-' : sink.entries.map(([r, f]) => `${r.left},${r.top},${r.right},${r.bottom},${f}`).join(';'));
    let count = 0;
    // fixture 입력과 독립 원본 기대값을 한 행씩 직접 대조한다.
    for (const row of geyserRows) {
        assert.equal(row.length, 12);
        types[74].defaultFrame = Number(row[1]) | 0;
        types[75].defaultFrame = (types[74].defaultFrame + 1) % 3;
        types[74].flags1 = Number(row[6]) >>> 0;
        types[75].flags1 = Number(row[6]) >>> 0;
        sink.flag = Number(row[8]) !== 0;
        sink.entries = [];
        display.setView({ cameraX: 7, cameraY: 9, zoom: Number(row[4]) | 0, viewport: { left: 12, top: 9, right: 620, bottom: 450 } });
        const raw = new Uint8Array(50);
        // 원본 실행 직전 50바이트만 복사한다.
        for (let index = 0; index < raw.length; index += 1) raw[index] = Number.parseInt(row[9].slice(index * 2, index * 2 + 2), 16);
        const before = raw.slice();
        display.validate(raw, 0x2000);
        assert.equal(sink.entries.length, 0);
        display.update(raw, 0x2000);
        assert.deepEqual(raw, before);
        assert.equal(Number(row[10]), Number(row[8]));
        assert.equal(packed(), row[11]);
        count += 1;
    }
    assert.equal(count, 432);
});

// 실제 공통 88 과 서로 다른 두 타입/프레임의 최소 물리 입력이다: 진단 장치가 현재 raw/TYPE 를 바꾸는 명시 입력을 만든다. (2026-11-08 WB04-E59)
function liveScene() {
    const types = displayTypes();
    const shapes = types.map(() => ({ frameCount: 0, loaded: false, frames: [] }));
    types[70].name = 'oldType';
    types[71].name = 'newType';
    shapes[70] = { frameCount: 1, loaded: true, frames: [{ width: 0, height: 0, hotspotX: 0, hotspotY: 0, cellWidth: 0, cellHeight: 0 }] };
    shapes[71] = { frameCount: 3, loaded: true, frames: [0, 1, 2].map(() => ({ width: 0, height: 0, hotspotX: 0, hotspotY: 0, cellWidth: 0, cellHeight: 0 })) };
    shapes[71].frames[0].width = 10;
    shapes[71].frames[0].height = 12;
    shapes[71].frames[2].width = 20;
    shapes[71].frames[2].height = 6;
    const raw = new Uint8Array(50);
    put(raw, 0, 0x501dd0);
    raw[10] = 70;
    raw[40] = 1;
    const scene = { types, shapes, raw, checking: 1, detail: null, events: [], rectangles: [], markers: [], suppressedValue: false, errorEffect: null, assertionEffect: null };
    const sink = {
        checking: () => scene.checking,
        suppressed: () => { scene.events.push('S'); return scene.suppressedValue; },
        invalidate: (rect, flags) => { scene.events.push('I'); scene.rectangles.push({ ...rect }); scene.markers.push(flags); },
        frameError: (type, frame, abstract) => {
            scene.events.push('E');
            scene.detail = buildSquidFrameAssertionDetail(types, shapes, type, frame, abstract);
            if (scene.errorEffect) scene.errorEffect();
        },
        assertion: (expression, file, line) => {
            assert.ok(expression === 'getTypeStruct()->frameCheck( isAbstract(), theFrame, true )' && file === '\\Ns\\O\\Squid.cpp' && line === 0x846);
            scene.events.push('D');
            if (scene.assertionEffect) scene.assertionEffect();
        }
    };
    scene.display = new SquidDisplay(types, shapes, sink, { cameraX: 0, cameraY: 0, zoom: 65536, viewport: { left: 0, top: 0, right: 640, bottom: 480 } });
    // raw frame 을 바꿔도 부모의 지역값은 고정되어야 한다.
    scene.changeCurrent = (extra) => {
        raw[10] = 71;
        put(raw, 36, 9);
        put(raw, 14, floatToBits(4));
        put(raw, 18, floatToBits(5));
        raw[40] = extra;
        scene.checking = 0;
    };
    return scene;
}

test('SquidDisplayLive: SHP 해제 표식 진단이 복귀하면 현재 새 TYPE/좌표와 옛 지역 프레임으로 Mark 를 실행한다', () => {
    const scene = liveScene();
    scene.types[70].maxHitPoints = 0xdddddddd | 0;
    scene.assertionEffect = () => scene.changeCurrent(4);
    scene.display.update(scene.raw);
    assert.deepEqual(scene.events, ['D', 'S', 'I']);
    assert.equal(scene.detail, null);
    assert.deepEqual(scene.markers, [16]);
    assert.deepEqual(scene.rectangles[0], { left: 52, top: 40, right: 87, bottom: 68 });
    assert.equal(get(scene.raw, 36), 9);
});

test('SquidDisplayLive: 오류 상세 writer 가 checking 을 꺼도 이미 선택된 진단을 실행하고 고정 프레임을 유지한다', () => {
    const scene = liveScene();
    put(scene.raw, 36, 2);
    scene.errorEffect = () => scene.changeCurrent(0);
    scene.display.update(scene.raw);
    assert.deepEqual(scene.events, ['E', 'D', 'S', 'I']);
    assert.equal(scene.detail, 'Frame assertion failed. Asking for type=70 (oldType) frame=2 abs=1\n\nType has 1 frames\n');
    assert.deepEqual(scene.rectangles[0], { left: 64, top: 55, right: 85, bottom: 62 });
    assert.equal(scene.markers[0], 0);
});

test('SquidDisplayLive: 표시 억제는 부모 진단을 건너뛰지 않고 Mark 자식만 억제하며 무효 TYPE 상세는 부호 있는 범위로 판정한다', () => {
    const scene = liveScene();
    scene.suppressedValue = true;
    put(scene.raw, 36, 2);
    scene.display.update(scene.raw);
    assert.deepEqual(scene.events, ['E', 'D', 'E']);
    scene.events.length = 0;
    put(scene.raw, 36, 0);
    scene.display.update(scene.raw);
    assert.deepEqual(scene.events, ['S']);
    assert.equal(scene.rectangles.length, 0);
    assert.equal(buildSquidFrameAssertionDetail(scene.types, scene.shapes, 0xffffffff, -5, 0), 'Frame assertion failed. Asking for type=-1 (invalidType) frame=-5 abs=0\n\nType has 0 frames\n');
    assert.ok(throws(() => buildSquidFrameAssertionDetail(scene.types, scene.shapes.slice(1), 70, 0, 0)));
});

test('SquidDisplay: 카메라/viewport 변경 거부와 변경 표 병합·넘침, SHP 접근 순서를 보존한다', () => {
    const scene = liveScene();
    assert.ok(throws(() => scene.display.setView({ cameraX: 0, cameraY: 0, zoom: 0, viewport: { left: 0, top: 0, right: 640, bottom: 480 } })));
    assert.ok(throws(() => scene.display.setView({ cameraX: 0, cameraY: 0, zoom: 65536, viewport: { left: 10, top: 0, right: 0, bottom: 480 } })));
    // 변경 표: 화면 밖은 무시하고 100항목을 넘으면 전체 갱신이 된다(서로 멀어 병합되지 않는 작은 사각형).
    const dirty = new DirtyRegions(640, 480);
    dirty.add({ left: 700, top: 0, right: 710, bottom: 10 });
    assert.equal(dirty.entries.length, 0);
    for (let index = 0; index < DIRTY_CAPACITY + 1; index += 1) dirty.add({ left: (index % 10) * 60, top: Math.floor(index / 10) * 40, right: (index % 10) * 60 + 4, bottom: Math.floor(index / 10) * 40 + 4 });
    assert.ok(dirty.entries.length === DIRTY_CAPACITY && dirty.fullRedraw);
    dirty.clear();
    assert.ok(dirty.entries.length === 0 && !dirty.fullRedraw);
    // SHP 접근: 범위 안에서 SHP 가 있으면 진단 없이 읽고, 해제 표식이면 복귀 뒤에도 같은 프레임으로 읽는다.
    const events = [];
    const hooks = {
        typeNumber: (view) => view.token,
        frameError: (n, f, a) => events.push(`E:${n}:${f}:${a}`),
        assertion: (e, file, line) => events.push(`D:${e}:${file}:${line}`),
        shape: (token, frame) => { events.push(`S:${token}:${frame}`); return [1, 2]; }
    };
    const view = { token: 90, flags1: 0, frameCount: 2, loaded: true, header: 5 };
    assert.deepEqual(readRiftShapeFrame(view, 1, 0, hooks), [1, 2]);
    assert.deepEqual(events, ['S:90:1']);
    events.length = 0;
    view.header = -0x22222223;
    readRiftShapeFrame(view, 5, 1, hooks);
    assert.deepEqual(events, ['E:90:5:1', `D:${RIFT_SHAPE_ASSERTION.expression}:${RIFT_SHAPE_ASSERTION.file}:225`, 'S:90:5']);
    assert.ok(throws(() => readRiftShapeFrame(view, 0, 0, { ...hooks, shape: undefined })));
});
