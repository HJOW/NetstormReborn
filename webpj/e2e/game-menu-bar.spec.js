// 미션 중 ESC 로 여는 화면 위쪽 가로 게임 메뉴막대와 웹 전용 우측 상단 버튼을 실제 Chromium 에서 확인한다. (2026-11-20 WB04-E1084)
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

// Tutorial1 진입 → 안내 닫기 → 게임 시계 시작까지 진행한다. 소리·음악은 꺼 두어 설정 변경을 확인하기 쉽게 한다.
async function startTutorial(page) {
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } })));
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
  await page.waitForFunction(() => !window.netstorm._gameClock.paused() && window.netstorm._campaignProduction?.ui?.gameMenu(), null, { timeout: 20000 });
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

test('ESC와 우측 상단 버튼이 같은 게임 메뉴막대를 열고 닫으며 버튼이 메뉴보다 앞에 있다', async ({ page }) => {
  const errors = []; page.on('pageerror', (error) => errors.push(error.message));
  await startTutorial(page);
  const button = page.locator('[data-netstorm="game-menu-button"]'); await expect(button).toBeVisible();
  // 버튼은 Canvas 오른쪽 위 모서리 안쪽에 있고 Canvas 보다 높은 쌓임 순서를 가진다.
  const placement = await page.evaluate(() => { const box = document.querySelector('[data-netstorm="game-menu-button"]').getBoundingClientRect(), canvas = window.netstorm.canvas.getBoundingClientRect(),
    z = (element) => Number.parseInt(getComputedStyle(element).zIndex, 10) || 0; return { right: canvas.right - box.right, top: box.top - canvas.top, buttonZ: z(document.querySelector('[data-netstorm="game-menu-button"]')), canvasZ: z(window.netstorm.canvas) }; });
  expect(placement.right).toBeGreaterThanOrEqual(0); expect(placement.right).toBeLessThanOrEqual(12); expect(placement.top).toBeGreaterThanOrEqual(0); expect(placement.top).toBeLessThanOrEqual(12);
  expect(placement.buttonZ).toBeGreaterThan(placement.canvasZ);
  expect((await barState(page)).open).toBe(false);
  // ESC 로 연다. 막대는 지도 왼쪽(85)부터 화면 오른쪽 끝까지 위쪽에 있다.
  await page.locator('canvas').focus(); await page.keyboard.press('Escape'); await page.waitForTimeout(400);
  let state = await barState(page); expect(state.open).toBe(true);
  const bounds = await page.evaluate((token) => { const b = window.netstorm._clientGump.collection().bounds(token); return { left: b.left, top: b.top, right: b.right, bottom: b.bottom }; }, state.token);
  expect(bounds).toMatchObject({ left: 85, top: 0, right: 1024 });
  expect(await labelsOf(page, state.token)).toEqual(['Game', 'View', 'Options', 'Players', 'About']);
  // 열린 막대의 오른쪽 끝과 겹치는 버튼이 메뉴보다 앞에서 눌린다.
  const front = await page.evaluate(() => { const box = document.querySelector('[data-netstorm="game-menu-button"]').getBoundingClientRect(); return document.elementFromPoint(box.left + box.width / 2, box.top + box.height / 2)?.getAttribute('data-netstorm'); });
  expect(front).toBe('game-menu-button');
  await page.keyboard.press('Escape'); await page.waitForTimeout(300); expect((await barState(page)).open).toBe(false);
  // 버튼으로 열고 닫는다. 눌린 뒤에도 키 입력이 Canvas 로 돌아가 ESC 가 계속 동작한다.
  await button.click(); await page.waitForTimeout(300); expect((await barState(page)).open).toBe(true); await expect(button).toHaveAttribute('aria-pressed', 'true');
  await page.keyboard.press('Escape'); await page.waitForTimeout(300); expect((await barState(page)).open).toBe(false);
  await button.click(); await page.waitForTimeout(300); expect((await barState(page)).open).toBe(true); await button.click(); await page.waitForTimeout(300);
  expect((await barState(page)).open).toBe(false); await expect(button).toHaveAttribute('aria-pressed', 'false'); expect(errors).toEqual([]);
});

