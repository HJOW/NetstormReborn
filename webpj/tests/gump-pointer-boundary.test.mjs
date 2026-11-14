// 전용 PE fixture가 없는 포인터 안팎 전환 앞단(GumpPointerBoundary: 첫 폴링·정적 초기화·안→밖 기록·밖→안 합성 release 예약)을 기준선 소스 계약으로 검증한다. (2026-11-14 WB04-E433)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 호출 순서/분기는 기준선 소스(GumpPointerBoundary.cpp, 10.78 004d6379..004d65a0)에서 옮겼다. 큐 연결은 실제 40칸 InputQueue를 쓴다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { GumpPointerBoundaryState, prepareGumpPointerBoundary, prepareGumpPointerBoundaryWithQueue } from '../www/ui/gump-pointer-boundary.js';
import { InputCode, InputQueue } from '../www/core/input-queue.js';

const L = InputCode.leftButton; const R = InputCode.rightButton; const OUT = InputCode.outside; const REL = InputCode.release;
// 요청 code 별 현재 OS 상태를 표로 공급하고 폴링 순서를 기록한다.
function rig(table, options = {}) {
    const log = []; const queued = [];
    const hooks = { poll: (code) => { log.push(code); const value = table[code]; return typeof value === 'function' ? value() : { ...value }; }, queue: (event) => { queued.push({ ...event }); options.onQueue?.(event); } };
    return { log, queued, hooks };
}
const pointer = (code, x = 10, y = 20) => ({ code: code >>> 0, x, y });
const POINTER = 0x00120000; const CONTROL = 0x00110000;

test('상태: PE 초기값은 inside 1·초기화 비트 0·기록 0이다', () => {
    const state = new GumpPointerBoundaryState(); assert.deepEqual([state.initializationFlags, state.wasInside], [0, 1]); assert.deepEqual(state.leftOutside, { code: 0, x: 0, y: 0 }); assert.deepEqual(state.rightOutside, { code: 0, x: 0, y: 0 });
});

test('첫 폴링과 언어: 포인터 폴링은 0x120000 이고 언어 3 만 Ctrl(0x110000)을 함께 읽으며 둘 다 눌림이면 첫 code 에만 release 를 더한다', () => {
    const state = new GumpPointerBoundaryState(); const a = rig({ [POINTER]: pointer(0x41), [CONTROL]: pointer(0x42) });
    assert.deepEqual(prepareGumpPointerBoundary(state, 2, a.hooks), pointer(0x41)); assert.deepEqual(a.log, [POINTER]);
    const b = rig({ [POINTER]: pointer(0x41), [CONTROL]: pointer(0x42) }); assert.deepEqual(prepareGumpPointerBoundary(state, 3, b.hooks), pointer((0x41 | REL) >>> 0)); assert.deepEqual(b.log, [POINTER, CONTROL]);
    // 한쪽이라도 뗌이면 더하지 않는다. 언어 3 이 아니면(0 포함) Ctrl 을 읽지 않는다.
    const c = rig({ [POINTER]: pointer((0x41 | REL) >>> 0), [CONTROL]: pointer(0x42) }); assert.equal(prepareGumpPointerBoundary(state, 3, c.hooks).code, (0x41 | REL) >>> 0);
    const d = rig({ [POINTER]: pointer(0x41), [CONTROL]: pointer((0x42 | REL) >>> 0) }); assert.equal(prepareGumpPointerBoundary(state, 3, d.hooks).code, 0x41);
    const e = rig({ [POINTER]: pointer(0x41), [CONTROL]: pointer(0x42) }); assert.equal(prepareGumpPointerBoundary(state, 4, e.hooks).code, 0x41); assert.deepEqual(e.log, [POINTER]);
    const f = rig({ [POINTER]: pointer(0x41) }); assert.equal(prepareGumpPointerBoundary(state, 0, f.hooks).code, 0x41); assert.deepEqual(f.log, [POINTER]);
    // 반환은 사본이라 장치가 돌려준 객체를 바꾸지 않고, 좌표는 그대로 전달한다(signed).
    const original = pointer(0x41, -5, -6); const g = prepareGumpPointerBoundary(state, 3, { poll: (code) => (code === POINTER ? original : pointer(0x42)), queue() {} }); assert.deepEqual(g, pointer((0x41 | REL) >>> 0, -5, -6)); assert.equal(original.code, 0x41);
});

