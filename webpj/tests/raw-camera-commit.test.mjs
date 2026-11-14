// 497960 카메라 적용의 두 64BYTE·dirty 표·전역·호출 순서를 독립 Native 106행과, 433aa0 즉시 부모/목표와 결합한 168행에 대조한다. 원본게임을 실행하지 않는다. (2026-11-15 WB04-E463)
import test from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import { ClientCameraGlobals } from '../www/core/client-camera-globals.js';
import { commitCameraTarget } from '../www/render/raw-camera-commit.js';
import { Renderer } from '../www/render/renderer.js';
import { scrollScreenPixels } from '../www/render/screen.js';

// 원문 DWORD 입력 비트와 단정도 비트 변환이다.
const bits = (value) => { const view = new DataView(new ArrayBuffer(4)); view.setFloat32(0, value, true); return view.getUint32(0, true); };
const fromBits = (value) => { const view = new DataView(new ArrayBuffer(4)); view.setUint32(0, value >>> 0, true); return view.getFloat32(0, true); };
// 현재 DWORD 배열을 원문과 같은 손실 없는 콜론 구분자로 출력한다(signed 입력도 하위 32비트).
const words = (values) => Array.from(values, (value) => value >>> 0).join(':');
// 64BYTE view 의 16DWORD 를 읽고 쓴다.
const viewWords = (view) => Array.from({ length: 16 }, (_, index) => view.data.getUint32(index * 4, true));
const setViewWords = (view, values) => { values.forEach((value, index) => view.data.setUint32(index * 4, value >>> 0, true)); };

// 실제 초기 입력은 독립 Native 관찰의 기본 장면과 같은 값이다.
const defaultInput = () => [100, 200, 100, 200, 65536, 65536, 65536, 16, 11, 32, 22, 0, 15, 128, 0, 896, 700, 1024, 768, 99, 88, 17, 19, 1, 0, 317, 419, 32768];

