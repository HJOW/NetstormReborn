// 실제 Chromium에서 한국어 글꼴 등록과 작은 글자 및 영어 대체를 확인한다.
const { test, expect } = require('@playwright/test');

// 시작 화면을 통과하고 첫 대화상자를 표시한다.
async function startKorean(page) {
  await page.goto('/?lang=ko&mute&aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart');
  await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
}

// 표시된 조작부의 논리 좌표를 실제 화면 좌표로 바꾸어 클릭한다.
async function clickLabel(page, label) {
  const point = await page.evaluate((text) => {
    const app = window.netstorm;
    const logical = app._mainMenu.controlPoint(text);
    if (!logical) throw new Error(`조작부 없음: ${text}`);
    const rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width,
      y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y);
}

// 실제 글꼴 두께와 작은 글자 픽셀, 한국어 메뉴를 확인한다.
test('갈무리11 일반·굵은 글꼴과 한국어 메뉴', async ({ page }) => {
  const errors = [];
  page.on('pageerror', (error) => errors.push(String(error)));
  await startKorean(page);
  const result = await page.evaluate(() => {
    const app = window.netstorm;
    const fonts = app._resources.fonts;
    const provider = fonts.unicode;
    const regular = provider.get(0, 0, fonts.get(0, 0));
    const bold = provider.get(0, 2, fonts.get(0, 2));
    const pixels = (font) => font.glyph('한'.charCodeAt(0)).image.opacity.filter((v) => v).length;
    return { faces: [...document.fonts].map((f) => [f.family, f.weight, f.status]),
      size: regular.spec.emSize, advance: regular.glyph('한'.charCodeAt(0)).advance,
      regular: pixels(regular), bold: pixels(bold), tell: app._gameData.currentTell };
  });
  expect(result.faces).toEqual(expect.arrayContaining([['Galmuri11', '400', 'loaded'], ['Galmuri11', '700', 'loaded']]));
  // 갈무리11 WOFF2 의 설계 격자는 12px 이다(11px 는 가로획이 빠졌다). (2026-11-23 WB04-E1288)
  expect(result.size).toBe(12);
  expect(result.advance).toBe(12);
  expect(result.regular).toBeGreaterThan(0);
  expect(result.bold).toBeGreaterThan(result.regular);
  expect(result.tell).toBe('Welcome');
  await clickLabel(page, '확인');
  await expect.poll(() => page.evaluate(() => window.netstorm._gameData.currentTell)).toBe('Welcome2');
  await clickLabel(page, '메인 메뉴');
  await clickLabel(page, '옵션');
  // Options 목록은 E1095 뒤 Language(언어)가 맨 위이므로 첫 항목이 아니라 포함 여부로 확인한다. (2026-11-20 WB04-E1118: 낡은 첫 항목 기대값 정정)
  const optionLabels = await page.evaluate(() => window.netstorm._mainMenu.popup.levels[0].list.items.map((item) => item.label));
  expect(optionLabels[0]).toBe('언어'); expect(optionLabels).toContain('소리 켜기');
  await page.keyboard.press('Escape');
  await clickLabel(page, '도움말');
  await clickLabel(page, '일반 도움말 - F1');
  expect(await page.evaluate(() => Boolean(window.netstorm._mainMenu.dialog))).toBe(true);
  await page.screenshot({ path: test.info().outputPath('korean-help.png') });
  expect(errors).toEqual([]);
});

// 글꼴 파일을 받지 못하면 영어 자원으로 시작한다.
test('갈무리11 적재 실패 시 영어 대체', async ({ page }) => {
  await page.route('**/fonts/Galmuri11.woff2', (route) => route.abort());
  await startKorean(page);
  expect(await page.evaluate(() => window.netstorm._resources.translate('Options'))).toBe('Options');
  expect(await page.evaluate(() => window.netstorm._resources.fonts.unicode)).toBeNull();
});
