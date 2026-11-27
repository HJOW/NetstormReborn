// Reset All 의 확인 창(게임 안 Tell)·확정 가드·저장 범위·실패·앱 수명 순서를 검증한다. (2026-11-19 WB04-E863, 2026-11-23 WB04-E1251: 브라우저 confirm() 을 게임 안 확인 창으로 교체)
import test from 'node:test';
import assert from 'node:assert/strict';
import { Netstorm } from '../www/netstorm.js';
import { Settings, SETTINGS_STORAGE_KEY } from '../www/platform/settings.js';
import { Progress, PROGRESS_STORAGE_KEY } from '../www/platform/progress.js';
import { resetGameStorage } from '../www/platform/reset-storage.js';
import { MOD_STORAGE_KEY } from '../www/modding/mod-store.js';
import { buildOptionsItems } from '../www/ui/option-menus.js';
import { PRODUCT_RESET_ALL_CONFIRMED_TARGET, PRODUCT_RESET_ALL_TARGET, productCreditsText } from '../www/assets/product.js';

// 다른 사이트 항목을 포함한 가짜 저장소이다.
function makeStorage() {
    const values = new Map([[SETTINGS_STORAGE_KEY, 'settings'], [PROGRESS_STORAGE_KEY, 'progress'], [MOD_STORAGE_KEY, 'mods'], ['other.app', 'other']]);
    return {
        values,
        getItem: (key) => values.get(key) ?? null,
        setItem: (key, value) => values.set(key, String(value)),
        removeItem: (key) => values.delete(key),
    };
}

// 가짜 view 와 앱을 만든다. confirm() 이 불리면 시험이 실패한다(게임 안 창만 쓴다). actions 에 destroy·reload·alert 순서를 기록한다.
function makeApp({ language = 'english', storage = makeStorage(), view = null } = {}) {
    const actions = [];
    const fake = view ?? { localStorage: storage, confirm: () => { throw new Error('브라우저 confirm() 을 쓰면 안 된다'); }, alert: (message) => actions.push(`alert:${message}`), location: { reload: () => actions.push('reload') } };
    const app = new Netstorm({ language });
    app._root = { ownerDocument: { defaultView: fake } };
    app.destroy = () => actions.push('destroy');
    return { app, actions, storage };
}

// 웹 메뉴의 마지막 항목은 초기화를 직접 실행하지 않고 게임 안 확인 창(Tell)을 열며 원본 비교 메뉴에는 추가하지 않는다.
test('Reset All 메뉴는 웹 메뉴 최하단에서 확인 창 Tell 을 연다', () => {
    const settings = new Settings();
    const web = buildOptionsItems(settings);
    assert.deepEqual(web.at(-1).command, { name: 'Tell', argument: PRODUCT_RESET_ALL_TARGET });
    assert.equal(PRODUCT_RESET_ALL_TARGET, 'Patch1101.ResetAll');
    assert.equal(web.at(-1).label, 'Reset All');
    assert.equal(buildOptionsItems(settings, { includeAll: true }).some((item) => item.label === 'Reset All'), false);
});

// 확인 창 섹션은 제품 설정 객체(Patch1101)에 있고 확정·취소 단추를 가진다(cpppj ProductPresentation 의 [ResetAll] 과 같은 구성).
test('Reset All 확인 창 섹션은 영어·한국어 본문과 확정·취소 단추를 가진다', () => {
    const english = productCreditsText('english'), korean = productCreditsText('korean');
    for (const text of [english, korean]) {
        assert.ok(text.includes('\n[ResetAll]\n'), '[ResetAll] 섹션');
        assert.ok(text.includes(`,Tell,${PRODUCT_RESET_ALL_CONFIRMED_TARGET}\n`), '확정 단추 대상');
        assert.ok(text.includes(',Tell,Blank\n'), '취소 단추 대상');
    }
    assert.match(english, /<h2>Reset All<\/h2>\n<p>Reset all settings and campaign progress\?<br>\nThe game will reload after resetting\.\n\$Button=Reset All,Tell,Patch1101\.ResetAllConfirmed\n\$Button=Cancel,Tell,Blank\n/u);
    assert.match(korean, /<h2>설정 초기화<\/h2>\n<p>모든 설정과 캠페인 진행 상황을 초기화하시겠습니까\?<br>\n?초기화 후 게임을 다시 불러옵니다\.\n\$Button=초기화,Tell,Patch1101\.ResetAllConfirmed\n\$Button=취소,Tell,Blank\n/u);
});

