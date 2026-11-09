// Burning 프레임(raw-burning-frame.js)을 실제 보호 캡처(ShapeToBuffer)·기존 연소점/Append·dirty 표 위에서 원본 x86 fixture 로 대조한다(cpppj BurningFrameTests 의 10.78 범위).
//  - burningframe-x86.tsv originals 213행: 초기 캡처/단계 0·1·2, visible/off, timer, 연소점 선택, 방출/이동 입자(NaN·무한 수명 포함), dirty 표(병합 포함)와 난수 상태 15칸.
//    각 행은 입력(피해 슬롯·입자 패턴·시각·바람·배율·카메라)만 설치하고 모듈이 계산한다. 기대 열은 비교에만 쓴다. 마지막 actual_calls 칸은 원본 실행기 경로 증거라 비교하지 않는다.
//  - 같은 행을 현재 바람 owner(sharedWind)로, 그리고 세 소비자(캡처·첫 좌표·입자 프레임)의 피해 대상 진단 복귀로 다시 실행해 기존 전체 출력과 같은지 본다.
//  - 실제 피해 대상 어댑터의 상세 진단 owner·HP 진단 복귀(cpppj ActualVictimAdapter 시험)도 옮겼다. CD(10.72) 213행은 범위 밖이다. 실제 게임·브라우저 실행이 아니다.
// (2026-11-09 WB04-E200: Burning 프레임·BurningFrameCapture 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put, bitsToFloat, floatToBits } from './raw-support.mjs';
import { realCaptureAssets, clientPool, hexOrDash } from './capture-assets-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { GameRandom } from '../www/core/random.js';
import { RawRenderRegistry } from '../www/core/raw-construction-lifetime.js';
import { RawBurningVisual, createBurningVisualFields } from '../www/core/raw-burning-visual.js';
import { RawBurningSources } from '../www/core/raw-burning-sources.js';
import { BurningVictimEntry, makeBurningVictimHooks, runBurningVictimParent } from '../www/core/raw-burning-victim.js';
import { isSidRobust } from '../www/core/raw-sid-robust.js';
import { RawBurningFrame, createBurningFrameFields, createBurningFrameState, makeBurningFrameCapture } from '../www/core/raw-burning-frame.js';
import { RawShapeToBuffer, createShapeToBufferState } from '../www/render/raw-shape-to-buffer.js';
import { DirtyRegions } from '../www/render/dirty-regions.js';

// 원본 x86 fixture 의 10.78 행이다. (2026-11-09 WB04-E200)
const frameRows = (readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'burningframe-x86.tsv')) ?? []).filter((row) => row[0] === 'originals');
const f = Math.fround;

// 예외가 나는지 본다. (2026-11-09 WB04-E200)
function throws(action) {
    try { action(); } catch { return true; }
    return false;
}

// fixture 의 부호 없는 32비트 십진 입력이다. (2026-11-09 WB04-E200)
const u32 = (text) => Number(BigInt.asUintN(32, BigInt(text)));

// 부호 없는 64비트 십진 비트열을 double 로 바꾼다. (2026-11-09 WB04-E200)
function doubleFromBits(text) {
    const view = new DataView(new ArrayBuffer(8));
    view.setBigUint64(0, BigInt(text), true);
    return view.getFloat64(0, true);
}

// little endian double/float/DWORD 를 바이트 배열에 쓴다. (2026-11-09 WB04-E200)
function putDouble(bytes, offset, value) {
    new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength).setFloat64(offset, value, true);
}

// 원본 24바이트 입자 여섯 DWORD(source 포인터는 0/1 정규화)를 순서대로 쓴다. (2026-11-09 WB04-E200)
function particleBytes(fields) {
    const bytes = new Uint8Array(fields.capacity * 24);
    // 호스트 padding 없이 여섯 DWORD 를 쓴다.
    fields.particles.forEach((particle, index) => {
        const base = index * 24;
        put(bytes, base, floatToBits(particle.x));
        put(bytes, base + 4, floatToBits(particle.y));
        put(bytes, base + 8, floatToBits(particle.velocityX));
        put(bytes, base + 12, floatToBits(particle.velocityY));
        put(bytes, base + 16, floatToBits(particle.life));
        put(bytes, base + 20, particle.source ? 1 : 0);
    });
    return hexOrDash(bytes);
}

