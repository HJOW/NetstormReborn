// Client 수명 전역 두 묶음(ClientAiDebugGlobals·ClientCameraGlobals)의 계약을 대조한다(WB04-E333, 4-D).
//  - 원본 독립 관측 camera-view-x86.tsv 의 `init` 행(실제 4993c0 Native Renderer 초기화): 논리 Renderer 64BYTE·snapshot 복사·worldRedraw·mapRevision 을 재생하고 목표 한계/원점/배율을 대조한다.
//  - cpppj `CameraView_BssExplicitInitializeTargetSnapshotAndDenominatorStayDistinct` 이식(목표/논리 Renderer/snapshot/분모가 서로 다른 주소).
//  - 웹 계약: 497f20 목표 중심 수식, 즉시 중심의 부분 효과 보존, 497960 두 view 쓰기, 분모 선택, AI 진단 7DWORD 주소.
// (2026-11-12 WB04-E333: 4-D 전역 소유자 두 개 이식과 함께 추가)
import assert from 'node:assert/strict';
import test from 'node:test';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { readRows } from './raw-support.mjs';
import { ClientAiDebugGlobals, Int32Box } from '../www/core/client-ai-debug-globals.js';
import { ClientCameraGlobals } from '../www/core/client-camera-globals.js';
import { DwordBox } from '../www/core/client-subscriber-globals.js';
import { CameraView } from '../www/render/camera-view.js';

// 원본 관측 행이다. 파일이 없으면 해당 시험을 건너뛴다. (2026-11-12 WB04-E333)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'camera-view-x86.tsv'));

// 16진 문자열 ↔ 바이트 변환이다. (2026-11-12 WB04-E333)
const hex = (bytes) => Array.from(bytes, (byte) => byte.toString(16).padStart(2, '0')).join('');
const unhex = (text) => Uint8Array.from(text.match(/../g), (pair) => Number.parseInt(pair, 16));

// 상자 두 개(worldRedraw/mapRevision)를 만든다. (2026-11-12 WB04-E333)
const boxes = (redraw = 0, revision = 0) => ({ redraw: new DwordBox(redraw), revision: new DwordBox(revision) });

// ---------------------------------------------------------------- ClientAiDebugGlobals

// 최초 PE/BSS 값: checking 0·상세 null·7로그 DWORD 99·99·29·99·99·99·29, 접근자는 같은 객체이고 두 소유자는 독립이다.
test('AI 진단 전역: 초기값과 안정된 참조', () => {
    const ai = new ClientAiDebugGlobals();
    assert.deepEqual([ai.checking().value, ai.assertionDetail().value], [0, null]);
    const addresses = [0x52f3cc, 0x52f3d0, 0x52f3d4, 0x52f3d8, 0x52f3dc, 0x52f3e0, 0x52f3e4];
    assert.deepEqual(addresses.map((address) => ai.read(address)), [99, 99, 29, 99, 99, 99, 29]);
    assert.equal(ai.checking(), ai.checking());
    assert.equal(ai.assertionDetail(), ai.assertionDetail());
    assert.equal(ai.word(0x52f3d4), ai.word(0x52f3d4));
    assert.notEqual(ai.word(0x52f3cc), ai.word(0x52f3d0));
    assert.notEqual(new ClientAiDebugGlobals().checking(), ai.checking());
});

// 주소 식별자는 정확히 일치하는 일곱 개만 현재 저장소에 연결하고 쓰기는 DWORD 폭이며 checking 은 signed 폭이다. 마지막 오류 상세는 그대로 보존된다.
test('AI 진단 전역: 주소 일치·폭·보존', () => {
    const ai = new ClientAiDebugGlobals();
    for (const address of [0, 0x52f3c8, 0x52f3e8, 0x5e4794, 0x52f3cd]) {
        assert.throws(() => ai.word(address), RangeError);
        assert.throws(() => ai.read(address), /주소 미연결/);
    }
    ai.word(0x52f3e4).value = -1;
    assert.equal(ai.read(0x52f3e4), 0xffffffff);
    ai.word(0x52f3d0).value = 7;
    assert.deepEqual([ai.read(0x52f3cc), ai.read(0x52f3d0), ai.read(0x52f3e4)], [99, 7, 0xffffffff]);
    // checking 은 BOOL 로 좁히지 않는 signed 32비트 전체 비트이다.
    ai.checking().value = 0xfffffffe;
    assert.equal(ai.checking().value, -2);
    ai.checking().value = 2 ** 32 + 5;
    assert.equal(ai.checking().value, 5);
    ai.assertionDetail().value = '마지막 오류 상세';
    assert.equal(ai.assertionDetail().value, '마지막 오류 상세');
    assert.equal(new Int32Box(0x80000000).value, -(2 ** 31));
});

