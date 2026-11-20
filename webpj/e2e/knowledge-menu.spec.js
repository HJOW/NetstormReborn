// 미션 중 F6 으로 여는 지식 창(Storm 카드)과 ESC 닫기, 게임 메뉴 View 항목 연결을 실제 Chromium 에서 확인한다. (2026-11-20 WB04-E1088)
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

// 지식 창 열림 여부와 만든 카드 수(머리 네 개 + 기술 카드)다.
const knowledgeState = (page) => page.evaluate(() => { const k = window.netstorm._campaignProduction.ui.knowledge(); return { open: k.isOpen(), barOpen: window.netstorm._campaignProduction.ui.gameMenu().isOpen() }; });

test('F6 은 지식 창을 열고 ESC 는 팝업만 닫는다', async ({ page }) => {
  const errors = []; page.on('pageerror', (error) => errors.push(error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await startTutorial(page);
  expect((await knowledgeState(page)).open).toBe(false);
  await page.keyboard.press('F6');
  await page.waitForFunction(() => window.netstorm._campaignProduction.ui.knowledge().isOpen(), null, { timeout: 10000 });
  await page.waitForTimeout(600);
  await page.screenshot({ path: process.env.KNOWLEDGE_SHOT ?? 'test-results/knowledge-menu.png' });
  // 열린 상태에서 ESC 를 누르면 지식 팝업만 닫히고 메뉴막대는 열리지 않는다.
  await page.keyboard.press('Escape'); await page.waitForTimeout(400);
  const closed = await knowledgeState(page); expect(closed.open).toBe(false); expect(closed.barOpen).toBe(false);
  expect(errors).toEqual([]);
});

test('게임 메뉴 View 의 지식 보기 항목이 지식 창을 연다', async ({ page }) => {
  const errors = []; page.on('pageerror', (error) => errors.push(error.message));
  await startTutorial(page);
  const bar = await ensureBar(page), top = await labelsOf(page, bar), view = await clickRow(page, bar, top.findIndex((label) => /View/u.test(label)));
  const labels = await labelsOf(page, view), knowledge = labels.findIndex((label) => /Knowledge/u.test(label));
  expect(knowledge).toBeGreaterThanOrEqual(0);
  await clickRow(page, view, knowledge);
  await page.waitForFunction(() => window.netstorm._campaignProduction.ui.knowledge().isOpen(), null, { timeout: 10000 });
  // 항목을 고르면 메뉴막대는 닫히고 지식 창만 남는다.
  const state = await knowledgeState(page); expect(state.open).toBe(true); expect(state.barOpen).toBe(false);
  expect(errors).toEqual([]);
});

test('선택 메뉴와 World 카드를 열고 카드와 머리 항목을 누르면 도움말이 열린다', async ({ page }) => {
  const errors = []; page.on('pageerror', (error) => errors.push(error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await startTutorial(page);
  // Storm/World 선택 메뉴(두 팝업)가 열린다.
  await page.evaluate(() => window.netstorm._clientInputQueue.pushCommand(0x02000132, 0, 0)); await page.waitForTimeout(500);
  expect(await page.evaluate(() => window.netstorm._campaignProduction.ui.knowledge().tokens().length)).toBe(2);
  expect((await knowledgeState(page)).open).toBe(true);
  // 선택 메뉴의 World 항목은 World 카드를 만든다. 이 판본에는 World 원소 TYPE 이 없어 네 머리(EARTH·FIRE·WATER.·MOON)만 나온다.
  await page.evaluate(() => window.netstorm._clientInputQueue.pushCommand(0x02000133, 0, 0)); await page.waitForTimeout(500);
  const world = await page.evaluate(() => window.netstorm._campaignProduction.ui.knowledge().tokens().length); expect(world).toBe(4);
  await page.screenshot({ path: process.env.KNOWLEDGE_SHOT_WORLD ?? 'test-results/knowledge-world.png' });
  // Storm 카드를 다시 열고 카드를 누르면 TYPE 도움말이 열리고 지식 창은 닫힌다.
  await page.evaluate(() => window.netstorm._clientInputQueue.pushCommand(0x020000a0, 0, 0)); await page.waitForTimeout(500);
  const tokens = await page.evaluate(() => window.netstorm._campaignProduction.ui.knowledge().tokens());
  await clickRow(page, tokens[5], 0); await page.waitForTimeout(700);
  expect((await knowledgeState(page)).open).toBe(false);
  expect(await page.evaluate(() => Boolean(window.netstorm._mainMenu?.dialog))).toBe(true);
  expect(errors).toEqual([]);
});

test('원소 머리 카드의 도움말 항목은 정적 themeHelp 문자열을 가리키고 머리는 눌러도 닫히지 않는다', async ({ page }) => {
  const errors = []; page.on('pageerror', (error) => errors.push(error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await startTutorial(page);
  await page.keyboard.press('F6'); await page.waitForFunction(() => window.netstorm._campaignProduction.ui.knowledge().isOpen(), null, { timeout: 10000 });
  // 머리 카드의 항목은 0=제목, 1=구분선, 2~4=도움말 항목이다. 도움말 항목의 인자는 정적 문자열 토큰이며 'themeHelp' 를 읽는다.
  const heading = await page.evaluate(() => { const app = window.netstorm, node = app._clientGump.node(app._campaignProduction.ui.knowledge().tokens()[0]);
    return { count: node.itemCount, command: node.items[2].command, topic: app._clientChatWindows.strings.get(node.items[2].parameter), compact: (node.menuFlags & 0x1000000) !== 0 }; });
  expect(heading).toEqual({ count: 5, command: 0x02000017, topic: 'themeHelp', compact: true });
  // 압축 메뉴는 어디를 눌러도 첫 항목(비활성 제목)만 가리키므로 머리 카드를 눌러도 지식 창은 그대로 열려 있다(원본 동작).
  const target = await rowPoint(page, await page.evaluate(() => window.netstorm._campaignProduction.ui.knowledge().tokens()[0]), 2);
  await page.mouse.move(target.x, target.y); await page.mouse.click(target.x, target.y, { delay: 60 }); await page.waitForTimeout(500);
  expect((await knowledgeState(page)).open).toBe(true);
  expect(errors).toEqual([]);
});