// 입력·현재 상태와 명시 효과 경계를 같은 실제 자료형으로 준비한다. 줌/이동/확장 결과는 만들지 않는다.
class CommitScene {
    constructor(input) {
        const number = (index) => input[index] >>> 0; const integer = (index) => input[index] | 0;
        this.camera = new ClientCameraGlobals(); this.dirty = []; this.events = []; this.mutated = new Array(7).fill(false);
        setViewWords(this.camera.rendererView(), [bits(1.25), bits(2.5), 16, 11, number(7), number(8), number(4), number(0), number(1), bits(16), bits(11), bits(16), bits(11), bits(1), bits(100), bits(200)]);
        setViewWords(this.camera.snapshot(), [bits(9), bits(8), 32, 22, number(9), number(10), number(6), 75, 85, bits(32), bits(22), bits(16), bits(11), bits(1), bits(75), bits(85)]);
        const target = this.camera.target(); target.limits[0] = 480; target.limits[1] = 490; target.pixels[0] = integer(2); target.pixels[1] = integer(3); target.zoom = number(5);
        // 호출 뒤에도 같은 객체의 현재 속성을 다시 읽는 전역이다.
        this.globals = { screenFlags: number(11), denominator: integer(12), viewport: { x: integer(13), y: integer(14), width: integer(15), height: integer(16) }, width: integer(17), height: integer(18),
            fullViewport: { left: 3, top: 4, right: 997, bottom: 701 }, mouse: { x: integer(19), y: integer(20) } };
        this.redraw = { value: number(21) }; this.worldRedraw = { value: number(22) }; this.mutation = number(24); this.after = [number(25), number(26), number(27)];
        const profile = number(23);
        // 원문 입력 표만 공급하고 이동 결과는 계산하지 않는다.
        for (let index = 0; profile && index < (profile === 4 ? 100 : 4); index += 1) {
            const rect = profile === 3 ? { left: -2147483648 + index, top: 2147483647 - index, right: 17 + index, bottom: -33 - index } : profile === 4 ? { left: index, top: index + 1, right: index + 10, bottom: index + 20 } : { left: 10 + index, top: 20 + index, right: 30 + index, bottom: 40 + index };
            this.dirty.push({ rect, flags: profile === 4 ? index % 8 : index + (profile === 2 ? 4 : 0) });
        }
    }
    // 원문 외부 호출이 관측한 하위 DWORD 인자를 기록한다.
    event(code, ...values) { this.events.push(values.length ? `${code}:${words(values)}` : code); }
    // 계산 없이 Native 와 같은 명시 callback 입력만 현재 자료에 쓴다(첫 선택 위치에서 한 번만).
    mutate(phase) {
        if (this.mutation !== phase || this.mutated[phase]) return; this.mutated[phase] = true;
        if (phase <= 3) { const target = this.camera.target(); target.pixels[0] = this.after[0] | 0; target.pixels[1] = this.after[1] | 0; target.zoom = this.after[2]; }
        else if (phase === 4) this.dirty.push({ rect: { left: 100, top: 110, right: 120, bottom: 130 }, flags: 2 });
        else if (phase === 5) { this.camera.snapshot().data.setInt32(16, 63, true); this.camera.snapshot().data.setInt32(20, 31, true); }
        else if (phase === 6) { this.globals.viewport = { x: 7, y: 9, width: 333, height: 444 }; this.globals.screenFlags = 18; }
    }
    // 실제 적용 함수가 같은 현재 자료와 같은 표를 빌리게 한다.
    state() { return { renderer: this.camera.rendererView(), snapshot: this.camera.snapshot(), target: this.camera.target(), redraw: this.redraw, worldRedraw: this.worldRedraw, dirty: this.dirty }; }
    // 기존 장치와 dirty 추가의 호출 경계만 관찰한다. 원문 본체의 투영/표 이동을 대체하지 않는다.
    hooks() {
        const camera = this.camera;
        return {
            lockCursor: (skip) => { this.event('L', skip); this.mutate(1); }, unlockCursor: () => { this.event('U'); },
            redrawRoots: () => { const v = camera.rendererView().data; const s = camera.snapshot().data; this.event('R', v.getUint32(24, true), v.getInt32(28, true), v.getInt32(32, true), s.getUint32(24, true), s.getInt32(28, true), s.getInt32(32, true), this.redraw.value, this.worldRedraw.value); this.mutate(2); this.mutate(5); },
            scroll: (x, y, w, h, dx, dy) => { this.event('S', x, y, w, h, dx, dy); this.mutate(3); this.mutate(6); }, scrollBackground: (x, y, w, h, dx, dy) => { this.event('P', x, y, w, h, dx, dy); },
            addDirty: (r, f) => { this.event('D', r.left, r.top, r.right, r.bottom, f); }, addBackgroundDirty: (r) => { this.event('B', r.left, r.top, r.right, r.bottom); },
            updateMouse: (point) => { this.event('M', point.x, point.y); this.mutate(4); }, assertion: (_message, line) => { this.event('A', line); },
        };
    }
    // 전체 현재 변경 표를 원문 5DWORD 행으로 직렬화한다(보존 항목과 미사용 고비트도 비교한다).
    dirtyWords() { return this.dirty.length ? this.dirty.map((entry) => words([entry.rect.left, entry.rect.top, entry.rect.right, entry.rect.bottom, entry.flags])).join('|') : '-'; }
    // 적용 뒤 전체 현재 상태를 열 값으로 직렬화한다.
    observed() {
        const target = this.camera.target(); const g = this.globals;
        return { renderer: words(viewWords(this.camera.rendererView())), snapshot: words(viewWords(this.camera.snapshot())), target: words([target.limits[0], target.limits[1], target.pixels[0], target.pixels[1], target.zoom, this.redraw.value]),
            dirty: this.dirtyWords(), globals: words([g.screenFlags, g.denominator, g.viewport.x, g.viewport.y, g.viewport.width, g.viewport.height, g.width, g.height, g.mouse.x, g.mouse.y, this.worldRedraw.value]), events: this.events.join(';') };
    }
}

