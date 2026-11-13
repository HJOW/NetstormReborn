// 레인 신전 가상 Draw(004dcd00)의 그림/소리 제출 순서를 원본 x86 fixture 로 대조한다(cpppj RainDrawTests 의 10.78 행).
//  - rain-draw-x86.tsv originals 116행: 현재 프레임(+36 DWORD)·WORD 상태(+12)·제출 순서(F번호·S 소리). originalCD 116행은 범위 밖이다.
//  - 입력 코드 표는 cpppj 시험과 같은 29클러스터(A·B×14·C×13·P) 방향 표이며 기대 순서를 다시 계산하지 않는다. 호출 뒤 raw 가 바뀌지 않았는지도 본다.
//  - cpppj 의 Renderer 합성 두 시험(RainDraw_Renderer*)도 실제 SHP 픽셀을 독립적으로 덮어 계산한 화면과 대조한다(WB04-E397). 거부 경로는 웹 계약 검사다.
// 실제 게임·브라우저 실행이 아니다. (2026-11-09 WB04-E203: 신전·마나 추가 그림 이식과 함께 추가)
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows, put } from './raw-support.mjs';
import { FrameCodeTable } from '../www/assets/terrain-tiles.js';
import { isRainVortex, drawRainVortex } from '../www/core/raw-rain-vortex-draw.js';
import { realCaptureAssets } from './capture-assets-support.mjs';
import { GameAssets } from '../www/assets/game-assets.js';
import { Renderer } from '../www/render/renderer.js';

// 원본 x86 fixture 의 10.78 행이다. (2026-11-09 WB04-E203)
const rows = (readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'rain-draw-x86.tsv')) ?? []).filter((row) => row[0] === 'originals');

// 예외가 지정 종류로 나는지 본다. (2026-11-09 WB04-E203)
function throws(action, kind = Error) {
    try { action(); } catch (error) { return error instanceof kind; }
    return false;
}

// 원본 .type 순서와 같은 입력 코드 표이다(방향 글자만 Draw 에 필요하다). (2026-11-09 WB04-E203)
function rainFrames() {
    const letters = `A${'B'.repeat(14)}${'C'.repeat(13)}P`;
    return new FrameCodeTable([...letters].map((letter) => ({ side: letter.charCodeAt(0), variant: 0, flags: 0 })));
}

// 10.78 레인 신전 raw 입력이다. (2026-11-09 WB04-E203)
function rainRaw(frame, state) {
    const raw = new Uint8Array(50);
    put(raw, 0, 0x5196f0);
    raw[10] = 124;
    put(raw, 12, state, 2);
    put(raw, 36, frame);
    return raw;
}

test('RainDraw: 원본 10.78 116행의 그림/소리 제출 순서가 같다', { skip: rows.length === 0 && 'rain-draw-x86.tsv 없음' }, () => {
    const frames = rainFrames();
    let count = 0;
    // 같은 입력을 웹 모듈에만 제공한다.
    for (const row of rows) {
        assert.equal(row.length, 4);
        const raw = rainRaw(Number(row[1]), Number(row[2]));
        const before = raw.slice();
        const trace = [];
        drawRainVortex(raw, frames, { frame: (value) => trace.push(`F${value}`), ascendSound: () => trace.push('S') });
        assert.equal(trace.join(','), row[3], `행 ${count} frame ${row[1]} state ${row[2]}`);
        assert.deepEqual(raw, before);
        count += 1;
    }
    assert.equal(count, 116);
});

test('RainDraw: 다른 가상 표·소비자 누락·방향 프레임 범위 밖을 거부하고 콜백 뒤 현재 raw 를 다시 읽는다(웹 계약 검사)', () => {
    const frames = rainFrames();
    const raw = rainRaw(15, 1);
    assert.equal(isRainVortex(raw), true);
    assert.ok(throws(() => drawRainVortex(raw, frames, { frame() {} }), TypeError));
    assert.ok(throws(() => drawRainVortex(raw, frames, { ascendSound() {} }), TypeError));
    put(raw, 0, 0x501d28);
    assert.equal(isRainVortex(raw), false);
    assert.ok(throws(() => drawRainVortex(raw, frames, { frame() {}, ascendSound() {} }), TypeError));
    // 'C' 개수보다 작지만 코드 표 밖인 음수 프레임은 범위 오류다.
    const negative = rainRaw(-1, 1);
    assert.ok(throws(() => drawRainVortex(negative, frames, { frame() {}, ascendSound() {} }), RangeError));
    // 소리 소비자가 현재 프레임을 바꾸면 마지막 추가 그림은 바뀐 값을 쓴다.
    const live = rainRaw(1, 1);
    const trace = [];
    drawRainVortex(live, frames, { frame: (value) => trace.push(value), ascendSound: () => { trace.push('S'); put(live, 36, 3); } });
    assert.deepEqual(trace, [0, 1, 'S', frames.run('C').first + 3]);
});

