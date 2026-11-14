// 전용 PE fixture가 없는 Gump 입력 어댑터(GumpControlInput: 사각형 판정·돌 버튼 캡처/뗌 실행·목록 행 즉시 실행/호버·포커스 해제)를 기준선 소스 계약으로 검증한다. (2026-11-14 WB04-E433)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 판정/분기는 기준선 소스(Gump.cpp)에서 옮겼다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { GumpControl, GumpControlInput } from '../www/ui/gump-control-input.js';
import { InputCode } from '../www/core/input-queue.js';

const rect = (left, top, right, bottom) => ({ left, top, right, bottom });
// 돌 버튼(번호 1: 0..10), 목록 행(2: 20..30), 즉시 버튼(3: 40..50), 비활성(4: 60..70), 앞쪽에 겹치는 비활성(5: 0..10 아래 겹침)을 가진 어댑터다.
function made(extra = []) {
    const input = new GumpControlInput();
    input.setControls([{ id: 1, rect: rect(0, 0, 10, 10) }, { id: 2, rect: rect(20, 0, 30, 10), menu: true }, { id: 3, rect: rect(40, 0, 50, 10), instant: true }, { id: 4, rect: rect(60, 0, 70, 10), enabled: false }, ...extra]);
    return input;
}
const press = (x, y, extra = 0) => ({ code: (InputCode.leftButton | extra) >>> 0, x, y });
const release = (x, y) => ({ code: (InputCode.leftButton | InputCode.release) >>> 0, x, y });

test('GumpControl: 기본값은 menu/instant false·enabled true 이고 일부 필드만 주어도 나머지는 기본값이다', () => {
    const control = new GumpControl(); assert.deepEqual([control.id, control.menu, control.enabled, control.instant], [0, false, true, false]); assert.deepEqual(control.rect, { left: 0, top: 0, right: 0, bottom: 0 });
    const partial = new GumpControl({ id: 7, instant: true }); assert.deepEqual([partial.id, partial.menu, partial.enabled, partial.instant], [7, false, true, true]);
    // setControls 는 일반 객체도 같은 기본값으로 채운다.
    const input = new GumpControlInput(); input.setControls([{ id: 9, rect: rect(0, 0, 1, 1) }]); assert.equal(input.controls()[0].enabled, true); assert.ok(input.controls()[0] instanceof GumpControl);
    assert.deepEqual(new GumpControlInput().controls(), []);
});

test('contains: 왼쪽/위는 포함하고 오른쪽/아래 끝은 제외한다', () => {
    const r = rect(2, 3, 5, 7); const contains = (x, y) => GumpControlInput.contains(r, { x, y });
    assert.ok(contains(2, 3)); assert.ok(contains(4, 6)); assert.ok(!contains(5, 6)); assert.ok(!contains(4, 7)); assert.ok(!contains(1, 3)); assert.ok(!contains(2, 2)); assert.ok(!contains(5, 7));
    // 음수 좌표/빈 사각형도 정수 비교다.
    assert.ok(GumpControlInput.contains(rect(-5, -5, 0, 0), { x: -1, y: -1 })); assert.ok(!GumpControlInput.contains(rect(3, 3, 3, 3), { x: 3, y: 3 }));
});

test('돌 버튼: 누르면 캡처만 하고 같은 곳에서 뗄 때 번호를 돌려주며 밖에서 떼면 실행하지 않는다', () => {
    const input = made(); assert.equal(input.event(press(5, 5)), undefined); assert.equal(input.pressed(), 1);
    assert.equal(input.event(release(5, 5)), 1); assert.equal(input.pressed(), undefined);
    // 누른 채 밖으로 나가면 모양이 사라지고 거기서 떼면 실행하지 않으며 캡처가 지워진다. 다시 들어와도 눌린 모양이 아니다.
    input.event(press(5, 5)); assert.equal(input.move({ x: 100, y: 100 }), true); assert.equal(input.pressed(), undefined);
    assert.equal(input.event(release(100, 100)), undefined); assert.equal(input.move({ x: 5, y: 5 }), false); assert.equal(input.pressed(), undefined);
    // 누른 채 밖으로 나갔다 돌아오면 같은 번호의 눌린 모양이 돌아온다(캡처 유지).
    input.event(press(5, 5)); input.move({ x: 100, y: 100 }); assert.equal(input.move({ x: 6, y: 6 }), true); assert.equal(input.pressed(), 1); assert.equal(input.event(release(6, 6)), 1);
    // 아무 곳도 누르지 않은 뗌은 실행하지 않는다.
    assert.equal(input.event(release(5, 5)), undefined);
});

test('목록 행/즉시 버튼: 누르는 순간 번호를 돌려주고 캡처하지 않으며 목록 행만 호버가 있다', () => {
    const input = made(); assert.equal(input.event(press(25, 5)), 2); assert.equal(input.pressed(), undefined); assert.equal(input.hoveredMenu(), 2);
    assert.equal(input.event(press(45, 5)), 3); assert.equal(input.pressed(), undefined); assert.equal(input.hoveredMenu(), undefined);
    // 뗌은 캡처가 없어 실행하지 않는다.
    assert.equal(input.event(release(25, 5)), undefined);
    // 호버 이동: 목록 행 위에서만 true/false 로 변하고 같은 곳은 변화 없음.
    const hover = made(); assert.equal(hover.move({ x: 25, y: 5 }), true); assert.equal(hover.hoveredMenu(), 2); assert.equal(hover.move({ x: 26, y: 5 }), false); assert.equal(hover.move({ x: 1, y: 1 }), true); assert.equal(hover.hoveredMenu(), undefined);
    // 돌 버튼 위는 눌림이 없으면 아무 모양도 아니다.
    assert.equal(hover.move({ x: 5, y: 5 }), false); assert.equal(hover.pressed(), undefined);
});