// 원본 정규화 출력과 같은 전체 시각·메타데이터·dirty 표 15칸을 만든다. (2026-11-09 WB04-E200)
function output(fields, frame, random, regions, captured) {
    const rect = new Uint8Array(16);
    const timer = new Uint8Array(24);
    const metadata = new Uint8Array(16);
    const registration = fields.registration;
    [registration.left, registration.top, registration.right, registration.bottom].forEach((value, index) => put(rect, index * 4, floatToBits(value)));
    putDouble(timer, 0, fields.lastTime);
    putDouble(timer, 8, fields.elapsed);
    put(timer, 16, floatToBits(frame.delta));
    put(timer, 20, floatToBits(frame.scaledDelta));
    [fields.image.width, fields.image.height, fields.image.hotspotX, fields.image.hotspotY].forEach((value, index) => put(metadata, index * 4, value >>> 0));
    const dirty = regions.entries.map((entry) => `${entry.rect.left},${entry.rect.top},${entry.rect.right},${entry.rect.bottom},${entry.flags}`).join(';');
    return [
        hexOrDash(rect), String(registration.zOrder), String(fields.total), hexOrDash(timer), String(fields.stage), hexOrDash(metadata),
        fields.image.pixels.length ? hexOrDash(fields.image.pixels) : '-', fields.rows.length ? hexOrDash(Uint8Array.from(fields.rows)) : '-',
        String(fields.selected), fields.off ? '1' : '0', particleBytes(fields), String(random.state), regions.fullRedraw ? '1' : '0', dirty || '-',
        captured ? `${fields.image.pixels.length},${fields.rows.length}` : '-',
    ];
}

// 미관찰 목록 확보/반납/부착을 거부하는 표시 목록 경계이다. (2026-11-09 WB04-E200)
const rejectingRegistry = (pool) => new RawRenderRegistry(pool, {
    reserve: () => { throw new Error('프레임의 미관찰 표시 목록 확보'); },
    releaseArray: () => { throw new Error('프레임의 미관찰 표시 목록 반납'); },
    attach: () => { throw new Error('프레임의 미관찰 Host 부착'); },
});

