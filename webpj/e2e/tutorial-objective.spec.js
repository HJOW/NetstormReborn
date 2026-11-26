// 실제 F4 입력의 단계 전환과 F8/ESC의 가림·복원을 검증한다. 포획·희생·특정 적 공격·승리는 실행하지 않는다.
const { test, expect } = require('@playwright/test');
const { step, dismissTell, objective, startTutorial } = require('./tutorial-objective-control');

test('튜토리얼 안내: 실제 F4 단계 전환·F8 재열기·게임 메뉴 가림과 복원', async ({ page }) => {
  test.setTimeout(180000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message));
  page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await startTutorial(page);
  await page.keyboard.press('F4'); await step(page, 40);
  expect((await objective(page)).gate.stage).toBe(66); expect((await objective(page)).text).toBe('');
  await dismissTell(page); await step(page, 30);
  let current = await objective(page, 'Place a total of 8 bridges.'); expect(current.text).toBe(current.expected);
  await page.keyboard.press('F8'); await step(page, 2); expect((await objective(page)).dialog).toBe(true); expect((await objective(page)).text).toBe('');
  await dismissTell(page); await step(page, 15); current = await objective(page, 'Place a total of 8 bridges.'); expect(current.text).toBe(current.expected);
  await page.keyboard.press('Escape'); await step(page, 2); expect((await objective(page)).text).toBe('');
  await page.keyboard.press('Escape'); await step(page, 2); current = await objective(page, 'Place a total of 8 bridges.'); expect(current.text).toBe(current.expected);
  await page.screenshot({ path: test.info().outputPath('tutorial1-stage-b.png') });
  expect(errors).toEqual([]);
});
