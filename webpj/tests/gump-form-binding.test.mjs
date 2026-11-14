// 전용 PE fixture가 없는 Gump Form 연결(GumpFormBinding: 월드 객체 투영·소유 UI 알림·기본 해제·메뉴 Form 공급·Form 삭제 분배)을 기준선 소스 계약으로 검증한다. (2026-11-14 WB04-E433)
// cpppj에도 직접 시험이 없어 원본 직접 관측 수치가 아니다. 호출 순서/경계는 기준선 소스(GumpFormBinding.cpp, 기준선 뒤 4ee69f4f 포함)에서 옮겼다. 풀/Form 호스트/수명/카메라는 호출 기록 대역이고 Gump 등록 상태는 실제 GumpCreateState 다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { projectGumpObject, makeGumpFormNotice, releaseGumpObject, bindMenuGumpForms, bindGumpFormLifetime } from '../www/ui/gump-form-binding.js';
import { GumpCreateState } from '../www/ui/gump-create.js';
import { MenuGumpWidgetState } from '../www/ui/menu-gump-widgets.js';

// 슬롯 바이트(+14/+18 의 float32)를 가진 대역 풀이다. slot 은 독립 사본이다.
function poolWith(x, y, edition = 'patch1078') {
    const bytes = new Uint8Array(40); const view = new DataView(bytes.buffer); view.setFloat32(14, x, true); view.setFloat32(18, y, true);
    const requests = []; return { edition, slot: (sid) => { requests.push(sid); const copy = new Uint8Array(40); copy.set(bytes); return copy; }, requests };
}
// 호출을 기록하는 Form 호스트 대역이다. 현재 타입은 기본 3 이고 currentType 을 주면 Native 처럼 그 값이다.
function formsStub(options = {}) {
    const log = []; const forms = { pool: { edition: options.edition ?? 'patch1078' }, create: (flags) => { log.push(['create', flags]); return options.created ?? 77; }, setOwner: (...args) => log.push(['owner', ...args]), attach: (...args) => log.push(['attach', ...args]),
        destroy: (...args) => log.push(['destroy', ...args]), isGumpForm: (sid) => (options.gump ?? (() => true))(sid), log };
    if (options.currentType) forms.currentType = options.currentType; return forms;
}

test('projectGumpObject: 풀 슬롯의 +14/+18 float32 를 매번 읽어 카메라 투영 결과 {x,y} 를 돌려주고 판본/SID 범위를 먼저 확인한다', () => {
    const pool = poolWith(1.5, -2.25); const calls = []; const camera = { project: (x, y) => { calls.push([x, y]); return { x: 100, y: 200 }; } };
    assert.deepEqual(projectGumpObject(pool, camera, 12), { x: 100, y: 200 }); assert.deepEqual(calls, [[1.5, -2.25]]); assert.deepEqual(pool.requests, [12]);
    // float32 폭 그대로 읽는다(0.1 은 float32 값).
    const narrow = poolWith(0.1, 0.2); const seen = []; projectGumpObject(narrow, { project: (x, y) => { seen.push([x, y]); return { x: 0, y: 0 }; } }, 5); assert.deepEqual(seen, [[Math.fround(0.1), Math.fround(0.2)]]);
    // 0xffff 는 정상, 0x10000 은 표현 범위 밖(절단하지 않는다). 풀 판본이 다르면 거부한다.
    assert.doesNotThrow(() => projectGumpObject(pool, camera, 0xffff)); assert.throws(() => projectGumpObject(pool, camera, 0x10000), (error) => error instanceof RangeError && /SID 표현 범위 초과/.test(error.message)); assert.equal(pool.requests.at(-1), 0xffff);
    assert.throws(() => projectGumpObject(poolWith(0, 0, 'cd1072'), camera, 1), (error) => error instanceof TypeError && /10.78 객체 투영 필요/.test(error.message));
    // 투영 결과의 좌표 두 값만 새 객체로 돌려준다.
    const shared = { x: 1, y: 2, extra: 3 }; const result = projectGumpObject(pool, { project: () => shared }, 1); assert.deepEqual(result, { x: 1, y: 2 }); assert.notEqual(result, shared);
});

test('makeGumpFormNotice: 알림 시점의 등록 표에서 소유 UI 토큰을 읽어 현재 처리기에 (토큰, 객체 SID)를 전달한다', () => {
    const state = new GumpCreateState(); state.entries[5] = 99; const calls = []; const notice = makeGumpFormNotice(state, (token, object) => calls.push([token, object]));
    notice(5, 1234); assert.deepEqual(calls, [[99, 1234]]);
    // 생성 당시가 아니라 매 알림의 현재 등록 표를 읽는다.
    state.entries[5] = 55; notice(5, 7); assert.deepEqual(calls.at(-1), [55, 7]);
    // 범위: 마지막 칸은 정상, 표 크기 이상이나 비어 있는 등록은 거부한다.
    state.entries[state.entries.length - 1] = 3; assert.doesNotThrow(() => notice(state.entries.length - 1, 1)); assert.throws(() => notice(state.entries.length, 1), (error) => error instanceof RangeError && /등록 ID 범위 초과/.test(error.message));
    assert.throws(() => notice(6, 1), (error) => error.constructor === Error && /UI 등록이 없습니다/.test(error.message)); assert.equal(calls.length, 3);
    assert.throws(() => makeGumpFormNotice(state, null), (error) => error instanceof TypeError && /삭제 알림 경계 필요/.test(error.message));
});