// 탭 구분 fixture 의 주석/빈 줄을 건너뛰고 공백 구분 열로 읽는다.
function rows(name) { return fs.readFileSync(new URL(`../../cpppj/tests/fixtures/${name}`, import.meta.url), 'utf8').split(/\r?\n/).filter((line) => line && !line.startsWith('#')).map((line) => line.split('\t')); }

test('카메라 적용 원본 106행: 두 64BYTE·목표/전체 dirty·현재 표/전역·외부 호출이 모두 일치한다', () => {
    const table = rows('camera-commit-x86.tsv'); assert.equal(table.length, 106);
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 34, `행 ${index}`); const input = row.slice(0, 28).map(Number); const scene = new CommitScene(input);
        commitCameraTarget(scene.state(), scene.globals, scene.hooks()); const seen = scene.observed();
        assert.deepEqual([seen.renderer, seen.snapshot, seen.target, seen.dirty, seen.globals, seen.events], row.slice(28), `행 ${index}`);
    }
});

test('카메라 즉시 이동 원본 168행: 433aa0 부모/목표 쓰기 뒤 실제 497960 의 전체 현재 상태·효과·순서가 일치한다', () => {
    const table = rows('camera-immediate-x86.tsv'); assert.equal(table.length, 168);
    for (const [index, row] of table.entries()) {
        assert.equal(row.length, 39, `행 ${index}`); const input = row.slice(0, 28).map(Number); const [x, y, revisionBefore] = row.slice(28, 31).map(Number); const expected = row.slice(31); const scene = new CommitScene(input);
        const revision = { value: revisionBefore }; let entry = '';
        scene.camera.centerImmediately(fromBits(x), fromBits(y), scene.globals.width, scene.globals.height, scene.worldRedraw, revision, () => {
            const target = scene.camera.target(); entry = words([target.pixels[0], target.pixels[1], scene.worldRedraw.value, revision.value]);
            commitCameraTarget(scene.state(), scene.globals, scene.hooks()); // 원문 부모의 현재 전역 쓰기가 끝난 뒤 실제 적용 본체를 실행한다.
        });
        const seen = scene.observed();
        assert.deepEqual([seen.renderer, seen.snapshot, seen.target, seen.dirty, seen.globals, seen.events, String(revision.value), entry], [expected[0], expected[1], expected[2], expected[3], expected[4], expected[5], expected[6], expected[7]], `행 ${index}`);
    }
});

test('카메라 적용: 화면 이동 효과가 뒤 목표를 바꿔도 이동량은 앞에서 보존하고 두 현재 setter 는 새 목표를 읽는다', () => {
    const input = defaultInput(); input[2] = 317; input[3] = 419; input[24] = 3; input[25] = 701; input[26] = 905; const scene = new CommitScene(input);
    commitCameraTarget(scene.state(), scene.globals, scene.hooks()); assert.ok(scene.events.join(';').includes('S:128:0:896:700:217:219'));
    const view = scene.camera.rendererView(); assert.deepEqual([view.pixels.x, view.pixels.y], [701, 905]); assert.deepEqual(scene.camera.snapshot().pixels, view.pixels);
    assert.deepEqual([view.data.getUint32(24, true), scene.camera.target().zoom, scene.redraw.value, scene.worldRedraw.value], [65536, 32768, 1, 19]);
});

test('카메라 적용: 같은 위치 경로는 화면 이동만, 배율 변경 경로는 Gump 재그리기 둘만 요구하고 칸 크기를 배율에 맞춘다', () => {
    const same = new CommitScene(defaultInput()); let scrolls = 0;
    commitCameraTarget(same.state(), same.globals, { scroll: (_x, _y, _w, _h, dx, dy) => { assert.deepEqual([dx, dy], [0, 0]); scrolls += 1; } });
    assert.deepEqual([scrolls, same.redraw.value, same.worldRedraw.value], [3, 17, 19]); assert.deepEqual(same.camera.snapshot().pixels, same.camera.rendererView().pixels);
    const input = defaultInput(); input[5] = 32768; const zoom = new CommitScene(input); let redraws = 0;
    commitCameraTarget(zoom.state(), zoom.globals, { redrawRoots: () => { redraws += 1; } });
    assert.deepEqual([redraws, zoom.redraw.value, zoom.worldRedraw.value], [2, 17, 1]);
    const cells = (view) => [view.data.getInt32(8, true), view.data.getInt32(12, true)]; assert.deepEqual(cells(zoom.camera.rendererView()), [8, 5]); assert.deepEqual(cells(zoom.camera.snapshot()), [16, 11]);
});

