// 카메라 Present 말미 render/raw-camera-present.js 를 원본 10.78 관측(camera-present-x86.tsv 178행)과 cpppj `CameraPresentTests.cpp` 의 진단/선택 장치/지연 적용 시험으로 검사한다.
//  - 관측 한 행 = 입력 17열(진단·폭·높이·세계 칸·한계 X/Y·목표 X/Y·flip 뒤 변형 여부와 flip 뒤 진단/폭/높이/칸/한계/목표)과 출력 8열(진단·폭·높이·칸·한계·목표), 호출열(F·C + 그 시점 목표 픽셀)이다.
//    flip 뒤 현재 값이 바뀌는 행에서는 flip 훅이 같은 현재 전역과 목표를 갱신해 제한이 그 뒤 값으로 계산되는지 본다. 현재 view/snapshot/zoom 은 이 본체가 바꾸지 않는다.
//  - cpppj 계약 시험: 진단 1 은 장치 연결 없이도 반환(CameraPresent_DiagnosticAndSelectedDeviceGuards)·선택된 flip/commit 누락은 각 위치에서 거부·실제 Client 진단값의 변경을 읽고 commit 전에는 현재 view/snapshot 을 바꾸지 않음.
//    `CameraPresent_SharedDiagnosticAndDeferredCurrentView` 의 실제 `ClientGameplayGlobals.PatchDiagnostic()` 공유는 그 전역 소유자를 옮기는 단계에서 같은 시험을 다시 건다 — 여기서는 `{ value }` 상자와 `ClientCameraGlobals` 로 같은 순서를 본다.
//  - 웹 계약: 세계 한계의 32비트 곱/뺄셈 감김과 −2^31·2^31−1 칸·한 축만 제한되는 경계.
// 실제 게임·브라우저 실행이 아니다. (2026-11-13 WB04-E383)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { presentCameraTarget } from '../www/render/raw-camera-present.js';
import { ClientCameraGlobals } from '../www/core/client-camera-globals.js';

// 원본 관측 행이다. 없으면 해당 검사를 건너뛴다. (2026-11-13 WB04-E383)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'camera-present-x86.tsv'));
const box = (value) => ({ value });

test('카메라 Present: 원본 10.78 관측 178행의 한계·목표·전역과 flip/commit 호출 시점 목표가 모두 일치한다', { skip: rows === null }, () => {
    assert.equal(rows.length, 178);
    for (const row of rows) {
        assert.equal(row.length, 26);
        const input = row.slice(0, 17).map(Number);
        const expected = row.slice(17, 25).map(Number);
        const integer = (index) => input[index] | 0;
        const camera = new ClientCameraGlobals();
        const target = camera.target();
        const diagnostic = box(input[0] >>> 0);
        const width = box(integer(1)); const height = box(integer(2)); const cells = box(integer(3));
        target.limits.set([integer(4), integer(5)]);
        target.pixels.set([integer(6), integer(7)]);
        target.zoom = 0xabcdef01;
        camera.rendererView().setPixels(123, 456);
        camera.snapshot().setPixels(789, 987);
        const events = [];
        const record = (code) => events.push(`${code}:${target.pixels[0] >>> 0}:${target.pixels[1] >>> 0}`);
        presentCameraTarget(target, { diagnostic, width, height, worldCells: cells }, {
            flip: () => {
                record('F');
                if (input[8]) {
                    diagnostic.value = input[9] >>> 0; width.value = integer(10); height.value = integer(11); cells.value = integer(12);
                    target.limits.set([integer(13), integer(14)]);
                    target.pixels.set([integer(15), integer(16)]);
                }
            },
            commit: () => record('C'),
        });
        const actual = [diagnostic.value >>> 0, width.value >>> 0, height.value >>> 0, cells.value >>> 0, target.limits[0] >>> 0, target.limits[1] >>> 0, target.pixels[0] >>> 0, target.pixels[1] >>> 0];
        const label = row.slice(0, 17).join(',');
        assert.deepEqual(actual, expected, `관측 ${label}`);
        assert.equal(events.length ? events.join(';') : '-', row[25], `호출 ${label}`);
        assert.equal(target.zoom, 0xabcdef01);
        assert.deepEqual([camera.rendererView().pixels, camera.snapshot().pixels], [{ x: 123, y: 456 }, { x: 789, y: 987 }]);
    }
});

