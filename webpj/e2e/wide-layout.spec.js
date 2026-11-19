// 실제 창 크기 변경이 논리 표면, Canvas와 입력 좌표에 반영되는지 확인한다. (2026-11-19 WB04-E943)
const { test, expect } = require('@playwright/test');

test('16:9 창을 넓혔다가 줄여도 논리 표면과 포인터 위치가 일치한다', async ({ page }) => {
  const errors = [];
  page.on('pageerror', (error) => errors.push(error.message));
  await page.setViewportSize({ width: 1280, height: 720 });
  await page.goto('/');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart');
  const canvas = page.locator('#netstorm_root canvas');

  // 시작 화면을 클릭해 실제 표시 표면을 설치한다.
  await canvas.click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  await page.setViewportSize({ width: 1920, height: 1080 });
  await expect(canvas).toHaveAttribute('width', '1920');
  await expect(canvas).toHaveAttribute('height', '1080');
  await page.mouse.move(960, 540);
  expect(await page.evaluate(() => {
    const app = window.netstorm;
    return [app._surface.width, app._surface.height, app._input.pointer.x, app._input.pointer.y];
  })).toEqual([1920, 1080, 960, 540]);
  await page.setViewportSize({ width: 1280, height: 720 });
  await expect(canvas).toHaveAttribute('width', '1280');
  await expect(canvas).toHaveAttribute('height', '720');
  expect(await page.evaluate(() => [window.netstorm._surface.width, window.netstorm._surface.height])).toEqual([1366, 768]);
  expect(errors).toEqual([]);
});