test('카메라 적용: IDIV 오류는 실제 시차 분기에서만 거부하고 DIB/일반 경로는 같은 분모 0 을 읽지 않는다', () => {
    const input = defaultInput(); input[12] = 0; const normal = new CommitScene(input); commitCameraTarget(normal.state(), normal.globals, normal.hooks());
    input[11] = 18; const dib = new CommitScene(input); commitCameraTarget(dib.state(), dib.globals, dib.hooks());
    input[11] = 16; const parallax = new CommitScene(input);
    assert.throws(() => commitCameraTarget(parallax.state(), parallax.globals, parallax.hooks()), RangeError); assert.ok(!parallax.events.join(';').includes('P:')); assert.equal(parallax.camera.snapshot().pixels.x, 75);
});

test('즉시 카메라: 실제 497960 장치가 없으면 원문 선행 쓰기를 보존한 채 거부하고 두 setter 로 성공을 만들지 않는다', () => {
    const scene = new CommitScene(defaultInput()); const before = viewWords(scene.camera.rendererView()); const snapshot = viewWords(scene.camera.snapshot()); const worldRedraw = { value: 0xffffffff }; const revision = { value: 0xffffffff };
    assert.throws(() => scene.camera.centerImmediately(100.25, 101.75, scene.globals.width, scene.globals.height, worldRedraw, revision, undefined), { name: 'Error', message: /497960 장치 미연결/ });
    assert.deepEqual([worldRedraw.value, revision.value, Array.from(scene.camera.target().pixels)], [0, 0, [1092, 735]]); assert.deepEqual(viewWords(scene.camera.rendererView()), before); assert.deepEqual(viewWords(scene.camera.snapshot()), snapshot);
});

// 관측 fixture 가 닿지 않는 경계를 독립 입력으로 고정한다(변형 확인에서 생존한 지점): 시차 분모 -1/INT_MIN, 한 칸 이동 띠, DWORD 감김으로 음수가 되는 노출 띠, viewport 경계, 화면 플래그 비트, 모든 항목이 이동하는 표. (2026-11-15 WB04-E463)
// 기본 장면에서 현재 view 위치/목표/전역만 바꾸고 외부 효과 기록을 돌려준다.
function run(mutate) { const scene = new CommitScene(defaultInput()); mutate?.(scene); commitCameraTarget(scene.state(), scene.globals, scene.hooks()); return scene; }
const pick = (scene, letter) => scene.events.filter((event) => event.startsWith(`${letter}:`) || event === letter);

test('카메라 적용: 시차 분모 -1 은 정상 나눗셈이고 INT_MIN/-1 만 원문 IDIV 오류로 거부한다', () => {
    const scene = run((s) => { s.globals.screenFlags = 16; s.globals.denominator = -1; s.camera.target().pixels[0] = 90; s.camera.target().pixels[1] = 190; });
    // 현재 (100,200) → 목표 (90,190): 몫 -100→-90, -200→-190 이므로 배경 이동은 +10/+10 이다.
    assert.deepEqual(pick(scene, 'P'), ['P:128:0:896:700:10:10']);
    const failing = new CommitScene(defaultInput()); failing.globals.screenFlags = 16; failing.globals.denominator = -1; failing.camera.rendererView().data.setInt32(28, -0x80000000, true);
    assert.throws(() => commitCameraTarget(failing.state(), failing.globals, failing.hooks()), /IDIV 오류/); assert.deepEqual(pick(failing, 'P'), []);
    // 몫은 0 쪽으로 절삭한다: -7/2=-3, -4/2=-2 이므로 이동은 +1 이다(바닥 나눗셈이면 -4→-2 로 +2).
    const other = new CommitScene(defaultInput()); other.globals.screenFlags = 16; other.globals.denominator = 2; other.camera.rendererView().data.setInt32(28, -7, true); other.camera.target().pixels[0] = -4;
    commitCameraTarget(other.state(), other.globals, other.hooks()); assert.deepEqual(pick(other, 'P'), ['P:128:0:896:700:1:0']);
});

