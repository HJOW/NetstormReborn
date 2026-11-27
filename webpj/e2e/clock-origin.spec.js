// 실제 비고정 부팅의 epoch/최초Capture와 준비 중destroy/재초기화를 검사한다. 목표 수행이나 진행 상태는 주입하지 않는다.
const { test, expect } = require('@playwright/test');

// 현재 실제 Client의 시각과 같은 순간의 UTC 기대값을 읽는다. 시간 설정은 바꾸지 않는다.
async function time(page) {
  return page.evaluate(() => {
    const app = window.netstorm, now = Date.now(), ticks = BigInt.asUintN(32, BigInt(now) - BigInt(0x32c85780) * 1000n);
    return { ...app._gameClock.time(), currentWall: app._gameClock.wallSeconds(), currentGame: app._gameClock.gameSeconds(), expectedWall: Number(ticks) / 1000,
      preparing: app._clientClockPreparing, paused: app._gameClock.paused(), fixed: app.options.fixedFrameMs };
  });
}
// 실제 사용자 설정은 시작 안내/자동데모를 끄며 시각 공급원이나 게임 상태는 건드리지 않는다.
async function settings(page) {
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, autoDemo: 0, sound: 0, music: 0 } })));
}

test('실제 비고정 시계: epoch·최초 delta와 Tutorial1 안내 정지의 벽/게임 시각', async ({ page }) => {
  test.setTimeout(180000); const errors = [];
  page.on('pageerror', (error) => errors.push(error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await settings(page); await page.goto('/debug.html?title=0&startwait=0');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 });
  await page.waitForTimeout(100); const first = await time(page);
  expect(first.fixed).toBe(0); expect(first.preparing).toBe(false); expect(first.number).toBeGreaterThan(1);
  expect(Math.abs(first.currentWall - first.expectedWall)).toBeLessThan(1); expect(Math.abs(first.delta)).toBeLessThan(5);
  await page.locator('canvas').click(); await page.waitForFunction(() => window.netstorm.state === 'mainMenu');
  await page.evaluate(() => window.netstorm._beginCampaignFromDialog('Tutorial1'));
  await page.waitForFunction(() => window.netstorm._campaignDisplayActive() && window.netstorm._clientGuide.current().runtime.gate.started && window.netstorm._mainMenu.dialog, null, { timeout: 90000 });
  const paused = await time(page); expect(paused.paused).toBe(true);
  await page.waitForTimeout(250); const later = await time(page); expect(later.currentWall).toBeGreaterThan(paused.currentWall);
  expect(later.currentGame).toBe(paused.currentGame); expect(Math.abs(later.currentWall - later.expectedWall)).toBeLessThan(1);
  expect(errors).toEqual([]); console.log('비고정 epoch 실제시각', JSON.stringify({ first, paused, later }));
});

// 호스트 초를 고정해 실제 비동기 준비를 대기시킨다. 게임 프레임은0이며 취소 후 새 앱만 실제 시계로 진행해야 한다.
test('실제 초기 시계 수명: 준비 중destroy 뒤 재초기화에서 이전 결과를 버린다', async ({ page }) => {
  test.setTimeout(180000); const errors = [];
  page.on('pageerror', (error) => errors.push(error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await settings(page);
  await page.addInitScript(() => { const actual = Date.now, frozen = actual(); Date.now = () => frozen; window.__restoreClockDate = () => { Date.now = actual; }; });
  await page.goto('/debug.html?title=0&startwait=0');
  await page.waitForFunction(() => window.netstorm?._clientClockPreparing && window.netstorm.state === 'loading');
  expect(await page.evaluate(() => window.netstorm._gameClock.time().number)).toBe(0);
  await page.evaluate(() => { const app = window.netstorm, root = document.getElementById('netstorm_root'); window.__retiredClock = app._gameClock; app.destroy(); window.__restoreClockDate(); app.init(root); });
  await page.waitForFunction(() => window.netstorm.state === 'clickToStart', null, { timeout: 60000 });
  const result = await time(page); expect(result.preparing).toBe(false); expect(Math.abs(result.currentWall - result.expectedWall)).toBeLessThan(1);
  expect(await page.evaluate(() => ({ prior: window.__retiredClock.time().number, replaced: window.__retiredClock !== window.netstorm._gameClock }))).toEqual({ prior: 0, replaced: true });
  expect(errors).toEqual([]);
});
