// 가장자리 스크롤(RawEdgeScroll)의 초기화/관문/가속/팝업/Shift 축 선택/목표 요청을 기준선 소스 계약과 손 계산 값으로 검증한다. (2026-11-14 WB04-E442)
// 이 파일의 값은 손 계산이며 원본 직접 관측(edge-scroll 135행) 대조는 raw-camera-native.test.mjs(E465)가 한다. 순서/경계는 기준선 소스(RawEdgeScroll.cpp)에서 옮겼다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { InputCode } from '../www/core/input-queue.js';
import { processEdgeScroll, requestCameraDelta } from '../www/core/raw-edge-scroll.js';
import { CameraView } from '../www/render/camera-view.js';

const RELEASE = InputCode.release; const SHIFT = 0x00100000;
// 기본 입력: 화면 1024×768, 포인터 중앙, 속도 20, 시각 10.
const base = { enabled: 1, dialog: 0, menuSerial: 5, borderX: 0, borderY: 0, width: 1024, height: 768, x: 512, y: 384, speed: 20, wall: 10 };
// 모든 장치 호출을 기록하는 장치 묶음이다. left/shift 는 폴링 코드, menu 는 serial 조회 결과, popup 은 막는 팝업 여부다.
function rig({ left = RELEASE, shift = RELEASE, menu = 0, popup = false, flags, since = 0 } = {}) {
    // 시각이 이미 있는 상태는 초기화(비트 4)가 끝난 상태다(비트가 없으면 시각이 지워진다).
    const log = []; const state = { initializationFlags: flags ?? (since ? 4 : 0), since };
    const hooks = { poll: (code) => { log.push(['poll', code]); return { code: code === InputCode.leftButton ? left : shift, x: 0, y: 0 }; }, menu: (serial) => { log.push(['menu', serial]); return menu; }, blockingPopup: () => { log.push(['popup']); return popup; }, closePopups: () => log.push(['close']), request: (dx, dy) => log.push(['request', dx, dy]) };
    return { log, state, hooks };
}
const run = (r, input = {}) => processEdgeScroll(r.state, { ...base, ...input }, r.hooks);

test('초기화: 비트 4 가 없으면 켜고 시각을 0 으로 지우며 다른 비트는 보존하고, 있으면 시각을 지우지 않는다', () => {
    const r = rig({ flags: 0x8, since: 7 }); run(r, { enabled: 0 }); assert.deepEqual([r.state.initializationFlags, r.state.since], [0xc, 0]);
    const keep = rig({ flags: 0xc, since: 7 }); run(keep, { enabled: 0 }); assert.deepEqual([keep.state.initializationFlags, keep.state.since], [0xc, 7]);
    const wide = rig({ flags: 0x80000000 }); run(wide, { enabled: 0 }); assert.equal(wide.state.initializationFlags, 0x80000004);
});

test('관문: 꺼짐/테두리/대화상자/왼쪽 버튼 눌림이면 방향 0 이고 이후 장치를 읽지 않는다', () => {
    for (const input of [{ enabled: 0 }, { borderX: 3 }, { borderY: -1 }, { dialog: 1 }]) { const r = rig(); assert.deepEqual(run(r, { ...input, x: 0, y: 0 }), [0, 0]); assert.deepEqual(r.log, [], JSON.stringify(input)); }
    // 왼쪽 버튼이 눌려 있으면(폴링 뗌 비트 없음) 방향 0, 뗌만 비교한다(다른 비트는 무시).
    const down = rig({ left: 0x7fffffff }); assert.deepEqual(run(down, { x: 0, y: 0 }), [0, 0]); assert.deepEqual(down.log, [['poll', InputCode.leftButton]]);
    const up = rig({ left: RELEASE | 5 }); assert.deepEqual(run(up, { x: 0 }), [-1, 0]);
});

