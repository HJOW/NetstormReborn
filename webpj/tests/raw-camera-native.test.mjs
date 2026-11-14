// 카메라 입력 세 부품(497f20 중심 요청·4d6b54 카메라 이동·004d65ac 가장자리 스크롤)과 497490 월드 setter 진단 재조회를 독립 원본 관측 fixture 에 대조한다.
// 앞선 E333/E437/E442 이식은 기준선 소스 계약과 손 계산만 썼고(cpppj 직접 시험이 있는 줄 몰랐다), 이 시험이 원본 140/259/135행을 처음으로 직접 연결한다. 원본게임을 실행하지 않는다. (2026-11-15 WB04-E465)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { ClientCameraGlobals } from '../www/core/client-camera-globals.js';
import { CameraView, projectCameraCoordinates } from '../www/render/camera-view.js';
import { put } from './raw-support.mjs';
import { SidPool } from '../www/core/sid-pool.js';
import { CursorDropState } from '../www/core/raw-cursor-drop.js';
import { CursorLifecycleState } from '../www/core/raw-cursor-lifecycle.js';
import { CursorPlacementState, RawCursorPlacement } from '../www/core/raw-cursor-placement.js';
import { InputCode } from '../www/core/input-queue.js';
import { processCameraPan } from '../www/core/raw-camera-pan.js';
import { processEdgeScroll, requestCameraDelta } from '../www/core/raw-edge-scroll.js';

// 탭 구분 fixture 의 주석 줄(머리글 포함)을 건너뛰고 열 목록으로 읽는다.
function rows(name) { return fs.readFileSync(new URL(`../../cpppj/tests/fixtures/${name}`, import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line && !line.startsWith('#')).map((line) => line.split('\t')); }
// hex 문자열을 바이트로, 바이트를 hex 로 바꾼다.
const bytesOf = (hex) => Uint8Array.from(Buffer.from(hex, 'hex')); const hexOf = (bytes) => Buffer.from(bytes).toString('hex');
// 원문 비트를 단정도로 읽는다.
const floatOf = (bits) => { const view = new DataView(new ArrayBuffer(4)); view.setUint32(0, bits >>> 0, true); return view.getFloat32(0, true); };
const bitsOf = (value) => { const view = new DataView(new ArrayBuffer(4)); view.setFloat32(0, value, true); return view.getUint32(0, true); };
// view 의 현재 픽셀 원점을 쓴다.
const setPixels = (view, x, y) => { view.data.setInt32(28, x, true); view.data.setInt32(32, y, true); };

test('CameraCenter 원본 140행: 497f20 은 목표 X/Y 와 변환 번호만 쓰고 현재 view/snapshot/분모는 보존한다', () => {
    const table = rows('camera-center-x86.tsv'); assert.equal(table.length, 140);
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 10, `행 ${index}`); const camera = new ClientCameraGlobals();
        // Renderer 160BYTE 중 현재 view 64BYTE 와 +48..5b 의 한계/목표/배율 20BYTE 를 같은 자료형으로 복원한다.
        camera.rendererView().bytes.set(bytesOf(row[5].slice(0, 128))); camera.snapshot().bytes.set(bytesOf(row[6]));
        const target = new DataView(bytesOf(row[5].slice(0x48 * 2, 0x48 * 2 + 40)).buffer); camera.target().limits[0] = target.getInt32(0, true); camera.target().limits[1] = target.getInt32(4, true); camera.target().pixels[0] = target.getInt32(8, true); camera.target().pixels[1] = target.getInt32(12, true); camera.target().zoom = target.getUint32(16, true);
        camera.denominator().current = 23; camera.denominator().defaultValue = 17; const revision = { value: Number(row[4]) };
        camera.requestCenter(floatOf(parseInt(row[2], 16)), floatOf(parseInt(row[3], 16)), Number(row[0]), Number(row[1]), revision);
        const out = new Uint8Array(20); const view = new DataView(out.buffer); view.setInt32(0, camera.target().limits[0], true); view.setInt32(4, camera.target().limits[1], true); view.setInt32(8, camera.target().pixels[0], true); view.setInt32(12, camera.target().pixels[1], true); view.setUint32(16, camera.target().zoom, true);
        assert.equal(hexOf(camera.rendererView().bytes), row[7].slice(0, 128), `행 ${index} renderer`); assert.equal(hexOf(camera.snapshot().bytes), row[8], `행 ${index} snapshot`); assert.equal(hexOf(out), row[7].slice(0x48 * 2, 0x48 * 2 + 40), `행 ${index} 목표`);
        assert.deepEqual([revision.value, camera.denominator().current, camera.denominator().defaultValue], [Number(row[9]), 23, 17], `행 ${index}`);
    }
});

