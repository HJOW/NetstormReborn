// 한국어 캠페인 미션(TheWarBegins)의 브리핑과 적 이름이 실제 Chromium 에서 한국어로 나오고 오류 없이 시작되는지 확인한다. (2026-11-20 WB04-E1117)
const { test, expect } = require('@playwright/test');

// 실제 게임 조작부의 논리 좌표를 브라우저 마우스 좌표로 바꿔 누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 30000 });
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

test('한국어 캠페인 첫 미션: 브리핑·제목·적 이름이 한국어이고 오류 없이 시작한다', async ({ page }) => {
  test.setTimeout(150000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?lang=ko&aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 });
  await clickLabel(page, '캠페인');
  // 첫 캠페인 묶음 '자유를 위한 투쟁'(offical3)을 연다.
  await clickLabel(page, '자유를 위한 투쟁');
  const mission = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'TheWarBegins')?.label);
  expect(mission).toMatch(/[가-힣]/u); await clickLabel(page, mission);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 50000 });
  await page.waitForFunction(() => Boolean(window.netstorm._mainMenu.dialog?.content), null, { timeout: 15000 });
  const info = await page.evaluate(() => { const app = window.netstorm, script = app._clientGuide.current().script; return { title: script.get('title'), enemy: script.get('aiName'), brief: app._mainMenu.dialog.content.styleText }; });
  expect(info.title).toBe('전쟁의 시작!'); expect(info.enemy).toBe('간수'); expect(info.brief).toContain('어둠의 군주'); expect(info.brief).toContain('사제');
  await page.screenshot({ path: test.info().outputPath('korean-campaign.png') });
  await clickLabel(page, '미션 시작'); await page.waitForTimeout(4000);
  expect(await page.evaluate(() => window.netstorm._gameClock.gameSeconds())).toBeGreaterThan(1);
  expect(errors).toEqual([]);
});
