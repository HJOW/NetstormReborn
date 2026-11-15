// 커서 위치 갱신 raw-cursor-position.js(RawCursorPosition.update·screenToWorld·truncatePoint·makeCursorPositionHooks·bindCursorPosition)를 원본 10.78 관측 687행과 cpppj 시험 계약으로 대조한다(WB04-E548).
//  - cursor-position-x86.tsv Point 623행: 18 DWORD 독립 입력(연산·마우스·기준점·그림 크기·카메라·스냅·이전 위치·잠금·Gump·변경·결과 입력)의 호출 순서와 15 DWORD 전체 상태·결과 비트.
//  - cursor-position-x86.tsv Chain 64행: 실제 수명·미리보기·배치를 이은 두 번의 이동 직후 52+11 DWORD 상태와 접근한 슬롯 바이트 전체.
//  - 계약: 현재 카메라/스냅을 복사하지 않고 Take 꼬리 연결·필수 의존·단일 풀·판본 거부.
// 실제 게임·브라우저 실행이 아니다. (2026-11-15 WB04-E548)
import './nan-exact.mjs';
import test from 'node:test';
import assert from 'node:assert/strict';
import path from 'node:path';
import { REPO_ROOT } from './helpers.mjs';
import { bitsToFloat, floatToBits, readRows } from './raw-support.mjs';
import { CursorPreviewScene, CHAIN } from './cursor-preview-support.mjs';
import { CursorDropState } from '../www/core/raw-cursor-drop.js';
import { CursorPlacementState, RawCursorPlacement } from '../www/core/raw-cursor-placement.js';
import { RawCursorLifecycle } from '../www/core/raw-cursor-lifecycle.js';
import { RawCursorPreviews, bindCursorPreviewPlacement } from '../www/core/raw-cursor-previews.js';
import { RawCursorPosition, bindCursorPosition, makeCursorPositionHooks } from '../www/core/raw-cursor-position.js';

// 원본 관측 fixture 이다. 없으면 해당 검사를 건너뛴다. (2026-11-15 WB04-E548)
const rows = readRows(path.join(REPO_ROOT, 'cpppj', 'tests', 'fixtures', 'cursor-position-x86.tsv'));
// 독립 단독 입력 18개의 열 순서이다. 원문 DWORD 를 산술 없이 비트로 읽는다. (2026-11-15 WB04-E548)
const C = Object.freeze({ Operation: 0, MouseX: 1, MouseY: 2, HotX: 3, HotY: 4, Width: 5, Height: 6, CameraX: 7, CameraY: 8, Snap: 9, OldX: 10, OldY: 11, Lock: 12, Legal: 13, ActivateMutation: 14, CreateMutation: 15, InputX: 16, InputY: 17 });
// 쉼표 DWORD 를 원문 순서대로 읽는다. (2026-11-15 WB04-E548)
const numbers = (text) => text.split(',').map((value) => Number(value) >>> 0);

// 단독 위치 갱신의 명시 외부 효과와 공유 참조를 가진 장면이다. (2026-11-15 WB04-E548)
class PositionScene {
    constructor() {
        this.drop = new CursorDropState();
        this.placement = new CursorPlacementState();
        this.camera = [0, 0];
        this.snap = { value: 0 };
        this.input = [];
        this.trace = '';
    }

    // 실제 DWORD 인자를 부호 손실 없이 표시한다(모든 호출 인자를 원문 unsigned 십진수로 기록).
    event(letter, values = []) { this.trace = `${this.trace}${this.trace ? ';' : ''}${letter}${values.map((value) => `:${value >>> 0}`).join('')}`; }

    // 이전 결과를 재사용하지 않고 원문 입력만 초기화한다.
    prepare(values) {
        this.input = values;
        this.drop = new CursorDropState();
        this.placement = new CursorPlacementState();
        this.trace = '';
        this.camera[0] = values[C.CameraX];
        this.camera[1] = values[C.CameraY];
        this.snap.value = values[C.Snap];
        Object.assign(this.drop, { screenX: -3, screenY: 47, x: bitsToFloat(values[C.OldX]), y: bitsToFloat(values[C.OldY]), lockDepth: values[C.Lock], legalGump: values[C.Legal] });
        Object.assign(this.placement, { hotspotX: values[C.HotX] | 0, hotspotY: values[C.HotY] | 0, imageWidth: values[C.Width] | 0, imageHeight: values[C.Height] | 0 });
    }