test('정적 초기화: 첫 호출에서 비트 1/2 만 켜고 다른 비트는 보존하며 두 기록을 0 으로 초기화하고 이후에는 다시 쓰지 않는다', () => {
    const state = new GumpPointerBoundaryState(); state.initializationFlags = 0x100; state.leftOutside = pointer(7); state.rightOutside = pointer(9);
    prepareGumpPointerBoundary(state, 1, rig({ [POINTER]: pointer(0) }).hooks); assert.equal(state.initializationFlags, 0x103); assert.deepEqual(state.leftOutside, { code: 0, x: 0, y: 0 }); assert.deepEqual(state.rightOutside, { code: 0, x: 0, y: 0 });
    state.leftOutside = pointer(5); state.rightOutside = pointer(6); prepareGumpPointerBoundary(state, 1, rig({ [POINTER]: pointer(0) }).hooks); assert.deepEqual([state.leftOutside.code, state.rightOutside.code], [5, 6]); assert.equal(state.initializationFlags, 0x103);
    // 비트 하나만 켜져 있으면 나머지 기록만 초기화한다.
    const half = new GumpPointerBoundaryState(); half.initializationFlags = 1; half.leftOutside = pointer(5); half.rightOutside = pointer(6); prepareGumpPointerBoundary(half, 1, rig({ [POINTER]: pointer(0) }).hooks); assert.deepEqual([half.leftOutside.code, half.rightOutside.code, half.initializationFlags], [5, 0, 3]);
    const other = new GumpPointerBoundaryState(); other.initializationFlags = 2; other.leftOutside = pointer(5); other.rightOutside = pointer(6); prepareGumpPointerBoundary(other, 1, rig({ [POINTER]: pointer(0) }).hooks); assert.deepEqual([other.leftOutside.code, other.rightOutside.code, other.initializationFlags], [0, 6, 3]);
});

test('안→밖: inside 였다가 창 밖 포인터가 되면 좌 버튼 폴링 뒤 우 버튼 폴링을 기록하고 wasInside 를 0 으로 쓴다', () => {
    const state = new GumpPointerBoundaryState(); const r = rig({ [POINTER]: pointer((0x41 | OUT) >>> 0), [L]: pointer(L, 1, 2), [R]: pointer(R, 3, 4) });
    prepareGumpPointerBoundary(state, 1, r.hooks); assert.deepEqual(r.log, [POINTER, L, R]); assert.deepEqual(state.leftOutside, pointer(L, 1, 2)); assert.deepEqual(state.rightOutside, pointer(R, 3, 4)); assert.equal(state.wasInside, 0); assert.deepEqual(r.queued, []);
    // 안→안은 추가 폴링이 없고 wasInside 는 1 이다. 밖→밖도 같다.
    const stay = new GumpPointerBoundaryState(); const s = rig({ [POINTER]: pointer(0x41) }); prepareGumpPointerBoundary(stay, 1, s.hooks); assert.deepEqual(s.log, [POINTER]); assert.equal(stay.wasInside, 1);
    const away = new GumpPointerBoundaryState(); away.wasInside = 0; const o = rig({ [POINTER]: pointer((0x41 | OUT) >>> 0) }); prepareGumpPointerBoundary(away, 1, o.hooks); assert.deepEqual(o.log, [POINTER]); assert.equal(away.wasInside, 0);
});

