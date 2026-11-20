// 12차 스커미시 게임이 실제로 진행되는지(AI 의 건설·생산, 소유자별 객체 증가, 금지 유닛, 오류 없음) 실제 Chromium 에서 시간이 흐르는 동안 관찰한다. (2026-11-20 WB04-E1106)
const { test, expect } = require('@playwright/test');

// 실제 돌 버튼·메뉴 항목의 논리 좌표를 브라우저 마우스로 누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 30000 });
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

// 소유자별 객체 수와 TYPE 이름별 수를 읽는다.
const census = (page) => page.evaluate(() => {
  const app = window.netstorm, session = app._campaignWorld.session(), types = app._gameAssets.types(), owners = {}, names = {};
  for (const snapshot of session.world().objects()) {
    const object = snapshot.object, owner = object.owner; owners[owner] = (owners[owner] ?? 0) + 1;
    const type = types[object.type - 70]; if (type && owner > 0) { const key = `${owner}:${type.assetName}`; names[key] = (names[key] ?? 0) + 1; }
  }
  return { owners, names, clock: app._gameClock.gameSeconds() };
});

test('스커미시 게임: AI 가 시간이 흐르면 건설·생산하고 오류가 없다', async ({ page }) => {
  test.setTimeout(200000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, skirmishAi2Diff: 3 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 });
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Skirmish'); await clickLabel(page, 'Start');
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 90000 });
  await clickLabel(page, 'Go!'); await page.waitForTimeout(3000);
  const samples = [await census(page)];
  for (let step = 0; step < 3; step += 1) { await page.waitForTimeout(20000); samples.push(await census(page)); }
  await page.screenshot({ path: test.info().outputPath('skirmish-play.png') });
  const last = samples.at(-1);
  // 게임 시계가 흐르고 사람(소유자 1)과 AI(소유자 2)가 각각 플레이어·사제를 하나씩 가지며, 어려움 AI 는 시작 지연(실행마다 다름) 뒤 다리를 계속 늘려 마지막 관찰에서 객체 수가 크게 늘어 있다.
  expect(last.clock).toBeGreaterThan(samples[0].clock + 40);
  for (const owner of [1, 2]) { expect(last.names[`${owner}:player`]).toBe(1); expect(last.names[`${owner}:priest`]).toBe(1); }
  expect(last.owners[2]).toBeGreaterThan(100); expect(last.names['2:bridge']).toBeGreaterThan(50); expect(last.names['2:bridgeConnector']).toBeGreaterThan(0);
  expect(errors).toEqual([]);
});
