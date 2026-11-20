// 미션 중 Shift-F7 이 현재 화면을 GIF 로 브라우저 내려받기하고 일반 F7(섬 소유자 표식 토글)로 새지 않는지 실제 Chromium 에서 확인한다. (2026-11-20 WB04-E1100)
const { test, expect } = require('@playwright/test');
const fs = require('node:fs');

// 실제 돌 버튼의 논리 좌표를 브라우저 마우스로 누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 15000 });
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

// Tutorial1 진입 → 안내 닫기 → 게임 시계 시작까지 진행한다.
async function startTutorial(page) {
  await page.addInitScript(() => { localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } }));
    localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: { DoneTutorial1: '1' } })); });
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu'); await clickLabel(page, 'Campaign'); await clickLabel(page, 'Early Missions');
  const first = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'Tutorial1')?.label);
  expect(first).toBeTruthy(); await clickLabel(page, first);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 50000 });
  await page.waitForFunction(() => Boolean(window.netstorm._mainMenu.dialog), null, { timeout: 15000 });
  for (let index = 0; index < 8; index += 1) {
    const label = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons.at(-1)?.label ?? null);
    if (label === null) break; await clickLabel(page, label);
  }
  await page.waitForFunction(() => !window.netstorm._gameClock.paused() && window.netstorm._campaignProduction?.ui?.knowledge(), null, { timeout: 20000 });
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




test('Shift-F7 은 현재 화면을 GIF 로 내려받고 표식 토글은 건드리지 않는다', async ({ page }) => {
  const errors = []; page.on('pageerror', (error) => errors.push(error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await startTutorial(page);
  const before = await page.evaluate(() => ({ banners: window.netstorm._clientColorSort.ownerTables.enabled, size: [window.netstorm._clientPaletteScreen.width, window.netstorm._clientPaletteScreen.height] }));
  const [download] = await Promise.all([page.waitForEvent('download', { timeout: 15000 }), page.keyboard.press('Shift+F7')]);
  expect(download.suggestedFilename()).toBe('pic0.gif');
  const saved = test.info().outputPath('pic0.gif'); await download.saveAs(saved);
  const bytes = fs.readFileSync(saved);
  expect(bytes.subarray(0, 6).toString('latin1')).toBe('GIF87a'); expect(bytes[bytes.length - 1]).toBe(0x3b);
  expect([bytes.readUInt16LE(6), bytes.readUInt16LE(8)]).toEqual(before.size);
  // 화면에는 섬·사이드바 등 여러 색이 있어 색 번호가 하나가 아니고 파일이 충분히 크다.
  expect(bytes.length).toBeGreaterThan(20000);
  // 일반 F7(표식 토글)과 달리 소유자 표식 값이 그대로다. 다시 누르면 번호가 올라간다.
  expect((await page.evaluate(() => window.netstorm._clientColorSort.ownerTables.enabled))).toBe(before.banners);
  // 원본처럼 저장 뒤 TellGifWritten 안내 창이 열린다. 닫아야 게임 입력이 이어진다.
  await page.waitForFunction(() => Boolean(window.netstorm._mainMenu.dialog) && window.netstorm._gameData.currentTell === 'TellGifWritten', null, { timeout: 10000 });
  await clickLabel(page, await page.evaluate(() => window.netstorm._mainMenu.dialog.buttons.at(-1).label));
  await page.waitForFunction(() => !window.netstorm._mainMenu.dialog && !window.netstorm._gameClock.paused(), null, { timeout: 10000 });
  const [second] = await Promise.all([page.waitForEvent('download', { timeout: 15000 }), page.keyboard.press('Shift+F7')]);
  expect(second.suggestedFilename()).toBe('pic1.gif');
  expect(errors).toEqual([]);
});