    // 순서 관찰과 원문에 명시한 전역 변경만 수행한다. 좌표 계산은 제품 몸체가 한다.
    hooks() {
        const input = this.input;
        return {
            activate: (force) => {
                this.event('A', [force, floatToBits(this.drop.x), floatToBits(this.drop.y), this.drop.screenX, this.drop.screenY]);
                if (input[C.ActivateMutation] === 1) this.drop.lockDepth = 3;
                if (input[C.ActivateMutation] === 2) this.drop.lockDepth = 0;
                if (input[C.ActivateMutation] === 3) { this.drop.x = 99; this.drop.y = -77; }
                if (input[C.ActivateMutation] === 4) {
                    this.snap.value = 0;
                    this.camera[0] = floatToBits(1.25);
                    this.camera[1] = floatToBits(-2.5);
                    Object.assign(this.placement, { hotspotX: -5, hotspotY: 9, imageWidth: 13, imageHeight: 17 });
                }
            },
            createPreviews: () => {
                this.event('B', [floatToBits(this.drop.x), floatToBits(this.drop.y), this.drop.screenX, this.drop.screenY, this.drop.lockDepth]);
                const mode = input[C.CreateMutation];
                this.drop.legalGump = mode === 1 || mode === 2 ? 77 : 0;
                if (mode === 2) this.drop.lockDepth = 5;
                if (mode === 3) { this.drop.x = -11.25; this.drop.y = 7.5; }
            },
            selectCursor: (cursor) => this.event('C', [cursor]),
        };
    }

    // 원문의 13 전역과 두 출력 float 의 비트를 같은 순서로 보존한다.
    snapshot(result) {
        const d = this.drop;
        const p = this.placement;
        return [d.screenX, d.screenY, floatToBits(d.x), floatToBits(d.y), d.lockDepth, d.legalGump, this.snap.value, this.camera[0], this.camera[1], p.hotspotX, p.hotspotY, p.imageWidth, p.imageHeight,
            floatToBits(result[0]), floatToBits(result[1])].map((value) => value >>> 0).join(',');
    }
}

// 사슬의 각 이동 직후 63 DWORD 를 같은 장면의 실제 상태에서 직렬화한다. (2026-11-15 WB04-E548)
function chainSnapshot(scene, snap, camera) {
    const values = [scene.drop.screenX, scene.drop.screenY, scene.drop.legalGump, scene.life.currentGump, scene.drop.lockDepth, snap, camera[0], camera[1], scene.drop.inputFlags, scene.drop.costFlags, scene.state.placementFlags];
    // 기존 52 DWORD 뒤에 실제 화면점/Gump/잠금/snap/카메라를 붙인다.
    return values.reduce((text, value) => `${text},${value >>> 0}`, scene.snapshot());
}

test('커서 위치 갱신: 원본 10.78 Point 623입력의 갱신·역투영·절삭 상태·효과 순서·결과 비트가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new PositionScene();
    let count = 0;
    // 각 원문 입력은 새 상태에서만 실행한다.
    for (const row of rows) {
        if (row[0] !== 'Point') continue;
        assert.equal(row.length, 5);
        const input = numbers(row[1]);
        assert.equal(input.length, 18);
        scene.prepare(input);
        const position = new RawCursorPosition(scene.drop, scene.placement, scene.snap, scene.camera, scene.hooks());
        let result = [bitsToFloat(input[C.InputX]), bitsToFloat(input[C.InputY])];
        if (!input[C.Operation]) position.update(input[C.MouseX] | 0, input[C.MouseY] | 0);
        else if (input[C.Operation] === 1) result = RawCursorPosition.screenToWorld(input[C.MouseX] | 0, input[C.MouseY] | 0, scene.camera);
        else result = RawCursorPosition.truncatePoint(result);
        assert.equal(scene.trace || '-', row[2], `Point ${count}: ${row[1]}`);
        assert.equal(scene.snapshot(result), row[3], `Point ${count}: ${row[1]}`);
        count += 1;
    }
    assert.equal(count, 623);
});

