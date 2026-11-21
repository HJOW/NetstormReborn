// 한국어 모드에서 게임 메뉴 막대의 Players 메뉴 행이 한국어 이름으로 그려지는지 실제 Chromium 에서 확인한다. (2026-11-21 WB04-E1166)
// Player 이름은 한국어·일본어 확장 문자를 4바이트 시퀀스(선두 0x1f)로 담은 표시 부호화 바이트이다. 디코딩하지 않으면 메뉴 행이 깨진 글자(제어 문자)로 그려졌다.
const { test, expect } = require('@playwright/test');

// 실제 돌 버튼의 논리 좌표를 브라우저 마우스 좌표로 바꿔 누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 30000 });
  const point = await page.evaluate((text) => { const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect(); return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height }; }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

// 항목을 고르면 막대가 닫히므로(원본 동작) 닫혀 있으면 우측 상단 버튼으로 다시 열고 현재 막대 토큰을 돌려준다.
async function ensureBar(page) {
  if (!(await page.evaluate(() => window.netstorm._campaignProduction.ui.gameMenu().isOpen()))) { await page.locator('[data-netstorm="game-menu-button"]').click(); await page.waitForTimeout(300); }
  return page.evaluate(() => window.netstorm._campaignProduction.ui.gameMenu().token());
}
// 열린 메뉴막대의 현재 토큰과 열림 상태다.
const barState = (page) => page.evaluate(() => { const bar = window.netstorm._campaignProduction.ui.gameMenu(); return { open: bar.isOpen(), token: bar.token() }; });
// 메뉴 작성기에 쓴 항목 라벨(구분선은 빈 문자열이며 메뉴 처음과 연속 구분선은 작성기가 버린다)이다. 원본 표시 글자는 StyleText 로 그려져 노드 라벨 바이트에는 남지 않는다.
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


test('한국어 The War Begins!: Players 메뉴 행이 한국어 이름으로 나온다', async ({ page }) => {
  test.setTimeout(300000); const errors = []; page.on('pageerror', (error) => errors.push(error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => {
    localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, autoDemo: 0, language: 2 } }));
    const done = {}; for (const name of ['Tutorial1', 'Tutorial2', 'Tutorial3', 'Tutorial4', 'Tutorial5', 'Tutorial6']) done['Done' + name] = '1'; localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: done }));
  });
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?lang=ko&mute&aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  await clickLabel(page, '캠페인');
  const items = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.map((item) => item.label));
  await clickLabel(page, items.find((label) => label.includes('투쟁')));
  const war = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'TheWarBegins')?.label);
  await clickLabel(page, war);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
  for (let quiet = 0; quiet < 6;) { const l = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons?.at(-1)?.label ?? null); if (l === null) { quiet += 1; await page.waitForTimeout(500); continue; } quiet = 0; await clickLabel(page, l); await page.waitForTimeout(400); }
  // 메뉴 막대의 Players(네 번째) 메뉴 행과 F9 의 Player 목록 모두 한국어 이름이어야 한다. 이름(적 AI 이름 `간수`)이 들어 있고 제어 문자가 없다.
  const token = await ensureBar(page); const child = await clickRow(page, token, 3); expect(child, 'Players 하위 메뉴').toBeTruthy();
  const rows = await labelsOf(page, child);
  expect(rows.length, 'Player 행').toBeGreaterThanOrEqual(2);
  for (const row of rows) expect(row, '행 글자').not.toMatch(/[\u0000-\u001f]/u);
  expect(rows.some((row) => row.includes('간수')), 'AI 이름 행: ' + JSON.stringify(rows)).toBe(true);
  expect(errors).toEqual([]);
});
