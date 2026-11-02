// 기본 구동 확인용 스모크 테스트: 페이지가 열리고 Netstorm 객체가 초기화되며 스크립트 오류가 없는지 본다.
const { test, expect } = require('@playwright/test');

test('메인 페이지가 열리고 Netstorm 이 초기화된다', async ({ page }) => {
  // 페이지에서 발생한 처리되지 않은 예외 수집
  const errors = [];
  page.on('pageerror', (e) => errors.push(String(e)));

  await page.goto('/');
  await expect(page).toHaveTitle('NetStorm');
  await page.waitForFunction(() => !!window.netstorm);
  await expect(page.locator('#netstorm_root')).toBeAttached();
  expect(errors).toEqual([]);
});
