// UberGump 카메라 연결부(가장자리 스크롤 입력·497960 적용·ViewSpot 즉시 이동·Present 말미)가 실제 ClientCameraGlobals/Renderer 와 가짜 화면·버튼·월드 장치에서 원문 호출 순서와 공개 조건을 지키는지 검사한다.
// 계산 본체(processEdgeScroll·commitCameraTarget·presentCameraTarget)는 각자의 원본 관측 시험이 대조하므로 여기서는 연결(훅·전역·상태 공유·변경 조건)만 본다. 원본게임을 실행하지 않는다. (2026-11-15 WB04-E469)
import test from 'node:test';
import assert from 'node:assert/strict';
import { ClientCameraGlobals } from '../www/core/client-camera-globals.js';
import { InputCode } from '../www/core/input-queue.js';
import { Renderer } from '../www/render/renderer.js';
import { ScreenMode } from '../www/render/screen.js';
import { UberGumpCamera } from '../www/ui/uber-gump-camera.js';

// Shift 폴링 코드(원문 0x00100000)이다.
const SHIFT = 0x00100000;

// 현재 장치 한 벌을 만든다. options = { pixels, flags, active, world, menu, width, height, config, cells }. 반환된 scene 의 log 에 모든 장치 호출이 순서대로 쌓인다.
function scene(options = {}) {
    const log = []; const width = options.width ?? 640; const height = options.height ?? 480;
    const camera = new ClientCameraGlobals(); const worldRedraw = { value: 5 }; const mapRevision = { value: 0 };
    camera.initializeNativeRenderer(width, height, options.cells ?? 256, worldRedraw, mapRevision);
    if (options.pixels) camera.applyPixelsToRendererAndSnapshot(options.pixels[0], options.pixels[1]);
    const renderer = new Renderer(width, height); renderer.clearDirty();
    const meta = { value: 0 }; const diagnostic = { value: 0 }; const cells = { value: options.cells ?? 256 };
    const state = { active: options.active ?? true, wall: 10, dialog: 0, blocking: false, menuOpen: false, polls: new Map([[InputCode.leftButton, InputCode.release], [SHIFT, InputCode.release]]), world: options.world ?? null, menu: options.menu ?? null, onRedraw: null, onScroll: null };
    const config = { values: { ...(options.config ?? {}) }, readInt(key, current) { log.push(['readInt', key, current]); const found = this.values[key]; return found === undefined || found === (current | 0) ? { changed: false, value: current } : { changed: true, value: found }; } };
    const screen = { width, height, flags: options.flags ?? 0, border: { x: 0, y: 0 }, borderOffset() { return { ...this.border }; },
        scroll: (...args) => { log.push(['scroll', ...args]); state.onScroll?.(); }, flip: () => { log.push(['flip']); state.onFlip?.(); } };
    const cursor = { lockDepth: 1 }; const selection = { mouse: { x: 400, y: 300 } };
    const gameplay = { camera: () => camera, cursor: () => cursor, selection: () => selection, patchDiagnostic: () => diagnostic, worldCells: () => cells };
    const image = { width: 16, height: 16 }; const pointer = { hotspot: () => ({ x: 5, y: 7 }), softwareImage: () => image };
    const client = { configuration: () => config, screen: () => screen, renderer: () => renderer, gameplay: () => gameplay, localState: () => ({ metaRebuild: () => meta }), time: () => ({ wall: state.wall }),
        active: () => state.active, poll: (code) => { log.push(['poll', code]); return { code: state.polls.get(code) ?? 0 }; }, cursor: () => pointer };
    const buttons = { serialGump: (serial) => { log.push(['serial', serial]); return state.menuOpen; }, blockingGumpOpen: () => state.blocking, requestClosePopups: () => { log.push(['close']); },
        currentDialog: () => state.dialog, redrawRoots: () => { log.push(['redraw', cursor.lockDepth]); state.onRedraw?.(); } };
    const host = { client, buttons: () => buttons, world: () => state.world, gameMenuState: () => state.menu, pointerBoundary: { initializationFlags: 0 }, worldRedraw, rebuild: { value: false } };
    return { log, camera, renderer, screen, cursor, selection, diagnostic, cells, meta, state, config, host, image, worldRedraw, uber: new UberGumpCamera(host) };
}

