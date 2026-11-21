// 스커미시 AI 난이도(쉬움·보통·어려움)가 실제 게임에서 AI 의 다리·섬 건설 속도에 반영되는지 같은 시간 동안 비교한다. (2026-11-21 WB04-E1149)
// AGENTS 11차: 쉬움은 AI 다리 건설·유닛 생성 제어 주기가 4배 늘고, 어려움은 2배 빨라진다(기본 보통). 실제 진행 150초에서 쉬움 57개·보통 209개·어려움 357개 다리 조각이 관찰됐다.
// 시간이 오래 걸려(난이도마다 약 2~3분) NETSTORM_DIFF_SECONDS 를 정한 실행에서만 돈다. 사용: `$env:NETSTORM_DIFF_SECONDS=100; npx playwright test e2e/skirmish-difficulty.spec.js`.
const { test, expect } = require('@playwright/test');

// 난이도마다 진행할 게임 시간(초)이다. 설정하지 않으면 시험을 건너뛴다.
const SECONDS = Number(process.env.NETSTORM_DIFF_SECONDS ?? 0);

// 실제 돌 버튼·메뉴 항목의 논리 좌표를 브라우저 마우스로 누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 30000 });
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

// AI(소유자 2)가 지은 다리 조각·다리 연결부·섬·빈 섬 조각의 합이다. 지형 바닥(`isle`)은 지도마다 크게 달라 세지 않는다.
const built = (page) => page.evaluate(() => {
  const app = window.netstorm, types = app._gameAssets.types(); let total = 0;
  for (const { object } of app._campaignWorld.session().world().objects()) {
    if (object.owner !== 2) continue;
    const name = types[object.type - 70]?.assetName; if (name === 'bridge' || name === 'bridgeConnector' || name === 'island' || name === 'noIsland') total += 1;
  }
  return total;
});

test('스커미시 AI 난이도: 어려움 > 보통 > 쉬움 순으로 같은 시간에 더 많이 짓는다', async ({ context }) => {
  test.skip(!(SECONDS > 0), 'NETSTORM_DIFF_SECONDS 를 설정한 실행에서만 돈다');
  test.setTimeout((SECONDS + 150) * 3 * 1000);
  const errors = [];
  const growth = {};
  // 난이도 값: 1 쉬움·2 보통·3 어려움(core/ai-difficulty.js). 같은 설정 키 skirmishAi2Diff 로 AI 한 명의 난이도를 정한다.
  for (const [name, value] of [['easy', 1], ['normal', 2], ['hard', 3]]) {
    const page = await context.newPage();
    page.on('pageerror', (error) => errors.push(error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
    await page.addInitScript((level) => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, skirmishAi2Diff: level } })), value);
    await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3');
    await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
    await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 });
    await clickLabel(page, 'Campaign'); await clickLabel(page, 'Skirmish'); await clickLabel(page, 'Start');
    await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 90000 });
    await clickLabel(page, 'Go!'); await page.waitForTimeout(3000);
    const start = await built(page); await page.waitForTimeout(SECONDS * 1000); const end = await built(page);
    growth[name] = end - start; await page.close();
  }
  console.log('난이도별 AI 건설 증가', JSON.stringify(growth));
  // AI 시작 지연이 실행마다 달라 잡음이 있으므로 순서와 넉넉한 비율만 확인한다(관찰: 쉬움:보통:어려움 ≈ 1:3.7:6.3).
  expect(growth.hard, '어려움 > 보통').toBeGreaterThan(growth.normal);
  expect(growth.normal, '보통 > 쉬움').toBeGreaterThan(growth.easy);
  expect(growth.normal, '보통은 쉬움의 2배 이상').toBeGreaterThanOrEqual(growth.easy * 2);
  expect(errors).toEqual([]);
});
