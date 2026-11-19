// 실제 미션 생산 항목 우클릭과 원본 정보 메뉴의 Help 입력을 확인한다.
const { test, expect } = require('@playwright/test');

// 실제 돌 버튼의 논리 좌표를 브라우저 마우스로 누른다.
async function clickLabel(page, label) {
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    if (!logical) throw new Error(`조작부 없음: ${text}`);
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

test('Tutorial 생산 항목에서 우클릭으로 정보와 도움말을 연다', async ({ page }) => {
  const errors = []; page.on('pageerror', (error) => errors.push(error.message));
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } })));
  await page.setViewportSize({ width: 1280, height: 720 }); await page.goto('/');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu'); await clickLabel(page, 'Campaign'); await clickLabel(page, 'Early Missions');
  const first = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'Tutorial1')?.label);
  expect(first).toBeTruthy(); await clickLabel(page, first);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 50000 });
  await page.waitForFunction(() => Boolean(window.netstorm._mainMenu.dialog), null, { timeout: 15000 });
  // 첫 안내 페이지를 실제 화면 버튼으로 닫는다.
  for (let index = 0; index < 8; index += 1) {
    const label = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons.at(-1)?.label ?? null);
    if (label === null) break; await clickLabel(page, label);
  }
  await page.waitForFunction(() => !window.netstorm._gameClock.paused() && window.netstorm._clientGump.node(window.netstorm._campaignProduction.ui.node().bridge).child, null, { timeout: 20000 });
  const item = await page.evaluate(() => {
    const app = window.netstorm, root = app._clientGump, token = root.node(app._campaignProduction.ui.node().bridge).child;
    const bounds = root.collection().bounds(token), rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + (bounds.left + bounds.right) / 2 * rect.width / app._layout.logicalWidth,
      y: rect.top + (bounds.top + bounds.bottom) / 2 * rect.height / app._layout.logicalHeight };
  });
  await page.mouse.click(item.x, item.y, { button: 'right', delay: 60 });
  await page.waitForFunction(() => { const root = window.netstorm._clientGump; for (let t = root.root().child; t; t = root.node(t).next) if (root.node(t).command === 26) return true; return false; });
  const menu = await page.evaluate(() => {
    const app = window.netstorm, root = app._clientGump, rect = app.canvas.getBoundingClientRect(); let token = root.root().child;
    while (token && root.node(token).command !== 26) token = root.node(token).next;
    const node = root.node(token), bounds = root.collection().bounds(token);
    return { count: node.itemCount, title: Boolean(node.title), form: Boolean(node.form), x: rect.left + (bounds.left + 8) * rect.width / app._layout.logicalWidth,
      y: rect.top + (bounds.top + 5) * rect.height / app._layout.logicalHeight };
  });
  expect(menu.count).toBeGreaterThan(0); expect(menu.title).toBe(true); expect(menu.form).toBe(true);
  await page.mouse.move(menu.x, menu.y); await page.mouse.click(menu.x, menu.y, { delay: 60 });
  await page.waitForFunction(() => window.netstorm._mainMenu.dialog?.topic === 'bridgeType');
  await clickLabel(page, 'OK'); await page.waitForFunction(() => !window.netstorm._mainMenu.dialog);
  // 원본 다리에는 정보 태그가 없으므로 실제 썬더 팩토리 TYPE 명령을 큐에 넣어 별도 정보 화면을 확인한다.
  await page.evaluate(() => { const app = window.netstorm, type = app._gameAssets.find('thunderFactory').block + 70; app._clientInputQueue.pushCommand(0x02000017, 0, type); });
  await page.waitForFunction(() => window.netstorm._mainMenu.dialog?.topic === 'thunderFactoryType' && window.netstorm._mainMenu.dialog.information);
  const information = await page.evaluate(() => { const pane = window.netstorm._mainMenu.dialog.information; return { x: pane.x, y: pane.y, lines: pane.softLines.length }; });
  expect(information.lines).toBeGreaterThan(0); await clickLabel(page, 'OK'); await page.waitForFunction(() => !window.netstorm._mainMenu.dialog);
  await expect.poll(() => page.evaluate(() => window.netstorm._clientInputQueue.size)).toBe(0); expect(errors).toEqual([]);
});
