// 회수 경고 창(SalvageVortex·SalvageInf)의 `Salvage It` 단추가 회수 명령(0x0200005d)으로 이어지는지 실제 Chromium 에서 확인한다. (2026-11-22 WB04-E1190)
// 객체 메뉴가 신전(flags2 0x200)·영향 건물(flags2 0x4000)의 회수 항목에 경고 Tell 을 거는데, 그 단추의 `Salvage,<sid>` 가 앱에 연결돼 있지 않아 눌러도 아무 일도 없었다.
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

test('회수 경고 창: Salvage It 단추가 그 대상으로 회수를 요청한다', async ({ page }) => {
  test.setTimeout(240000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message));
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, autoDemo: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3&lang=en');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  await page.evaluate(() => window.netstorm._beginCampaignFromDialog('tutorial2'));
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
  await page.waitForTimeout(1500);
  // 안내 창이 있으면 닫고 회수 요청을 가로채 기록한다(실제 회수는 실행하지 않는다).
  for (let index = 0; index < 20; index += 1) {
    const label = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons?.at(-1)?.label ?? null);
    if (label === null) break; await clickLabel(page, label); await page.waitForTimeout(500);
  }
  await page.evaluate(() => { const session = window.netstorm._campaignWorld.session(); window.__salvaged = []; session.requestSalvage = (sid) => { window.__salvaged.push(sid); return 1; }; });
  // 경고 창을 직접 열고(객체 메뉴가 거는 것과 같은 Tell 과 전역) 단추를 누른다.
  await page.evaluate(() => { const app = window.netstorm; app._gameData.set('global.salvageSid', '321'); app._openTell('SalvageInf'); });
  await page.waitForTimeout(500);
  const labels = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons?.map((button) => button.label) ?? null);
  expect(labels).toEqual(['Salvage It!', 'Cancel']);
  await clickLabel(page, 'Salvage It!'); await page.waitForTimeout(1200);
  expect(await page.evaluate(() => window.__salvaged)).toEqual([321]);
  // 취소는 아무것도 요청하지 않는다.
  await page.evaluate(() => { window.netstorm._openTell('SalvageVortex'); }); await page.waitForTimeout(500);
  await clickLabel(page, 'Cancel'); await page.waitForTimeout(800);
  expect(await page.evaluate(() => window.__salvaged)).toEqual([321]);
  expect(errors).toEqual([]);
});
