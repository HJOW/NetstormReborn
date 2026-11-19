// 실제 브라우저에서 캠페인 목록 클릭부터 첫 Tutorial 화면까지 연결을 확인한다.
const { test, expect } = require('@playwright/test');

// 현재 조작부의 논리 좌표를 Canvas CSS 좌표로 바꾸어 실제 마우스로 누른다.
async function clickLabel(page, label) {
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    if (!logical) throw new Error(`조작부 없음: ${text}`);
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

test('캠페인 목록에서 첫 Tutorial의 월드와 안내를 표시한다', async ({ page }) => {
  const errors = []; page.on('pageerror', (error) => errors.push(error.message));
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } })));
  await page.setViewportSize({ width: 1280, height: 720 }); await page.goto('/');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Early Missions');
  const first = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'Tutorial1')?.label);
  expect(first).toBeTruthy(); await clickLabel(page, first);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 50000 });
  await page.waitForFunction(() => Boolean(window.netstorm._mainMenu.dialog), null, { timeout: 15000 });
  const state = await page.evaluate(() => {
    const app = window.netstorm;
    return { title: document.title, frame: app._clientRenderer.drawCount, paused: app._gameClock.paused(), width: app._surface.width,
      visiblePixels: app._surface.pixels.some((pixel) => pixel !== 0) };
  });
  expect(state.frame).toBeGreaterThan(0); expect(state.paused).toBe(true); expect(state.width).toBe(1366); expect(state.visiblePixels).toBe(true);
  expect(errors).toEqual([]);
});