// ---------------------------------------------------------------- ClientCameraGlobals

// 원본 관측 init 행: 입력 64BYTE 를 논리 Renderer 에 두고 4993c0 의 명시 초기화를 실행하면 Renderer 64BYTE·snapshot 복사·worldRedraw·mapRevision 이 관측과 같다.
test('카메라 전역: Native Renderer 초기화 (원본 관측 init 행)', { skip: !rows && 'camera-view-x86.tsv 없음' }, () => {
    const init = rows.filter((row) => row[0] === 'init');
    assert.equal(init.length, 1);
    const row = init[0];
    const camera = new ClientCameraGlobals();
    camera.rendererView().bytes.set(unhex(row[1]));
    const { redraw, revision } = boxes(0x11111111, 0xffffffff);
    camera.initializeNativeRenderer(1024, 768, 256, redraw, revision);
    assert.equal(hex(camera.rendererView().bytes), row[4]);
    assert.equal(hex(camera.snapshot().bytes), row[5]);
    assert.deepEqual([redraw.value, revision.value], [Number(row[7]), Number(row[8])]);
    // 목표: 한계 16×256−1024 / 11×256−768, 원점 0, 배율 Q16 1배이다.
    assert.deepEqual([...camera.target().limits, ...camera.target().pixels, camera.target().zoom], [3072, 2048, 0, 0, 65536]);
});

// 같은 형식이라는 이유로 목표/논리 Renderer/snapshot/분모를 alias 하지 않으며 명시 복사 위치만 현재 값을 바꾼다(cpppj 이식).
test('카메라 전역: 목표·논리 Renderer·snapshot·분모는 서로 다른 주소', () => {
    const first = new ClientCameraGlobals();
    const second = new ClientCameraGlobals();
    const zero = '0'.repeat(128);
    assert.deepEqual([hex(first.snapshot().bytes), hex(first.rendererView().bytes)], [zero, zero]);
    assert.notEqual(first.snapshot().bytes.buffer, first.rendererView().bytes.buffer);
    const { redraw, revision } = boxes(9, 31);
    first.initializeNativeRenderer(1024, 768, 256, redraw, revision);
    assert.deepEqual([redraw.value, revision.value, first.denominator().defaultValue, first.denominator().current], [1, 32, 15, 15]);
    const initial = hex(first.snapshot().bytes);
    first.rendererView().setWorld(1.25, 2.5);
    first.requestPixels(71, 83);
    // 논리 Renderer 만 바뀌었고 snapshot 은 초기 그대로, 목표만 71 이다.
    assert.deepEqual([hex(first.snapshot().bytes), first.rendererView().world.x, first.target().pixels[0]], [initial, 1.25, 71]);
    first.copyRendererSnapshot();
    assert.deepEqual([first.snapshot().world.x, first.snapshot().pixels.x, first.snapshot().pixels.y], [1.25, 20, 27]);
    // 한계는 칸 크기×세계 크기(단정도)−화면 크기이다.
    first.updateTargetLimits(1280, 720, 256, 256);
    assert.deepEqual([first.snapshot().world.y, first.target().limits[0], first.target().limits[1]], [2.5, 2816, 2096]);
    first.applyPixelsToRendererAndSnapshot(17, 29);
    assert.deepEqual([first.snapshot().world.x, first.snapshot().pixels.x, first.target().pixels[0]], [1.0625, 17, 71]);
    const before = hex(first.snapshot().bytes);
    redraw.value = 7;
    first.selectDenominator(0, redraw);
    assert.equal(redraw.value, 7);
    first.selectDenominator(23, redraw);
    assert.deepEqual([redraw.value, first.denominator().current, first.denominator().defaultValue, hex(first.snapshot().bytes)], [1, 23, 15, before]);
    // 다른 소유자는 영향을 받지 않는다.
    assert.deepEqual([hex(second.snapshot().bytes), second.denominator().current], [zero, 15]);
});

