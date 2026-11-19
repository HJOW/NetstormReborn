// 실제 브라우저 포인터·키 사건이 앱의 같은 폴링 소유자에 반영된다.
const { test, expect } = require('@playwright/test');

test('브라우저 포인터와 보조 키 폴링', async ({ page }) => {
  const errors = []; page.on('pageerror', (error) => errors.push(String(error)));
  await page.goto('/'); await page.waitForFunction(() => window.netstorm?.state === 'clickToStart');
  const canvas = page.locator('canvas'); const bounds = await canvas.boundingBox();
  await page.mouse.move(bounds.x + bounds.width / 2, bounds.y + bounds.height / 2); await page.mouse.down();
  const down = await page.evaluate(() => window.netstorm._clientInputPoll.poll(0x10000));
  expect(down.code).toBe(0x40010000); expect(down.x).toBeGreaterThan(0); expect(down.y).toBeGreaterThan(0);
  await page.mouse.up(); expect((await page.evaluate(() => window.netstorm._clientInputPoll.poll(0x10000))).code).toBe(0x80010000);
  await page.keyboard.down('Control'); expect((await page.evaluate(() => window.netstorm._clientInputPoll.poll(0x110000))).code).toBe(0x40110000);
  await page.keyboard.up('Control'); expect((await page.evaluate(() => window.netstorm._clientInputPoll.poll(0x110000))).code).toBe(0x80110000);
  expect(errors).toEqual([]);
});
