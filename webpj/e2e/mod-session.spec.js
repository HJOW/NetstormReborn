// 실제 비고정 1-1/아군 AI에서 자동 모드 사건·시계 장벽·원래 입력·오류 저장/미션 반납을 검증한다.
const { test, expect } = require('@playwright/test');
const { installAllyAi } = require('./ally-ai-test-control');
// 두 패키지는 같은 형식이며 새로 설치한 값 대신 명시 활성 저장 상태로 시작한다.
function packageText(id, source) { return JSON.stringify({ format: 'netstorm-mod-package', formatVersion: 1, manifest: { id, name: id, version: '1.0.0', apiVersion: '0.1.0', entry: 'main.js' }, modules: { 'main.js': source } }); }
// 정상 모드는 실제 입력 단계로 사제 선택 한 번만 요청하고 상태 조회를 로그에 남긴다.
const good = packageText('test.good', "export default function(api){let sent=false;api.on('sessionStart',()=>api.log('info','start:'+api.getPlayers().map(p=>p.owner).join(',')));api.on('tick',()=>{if(api.getState().paused)return;if(!sent){api.commands.selectPriest();sent=true;}else api.log('info','selection:'+JSON.stringify(api.getSelection()));});api.on('sessionEnd',e=>api.log('info','end:'+e.reason));}");
// 한 번의 실제 사건이 무한 반복한다. 호스트는 이 Worker만 종료해야 한다.
const bad = packageText('test.bad', "export default function(api){api.on('tick',()=>{if(!api.getState().paused){while(true){}}});}");

// 현재 Native 메뉴의 실제 좌표를 눌러 하위 메뉴나 Tell을 연다. 명령 분배 자체는 우회하지 않는다.
async function clickGameMenuRow(page, token, label) {
    const point = await page.evaluate(({ token, label }) => {
        const app = window.netstorm, root = app._clientGump, bar = app._campaignProduction.ui.gameMenu(), row = bar.labels(token).indexOf(label), node = root.node(token);
        if (row < 0) throw new Error('게임 메뉴 행 없음: ' + label);
        const bounds = root.collection().bounds(token), rect = app.canvas.getBoundingClientRect(), horizontal = (node.menuFlags & 0x40000) !== 0;
        return { child: node.items[row].childId ? root.createState().entries[node.items[row].childId] : 0,
            x: rect.left + (bounds.left + (horizontal ? node.itemWidth * (row + .5) : 20)) * rect.width / app._layout.logicalWidth,
            y: rect.top + (bounds.top + (horizontal ? (bounds.bottom - bounds.top) / 2 : node.offsets[row] + node.heights[row] / 2)) * rect.height / app._layout.logicalHeight };
    }, { token, label });
    await page.mouse.move(point.x, point.y); await page.mouse.click(point.x, point.y, { delay: 60 });
    if (point.child) await page.waitForFunction((child) => { const node = window.netstorm._clientGump.node(child); return node && (node.flags & 1) && !node.expanding; }, point.child);
    return point.child;
}