// 설치된 월드 대역이다. 커서 장치가 있으면 활성 커서 잠금/해제 호출을 log 에 남기고, 없으면 hasCursorRuntimeForState 가 거짓이다.
function fakeWorld(log, mapView, cursorRuntime = true) {
    return { mapView: () => ({ ...mapView }), cameraApplied: () => { log.push(['applied']); },
        session: () => ({ hasCursorRuntimeForState: () => cursorRuntime, cursor: () => ({ lock: (skip) => { log.push(['lock', skip]); }, unlock: () => { log.push(['unlock']); } }) }) };
}

test('장치 계약: 장치 한 묶음이 부족하면 만들기 전에 TypeError 이고 초기 가장자리 설정은 보호 PE 값이다', () => {
    const full = scene().host;
    assert.throws(() => new UberGumpCamera({}), TypeError);
    assert.throws(() => new UberGumpCamera(null), TypeError);
    for (const name of ['configuration', 'screen', 'renderer', 'gameplay', 'localState', 'time', 'active', 'poll', 'cursor']) assert.throws(() => new UberGumpCamera({ ...full, client: { ...full.client, [name]: undefined } }), TypeError, name);
    for (const name of ['buttons', 'world', 'gameMenuState']) assert.throws(() => new UberGumpCamera({ ...full, [name]: undefined }), TypeError, name);
    for (const name of ['pointerBoundary', 'worldRedraw', 'rebuild']) assert.throws(() => new UberGumpCamera({ ...full, [name]: null }), TypeError, name);
    const camera = new UberGumpCamera(full);
    assert.equal(camera.edgeScrollEnabled, 1); assert.equal(camera.edgeScrollSpeed, 20); assert.equal(camera.edgeScrollSince, 0);
});

test('가장자리 스크롤: 왼쪽 가장자리에서 설정 읽기→좌버튼 폴링→팝업 닫기→Shift 폴링→목표 쓰기 순서와 시간 가속·속도 한계·공유 상태', () => {
    const s = scene({ pixels: [100, 200] });
    s.uber.pollEdgeScroll({ x: 0, y: 200 });
    // 처음 가장자리: 최초 시각만 정하고 가속 0 → 이동량 2, X 만 -2, 현재 view 는 바꾸지 않는다.
    assert.deepEqual(s.log, [['readInt', 'edgeScroll', 1], ['readInt', 'edgeScrollSpeed', 20], ['poll', InputCode.leftButton], ['close'], ['poll', SHIFT]]);
    assert.deepEqual([...s.camera.target().pixels], [98, 200]); assert.deepEqual(s.camera.rendererView().pixels, { x: 100, y: 200 });
    // snapshot 이 다른 원점이어도 목표는 현재 Renderer view 에서 만든다.
    const apart = scene({ pixels: [100, 200] }); apart.camera.snapshot().setPixels(7, 9); apart.uber.pollEdgeScroll({ x: 0, y: 200 }); assert.deepEqual([...apart.camera.target().pixels], [98, 200]);
    assert.equal(s.meta.value, 1); assert.equal(s.uber.edgeScrollSince, 10); assert.equal(s.host.pointerBoundary.initializationFlags, 4);
    // 0.5초 뒤: 0.5×30+2=17, 같은 목표는 이전 목표에서 누적하지 않고 현재 view 에서 다시 계산한다.
    s.state.wall = 10.5; s.uber.pollEdgeScroll({ x: 0, y: 200 });
    assert.deepEqual([...s.camera.target().pixels], [83, 200]); assert.equal(s.meta.value, 2); assert.equal(s.uber.edgeScrollSince, 10);
    // 1초 뒤: 32 이지만 속도 한계 20 에서 멈춘다.
    s.state.wall = 11; s.uber.pollEdgeScroll({ x: 0, y: 200 });
    assert.deepEqual([...s.camera.target().pixels], [80, 200]);
    // 가장자리를 벗어나면 최초 시각을 지우고 요청하지 않는다. 이미 켜진 초기화 비트와 그 밖의 비트는 보존한다.
    s.host.pointerBoundary.initializationFlags = 0x108 | 4; s.state.wall = 12; s.log.length = 0; s.uber.pollEdgeScroll({ x: 300, y: 200 });
    assert.equal(s.uber.edgeScrollSince, 0); assert.equal(s.meta.value, 3); assert.deepEqual(s.log.map((entry) => entry[0]), ['readInt', 'readInt', 'poll']);
    assert.equal(s.host.pointerBoundary.initializationFlags, 0x10c);
});

