// 원본 10.78 소리 위치 관측의 모든 화면 투영·가시·좌우·음량 행을 재생한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { soundOnScreen, soundPan, soundToScreen, soundVolume } from '../www/audio/sound-spatial.js';
import { REPO_ROOT } from './helpers.mjs';

// DWORD 비트를 원본 관측의 단정도 입력으로 해석한다.
function floatBits(value) { const bytes = new DataView(new ArrayBuffer(4)); bytes.setUint32(0, Number(value) >>> 0, true); return bytes.getFloat32(0, true); }
// 부호 없는 관측 DWORD를 원본 signed 정수로 해석한다.
function signed(value) { return Number(value) | 0; }

// 장치 대역 없이 관측한 399개 투영과 3360개 화면 위치의 결과를 전부 비교한다.
test('위치 음향: 원본 10.78 화면 투영·가시·좌우·감쇠 3759행', () => {
    const fixture = fs.readFileSync(path.join(REPO_ROOT, 'cpppj/tests/fixtures/soundplay-x86.tsv'), 'utf8');
    let screens = 0, points = 0;
    // 다른 판본과 재생 장치 스크립트는 이 좌표 단위의 대상이 아니다.
    for (const line of fixture.split(/\r?\n/)) {
        if (!line || line.startsWith('#')) continue;
        const row = line.split('\t'); if (row[0] !== 'originals') continue;
        if (row[1] === 'Screen') {
            const view = { cameraX: signed(row[4]), cameraY: signed(row[5]) };
            assert.deepEqual(soundToScreen(view, floatBits(row[2]), floatBits(row[3])), { x: signed(row[6]), y: signed(row[7]) }, `화면 ${screens}`); screens += 1;
        } else if (row[1] === 'Point') {
            const view = { left: signed(row[4]), top: signed(row[5]), right: signed(row[6]), bottom: signed(row[7]) }, point = { x: signed(row[2]), y: signed(row[3]) };
            assert.deepEqual([Number(soundOnScreen(view, point)), soundPan(view, point, row[8] === '1'), soundVolume(view, point)], [Number(row[9]), signed(row[10]), signed(row[11])], `위치 ${points}`); points += 1;
        }
    }
    assert.deepEqual([screens, points], [399, 3360]);
});

// 원본 IDIV 실패 경계는 가운데 좌우 예외를 포함해 구분한다.
test('위치 음향: 0 폭의 가운데와 나누기 오류', () => {
    const view = { left: 10, top: 0, right: 10, bottom: 4 };
    assert.equal(soundPan(view, { x: 10, y: 0 }, false), 0);
    assert.throws(() => soundPan(view, { x: 11, y: 0 }, false), /화면 폭/);
    assert.throws(() => soundVolume(view, { x: 10, y: 0 }), /화면 폭/);
});