test('실제 1-1에서 모드 대기 중 Native 상태를 보존하고 다른 모드/이탈을 계속한다', async ({ page }) => {
    test.setTimeout(180000); const errors = [], logs = [];
    page.on('pageerror', (error) => errors.push(error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); if (message.text().startsWith('[mod:test.good]')) logs.push(message.text()); });
    await page.setViewportSize({ width: 1024, height: 768 });
    await page.addInitScript(({ good, bad }) => {
        if (window !== window.top) return;
        localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, autoDemo: 0, sound: 0, music: 0 } }));
        localStorage.setItem('netstorm.mods.v0', JSON.stringify({ version: 1, mods: [{ package: good, enabled: true, lastError: null }, { package: bad, enabled: true, lastError: null }] }));
    }, { good, bad });
    await page.goto('/debug.html?title=1&aspect=4:3&lang=english');
    await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 120000 }); await page.locator('canvas').click();
    await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 }); await page.evaluate(() => window.netstorm._prepareCampaignAudio());
    await installAllyAi(page, 'thewarbegins'); await page.evaluate(() => window.netstorm._beginCampaignFromDialog('TheWarBegins'));
    await page.waitForFunction(() => window.netstorm?._mainMenu?.controlPoint('Play Mission') && window.netstorm?._campaignWorld?.session().localConnectionReady(), null, { timeout: 60000 });
    // 브리핑에서도 Ready 조회/등록은 가능하나 모드 입력은 원래 모달 경계에서 차단한다.
    await page.waitForFunction(() => !window.netstorm._mods.pending); expect(logs.some((line) => line.endsWith('start:1,2'))).toBe(true);
    const point = await page.evaluate(() => { const app = window.netstorm, point = app._mainMenu.controlPoint('Play Mission'), rect = app.canvas.getBoundingClientRect(); return { x: rect.left + point.x * rect.width / app._layout.logicalWidth, y: rect.top + point.y * rect.height / app._layout.logicalHeight, game: app._gameClock.time().game }; });
    await page.mouse.click(point.x, point.y);
    await page.waitForFunction((start) => window.netstorm._mods.pending && !window.netstorm._gameClock.paused() && window.netstorm._gameClock.time().game > start + .7, point.game, { polling: 5, timeout: 20000 });
    const stable = await page.evaluate(async () => {
        const app = window.netstorm, pool = app._campaignWorld.session().core().pool;
        // 대기 중 두 표본의 전체 Native 풀/RNG/고정 프레임과 live game/wall 값을 비교한다.
        const capture = () => { let hash = 2166136261; for (const value of pool.bytes()) hash = Math.imul(hash ^ value, 16777619) >>> 0; return { hash, frame: { ...app._gameClock.time() }, game: app._gameClock.gameSeconds(), wall: app._gameClock.wallSeconds(), rng: [app._clientRandom.game.state, app._clientRandom.crt.state] }; };
        const before = capture(), pendingBefore = app._mods.pending; let heartbeat = false;
        await new Promise((resolve) => setTimeout(() => { heartbeat = true; resolve(); }, 250));
        return { before, after: capture(), heartbeat, pendingBefore, pendingAfter: app._mods.pending, ai: window.netstormAllyTestState };
    });
    expect(stable.pendingBefore).toBe(true); expect(stable.pendingAfter).toBe(true); expect(stable.heartbeat).toBe(true); expect(stable.after).toEqual(stable.before); expect(stable.ai.brain).toBeGreaterThan(0);
    await page.waitForFunction(() => { const app = window.netstorm, item = app._mods.store.list().find((item) => item.manifest.id === 'test.bad'); return !app._mods.pending && item?.enabled === false && item.lastError?.includes('timeLimit'); }, null, { timeout: 10000 });
    await page.waitForFunction(() => { const app = window.netstorm, world = app._campaignWorld; return world.selected() === world.session().findPriestParticipation(world.session().localOwner()); }, null, { timeout: 10000 });
    await expect.poll(() => logs.some((line) => line.includes('selection:{'))).toBe(true);
    const finished = await page.evaluate(() => { const app = window.netstorm; return { mods: app._mods.store.list(), frame: { ...app._gameClock.time() }, selected: app._campaignWorld.selected() }; });
    expect(finished.mods.find((item) => item.manifest.id === 'test.good').enabled).toBe(true); expect(finished.frame.number).toBeGreaterThan(stable.after.frame.number);
    // 실제 미션에서 DOM 관리가 원래 정지 한 단계만 빌리고 변경은 다음 미션용으로 저장되는지 확인한다.
    await page.locator('[data-netstorm="game-menu-button"]').click(); await page.waitForFunction(() => window.netstorm._campaignProduction.ui.gameMenu().isOpen());
    const barToken = await page.evaluate(() => window.netstorm._campaignProduction.ui.gameMenu().token()), optionsToken = await clickGameMenuRow(page, barToken, 'Options');
    await clickGameMenuRow(page, optionsToken, 'Mods'); let panel = page.getByRole('dialog', { name: 'Mods', exact: true }); await expect(panel).toBeVisible();
    expect(await page.evaluate(() => window.netstorm._gameClock.paused())).toBe(true); await expect(panel.getByText(/timeLimit/)).toBeVisible();
    await panel.getByRole('checkbox', { name: 'Enabled: test.good', exact: true }).uncheck(); await expect.poll(() => page.evaluate(() => window.netstorm._mods.store.list().find((item) => item.manifest.id === 'test.good').enabled)).toBe(false);
    const priorLogs = logs.filter((line) => line.includes('selection:')).length; await page.keyboard.press('Escape'); await expect(panel).toHaveCount(0);
    await expect.poll(() => logs.filter((line) => line.includes('selection:')).length).toBeGreaterThan(priorLogs);
    await page.evaluate(() => window.netstorm._openTell('Patch1101.Mods')); panel = page.getByRole('dialog', { name: 'Mods', exact: true }); await expect(panel).toBeVisible();
    await page.evaluate(() => window.netstorm._leaveCampaignMission());
    await page.waitForFunction(() => !window.netstorm._mods.pending && !window.netstorm._campaignWorld && document.querySelectorAll('iframe[sandbox]').length === 0);
    await expect(panel).toHaveCount(0); expect(await page.evaluate(() => window.netstorm._gameClock.paused())).toBe(false);
    await test.info().attach('actual-mod-session', { body: JSON.stringify({ stable, finished, logs, errors }), contentType: 'application/json' });
    expect(logs.some((line) => line.endsWith('end:leave'))).toBe(true); expect(errors).toEqual([]);
});

test('고정 오라클에서는 저장된 모드 검증/실행을 강제로 끈다', async ({ page }) => {
    const errors = []; page.on('pageerror', (error) => errors.push(error.message));
    await page.addInitScript(() => { if (window === window.top) localStorage.setItem('netstorm.mods.v0', '의도적으로 손상된 저장소'); });
    await page.goto('/index.html?fixed=25&title=1'); await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 120000 });
    const result = await page.evaluate(() => ({ pending: window.netstorm._mods.pending, error: window.netstorm._mods.error, list: window.netstorm._mods.store.list(), frames: document.querySelectorAll('iframe[sandbox]').length, raw: localStorage.getItem('netstorm.mods.v0') }));
    expect(result).toEqual({ pending: false, error: null, list: [], frames: 0, raw: '의도적으로 손상된 저장소' }); expect(errors).toEqual([]);
});