test('가장자리 스크롤: 모서리/화면 크기 끝 방향과 아래 방향 요청은 현재 화면 높이의 절반을 지나 목표 Y 에 쓴다', () => {
    const s = scene({ pixels: [100, 200] });
    s.uber.pollEdgeScroll({ x: 639, y: 479 });
    // 오른쪽 끝(x ≥ 폭-1)과 아래 끝(y ≥ 높이-1): 둘 다 +2.
    assert.deepEqual([...s.camera.target().pixels], [102, 202]);
    // 아래쪽이 아닌 위쪽 가장자리는 메뉴 직렬 번호를 조회한다(번호 0, 메뉴 상태 없음).
    const up = scene({ pixels: [100, 200] }); up.uber.pollEdgeScroll({ x: 300, y: 0 });
    assert.deepEqual(up.log.filter((entry) => entry[0] === 'serial'), [['serial', 0]]); assert.deepEqual([...up.camera.target().pixels], [100, 198]);
    // 메뉴 상태가 있으면 그 serial 을 쓰고, 메뉴가 열려 있으면 위쪽 방향이 지워져 요청이 없다.
    const menu = scene({ pixels: [100, 200], menu: { menuSerial: 0x1234 } }); menu.state.menuOpen = true; menu.uber.pollEdgeScroll({ x: 300, y: 0 });
    assert.deepEqual(menu.log.filter((entry) => entry[0] === 'serial'), [['serial', 0x1234]]); assert.equal(menu.meta.value, 0); assert.equal(menu.uber.edgeScrollSince, 0);
});

