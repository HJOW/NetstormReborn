// 실제 웹 메뉴에서 Reset All 의 게임 안 확인 창(Tell)·취소·확정과 새로고침 후 최초 실행을 확인한다. 브라우저 confirm() 은 한 번도 뜨지 않아야 한다. (2026-11-19 WB04-E863, 2026-11-23 WB04-E1251: 브라우저 confirm() 을 게임 안 확인 창으로 교체)
const { test, expect } = require('@playwright/test');

// Canvas 의 현재 메뉴·대화상자 조작부 좌표를 실제 마우스 좌표로 바꾸어 누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 30000 });
  const point = await page.evaluate((text) => {
    const app = window.netstorm;
    const logical = app._mainMenu.controlPoint(text);
    const rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

// 현재 열린 대화상자의 단추 이름 목록이다(창이 없으면 null).
const buttons = (page) => page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons?.map((button) => button.label) ?? null);

// 두 게임 항목과 다른 앱의 항목을 넣고 게임 안 확인 창에서 취소와 확정의 차이를 확인한다.
test('Reset All 게임 안 확인 창: 취소와 확정 후 최초 실행', async ({ page }) => {
  const errors = [], dialogs = [];
  page.on('pageerror', (error) => errors.push(String(error)));
  // 브라우저 대화상자(confirm/alert)가 뜨면 기록하고 닫는다. 이 시험은 하나도 뜨지 않아야 통과한다.
  page.on('dialog', (dialog) => { dialogs.push(dialog.message()); dialog.dismiss(); });
  await page.goto('/');
  await page.evaluate(() => {
    localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, soundVolume: 2 } }));
    localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: { DoneTheWarBegins: '1' } }));
    // 설치한 모드도 확인 창 취소 때 유지되고 확정 때 함께 삭제되어야 한다.
    const pkg = { format: 'netstorm-mod-package', formatVersion: 1, manifest: { id: 'test.reset', name: 'Reset test', version: '1.0.0', apiVersion: '0.1.0', entry: 'main.js' }, modules: { 'main.js': 'export default function(){}' } };
    localStorage.setItem('netstorm.mods.v0', JSON.stringify({ version: 1, mods: [{ package: JSON.stringify(pkg), enabled: false, lastError: null }] }));
    localStorage.setItem('other.app', 'keep');
  });
  await page.reload();
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart');
  await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  // Options 의 마지막 행은 게임 안 확인 창을 연다: 제목·본문과 확정·취소 두 단추.
  await clickLabel(page, 'Options');
  await clickLabel(page, 'Reset All');
  await expect.poll(() => buttons(page), { timeout: 10000, message: 'Reset All 확인 창' }).toEqual(['Reset All', 'Cancel']);
  expect(await page.evaluate(() => window.netstorm._resetAllArmed), '확인 창이 열리면 확정이 무장된다').toBe(true);
  // 취소: 아무것도 지워지지 않고 창이 닫히며 무장이 풀린다.
  await clickLabel(page, 'Cancel');
  await expect.poll(() => buttons(page), { timeout: 10000 }).toBeNull();
  expect(await page.evaluate(() => [window.netstorm.state, window.netstorm._settings.get('soundVolume'), window.netstorm.progress.get('DoneTheWarBegins'), window.netstorm._resetAllArmed])).toEqual(['mainMenu', 2, '1', false]);
  expect(await page.evaluate(() => JSON.parse(localStorage.getItem('netstorm.mods.v0')).mods.length)).toBe(1);
  // 확인 창 밖에서 확정 대상을 보내도 무시된다(원본의 제출 토큰 가드).
  await page.evaluate(() => window.netstorm._openTell('Patch1101.ResetAllConfirmed'));
  expect(await page.evaluate(() => [window.netstorm.state, window.netstorm._settings.get('soundVolume')])).toEqual(['mainMenu', 2]);
  // 확정: 확인 창의 첫 단추가 저장을 지우고 페이지를 다시 불러온다.
  await clickLabel(page, 'Options');
  await clickLabel(page, 'Reset All');
  await expect.poll(() => buttons(page), { timeout: 10000 }).toEqual(['Reset All', 'Cancel']);
  const loaded = page.waitForEvent('load');
  await clickLabel(page, 'Reset All');
  await loaded;
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart');
  expect(await page.evaluate(() => [localStorage.getItem('netstorm.settings'), localStorage.getItem('netstorm.progress'), localStorage.getItem('netstorm.mods.v0'), localStorage.getItem('other.app')])).toEqual([null, null, null, 'keep']);
  await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  expect(await page.evaluate(() => window.netstorm._gameData.currentTell)).toBe('Welcome');
  expect(dialogs, '브라우저 대화상자').toEqual([]);
  expect(errors).toEqual([]);
});

// 한국어 모드의 확인 창은 한국어 제목·본문·단추(초기화·취소)로 열리고 취소는 아무것도 지우지 않는다.
test('Reset All 게임 안 확인 창: 한국어 모드', async ({ page }) => {
  const errors = [], dialogs = [];
  page.on('pageerror', (error) => errors.push(String(error)));
  page.on('dialog', (dialog) => { dialogs.push(dialog.message()); dialog.dismiss(); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, soundVolume: 2, language: 2 } })));
  await page.setViewportSize({ width: 1024, height: 768 });
  await page.goto('/?lang=ko&mute&aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart');
  await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  await clickLabel(page, '옵션');
  await clickLabel(page, '모두 초기화'); // 옵션 메뉴 행의 한국어 이름이다(번역표 `Reset All`).
  await expect.poll(() => buttons(page), { timeout: 10000, message: '한국어 확인 창' }).toEqual(['초기화', '취소']);
  await page.screenshot({ path: test.info().outputPath('reset-all-korean.png') });
  await clickLabel(page, '취소');
  await expect.poll(() => buttons(page), { timeout: 10000 }).toBeNull();
  expect(await page.evaluate(() => [window.netstorm.state, window.netstorm._settings.get('soundVolume')])).toEqual(['mainMenu', 2]);
  expect(dialogs, '브라우저 대화상자').toEqual([]);
  expect(errors).toEqual([]);
});
