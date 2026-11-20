// 미션에서 유닛·건물을 우클릭해 연 컨텍스트 메뉴가 바깥 클릭으로 닫히는지 실제 Chromium 의 마우스 조작으로 확인한다. (2026-11-21 WB04-E1135)
// 원본 UberGump 는 Gump 가 소비하지 않은 좌·우 버튼 누름을 팝업 닫기로 처리한 뒤 같은 사건을 월드로 넘긴다. 연결 전에는 메뉴를 연 채 바깥을 눌러도 메뉴가 남았다.
const { test, expect } = require('@playwright/test');

// 실제 돌 버튼의 논리 좌표를 브라우저 마우스 좌표로 바꿔 누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 30000 });
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

test('우클릭 컨텍스트 메뉴는 바깥을 좌·우클릭하면 닫히고 다른 객체 우클릭은 새 메뉴로 바뀐다', async ({ page }) => {
  test.setTimeout(240000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => {
    localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } }));
    const done = {}; for (const name of ['Tutorial1', 'Tutorial2', 'Tutorial3', 'Tutorial4', 'Tutorial5', 'Tutorial6']) done[`Done${name}`] = '1'; localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: done }));
  });
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  // The War Begins! 를 시작하고 안내 창을 모두 닫는다.
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Struggle For Freedom');
  const label = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'TheWarBegins')?.label);
  await clickLabel(page, label);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
  for (let index = 0; index < 8; index += 1) {
    const button = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons?.at(-1)?.label ?? null);
    if (button === null) break; await clickLabel(page, button); await page.waitForTimeout(400);
  }
  await page.waitForTimeout(1500);
  // 화면 격자를 원본 선택기(world.pick)로 훑어 내 사제·신전이 잡히는 화면 좌표를 찾고, 아무것도 잡히지 않는 빈 지점도 찾는다.
  const spots = await page.evaluate(() => {
    const app = window.netstorm, world = app._campaignWorld, types = app._gameAssets.types(), mine = {}; const sids = {};
    for (const snapshot of world.session().world().objects()) { const name = types[snapshot.object.type - 70]?.assetName; if (snapshot.object.owner === 1 && (name === 'priest' || name === 'rainVortex')) sids[snapshot.sid] = name; }
    const hits = {}; let empty = null;
    for (let y = 10; y < 760; y += 6) for (let x = 90; x < 1020; x += 6) { const sid = world.pick(x, y, 0); if (!sid) { if (!empty && y > 560 && x > 600) empty = [x, y]; continue; } if (sids[sid]) (hits[sids[sid]] ??= []).push([x, y]); }
    const mean = (points) => points && { x: Math.round(points.reduce((sum, point) => sum + point[0], 0) / points.length), y: Math.round(points.reduce((sum, point) => sum + point[1], 0) / points.length) };
    const rect = app.canvas.getBoundingClientRect();
    return { priest: mean(hits.priest), temple: mean(hits.rainVortex), empty: empty && { x: empty[0], y: empty[1] }, left: rect.left, top: rect.top, w: rect.width, h: rect.height, lw: app._layout.logicalWidth, lh: app._layout.logicalHeight };
  });
  expect(spots.priest).toBeTruthy(); expect(spots.temple).toBeTruthy(); expect(spots.empty).toBeTruthy();
  const at = (point) => ({ x: spots.left + point.x * spots.w / spots.lw, y: spots.top + point.y * spots.h / spots.lh });
  // 루트 직계 팝업(0x4000) 개수를 읽는다.
  const popups = () => page.evaluate(() => { const root = window.netstorm._clientGump; let count = 0; for (let token = root.root().child; token; token = root.node(token).next) if (root.node(token).flags & 0x4000) count += 1; return count; });
  const click = async (point, button) => { const target = at(point); await page.mouse.move(target.x, target.y); await page.mouse.click(target.x, target.y, { button, delay: 60 }); await page.waitForTimeout(500); };

  // 사제 우클릭 → 메뉴가 열리고, 빈 곳을 왼쪽 클릭하면 닫힌다.
  await click(spots.priest, 'right'); expect(await popups(), '사제 우클릭 메뉴').toBe(1);
  await click(spots.empty, 'left'); expect(await popups(), '왼쪽 바깥 클릭').toBe(0);
  // 신전 우클릭 → 메뉴가 열리고, 빈 곳을 오른쪽 클릭하면 닫힌다.
  await click(spots.temple, 'right'); expect(await popups(), '신전 우클릭 메뉴').toBeGreaterThanOrEqual(1);
  await click(spots.empty, 'right'); expect(await popups(), '오른쪽 바깥 클릭').toBe(0);
  // 메뉴를 연 채 다른 객체를 우클릭하면 이전 메뉴는 닫히고 새 메뉴 하나만 남는다.
  await click(spots.priest, 'right'); expect(await popups()).toBe(1);
  await click(spots.temple, 'right'); expect(await popups(), '다른 객체 우클릭').toBeGreaterThanOrEqual(1);
  const stacked = await popups(); await click(spots.priest, 'right'); expect(await popups(), '메뉴가 겹쳐 쌓이면 안 된다').toBeLessThanOrEqual(stacked);
  await click(spots.empty, 'left'); expect(await popups()).toBe(0);
  expect(errors).toEqual([]);
});
