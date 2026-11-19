// 실제 웹 메뉴에서 초기화 취소·확정과 새로고침 후 최초 실행을 확인한다. (2026-11-19 WB04-E863)
const { test, expect } = require('@playwright/test');

// Canvas 의 현재 메뉴 항목 좌표를 실제 마우스 좌표로 바꾸어 누른다.
async function clickLabel(page, label) {
  const point = await page.evaluate((text) => {
    const app = window.netstorm;
    const logical = app._mainMenu.controlPoint(text);
    if (!logical) throw new Error(`조작부 없음: ${text}`);
    const rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

// 두 게임 항목과 다른 앱의 항목을 넣고 취소와 확정의 차이를 확인한다.
test('Reset All 확인 취소와 확정 후 최초 실행', async ({ page }) => {
  const errors = [];
  page.on('pageerror', (error) => errors.push(String(error)));
  await page.goto('/');
  await page.evaluate(() => {
    localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, soundVolume: 2 } }));
    localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: { DoneTheWarBegins: '1' } }));
    localStorage.setItem('other.app', 'keep');
  });
  await page.reload();
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart');
  await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  await clickLabel(page, 'Options');
  page.once('dialog', (dialog) => dialog.dismiss());
  await clickLabel(page, 'Reset All');
  expect(await page.evaluate(() => [window.netstorm.state, window.netstorm._settings.get('soundVolume'), window.netstorm.progress.get('DoneTheWarBegins')])).toEqual(['mainMenu', 2, '1']);
  await clickLabel(page, 'Options');
  page.once('dialog', (dialog) => dialog.accept());
  const loaded = page.waitForEvent('load');
  await clickLabel(page, 'Reset All');
  await loaded;
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart');
  expect(await page.evaluate(() => [localStorage.getItem('netstorm.settings'), localStorage.getItem('netstorm.progress'), localStorage.getItem('other.app')])).toEqual([null, null, 'keep']);
  await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  expect(await page.evaluate(() => window.netstorm._gameData.currentTell)).toBe('Welcome');
  expect(errors).toEqual([]);
});