test('가장자리 스크롤: 설정 두 값을 읽고(누락/같은 값은 유지), 비활성/대화상자/창 테두리/비활성 앱은 폴링 전에 멈춘다', () => {
    const slow = scene({ pixels: [100, 200], config: { edgeScrollSpeed: 1 } }); slow.state.wall = 10; slow.uber.pollEdgeScroll({ x: 0, y: 100 });
    assert.equal(slow.uber.edgeScrollSpeed, 1); assert.deepEqual([...slow.camera.target().pixels], [99, 200]);
    // 같은 값이면 changed=false 로 현재값을 유지한다.
    slow.config.values.edgeScrollSpeed = 1; slow.log.length = 0; slow.uber.pollEdgeScroll({ x: 300, y: 100 }); assert.deepEqual(slow.log[1], ['readInt', 'edgeScrollSpeed', 1]);
    // 설정 0 이면 켜짐 DWORD 가 0 이 되어 어떤 폴링도 하지 않지만 초기화 비트는 이 호출에서 켜진다.
    const off = scene({ pixels: [100, 200], config: { edgeScroll: 0 } }); off.uber.pollEdgeScroll({ x: 0, y: 100 });
    assert.equal(off.uber.edgeScrollEnabled, 0); assert.deepEqual(off.log.map((entry) => entry[0]), ['readInt', 'readInt']); assert.equal(off.host.pointerBoundary.initializationFlags, 4);
    // 설정 키가 사라지면 현재 값(0)을 기본 입력으로 읽으므로 계속 꺼져 있다.
    delete off.config.values.edgeScroll; off.log.length = 0; off.uber.pollEdgeScroll({ x: 0, y: 100 }); assert.equal(off.uber.edgeScrollEnabled, 0); assert.deepEqual(off.log[0], ['readInt', 'edgeScroll', 0]);
    // 다른 앱이 활성일 때는 설정 값이 켜져 있어도 같은 본체에 0 을 준다.
    const inactive = scene({ pixels: [100, 200], active: false }); inactive.uber.pollEdgeScroll({ x: 0, y: 100 });
    assert.equal(inactive.uber.edgeScrollEnabled, 1); assert.deepEqual(inactive.log.map((entry) => entry[0]), ['readInt', 'readInt']); assert.equal(inactive.meta.value, 0);
    // 현재 대화상자나 창 테두리가 있으면 폴링 전에 반환한다.
    const dialog = scene({ pixels: [100, 200] }); dialog.state.dialog = 7; dialog.uber.pollEdgeScroll({ x: 0, y: 100 }); assert.equal(dialog.log.filter((entry) => entry[0] === 'poll').length, 0);
    const border = scene({ pixels: [100, 200] }); border.screen.border = { x: 0, y: 1 }; border.uber.pollEdgeScroll({ x: 0, y: 100 }); assert.equal(border.log.filter((entry) => entry[0] === 'poll').length, 0);
    // 좌버튼이 눌려 있으면(뗌 비트 없음) 가장자리 판정에 들어가지 않는다. 팝업이 막고 있으면 최초 시각을 지우고 닫지 않는다.
    const pressed = scene({ pixels: [100, 200] }); pressed.state.polls.set(InputCode.leftButton, 0); pressed.uber.pollEdgeScroll({ x: 0, y: 100 }); assert.equal(pressed.meta.value, 0);
    const blocking = scene({ pixels: [100, 200] }); blocking.state.blocking = true; blocking.uber.pollEdgeScroll({ x: 0, y: 100 }); assert.equal(blocking.meta.value, 0); assert.equal(blocking.log.some((entry) => entry[0] === 'close'), false);
    // Shift 가 눌려 있으면 절댓값이 큰 한 축만 남긴다(같으면 X 를 지움): 좌상단 모서리 → Y 만.
    const shift = scene({ pixels: [100, 200] }); shift.state.polls.set(SHIFT, 0); shift.uber.pollEdgeScroll({ x: 0, y: 0 }); assert.deepEqual([...shift.camera.target().pixels], [100, 198]);
});

test('카메라 적용: World 없는 이동은 전체 화면 viewport 로 커서 깊이 1→2→1·화면 이동·노출 띠 dirty·재그리기 요청을 한다', () => {
    const s = scene({ pixels: [100, 200] }); s.camera.target().pixels.set([130, 220]); let depthAtRedraw = [];
    s.state.onRedraw = () => { depthAtRedraw.push(s.cursor.lockDepth); };
    s.uber.commitCameraNow();
    assert.deepEqual(s.log, [['redraw', 2], ['scroll', 0, 0, 640, 480, 30, 20], ['redraw', 1]]); assert.deepEqual(depthAtRedraw, [2, 1]); assert.equal(s.cursor.lockDepth, 1);
    assert.deepEqual(s.camera.rendererView().pixels, { x: 130, y: 220 }); assert.deepEqual(s.camera.snapshot().pixels, { x: 130, y: 220 });
    assert.deepEqual(s.renderer.cameraDirtyEntries(), [{ rect: { left: 610, top: 0, right: 640, bottom: 480 }, flags: 0 }, { rect: { left: 0, top: 460, right: 640, bottom: 480 }, flags: 0 }]);
    assert.equal(s.renderer.cameraRedraw.value, 1); assert.equal(s.host.rebuild.value, true);
});

