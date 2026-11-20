// 한국어 모드의 미션 화면에서 ESC 메뉴 막대·Game/View/Options 항목과 Shift-F9 정지 안내가 한국어로 나오는지 실제 Chromium 에서 확인한다. (2026-11-20 WB04-E1118)
const { test, expect } = require('@playwright/test');

// 실제 게임 조작부의 논리 좌표를 브라우저 마우스 좌표로 바꿔 누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 30000 });
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
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

test('한국어 미션 화면: ESC 막대·Game/View/Options 항목과 정지 안내가 한국어이다', async ({ page }) => {
  test.setTimeout(180000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?lang=ko&aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 });
  await clickLabel(page, '캠페인'); await clickLabel(page, '자유를 위한 투쟁');
  const mission = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'TheWarBegins')?.label);
  await clickLabel(page, mission);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 50000 });
  await page.waitForFunction(() => Boolean(window.netstorm._mainMenu.dialog?.content), null, { timeout: 15000 });
  // 시작 직후 진행 창 문구가 한국어였는지 확인할 수는 없으므로(이미 지나감) 번역 장치에서 직접 확인한다.
  expect(await page.evaluate(() => window.netstorm._resources.translate('Starting Mission...'))).toBe('미션 시작 중...');
  await clickLabel(page, '미션 시작'); await page.waitForFunction(() => window.netstorm._gameClock.gameSeconds() > 1 && window.netstorm._campaignProduction?.ui?.warnings(), null, { timeout: 30000 });
  // 막대를 열어 라벨을 읽는다.
  await page.locator('[data-netstorm="game-menu-button"]').click(); await page.waitForTimeout(400);
  const bar = await page.evaluate(() => window.netstorm._campaignProduction.ui.gameMenu().token()), top = await labelsOf(page, bar);
  expect(top).toEqual(expect.arrayContaining(['게임', '보기', '옵션', '플레이어', '정보']));
  // Game 하위: 미션 목표·다시 시작·나가기가 한국어이다.
  const game = await clickRow(page, bar, top.indexOf('게임')), gameLabels = await labelsOf(page, game);
  expect(gameLabels.join('|')).toContain('미션 다시 시작'); expect(gameLabels.join('|')).toContain('미션 나가기');
  await page.screenshot({ path: test.info().outputPath('korean-ingame-game.png') });
  // View 하위: 단축키 표기(~iF4~)를 보존한 한국어 항목이 있다.
  const view = await clickRow(page, bar, top.indexOf('보기')), viewLabels = await labelsOf(page, view);
  expect(viewLabels.join('|')).toMatch(/건물 숨기기/u); expect(viewLabels.join('|')).toMatch(/플레이어 목록 보기/u);
  await page.screenshot({ path: test.info().outputPath('korean-ingame-view.png') });
  // 막대를 닫고 Shift-F9 로 정지하면 오른쪽 위 안내가 한국어이다.
  await page.locator('[data-netstorm="game-menu-button"]').click(); await page.waitForTimeout(300);
  await page.keyboard.press('Shift+F9'); await page.waitForFunction(() => window.netstorm._gameClock.paused(), null, { timeout: 10000 }); await page.waitForTimeout(400);
  const report = await page.evaluate(() => window.netstorm._campaignProduction.ui.warnings().report());
  expect(report).toContain('게임 일시 정지!');
  await page.screenshot({ path: test.info().outputPath('korean-ingame-paused.png') });
  await page.keyboard.press('Shift+F9'); await page.waitForFunction(() => !window.netstorm._gameClock.paused(), null, { timeout: 10000 });
  expect(errors).toEqual([]);
});
