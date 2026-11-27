// 새 모드 호스트를 실제 아군 AI 1-1의 같은 Session/큐에 연결한다. 격리 로더/UI 완료로 세지 않는다.
const { test, expect } = require('@playwright/test');
const { installAllyAi } = require('./ally-ai-test-control');

// 원래 프레임만 진행하고 비동기 파일 적재에 실행 기회를 준다.
async function step(page, frames = 1) { await page.evaluate((count) => window.netstorm.stepFixedFrames(count), frames); }
// 실제 메뉴가 준 논리 좌표를 브라우저 입력으로 누른다.
async function clickLabel(page, label) {
    const point = await page.evaluate((text) => { const app = window.netstorm, point = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect(); if (!point) throw new Error('실제 단추 없음: ' + text); return { x: rect.left + point.x * rect.width / app._layout.logicalWidth, y: rect.top + point.y * rect.height / app._layout.logicalHeight }; }, label);
    await page.mouse.click(point.x, point.y); await step(page, 2);
}

test('공개 조회는 RNG/풀을 바꾸지 않고 실제 입력 큐가 사제 선택을 처리한다', async ({ page }) => {
    test.setTimeout(240000); const errors = [];
    page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
    await page.setViewportSize({ width: 1024, height: 768 });
    await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, autoDemo: 0, sound: 0, music: 0 } })));
    await page.goto('/debug.html?fixed=25&title=1&aspect=4:3&lang=english');
    await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 120000 }); await page.locator('canvas').click(); await page.evaluate(() => window.netstorm._prepareCampaignAudio());
    // 실제 시작 그림을 정상 프레임으로 끝낸다.
    for (let frame = 0; frame < 1200 && await page.evaluate(() => window.netstorm.state !== 'mainMenu'); frame += 20) await step(page, 20);
    expect(await page.evaluate(() => window.netstorm.state)).toBe('mainMenu'); await step(page);
    await installAllyAi(page, 'thewarbegins'); await page.evaluate(() => window.netstorm._beginCampaignFromDialog('TheWarBegins'));
    // 브리핑을 넘기기 전의 모달 거부도 같은 실제 호스트로 검사한다.
    for (let frame = 0; frame < 1500; frame += 5) { if (await page.evaluate(() => Boolean(window.netstorm._mainMenu.controlPoint('Play Mission')) && Boolean(window.netstorm._campaignWorld?.session().localConnectionReady()))) break; await step(page, 5); }
    const before = await page.evaluate(async () => {
        const { ModGameHost } = await import('./modding/mod-game-host.js'), app = window.netstorm;
        window.netstormModHostTest = new ModGameHost(app); return { blocked: window.netstormModHostTest.inputBlocked(), snapshot: window.netstormModHostTest.snapshot() };
    }); expect(before.blocked).toBe(true);
    await clickLabel(page, 'Play Mission'); await step(page, 4);
    const actual = await page.evaluate(async () => {
        const { ModRuntime } = await import('./modding/mod-api.js'), app = window.netstorm, host = window.netstormModHostTest, session = app._campaignWorld.session(), pool = session.core().pool;
        // 동기 조회 전후의 같은 바이트/RNG/시각을 비교한다.
        const before = { bytes: Array.from(pool.bytes()), game: app._clientRandom.game.state, crt: app._clientRandom.crt.state, frame: { ...app._gameClock.time() } };
        const snapshot = host.snapshot(); snapshot.players[0].stormPower = 0; const clean = host.snapshot();
        const after = pool.bytes(), unchanged = before.game === app._clientRandom.game.state && before.crt === app._clientRandom.crt.state && JSON.stringify(before.frame) === JSON.stringify(app._gameClock.time()) && before.bytes.every((value, index) => value === after[index]);
        const runtime = new ModRuntime({ modId: 'test.actual-host', sessionSeed: host.sessionSeed() }); runtime.register((api) => api.on('sessionStart', () => api.commands.selectPriest()));
        const output = runtime.dispatch({ name: 'sessionStart', sequence: 1, sessionId: clean.state.sessionId, mission: clean.state.mission, gameSeconds: clean.state.gameSeconds }, clean), queueSize = app._clientInputQueue.size;
        const result = host.commit(output, { eventSequence: 1, sessionId: clean.state.sessionId });
        return { unchanged, snapshot: clean, result, beforeQueue: queueSize, afterQueue: app._clientInputQueue.size, activation: window.netstormAllyTestState };
    });
    expect(actual.unchanged).toBe(true); expect(actual.activation.brain).toBeTruthy(); expect(actual.result[0].status).toBe('queued'); expect(actual.afterQueue - actual.beforeQueue).toBe(2); expect(actual.snapshot.state.mission.toLowerCase()).toBe('thewarbegins'); expect(actual.snapshot.players.length).toBeGreaterThan(0);
    await step(page, 2);
    const selected = await page.evaluate(() => { const world = window.netstorm._campaignWorld, session = world.session(), priest = session.findPriestParticipation(session.localOwner()); return { raw: world.selected(), priest, expectedType: session.core().pool.slot(priest)[10], public: window.netstormModHostTest.snapshot().selection }; });
    expect(selected.raw).toBeGreaterThan(0); expect(selected.raw).toBe(selected.priest); expect(selected.public.typeId).toBe(selected.expectedType); expect(Object.keys(selected.public).sort()).toEqual(['health', 'owner', 'typeId']);
    await page.evaluate(() => { window.netstormModHostTest.dispose(); delete window.netstormModHostTest; }); expect(errors).toEqual([]);
    await test.info().attach('mod-game-host', { body: JSON.stringify({ before, actual, selected, errors }), contentType: 'application/json' });
});