test('카메라 적용: 설치된 World 는 현재 지도 영역을 viewport 로 쓰고 활성 커서를 잠그며 viewport 밖 두 영역도 이동 없이 갱신한다', () => {
    const s = scene({ pixels: [100, 200] }); s.state.world = fakeWorld(s.log, { left: 10, top: 20, right: 630, bottom: 460 }); s.camera.target().pixels.set([130, 220]);
    s.uber.commitCameraNow();
    assert.deepEqual(s.log, [['lock', 0], ['redraw', 1], ['scroll', 10, 20, 620, 440, 30, 20], ['scroll', 0, 0, 10, 480, 0, 0], ['scroll', 10, 440, 630, 40, 0, 0], ['unlock'], ['redraw', 1]]);
    assert.equal(s.cursor.lockDepth, 1);
    assert.deepEqual(s.renderer.cameraDirtyEntries(), [{ rect: { left: 600, top: 20, right: 630, bottom: 460 }, flags: 0 }, { rect: { left: 10, top: 440, right: 630, bottom: 460 }, flags: 0 }]);
    // 활성 커서 장치가 없는 World 는 같은 DWORD 증감 경로를 쓰므로 원문 viewport 만 다르다.
    const bare = scene({ pixels: [100, 200] }); bare.state.world = fakeWorld(bare.log, { left: 0, top: 0, right: 640, bottom: 480 }, false); bare.camera.target().pixels.set([101, 200]); bare.uber.commitCameraNow();
    assert.equal(bare.log.some((entry) => entry[0] === 'lock'), false); assert.equal(bare.cursor.lockDepth, 1);
});

test('카메라 적용: 미설치 World 의 잠금 깊이 0 과 해제 결과 0 은 원문 경계 오류이며 감소는 먼저 일어난다', () => {
    const zero = scene({ pixels: [100, 200] }); zero.cursor.lockDepth = 0; zero.camera.target().pixels.set([101, 200]);
    assert.throws(() => zero.uber.commitCameraNow(), /Lock 경계 필요/); assert.equal(zero.cursor.lockDepth, 0); assert.deepEqual(zero.camera.rendererView().pixels, { x: 100, y: 200 });
    const one = scene({ pixels: [100, 200] }); one.camera.target().pixels.set([101, 200]); one.state.onRedraw = () => { one.cursor.lockDepth = 1; };
    assert.throws(() => one.uber.commitCameraNow(), /preview 재생성 경계 필요/); assert.equal(one.cursor.lockDepth, 0);
});

test('카메라 적용: 소프트웨어 마우스는 비트가 있을 때만 핫스폿을 뺀 현재 마우스로 커서를 강제 갱신하고 비활성 앱은 그림이 없다', () => {
    for (const active of [true, false]) {
        const s = scene({ pixels: [100, 200], flags: ScreenMode.softwareMouse, active }); const calls = [];
        s.renderer.setSoftwareCursor = (...args) => { calls.push(args); }; s.camera.target().pixels.set([130, 220]);
        // 마우스 좌표는 호출 뒤 장치 효과(첫 화면 이동)가 바꾼 현재 값을 읽는다.
        s.state.onScroll = () => { s.selection.mouse = { x: 50, y: 60 }; s.state.onScroll = null; };
        s.uber.commitCameraNow();
        assert.deepEqual(calls, [[active ? s.image : null, { x: 45, y: 53 }, true]]);
    }
    const none = scene({ pixels: [100, 200] }); const calls = []; none.renderer.setSoftwareCursor = (...args) => { calls.push(args); }; none.camera.target().pixels.set([130, 220]); none.uber.commitCameraNow(); assert.deepEqual(calls, []);
});