test('방향: 포인터 x<2 면 -1, x>=너비-1 이면 +1, y<2 이면(메뉴가 없을 때만) -1, y>=높이-1 이면 +1 이고 경계 값이 정확하다', () => {
    const at = (x, y, extra = {}) => run(rig(extra), { x, y });
    assert.deepEqual([at(1, 384)[0], at(2, 384)[0], at(1022, 384)[0], at(1023, 384)[0], at(1024, 384)[0]], [-1, 0, 0, 1, 1]);
    assert.deepEqual([at(512, 1)[1], at(512, 2)[1], at(512, 766)[1], at(512, 767)[1]], [-1, 0, 0, 1]);
    // 위쪽 가장자리는 메뉴 serial 이 있으면 무시하고(아래쪽은 메뉴를 보지 않는다). 메뉴 조회는 y<2 일 때만 일어난다.
    const menu = rig({ menu: 9 }); assert.deepEqual(run(menu, { x: 512, y: 0 }), [0, 0]); assert.deepEqual(menu.log, [['poll', InputCode.leftButton], ['menu', 5]]);
    const noMenu = rig({ menu: 9 }); run(noMenu, { x: 512, y: 767 }); assert.equal(noMenu.log.some((entry) => entry[0] === 'menu'), false);
    assert.deepEqual(at(0, 767), [-1, 1]); assert.deepEqual(at(1023, 0), [1, -1]);
});

test('방향이 없거나 팝업이 막으면 시각을 0 으로 지우고 요청/팝업 닫기 없이 방향만 돌려준다', () => {
    const none = rig({ since: 4 }); assert.deepEqual(run(none), [0, 0]); assert.equal(none.state.since, 0); assert.deepEqual(none.log, [['poll', InputCode.leftButton]]);
    const blocked = rig({ since: 4, popup: true }); assert.deepEqual(run(blocked, { x: 0 }), [-1, 0]); assert.equal(blocked.state.since, 0); assert.deepEqual(blocked.log, [['poll', InputCode.leftButton], ['popup']]);
});

test('가속: 최초 시각을 현재 시각으로 잡고 30×경과+2 의 ftol 을 속도로 제한해 팝업을 닫은 뒤 Shift 를 폴링해 목표를 요청한다(부호 있는 방향×float32 양)', () => {
    // 최초: since=0 → since=wall(10), 경과 0 → amount min(20, ftol(2.0)) = 2 → (-2, 0).
    const first = rig(); assert.deepEqual(run(first, { x: 0 }), [-1, 0]); assert.equal(first.state.since, 10); assert.deepEqual(first.log, [['poll', InputCode.leftButton], ['popup'], ['close'], ['poll', SHIFT], ['request', -2, 0]]);
    // 경과 0.5초: 15+2=17 → 17. 속도가 더 작으면 속도, 경과가 크면 속도에 묶인다.
    const second = rig({ since: 10 }); run(second, { x: 1023, wall: 10.5 }); assert.deepEqual(second.log.at(-1), ['request', 17, 0]);
    const capped = rig({ since: 10 }); run(capped, { x: 1023, wall: 10.5, speed: 10 }); assert.deepEqual(capped.log.at(-1), ['request', 10, 0]); const long = rig({ since: 10 }); run(long, { x: 1023, wall: 100 }); assert.deepEqual(long.log.at(-1), ['request', 20, 0]);
    // 절삭: 경과 0.05 → 1.5+2=3.5 → 3. 이미 시각이 있으면 바꾸지 않는다.
    const trunc = rig({ since: 10 }); run(trunc, { y: 767, wall: 10.05 }); assert.equal(trunc.state.since, 10); assert.deepEqual(trunc.log.at(-1), ['request', 0, 3]);
    // 속도가 정수 최솟값 방향(음수)이어도 min 은 부호 있는 비교이고 float32 로 한 번 저장한다.
    const negative = rig({ since: 10 }); run(negative, { x: 0, wall: 10.5, speed: -4 }); assert.deepEqual(negative.log.at(-1), ['request', 4, 0]);
});

