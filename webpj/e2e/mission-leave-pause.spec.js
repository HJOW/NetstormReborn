// 열린 안내 창(게임 시계 정지) 상태에서 미션을 떠난 뒤 다시 시작해도 게임 시계가 정상으로 흐르는지 실제 Chromium 에서 확인한다. (2026-11-21 WB04-E1132)
// 이전에는 미션 관문이 안내 창 때문에 건 정지 깊이가 떠난 뒤에도 남아 다음 미션의 시계가 멈춘 채 시작했다(장시간 e2e 의 마지막 미션에서 발견).
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

test('안내 창이 열린 채 미션을 떠나도 다음 미션의 게임 시계가 정지하지 않는다', async ({ page }) => {
  test.setTimeout(240000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  // 시계 정지 여부·열린 안내 창·게임 시각을 읽는다.
  const state = () => page.evaluate(() => ({ paused: window.netstorm._gameClock.paused(), dialog: window.netstorm._mainMenu.dialog?.constructor?.name ?? null, game: window.netstorm._gameClock.gameSeconds() }));
  // Tutorial1 을 시작해 첫 안내 창이 열릴 때까지 기다린다.
  const start = async () => {
    await clickLabel(page, 'Campaign'); await clickLabel(page, 'Early Missions');
    const first = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'Tutorial1')?.label);
    await clickLabel(page, first);
    await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 60000 });
    await page.waitForFunction(() => Boolean(window.netstorm._mainMenu.dialog?.content), null, { timeout: 15000 });
  };
  // A: 첫 안내 창이 열려 있으면 시계가 멈춰 있다.
  await start(); const opened = await state(); expect(opened.paused).toBe(true); expect(opened.dialog).toBe('TellWindow');
  // B: 창이 열린 채 떠나면 시계 정지가 풀린다(정지 깊이가 남지 않는다).
  await page.evaluate(() => window.netstorm._leaveCampaignMission()); await page.waitForTimeout(500);
  expect((await state()).paused).toBe(false);
  // C: 다시 시작해도 첫 안내 창에서는 멈추고, D: 창을 모두 닫고 몇 초 지나면 시계가 흐른다.
  await start(); expect((await state()).paused).toBe(true);
  for (let index = 0; index < 4; index += 1) {
    const label = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons?.at(-1)?.label ?? null);
    if (label === null) break; await clickLabel(page, label); await page.waitForTimeout(700);
  }
  const before = await state(); await page.waitForTimeout(4000); const after = await state();
  expect(after.paused).toBe(false); expect(after.game - before.game).toBeGreaterThan(2);
  expect(errors).toEqual([]);
});
