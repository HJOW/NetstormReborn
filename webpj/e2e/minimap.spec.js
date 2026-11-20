// 좌하단 미니맵을 클릭하면 카메라가 그 위치로 가고 눌러서 끌면 계속 따라 움직이는지, 사제 시점(F5)으로 돌아올 수 있는지 실제 Chromium 에서 확인한다. (2026-11-20 WB04-E1128)
const { test, expect } = require('@playwright/test');

// 실제 돌 버튼의 논리 좌표를 브라우저 마우스 좌표로 바꿔 누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 15000 });
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}
// 현재 카메라의 월드 좌표(float 두 개)이다.
const camera = (page) => page.evaluate(() => { const snapshot = window.netstorm._clientGameplay.camera().snapshot(); return [snapshot.data.getFloat32(0, true), snapshot.data.getFloat32(4, true)]; });

test('미니맵 클릭과 끌기가 카메라를 움직이고 F5 가 사제 시점으로 되돌린다', async ({ page }) => {
  test.setTimeout(120000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => { localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } }));
    const done = {}; for (const name of ['Tutorial1', 'Tutorial2', 'Tutorial3', 'Tutorial4', 'Tutorial5', 'Tutorial6']) done[`Done${name}`] = '1'; localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: done })); });
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Struggle For Freedom');
  const label = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'TheWarBegins')?.label);
  await clickLabel(page, label);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 60000 });
  for (let index = 0; index < 6; index += 1) { const button = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons?.at(-1)?.label ?? null); if (button === null) break; await clickLabel(page, button); await page.waitForTimeout(400); }
  await page.waitForTimeout(1500);
  const home = await camera(page);
  // 미니맵 왼쪽 위 모서리 쪽을 누르면 카메라가 멀리 이동한다.
  await page.mouse.click(20, 700, { delay: 80 }); await page.waitForTimeout(1200);
  const clicked = await camera(page); expect(Math.hypot(clicked[0] - home[0], clicked[1] - home[1])).toBeGreaterThan(20);
  // 누른 채 끌면 카메라가 계속 따라간다.
  await page.mouse.move(60, 740); await page.mouse.down(); await page.mouse.move(40, 720, { steps: 6 }); await page.mouse.up(); await page.waitForTimeout(1200);
  const dragged = await camera(page); expect(Math.hypot(dragged[0] - clicked[0], dragged[1] - clicked[1])).toBeGreaterThan(10);
  // F5(내 사제 보기)가 시작 시점으로 되돌린다.
  await page.keyboard.press('F5'); await page.waitForTimeout(1500);
  const back = await camera(page); expect(Math.hypot(back[0] - home[0], back[1] - home[1])).toBeLessThan(Math.hypot(dragged[0] - home[0], dragged[1] - home[1]));
  expect(errors).toEqual([]);
});