test('Shift 가 눌려 있으면(폴링 뗌 비트 없음) 한 축만 남기고 같은 크기면 X 를 지우며 최종 방향을 반환하고, 풀려 있으면 대각선 그대로다', () => {
    const diagonal = rig({ shift: 0 }); assert.deepEqual(run(diagonal, { x: 0, y: 0 }), [0, -1]); assert.deepEqual(diagonal.log.at(-1), ['request', 0, -2]);
    const free = rig({ shift: RELEASE }); assert.deepEqual(run(free, { x: 0, y: 0 }), [-1, -1]); assert.deepEqual(free.log.at(-1), ['request', -2, -2]);
    const xOnly = rig({ shift: 0 }); assert.deepEqual(run(xOnly, { x: 0 }), [-1, 0]); assert.deepEqual(xOnly.log.at(-1), ['request', -2, 0]);
    const yOnly = rig({ shift: 0 }); assert.deepEqual(run(yOnly, { y: 767 }), [0, 1]); assert.deepEqual(yOnly.log.at(-1), ['request', 0, 2]);
    const other = rig({ shift: 0x7fffffff }); assert.deepEqual(run(other, { x: 1023, y: 767 }), [0, 1]);
});

test('장치 누락은 도달한 위치에서만 Error 이다', () => {
    const state = { initializationFlags: 4, since: 0 };
    assert.throws(() => processEdgeScroll(state, base, {}), { name: 'Error', message: /현재 장치 미연결/ });
    const r = rig(); for (const name of ['menu', 'blockingPopup', 'closePopups', 'request']) { const hooks = { ...r.hooks, [name]: undefined }; const input = { ...base, x: name === 'menu' ? 512 : 0, y: name === 'menu' ? 0 : 384 }; assert.throws(() => processEdgeScroll({ initializationFlags: 4, since: 0 }, input, hooks), { name: 'Error', message: /현재 장치 미연결/ }, name); }
    // 방향이 없으면 팝업/닫기/요청 장치를 요구하지 않는다.
    assert.deepEqual(processEdgeScroll({ initializationFlags: 4, since: 0 }, { ...base, enabled: 1 }, { poll: r.hooks.poll, menu: undefined, blockingPopup: undefined }), [0, 0]);
});

test('requestCameraDelta: 목표 = (현재 X+dx, 높이/2+현재 Y+dy-높이/2)이며 하위 32비트로 감기고 현재 목표에 누적하지 않으며 metaRebuild 를 1 올린다', () => {
    const view = new CameraView(); view.setPixels(100, 50); const target = { pixels: new Int32Array([7, 9]) }; const rebuild = { value: 4 };
    requestCameraDelta(view, target, 768, 5, -3, rebuild); assert.deepEqual(Array.from(target.pixels), [105, 47]); assert.equal(rebuild.value, 5);
    requestCameraDelta(view, target, 767, 0, 0, rebuild); assert.deepEqual(Array.from(target.pixels), [100, 50]); assert.equal(rebuild.value, 6);
    // 감김: 현재 X 가 INT_MAX 면 +1 에서 INT_MIN, 높이/2 합이 감겨도 결과는 현재 Y+dy 이다. metaRebuild 도 32비트로 감긴다.
    view.setPixels(0x7fffffff, 0x7fffffff); const wrap = { value: 0xffffffff }; requestCameraDelta(view, target, 2147483647, 1, 1, wrap); assert.deepEqual(Array.from(target.pixels), [-2147483648, -2147483648]); assert.equal(wrap.value, 0);
    // 음수 높이는 0 방향으로 절삭한 절반을 쓴다(상쇄되므로 결과는 같다).
    requestCameraDelta(view, target, -5, 0, 0, rebuild); assert.deepEqual(Array.from(target.pixels), [0x7fffffff, 0x7fffffff]);
});

test('초기화 경계와 가속 계수: 최하위 비트만 있는 flags 도 비트 4 가 없어 초기화하고, 경과 1초면 30+2=32 이다', () => {
    const r = rig({ flags: 1, since: 7 }); run(r, { enabled: 0 }); assert.deepEqual([r.state.initializationFlags, r.state.since], [5, 0]);
    const second = rig({ since: 10 }); run(second, { x: 1023, wall: 11, speed: 100 }); assert.deepEqual(second.log.at(-1), ['request', 32, 0]);
    // 경과 2초면 60+2=62, 속도 50 이면 50.
    const long = rig({ since: 10 }); run(long, { x: 1023, wall: 12, speed: 100 }); assert.deepEqual(long.log.at(-1), ['request', 62, 0]);
});
