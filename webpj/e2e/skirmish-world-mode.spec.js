// 스커미시 월드의 실행 모드를 실제 Chromium 에서 고정한다: 스커미시는 미션 모드(missionPost 참)로 도는 단독 전투라 멀티플레이 전투 전용인 비미션 너겟 배치·보충(PlaceBattleNuggets·ReplenishNuggets)은
// 도달하지 않고(월드에 너겟 객체가 없음), 간헐천은 무작위로 놓인다. 이 모드가 바뀌면 이 시험이 먼저 알려 준다. (2026-11-23 WB04-E1270)
const { test, expect } = require('@playwright/test');

// 실제 돌 버튼의 논리 좌표를 브라우저 마우스 좌표로 바꿔 누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 30000 });
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

test('스커미시: 미션 모드의 전투 월드이며 비미션 너겟은 없고 간헐천이 놓인다', async ({ page }) => {
  test.setTimeout(240000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, autoDemo: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Skirmish'); await clickLabel(page, 'Start');
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
  const world = await page.evaluate(() => {
    const session = window.netstorm._campaignWorld.session(), types = session.core().types, counts = {};
    for (const { object } of session.world().objects()) { const name = types[object.type]?.name ?? String(object.type); counts[name] = (counts[name] ?? 0) + 1; }
    return { missionPost: Boolean(window.netstorm._clientGameplay.world?.missionPost), battle: Boolean(window.netstorm._surfaceStorage.ownerMode.battle), counts };
  });
  expect(world.missionPost, '스커미시는 미션 모드').toBe(true);
  expect(world.battle, '전투 모드').toBe(true);
  expect(Object.keys(world.counts).filter((name) => /nugget/i.test(name)), '비미션 너겟 객체 없음').toEqual([]);
  expect(world.counts.geyser ?? 0, '무작위로 놓인 간헐천').toBeGreaterThan(0);
  expect(world.counts.priest ?? 0, '플레이어와 AI 사제').toBeGreaterThanOrEqual(2);
  expect(errors).toEqual([]);
});