// 목표 한계는 double 곱셈의 중간 정밀도를 유지하고 입력 세계 크기를 단정도로 저장한다.
test('카메라 전역: 목표 한계의 단정도 입력과 중간 정밀도', () => {
    const camera = new ClientCameraGlobals();
    const { redraw, revision } = boxes();
    camera.initializeNativeRenderer(1024, 768, 256, redraw, revision);
    // 16×fround(300.7)=4811.2001953…−1024 → 3787, 11×fround(200.3)=2203.30003…−768 → 1435.
    camera.updateTargetLimits(1024, 768, 300.7, 200.3);
    assert.deepEqual([...camera.target().limits], [3787, 1435]);
    // 음수 결과는 0 방향 절삭이고 범위 밖(비유한)은 하위 DWORD 0 이다.
    camera.updateTargetLimits(1024, 768, 10.5, 10.5);
    assert.deepEqual([...camera.target().limits], [Math.trunc(16 * 10.5 - 1024), Math.trunc(11 * 10.5 - 768)]);
    camera.updateTargetLimits(1024, 768, Infinity, NaN);
    assert.deepEqual([...camera.target().limits], [0, 0]);
    assert.throws(() => camera.initializeNativeRenderer(0, 768, 256, redraw, revision), RangeError);
    assert.throws(() => camera.initializeNativeRenderer(1024, -1, 256, redraw, revision), /초기 화면 크기 오류/);
});

// 497f20: 음수 16/11 곱의 ftol 하위 DWORD 를 NEG/SUB 하고 (화면/2 절삭)을 뺀다. 두 목표 store 뒤에만 revision 이 오른다. 현재 view/snapshot 은 건드리지 않는다.
test('카메라 전역: 목표 중심 수식과 revision', () => {
    const camera = new ClientCameraGlobals();
    const { redraw, revision } = boxes(5, 40);
    camera.initializeNativeRenderer(1024, 768, 256, redraw, revision);
    const view = hex(camera.rendererView().bytes);
    const snapshot = hex(camera.snapshot().bytes);
    const counter = new DwordBox(10);
    camera.requestCenter(10.5, 7.25, 1024, 768, counter);
    // X: 0 − ftol(−168) − 512 = −344, Y: 0 − ftol(−79.75) − 384 = 0 + 79 − 384 = −305.
    assert.deepEqual([camera.target().pixels[0], camera.target().pixels[1], counter.value], [-344, -305, 11]);
    // 화면 크기가 홀수이면 절삭이다(1025/2 = 512, 769/2 = 384).
    camera.requestCenter(10.5, 7.25, 1025, 769, counter);
    assert.deepEqual([camera.target().pixels[0], camera.target().pixels[1], counter.value], [-344, -305, 12]);
    // 음수 중심은 부호 있는 DWORD 감김으로 계산한다: X: 0 − ftol(56) − 512 = −568, Y: 0 − ftol(27.5) − 384 = −411.
    camera.requestCenter(-3.5, -2.5, 1024, 768, counter);
    assert.deepEqual([camera.target().pixels[0], camera.target().pixels[1]], [-568, -411]);
    assert.deepEqual([hex(camera.rendererView().bytes), hex(camera.snapshot().bytes)], [view, snapshot]);
});

// 433aa0: worldRedraw 증가 → 497f20 → commit 순서이고 commit 이 없으면 앞선 쓰기를 보존한 채 거부한다.
test('카메라 전역: 즉시 중심의 순서와 부분 효과 보존', () => {
    const camera = new ClientCameraGlobals();
    const redraw = new DwordBox(3);
    const revision = new DwordBox(8);
    const trace = [];
    camera.centerImmediately(10.5, 7.25, 1024, 768, redraw, revision, () => {
        trace.push([redraw.value, revision.value, camera.target().pixels[0]]);
    });
    // commit 시점에는 worldRedraw 와 목표/revision 이 이미 쓰였다.
    assert.deepEqual(trace, [[4, 9, -344]]);
    // commit 이 없으면 거부하지만 앞선 쓰기는 남는다.
    assert.throws(() => camera.centerImmediately(1, 1, 1024, 768, redraw, revision, null), /497960 장치 미연결/);
    assert.deepEqual([redraw.value, revision.value, camera.target().pixels[0]], [5, 10, 0 - Math.trunc(-16) - 512]);
});