// 현재 입력만 만들고 실제 Frame → Capture/Sources/Append/dirty 사슬을 수행한다(cpppj RunFrame). (2026-11-09 WB04-E200)
function runFrame(row, victim = { checking: () => 0 }, sharedWind = false) {
    const pool = clientPool();
    const raw = pool.allocated(50);
    raw.fill(0);
    put(raw, 0, 0x508cc8);
    raw[10] = 70;
    put(raw, 14, floatToBits(20.75));
    put(raw, 18, floatToBits(21.9));
    put(raw, 26, 10);
    raw[35] = Number(row[9]) & 0xff;
    put(raw, 36, u32(row[10]));
    const assets = realCaptureAssets();
    const capture = new RawShapeToBuffer(pool, assets.types, assets.shapes, assets.database, createShapeToBufferState({ zoom: u32(row[11]) | 0 }));
    const random = new GameRandom(u32(row[6]));
    const registry = rejectingRegistry(pool);
    const visual = new RawBurningVisual(pool, registry, { processType: 14, palette: 0 }, { reserve: () => { throw new Error('프레임의 미관찰 표시 목록 확보'); } });
    const sourceState = { weakenedMana: false, manaType: 158, daisType: 165, victim: { checking: () => 0 } };
    const sources = new RawBurningSources(pool, assets.types, visual, random, sourceState);
    const state = createBurningFrameState();
    state.now = doubleFromBits(row[7]);
    state.pixelScaleX = bitsToFloat(u32(row[12]));
    state.pixelScaleY = state.pixelScaleX;
    state.cameraX = 0.25;
    state.cameraY = 0.5;
    state.wind = bitsToFloat(u32(row[8]));
    state.deltaMultiplier = f(0.3);
    state.viewport = { left: 0, top: 0, right: 1024, bottom: 768 };
    const originalWind = state.wind;
    let liveWind = 7;
    // 생성 때의 잘못된 값/사본을 쓰면 원문 전체 출력과 달라진다.
    if (sharedWind) {
        state.wind = -13;
        state.sharedWind = () => liveWind;
    }
    const regions = new DirtyRegions(1024, 768);
    const sink = { suppressed: () => regions.fullRedraw, invalidate: (rect, flags) => regions.add(rect, flags) };
    let captured = false;
    const hooks = makeBurningFrameCapture(capture, pool);
    const actual = hooks.capture;
    hooks.capture = (sid) => { captured = true; return actual(sid); };
    const logic = new RawBurningFrame(pool, assets.types, visual, sources, random, state, sink, hooks);
    // 실제 처리기 생성 뒤 owner 를 바꿔 다음 Step 의 현재 읽기를 검증한다.
    if (sharedWind) liveWind = originalWind;
    const fields = createBurningVisualFields();
    fields.capacity = u32(row[1]);
    fields.particles = Array.from({ length: fields.capacity }, () => ({ x: 0, y: 0, velocityX: 0, velocityY: 0, life: 0, source: false }));
    const near = u32(row[4]) !== 0;
    Object.assign(fields.registration, { zOrder: 1234, left: near ? 12.25 : 1100.25, top: near ? 13.75 : 1100.75, right: near ? 30.5 : 1120.5, bottom: near ? 35.5 : 1125.5 });
    fields.lastTime = 1;
    fields.elapsed = 0.123456789;
    fields.off = u32(row[3]) !== 0;
    fields.victim = 50;
    const frame = createBurningFrameFields();
    frame.delta = 123;
    frame.scaledDelta = -456;
    const stage = u32(row[2]);
    if (fields.capacity && stage) logic.capture(fields);
    fields.stage = stage;
    fields.victim = u32(row[13]);
    const pattern = u32(row[5]);
    if (fields.capacity && stage) {
        fields.total = pattern === 1 ? Math.min(fields.capacity, 2) : pattern === 2 ? Math.min(fields.capacity, 3) : pattern === 3 ? Math.min(fields.capacity, 4) : 0;
        fields.selected = pattern ? 1 : 0;
        const exceptional = [0x7fc00001, 0xffc00001, 0x7f800000, 0x80000000];
        // 원본과 같은 source 좌표/수명 비트·이동 입자 패턴만 공급한다.
        for (let index = 0; index < fields.particles.length; index += 1) {
            const source = pattern === 1 && index < 2;
            const x = source ? (index ? 7 : 2) : f(25.125 + index);
            const y = source ? 3 : f(30.375 + index);
            const life = source ? 9999 : pattern === 2 ? (index === 0 ? 5.25 : index === 1 ? f(0.1) : 0) : pattern === 3 && index < 4 ? bitsToFloat(exceptional[index]) : 0;
            fields.particles[index] = { x, y, velocityX: f(0.1), velocityY: -1.25, life, source };
            if (source) {
                const group = Math.trunc((Math.trunc(x) + Math.trunc(y)) / 5);
                if (group < fields.rows.length) fields.rows[group] = 1;
            }
        }
    }
    // 검사 준비 캡처를 마친 뒤 실제 선택 프레임에만 현재 진단 장치를 공급한다.
    state.victim = victim;
    sourceState.victim = victim;
    logic.step(fields, frame);
    return output(fields, frame, random, regions, captured);
}

// 현재 입력을 실행한 뒤에만 실제 독립 출력과 비교한다(첫 차이 칸을 메시지에 남긴다). (2026-11-09 WB04-E200)
function replay(sharedWind) {
    let count = 0;
    // 마지막 actual_calls 는 비교하지 않는다.
    for (const row of frameRows) {
        assert.equal(row.length, 30);
        const actual = runFrame(row, { checking: () => 0 }, sharedWind);
        const expected = row.slice(14, 29);
        const index = actual.findIndex((value, at) => value !== expected[at]);
        assert.equal(index, -1, `행 ${count} 칸 ${index}: 기대 ${expected[index]?.slice(0, 120)} / 실제 ${actual[index]?.slice(0, 120)}`);
        count += 1;
    }
    assert.equal(count, 213);
}

test('BurningFrame: 원본 10.78 213행의 초기 캡처/단계·visible·timer·연소점·입자와 전체 dirty 표가 같다', { skip: frameRows.length === 0 && 'burningframe-x86.tsv 없음' }, () => {
    replay(false);
});

