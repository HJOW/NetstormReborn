// 클릭하여 시작 뒤 보이는 시작 title 그림 세 장(원본 4ab3e0)을 실제 Chromium 에서 확인한다. (2026-11-22 WB04-E1184)
// 자동화 브라우저(navigator.webdriver)에서는 기본으로 건너뛰므로 주소 `?title=1` 로 켠다. 세 장은 3초씩 보이고 입력(클릭·키)이 있으면 남은 그림을 건너뛰어 메인 메뉴로 간다.
const { test, expect } = require('@playwright/test');

// 논리 화면 가운데 640×480 영역의 서로 다른 색 수와, 그 바깥 왼쪽 띠가 모두 검정인지이다(그림이 검은 화면 가운데에 놓였는지).
const frame = (page) => page.evaluate(() => {
  const app = window.netstorm, canvas = app.canvas, context = canvas.getContext('2d'), scale = canvas.width / app._layout.logicalWidth;
  const left = Math.round((app._layout.logicalWidth - 640) / 2 * scale), top = Math.round((app._layout.logicalHeight - 480) / 2 * scale);
  const inside = context.getImageData(left, top, Math.round(640 * scale), Math.round(480 * scale)).data, margin = context.getImageData(0, 0, Math.max(1, left - 2), canvas.height).data;
  const colors = new Set(); for (let index = 0; index < inside.length; index += 4 * 7) colors.add(inside[index] << 16 | inside[index + 1] << 8 | inside[index + 2]);
  let dark = true; for (let index = 0; index < margin.length; index += 4) if (margin[index] + margin[index + 1] + margin[index + 2] > 12) { dark = false; break; }
  return { state: app.state, colors: colors.size, marginDark: dark };
});

test('시작 title: 클릭 뒤 title 그림이 가운데에 보이고 세 장이 끝나면 메인 메뉴로 간다', async ({ page }) => {
  test.setTimeout(120000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (['error'].includes(message.type())) errors.push(message.text()); });
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3&lang=en&title=1');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 });
  await page.locator('canvas').click(); await page.waitForTimeout(700);
  const first = await frame(page); expect(first.state).toBe('title'); expect(first.colors).toBeGreaterThan(8); expect(first.marginDark).toBe(true);
  // 3초 뒤 둘째 그림, 6초 뒤 셋째 그림이며 둘 다 여전히 title 상태이다.
  await page.waitForTimeout(3000); const second = await frame(page); expect(second.state).toBe('title'); expect(second.colors).toBeGreaterThan(8);
  await page.waitForTimeout(3000); const third = await frame(page); expect(third.state).toBe('title'); expect(third.colors).toBeGreaterThan(8);
  // 9초가 지나면 메인 메뉴이다.
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 8000 });
  expect(errors).toEqual([]);
});

test('시작 title: 클릭하면 남은 그림을 건너뛰고 메인 메뉴로 가며 키 입력도 건너뛴다', async ({ page }) => {
  test.setTimeout(120000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (['error'].includes(message.type())) errors.push(message.text()); });
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3&lang=en&title=1');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 });
  await page.locator('canvas').click(); await page.waitForTimeout(500);
  expect((await frame(page)).state).toBe('title');
  await page.locator('canvas').click(); await page.waitForTimeout(800);
  expect((await frame(page)).state).toBe('mainMenu');
  // 새로 시작해 키로 건너뛴다.
  await page.goto('/?aspect=4:3&lang=en&title=1');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 });
  await page.keyboard.press('Enter'); await page.waitForTimeout(500);
  expect((await frame(page)).state).toBe('title');
  await page.keyboard.press('Space'); await page.waitForTimeout(800);
  expect((await frame(page)).state).toBe('mainMenu');
  // 자동화 기본값은 건너뛰기이다(주소에 title 이 없으면 바로 메인 메뉴).
  await page.goto('/?aspect=4:3&lang=en');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 });
  await page.locator('canvas').click(); await page.waitForTimeout(500);
  expect((await frame(page)).state).toBe('mainMenu');
  expect(errors).toEqual([]);
});