test('releaseGumpObject: 부모 SID 를 쓰지 않고 현재 Gump 토큰을 자식 우선 Kill 로 요청한다', () => {
    const killed = []; releaseGumpObject({ kill: (token) => killed.push(token) }, 42, 9999); assert.deepEqual(killed, [42]);
});

test('bindMenuGumpForms: 현재 TYPE(독립 입력 3)을 메뉴 상태에 공급하고 Form 생성/소유자/객체 연결을 같은 풀에 연결한다', () => {
    const state = new MenuGumpWidgetState(); const hooks = {}; const forms = formsStub(); bindMenuGumpForms(new GumpCreateState(), state, hooks, forms);
    assert.equal(state.objectFormType, 3); assert.equal(state.currentObjectFormType(), 3); assert.equal(state.objectFormTypeNow(), 3);
    // Form(type 3, flags 2): 같은 값을 토큰/인덱스로 돌려준다. 다른 타입은 거부한다.
    assert.deepEqual(hooks.objectForm(3, 2), { token: 77, index: 77 }); assert.deepEqual(forms.log, [['create', 2]]);
    assert.throws(() => hooks.objectForm(4, 2), (error) => error instanceof TypeError && /현재 타입 오류/.test(error.message)); assert.equal(forms.log.length, 1);
    forms.log.length = 0; hooks.formOwner(77, 0x12345678); hooks.formObject(77, 88, 5); assert.deepEqual(forms.log, [['owner', 77, 0x12345678], ['attach', 77, 88, 5]]);
    // 객체/Form SID 는 원본 번호 범위를 확인한다(DWORD 전체 소유 ID 는 그대로).
    assert.throws(() => hooks.formOwner(0x10000, 1), /SID 표현 범위 초과/); assert.throws(() => hooks.formObject(1, 0x10000, 0), /SID 표현 범위 초과/); assert.throws(() => hooks.formObject(0x10000, 1, 0), /SID 표현 범위 초과/);
    hooks.formOwner(0xffff, 0xffffffff); assert.deepEqual(forms.log.at(-1), ['owner', 0xffff, 0xffffffff]);
});

test('bindMenuGumpForms(현재 TYPE): Native 는 현재 TYPE 조회를 쓰며 이미 설치된 메뉴 조회는 보존하고 판본이 다르면 거부한다', () => {
    let type = 3; const forms = formsStub({ currentType: () => type }); const state = new MenuGumpWidgetState(); const hooks = {}; bindMenuGumpForms(new GumpCreateState(), state, hooks, forms);
    // 사본은 연결 시점 값, 조회는 호출 때마다 현재 값을 읽는다.
    type = 6; assert.equal(state.objectFormType, 3); assert.equal(state.objectFormTypeNow(), 6); assert.throws(() => hooks.objectForm(3, 2), /현재 타입 오류/); assert.deepEqual(hooks.objectForm(6, 2), { token: 77, index: 77 });
    // 이미 실제 메뉴가 같은 현재 Gump TYPE 조회를 설치했으면 덮어쓰지 않는다.
    const installed = new MenuGumpWidgetState(); const mine = () => 1234; installed.currentObjectFormType = mine; bindMenuGumpForms(new GumpCreateState(), installed, {}, formsStub()); assert.equal(installed.currentObjectFormType, mine); assert.equal(installed.objectFormType, 3);
    assert.throws(() => bindMenuGumpForms(new GumpCreateState(), new MenuGumpWidgetState(), {}, formsStub({ edition: 'cd1072' })), (error) => error instanceof TypeError && /판본이 다릅니다/.test(error.message));
});

test('bindGumpFormLifetime: GumpForm 은 호스트 삭제로 분배하고 다른 Form 은 기존 삭제 경계에 보존하며 없으면 거부한다', () => {
    const calls = []; const forms = formsStub({ gump: (sid) => sid === 10 }); const hooks = { deleteForm: (form, flags) => calls.push(['previous', form, flags]) };
    bindGumpFormLifetime(hooks, forms); hooks.deleteForm(10, 0x10); hooks.deleteForm(11, 0x20); assert.deepEqual(forms.log, [['destroy', 10, 0x10]]); assert.deepEqual(calls, [['previous', 11, 0x20]]);
    // SID 범위 밖은 절단하지 않고 거부한다. 이전 경계가 없으면 다른 Form 에서만 거부한다.
    assert.throws(() => hooks.deleteForm(0x10000, 0), /SID 표현 범위 초과/); const bare = {}; bindGumpFormLifetime(bare, formsStub({ gump: (sid) => sid === 10 })); assert.doesNotThrow(() => bare.deleteForm(10, 0)); assert.throws(() => bare.deleteForm(3, 0), (error) => error instanceof TypeError && /UI Form 삭제 경계 필요/.test(error.message));
});
