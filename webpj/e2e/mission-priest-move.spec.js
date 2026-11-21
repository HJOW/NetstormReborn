// 사제를 실제 마우스로 선택하고 같은 섬의 땅을 눌러 이동시키는 흐름을 실제 Chromium 에서 확인한다. (2026-11-21 WB04-E1139)
// 선택(좌클릭) → 이동 커서(번호 18) → 땅 좌클릭 → 사제가 걸어 목표 근처에 선다. LEFT_JOBS "사제·운반·경로 바인딩"의 실사용 점검이다.
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

test('The War Begins!: 사제를 선택하고 땅을 누르면 사제가 그 쪽으로 걸어간다', async ({ page }) => {
  test.setTimeout(240000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => {
    localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } }));
    const done = {}; for (const name of ['Tutorial1', 'Tutorial2', 'Tutorial3', 'Tutorial4', 'Tutorial5', 'Tutorial6']) done[`Done${name}`] = '1'; localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: done }));
  });
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Struggle For Freedom');
  const label = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'TheWarBegins')?.label);
  await clickLabel(page, label);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
  for (let index = 0; index < 10; index += 1) {
    const button = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons?.at(-1)?.label ?? null);
    if (button === null) break; await clickLabel(page, button); await page.waitForTimeout(500);
  }
  await page.waitForFunction(() => !window.netstorm._gameClock.paused(), null, { timeout: 20000 });
  // 사제의 슬롯 좌표(셀)와 현재 선택, 화면 적중 픽셀(브라우저 좌표)을 읽는다.
  const priest = () => page.evaluate(() => { const app = window.netstorm, world = app._campaignWorld, session = world.session(), sid = session.findPriestParticipation(session.localOwner()), raw = session.core().pool.slot(sid), view = new DataView(raw.buffer, raw.byteOffset, raw.byteLength);
    return { sid, x: view.getFloat32(14, true), y: view.getFloat32(18, true), selected: app._clientGameplay.selection.selected }; });
  const before = await priest(); expect(before.selected, '시작 때는 아무것도 선택되지 않는다').toBe(0);
  const spot = await page.evaluate((sid) => { const app = window.netstorm, world = app._campaignWorld, hits = [];
    for (let y = 10; y < 760; y += 2) for (let x = 90; x < 1020; x += 2) if (world.pick(x, y, 0) === sid) hits.push([x, y]);
    const rect = app.canvas.getBoundingClientRect(), hit = hits[Math.floor(hits.length / 2)]; return { x: rect.left + hit[0] * rect.width / app._layout.logicalWidth, y: rect.top + hit[1] * rect.height / app._layout.logicalHeight }; }, before.sid);
  // 좌클릭으로 선택한다.
  await page.mouse.move(spot.x, spot.y); await page.mouse.click(spot.x, spot.y, { delay: 60 }); await page.waitForTimeout(400);
  expect((await priest()).selected, '사제 선택').toBe(before.sid);
  // 오른쪽 땅(약 90 픽셀)에 마우스를 올리면 이동 커서(18)가 되고, 누르면 사제가 그쪽으로 걸어 멈춘다.
  const destination = { x: spot.x + 90, y: spot.y + 10 };
  await page.mouse.move(destination.x, destination.y); await page.waitForTimeout(300);
  expect(await page.evaluate(() => window.netstorm._campaignCursor), '이동 커서').toBe(18);
  await page.mouse.click(destination.x, destination.y, { delay: 60 });
  await expect.poll(async () => (await priest()).x, { timeout: 15000, message: '사제 이동' }).toBeGreaterThan(before.x + 3);
  expect(errors).toEqual([]);
});