test('비활성/겹침: 활성 영역의 앞쪽 항목이 사건을 받고 비활성 영역을 누르면 아무것도 하지 않는다', () => {
    const input = made(); assert.equal(input.event(press(65, 5)), undefined); assert.equal(input.pressed(), undefined); assert.equal(input.move({ x: 65, y: 5 }), false);
    // 같은 자리에 겹친 두 번째 항목은 첫 항목이 받는다(첫 항목이 비활성이어도 그 항목에서 끝난다).
    const overlap = new GumpControlInput(); overlap.setControls([{ id: 10, rect: rect(0, 0, 10, 10), enabled: false }, { id: 11, rect: rect(0, 0, 10, 10) }]); assert.equal(overlap.event(press(5, 5)), undefined); assert.equal(overlap.pressed(), undefined);
    const front = new GumpControlInput(); front.setControls([{ id: 12, rect: rect(0, 0, 10, 10) }, { id: 13, rect: rect(0, 0, 10, 10), menu: true }]); assert.equal(front.event(press(5, 5)), undefined); assert.equal(front.pressed(), 12);
    // Move 의 호버/눌림 계산은 활성 항목만 본다: 비활성이 앞이고 활성 메뉴가 뒤면 뒤 항목이 강조된다.
    overlap.setControls([{ id: 14, rect: rect(0, 0, 10, 10), enabled: false, menu: true }, { id: 15, rect: rect(0, 0, 10, 10), menu: true }]); assert.equal(overlap.move({ x: 5, y: 5 }), true); assert.equal(overlap.hoveredMenu(), 15);
});

test('사건 종류: 창 밖 비트는 취소하고 왼쪽 버튼 이외의 누름은 위치도 갱신하지 않으며 종류 마스크 밖 비트는 무시한다', () => {
    const input = made(); input.event(press(5, 5)); assert.equal(input.pressed(), 1);
    // 창 밖 사건: 캡처/눌림/호버를 지우고 아무것도 돌려주지 않는다.
    assert.equal(input.event({ code: (InputCode.leftButton | InputCode.outside) >>> 0, x: 5, y: 5 }), undefined); assert.equal(input.pressed(), undefined);
    // 오른쪽/가운데/키 사건은 무시한다(호버도 안 바뀐다).
    input.move({ x: 25, y: 5 }); assert.equal(input.hoveredMenu(), 2);
    for (const code of [InputCode.rightButton, InputCode.middleButton, 0x41, 0, (InputCode.leftButton | 0x100000) >>> 0]) assert.equal(input.event({ code: code >>> 0, x: 5, y: 5 }), undefined, String(code));
    assert.equal(input.hoveredMenu(), 2);
    // 마스크 밖 비트(shift/control/pressedSince)는 왼쪽 버튼 판정에 영향이 없다.
    assert.equal(input.event({ code: (InputCode.leftButton | InputCode.shift | InputCode.control | InputCode.pressedSince) >>> 0, x: 5, y: 5 }), undefined); assert.equal(input.pressed(), 1);
    assert.equal(input.event({ code: (InputCode.leftButton | InputCode.shift | InputCode.release) >>> 0, x: 5, y: 5 }), 1);
    // 사건 위치가 곧 새 포인터 위치다: 누른 곳과 다른 곳에서 뗄 때 모양이 사라진다.
    input.event(press(5, 5)); assert.equal(input.event(release(25, 5)), undefined);
});

test('move(active=false)·cancel·setControls: 캡처와 강조를 지우고 변화 여부를 돌려주며 새 화면은 이전 누름을 실행하지 않는다', () => {
    const input = made(); input.event(press(5, 5)); assert.equal(input.move({ x: 5, y: 5 }, false), true); assert.equal(input.pressed(), undefined);
    assert.equal(input.event(release(5, 5)), undefined); assert.equal(input.move({ x: 5, y: 5 }, false), false);
    // 비활성 이동은 목록 행의 호버도 지운다.
    input.move({ x: 25, y: 5 }); assert.equal(input.hoveredMenu(), 2); assert.equal(input.move({ x: 25, y: 5 }, false), true); assert.equal(input.hoveredMenu(), undefined);
    // 포커스 상실 취소와 화면 교체.
    input.event(press(5, 5)); input.cancel(); assert.equal(input.pressed(), undefined); assert.equal(input.event(release(5, 5)), undefined);
    input.event(press(5, 5)); input.setControls([{ id: 1, rect: rect(0, 0, 10, 10) }]); assert.equal(input.pressed(), undefined); assert.equal(input.event(release(5, 5)), undefined);
    // 입력 목록의 복사본이다(외부 배열 변경이 영향을 주지 않는다).
    const source = [{ id: 1, rect: rect(0, 0, 10, 10) }]; input.setControls(source); source.length = 0; assert.equal(input.controls().length, 1);
});
