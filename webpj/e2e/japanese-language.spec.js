// 실제운영번들에서일본어선택/재적재/실제CDN과차단시영어대체를확인한다.
const { test, expect } = require('@playwright/test');
// 보고서가덮여도실제폰트요청/언어상태JSON을독립보존한다.
const fs = require('node:fs');

// 돌단추/팝업의실제제어점을마우스로누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label);
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

// 실제시작클릭과원래타이틀끝까지기다린다. 미션플레이는시작하지않는다.
async function enterMenu(page) {
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 120000 });
  await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 });
}

// 외부글꼴실측은명시옵트인으로실행한다. 차단대체검사는실제외부접속없이항상실행할수있다.
test('일본어실제CDN: 메뉴선택/저장/재적재와패치크레딧', async ({ page }, info) => {
  test.skip(process.env.NETSTORM_E2E_JAPANESE !== '1', '실제Google Fonts 접속은 NETSTORM_E2E_JAPANESE=1에서검사한다.');
  test.setTimeout(240000);
  // 영어첫페이지에서는CDN을요청하지않는지관측한다.
  const requests = [], errors = [];
  page.on('request', (request) => { if (/fonts\.(?:googleapis|gstatic)\.com/u.test(request.url())) requests.push(request.url()); });
  page.on('pageerror', (error) => errors.push(error.stack ?? error.message));
  await page.addInitScript(() => { if (!localStorage.getItem('netstorm.settings')) localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, autoDemo: 0, sound: 0, music: 0 } })); });
  await page.goto('/?lang=en&aspect=4:3'); await enterMenu(page); expect(requests).toEqual([]);
  await clickLabel(page, 'Options'); await clickLabel(page, 'Language');
  await Promise.all([page.waitForEvent('framenavigated', { predicate: (frame) => frame === page.mainFrame() }), clickLabel(page, 'Japanese')]);
  await enterMenu(page);
  // 실제저장선택3과엔진번호5/일본어파일/완료된부분글꼴을확인한다.
  const state = await page.evaluate(() => ({ language: window.netstorm.options.language, effective: window.netstorm._gameData.language, selected: JSON.parse(localStorage.getItem('netstorm.settings')).values.language, prompt: window.netstorm.texts.clickToStart, faces: Array.from(document.fonts).filter((face) => face.family.replaceAll('"', '') === 'Noto Sans JP').map((face) => face.status) }));
  expect(state.language).toBe('japanese'); expect(state.effective).toBe('japanese'); expect(state.selected).toBe(3); expect(state.prompt).toBe('クリックして開始');
  expect(state.faces.length).toBeGreaterThan(0); expect(state.faces.every((status) => status === 'loaded')).toBe(true);
  expect(new URL(page.url()).searchParams.has('lang')).toBe(false); expect(requests.some((url) => url.startsWith('https://fonts.googleapis.com/'))).toBe(true);
  await clickLabel(page, 'オプション'); await clickLabel(page, '言語');
  expect(await page.evaluate(() => window.netstorm._mainMenu.popup.levels.at(-1).list.items.filter((item) => item.checked).map((item) => item.label))).toEqual(['日本語']);
  await clickLabel(page, '日本語'); await clickLabel(page, 'クレジット'); await clickLabel(page, 'NetStorm 11.01 パッチ クレジット');
  const patch = await page.evaluate(() => window.netstorm._gameData.tellSection('Patch1101.Credits'));
  expect(patch).toContain('Copyright 2026 HJOW'); expect(patch).toContain('Anthropic Claude Code');
  await page.screenshot({ path: info.outputPath('japanese-credits.png') });
  const statePath = info.outputPath('japanese-state.json'); fs.writeFileSync(statePath, JSON.stringify({ state, requests, errors }, null, 2));
  await info.attach('japanese-state', { path: statePath, contentType: 'application/json' });
  expect(errors).toEqual([]);
});

// CDN이오류일때는일본어저장을보존하면서영어안내/메뉴를보여주고동일선택재시도도재적재한다.
test('일본어CDN차단: 영어시작안내와언어재선택재시도', async ({ page }, info) => {
  test.setTimeout(180000);
  const errors = [], requests = [];
  page.on('pageerror', (error) => errors.push(error.message));
  await page.route('https://fonts.googleapis.com/**', (route) => { requests.push(route.request().url()); return route.abort('failed'); });
  await page.addInitScript(() => { if (!localStorage.getItem('netstorm.settings')) localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, autoDemo: 0, sound: 0, music: 0, language: 3 } })); });
  await page.goto('/?lang=ja&aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 90000 });
  expect(await page.evaluate(() => ({ prompt: window.netstorm.texts.clickToStart, requested: window.netstorm.options.language, effective: window.netstorm._gameData.language, unicode: Boolean(window.netstorm._resources.fonts.unicode), fontLinks: document.querySelectorAll('link[href*="fonts.googleapis.com"]').length }))).toEqual({ prompt: 'Click to start', requested: 'japanese', effective: 'english', unicode: false, fontLinks: 0 });
  await page.locator('canvas').click(); await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  await clickLabel(page, 'Options'); await clickLabel(page, 'Language');
  expect(await page.evaluate(() => window.netstorm._mainMenu.popup.levels.at(-1).list.items.filter((item) => item.checked).map((item) => item.label))).toEqual(['English']);
  await Promise.all([page.waitForEvent('framenavigated', { predicate: (frame) => frame === page.mainFrame() }), clickLabel(page, 'Japanese')]);
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 90000 });
  expect(await page.evaluate(() => window.netstorm.texts.clickToStart)).toBe('Click to start');
  expect(await page.evaluate(() => JSON.parse(localStorage.getItem('netstorm.settings')).values.language)).toBe(3);
  expect(requests.length).toBe(2); expect(errors).toEqual([]);
  await page.screenshot({ path: info.outputPath('japanese-fallback.png') });
  fs.writeFileSync(info.outputPath('japanese-fallback.json'), JSON.stringify({ errors, requests, selected: 3, effective: 'english', prompt: 'Click to start' }, null, 2));
});
