// 메인 메뉴의 Demo 목록에서 데모 미션을 시작해 데모 표시로 월드가 진행되는지, 입력이 데모를 끝내는지 실제 Chromium 에서 확인한다. (2026-11-20 WB04-E1092)
const { test, expect } = require('@playwright/test');

// 실제 돌 버튼의 논리 좌표를 브라우저 마우스로 누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 15000 });
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

// 막대가 닫혀 있으면 우측 상단 버튼으로 열고 현재 막대 토큰을 돌려준다.
async function ensureBar(page) {
  if (!(await page.evaluate(() => window.netstorm._campaignProduction.ui.gameMenu().isOpen()))) { await page.locator('[data-netstorm="game-menu-button"]').click(); await page.waitForTimeout(300); }
  return page.evaluate(() => window.netstorm._campaignProduction.ui.gameMenu().token());
}
// 메뉴 작성기에 쓴 항목 라벨이다.
const labelsOf = (page, token) => page.evaluate((current) => window.netstorm._campaignProduction.ui.gameMenu().labels(current), token);
// 메뉴 항목 한 줄의 화면 좌표와 자식 메뉴 토큰이다(가로 메뉴는 항목 폭×번호, 세로 메뉴는 저장된 offset).
const rowPoint = (page, token, index) => page.evaluate(({ current, row }) => {
  const app = window.netstorm, root = app._clientGump, node = root.node(current), bounds = root.collection().bounds(current), rect = app.canvas.getBoundingClientRect();
  const horizontal = (node.menuFlags & 0x40000) !== 0, localX = horizontal ? node.itemWidth * (row + 0.5) : 20, localY = horizontal ? (bounds.bottom - bounds.top) / 2 : node.offsets[row] + node.heights[row] / 2;
  return { child: node.items[row].childId ? root.createState().entries[node.items[row].childId] : 0, x: rect.left + (bounds.left + localX) * rect.width / app._layout.logicalWidth, y: rect.top + (bounds.top + localY) * rect.height / app._layout.logicalHeight };
}, { current: token, row: index });
// 한 줄을 눌러 하위 메뉴가 있으면 완전히 펼쳐질 때까지 기다리고 자식 토큰을 돌려준다.
async function clickRow(page, token, index) {
  const point = await rowPoint(page, token, index); await page.mouse.move(point.x, point.y); await page.mouse.click(point.x, point.y, { delay: 60 });
  if (point.child) await page.waitForFunction((child) => { const node = window.netstorm._clientGump.node(child); return node && (node.flags & 1) && !node.expanding; }, point.child, { timeout: 10000 });
  return point.child;
}

test('Demo 목록의 데모를 시작하면 데모 표시로 월드가 진행된다', async ({ page }) => {
  test.setTimeout(180000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu'); await clickLabel(page, 'Demo');
  const items = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.filter((item) => item.command?.name === 'StartDemo').map((item) => [item.label, item.command.argument]));
  expect(items.map((item) => item[1])).toEqual(['DemoStormRages', 'GuardMyBack', 'DissolvedAlliance']);
  await clickLabel(page, items[0][0]);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 90000 });
  const gate = await page.evaluate(() => { const runtime = window.netstorm._clientGuide.current().runtime; return { demo: runtime.gate.demo, name: window.netstorm._lastCampaignMission.name }; });
  expect(gate).toEqual({ demo: 1, name: 'DemoStormRages' });
  // 안내가 있으면 닫고 게임 시계가 흐르는지 확인한다.
  for (let index = 0; index < 8; index += 1) {
    const label = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons.at(-1)?.label ?? null);
    if (label === null) break; await clickLabel(page, label);
  }
  await page.waitForFunction(() => !window.netstorm._gameClock.paused(), null, { timeout: 20000 });
  const started = await page.evaluate(() => window.netstorm._gameClock.time().game);
  await page.waitForFunction((before) => window.netstorm._gameClock.time().game >= before + 8, started, { timeout: 30000 });
  await page.screenshot({ path: process.env.DEMO_SHOT ?? 'test-results/demo-start.png' });
  // 데모 중에도 ESC 게임 메뉴가 열리고 Game 하위 메뉴의 미션 나가기 항목이 있다(원본은 360초마다 같은 데모를 다시 시작하며 사용자는 메뉴로 나간다).
  const bar = await ensureBar(page), top = await labelsOf(page, bar), game = await clickRow(page, bar, top.findIndex((label) => /Game/u.test(label)));
  const labels = await labelsOf(page, game); console.log('DEMO_GAME_LABELS', JSON.stringify(labels));
  const leave = labels.findIndex((label) => /Leave|Abort|Exit/iu.test(label)); expect(leave).toBeGreaterThanOrEqual(0);
  await clickRow(page, game, leave); await page.waitForTimeout(800);
  // ABORT 확인 창이 있으면 첫 단추(떠나기)를 누른다.
  const abort = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons.map((button) => button.label) ?? null); console.log('DEMO_ABORT_BUTTONS', JSON.stringify(abort));
  if (abort) await clickLabel(page, abort[0]);
  await page.waitForFunction(() => !window.netstorm._campaignWorld && window.netstorm.state === 'mainMenu', null, { timeout: 20000 });
  expect(await page.evaluate(() => ({ world: Boolean(window.netstorm._campaignWorld), guide: Boolean(window.netstorm._clientGuide.current()), paused: window.netstorm._gameClock.paused() }))).toEqual({ world: false, guide: false, paused: false });
  expect(errors).toEqual([]);
});

