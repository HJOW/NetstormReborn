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

// 번들 경로와 디버그 경로가 서로 다른 스크립트 적재 방식을 쓰는지 확인한다. index.html 은 webpack 번들 하나만 불러오고 ES 모듈 파일(netstorm.js 등)은 불러오지 않으며,
// debug.html 은 번들 없이 이전 방식대로 ES 모듈을 직접 불러온다. 둘 다 시작 클릭 화면까지 오류 없이 도달해야 한다. (2026-11-24 WB04-E1293, 사용자 지시)
for (const [page_, bundled] of [['/', true], ['/debug.html', false]]) {
  test(`${bundled ? 'index.html' : 'debug.html'}: ${bundled ? '번들 하나만' : '번들 없이 ES 모듈로'} 불러와 시작 화면에 도달한다`, async ({ page }) => {
    const errors = [], scripts = [];
    page.on('pageerror', (e) => errors.push(String(e)));
    page.on('request', (request) => { const path = new URL(request.url()).pathname; if (path.endsWith('.js')) scripts.push(path); });
    await page.goto(page_);
    await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 });
    if (bundled) {
      expect(scripts, '번들 하나').toEqual(['/dist/netstorm.bundle.js']);
    } else {
      expect(scripts.includes('/dist/netstorm.bundle.js'), '번들을 쓰지 않는다').toBe(false);
      expect(scripts.includes('/netstorm.js') && scripts.length > 100, '모듈을 하나씩 불러온다').toBe(true);
    }
    expect(errors).toEqual([]);
  });
}
