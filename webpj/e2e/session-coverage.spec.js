// 옵트인 수집: 고정 시각 Tutorial1(4:3) 130프레임을 도는 동안 실제로 실행된 웹 모듈을 V8 함수 커버리지로 모아 JSON 으로 쓴다.
// 환경 변수 NETSTORM_COVERAGE_OUT 에 출력 경로를 주면 실행하고, 없으면 건너뛴다. `tools/port-map-session.mjs` 가 이 파일을 읽어 대응표의 `port` 행을 `session` 으로 올린다. (2026-11-23 WB04-E1282)
const { test } = require('@playwright/test');
const fs = require('node:fs');

test('Tutorial1 130프레임 동안 실행된 웹 모듈 목록을 수집한다(옵트인)', async ({ browser }) => {
  const out = process.env.NETSTORM_COVERAGE_OUT;
  test.skip(!out, 'NETSTORM_COVERAGE_OUT 이 없어 건너뜀');
  test.setTimeout(300000);
  const context = await browser.newContext({ viewport: { width: 1024, height: 768 }, locale: 'en-US' }), page = await context.newPage();
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, autoDemo: 0 } })));
  // 페이지를 열기 전부터 모아야 모듈 적재 시점의 함수도 센다.
  await page.coverage.startJSCoverage({ resetOnNavigation: false });
  await page.goto('/?fixed=25&aspect=4:3&title=1');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 });
  await page.locator('canvas').click();
  await page.evaluate(() => window.netstorm._prepareCampaignAudio());
  while ((await page.evaluate(() => window.netstorm.state)) !== 'mainMenu') await page.evaluate(() => window.netstorm.stepFixedFrames(1));
  await page.evaluate(() => window.netstorm.stepFixedFrames(1));
  await page.evaluate(() => window.netstorm._beginCampaignFromDialog('Tutorial1'));
  await page.evaluate(() => window.netstorm.stepFixedFrames(129));
  const entries = await page.coverage.stopJSCoverage();
  await context.close();
  const modules = {};
  // 스크립트마다 본문 전체를 덮는 최상위 함수(모듈 평가)를 뺀 실행 함수 수를 센다.
  for (const entry of entries) {
    const match = /^https?:\/\/[^/]+\/(.+\.js)(?:\?.*)?$/.exec(entry.url);
    if (!match) continue;
    const length = entry.source?.length ?? 0;
    let executed = 0, total = 0;
    for (const fn of entry.functions) {
      const range = fn.ranges[0];
      if (range.startOffset === 0 && range.endOffset >= length - 1) continue;
      total += 1; if (range.count > 0) executed += 1;
    }
    modules[`www/${match[1]}`] = { executed, total };
  }
  fs.writeFileSync(out, `${JSON.stringify({ scenario: 'Tutorial1 4:3 130프레임', modules }, null, 2)}\n`, 'utf8');
});
