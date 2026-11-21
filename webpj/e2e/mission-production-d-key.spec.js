// D 키(원본 도움말: "D 는 생산 창에서 마지막으로 고른 유닛을 다시 쥔다")를 실제 Chromium 에서 확인한다. (2026-11-21 WB04-E1155)
// 연결 전에는 D 가 큐에 들어가지도 않았고(문자 키 허용 목록에 없음), 들어가도 최근 항목 명령(0x02000107)이 미구현 사건으로 막혔다.
const { test, expect } = require('@playwright/test');

// 화면비를 환경 변수 NETSTORM_E2E_ASPECT('4:3' 기본, '16:9', '16:10')로 고른다.
const ASPECT = process.env.NETSTORM_E2E_ASPECT ?? '4:3';
const VIEWPORTS = { '4:3': { width: 1024, height: 768 }, '16:9': { width: 1280, height: 720 }, '16:10': { width: 1280, height: 800 } };
if (!VIEWPORTS[ASPECT]) throw new Error(`지원하지 않는 화면비 ${ASPECT}`);

// 실제 돌 버튼의 논리 좌표를 브라우저 마우스 좌표로 바꿔 누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 30000 });
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

// The War Begins! 를 시작하고 안내 창을 닫아 시계가 흐르게 한다.
async function startWarBegins(page) {
  await page.addInitScript(() => {
    localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } }));
    const done = {}; for (const name of ['Tutorial1', 'Tutorial2', 'Tutorial3', 'Tutorial4', 'Tutorial5', 'Tutorial6']) done[`Done${name}`] = '1'; localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: done }));
  });
  await page.setViewportSize(VIEWPORTS[ASPECT]); await page.goto(`/?aspect=${ASPECT}`);
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Struggle For Freedom');
  const label = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'TheWarBegins')?.label);
  await clickLabel(page, label);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
  for (let index = 0; index < 10; index += 1) {
    const button = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons?.at(-1)?.label ?? null);
    if (button === null) break; await clickLabel(page, button); await page.waitForTimeout(500);
  }
  await page.waitForFunction(() => !window.netstorm._gameClock.paused(), null, { timeout: 20000 });
}

// 새 페이지의 오류를 모은다.
function collectErrors(page) { const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (message.type() === 'error' || message.type() === 'warning') errors.push(message.text()); }); return errors; }

test('The War Begins!: D 는 생산 창에서 마지막으로 고른 다리 조각을 다시 쥔다', async ({ page }) => {
  test.setTimeout(300000); const errors = collectErrors(page); await startWarBegins(page);
  const held = () => page.evaluate(() => { const drop = window.netstorm._campaignWorld.session().cursorDropSettings(); return drop.preview !== 0 ? { type: drop.type, argument: drop.argument } : null; });
  // 생산 창의 다리 조각 항목 목록(화면 좌표)이다.
  const readItems = () => page.evaluate(() => { const app = window.netstorm, root = app._clientGump, rect = app.canvas.getBoundingClientRect(), out = [];
    for (let token = root.node(app._campaignProduction.ui.node().bridge).child; token; token = root.node(token).next) { const bounds = root.collection().bounds(token); out.push({ x: rect.left + (bounds.left + bounds.right) / 2 * rect.width / app._layout.logicalWidth, y: rect.top + (bounds.top + bounds.bottom) / 2 * rect.height / app._layout.logicalHeight }); }
    return out; });
  await expect.poll(async () => (await readItems()).length, { timeout: 15000, message: '생산 창 다리 조각 항목 수' }).toBeGreaterThanOrEqual(3);
  const items = await readItems();
  expect(items.length, '생산 창 다리 조각 항목 수').toBeGreaterThanOrEqual(3);
  // 아직 아무 항목도 고르지 않았으면 D 는 아무것도 쥐지 않는다(마지막 선택 종류가 없다).
  await page.mouse.move(540, 380); await page.keyboard.press('d'); await page.waitForTimeout(800); expect(await held(), '선택 전 D 는 효과 없음').toBeNull();
  // 세 번째 항목을 눌러 쥐고(마지막 선택이 된다) Esc 로 놓은 뒤 D 를 누르면 같은 종류의 조각을 다시 쥔다. 다리 조각은 모두 같은 종류(TYPE)라 원본처럼 그 종류의 첫 항목이 쥐어진다.
  await page.mouse.move(items[2].x, items[2].y); await page.mouse.click(items[2].x, items[2].y, { delay: 60 });
  await expect.poll(() => held(), { timeout: 10000, message: '세 번째 항목 쥐기' }).not.toBeNull(); const third = await held();
  await page.mouse.move(540, 380); await page.keyboard.press('Escape'); await expect.poll(() => held(), { timeout: 5000, message: 'Esc 로 놓기' }).toBeNull();
  await page.keyboard.press('d'); await expect.poll(() => held(), { timeout: 10000, message: 'D 로 다시 쥐기' }).not.toBeNull();
  const again = await held(); expect(again.type, 'D 로 쥔 조각은 마지막으로 고른 종류이다').toBe(third.type);
  // D 로 쥔 뒤 한 번 더 D 는 이미 쥔 조각을 그대로 둔다(예외 없음).
  await page.keyboard.press('d'); await page.waitForTimeout(500); expect((await held())?.type, '쥔 채 D').toBe(third.type);
  expect(errors).toEqual([]);
});
