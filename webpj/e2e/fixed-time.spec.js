// 검사 전용 고정 시각 모드(?fixed=25)의 결정론을 실제 Chromium 에서 확인한다: 같은 입력 단계표의 두 실행이 같은 시계·난수·Session 덤프를 낸다(cpppj `--fixed-frame-ms 25` 와 같은 L2·L3 대조의 바탕).
// 시작 그림 구간(3초씩 세 장)은 시각만 흐르고 프레임으로 세지 않아 정확히 360프레임·게임 시각 9초이며(원본 cpppj 의 고정 프레임 실행과 같음), 이어 Tutorial1 을 시작해 90프레임을 진행한다. (2026-11-23 WB04-E1271)
const { test, expect } = require('@playwright/test');

// 새 페이지에서 고정 시각 모드로 시작 그림을 끝내고 Tutorial1 을 시작해 mission 프레임 수만큼 진행한 뒤 덤프와 시작 그림 정보를 돌려준다.
async function run(browser, missionFrames) {
  const context = await browser.newContext({ viewport: { width: 1024, height: 768 }, locale: 'en-US' }), page = await context.newPage();
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, autoDemo: 0 } })));
  await page.goto('/?fixed=25&aspect=4:3&title=1');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 });
  await page.locator('canvas').click();
  // 효과음 준비는 비동기라 프레임을 세기 전에 마친다(준비가 끝나지 않은 채 프레임을 돌리면 시작 시점이 달라진다).
  await page.evaluate(() => window.netstorm._prepareCampaignAudio());
  let titleFrames = 0;
  while ((await page.evaluate(() => window.netstorm.state)) !== 'mainMenu' && titleFrames < 1000) { await page.evaluate(() => window.netstorm.stepFixedFrames(1)); titleFrames += 1; }
  const titleGame = await page.evaluate(() => window.netstorm._gameClock.gameSeconds());
  await page.evaluate(() => window.netstorm._beginCampaignFromDialog('Tutorial1'));
  await page.evaluate((count) => window.netstorm.stepFixedFrames(count), missionFrames);
  const dump = await page.evaluate(() => window.netstorm.sessionDump());
  const frame = await page.evaluate(() => window.netstorm._gameClock.time().number);
  await context.close();
  return { dump, titleFrames, titleGame, frame, errors };
}

test('고정 시각 모드: 시작 그림은 360프레임·9초이고 같은 입력의 두 실행은 같은 덤프를 낸다', async ({ browser }) => {
  test.setTimeout(300000);
  const first = await run(browser, 90), second = await run(browser, 90);
  for (const result of [first, second]) {
    expect(result.titleFrames, '시작 그림 프레임 수').toBe(360);
    expect(result.titleGame, '시작 그림 뒤 게임 시각').toBeCloseTo(9, 6);
    expect(result.frame, '주 루프 프레임 번호(시작 그림은 세지 않음)').toBe(90);
    expect(result.errors).toEqual([]);
  }
  expect(second.dump, '두 실행의 Session 덤프').toBe(first.dump);
});

test('고정 시각 모드가 아니면 프레임을 직접 진행할 수 없다', async ({ page }) => {
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, autoDemo: 0 } })));
  await page.goto('/?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 });
  const message = await page.evaluate(async () => { try { await window.netstorm.stepFixedFrames(1); return ''; } catch (error) { return error.message; } });
  expect(message).toMatch(/고정 시각 모드/);
});
