// 운반체 유닛(Bulf 등)을 실제 마우스로 우클릭하면 객체 메뉴가 열리고 바깥 클릭으로 닫히는지 실제 Chromium 에서 확인한다. (2026-11-21 WB04-E1138)
// 운반체(+ac 004268c0, +a8 004288f0)와 Archer 계열(+ac 0041e660)의 열기가 객체 메뉴 열기에 연결되지 않았을 때는 이런 유닛을 우클릭해도 메뉴가 열리지 않았다.
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

test('Priest Training 5: 내 Bulf 를 우클릭하면 운반체 객체 메뉴가 열리고 바깥 클릭으로 닫힌다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => {
    localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } }));
    const done = {}; for (const name of ['Tutorial1', 'Tutorial2', 'Tutorial3', 'Tutorial4', 'Tutorial5', 'Tutorial6']) done[`Done${name}`] = '1'; localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: done }));
  });
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Priest Training');
  const label = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'bc5walkers76')?.label);
  await clickLabel(page, label);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
  for (let index = 0; index < 10; index += 1) {
    const button = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons?.at(-1)?.label ?? null);
    if (button === null) break; await clickLabel(page, button); await page.waitForTimeout(500);
  }
  await page.waitForFunction(() => !window.netstorm._gameClock.paused(), null, { timeout: 20000 });
  // 내 Bulf 하나의 화면 적중 픽셀(브라우저 좌표)과 빈 곳(아무것도 잡히지 않는 지점)을 찾는다.
  const spots = await page.evaluate(() => {
    const app = window.netstorm, world = app._campaignWorld, session = world.session(), types = app._gameAssets.types(); let bulf = null;
    for (const { sid, object } of session.world().objects()) if (object.owner === 1 && types[object.type - 70]?.assetName === 'bulf') { bulf = sid; break; }
    const hits = []; let empty = null;
    for (let y = 10; y < 760; y += 4) for (let x = 90; x < 1020; x += 4) { const sid = world.pick(x, y, 0); if (sid === bulf) hits.push([x, y]); else if (!sid && !empty && y > 600 && x > 900) empty = [x, y]; }
    const rect = app.canvas.getBoundingClientRect(), hit = hits[Math.floor(hits.length / 2)];
    return bulf && hit && empty ? { bulf, x: rect.left + hit[0] * rect.width / app._layout.logicalWidth, y: rect.top + hit[1] * rect.height / app._layout.logicalHeight,
      ex: rect.left + empty[0] * rect.width / app._layout.logicalWidth, ey: rect.top + empty[1] * rect.height / app._layout.logicalHeight } : null;
  });
  expect(spots, '내 Bulf 와 빈 지점').toBeTruthy();
  // 우클릭 → 그 Bulf 의 객체 메뉴(command 26, object 가 Bulf)가 열린다.
  await page.mouse.move(spots.x, spots.y); await page.mouse.click(spots.x, spots.y, { button: 'right', delay: 60 });
  await page.waitForFunction((sid) => { const root = window.netstorm._clientGump; for (let token = root.root().child; token; token = root.node(token).next) if (root.node(token).command === 26 && root.node(token).object === sid) return true; return false; }, spots.bulf, { timeout: 10000 });
  // 바깥(빈 곳)을 눌러 닫는다.
  await page.mouse.move(spots.ex, spots.ey); await page.mouse.click(spots.ex, spots.ey, { delay: 60 }); await page.waitForTimeout(500);
  expect(await page.evaluate(() => window.netstorm._clientGump.blockingPopups())).toBe(false);
  expect(errors).toEqual([]);
});