test('카메라 적용: 시차 비트는 DIB 가 없으면 장치 미복원 오류이고 DIB 가 있으면 이동 시 전체 viewport 를 변경 영역에 더한 뒤 같은 이동을 적용한다', () => {
    const plain = scene({ pixels: [100, 200], flags: 0x10 }); plain.camera.target().pixels.set([130, 220]);
    assert.throws(() => plain.uber.commitCameraNow(), /비DIB DirectDraw 시차 장치 미복원/); assert.deepEqual(plain.camera.rendererView().pixels, { x: 100, y: 200 });
    const dib = scene({ pixels: [100, 200], flags: 0x12 }); dib.camera.target().pixels.set([130, 220]); dib.uber.commitCameraNow();
    // 전체 viewport 항목은 변경 표 이동 단계에서 이동량만큼 옮겨져 {-30,-20,610,460} 이 되고, 뒤에 더해진 노출 띠 둘은 합친 면적이 두 면적 합보다 작아 같은 항목에 병합된다(497580 병합 규칙).
    assert.deepEqual(dib.renderer.cameraDirtyEntries(), [{ rect: { left: -30, top: -20, right: 640, bottom: 480 }, flags: 0 }]);
});

test('카메라 적용: 화면 크기·모드 플래그·지도 영역은 호출 시작의 사본이라 장치 효과가 바꿔도 원문 순서의 뒤 단계는 처음 값을 쓴다', () => {
    // 첫 화면 이동이 폭/높이를 바꿔도 viewport 밖 두 영역의 이동은 처음 640×480 으로 계산한다.
    const sized = scene({ pixels: [100, 200] }); sized.state.world = fakeWorld(sized.log, { left: 10, top: 20, right: 630, bottom: 460 }); sized.camera.target().pixels.set([130, 220]);
    sized.state.onScroll = () => { sized.screen.width = 800; sized.screen.height = 600; sized.state.onScroll = null; }; sized.uber.commitCameraNow();
    assert.deepEqual(sized.log.filter((entry) => entry[0] === 'scroll').slice(1), [['scroll', 0, 0, 10, 480, 0, 0], ['scroll', 10, 440, 630, 40, 0, 0]]);
    // 첫 화면 이동이 모드 플래그의 시차 비트를 지워도 처음 플래그(DIB 시차)로 변경 영역을 더한다.
    const flagged = scene({ pixels: [100, 200], flags: 0x12 }); flagged.camera.target().pixels.set([130, 220]); flagged.state.onScroll = () => { flagged.screen.flags = 0; };
    flagged.uber.commitCameraNow(); assert.deepEqual(flagged.renderer.cameraDirtyEntries(), [{ rect: { left: -30, top: -20, right: 640, bottom: 480 }, flags: 0 }]);
    // 지도 영역 객체를 장치가 계속 바꿔도(살아 있는 객체) 전체 viewport 변경 영역은 호출 시작의 사각형 값이다.
    const live = scene({ pixels: [100, 200], flags: 0x12 }); const rect = { left: 0, top: 0, right: 640, bottom: 480 };
    live.state.world = { ...fakeWorld(live.log, rect), mapView: () => rect }; live.camera.target().pixels.set([130, 220]); live.state.onScroll = () => { rect.right = 100; rect.bottom = 100; };
    live.uber.commitCameraNow(); assert.deepEqual(live.renderer.cameraDirtyEntries(), [{ rect: { left: -30, top: -20, right: 640, bottom: 480 }, flags: 0 }]);
});

test('카메라 적용: 시차 분모와 소프트웨어 마우스 비트는 호출 지점의 현재 값을 읽는다', () => {
    // 첫 화면 이동이 시차 분모를 0 으로 바꾸면 비DIB 시차의 원문 IDIV 오류가 먼저 난다(처음 값 15 였다면 장치 미복원 오류).
    const denominator = scene({ pixels: [100, 200], flags: 0x10 }); denominator.camera.target().pixels.set([130, 220]); denominator.state.onScroll = () => { denominator.camera.denominator().current = 0; };
    assert.throws(() => denominator.uber.commitCameraNow(), /IDIV/);
    // 시작 플래그에 소프트웨어 마우스가 없어도 첫 화면 이동 뒤 켜졌다면 마우스 갱신을 한다(화면 플래그 조회는 4a41c0 호출 위치의 현재 값).
    const mouse = scene({ pixels: [100, 200] }); const calls = []; mouse.renderer.setSoftwareCursor = (...args) => { calls.push(args); }; mouse.camera.target().pixels.set([130, 220]);
    mouse.state.onScroll = () => { mouse.screen.flags = ScreenMode.softwareMouse; }; mouse.uber.commitCameraNow(); assert.equal(calls.length, 1);
});