// 분모: 인자 0 은 기본 선택이며 현재와 같으면 redraw 를 쓰지 않는다. 값이 다르면 현재 분모를 먼저 저장한 뒤 redraw 1 이다.
test('카메라 전역: 분모 선택', () => {
    const camera = new ClientCameraGlobals();
    const redraw = new DwordBox(0);
    camera.selectDenominator(15, redraw);
    assert.deepEqual([redraw.value, camera.denominator().current], [0, 15]);
    camera.selectDenominator(20, redraw);
    assert.deepEqual([redraw.value, camera.denominator().current], [1, 20]);
    redraw.value = 0;
    camera.selectDenominator(20, redraw);
    assert.equal(redraw.value, 0);
    camera.selectDenominator(0, redraw);
    assert.deepEqual([redraw.value, camera.denominator().current, camera.denominator().defaultValue], [1, 15, 15]);
});

// 최근 건설 이력과 숫자 키 저장 위치는 최초 BSS 0 이며 접근자는 같은 객체이다.
test('카메라 전역: 건설 이력과 저장 위치의 안정된 소유', () => {
    const camera = new ClientCameraGlobals();
    assert.equal(camera.constructionHistory(), camera.constructionHistory());
    assert.equal(camera.savedPositions(), camera.savedPositions());
    assert.equal(camera.snapshot(), camera.snapshot());
    assert.equal(camera.rendererView(), camera.rendererView());
    assert.equal(camera.target(), camera.target());
    assert.ok(camera.constructionHistory().points.every((point) => point.x === 0 && point.y === 0));
    assert.equal(camera.constructionHistory().index, 0);
    assert.ok(camera.savedPositions().bits.every((bit) => bit === 0));
    camera.savedPositions().set(3, 1.5, 2.5);
    assert.deepEqual(camera.savedPositions().get(3), { x: 1.5, y: 2.5 });
    assert.ok(camera.snapshot() instanceof CameraView);
});

// ---------------------------------------------------------------- 변형 확인으로 보강한 경계 (WB04-E333)

// 초기화는 이전 목표 원점을 0 으로 되돌린다(미리 쓴 요청이 남지 않음).
test('카메라 전역: Native 초기화는 이전 목표 원점을 지운다', () => {
    const camera = new ClientCameraGlobals();
    camera.requestPixels(5, 6);
    camera.target().limits.fill(9);
    const { redraw, revision } = boxes();
    camera.initializeNativeRenderer(1024, 768, 256, redraw, revision);
    assert.deepEqual([...camera.target().pixels], [0, 0]);
    assert.deepEqual([...camera.target().limits], [3072, 2048]);
});

// 입력 세계 크기는 단정도로 저장된 값을 쓴다: 2^24+1 은 단정도로 2^24 이다.
test('카메라 전역: 세계 크기와 중심 입력의 단정도 저장', () => {
    const camera = new ClientCameraGlobals();
    const { redraw, revision } = boxes();
    camera.initializeNativeRenderer(1024, 768, 256, redraw, revision);
    camera.updateTargetLimits(1024, 768, 16777217, 16777217);
    assert.deepEqual([...camera.target().limits], [16 * 16777216 - 1024, 11 * 16777216 - 768]);
    // 중심 입력이 단정도로 저장된 뒤 곱한다: 0.0624999999 는 단정도 0.0625 이므로 16 배는 정확히 1 이다.
    const counter = new DwordBox(0);
    camera.requestCenter(0.0624999999, 2 - 1e-9, 1024, 768, counter);
    assert.deepEqual([camera.target().pixels[0], camera.target().pixels[1]], [1 - 512, 22 - 384]);
});

// 497960 조각은 논리 Renderer 와 전역 snapshot 두 곳에 같은 픽셀 원점을 쓴다.
test('카메라 전역: 497960 조각은 두 view 모두에 쓴다', () => {
    const camera = new ClientCameraGlobals();
    camera.applyPixelsToRendererAndSnapshot(-17, 29);
    assert.deepEqual([camera.rendererView().pixels, camera.snapshot().pixels], [{ x: -17, y: 29 }, { x: -17, y: 29 }]);
    assert.notEqual(camera.rendererView().bytes, camera.snapshot().bytes);
    // 한쪽만 바꾸면 다른 쪽은 그대로이다.
    camera.rendererView().setPixels(1, 2);
    assert.deepEqual([camera.rendererView().pixels, camera.snapshot().pixels], [{ x: 1, y: 2 }, { x: -17, y: 29 }]);
});