// 카메라 이동 관측 입력을 읽어 같은 순서의 호출/목표/현재 view 를 재생한다. 반환은 [선택 여부, 현재 X, 현재 Y, 목표 X, 목표 Y, 변환 번호] 와 호출 문자열이다.
function replayPan(input) {
    const value = (index) => input[index] | 0; const globals = { scrollWhileMouseOut: value(5), dialog: value(6), active: value(7), clientWidth: value(8), clientHeight: value(9), viewX: value(10), viewY: value(11), viewWidth: value(12), viewHeight: value(13) };
    const camera = new ClientCameraGlobals(); setPixels(camera.rendererView(), value(14), value(15)); camera.target().pixels[0] = value(16); camera.target().pixels[1] = value(17); setPixels(camera.snapshot(), 777, 888); const revision = { value: input[18] >>> 0 }; const events = [];
    const hooks = {
        globals: () => globals,
        // 폴링 callback 이 변경한 현재 view 는 뒤 497e00 에서 다시 조회한다.
        poll: (code) => { events.push(`P:${code}`); if (code === 0x100000 && input[19] === 2) { setPixels(camera.rendererView(), 900, 901); revision.value = 0xffffffff; } return { code: (code | ((code === InputCode.middleButton ? input[3] : input[4]) ? InputCode.release : 0)) >>> 0, x: value(1), y: value(2) }; },
        closePopups: () => { events.push('C'); if (input[19] === 1) Object.assign(globals, { viewX: 13, viewY: 17, viewWidth: 440, viewHeight: 330, clientWidth: 880, clientHeight: 660 }); },
        request: (x, y) => { events.push(`D:${x}:${y}`); requestCameraDelta(camera.rendererView(), camera.target(), 768, x, y, revision); },
    };
    const selected = processCameraPan({ code: input[0] >>> 0, x: value(1), y: value(2) }, hooks);
    return { actual: [selected ? 1 : 0, camera.rendererView().pixels.x >>> 0, camera.rendererView().pixels.y >>> 0, camera.target().pixels[0] >>> 0, camera.target().pixels[1] >>> 0, revision.value >>> 0], events: events.join(';'), snapshot: camera.snapshot().pixels };
}

test('CameraPan 원본 259행: 실제 관문/중심 거리/현재 목표와 callback 뒤 재조회가 모두 일치한다', () => {
    const table = rows('camera-pan-x86.tsv'); assert.equal(table.length, 259);
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 27, `행 ${index}`); const result = replayPan(row.slice(0, 20).map(Number));
        assert.deepEqual(result.actual, row.slice(20, 26).map(Number), `행 ${index}`); assert.equal(result.events, row[26], `행 ${index}`); assert.deepEqual(result.snapshot, { x: 777, y: 888 }, `행 ${index}`);
    }
});

test('CameraPan 선택 장치: 비활성/뗌 입력의 이동 전용 큐 선택과 미연결 팝업 닫기 거부', () => {
    const current = { scrollWhileMouseOut: 1, dialog: 0, active: 0, clientWidth: 1024, clientHeight: 768, viewX: 128, viewY: 26, viewWidth: 896, viewHeight: 680 }; let events = '';
    const hooks = { poll: (code) => { events += 'P'; return { code: (code | InputCode.release) >>> 0, x: 0, y: 0 }; }, globals: () => current };
    assert.equal(processCameraPan({ code: 0x80120000, x: 600, y: 400 }, hooks), false); assert.equal(events, 'P');
    assert.throws(() => processCameraPan({ code: 0x120000, x: 600, y: 400 }, hooks), { name: 'Error', message: /카메라 pan 현재 장치 미연결/ });
    hooks.closePopups = () => { events += 'C'; }; assert.equal(processCameraPan({ code: 0x120000, x: 600, y: 400 }, hooks), true); assert.equal(events, 'PPPC'); // 비활성은 팝업 정리 뒤 전용 큐를 선택하되 Shift/목표를 요구하지 않는다.
});

