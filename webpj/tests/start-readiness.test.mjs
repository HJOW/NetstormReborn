// '클릭하여 시작' 관문과 브라우저 언어 탐지를 검사한다. 처음 방문(캐시 없음)에서 아직 받는 중인 자원(메뉴 음악·글꼴)이 끝나기 전에 클릭을 받지 않고,
// 자원이 영영 끝나지 않아도 제한 시간 뒤에는 시작할 수 있으며, 언어는 브라우저의 선호 언어 목록 첫 항목으로 정한다. (2026-11-24 WB04-E1300)
import test from 'node:test';
import assert from 'node:assert/strict';
import { Netstorm, START_READY_TIMEOUT_MS, detectBrowserLanguage, normalizeLanguage } from '../www/netstorm.js';

// 직접 이행시킬 수 있는 약속이다.
function deferred() {
    let resolve;
    const promise = new Promise((done) => { resolve = done; });
    return { promise, resolve };
}

// 가짜 시계가 있는 앱이다: setTimeout/clearTimeout 은 목록에 쌓기만 하고 시험이 직접 만료시킨다. fonts 는 시험이 정한 약속이다.
function readinessApp({ fonts = null } = {}) {
    const app = new Netstorm(), timers = [];
    const view = { setTimeout: (callback, milliseconds) => { timers.push({ callback, milliseconds, cleared: false }); return timers.length - 1; }, clearTimeout: (id) => { timers[id].cleared = true; } };
    app._root = { ownerDocument: { fonts: fonts ? { ready: fonts } : undefined, defaultView: view } };
    return { app, timers };
}

test('언어 탐지: navigator.languages 첫 항목을 우선하고 없으면 language 로 대체한다', () => {
    assert.equal(normalizeLanguage(detectBrowserLanguage({ languages: ['ko-KR', 'en-US'], language: 'en-US' })), 'korean', '선호 첫 항목이 한국어');
    assert.equal(normalizeLanguage(detectBrowserLanguage({ languages: ['en-US', 'ko'], language: 'ko' })), 'english', '선호 첫 항목이 영어');
    assert.equal(detectBrowserLanguage({ languages: [], language: 'ko' }), 'ko', '목록이 비면 language');
    assert.equal(detectBrowserLanguage({ language: 'ko-KR' }), 'ko-KR');
    assert.equal(detectBrowserLanguage({ userLanguage: 'ko' }), 'ko');
    assert.equal(detectBrowserLanguage({}), undefined);
    assert.equal(detectBrowserLanguage(null), undefined);
    assert.equal(normalizeLanguage(detectBrowserLanguage({})), 'english', '탐지하지 못하면 영어');
});

test('시작 관문: 글꼴과 메뉴 음악 내려받기가 모두 끝날 때까지 기다린다', async () => {
    const fonts = deferred(), music = deferred(), { app, timers } = readinessApp({ fonts: fonts.promise });
    app._musicStatus = 'loading'; app._musicSettled = music.promise;
    let done = false;
    const waiting = app._awaitStartReadiness().then(() => { done = true; });
    await new Promise((resolve) => setImmediate(resolve));
    assert.equal(done, false, '둘 다 끝나기 전에는 열리지 않는다');
    assert.equal(timers[0].milliseconds, START_READY_TIMEOUT_MS, '제한 시간 타이머');
    fonts.resolve();
    await new Promise((resolve) => setImmediate(resolve));
    assert.equal(done, false, '글꼴만 끝나서는 열리지 않는다');
    music.resolve();
    await waiting;
    assert.equal(done, true);
    assert.equal(timers[0].cleared, true, '끝나면 제한 시간 타이머를 지운다');
});

test('시작 관문: 자원이 끝나지 않아도 제한 시간이 지나면 열리고 기다릴 것이 없으면 바로 열린다', async () => {
    const stuck = new Promise(() => {}), { app, timers } = readinessApp({ fonts: stuck });
    app._musicStatus = 'loading'; app._musicSettled = stuck;
    let done = false;
    const waiting = app._awaitStartReadiness().then(() => { done = true; });
    await new Promise((resolve) => setImmediate(resolve));
    assert.equal(done, false);
    timers[0].callback();
    await waiting;
    assert.equal(done, true, '제한 시간 만료로 열림');
    // 음악이 이미 끝났거나(상태가 loading 이 아님) 글꼴 장치가 없으면 기다릴 것이 없다.
    const idle = readinessApp();
    idle.app._musicStatus = 'ready'; idle.app._musicSettled = stuck;
    await idle.app._awaitStartReadiness();
    assert.equal(idle.timers.length, 0, '타이머조차 만들지 않는다');
});

test('시작 관문: 글꼴 준비가 거부돼도 음악이 실패(약속은 이행)해도 열린다', async () => {
    const { app } = readinessApp({ fonts: Promise.reject(new Error('fonts')) });
    app._musicStatus = 'loading'; app._musicSettled = Promise.resolve();
    await app._awaitStartReadiness();
});