test('카메라 적용: 배율이 다르면 배율 분기만 실행해 현재 배율·worldRedraw 1 이 되고 이동 분기의 화면 이동은 하지 않는다', () => {
    const s = scene({ pixels: [100, 200] }); s.camera.target().zoom = 131072; s.camera.target().pixels.set([100, 200]); s.uber.commitCameraNow();
    assert.equal(s.camera.rendererView().data.getUint32(24, true), 131072); assert.equal(s.camera.snapshot().data.getUint32(24, true), 131072);
    assert.equal(s.host.worldRedraw.value, 1); assert.deepEqual(s.log, [['redraw', 1], ['redraw', 1]]);
});

test('ViewSpot 즉시 이동: 대본 문자열이 칸 좌표 목표로 바뀌어 같은 현재 부모로 적용되고 위치가 바뀐 때만 월드/재구성을 공개한다', () => {
    const s = scene(); s.state.world = fakeWorld(s.log, { left: 0, top: 0, right: 640, bottom: 480 });
    s.uber.centerViewSpot('(139x88)');
    // 목표는 -(-16×139) - 폭/2 = 1904, -(-11×88) - 높이/2 = 728 이며 즉시 적용되어 현재 Renderer/snapshot 에 같다. 433aa0 은 worldRedraw 를 1 올리고 497f20 은 변환 번호를 1 올린다.
    assert.deepEqual([...s.camera.target().pixels], [1904, 728]); assert.deepEqual(s.camera.rendererView().pixels, { x: 1904, y: 728 }); assert.deepEqual(s.camera.snapshot().pixels, { x: 1904, y: 728 });
    assert.equal(s.worldRedraw.value, 2); assert.equal(s.meta.value, 1); assert.equal(s.host.rebuild.value, true); assert.equal(s.log.filter((entry) => entry[0] === 'applied').length, 1);
    // 같은 위치를 한 번 더 부르면 현재 원점이 바뀌지 않으므로 공개하지 않는다(번호는 다시 증가한다).
    s.host.rebuild.value = false; s.log.length = 0; s.uber.centerViewPoint(139, 88);
    assert.equal(s.worldRedraw.value, 3); assert.equal(s.meta.value, 2); assert.equal(s.host.rebuild.value, false); assert.equal(s.log.some((entry) => entry[0] === 'applied'), false);
    // 한 축만 바뀌어도 공개한다(X 만 → Y 만).
    s.host.rebuild.value = false; s.log.length = 0; s.uber.centerViewPoint(140, 88); assert.equal(s.host.rebuild.value, true); assert.equal(s.log.filter((entry) => entry[0] === 'applied').length, 1);
    s.host.rebuild.value = false; s.log.length = 0; s.uber.centerViewPoint(140, 89); assert.equal(s.host.rebuild.value, true); assert.equal(s.log.filter((entry) => entry[0] === 'applied').length, 1);
    // World 가 없어도 위치가 바뀌면 재구성 예약만 한다.
    const bare = scene(); bare.uber.centerViewPoint(10, 10); assert.equal(bare.host.rebuild.value, true);
    // 숫자가 아닌 필드는 atol 처럼 0 이므로 예외 없이 (0, 0) 칸으로 이동한다.
    const text = scene(); text.uber.centerViewSpot('(abcxdef)'); assert.deepEqual([...text.camera.target().pixels], [-320, -240]);
});