test('EdgeScroll 원본 135행: 입력 21열과 방향/시간/목표/전체 전역 및 호출 순서가 모두 일치한다', () => {
    const table = rows('edge-scroll-x86.tsv'); assert.equal(table.length, 135);
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 27, `행 ${index}`); const n = row.map((cell, column) => (column === 1 || column === 14 || column === 22 || column === 23 || column === 24 || column >= 26 ? cell : Number(cell)));
        const sinceBits = BigInt(row[1]); const view = new DataView(new ArrayBuffer(8)); view.setBigUint64(0, sinceBits, true); const state = { initializationFlags: n[0] >>> 0, since: view.getFloat64(0, true) };
        view.setBigUint64(0, BigInt(row[14]), true); const wall = view.getFloat64(0, true);
        const camera = new ClientCameraGlobals(); setPixels(camera.rendererView(), n[16] | 0, n[17] | 0); camera.target().pixels[0] = n[18] | 0; camera.target().pixels[1] = n[19] | 0; const revision = { value: n[20] >>> 0 }; const events = [];
        const hooks = {
            poll: (code) => { events.push(`P:${code}`); return { code: (code | ((code === InputCode.leftButton ? n[10] : n[11]) ? InputCode.release : 0)) >>> 0, x: n[6] | 0, y: n[7] | 0 }; },
            menu: (serial) => { events.push(`M:${serial}`); return n[12] >>> 0; }, blockingPopup: () => { events.push('B'); return n[13] !== 0; }, closePopups: () => { events.push('C'); },
            request: (dx, dy) => { events.push(`D:${dx >>> 0}:${dy >>> 0}`); requestCameraDelta(camera.rendererView(), camera.target(), n[9] | 0, dx, dy, revision); },
        };
        const result = processEdgeScroll(state, { enabled: n[2] >>> 0, dialog: n[5] >>> 0, menuSerial: 731, borderX: n[3] | 0, borderY: n[4] | 0, width: n[8] | 0, height: n[9] | 0, x: n[6] | 0, y: n[7] | 0, speed: n[15] | 0, wall }, hooks);
        view.setFloat64(0, state.since, true);
        assert.deepEqual([state.initializationFlags >>> 0, view.getBigUint64(0, true).toString(), revision.value], [n[21], row[22], n[25]], `행 ${index}`);
        assert.equal(`${bitsOf(result[1])}:${bitsOf(result[0])}`, row[23], `행 ${index} 방향`); assert.equal(`${camera.target().pixels[0] >>> 0}:${camera.target().pixels[1] >>> 0}`, row[24], `행 ${index} 목표`);
        assert.equal(events.length ? events.join(';') : '-', row[26], `행 ${index} 호출`);
    }
});

test('EdgeScroll 목표 요청은 현재 view 를 보존하고 이전 목표 대신 같은 현재 원점에 델타를 더하며 변환 번호는 감긴다', () => {
    const camera = new ClientCameraGlobals(); setPixels(camera.rendererView(), 100, 200); camera.target().pixels[0] = 1000; camera.target().pixels[1] = 2000; const revision = { value: 0xffffffff };
    requestCameraDelta(camera.rendererView(), camera.target(), 768, 2, -3, revision);
    assert.deepEqual([camera.rendererView().pixels.x, camera.rendererView().pixels.y, ...camera.target().pixels, revision.value], [100, 200, 102, 197, 0]);
});

test('EdgeScroll 연속 호버는 같은 flags/최초 시각을 공유하고 중앙 복귀 때만 가속이 해제된다', () => {
    const state = { initializationFlags: 0x20, since: 0 }; const input = { enabled: 1, dialog: 0, menuSerial: 731, borderX: 0, borderY: 0, width: 1024, height: 768, x: 1023, y: 384, speed: 30, wall: 10.0 }; let delta = 0;
    const hooks = { poll: (code) => ({ code: (code | InputCode.release) >>> 0, x: 0, y: 0 }), menu: () => 0, blockingPopup: () => false, closePopups: () => {}, request: (x) => { delta = x; } };
    processEdgeScroll(state, input, hooks); assert.deepEqual([delta, state.since, state.initializationFlags], [2, 10.0, 0x24]);
    input.wall = 10.25; processEdgeScroll(state, input, hooks); assert.deepEqual([delta, state.since], [9, 10.0]);
    input.x = 512; processEdgeScroll(state, input, hooks); assert.equal(state.since, 0.0);
    input.x = 1023; input.wall = 11.0; processEdgeScroll(state, input, hooks); assert.deepEqual([delta, state.since, state.initializationFlags], [2, 11.0, 0x24]);
});