// 메인 메뉴를 가만히 두면 Auto-Demo 옵션에 따라 DemoStormRages 가 저절로 시작되고, 옵션을 끄면 시작되지 않는다. (기한은 시험에서 앞당긴다)
test('메인 메뉴 무활동 감시는 Auto-Demo 가 켜진 때만 데모를 자동 시작한다', async ({ page }) => {
  test.setTimeout(180000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.message));
  const open = async (autoDemo) => {
    await page.addInitScript((value) => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, autoDemo: value } })), autoDemo);
    await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3');
    await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
    await page.waitForFunction(() => window.netstorm?.state === 'mainMenu' && window.netstorm._mainMenuActivity.armed, null, { timeout: 15000 });
  };
  // 켜진 경우: 기한을 곧 지나게 하면 메인 메뉴에서 데모가 시작된다.
  await open(1);
  await page.evaluate(() => { window.netstorm._mainMenuActivity.deadline = window.netstorm._frameSeconds + 0.5; });
  await page.waitForFunction(() => window.netstorm._clientGuide.current()?.runtime.gate.demo === 1, null, { timeout: 60000 });
  expect(await page.evaluate(() => window.netstorm._lastCampaignMission.name)).toBe('DemoStormRages');
  expect(errors).toEqual([]);
});

test('Auto-Demo 가 꺼져 있으면 메인 메뉴 무활동에도 데모를 시작하지 않는다', async ({ page }) => {
  const errors = []; page.on('pageerror', (error) => errors.push(error.message));
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, autoDemo: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu' && window.netstorm._mainMenuActivity.armed, null, { timeout: 15000 });
  await page.evaluate(() => { window.netstorm._mainMenuActivity.deadline = window.netstorm._frameSeconds + 0.5; });
  await page.waitForTimeout(3000);
  expect(await page.evaluate(() => ({ demo: window.netstorm._clientGuide.current()?.runtime.gate.demo ?? 0, world: Boolean(window.netstorm._campaignWorld), state: window.netstorm.state }))).toEqual({ demo: 0, world: false, state: 'mainMenu' });
  expect(errors).toEqual([]);
});

// 나머지 두 데모(GuardMyBack·DissolvedAlliance)도 같은 방식으로 시작되어 월드가 흐르고, 나가면 메인 메뉴로 돌아온다.
for (const [index, name] of [[1, 'GuardMyBack'], [2, 'DissolvedAlliance']]) {
  test(`${name} 데모를 시작하면 월드가 흐른다`, async ({ page }) => {
    test.setTimeout(180000);
    const errors = []; page.on('pageerror', (error) => errors.push(error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
    await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } })));
    await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3');
    await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
    await page.waitForFunction(() => window.netstorm?.state === 'mainMenu'); await clickLabel(page, 'Demo');
    const label = await page.evaluate((row) => window.netstorm._mainMenu.dialog.menu.items.filter((item) => item.command?.name === 'StartDemo')[row].label, index);
    await clickLabel(page, label);
    await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 90000 });
    expect(await page.evaluate(() => ({ demo: window.netstorm._clientGuide.current().runtime.gate.demo, name: window.netstorm._lastCampaignMission.name }))).toEqual({ demo: 1, name });
    for (let step = 0; step < 8; step += 1) {
      const button = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons.at(-1)?.label ?? null);
      if (button === null) break; await clickLabel(page, button);
    }
    await page.waitForFunction(() => !window.netstorm._gameClock.paused(), null, { timeout: 20000 });
    const started = await page.evaluate(() => window.netstorm._gameClock.time().game);
    await page.waitForFunction((before) => window.netstorm._gameClock.time().game >= before + 8, started, { timeout: 30000 });
    await page.evaluate(() => window.netstorm._leaveCampaignMission());
    await page.waitForFunction(() => !window.netstorm._campaignWorld && window.netstorm.state === 'mainMenu', null, { timeout: 20000 });
    expect(errors).toEqual([]);
  });
}

// 데모는 `demoRestartTimer`(360초) 기한이 지나면 원본처럼 같은 데모를 처음부터 다시 시작한다(MissionReload 예약 → Leave → 같은 이름·모드로 시작). 기한은 시험에서 앞당긴다. (2026-11-20 WB04-E1125)
test('데모 종료 기한이 지나면 같은 데모를 새 월드로 다시 시작한다', async ({ page }) => {
  test.setTimeout(240000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu'); await clickLabel(page, 'Demo');
  const label = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'StartDemo' && item.command.argument === 'DemoStormRages')?.label);
  await clickLabel(page, label);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 90000 });
  // 시작 직후의 데모 종료 기한은 시작 시각 + 360초 이다.
  const first = await page.evaluate(() => { const app = window.netstorm, gate = app._clientGuide.current().runtime.gate; window.__firstWorld = app._campaignWorld; return { end: gate.demoEndSeconds, now: app._gameClock.gameSeconds() }; });
  expect(first.end - first.now).toBeGreaterThan(300); expect(first.end - first.now).toBeLessThanOrEqual(360);
  // 기한을 곧 지나게 하면 새 월드로 같은 데모가 다시 시작하고 기한이 새로 잡힌다.
  await page.evaluate(() => { const app = window.netstorm; app._clientGuide.current().runtime.gate.demoEndSeconds = app._gameClock.gameSeconds() + 1; });
  await page.waitForFunction(() => window.netstorm._campaignWorld && window.netstorm._campaignWorld !== window.__firstWorld && window.netstorm._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 90000 });
  const second = await page.evaluate(() => { const app = window.netstorm, gate = app._clientGuide.current().runtime.gate; return { name: app._lastCampaignMission.name, demo: gate.demo, end: gate.demoEndSeconds, now: app._gameClock.gameSeconds() }; });
  expect(second.name).toBe('DemoStormRages'); expect(second.demo).toBe(1); expect(second.end - second.now).toBeGreaterThan(300); expect(second.end - second.now).toBeLessThanOrEqual(360);
  expect(errors).toEqual([]);
});
