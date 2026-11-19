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

// 실제 Canvas의 영문과 Enter는 누름 한 번당 원본 문자 사건 하나다.
test('브라우저 문자 키의 원본 큐 예약', async ({ page }) => {
  await page.goto('/'); await page.waitForFunction(() => window.netstorm?.state === 'clickToStart');
  const canvas = page.locator('canvas'); await canvas.focus();
  await page.evaluate(() => { window.netstorm._clientNativeInputEnabled = true; });
  await page.keyboard.press('a'); await page.keyboard.press('Enter');
  const codes = await page.evaluate(() => { const queue = window.netstorm._clientInputQueue; return [queue.pop(false).code, queue.pop(false).code, queue.empty]; });
  expect(codes).toEqual([0x61, 13, true]);
});

// 합성 조합 사건으로 확정 본문만 큐에 한 번 들어가고 중간 Enter는 실행되지 않는다.
test('브라우저 조합 확정 사건의 원본 큐 예약', async ({ page }) => {
  await page.goto('/'); await page.waitForFunction(() => window.netstorm?.state === 'clickToStart');
  const canvas = page.locator('canvas'); await canvas.focus();
  await page.evaluate(() => { window.netstorm._clientNativeInputEnabled = true; });
  await canvas.evaluate((element) => element.dispatchEvent(new CompositionEvent('compositionstart', { bubbles: true, data: '' }))); await page.keyboard.press('Enter');
  await canvas.evaluate((element) => element.dispatchEvent(new CompositionEvent('compositionend', { bubbles: true, data: '한글' })));
  const result = await page.evaluate(() => { const app = window.netstorm, queue = app._clientInputQueue; return [app.state, queue.pop(false).code, queue.pop(false).code, queue.empty]; });
  expect(result).toEqual(['clickToStart', 0x0100d55c, 0x0100ae00, true]);
});

// 편집 요소에 포커스한 조합·키 사건이 같은 큐로 가고 종료 시 Canvas로 돌아간다.
test('브라우저 IME 편집 요소의 포커스와 수명', async ({ page }) => {
  await page.goto('/'); await page.waitForFunction(() => window.netstorm?.state === 'clickToStart');
  const canvas = page.locator('canvas'); await canvas.focus();
  const activated = await page.evaluate(() => {
    const app = window.netstorm; app._clientNativeInputEnabled = true; app._input.pointer.inside = true;
    const element = app._activateClientTextInput(100, 120); return [document.activeElement === element, app._input.pointer.inside, element.tagName, element.getAttribute('aria-label')];
  });
  expect(activated).toEqual([true, true, 'TEXTAREA', '문자 입력']);
  const input = page.locator('textarea[aria-label="문자 입력"]');
  await input.evaluate((element) => element.dispatchEvent(new CompositionEvent('compositionstart', { bubbles: true, data: '' })));
  await page.keyboard.press('Enter');
  await input.evaluate((element) => element.dispatchEvent(new CompositionEvent('compositionend', { bubbles: true, data: '한글' })));
  await page.keyboard.press('a');
  const values = await page.evaluate(() => {
    const app = window.netstorm, queue = app._clientInputQueue; return [app.state, queue.pop(false).code, queue.pop(false).code, queue.pop(false).code, queue.empty, app._clientImeElement.value];
  });
  expect(values).toEqual(['clickToStart', 0x0100d55c, 0x0100ae00, 0x61, true, '']);
  expect(await page.evaluate(() => { const app = window.netstorm, old = app._clientImeElement; app._deactivateClientTextInput(); return [old.isConnected, document.activeElement === app._canvas, app._clientImeElement === null]; })).toEqual([false, true, true]);
  expect(await page.evaluate(() => { const app = window.netstorm, old = app._activateClientTextInput(5, 5); app.destroy(); return [old.isConnected, app._clientImeElement === null]; })).toEqual([false, true]);
});