test('커서 위치 갱신: 원본 10.78 Chain 64입력의 실제 수명→미리보기→배치 사슬 두 번 이동 상태와 슬롯 바이트가 같다', { skip: !rows && '원본 fixture 없음' }, () => {
    const scene = new CursorPreviewScene();
    let count = 0;
    // 기존 미리보기 장면과 원문 공급자를 공유한다. 원문 fixture 는 바꾸지 않는다.
    for (const row of rows) {
        if (row[0] !== 'Chain') continue;
        const input = numbers(row[1]);
        scene.prepare([input[0], 1, input[1], input[2], 1, 2, 1, 0, floatToBits(20.25), floatToBits(21.75), 0, 1, 0, 1]);
        const snap = { value: input[3] };
        const camera = [floatToBits(3.25), floatToBits(-5.75)];
        scene.drop.lockDepth = input[4];
        scene.drop.costFlags |= input[2] ? 0x200 : 0;
        const previewHooks = scene.hooks();
        const placementHooks = scene.placementHooks(previewHooks);
        if (input[5]) {
            placementHooks.gumpAt = (x, y) => { scene.event('G', x, y); return 1; };
            placementHooks.accept = (gump, event, first, second, third) => { scene.event('A', gump, event, first, second, third); return 2; };
            placementHooks.parent = (gump) => { assert.equal(gump, 2); return 77; };
        }
        const placement = new RawCursorPlacement(scene.pool, scene.types, [], scene.drop, scene.life, scene.state, placementHooks);
        bindCursorPreviewPlacement(previewHooks, scene.pool, placement, placementHooks);
        const previews = new RawCursorPreviews(scene.pool, scene.types, scene.drop, scene.life, scene.surfaces, previewHooks);
        const lifecycle = new RawCursorLifecycle(scene.pool, scene.types, scene.drop, scene.life, { prepareCursor: () => placement.prepare(), destroy: (sid, flags) => scene.event('D', sid, flags) });
        const position = new RawCursorPosition(scene.drop, scene.state, snap, camera, makeCursorPositionHooks(lifecycle, previews, (cursor) => scene.event('K', cursor)));
        let states = '';
        let raws = '';
        // 각 실제 이동의 즉시 상태/바이트를 따로 보존해 두 번째 갱신이 첫 오류를 가리지 못하게 한다.
        for (const mouse of [{ x: 320, y: 220 }, { x: 337, y: 243 }]) {
            position.update(mouse.x, mouse.y);
            if (states) { states += '|'; raws += '|'; }
            states += chainSnapshot(scene, snap.value, camera);
            raws += scene.raws();
        }
        assert.equal(scene.trace.join(';') || '-', row[2], `Chain ${count}: ${row[1]}`);
        assert.equal(states, row[3], `Chain ${count}: ${row[1]}`);
        assert.equal(raws, row[4], `Chain ${count}: ${row[1]}`);
        count += 1;
    }
    assert.equal(count, 64);
    assert.ok(CHAIN === 13);
});

test('커서 위치 계약: 현재 카메라/snap 을 복사하지 않고 Take 꼬리 연결·필수 의존·단일 풀·판본을 확인한다', () => {
    const scene = new PositionScene();
    scene.prepare([0, 0, 0, 0, 0, 0, 0, floatToBits(1.0), floatToBits(2.0), 0, 0, 0, 1, 0, 0, 0, 0, 0]);
    const position = new RawCursorPosition(scene.drop, scene.placement, scene.snap, scene.camera, {});
    const tail = {};
    bindCursorPosition(tail, position);
    // 잠금(깊이 1)이 켜져 있으면 위치만 바뀌고 효과 장치는 필요 없다.
    tail.updatePosition(0, 0);
    assert.ok(scene.drop.x === 1 && scene.drop.y === 2);
    // Take 꼬리의 두 정수 인자는 순서대로 x,y 이다(기준점/크기 0 이면 screenX=x-16, screenY=y-11).
    tail.updatePosition(5, 9);
    assert.ok(scene.drop.screenX === -11 && scene.drop.screenY === -2);
    // 같은 배열의 현재 값과 snap 상자의 현재 값을 읽는다(복사하지 않음): 카메라를 제자리 갱신하고 snap 을 켜면 절삭된다.
    scene.camera[0] = floatToBits(3.75);
    scene.camera[1] = floatToBits(-5.5);
    scene.snap.value = 1;
    tail.updatePosition(0, 0);
    assert.ok(scene.drop.x === 3 && scene.drop.y === -5);
    // 잠금이 풀린 뒤 위치가 바뀌면 필수 효과가 없을 때 거부한다. 판본/단일 풀/선택기도 확인한다.
    scene.drop.lockDepth = 0;
    assert.throws(() => position.update(99, 77), /의존 동작 누락/);
    assert.throws(() => new RawCursorPosition(scene.drop, scene.placement, scene.snap, scene.camera, {}, 'cd1072'), TypeError);
    const first = new CursorPreviewScene().prepare();
    const second = new CursorPreviewScene().prepare();
    const lifecycle = new RawCursorLifecycle(first.pool, first.types, first.drop, first.life, {});
    const previews = new RawCursorPreviews(first.pool, first.types, first.drop, first.life, first.surfaces, {});
    const other = new RawCursorPreviews(second.pool, second.types, second.drop, second.life, second.surfaces, {});
    assert.throws(() => makeCursorPositionHooks(lifecycle, previews, undefined), TypeError);
    assert.throws(() => makeCursorPositionHooks(lifecycle, other, () => {}), TypeError);
    // 정수 산술은 DWORD 로 감기고 카메라 비트 변환은 float 로 좁히지 않는 마지막 저장만 쓴다.
    assert.deepEqual(RawCursorPosition.cameraFromPixels(160, 110).map((bits) => bitsToFloat(bits)), [10, Math.fround(110 * bitsToFloat(0x3dba2e8c))]);
    assert.deepEqual(RawCursorPosition.truncatePoint([-2.75, 3.75]), [-2, 3]);
    assert.deepEqual(RawCursorPosition.truncatePoint([Infinity, 2 ** 70]), [0, 0]);
    assert.deepEqual(RawCursorPosition.screenToWorld(0x7fffffff, 0, [0, 0]).map(Number.isFinite), [true, true]);
});
