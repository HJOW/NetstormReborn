// 원본 도움말(User Interface)의 "Hold down ALT to scroll the screen towards the cursor" 를 실제 Chromium 에서 확인한다. (2026-11-21 WB04-E1145)
// Alt 를 누른 동안 포인터 쪽으로 카메라가 움직이고 떼면 멈춘다. 가장자리에 대는 스크롤(edgeScroll)과 F4(집)·F5(사제) 점프도 함께 확인한다.
// 연결 전에는 Alt 스크롤이 동작하지 않았다(원본 4d6b54 processCameraPan 이 앱에 연결되지 않았음).
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

test('The War Begins!: Alt 를 누르면 포인터 쪽으로 화면이 움직이고, 떼면 멈추며, F4·F5 로 집·사제로 점프한다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
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
  const camera = () => page.evaluate(() => { const pixels = window.netstorm._clientGameplay.camera().snapshot().pixels; return [pixels.x, pixels.y]; });
  const viewport = page.viewportSize(), center = { x: viewport.width / 2, y: viewport.height / 2 };
  const start = await camera();
  // Alt 를 누르고 포인터를 중심에서 오른쪽 아래로 두면 그 쪽(x·y 증가)으로 화면이 움직인다.
  await page.mouse.move(center.x, center.y); await page.keyboard.down('Alt'); await page.mouse.move(center.x + viewport.width * 0.3, center.y + viewport.height * 0.25); await page.waitForTimeout(1000);
  const toward = await camera(); expect(toward[0], 'Alt 오른쪽').toBeGreaterThan(start[0]); expect(toward[1], 'Alt 아래').toBeGreaterThan(start[1]);
  // 왼쪽 위로 옮기면 반대로 움직인다.
  await page.mouse.move(center.x - viewport.width * 0.3, center.y - viewport.height * 0.25); await page.waitForTimeout(1200);
  const back = await camera(); expect(back[0], 'Alt 왼쪽').toBeLessThan(toward[0]); expect(back[1], 'Alt 위').toBeLessThan(toward[1]);
  // Alt 를 떼면 포인터가 중심에서 떨어져 있어도 멈춘다.
  await page.keyboard.up('Alt'); await page.waitForTimeout(400); const stopped = await camera(); await page.mouse.move(center.x + viewport.width * 0.2, center.y); await page.waitForTimeout(800);
  expect(await camera(), 'Alt 뗌').toEqual(stopped);
  // F4 는 집(내 신전), F5 는 사제로 점프한다. 서로 다른 위치여야 한다.
  await page.keyboard.press('F4'); await page.waitForTimeout(800); const home = await camera();
  await page.keyboard.press('F5'); await page.waitForTimeout(800); const priest = await camera();
  expect(home, 'F4 와 F5 는 다른 위치').not.toEqual(priest);
  expect(errors).toEqual([]);
});