test('카메라 적용: 한 칸 이동의 노출 띠와 DWORD 감김으로 음수가 된 길이의 끝 좌표 NEG 순서를 원문대로 쓴다', () => {
    const strips = (mutate) => pick(run(mutate), 'D').filter((event) => event.endsWith(':0') && !event.startsWith('D:3:'));
    // dx=+1, dy=+1: 오른쪽/아래 띠가 한 칸 두께다(뷰포트 128,0,896×700).
    assert.deepEqual(strips((s) => { s.camera.target().pixels[0] = 101; s.camera.target().pixels[1] = 201; }), ['D:1023:0:1024:700:0', 'D:128:699:1024:700:0']);
    // dx=-1, dy=-1: 가로 띠는 오른쪽 끝을 x-dx 로, 세로 띠는 끝 좌표를 y+dy 로 두어 길이가 음수라 끝 좌표 자체를 NEG 한다.
    assert.deepEqual(strips((s) => { s.camera.target().pixels[0] = 99; s.camera.target().pixels[1] = 199; }), ['D:128:0:129:700:0', 'D:128:0:1024:1:0']);
    // dx=INT_MIN: right=128-INT_MIN 이 감겨 음수이고 길이도 INT_MIN(<0)이라 NEG 한다. dy=INT_MIN 은 세로에서 같다(y=9).
    assert.deepEqual(strips((s) => { s.camera.target().pixels[0] = (100 - 0x80000000) | 0; }), ['D:128:0:2147483520:700:0']);
    assert.deepEqual(strips((s) => { s.globals.viewport.y = 9; s.camera.target().pixels[1] = (200 - 0x80000000) | 0; }), ['D:128:9:1024:2147483639:0']);
});

test('카메라 적용: viewport 가장자리 화면 이동 조각은 x>0 과 height<화면 높이에서만 추가된다', () => {
    const edge = run((s) => { s.globals.viewport = { x: 1, y: 0, width: 500, height: 768 }; });
    assert.deepEqual(pick(edge, 'S'), ['S:1:0:500:768:0:0', 'S:0:0:1:768:0:0']);
    const flat = run((s) => { s.globals.viewport = { x: 0, y: 0, width: 1024, height: 768 }; }); assert.deepEqual(pick(flat, 'S'), ['S:0:0:1024:768:0:0']);
    const low = run((s) => { s.globals.viewport = { x: 0, y: 0, width: 1024, height: 767 }; }); assert.deepEqual(pick(low, 'S'), ['S:0:0:1024:767:0:0', 'S:0:767:1024:1:0:0']);
});

test('카메라 적용: 화면 플래그는 시차(16)와 DIB(2) 비트만 읽고 다른 비트는 무시한다', () => {
    const moved = (flags) => run((s) => { s.globals.screenFlags = flags; s.globals.denominator = 15; s.camera.target().pixels[0] = 130; });
    for (const flags of [0, 1, 4, 8, 0x20]) assert.deepEqual([pick(moved(flags), 'P'), pick(moved(flags), 'B')], [[], []], `flags ${flags}`);
    assert.equal(pick(moved(17), 'P').length, 1); assert.equal(pick(moved(16), 'P').length, 1); assert.equal(pick(moved(0x30), 'P').length, 1);
    // DIB(2)가 켜지면 시차 이동 대신 이동이 있을 때만 전체 viewport 를 변경 영역으로 더한다(18, 19 모두).
    for (const flags of [18, 19]) { const scene = moved(flags); assert.deepEqual(pick(scene, 'P'), []); assert.ok(scene.events.includes('D:3:4:997:701:0')); }
    const still = run((s) => { s.globals.screenFlags = 18; }); assert.ok(!still.events.includes('D:3:4:997:701:0'));
});

