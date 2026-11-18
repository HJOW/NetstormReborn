// Client 입력 장치가 같은 World에 전달하는 함수와 지연 오류를 확인한다.
import test from 'node:test';
import assert from 'node:assert/strict';
import { bindCampaignUserInputServices } from '../www/core/campaign-user-input-services.js';

// 설치 후 변경된 정지 상태와 현재 Session·화면 장치를 호출 시점에 사용한다.
test('캠페인 Client 입력: 선택·강조·운반 장치 연결', () => {
    const calls = []; let paused = false, installed, session = { sendPlayerTell: (...args) => calls.push(['remote', ...args]), showSacrificeMenu: (...args) => calls.push(['sacrifice', ...args]) };
    const client = { paused: () => paused, traceObject: (sid) => calls.push(['trace', sid]), drawNativePathMarker: (...args) => calls.push(['marker', ...args]), presentNativeCursorClear: () => calls.push('present') };
    const world = { session: () => session, bindUserInput: (...args) => { installed = args; } }, cursor = (value) => calls.push(['cursor', value]), tell = (text) => calls.push(['tell', text]);
    bindCampaignUserInputServices(client, world, cursor, tell); const [selection, highlight, carrier] = installed;
    assert.equal(selection.paused(), 0); paused = true; assert.equal(selection.paused(), 1); selection.dump(9);
    highlight.selectCursor(4); carrier.selectCursor(5); carrier.earlyLocal(); carrier.earlyRemote(2); carrier.showSacrifice(3); carrier.marker({ x: 7, y: 8 }, 9); carrier.refreshCursor();
    assert.deepEqual(calls, [['trace', 9], ['cursor', 4], ['cursor', 5], ['tell', 'EarlySacrifice'], ['remote', 2, 'EarlySacrifice'], ['sacrifice', 3, tell], ['marker', 7, 8, 9], 'present']);
    session = { sendPlayerTell: (...args) => calls.push(['new', ...args]) }; carrier.earlyRemote(4); assert.deepEqual(calls.at(-1), ['new', 4, 'EarlySacrifice']);
    assert.throws(() => carrier.showSacrifice(3), /메뉴 부모 미연결/); assert.throws(() => selection.assertion(), /selectedSid/);
    assert.throws(() => selection.previewAssertion(), /isForm/); assert.throws(() => carrier.assertion(17), /17/);
    bindCampaignUserInputServices(client, world, cursor, null); assert.throws(() => installed[2].earlyLocal(), /Tell 미연결/);
});
