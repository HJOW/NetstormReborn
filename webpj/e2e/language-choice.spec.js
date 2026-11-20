// 옵션 메뉴 Language 선택이 설정에 저장되고 lang 매개변수를 지운 뒤 페이지를 새 언어로 다시 불러오며, 저장된 선택이 URL 매개변수 없이도 시작 언어가 되는지 실제 Chromium 에서 확인한다. (2026-11-20 WB04-E1095)
const { test, expect } = require('@playwright/test');

// 실제 돌 버튼의 논리 좌표를 브라우저 마우스로 누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 20000 });
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

test('Language 선택은 설정에 저장하고 lang 매개변수를 지운 뒤 다시 불러온다', async ({ page }) => {
  const errors = []; page.on('pageerror', (error) => errors.push(error.message));
  await page.addInitScript(() => { if (!localStorage.getItem('netstorm.settings')) localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } })); });
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?lang=en&aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  // 실제로 다시 불러오는 대신 호출 횟수만 센다.
  await page.evaluate(() => { window.__reloads = 0; window.netstorm.options.reload = () => { window.__reloads += 1; }; });
  await clickLabel(page, 'Options'); await clickLabel(page, 'Language');
  expect(await page.evaluate(() => window.netstorm._mainMenu.popup.levels.at(-1).list.items.filter((item) => item.checked).map((item) => item.label))).toEqual(['English']);
  // 현재 언어(English)를 다시 고르면 아무 일도 없다.
  await clickLabel(page, 'English'); await page.waitForTimeout(300);
  expect(await page.evaluate(() => window.__reloads)).toBe(0); expect(await page.evaluate(() => location.search)).toContain('lang=en');
  await clickLabel(page, 'Options'); await clickLabel(page, 'Language'); await clickLabel(page, 'Korean'); await page.waitForTimeout(300);
  expect(await page.evaluate(() => JSON.parse(localStorage.getItem('netstorm.settings')).values.language)).toBe(2);
  expect(await page.evaluate(() => window.__reloads)).toBe(1);
  const search = new URLSearchParams(await page.evaluate(() => location.search)); expect(search.has('lang')).toBe(false); expect(search.get('aspect')).toBe('4:3');
  expect(errors).toEqual([]);
});

test('저장된 한국어 선택은 URL 매개변수 없이도 한국어로 시작한다', async ({ page }) => {
  test.setTimeout(90000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.message));
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, language: 2 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 });
  expect(await page.evaluate(() => window.netstorm.options.language)).toBe('korean');
  await clickLabel(page, '옵션'); await clickLabel(page, '언어');
  expect(await page.evaluate(() => window.netstorm._mainMenu.popup.levels.at(-1).list.items.filter((item) => item.checked).map((item) => item.label))).toEqual(['한국어']);
  expect(errors).toEqual([]);
});
