// 옵션 메뉴의 Frame Rate(프레임 상한 60·75·120 FPS) 하위 메뉴가 메인 메뉴와 게임 중 메뉴에서 같은 설정(maxFPS)을 쓰고 저장하는지 실제 Chromium 에서 확인한다. (2026-11-20 WB04-E1094)
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

// 현재 펼침 메뉴 맨 위 목록에서 체크된 항목 라벨들이다.
const checkedLabels = (page) => page.evaluate(() => window.netstorm._mainMenu.popup.levels.at(-1).list.items.filter((item) => item.checked).map((item) => item.label));

test('메인 메뉴 Options 의 Frame Rate 하위 메뉴가 기본 75 에 체크하고 선택을 저장한다', async ({ page }) => {
  const errors = []; page.on('pageerror', (error) => errors.push(error.message));
  await page.addInitScript(() => { if (!localStorage.getItem('netstorm.settings')) localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } })); });
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  await clickLabel(page, 'Options'); await clickLabel(page, 'Frame Rate');
  expect(await checkedLabels(page)).toEqual(['75 FPS (Original)']);
  await clickLabel(page, '120 FPS'); await page.waitForTimeout(300);
  expect(await page.evaluate(() => JSON.parse(localStorage.getItem('netstorm.settings')).values.maxFPS)).toBe(120);
  // 다시 열면 체크가 120 으로 옮겨 있고 선택 뒤 Tell 창은 열리지 않는다.
  expect(await page.evaluate(() => window.netstorm._mainMenu.dialog)).toBeNull();
  await clickLabel(page, 'Options'); await clickLabel(page, 'Frame Rate');
  expect(await checkedLabels(page)).toEqual(['120 FPS']);
  await clickLabel(page, '60 FPS'); await page.waitForTimeout(300);
  expect(await page.evaluate(() => JSON.parse(localStorage.getItem('netstorm.settings')).values.maxFPS)).toBe(60);
  // 처리한 프레임은 선택한 상한을 따른다(프레임 간격이 상한 간격 이상이다).
  const stamps = await page.evaluate(() => new Promise((resolve) => { const app = window.netstorm, seen = []; let last = app._lastFrameStamp; const timer = setInterval(() => { if (app._lastFrameStamp !== last) { last = app._lastFrameStamp; seen.push(last); } }, 1); setTimeout(() => { clearInterval(timer); resolve(seen); }, 1500); }));
  expect(stamps.length).toBeGreaterThan(10);
  const gaps = stamps.slice(1).map((stamp, index) => stamp - stamps[index]); expect(Math.min(...gaps)).toBeGreaterThanOrEqual(1000 / 60 - 1.6);
  expect(errors).toEqual([]);
});

test('게임 중 ESC 메뉴 Options 에도 같은 Frame Rate 하위 메뉴가 있고 같은 설정에 쓴다', async ({ page }) => {
  test.setTimeout(120000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.message));
  await page.addInitScript(() => { localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } })); localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: { DoneTutorial1: '1' } })); });
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu'); await clickLabel(page, 'Campaign'); await clickLabel(page, 'Early Missions');
  const first = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'Tutorial1')?.label);
  await clickLabel(page, first);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 50000 });
  await page.waitForFunction(() => Boolean(window.netstorm._mainMenu.dialog), null, { timeout: 15000 });
  for (let index = 0; index < 8; index += 1) { const label = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons.at(-1)?.label ?? null); if (label === null) break; await clickLabel(page, label); }
  await page.waitForFunction(() => !window.netstorm._gameClock.paused() && window.netstorm._campaignProduction?.ui?.gameMenu(), null, { timeout: 20000 });
  // ESC 막대의 Options 하위 맨 앞에 Language·Frame Rate·Difficulty 가 있고 세 항목 중 75 에 체크가 있다.
  await page.locator('[data-netstorm="game-menu-button"]').click(); await page.waitForTimeout(300);
  const labelsOf = (token) => page.evaluate((current) => window.netstorm._campaignProduction.ui.gameMenu().labels(current), token);
  const rowPoint = (token, row) => page.evaluate(({ current, index }) => {
    const app = window.netstorm, root = app._clientGump, node = root.node(current), bounds = root.collection().bounds(current), rect = app.canvas.getBoundingClientRect();
    const horizontal = (node.menuFlags & 0x40000) !== 0, localX = horizontal ? node.itemWidth * (index + 0.5) : 20, localY = horizontal ? (bounds.bottom - bounds.top) / 2 : node.offsets[index] + node.heights[index] / 2;
    return { child: node.items[index].childId ? root.createState().entries[node.items[index].childId] : 0, x: rect.left + (bounds.left + localX) * rect.width / app._layout.logicalWidth, y: rect.top + (bounds.top + localY) * rect.height / app._layout.logicalHeight };
  }, { current: token, index: row });
  const clickRow = async (token, row) => {
    const point = await rowPoint(token, row); await page.mouse.move(point.x, point.y); await page.mouse.click(point.x, point.y, { delay: 60 });
    if (point.child) await page.waitForFunction((child) => { const node = window.netstorm._clientGump.node(child); return node && (node.flags & 1) && !node.expanding; }, point.child, { timeout: 10000 });
    return point.child;
  };
  const bar = await page.evaluate(() => window.netstorm._campaignProduction.ui.gameMenu().token()), top = await labelsOf(bar);
  const options = await clickRow(bar, top.findIndex((label) => /Options/u.test(label))), optionLabels = await labelsOf(options);
  expect(optionLabels.slice(0, 3)).toEqual(['Language', 'Frame Rate', 'Difficulty']);
  const rates = await clickRow(options, 1), rateLabels = await labelsOf(rates);
  expect(rateLabels).toEqual(['60 FPS', '75 FPS (Original)', '120 FPS']);
  await clickRow(rates, 0); await page.waitForTimeout(500);
  expect(await page.evaluate(() => window.netstorm._settings.get('maxFPS'))).toBe(60);
  expect(errors).toEqual([]);
});
