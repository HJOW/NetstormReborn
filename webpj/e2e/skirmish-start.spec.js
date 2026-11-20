// 12차 스커미시를 실제 Chromium 에서 시작해 월드 생성(중앙섬·시작 섬·간헐천)과 게임 진행을 확인한다: 설정 화면의 Start → 시작 섬 fort 묶음 적재 → 생성 대본·빈 월드 fort 로 미션 시작 → 세션이 미션 Start 직전에 MP 규칙으로 섬을 만듦 → 브리핑 Go! → 게임 시계가 흐름. (2026-11-20 WB04-E1105)
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

// 저장된 설정으로 스커미시를 시작해 브리핑 창까지 기다린 뒤 Go! 를 누르고 게임 시계가 흐르는지 돌려준다.
async function runSkirmish(page, values) {
  const errors = []; page.on('pageerror', (error) => errors.push(error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript((extra) => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, ...extra } })), values);
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 });
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Skirmish'); await clickLabel(page, 'Start');
  // 시작 뒤 Guide 미션이 생기고 월드가 만들어지기를 기다린다. 실패하면 모은 오류를 보여 준다.
  try {
    await page.waitForFunction(() => window.netstorm?._clientGuide?.current()?.script.name === 'Skirmish', null, { timeout: 30000 });
    await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 90000 });
  } catch (error) { throw new Error(`${error.message}\n오류: ${errors.join('\n')}`); }
  const before = await page.evaluate(() => { const app = window.netstorm; return { run: app._gameData.getInt('skirmishRun'), players: app._skirmishPlan.islands.length, central: Array.from(app._skirmishPlan.central), mapSize: app._skirmishPlan.mapSize, clock: app._gameClock.gameSeconds() }; });
  await clickLabel(page, 'Go!'); await page.waitForTimeout(6000);
  const after = await page.evaluate(() => { const app = window.netstorm; return { clock: app._gameClock.gameSeconds(), paused: app._gameClock.paused(), dialog: Boolean(app._mainMenu.dialog), missionType: app._clientGuide.current().script.missionType() }; });
  return { before, after, errors };
}

test('스커미시: 기본 설정(AI 1명)으로 시작해 월드가 만들어지고 게임이 흐른다', async ({ page }) => {
  test.setTimeout(150000);
  const { before, after, errors } = await runSkirmish(page, {});
  expect(before.run).toBe(1); expect(before.players).toBe(2); expect(before.mapSize).toBe(1); expect(before.central[1]).toBe(9); expect(before.central[2]).toBe(0);
  expect(after.clock).toBeGreaterThan(before.clock + 3); expect(after.paused).toBe(false); expect(after.dialog).toBe(false); expect(after.missionType.toLowerCase()).toBe('tutorial');
  expect(errors).toEqual([]);
});

test('스커미시: AI 7명·동맹·금지 유닛 설정으로도 8명 월드가 만들어진다', async ({ page }) => {
  test.setTimeout(150000);
  const { before, after, errors } = await runSkirmish(page, { skirmishAiCount: 7, skirmishAi3Ally: 1, skirmishAi8Diff: 3, skirmishBans: 'outpost,sunCannon' });
  expect(before.players).toBe(8); expect(before.mapSize).toBe(3);
  expect(after.clock).toBeGreaterThan(before.clock + 3); expect(after.paused).toBe(false);
  expect(errors).toEqual([]);
});