test('CameraView_WorldSetterAssertionRequeriesCurrentFieldsAfterAllStores: X 진단 하위가 Y 를 바꾸면 두 번째 진단은 현재 원문 필드를 다시 읽는다', () => {
    const view = new CameraView(); view.initialize(); let calls = 0;
    view.setWorld(floatOf(0x4d000001), floatOf(0x4d800001), (message, line) => {
        assert.equal(message, 'scrollPosPixX >= 0'); assert.equal(line, 0x344); assert.equal(calls, 0); calls += 1;
        // 선행 여섯 store 는 이미 끝나 있다: 월드 float 두 개, 픽셀 두 개, 픽셀 float 두 개.
        assert.deepEqual([view.data.getUint32(0, true), view.data.getUint32(4, true)], [0x4d000001, 0x4d800001]); assert.ok(view.pixels.x < 0 && view.pixels.y < 0);
        assert.equal(view.data.getUint32(56, true), bitsOf(view.pixels.x)); assert.equal(view.data.getUint32(60, true), bitsOf(view.pixels.y)); view.data.setUint32(4, 0xbf800000, true);
    });
    assert.equal(calls, 1); assert.equal(view.data.getUint32(4, true), 0xbf800000);
});

test('CameraView_PlacementCostRequeriesSharedPixelsAfterDirtyEffect: 먼저 보낸 dirty 효과가 원점을 바꾸면 비용 사각형은 같은 snapshot 의 변경 후 현재값을 읽는다', () => {
    const camera = new ClientCameraGlobals(); camera.initializeNativeRenderer(1024, 768, 256, { value: 0 }, { value: 0 }); const snapshot = camera.snapshot(); snapshot.setWorld(5, 5);
    const pool = new SidPool(24000, { server: false }); for (let sid = 5; sid <= 100; sid += 1) assert.equal(pool.allocate(2), sid);
    const types = Array.from({ length: 188 }, () => ({ flags1: 0, flags2: 0, footX: 0, footY: 0 })); types[82].footX = 1;
    const drop = new CursorDropState(); drop.x = 10; drop.y = 10; drop.costFlags = 0x40; drop.cursorWidth = 6; const life = new CursorLifecycleState(); life.abstractType = 82; life.previews[0] = 50; life.costHeight = 8;
    put(pool.allocated(50), 22, 200, 2); put(pool.allocated(50), 24, 150, 2);
    // 같은 전역 snapshot 의 현재 픽셀 두 칸을 직접 읽는 공유 원점이다(복사본이 아니다).
    const state = new CursorPlacementState(); state.descriptionAllowed = 1; state.spaceAllowed = 1; state.sharedScreenOrigin = { get 0() { return snapshot.pixels.x; }, get 1() { return snapshot.pixels.y; } };
    const requests = []; const flags = [];
    const hooks = {
        project: (x, y) => { const point = projectCameraCoordinates(snapshot.pixels.x, snapshot.pixels.y, x, y); return { x: point[0], y: point[1] }; },
        invalidate: (rect, flag) => { requests.push({ left: rect.left, top: rect.top, right: rect.right, bottom: rect.bottom }); flags.push(flag); if (requests.length === 1) snapshot.setWorld(6, 6); },
    };
    new RawCursorPlacement(pool, types, new Uint16Array(65536), drop, life, state, hooks).prepare();
    assert.deepEqual(requests, [{ left: 78, top: 53, right: 86, bottom: 69 }, { left: 93, top: 84, right: 101, bottom: 94 }]); assert.deepEqual(flags, [2, 0]); assert.deepEqual([state.screenOriginX(), state.screenOriginY()], [96, 66]);
});