test('밖→안: 좌·우를 먼저 모두 폴링한 뒤 기록상 눌려 있었지만 지금 뗀 버튼마다 현재 폴링 사건을 입력 큐에 예약한다', () => {
    const state = new GumpPointerBoundaryState(); state.initializationFlags = 3; state.wasInside = 0; state.leftOutside = pointer(L); state.rightOutside = pointer(R);
    const r = rig({ [POINTER]: pointer(0x41), [L]: pointer((L | REL) >>> 0, 5, 6), [R]: pointer((R | REL) >>> 0, 7, 8) });
    prepareGumpPointerBoundary(state, 1, r.hooks); assert.deepEqual(r.log, [POINTER, L, R]); assert.deepEqual(r.queued, [pointer((L | REL) >>> 0, 5, 6), pointer((R | REL) >>> 0, 7, 8)]); assert.equal(state.wasInside, 1);
    // 아직 누르고 있으면 예약하지 않는다. 기록이 이미 뗌이면 예약하지 않는다.
    const still = new GumpPointerBoundaryState(); still.initializationFlags = 3; still.wasInside = 0; still.leftOutside = pointer(L); still.rightOutside = pointer((R | REL) >>> 0);
    const s = rig({ [POINTER]: pointer(0x41), [L]: pointer(L), [R]: pointer((R | REL) >>> 0) }); prepareGumpPointerBoundary(still, 1, s.hooks); assert.deepEqual(s.queued, []);
    // 좌만 해당되는 경우와 우만 해당되는 경우.
    const onlyRight = new GumpPointerBoundaryState(); onlyRight.initializationFlags = 3; onlyRight.wasInside = 0; onlyRight.leftOutside = pointer((L | REL) >>> 0); onlyRight.rightOutside = pointer(R);
    const t = rig({ [POINTER]: pointer(0x41), [L]: pointer((L | REL) >>> 0), [R]: pointer((R | REL) >>> 0, 9, 9) }); prepareGumpPointerBoundary(onlyRight, 1, t.hooks); assert.deepEqual(t.queued, [pointer((R | REL) >>> 0, 9, 9)]);
    // 밖에 머무는 동안은 예약하지 않는다(창 밖 포인터에서 밖→밖).
    const outside = new GumpPointerBoundaryState(); outside.initializationFlags = 3; outside.wasInside = 0; outside.leftOutside = pointer(L); const u = rig({ [POINTER]: pointer(OUT) }); prepareGumpPointerBoundary(outside, 1, u.hooks); assert.deepEqual(u.queued, []); assert.deepEqual(u.log, [POINTER]);
});

test('콜백 재진입: 좌 예약 콜백이 바꾼 현재 우 기록을 읽고 마지막에는 현재 포인터 값으로 wasInside 를 덮어쓴다', () => {
    const state = new GumpPointerBoundaryState(); state.initializationFlags = 3; state.wasInside = 0; state.leftOutside = pointer(L); state.rightOutside = pointer(R);
    const r = rig({ [POINTER]: pointer(0x41), [L]: pointer((L | REL) >>> 0), [R]: pointer((R | REL) >>> 0) }, { onQueue: (event) => { state.rightOutside = pointer((R | REL) >>> 0); state.wasInside = 9; } });
    prepareGumpPointerBoundary(state, 1, r.hooks); assert.equal(r.queued.length, 1); assert.equal(state.wasInside, 1);
    // 큐 장치가 없으면 예약이 필요한 분기에서만 거부하고 기록 갱신은 되돌리지 않는다.
    const bare = new GumpPointerBoundaryState(); bare.initializationFlags = 3; bare.wasInside = 0; bare.leftOutside = pointer(L);
    assert.throws(() => prepareGumpPointerBoundary(bare, 1, { poll: (code) => (code === POINTER ? pointer(0x41) : pointer((code | REL) >>> 0)) }), /입력 큐 미연결/); assert.equal(bare.wasInside, 0);
    const fine = new GumpPointerBoundaryState(); fine.initializationFlags = 3; assert.doesNotThrow(() => prepareGumpPointerBoundary(fine, 1, { poll: () => pointer(0x41) }));
    assert.throws(() => prepareGumpPointerBoundary(new GumpPointerBoundaryState(), 1, {}), (error) => error instanceof TypeError && /현재 폴링 필요/.test(error.message)); assert.throws(() => prepareGumpPointerBoundary(new GumpPointerBoundaryState(), 1, undefined), /현재 폴링 필요/);
});

test('실제 40칸 입력 큐 연결: 합성 release 는 raw code/x/y(signed)를 그대로 푸시한다', () => {
    const state = new GumpPointerBoundaryState(); state.initializationFlags = 3; state.wasInside = 0; state.leftOutside = pointer(L); state.rightOutside = pointer(R); const queue = new InputQueue();
    const table = { [POINTER]: pointer(0x41), [L]: pointer((L | REL) >>> 0, -3, -4), [R]: pointer((R | REL) >>> 0, 5, 6) };
    assert.deepEqual(prepareGumpPointerBoundaryWithQueue(state, 1, (code) => ({ ...table[code] }), queue), pointer(0x41));
    assert.deepEqual([queue.pop(false), queue.pop(false)], [{ code: (L | REL) >>> 0, x: -3, y: -4 }, { code: (R | REL) >>> 0, x: 5, y: 6 }]); assert.equal(queue.empty, true);
});
