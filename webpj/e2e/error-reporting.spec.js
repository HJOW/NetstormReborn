// 브라우저가 실제 ResizeObserver 콜백의 원인을 콘솔과 pageerror에 전달하는지 확인한다. (2026-11-19 WB04-E944)
const { test, expect } = require('@playwright/test');

// 실제 미션의 갱신 본문에서 한 번 실패해도 다음 표시 프레임과 게임 시계가 진행되는지 확인한다.
test('미션 프레임 오류를 기록한 뒤 다음 프레임을 진행한다', async ({ page }) => {
  test.setTimeout(120000);
  // 검증은 미션 진입과 짧은 대기로 끝내며 특정 적 공격·포획·희생은 수행하지 않는다.
  const pageErrors = [];
  page.on('pageerror', (error) => pageErrors.push(error.message));
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, autoDemo: 0 } })));
  await page.goto('/?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart');
  await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  await page.evaluate(async () => { await window.netstorm._prepareCampaignAudio(); window.netstorm._beginCampaignFromDialog('Tutorial1'); });
  await page.waitForFunction(() => window.netstorm._campaignDisplayActive() && Boolean(window.netstorm._clientGuide?.current()));
  // 다음 실제 프레임에서만 예외를 발생시키고 원래 본문을 바로 복원한다.
  const diagnosticPromise = page.waitForEvent('console', (message) => message.type() === 'error' && message.text().includes('Netstorm: 실행 중 오류'));
  const before = await page.evaluate(() => {
    const app = window.netstorm, original = app._onFrame;
    app._onFrame = function (now) { this._onFrame = original; throw new Error('프레임 계속 진행 검사'); };
    return app._lastFrameStamp;
  });
  const diagnostic = await diagnosticPromise;
  expect(await diagnostic.args()[1].jsonValue()).toMatchObject({ phase: 'frame', state: 'mainMenu', mission: 'Tutorial1', frameTime: expect.any(Number) });
  expect(await diagnostic.args()[2].evaluate((error) => error.message)).toBe('프레임 계속 진행 검사');
  await page.waitForFunction((previous) => window.netstorm._lastFrameStamp > previous + 100, before);
  expect(await page.evaluate(() => Boolean(window.netstorm._frameHandle) && window.netstorm._campaignDisplayActive())).toBe(true);
  expect(pageErrors).toEqual([]);
});

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
