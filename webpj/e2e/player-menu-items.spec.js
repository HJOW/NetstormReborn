// 스커미시의 Players ▸ (Player 이름) 하위 메뉴가 실제로 어떤 항목(명령 번호)을 만드는지 Chromium 에서 확인한다. (2026-11-22 WB04-E1167)
// 원본 Player 메뉴(004c9700)는 단독 미션에서 `Ignore chat`(0x20000ee/ef)·`Whisper To`(0x2000027/0x2000101)·`View Player Info`(0x200009b) 를 만들지 않는다.
// 미션이면 채팅 항목이 자기 자신에게만 붙고(그것도 `subscriber !== 나` 조건이 막는다) 정보 보기는 메타맵 전용이다. 웹은 단독 미션뿐이므로 이 세 항목은 도달할 수 없다는 판정을 실제 항목 목록으로 고정한다.
// 같은 메뉴의 `Send Money` 하위는 300/700/1000/X(0x2000100)/All(미션이면 Tell 0x2000095)을 만들며 이 중 Send X 만 숫자 입력 창이 필요하다.
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
// 메뉴 작성기에 쓴 항목 라벨(구분선은 빈 문자열)이다. Player 하위 메뉴는 이 기록이 없으므로 노드 항목의 명령 번호로 본다.
const labelsOf = (page, token) => page.evaluate((current) => window.netstorm._campaignProduction.ui.gameMenu().labels(current), token);
// 메뉴 노드의 실제 항목(명령 번호·인자·자식 메뉴 번호)이다. 구분선과 제목은 명령이 0 이다.
const itemsOf = (page, token) => page.evaluate((current) => { const node = window.netstorm._clientGump.node(current); return node.items.slice(0, node.itemCount).map((item) => ({ command: item.command >>> 0, parameter: item.parameter >>> 0, child: item.childId || 0 })); }, token);
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

// 원본 Player 메뉴가 만들지 않아야 하는 채팅·정보 명령 번호다(상태 0/1 두 가지씩).
const CHAT_AND_INFO = [0x020000ee, 0x020000ef, 0x02000027, 0x02000101, 0x0200009b];
const FIND_PRIEST = 0x020000b8, SEND_X = 0x02000100;

test('스커미시 Players 하위 메뉴: 채팅·정보 항목 없이 Find Priest 와 Send Money 만 나오고 Send X 가 있다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, autoDemo: 0, skirmishAiCount: 3, skirmishAi2Ally: 0, skirmishAi3Ally: 0, skirmishAi4Ally: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3&lang=en');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 });
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Skirmish'); await clickLabel(page, 'Start');
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
  await clickLabel(page, 'Go!'); await page.waitForTimeout(2500);

  // Players(네 번째) 메뉴의 행 목록이다. 행마다 그 Player 의 하위 메뉴(원본 Player 메뉴)가 열린다.
  const token = await ensureBar(page); const players = await clickRow(page, token, 3); expect(players, 'Players 하위 메뉴').toBeTruthy();
  const rows = await labelsOf(page, players); expect(rows).toEqual(['You', 'Computer #1', 'Computer #2', 'Computer #3']);
  for (let index = 0; index < rows.length; index += 1) {
    const bar = await ensureBar(page); const list = await clickRow(page, bar, 3), child = await clickRow(page, list, index); expect(child, `${rows[index]} 하위 메뉴`).toBeTruthy();
    const items = await itemsOf(page, child), commands = items.map((item) => item.command);
    // 어느 Player 의 메뉴에도 채팅·정보 명령이 없고(자기 자신 포함), 첫 항목은 사제 찾기이다.
    for (const command of CHAT_AND_INFO) expect(commands, `${rows[index]} 에 0x${command.toString(16)} 항목이 없어야 한다`).not.toContain(command);
    expect(commands[0], `${rows[index]} 첫 항목`).toBe(FIND_PRIEST);
    if (index === 0) { expect(items.length, '자기 자신 메뉴는 사제 찾기뿐').toBe(1); continue; }
    // 다른 Player 는 Send Money 하위(자식 메뉴)가 있고, 그 안에 Send X 가 있다.
    const send = items.findIndex((item) => item.child !== 0); expect(send, `${rows[index]} Send Money 하위`).toBeGreaterThan(0);
    const sendMenu = await clickRow(page, child, send), sendItems = await itemsOf(page, sendMenu), sendCommands = sendItems.map((item) => item.command);
    expect(sendCommands, `${rows[index]} Send Money 항목`).toContain(SEND_X); expect(sendCommands.filter((command) => command !== 0).length, '고정 300/700/1000·X·All 다섯 행').toBe(5);
    expect(sendItems.find((item) => item.command === SEND_X).parameter, 'Send X 대상 owner').toBe(index + 1);
  }
  expect(errors).toEqual([]);
});
