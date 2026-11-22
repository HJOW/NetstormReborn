// Run For It!(runforit)의 [Init] 이 쓰는 `<$ViewSpot,(139x88)>` 가 미션 시작 카메라를 그 칸으로 옮기는지 실제 Chromium 에서 확인한다. (2026-11-22 WB04-E1190)
// 이 명령이 웹에 연결돼 있지 않아 카메라가 다른 곳에서 시작했다(칸 (139,88)이 화면 한참 밖이었다). 시험은 시작 직후 그 칸이 지도 영역 가운데 근처로 투영되는지 본다.
const { test, expect } = require('@playwright/test');

test('Run For It: 시작 카메라가 ViewSpot (139,88) 을 가운데로 본다', async ({ page }) => {
  test.setTimeout(240000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message));
  page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, autoDemo: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3&lang=en');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  await page.evaluate(() => window.netstorm._beginCampaignFromDialog('runforit'));
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
  await page.waitForTimeout(1500);
  const info = await page.evaluate(() => {
    const app = window.netstorm, view = app._campaignWorld.mapView(), point = app._clientGameplay.camera().snapshot().project(139, 88);
    return { point, centerX: (view.left + view.right) / 2, centerY: (view.top + view.bottom) / 2, pending: app._pendingViewSpot };
  });
  expect(info.pending).toBeNull();
  // 그 칸의 화면 위치가 지도 영역 가운데에서 한 화면(사방 400픽셀) 안에 있어야 한다.
  expect(Math.abs(info.point.x - info.centerX)).toBeLessThan(400); expect(Math.abs(info.point.y - info.centerY)).toBeLessThan(400);
  expect(errors).toEqual([]);
});
