// 한국어 모드의 ESC 게임 메뉴(Game·View·Options·Players·About)와 모든 하위 메뉴 행을 실제 Chromium 에서 모두 열어, 번역되지 않은 영어 행이 남지 않았고 Players 행이 깨지지 않았는지 확인한다. (2026-11-23 WB04-E1290, 사용자 제보 점검)
// 영어 라벨 판정은 라벨의 영문 낱말(3자 이상)이 허용 목록(언어 이름·FPS 같은 고유 기술 이름)에 없을 때만 실패로 본다. cpppj 1329 의 점검 방식과 같다.
const { test, expect } = require('@playwright/test');

// 실제 게임 조작부의 논리 좌표를 브라우저 마우스 좌표로 바꿔 누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 30000 });
  const point = await page.evaluate((text) => { const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect(); return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height }; }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}
// 항목을 고르면 막대가 닫히므로 닫혀 있으면 우측 상단 버튼으로 다시 열고 현재 막대 토큰을 돌려준다.
async function ensureBar(page) {
  if (!(await page.evaluate(() => window.netstorm._campaignProduction.ui.gameMenu().isOpen()))) { await page.locator('[data-netstorm="game-menu-button"]').click(); await page.waitForTimeout(300); }
  return page.evaluate(() => window.netstorm._campaignProduction.ui.gameMenu().token());
}
// 메뉴 작성기에 쓴 항목 라벨이다(구분선은 빈 문자열).
const labelsOf = (page, token) => page.evaluate((current) => window.netstorm._campaignProduction.ui.gameMenu().labels(current), token);
// 메뉴 항목 한 줄의 화면 좌표와 자식 메뉴 토큰이다.
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
// 하위 메뉴가 있는 행만 눌러 내려가며 모든 메뉴의 라벨을 모은다. 명령 행은 누르지 않는다. path 는 사람이 읽을 위치 표시이다.
async function collect(page, token, path, out, depth = 0) {
  const labels = await labelsOf(page, token); out.push({ path, labels });
  for (let row = 0; row < labels.length; row += 1) {
    if (!labels[row]) continue;
    const { child } = await rowPoint(page, token, row);
    if (!child) continue;
    await clickRow(page, token, row);
    if (depth < 3) await collect(page, child, `${path} > ${labels[row].replace(/~i|~\./gu, '')}`, out, depth + 1);
  }
}

// 영문 낱말 3자 이상이 남은 라벨을 찾는다. 단축키 표기(~iF4~.)·서식 자리표시자는 먼저 뗀다. 허용 목록은 번역하지 않는 고유 이름이다.
const ALLOWED = new Set(['FPS', 'English', 'Deutsch', 'NetStorm', 'Netstorm']);
function englishLeft(label) {
  const stripped = label.replace(/~i[^~]*~\./gu, '').replace(/~[a-zA-Z.]/gu, '').replace(/%[sd]\d*/gu, '');
  return (stripped.match(/[A-Za-z]{3,}/gu) ?? []).filter((word) => !ALLOWED.has(word));
}

test('한국어 ESC 게임 메뉴의 모든 행이 번역되고 Players 행이 깨지지 않는다', async ({ page }) => {
  test.setTimeout(420000); const errors = []; page.on('pageerror', (error) => errors.push(error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
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
  // 막대의 다섯 메뉴를 하나씩 열어 하위까지 모든 행을 모은다. 메뉴마다 막대를 새로 열어 다른 메뉴의 펼침이 섞이지 않게 한다.
  const all = [];
  for (const [index, name] of ['게임', '보기', '옵션', '플레이어', '정보'].entries()) {
    const token = await ensureBar(page), top = await labelsOf(page, token); expect(top[index], `막대 ${index}번째 메뉴`).toBe(name);
    const child = await clickRow(page, token, index); expect(child, `${name} 하위 메뉴`).toBeTruthy();
    await collect(page, child, name, all);
    await page.screenshot({ path: test.info().outputPath(`korean-esc-${index}-${name}.png`) });
    await page.locator('[data-netstorm="game-menu-button"]').click(); await page.waitForTimeout(300);
  }
  await test.info().attach('labels.json', { body: JSON.stringify(all, null, 1), contentType: 'application/json' });
  // 모든 행의 영어 낱말을 모아 번역 누락을 찾는다. 제어 문자는 Players 행 깨짐의 신호이다.
  const left = []; let rows = 0;
  // eslint-disable-next-line no-control-regex -- NUL/제어문자 검출 자체가 이 검사의 목적이다.
  for (const { path, labels } of all) for (const label of labels) { if (!label) continue; rows += 1; const words = englishLeft(label); if (words.length) left.push(`${path}: ${JSON.stringify(label)}`); expect(label, `${path} 행 글자`).not.toMatch(/[\u0000-\u001f]/u); }
  expect(rows, '수집한 메뉴 행').toBeGreaterThan(40);
  expect(left, '영어가 남은 행').toEqual([]);
  expect(all.find((menu) => menu.path === '플레이어')?.labels.some((label) => label.includes('간수')), 'Players 행에 적 이름').toBe(true);
  expect(errors).toEqual([]);
});
