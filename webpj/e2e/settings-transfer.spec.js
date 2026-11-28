// 실제 메뉴·클립보드·JSON 입력과 같은 문서의 앱 재초기화를 확인한다. 목표 수행/승리 입력은 없다.
const { test, expect } = require('@playwright/test');

// 화면에 그려진 메뉴 글자의 논리 좌표를 실제 캔버스 위치로 바꿔 클릭한다.
async function clickLabel(page, label) {
    await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 90000 });
    const point = await page.evaluate((text) => { const app = window.netstorm, point = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect(); return { x: rect.left + point.x * rect.width / app._layout.logicalWidth, y: rect.top + point.y * rect.height / app._layout.logicalHeight }; }, label); // 현재 표시 배율을 반영한 좌표다.
    await page.mouse.click(point.x, point.y);
}

// 첫 안내/자동 데모/음향 없이 실제 영어 메인 메뉴에 진입한다.
async function start(page) {
    await page.addInitScript(() => { localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, autoDemo: 0, difficulty: 1 } })); localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: { DoneTheWarBegins: '1' } })); localStorage.setItem('other-app-test', 'keep'); });
    await page.goto('/?title=0&lang=en&aspect=4:3');
    await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 120000 }); await page.locator('canvas').click();
    await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 });
}

test('설정 복사와 불러오기: 실제 클립보드·취소·오류 보존·한국어 메인 메뉴 재시작', async ({ page, context }) => {
    test.setTimeout(240000);
    const errors = []; page.on('pageerror', (error) => errors.push(error.message)); // 처리되지 않은 게임 예외만 모은다.
    await context.grantPermissions(['clipboard-read', 'clipboard-write']); await page.setViewportSize({ width: 1024, height: 768 }); await start(page);
    await clickLabel(page, 'Options'); await clickLabel(page, 'Copy Settings');
    let panel = page.getByRole('dialog', { name: 'Copy Settings', exact: true }); // 실제 게임 루트의 복사 창이다.
    await expect(panel).toBeVisible(); await expect(panel.getByRole('status')).toHaveText('Copied to clipboard.');
    const backup = JSON.parse(await page.evaluate(() => navigator.clipboard.readText())); // 사용자가 받을 실제 클립보드 JSON이다.
    expect(backup).toMatchObject({ format: 'netstorm-settings', version: 1, settings: { difficulty: 1 }, progress: { DoneTheWarBegins: '1' } });
    await panel.getByRole('button', { name: 'Close', exact: true }).click();
    const before = await page.evaluate(() => [localStorage.getItem('netstorm.settings'), localStorage.getItem('netstorm.progress')]); // 취소/잘못된 JSON 전 원문이다.
    await clickLabel(page, 'Options'); await clickLabel(page, 'Load Settings'); panel = page.getByRole('dialog', { name: 'Load Settings', exact: true });
    await panel.getByRole('button', { name: 'Apply', exact: true }).click(); await expect(panel).toBeHidden();
    expect(await page.evaluate(() => [localStorage.getItem('netstorm.settings'), localStorage.getItem('netstorm.progress')])).toEqual(before);
    await clickLabel(page, 'Options'); await clickLabel(page, 'Load Settings'); panel = page.getByRole('dialog', { name: 'Load Settings', exact: true });
    await panel.getByRole('textbox').fill('{wrong'); await panel.getByRole('button', { name: 'Apply', exact: true }).click();
    await expect(panel.getByRole('status')).toContainText('Could not load settings');
    expect(await page.evaluate(() => [localStorage.getItem('netstorm.settings'), localStorage.getItem('netstorm.progress')])).toEqual(before);
    backup.settings.language = 2; backup.settings.soundVolume = 2; backup.settings.difficulty = 3; backup.progress = { DoneTrainingDay: '1' };
    await panel.getByRole('textbox').fill(JSON.stringify(backup)); await panel.getByRole('button', { name: 'Apply', exact: true }).click();
    await page.waitForFunction(() => window.netstorm?.state === 'mainMenu' && window.netstorm._gameData?.language === 'korean', null, { timeout: 120000 });
    expect(await page.evaluate(() => ({ settings: window.netstorm._settings.snapshot(), progress: window.netstorm._progress.snapshot(), foreign: localStorage.getItem('other-app-test'), dialog: Boolean(window.netstorm._mainMenu.dialog), guide: window.netstorm._clientGuide.current() }))).toMatchObject({ settings: { language: 2, soundVolume: 2, difficulty: 3 }, progress: { DoneTrainingDay: '1' }, foreign: 'keep', dialog: false, guide: null });
    expect(await page.evaluate(() => window.netstorm._progress.get('DoneTheWarBegins') ?? null)).toBeNull();
    await clickLabel(page, 'Options'); await clickLabel(page, '설정 복사'); await expect(page.getByRole('dialog', { name: '설정 복사', exact: true })).toBeVisible();
    expect(errors).toEqual([]);
});

test('튜토리얼6 실제 안내 가격과 미션 난이도 변경 차단', async ({ page }) => {
    test.setTimeout(240000); await page.setViewportSize({ width: 1024, height: 768 }); await start(page);
    await page.evaluate(() => window.netstorm._beginCampaignFromDialog('Tutorial6'));
    await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
    const result = await page.evaluate(() => {
        const app = window.netstorm, original = app._gameData.substitute.bind(app._gameData); let shown = ''; // 실제 HTML 공장으로 전달된 치환 결과다.
        app._gameData.substitute = (source) => { const value = original(source); if (/crossbowcost/iu.test(source)) shown = value; return value; };
        try { app._openTell('D.'); } finally { app._gameData.substitute = original; }
        const cost = Math.trunc(app._campaignWorld.session().core().types[app._gameAssets.typeTable.find('windArcher')].cost), before = app._settings.get('difficulty'); // 현재 실제 건설 비용/난이도다.
        app._openTell('Patch1101.SetDifficulty(3)');
        return { shown, cost, before, after: app._settings.get('difficulty'), rows: app._campaignGameMenuDevices(app._campaignProduction).difficulties() };
    });
    expect(result.shown).toContain(`${result.cost} Storm Power`); expect(result.shown).not.toMatch(/crossbowcost/iu); expect(result.after).toBe(result.before); expect(result.rows).toEqual([]);
});