// 실제 rainVortex 그림을 쓰는 스프라이트이다. 원본 검사와 같은 좌표·순서·프레임을 지정한다. (2026-11-14 WB04-E397)
function rainSprite(extra = {}) {
    const { table, database } = realCaptureAssets();
    const assets = new GameAssets(table, database);
    return {
        database, block: assets.find('rainVortex').block, frame: 0, x: 200, y: 180,
        clip: { left: 0, top: 0, right: 320, bottom: 240 }, order: { x: 89, y: 116, depth: -1 },
        colors: null, shadow: false, followingFrames: [1], procedure: null, sourceSid: 0, ...extra,
    };
}

// Renderer/공통 픽셀 출력 함수를 쓰지 않고 해독한 SHP의 불투명 픽셀만 직접 덮는 참조 화면이다. (2026-11-14 WB04-E397)
function expectedRain(sprite, width, height, background) {
    const pixels = new Uint8Array(width * height).fill(background);
    // 본체 뒤에 후속 프레임을 같은 순서로 덮되 화면과 스프라이트 clip을 각각 검사한다.
    for (const number of [sprite.frame, ...sprite.followingFrames]) {
        const frame = sprite.database.blocks[sprite.block].frames[number];
        const image = sprite.database.decode(sprite.block, number);
        // 모든 소스 픽셀의 기준점·투명도·화면 경계를 독립 계산한다.
        for (let y = 0; y < image.height; y += 1) {
            // 투명 픽셀은 이미 그려진 본체나 배경을 유지한다.
            for (let x = 0; x < image.width; x += 1) {
                const dx = sprite.x + frame.rect.left + x;
                const dy = sprite.y + frame.rect.top + y;
                const index = y * image.width + x;
                if (image.opacity[index] && dx >= 0 && dx < width && dy >= 0 && dy < height
                    && dx >= sprite.clip.left && dx < sprite.clip.right && dy >= sprite.clip.top && dy < sprite.clip.bottom) {
                    pixels[dy * width + dx] = image.indices[index];
                }
            }
        }
    }
    return pixels;
}

test('RainDraw_RendererLayersAndDirtyPixels: 실제 본체·물결의 전체 픽셀과 이동·교체·삭제 뒤 배경 복원이 같다', () => {
    let sprite = rainSprite();
    const renderer = new Renderer(320, 240);
    const pixels = new Uint8Array(320 * 240).fill(7);
    renderer.setScene([sprite]);
    renderer.draw(pixels, 320, 7);
    renderer.present(() => {});
    assert.deepEqual(pixels, expectedRain(sprite, 320, 240, 7));
    const capture = new Renderer(320, 240);
    capture.setScene([sprite]);
    assert.deepEqual(capture.sceneImage().indices, expectedRain(sprite, 320, 240, 0));
    // 물결을 교체하고 움직이면 이전 본체와 후속 프레임 범위까지 배경으로 복원한다.
    for (const frame of [14, 2, 1]) {
        sprite = { ...sprite, followingFrames: [frame], x: sprite.x + 9, y: sprite.y + 3 };
        renderer.setScene([sprite]);
        renderer.draw(pixels, 320, 7);
        renderer.present(() => {});
        assert.deepEqual(pixels, expectedRain(sprite, 320, 240, 7), `물결 프레임 ${frame}`);
    }
    renderer.setScene([]);
    renderer.draw(pixels, 320, 7);
    assert.ok(pixels.every((color) => color === 7));
});

test('RainDraw_RendererFollowingFrameExtendsBounds: 본체 밖의 후속 프레임만 보여도 범위·합성에 포함된다', () => {
    const sprite = rainSprite({ frame: 1, x: 48, y: 98, clip: { left: 0, top: 0, right: 10, bottom: 10 }, followingFrames: [0] });
    const renderer = new Renderer(10, 10);
    const pixels = new Uint8Array(100);
    renderer.setScene([sprite]);
    renderer.draw(pixels, 10);
    assert.deepEqual(pixels, expectedRain(sprite, 10, 10, 0));
    assert.ok(pixels.some((color) => color !== 0));
    // 후속 그림을 제거하면 같은 화면의 흔적도 남지 않아야 한다(웹 수명 보강).
    renderer.present(() => {});
    renderer.setScene([{ ...sprite, followingFrames: [] }]);
    renderer.draw(pixels, 10);
    assert.deepEqual(pixels, expectedRain({ ...sprite, followingFrames: [] }, 10, 10, 0));
    assert.ok(pixels.every((color) => color === 0));
});