test('게임 메뉴막대의 Game·View·Options·About 항목이 원본 구성으로 열리고 연결된 명령이 실행된다', async ({ page }) => {
  const errors = []; page.on('pageerror', (error) => errors.push(error.message));
  await startTutorial(page); await page.locator('[data-netstorm="game-menu-button"]').click(); await page.waitForTimeout(300);
  const bar = (await barState(page)).token;
  // Game: 목표 보기·재시작·나가기(Quit Game 은 웹 범위 밖). 재시작·나가기는 아직 연결하지 않아 비활성이다.
  const game = await clickRow(page, bar, 0); expect(game).toBeTruthy();
  expect((await labelsOf(page, game)).filter(Boolean).map((text) => text.replace(/~i|~\./gu, ''))).toEqual(['Review Mission Objectives - F8', 'Restart Mission', 'Leave Mission']);
  // 목표 보기는 Tutorial 첫 안내를 다시 보인다.
  await clickRow(page, game, 0); await page.waitForFunction(() => Boolean(window.netstorm._mainMenu.dialog), null, { timeout: 10000 });
  await page.screenshot({ path: test.info().outputPath('game-menu-review.png') });
  for (let index = 0; index < 8; index += 1) { const label = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons.at(-1)?.label ?? null); if (label === null) break; await clickLabel(page, label); }
  await page.waitForFunction(() => !window.netstorm._mainMenu.dialog && !window.netstorm._gameClock.paused(), null, { timeout: 15000 });
  // View: 건물 숨김을 눌러 실제 전역을 뒤집고 다시 눌러 되돌린다.
  let token = await ensureBar(page);
  let view = await clickRow(page, token, 1); expect(view).toBeTruthy(); expect(await labelsOf(page, view)).toEqual(expect.arrayContaining([expect.stringContaining('Hide buildings'), expect.stringContaining('View Home Temple'), expect.stringContaining('View Your Priest')]));
  expect(await page.evaluate(() => window.netstorm._clientGameplay.hideBuildings().value)).toBe(0);
  const hide = (await labelsOf(page, view)).findIndex((text) => text.includes('Hide buildings')); await clickRow(page, view, hide);
  await expect.poll(() => page.evaluate(() => window.netstorm._clientGameplay.hideBuildings().value)).toBe(1);
  // Options: 소리 켜기를 눌러 설정 층 값이 바뀌고, 음량 하위 메뉴의 Volume 3 이 음량 설정을 정한다.
  token = await ensureBar(page); const options = await clickRow(page, token, 2); expect(options).toBeTruthy();
  const optionLabels = await labelsOf(page, options); expect(optionLabels).toEqual(expect.arrayContaining(['Sound On', 'Play Music', 'Wind Noise', 'Speaker Swap L/R', 'Sound Effect Volume', 'Music Volume', 'Edge Scroll in Fullscreen', 'Auto-Demo', 'Tell Tips at Startup']));
  expect(await page.evaluate(() => window.netstorm._settings.get('sound'))).toBe(0);
  await clickRow(page, options, optionLabels.indexOf('Sound On')); await expect.poll(() => page.evaluate(() => window.netstorm._settings.get('sound'))).toBe(1);
  token = await ensureBar(page); const options2 = await clickRow(page, token, 2); const volume = await clickRow(page, options2, optionLabels.indexOf('Sound Effect Volume')); expect(volume).toBeTruthy();
  await clickRow(page, volume, 2); await expect.poll(() => page.evaluate(() => window.netstorm._settings.get('soundVolume'))).toBe(3);
  // About: Version 이 Tell 창을 연다.
  token = await ensureBar(page); const about = await clickRow(page, token, 4); const aboutLabels = await labelsOf(page, about);
  expect(aboutLabels.filter(Boolean).map((text) => text.replace(/~i|~\./gu, ''))).toEqual(['General Help - F1', 'Version']);
  await clickRow(page, about, aboutLabels.indexOf('Version')); await page.waitForFunction(() => Boolean(window.netstorm._mainMenu.dialog), null, { timeout: 10000 });
  await page.screenshot({ path: test.info().outputPath('game-menu-about.png') }); expect(errors).toEqual([]);
});