test('카메라 Present: 진단 1 은 장치 연결 없이 반환하고 선택된 flip/commit 누락은 각 호출 위치에서 거부한다(CameraPresent_DiagnosticAndSelectedDeviceGuards)', () => {
    const target = { limits: Int32Array.of(3072, 2048), pixels: Int32Array.of(4000, -1), zoom: 0 };
    const globals = { diagnostic: box(1), width: box(1024), height: box(768), worldCells: box(256) };
    presentCameraTarget(target, globals, {});
    assert.deepEqual(Array.from(target.pixels), [4000, -1]);
    globals.diagnostic.value = 6;
    assert.throws(() => presentCameraTarget(target, globals, {}), /현재 장치 미연결/);
    assert.deepEqual(Array.from(target.pixels), [4000, -1]);
    assert.throws(() => presentCameraTarget(target, globals, { flip: () => {} }), /현재 장치 미연결/);
    // commit 누락은 두 제한이 모두 적용된 뒤에 거부된다.
    assert.deepEqual(Array.from(target.pixels), [3072, 0]);
});

test('카메라 Present: 실제 카메라 전역의 현재 view/snapshot 은 commit 전에는 바뀌지 않고 같은 진단 상자의 변경을 읽는다(CameraPresent_SharedDiagnosticAndDeferredCurrentView)', () => {
    const camera = new ClientCameraGlobals();
    const target = camera.target();
    target.limits.set([3072, 2048]);
    target.pixels.set([4000, 4000]);
    target.zoom = 65536;
    camera.rendererView().initialize();
    camera.snapshot().initialize();
    const globals = { diagnostic: box(1), width: box(1024), height: box(768), worldCells: box(256) };
    let order = '';
    const hooks = {
        flip: () => { order += 'F'; assert.equal(camera.rendererView().pixels.x, 0); assert.equal(target.pixels[0], 4000); },
        commit: () => {
            order += 'C';
            assert.deepEqual([target.pixels[0], target.pixels[1], camera.snapshot().pixels.x], [3072, 2048, 0]);
            camera.rendererView().setPixels(target.pixels[0], target.pixels[1]);
            camera.snapshot().setPixels(target.pixels[0], target.pixels[1]);
        },
    };
    presentCameraTarget(target, globals, hooks);
    assert.deepEqual([order, camera.rendererView().pixels.x], ['', 0]);
    globals.diagnostic.value = 6;
    presentCameraTarget(target, globals, hooks);
    assert.equal(order, 'FC');
    const expectedPixels = { x: target.pixels[0], y: target.pixels[1] };
    assert.deepEqual([camera.rendererView().pixels, camera.snapshot().pixels], [expectedPixels, expectedPixels]);
});

test('카메라 Present 계약: 세계 한계는 32비트 곱/뺄셈을 감겨 signed 로 해석하고 두 축을 따로 제한한다', () => {
    const globals = (cells, width = 1024, height = 768) => ({ diagnostic: box(6), width: box(width), height: box(height), worldCells: box(cells) });
    const run = (cells, pixels, limits, size) => {
        const target = { limits: Int32Array.from(limits), pixels: Int32Array.from(pixels), zoom: 0 };
        presentCameraTarget(target, globals(cells, size?.[0], size?.[1]), { flip: () => {}, commit: () => {} });
        return Array.from(target.pixels);
    };
    // 칸 256: X 한계 256×16−1024=3072, Y 한계 256×11−768=2048.
    assert.deepEqual(run(256, [5000, 5000], [9999, 9999]), [3072, 2048]);
    assert.deepEqual(run(256, [-5, 100], [9999, 9999]), [0, 100]);
    // 현재 한계가 더 작으면 그 한계가 이긴다.
    assert.deepEqual(run(256, [5000, 5000], [100, 200]), [100, 200]);
    // 한 축만 세계 한계에 걸리는 경계(X 3072 정확히, Y 2049 → 2048).
    assert.deepEqual(run(256, [3072, 2049], [9999, 9999]), [3072, 2048]);
    assert.deepEqual(run(256, [3073, 2048], [9999, 9999]), [3072, 2048]);
    // 세계 한계가 음수가 되면(작은 세계) 0 으로 제한된다. 2^31−1 칸은 곱이 감겨 음수 한계가 된다.
    assert.deepEqual(run(64, [100, 200], [9999, 9999]), [0, 0]);
    assert.deepEqual(run(0x7fffffff, [100, 200], [9999, 9999]), [0, 200]);
});
