// 정보(About) ▸ 버전 창에 원작 저작권·기존 패치 줄과 함께 11.01 패치(HJOW)·`Copyright 2026 HJOW` 줄이 영어·한국어로 나오는지 실제 Chromium 에서 확인한다. (2026-11-23 WB04-E1289, 사용자 요구)
// 버전 창은 Help 메뉴 Version·게임 메뉴 About ▸ Version 이 같은 Tell `About` 를 연다. 원작 줄은 그대로 두고 확인 단추 앞에 새 줄만 들어가야 한다.
const { test, expect } = require('@playwright/test');

// 메인 메뉴까지 가서 Tell `About` 창을 열고 본문과 단추를 돌려주는 도우미이다. language 는 주소 `lang` 값이다.
async function openAbout(page, language) {
  const errors = []; page.on('pageerror', (error) => errors.push(error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto(`/?aspect=4:3${language ? `&lang=${language}` : ''}`);
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 });
  await page.evaluate(() => window.netstorm._openTell('About'));
  await page.waitForFunction(() => Boolean(window.netstorm._mainMenu.dialog?.content), null, { timeout: 15000 });
  const about = await page.evaluate(() => ({ text: window.netstorm._mainMenu.dialog.content.styleText, buttons: window.netstorm._mainMenu.dialog.buttons.map((button) => button.label) }));
  return { about, errors };
}

test('영어 버전 창: 원작 저작권·10.78 패치 줄 뒤에 11.01 HJOW 패치와 Copyright 2026 HJOW 가 나온다', async ({ page }) => {
  test.setTimeout(120000);
  const { about, errors } = await openAbout(page, '');
  expect(about.text).toContain('(c) 1997 Titanic Entertainment, Inc. and Activision Inc.');
  expect(about.text).toMatch(/10\.78 Patch by Ticonderoga Entertainment\./u);
  expect(about.text).toContain('NetStorm 11.01 Patch by HJOW.'); expect(about.text).toContain('Copyright 2026 HJOW');
  // 새 줄은 원작 저작권·기존 패치 줄 뒤에 오고, 한 번만 나온다. 확인 단추는 그대로이다.
  expect(about.text.indexOf('Ticonderoga')).toBeLessThan(about.text.indexOf('NetStorm 11.01 Patch by HJOW.'));
  expect(about.text.split('Copyright 2026 HJOW')).toHaveLength(2);
  expect(about.buttons).toHaveLength(1);
  await page.screenshot({ path: test.info().outputPath('about-version-english.png') });
  expect(errors).toEqual([]);
});

test('한국어 버전 창: 11.01 패치 줄이 한국어로 나오고 원작 저작권 줄과 확인 단추가 보존된다', async ({ page }) => {
  test.setTimeout(120000);
  const { about, errors } = await openAbout(page, 'ko');
  expect(about.text).toContain('(c) 1997 Titanic Entertainment, Inc. and Activision Inc.');
  expect(about.text).toContain('10.78 패치: Ticonderoga Entertainment.');
  expect(about.text).toContain('NetStorm 11.01 패치: HJOW.'); expect(about.text).toContain('Copyright 2026 HJOW');
  expect(about.text.indexOf('Ticonderoga')).toBeLessThan(about.text.indexOf('NetStorm 11.01 패치: HJOW.'));
  expect(about.text.split('Copyright 2026 HJOW')).toHaveLength(2);
  expect(about.buttons).toEqual(['확인']);
  await page.screenshot({ path: test.info().outputPath('about-version-korean.png') });
  expect(errors).toEqual([]);
});
