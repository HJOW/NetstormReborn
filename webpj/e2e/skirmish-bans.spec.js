// 스커미시 설정의 금지 유닛이 미션 허용 표(restrictions)에 반영되는지 실제 Chromium 에서 확인한다: 금지한 TYPE 만 금지(1)이고 나머지는 허용(0)이다. (2026-11-20 WB04-E1107)
const { test, expect } = require('@playwright/test');

// 실제 돌 버튼·메뉴 항목의 논리 좌표를 브라우저 마우스로 누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 30000 });
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

// 허용 표에서 금지(0 이 아닌 값)인 TYPE 의 자산 이름 목록이다.
const forbidden = (page) => page.evaluate(() => {
  const app = window.netstorm, table = app._clientGuide.restrictions(), types = app._gameAssets.types(), names = [];
  for (let type = 0; type < table.length; type += 1) if (table[type] !== 0) names.push(`${type}:${types[type - 70]?.assetName ?? '?'}`);
  return names;
});

test('스커미시 금지 유닛이 허용 표에 반영된다', async ({ page }) => {
  test.setTimeout(150000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, skirmishBans: 'outpost,sunCannon,bulf' } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 });
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Skirmish'); await clickLabel(page, 'Start');
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 90000 });
  // 설정한 세 금지 유닛만 허용 표에서 금지이고(TYPE 번호 126·128·135) 나머지 모든 TYPE 은 허용이다.
  expect(await forbidden(page)).toEqual(['126:outpost', '128:sunCannon', '135:bulf']);
  expect(errors).toEqual([]);
});