test('카메라 적용: 이동하는 모든 항목이 이동량만큼 옮겨지고 플래그 1 은 보존·2 는 이동 전후 합집합이다', () => {
    const scene = run((s) => { s.camera.target().pixels[0] = 110; s.camera.target().pixels[1] = 195; s.dirty.length = 0; for (let index = 0; index < 5; index += 1) s.dirty.push({ rect: { left: 50 + index, top: 60, right: 80 + index, bottom: 90 }, flags: 0 }); s.dirty.push({ rect: { left: 1, top: 2, right: 3, bottom: 4 }, flags: 1 }, { rect: { left: 50, top: 60, right: 80, bottom: 90 }, flags: 2 }, { rect: { left: 50, top: 60, right: 80, bottom: 90 }, flags: 6 }); });
    // dx=10, dy=-5. 기본 장면의 dirty 를 비우고 더한 8개만 남는다.
    const rects = scene.dirty.map((entry) => [entry.rect.left, entry.rect.top, entry.rect.right, entry.rect.bottom, entry.flags]);
    assert.deepEqual(rects, [[40, 65, 70, 95, 0], [41, 65, 71, 95, 0], [42, 65, 72, 95, 0], [43, 65, 73, 95, 0], [44, 65, 74, 95, 0], [1, 2, 3, 4, 1], [40, 60, 80, 95, 2], [40, 60, 80, 95, 6]]);
});

test('RendererPresentation_ActualCameraCommitSharesDirtyAndRequest: 실제 카메라 적용이 Renderer 의 전체 출력 요청과 변경 표를 같은 저장소로 쓴다', () => {
    const renderer = new Renderer(20, 10); const pixels = new Uint8Array(200); const camera = new ClientCameraGlobals(); const worldRedraw = { value: 0 }; const revision = { value: 0 };
    let depth = 0; let scrolls = 0; let roots = 0; // 원문 현재 전역과 명시 커서/화면 장치 호출 관측이다.
    camera.initializeNativeRenderer(20, 10, 256, worldRedraw, revision); worldRedraw.value = 0; renderer.draw(pixels, 20); renderer.present(() => {});
    // 이동 전 각 열을 구별해 내부 픽셀의 실제 이동을 확인한다.
    for (let y = 0; y < 10; y += 1) for (let x = 0; x < 20; x += 1) pixels[y * 20 + x] = x;
    camera.target().pixels[0] = 2; camera.target().pixels[1] = 0;
    const globals = { screenFlags: 2, denominator: 128, width: 20, height: 10, viewport: { x: 0, y: 0, width: 20, height: 10 }, fullViewport: { left: 0, top: 0, right: 20, bottom: 10 }, mouse: { x: 0, y: 0 } };
    const hooks = {
        lockCursor: (skip) => { assert.equal(skip, 0); depth += 1; }, unlockCursor: () => { depth -= 1; }, redrawRoots: () => { roots += 1; }, updateMouse: () => {},
        addDirty: (rect, value) => renderer.invalidate(rect, value), // 노출 영역 계산은 실제 원문 적용 본체가 수행한다.
        scroll: (x, y, w, h, dx, dy) => {
            assert.deepEqual([x, y, w, h, dx, dy], [0, 0, 20, 10, 2, 0]); scrolls += 1;
            for (let row = 0; row < h; row += 1) pixels.copyWithin(row * w, row * w + dx, row * w + w); // OS 출력 없이 같은 메모리 표면에서 양수 X 이동을 수행한다.
        },
    };
    commitCameraTarget({ renderer: camera.rendererView(), snapshot: camera.snapshot(), target: camera.target(), redraw: renderer.cameraRedraw, worldRedraw, dirty: renderer.cameraDirtyEntries() }, globals, hooks);
    assert.deepEqual([scrolls, roots, depth, renderer.cameraRedraw.value, worldRedraw.value], [1, 2, 0, 1, 0]); // 원문은 이동 시작과 끝에서 각각 Gump 갱신을 호출한다.
    assert.deepEqual([camera.rendererView().pixels.x, camera.snapshot().pixels.x, pixels[10]], [2, 2, 12]);
    const collected = []; const updated = [];
    const world = { collect: (rect) => { collected.push({ left: rect.left, top: rect.top, right: rect.right, bottom: rect.bottom }); return 0; }, background: () => {},
        // 실제 본체가 등록한 노출 띠만 다시 합성한다.
        submit: (surface, pitch, _height, rect) => { for (let row = rect.top; row < rect.bottom; row += 1) surface.fill(99, row * pitch + rect.left, row * pitch + rect.right); } };
    renderer.draw(pixels, 20, 0, world); assert.deepEqual(collected, [{ left: 18, top: 0, right: 20, bottom: 10 }]); assert.deepEqual([pixels[10], pixels[19]], [12, 99]);
    renderer.present((rect) => updated.push(`U:${rect.left}:${rect.top}:${rect.right}:${rect.bottom}`));
    assert.deepEqual(updated, ['U:0:0:20:10']); assert.equal(renderer.cameraRedraw.value, 0);
});

