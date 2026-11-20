// 파생 객체 메뉴(신전의 View Netstorm Knowledge·Salvage·About·Player, 배터리의 Meltdown! 등)가 우클릭 열기로 실제로 열리는지 실제 Chromium 에서 확인한다. (2026-11-20 WB04-E1127)
// 연결 전에는 신전·배터리·발전기를 우클릭해도 아무 메뉴가 열리지 않았다(원본 스크린샷 'Rain Temple Context Menu' 와 다름).
const { test, expect } = require('@playwright/test');

// 실제 돌 버튼의 논리 좌표를 브라우저 마우스 좌표로 바꿔 누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 15000 });
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

test('캠페인 1-1: 내 신전과 적 배터리·신전의 우클릭 객체 메뉴가 열린다', async ({ page }) => {
  test.setTimeout(150000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => { localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } }));
    const done = {}; for (const name of ['Tutorial1', 'Tutorial2', 'Tutorial3', 'Tutorial4', 'Tutorial5', 'Tutorial6']) done[`Done${name}`] = '1'; localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: done })); });
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Struggle For Freedom');
  const label = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'TheWarBegins')?.label);
  await clickLabel(page, label);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 60000 });
  for (let index = 0; index < 6; index += 1) { const button = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons?.at(-1)?.label ?? null); if (button === null) break; await clickLabel(page, button); await page.waitForTimeout(400); }
  await page.waitForTimeout(1500);
  // 종류 이름으로 내 객체와 적 객체의 SID 를 찾아 각각 열어 본다.
  const found = await page.evaluate(() => { const app = window.netstorm, types = app._gameAssets.types(), mine = {}, enemy = {};
    for (const snapshot of app._campaignWorld.session().world().objects()) { const name = types[snapshot.object.type - 70]?.assetName, owner = snapshot.object.owner; if (owner === 1) mine[name] ??= snapshot.sid; else if (owner > 1) enemy[name] ??= snapshot.sid; }
    return { temple: mine.rainVortex, battery: enemy.windBattery, enemyTemple: enemy.windVortex }; });
  expect(found.temple).toBeGreaterThan(0); expect(found.battery).toBeGreaterThan(0); expect(found.enemyTemple).toBeGreaterThan(0);
  for (const [name, sid] of Object.entries(found)) {
    const opened = await page.evaluate((target) => { const app = window.netstorm; const result = app._campaignProduction.ui.openObject(target, { code: 0, x: 400, y: 440 }); return { result, popup: app._clientGump.blockingPopups() }; }, sid);
    expect(opened.result, name).toBe(1); expect(opened.popup, name).toBe(true);
    if (name === 'temple') await page.screenshot({ path: test.info().outputPath('temple-menu.png') });
    await page.evaluate(() => window.netstorm._closeCampaignPopups()); await page.waitForTimeout(200);
  }
  expect(errors).toEqual([]);
});
