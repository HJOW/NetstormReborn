// Reset All 의 저장 범위·확인·실패·앱 수명 순서를 검증한다. (2026-11-19 WB04-E863)
import test from 'node:test';
import assert from 'node:assert/strict';
import { Netstorm } from '../www/netstorm.js';
import { Settings, SETTINGS_STORAGE_KEY } from '../www/platform/settings.js';
import { Progress, PROGRESS_STORAGE_KEY } from '../www/platform/progress.js';
import { resetGameStorage } from '../www/platform/reset-storage.js';
import { buildOptionsItems } from '../www/ui/option-menus.js';

// 다른 사이트 항목을 포함한 가짜 저장소이다.
function makeStorage() {
    const values = new Map([[SETTINGS_STORAGE_KEY, 'settings'], [PROGRESS_STORAGE_KEY, 'progress'], ['other.app', 'other']]);
    return {
        values,
        getItem: (key) => values.get(key) ?? null,
        setItem: (key, value) => values.set(key, String(value)),
        removeItem: (key) => values.delete(key),
    };
}

// 웹 메뉴의 마지막 항목을 실제 명령으로 연결하고 원본 비교 메뉴에는 추가하지 않는다.
test('Reset All 메뉴는 웹 메뉴 최하단에만 나타난다', () => {
    const settings = new Settings();
    const web = buildOptionsItems(settings);
    assert.deepEqual(web.at(-1).command, { name: 'ResetAll', argument: '0' });
    assert.equal(web.at(-1).label, 'Reset All');
    assert.equal(buildOptionsItems(settings, { includeAll: true }).some((item) => item.label === 'Reset All'), false);
});

// 지정한 두 항목만 지우며 게임을 새로 만들면 최초 실행 기본값과 빈 진행값이 된다.
test('게임 저장만 초기화하고 새 설정과 진행은 최초 상태다', () => {
    const storage = makeStorage();
    resetGameStorage(storage);
    assert.equal(storage.getItem('other.app'), 'other');
    assert.equal(storage.getItem(SETTINGS_STORAGE_KEY), null);
    assert.equal(storage.getItem(PROGRESS_STORAGE_KEY), null);
    assert.equal(new Settings({ storage }).get('welcomeGiven'), 0);
    assert.deepEqual(new Progress({ storage }).snapshot(), {});
});

// 두 번째 삭제 실패 시 앞서 지운 첫 항목을 복구하고 원래 오류를 알린다.
test('부분 삭제 실패는 기존 값을 복구하고 원인을 전달한다', () => {
    const storage = makeStorage();
    const error = new Error('삭제 거부');
    const remove = storage.removeItem;
    storage.removeItem = (key) => { if (key === PROGRESS_STORAGE_KEY) throw error; remove(key); };
    assert.throws(() => resetGameStorage(storage), (actual) => actual === error);
    assert.equal(storage.getItem(SETTINGS_STORAGE_KEY), 'settings');
    assert.equal(storage.getItem(PROGRESS_STORAGE_KEY), 'progress');
    assert.equal(storage.getItem('other.app'), 'other');
});

// 확인 취소는 저장소·앱 수명·새로고침에 영향을 주지 않는다.
test('초기화 취소는 아무 값도 바꾸지 않는다', () => {
    const storage = makeStorage();
    const actions = [];
    const view = { localStorage: storage, confirm: () => { actions.push('confirm'); return false; }, location: { reload: () => actions.push('reload') } };
    const app = new Netstorm();
    app._root = { ownerDocument: { defaultView: view } };
    app.destroy = () => actions.push('destroy');
    app._onPopupCommand({ name: 'ResetAll', argument: '0' });
    assert.deepEqual(actions, ['confirm']);
    assert.equal(storage.getItem(SETTINGS_STORAGE_KEY), 'settings');
    assert.equal(storage.getItem(PROGRESS_STORAGE_KEY), 'progress');
});

// 확인 후 저장 삭제 → 앱 정리 → 새로고침 순서를 지킨다.
test('초기화 확인은 저장 삭제 후 앱을 정리하고 새로고침한다', () => {
    const storage = makeStorage();
    const actions = [];
    const view = { localStorage: storage, confirm: (text) => { assert.match(text, /초기화/); actions.push('confirm'); return true; }, location: { reload: () => actions.push('reload') } };
    const app = new Netstorm({ language: 'korean' });
    app._root = { ownerDocument: { defaultView: view } };
    app.destroy = () => { assert.equal(storage.getItem(SETTINGS_STORAGE_KEY), null); assert.equal(storage.getItem(PROGRESS_STORAGE_KEY), null); actions.push('destroy'); };
    app._onPopupCommand({ name: 'ResetAll', argument: '0' });
    assert.deepEqual(actions, ['confirm', 'destroy', 'reload']);
    assert.equal(storage.getItem('other.app'), 'other');
});

// 저장소가 차단되면 오류를 표시하고 현재 앱을 종료하거나 새로고침하지 않는다.
test('저장소 접근 실패는 안내하고 앱을 계속 유지한다', () => {
    const actions = [];
    const view = { get localStorage() { throw new Error('차단'); }, confirm: () => true, alert: (message) => actions.push(message), location: { reload: () => actions.push('reload') } };
    const app = new Netstorm({ language: 'english' });
    app._root = { ownerDocument: { defaultView: view } };
    app.destroy = () => actions.push('destroy');
    const original = console.error;
    console.error = () => {};
    try { app._onPopupCommand({ name: 'ResetAll', argument: '0' }); } finally { console.error = original; }
    assert.equal(actions.length, 1);
    assert.match(actions[0], /Could not reset/);
});
