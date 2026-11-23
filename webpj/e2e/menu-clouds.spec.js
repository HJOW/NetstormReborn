// 메인 메뉴 배경이 정적 구름 GIF 타일이 아니라 팔레트 전환이 만든 Kencloud 구름 캐시(원본 DrawKencloud)로 그려지는지 실제 Chromium 에서 확인한다. (2026-11-24 WB04-E1298)
// 원본은 구름 파일을 읽은 뒤 `speed` 번 일부 행을 한 바이트씩 밀어 쓰므로 캐시는 GIF 와 일부 행이 어긋난다. 이전 웹은 GIF 를 그대로 타일로 깔아 37,054픽셀이 cpppj 와 달랐다.
const { test, expect } = require('@playwright/test');

test('메인 메뉴 배경은 Kencloud 구름 캐시이며 구름 GIF 타일과 일부 행이 다르다', async ({ page }) => {
  test.setTimeout(120000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.message));
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, autoDemo: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  await page.waitForTimeout(500);
  const result = await page.evaluate(() => {
    const app = window.netstorm, surface = app._surface, cache = app._clientKencloud.cacheImage(), gif = app._resources.clouds;
    // 제목 그림(가운데 639×480)과 버튼이 닿지 않는 왼쪽 띠(x 0~149)만 비교한다.
    let cacheMismatch = 0, gifMismatch = 0;
    for (let y = 0; y < surface.height; y += 1) for (let x = 0; x < 150; x += 1) {
      const shown = surface.pixels[y * surface.width + x];
      if (shown !== cache.indices[(y % 512) * 512 + (x % 512)]) cacheMismatch += 1;
      if (shown !== gif.indices[(y % gif.height) * gif.width + (x % gif.width)]) gifMismatch += 1;
    }
    return { cacheMismatch, gifMismatch, cacheSize: [cache.width, cache.height], gifSize: [gif.width, gif.height] };
  });
  expect(result.cacheSize).toEqual([512, 512]);
  expect(result.cacheMismatch, '왼쪽 띠가 구름 캐시와 같다').toBe(0);
  expect(result.gifMismatch, '구름 GIF 타일과는 일부 행이 다르다').toBeGreaterThan(0);
  expect(errors).toEqual([]);
});
