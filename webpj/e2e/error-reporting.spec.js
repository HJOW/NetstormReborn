// 브라우저가 실제 ResizeObserver 콜백의 원인을 콘솔과 pageerror에 전달하는지 확인한다. (2026-11-19 WB04-E944)
const { test, expect } = require('@playwright/test');

test('크기 관찰 콜백 예외에 앱 상태와 원본 원인을 기록한다', async ({ page }) => {
  await page.goto('/');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart');

  // 레이아웃 내부에서만 재현용 오류를 발생시키고 실제 브라우저 콜백을 기다린다.
  await page.evaluate(() => {
    window.netstorm._applyLayout = () => { throw new Error('레이아웃 검증 오류'); };
  });
  const [diagnostic, error] = await Promise.all([
    page.waitForEvent('console', (message) => message.type() === 'error' && message.text().includes('Netstorm: 실행 중 오류')),
    page.waitForEvent('pageerror'),
    page.setViewportSize({ width: 1024, height: 640 }),
  ]);
  expect(error.message).toBe('레이아웃 검증 오류');
  expect(await diagnostic.args()[1].jsonValue()).toMatchObject({
    phase: 'layout',
    source: 'resize-observer',
    state: 'clickToStart',
    timestamp: expect.stringMatching(/^\d{4}-\d\d-\d\dT/),
  });
  expect(await diagnostic.args()[2].evaluate((reason) => reason.message)).toBe(error.message);
});