// 지정한 두 항목만 지우며 게임을 새로 만들면 최초 실행 기본값과 빈 진행값이 된다.
test('게임 저장만 초기화하고 새 설정과 진행은 최초 상태다', () => {
    const storage = makeStorage();
    resetGameStorage(storage);
    assert.equal(storage.getItem('other.app'), 'other');
    assert.equal(storage.getItem(SETTINGS_STORAGE_KEY), null);
    assert.equal(storage.getItem(PROGRESS_STORAGE_KEY), null);
    assert.equal(storage.getItem(MOD_STORAGE_KEY), null);
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

test('모드 삭제 실패도 앞서 지운 설정/진행을 복원한다', () => {
    const storage = makeStorage(), remove = storage.removeItem;
    storage.removeItem = (key) => { if (key === MOD_STORAGE_KEY) throw new Error('모드 삭제 거부'); remove(key); };
    assert.throws(() => resetGameStorage(storage), /모드 삭제/u);
    assert.equal(storage.getItem(SETTINGS_STORAGE_KEY), 'settings'); assert.equal(storage.getItem(PROGRESS_STORAGE_KEY), 'progress'); assert.equal(storage.getItem(MOD_STORAGE_KEY), 'mods'); assert.equal(storage.getItem('other.app'), 'other');
});

// 확인 창을 거치지 않은 확정은 아무 값도 바꾸지 않는다(다른 Tell·링크가 같은 이름을 보내도 무시).
test('확인 창이 열리지 않았으면 확정 대상은 무시된다', () => {
    const { app, actions, storage } = makeApp();
    const originalWarn = console.warn; console.warn = () => {};
    try { assert.equal(app._openTell(PRODUCT_RESET_ALL_CONFIRMED_TARGET), false); } finally { console.warn = originalWarn; }
    assert.deepEqual(actions, []);
    assert.equal(storage.getItem(SETTINGS_STORAGE_KEY), 'settings');
    assert.equal(storage.getItem(PROGRESS_STORAGE_KEY), 'progress');
});

// 확인 창이 열린 직후의 확정은 저장 삭제 → 앱 정리 → 새로고침 순서이며 한 번만 효력이 있다.
test('확정은 저장 삭제 후 앱을 정리하고 새로고침하며 한 번만 효력이 있다', () => {
    const { app, actions, storage } = makeApp({ language: 'korean' });
    app.destroy = () => { assert.equal(storage.getItem(SETTINGS_STORAGE_KEY), null); assert.equal(storage.getItem(PROGRESS_STORAGE_KEY), null); actions.push('destroy'); };
    app._resetAllArmed = true; // 확인 창(Tell Patch1101.ResetAll)이 실제로 열렸을 때 앱이 세우는 표시이다(e2e 가 실제 창으로 확인).
    assert.equal(app._openTell(PRODUCT_RESET_ALL_CONFIRMED_TARGET), true);
    assert.deepEqual(actions, ['destroy', 'reload']);
    assert.equal(storage.getItem('other.app'), 'other');
    // 같은 확정을 다시 보내도 무시된다.
    const originalWarn = console.warn; console.warn = () => {};
    try { assert.equal(app._openTell(PRODUCT_RESET_ALL_CONFIRMED_TARGET), false); } finally { console.warn = originalWarn; }
    assert.deepEqual(actions, ['destroy', 'reload']);
});

// 확인 창이 열린 뒤 다른 Tell(취소의 Blank 포함)이 오면 무장이 풀려 이전 창의 확정이 되살아나지 않는다.
test('다른 Tell 이 오면 확정 가드가 풀린다', () => {
    const { app, actions, storage } = makeApp();
    app._resetAllArmed = true;
    app._openTell('Blank'); // 취소 단추의 대상이다(메뉴·게임 데이터가 없는 시험 앱이라 창은 열리지 않는다).
    assert.equal(app._resetAllArmed, false);
    const originalWarn = console.warn; console.warn = () => {};
    try { assert.equal(app._openTell(PRODUCT_RESET_ALL_CONFIRMED_TARGET), false); } finally { console.warn = originalWarn; }
    assert.deepEqual(actions, []);
    assert.equal(storage.getItem(SETTINGS_STORAGE_KEY), 'settings');
});

// 저장소가 차단되면 오류를 표시하고 현재 앱을 종료하거나 새로고침하지 않는다.
test('저장소 접근 실패는 안내하고 앱을 계속 유지한다', () => {
    const actions = [];
    const view = { get localStorage() { throw new Error('차단'); }, confirm: () => { throw new Error('confirm 금지'); }, alert: (message) => actions.push(message), location: { reload: () => actions.push('reload') } };
    const { app } = makeApp({ view });
    app.destroy = () => actions.push('destroy');
    app._resetAllArmed = true;
    const original = console.error;
    console.error = () => {};
    try { assert.equal(app._openTell(PRODUCT_RESET_ALL_CONFIRMED_TARGET), true); } finally { console.error = original; }
    assert.equal(actions.length, 1);
    assert.match(actions[0], /Could not reset/);
});