test('Present 말미: 진단 1 이면 아무것도 하지 않고, 아니면 flip 뒤 현재 한계로 목표를 제한해 적용하며 바뀐 때만 공개한다', () => {
    const skipped = scene({ pixels: [100, 200] }); skipped.diagnostic.value = 1; skipped.camera.target().pixels.set([5000, 3000]); skipped.uber.applyCameraFrame();
    assert.deepEqual(skipped.log, []); assert.deepEqual(skipped.camera.rendererView().pixels, { x: 100, y: 200 }); assert.equal(skipped.host.rebuild.value, false);
    const s = scene({ pixels: [100, 200] }); s.state.world = fakeWorld(s.log, { left: 0, top: 0, right: 640, bottom: 480 }); s.camera.target().pixels.set([5000, 3000]); s.uber.applyCameraFrame();
    // 한계(칸 크기×세계 칸 수−화면)는 3456/2336 이고 flip 이 가장 먼저이며 적용 호출이 뒤따른다.
    assert.equal(s.log[0][0], 'flip'); assert.deepEqual(s.camera.rendererView().pixels, { x: 3456, y: 2336 }); assert.deepEqual([...s.camera.target().pixels], [3456, 2336]);
    assert.equal(s.log.filter((entry) => entry[0] === 'applied').length, 1); assert.equal(s.host.rebuild.value, true);
    // 목표가 이미 현재 원점과 같고 배율도 같으면 공개하지 않는다.
    const same = scene({ pixels: [100, 200] }); same.state.world = fakeWorld(same.log, { left: 0, top: 0, right: 640, bottom: 480 }); same.camera.target().pixels.set([100, 200]); same.uber.applyCameraFrame();
    assert.equal(same.log.some((entry) => entry[0] === 'applied'), false); assert.equal(same.host.rebuild.value, false);
    // 배율만 바뀌어도 공개한다.
    const zoom = scene({ pixels: [0, 0] }); zoom.state.world = fakeWorld(zoom.log, { left: 0, top: 0, right: 640, bottom: 480 }); zoom.camera.target().zoom = 131072; zoom.uber.applyCameraFrame();
    assert.equal(zoom.log.filter((entry) => entry[0] === 'applied').length, 1); assert.equal(zoom.host.rebuild.value, true);
    // 배율의 낮은 바이트만 바뀌어도 같은 위치 필드가 아니라 +18 의 DWORD 전체를 비교해 공개한다.
    const small = scene({ pixels: [0, 0] }); small.state.world = fakeWorld(small.log, { left: 0, top: 0, right: 640, bottom: 480 }); small.camera.target().zoom = 65537; small.uber.applyCameraFrame();
    assert.equal(small.camera.rendererView().data.getUint32(24, true), 65537); assert.equal(small.log.filter((entry) => entry[0] === 'applied').length, 1);
    // 한 축만 바뀌어도 공개한다(X 만 → Y 만).
    for (const target of [[101, 200], [100, 201]]) {
        const axis = scene({ pixels: [100, 200] }); axis.state.world = fakeWorld(axis.log, { left: 0, top: 0, right: 640, bottom: 480 }); axis.camera.target().pixels.set(target); axis.uber.applyCameraFrame();
        assert.equal(axis.log.filter((entry) => entry[0] === 'applied').length, 1, String(target)); assert.equal(axis.host.rebuild.value, true);
    }
});

test('Present 말미: 화면 크기는 호출 시작의 사본이라 flip 이 크기를 바꿔도 원문 한계 계산은 처음 값을 쓴다', () => {
    const s = scene({ pixels: [100, 200], cells: 100 }); s.camera.target().limits[0] = 5000; s.camera.target().pixels.set([2000, 100]);
    s.state.onFlip = () => { s.screen.width = 1024; s.screen.height = 768; };
    s.uber.applyCameraFrame();
    // 100칸×16−640=960 (갱신된 1024 를 쓰면 576).
    assert.deepEqual([...s.camera.target().pixels], [960, 100]);
});