test('BurningFrame: 현재 바람 owner 를 생성 뒤 각 입자 가속에서 다시 읽는다(cpppj AmbientWind 시험의 10.78)', { skip: frameRows.length === 0 && 'burningframe-x86.tsv 없음' }, () => {
    replay(true);
});

test('BurningFrame: 캡처·첫 좌표·입자 프레임의 피해 대상 진단이 복귀하면 기존 전체 출력과 같다', { skip: frameRows.length === 0 && 'burningframe-x86.tsv 없음' }, () => {
    // 초기 캡처·첫 좌표·입자 프레임의 서로 다른 세 소비자를 실행한다.
    for (const stage of [0, 1, 2]) {
        const found = frameRows.find((row) => u32(row[1]) !== 0 && u32(row[2]) === stage && (stage !== 2 || (row[4] === '1' && row[5] === '0')));
        assert.ok(found);
        const baseline = runFrame(found);
        let calls = 0;
        const lines = [];
        const victim = {
            checking: () => 0x80000000,
            // 각 부모의 첫 조회는 진단을 선택하고 두 번째 조회는 기존 본문을 허용한다.
            robust: (sid, live) => { assert.ok(sid === 50 && live); calls += 1; return calls % 2 === 0; },
            assertion: (condition, source, line) => { assert.equal(condition, 'isSidRobust(burningSid)'); assert.equal(source, '.\\Burning.cpp'); lines.push(line); },
        };
        assert.deepEqual(runFrame(found, victim), baseline);
        const expected = stage === 0 ? [0x6f] : stage === 1 ? [0x11f] : [0x11f, 0x91];
        assert.deepEqual(lines, expected);
        assert.equal(calls, expected.length * 2);
    }
});

test('BurningFrame: 실제 피해 대상 어댑터가 상세 진단 owner 와 무조건 HP 진단 복귀를 보존한다', () => {
    const pool = new SidPool(32768);
    const sid = pool.allocate(0);
    const raw = pool.allocated(sid);
    raw.fill(0);
    raw[0] = 1;
    raw[10] = 70;
    const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, maxHitPoints: 0 }));
    types[70].flags1 = 0x10;
    let checking = 0;
    const owner = { lastRobust: 0x1111, type: 0x2222 };
    let detailed = 0;
    let calls = 0;
    // 호스트 주소 방식: 기준 0 에서 SID*50 + 칸으로 현재 물리 슬롯을 읽는다.
    const access = {
        addressMode: 'host',
        poolBase: () => 0,
        capacity: () => pool.capacity,
        checking: () => checking,
        memory: (address, width) => {
            const slot = Math.floor(address / 50);
            const offset = address % 50;
            if (slot < 0 || slot >= pool.capacity || offset + width > 50) throw new RangeError('연소 검사 현재 풀 범위');
            return pool.physical(slot).subarray(offset, offset + width);
        },
        assertion: (condition, line) => {
            assert.equal(condition, 'resolve( sid ).isRobust()');
            assert.equal(line, 0x218);
            assert.deepEqual([owner.lastRobust, owner.type], [2, 70]);
            detailed += 1;
            raw[0] = 1;
        },
    };
    const state = { checking: () => checking, robust: (current, live) => { calls += 1; return isSidRobust(current, live ? 1 : 0, owner, access); } };
    const hooks = makeBurningVictimHooks(pool, types, state);
    assert.equal(runBurningVictimParent(() => sid, BurningVictimEntry.Capture, state.checking, hooks), sid);
    assert.deepEqual([calls, owner.lastRobust, owner.type], [1, 0x1111, 0x2222]);
    // 내부 SID 진단이 돌아오면 외부 조회도 1 로 계속하고 다음 조회가 같은 owner 를 다시 갱신한다.
    checking = 0x80000000;
    raw[0] = 0;
    assert.equal(runBurningVictimParent(() => sid, BurningVictimEntry.Capture, state.checking, hooks), sid);
    assert.deepEqual([calls, detailed, owner.lastRobust, owner.type], [3, 1, 0, 70]);
    // HP 진단은 checking 0 에서도 선택하며 callback 뒤 정상 본문 진입을 허용한다.
    checking = 0;
    types[70].flags2 = 0x400000;
    types[70].flags1 = 0;
    raw[40] = 0;
    let hpDiagnostics = 0;
    state.assertion = (condition, source, line) => {
        assert.deepEqual([condition, source, line], ['s.hasHitPoints()', '.\\Burning.cpp', 0x7f]);
        hpDiagnostics += 1;
        raw[40] = 1;
    };
    assert.equal(runBurningVictimParent(() => sid, BurningVictimEntry.Capture, state.checking, hooks), sid);
    assert.deepEqual([hpDiagnostics, raw[40], checking], [1, 1, 0]);
});