test('ScreenScroll_CameraUsesActualRendererDirtyAndNextDraw: 카메라 적용 뒤 실제 Renderer 의 후처리 dirty 와 다음 Draw/Present 소비가 이어진다', () => {
    const renderer = new Renderer(16, 12); const camera = new ClientCameraGlobals(); camera.rendererView().initialize(); camera.snapshot().initialize(); camera.target().zoom = 65536; camera.target().pixels[0] = 2; camera.target().pixels[1] = 3;
    renderer.cameraDirtyEntries().push({ rect: { left: 4, top: 5, right: 8, bottom: 9 }, flags: 2 }); const pixels = new Uint8Array(16 * 12); const world = { value: 0 };
    const globals = { screenFlags: 2, denominator: 15, width: 16, height: 12, viewport: { x: 0, y: 0, width: 16, height: 12 }, fullViewport: { left: 0, top: 0, right: 16, bottom: 12 }, mouse: { x: 0, y: 0 } };
    const hooks = { lockCursor: () => {}, unlockCursor: () => {}, redrawRoots: () => {}, updateMouse: () => {}, scroll: (x, y, w, h, dx, dy) => scrollScreenPixels(pixels, 16, 16, 12, x, y, w, h, dx, dy), addDirty: (rect, value) => renderer.invalidate(rect, value) };
    commitCameraTarget({ renderer: camera.rendererView(), snapshot: camera.snapshot(), target: camera.target(), redraw: renderer.cameraRedraw, worldRedraw: world, dirty: renderer.cameraDirtyEntries() }, globals, hooks);
    assert.equal(renderer.cameraRedraw.value, 1); assert.ok(renderer.cameraDirtyEntries().length > 0);
    // 기존 표 항목(플래그 2)은 이동 전후 합집합으로 늘고 노출 띠가 더해진다. 다음 Draw 는 전체 화면 하나를 만들고 요청을 소비한다.
    // 전체 출력 요청은 Draw 가 아니라 Present 가 소비한다(기준선 Renderer.cpp:296). cpppj ScreenScrollTests 는 Draw 직후 요청이 0 이라고 단언하지만 같은 기준선 소스와 맞지 않는 시험 결함이다.
    const drawn = renderer.draw(pixels, 16); assert.equal(drawn.length, 1); assert.deepEqual([drawn[0].left, drawn[0].right], [0, 16]); assert.equal(renderer.cameraRedraw.value, 1);
    renderer.present(() => {}); assert.equal(renderer.cameraRedraw.value, 0); assert.equal(renderer.cameraDirtyEntries().length, 0);
});
