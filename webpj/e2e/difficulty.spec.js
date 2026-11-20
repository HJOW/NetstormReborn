// 옵션 메뉴의 Difficulty(쉬움·보통·어려움) 하위 메뉴가 기본 보통에 체크하고 선택을 설정(difficulty)과 게임 데이터에 저장하는지, 한국어 화면에서 번역되는지 실제 Chromium 에서 확인한다. (2026-11-20 WB04-E1096)
const { test, expect } = require('@playwright/test');

// 실제 돌 버튼의 논리 좌표를 브라우저 마우스로 누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 30000 });
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

// 현재 펼침 메뉴 맨 위 목록에서 체크된 항목 라벨들이다.
const checkedLabels = (page) => page.evaluate(() => window.netstorm._mainMenu.popup.levels.at(-1).list.items.filter((item) => item.checked).map((item) => item.label));

async function openMain(page, url) {
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto(url);
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 });
}

test('Difficulty 하위 메뉴는 기본 보통에 체크하고 선택을 저장하며 새 선택이 게임 데이터에 반영된다', async ({ page }) => {
  const errors = []; page.on('pageerror', (error) => errors.push(error.message));
  await page.addInitScript(() => { if (!localStorage.getItem('netstorm.settings')) localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } })); });
  await openMain(page, '/?aspect=4:3');
  await clickLabel(page, 'Options'); await clickLabel(page, 'Difficulty');
  expect(await checkedLabels(page)).toEqual(['Normal']);
  await clickLabel(page, 'Hard'); await page.waitForTimeout(300);
  expect(await page.evaluate(() => JSON.parse(localStorage.getItem('netstorm.settings')).values.difficulty)).toBe(3);
  // 적 AI 서비스가 읽는 게임 데이터 설정에도 같은 값이 있다.
  expect(await page.evaluate(() => window.netstorm._gameData.readInt('difficulty', 0).value)).toBe(3);
  await clickLabel(page, 'Options'); await clickLabel(page, 'Difficulty');
  expect(await checkedLabels(page)).toEqual(['Hard']);
  await clickLabel(page, 'Easy'); await page.waitForTimeout(300);
  expect(await page.evaluate(() => window.netstorm._gameData.readInt('difficulty', 0).value)).toBe(1);
  expect(errors).toEqual([]);
});

test('한국어 화면에서 난이도 메뉴 이름이 번역된다', async ({ page }) => {
  test.setTimeout(90000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.message));
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, difficulty: 3 } })));
  await openMain(page, '/?lang=ko&aspect=4:3');
  await clickLabel(page, '옵션'); await clickLabel(page, '난이도');
  expect(await checkedLabels(page)).toEqual(['어려움']);
  expect(await page.evaluate(() => window.netstorm._mainMenu.popup.levels.at(-1).list.items.map((item) => item.label))).toEqual(['쉬움', '보통', '어려움']);
  expect(errors).toEqual([]);
});
