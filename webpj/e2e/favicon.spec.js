// 브라우저가 상대 경로의 원본 아이콘을 정상적으로 가져오고 해독하는지 확인한다.
const { test, expect } = require('@playwright/test');
const { createHash } = require('node:crypto');

test('원본 favicon을 브라우저에서 불러온다', async ({ page, request }) => {
  await page.goto('/');
  const link = page.locator('link[rel="icon"]');
  await expect(link).toHaveAttribute('href', 'favicon.ico');
  const response = await request.get(new URL(await link.getAttribute('href'), page.url()).href);
  expect(response.status()).toBe(200);
  expect(response.headers()['content-type']).toContain('image/x-icon');
  expect(createHash('sha256').update(await response.body()).digest('hex'))
    .toBe('bd4c18c5cb5f42bc49a1de0c6d6623643fd5377f575e95a1de60a05f01e4f068');
  const size = await page.evaluate(async () => {
    const image = new Image();
    image.src = document.querySelector('link[rel="icon"]').href;
    await image.decode();
    return [image.naturalWidth, image.naturalHeight];
  });
  expect([[16, 16], [32, 32]]).toContainEqual(size);
});
