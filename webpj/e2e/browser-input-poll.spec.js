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

// 실제 Canvas 사건과 포인터 폴링이 같은 앱 큐에 재진입 release를 예약한다.
test('브라우저 포인터 재진입 보정', async ({ page }) => {
  await page.goto('/'); await page.waitForFunction(() => window.netstorm?.state === 'clickToStart');
  const canvas = page.locator('canvas'), bounds = await canvas.boundingBox();
  await page.mouse.move(bounds.x + bounds.width / 2, bounds.y + bounds.height / 2);
  await page.evaluate(() => window.netstorm._prepareClientPointer(0));
  await page.mouse.down();
  await canvas.dispatchEvent('pointerleave', { buttons: 1 });
  await page.evaluate(() => window.netstorm._prepareClientPointer(0));
  await page.mouse.up();
  const result = await page.evaluate(() => {
    const app = window.netstorm; app._prepareClientPointer(0); return [app._clientInputQueue.pop(false).code, app._clientInputQueue.empty];
  });
  expect(result).toEqual([0x80010000, true]);
});

// 원본 입력을 활성화했을 때 실제 Canvas 버튼 사건이 앱 큐에 들어간다.
test('브라우저 버튼 사건의 원본 큐 예약', async ({ page }) => {
  await page.goto('/'); await page.waitForFunction(() => window.netstorm?.state === 'clickToStart');
  await page.evaluate(() => { window.netstorm._clientNativeInputEnabled = true; });
  const canvas = page.locator('canvas'), bounds = await canvas.boundingBox();
  await page.mouse.move(bounds.x + bounds.width / 2, bounds.y + bounds.height / 2);
  await page.mouse.down(); await page.mouse.up();
  const events = await page.evaluate(() => {
    const queue = window.netstorm._clientInputQueue; return [queue.pop(false), queue.pop(false), queue.empty];
  });
  expect(events[0].code).toBe(0x00010000); expect(events[1].code).toBe(0x80010000);
  expect(events[0].x).toBe(events[1].x); expect(events[0].y).toBe(events[1].y); expect(events[2]).toBe(true);
});

// 실제 Canvas의 방향키 누름과 뗌이 원본 가상 키 코드로 예약된다.
test('브라우저 비문자 키의 원본 큐 예약', async ({ page }) => {
  await page.goto('/'); await page.waitForFunction(() => window.netstorm?.state === 'clickToStart');
  const canvas = page.locator('canvas'), bounds = await canvas.boundingBox();
  await page.mouse.move(bounds.x + bounds.width / 2, bounds.y + bounds.height / 2); await canvas.focus();
  await page.evaluate(() => { window.netstorm._clientNativeInputEnabled = true; });
  await page.keyboard.down('ArrowRight'); await page.keyboard.up('ArrowRight');
  const codes = await page.evaluate(() => { const queue = window.netstorm._clientInputQueue; return [queue.pop(false).code, queue.pop(false).code, queue.empty]; });
  expect(codes).toEqual([0x00270000, 0x80270000, true]);
});
