// 원본 x86 Renderer 수집의 분수 카메라·부분 화면과 행 한도를 비트 단위로 대조한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { CameraView } from '../www/render/camera-view.js';
import { rendererCollectionRowLimit, rendererCollectionViewport, rendererWorldRect } from '../www/render/renderer-collection-geometry.js';

// 단정도 결과를 원본 fixture의 DWORD 비트로 읽는다.
function bits(value) { const view = new DataView(new ArrayBuffer(4)); view.setFloat32(0, value, true); return view.getUint32(0, true); }

// 10.78 원본이 실제 지형 수집기에 넘긴 90개 범위와 행 한도다.
test('Renderer 수집 범위: 원본 x86 90행 비트 일치', () => {
    const rows = fs.readFileSync(path.join(REPO_ROOT, 'cpppj/tests/fixtures/renderer-projection-x86.tsv'), 'utf8').split(/\r?\n/).filter((line) => line && !line.startsWith('#'));
    assert.equal(rows.length, 90);
    // 각 관측은 독립 카메라 원점과 화면 사각형을 사용한다.
    for (const row of rows) {
        const v = row.trim().split(/\s+/).map(Number), integer = (index) => v[index] | 0;
        assert.equal(v.length, 17); const camera = new CameraView(); camera.data.setUint32(0, v[10], true); camera.data.setUint32(4, v[11], true);
        const map = { left: integer(2), top: integer(3), right: integer(4), bottom: integer(5) };
        const dirty = { left: integer(6), top: integer(7), right: integer(8), bottom: integer(9) };
        const result = rendererCollectionViewport(dirty, map, camera);
        assert.deepEqual([bits(result.left), bits(result.top), bits(result.right), bits(result.bottom)], v.slice(12, 16), row);
        assert.equal(rendererCollectionRowLimit(integer(0), integer(1)) >>> 0, v[16], row);
    }
});

// 단순 화면 교차가 아닌 월드 투영 뒤 제한과 signed 오버플로를 명시한다.
test('Renderer 수집 범위: 한도와 잘못된 장치', () => {
    assert.throws(() => rendererWorldRect({}, null), /카메라/); assert.throws(() => rendererCollectionRowLimit(1.5, 10), /크기/);
    assert.equal(rendererCollectionRowLimit(-16, 1), -1); assert.equal(rendererCollectionRowLimit(16, 0x7fffffff), -195225785);
});