test('BurningFrame: 풀 불일치·캡처 미연결·nonzero 소리 정지 미연결을 거부하고 capacity 0 은 무동작이다(웹 계약 검사)', () => {
    const assets = realCaptureAssets();
    const pool = clientPool();
    const other = clientPool();
    const capture = new RawShapeToBuffer(pool, assets.types, assets.shapes, assets.database, createShapeToBufferState());
    const visual = new RawBurningVisual(pool, rejectingRegistry(pool), { processType: 14, palette: 0 }, { reserve: () => false });
    const random = new GameRandom(5);
    const sources = new RawBurningSources(pool, assets.types, visual, random, { weakenedMana: false, manaType: 158, daisType: 165, victim: { checking: () => 0 } });
    const state = createBurningFrameState();
    state.viewport = { left: 0, top: 0, right: 640, bottom: 480 };
    const sink = { invalidate: () => { throw new Error('미관찰 dirty'); } };
    // 다른 풀의 캡처 어댑터와 캡처 없는 hooks 는 거부한다.
    assert.ok(throws(() => makeBurningFrameCapture(capture, other)));
    assert.ok(throws(() => new RawBurningFrame(pool, assets.types, visual, sources, random, state, sink, {})));
    const otherVisual = new RawBurningVisual(other, rejectingRegistry(other), { processType: 14, palette: 0 }, { reserve: () => false });
    assert.ok(throws(() => new RawBurningFrame(pool, assets.types, otherVisual, sources, random, state, sink, makeBurningFrameCapture(capture, pool))));
    // 호출자가 준 소리 정지 소비자는 유지한다.
    const stops = [];
    const hooks = makeBurningFrameCapture(capture, pool, { stopSound: (handle) => stops.push(handle) });
    assert.equal(typeof hooks.stopSound, 'function');
    const logic = new RawBurningFrame(pool, assets.types, visual, sources, random, state, sink, hooks);
    const fields = createBurningVisualFields();
    const frame = createBurningFrameFields();
    logic.step(fields, frame);
    assert.deepEqual([fields.stage, random.state, frame.delta], [0, 5, 0]);
    // 화면 밖에서 처음 꺼질 때 nonzero 소리만 정지한다.
    const raw = pool.allocated(50);
    raw.fill(0);
    put(raw, 0, 0x508cc8);
    raw[10] = 70;
    put(raw, 14, floatToBits(5000));
    put(raw, 18, floatToBits(5000));
    put(raw, 26, 10);
    fields.capacity = 4;
    fields.particles = Array.from({ length: 4 }, () => ({ x: 0, y: 0, velocityX: 0, velocityY: 0, life: 0, source: false }));
    fields.victim = 50;
    Object.assign(fields.registration, { left: 5000, top: 5000, right: 5010, bottom: 5010 });
    fields.stage = 2;
    fields.off = false;
    fields.sound = 77;
    logic.step(fields, frame);
    assert.deepEqual([stops, fields.off], [[77], true]);
    // 다시 화면 밖이면 이미 꺼져 있어 정지하지 않는다.
    logic.step(fields, frame);
    assert.deepEqual(stops, [77]);
    // 소리 정지 소비자가 없으면 도달 지점에서 거부한다.
    fields.off = false;
    const bare = new RawBurningFrame(pool, assets.types, visual, sources, random, state, sink, makeBurningFrameCapture(capture, pool));
    assert.ok(throws(() => bare.step(fields, frame)));
    // 입자 배열이 capacity 와 다르면 거부한다.
    fields.particles.pop();
    assert.ok(throws(() => logic.step(fields, frame)));
});
