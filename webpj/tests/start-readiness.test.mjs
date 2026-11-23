// '클릭하여 시작' 관문과 브라우저 언어 탐지를 검사한다. 처음 방문(캐시 없음)에서 아직 받는 중인 자원(메뉴 음악·글꼴)이 끝나기 전에 클릭을 받지 않고,
// 자원이 영영 끝나지 않아도 제한 시간 뒤에는 시작할 수 있으며, 언어는 브라우저의 선호 언어 목록 첫 항목으로 정한다. (2026-11-24 WB04-E1300)
import test from 'node:test';
import assert from 'node:assert/strict';
import { Netstorm, START_MIN_WAIT_MS, START_READY_TIMEOUT_MS, detectBrowserLanguage, normalizeLanguage, normalizeStartMinWaitMs } from '../www/netstorm.js';

// 직접 이행시킬 수 있는 약속이다.
function deferred() {
    let resolve;
    const promise = new Promise((done) => { resolve = done; });
    return { promise, resolve };
}

// 가짜 시계가 있는 앱이다: setTimeout/clearTimeout 은 목록에 쌓기만 하고 시험이 직접 만료시킨다. fonts 는 시험이 정한 약속이다.
function readinessApp({ fonts = null } = {}) {
    const app = new Netstorm(), timers = [];
    app.options.startMinWaitMs = 0; // 기본 시험은 고정 최소 대기를 끄고(자원 신호만 본다), 고정 최소 대기는 아래 전용 시험이 켠다. (2026-11-24 WB04-E1306)
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

test('고정 최소 대기 값: 비었거나 잘못된 값은 기본, 음수·비숫자는 기본, 큰 값은 60초로 제한한다', () => {
    assert.equal(START_MIN_WAIT_MS, 6000);
    assert.equal(normalizeStartMinWaitMs(undefined), 6000);
    assert.equal(normalizeStartMinWaitMs(null, 123), 123);
    assert.equal(normalizeStartMinWaitMs(''), 6000);
    assert.equal(normalizeStartMinWaitMs('0'), 0, '0 은 끄기');
    assert.equal(normalizeStartMinWaitMs('2500.9'), 2500);
    assert.equal(normalizeStartMinWaitMs('abc'), 6000);
    assert.equal(normalizeStartMinWaitMs('-5'), 6000);
    assert.equal(normalizeStartMinWaitMs(999999), 60000);
});

test('고정 최소 대기: 자원이 이미 다 와도 첫 적재 시작 뒤 6초가 지나기 전에는 열리지 않는다', async () => {
    const { app, timers } = readinessApp(); // 글꼴·음악 신호가 없어 자원은 이미 준비된 상태
    app.options.startMinWaitMs = START_MIN_WAIT_MS; app._loadStartedAt = Date.now() - 1000; // 1초 지남
    let done = false;
    const waiting = app._awaitStartReadiness().then(() => { done = true; });
    await new Promise((resolve) => setImmediate(resolve));
    assert.equal(done, false, '남은 약 5초 동안은 열리지 않는다');
    const floorTimer = timers.find((timer) => timer.milliseconds < START_READY_TIMEOUT_MS);
    assert.ok(floorTimer.milliseconds > 4000 && floorTimer.milliseconds <= 5000, `남은 시간 타이머 ${floorTimer.milliseconds}`);
    floorTimer.callback();
    await waiting;
    assert.equal(done, true, '남은 시간이 지나면 열린다');
    assert.ok(timers.every((timer) => timer.cleared), '끝나면 모든 타이머를 지운다');
});

test('고정 최소 대기: 이미 지났거나 0 이면 기다리지 않고, 상한은 최소 대기보다 짧아지지 않는다', async () => {
    const past = readinessApp();
    past.app.options.startMinWaitMs = START_MIN_WAIT_MS; past.app._loadStartedAt = Date.now() - 7000;
    await past.app._awaitStartReadiness();
    assert.equal(past.timers.length, 0, '6초가 지났으면 타이머조차 없다');
    const off = readinessApp();
    off.app._loadStartedAt = Date.now();
    await off.app._awaitStartReadiness();
    assert.equal(off.timers.length, 0, '0 이면 끄기');
    // 최소 대기가 상한(20초)보다 길게 설정되면 상한도 그만큼 늘려 최소 대기를 끊지 않는다.
    const long = readinessApp();
    long.app.options.startMinWaitMs = 30000; long.app._loadStartedAt = Date.now();
    const waiting = long.app._awaitStartReadiness();
    await new Promise((resolve) => setImmediate(resolve));
    assert.deepEqual(long.timers.map((timer) => timer.milliseconds).sort((a, b) => a - b).map((value) => value > 29000 && value <= 30000), [true, true], '최소 대기 타이머와 상한 타이머가 모두 30초');
    long.timers[1].callback(); // 상한 타이머 만료로 끝낸다
    await waiting;
});
