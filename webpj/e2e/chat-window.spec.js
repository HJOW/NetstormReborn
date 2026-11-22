// 미션 화면 아래 채팅 창(F3)이 실제 Gump 트리에 만들어지고, F3 로 보이며, 로그 줄이 색과 함께 그려지는지 실제 Chromium 에서 확인한다. (2026-11-22 WB04-E1169)
// 원본 `ChatWindow`(0x13c root)의 도우미·배경·입력줄(Console)·로그(ScrollingText) 네 자식을 같은 Client 수명에 조립하고 공유 로그 버퍼(0x548758)를 로그 자식이 빌려 쓴다.
const { test, expect } = require('@playwright/test');

// 실제 돌 버튼의 논리 좌표를 브라우저 마우스 좌표로 바꿔 누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 90000 });
  const point = await page.evaluate((text) => { const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect(); return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height }; }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}
// 현재 채팅 창 진단과 지도 영역 아래쪽, 공유 로그 줄 수이다.
const state = (page) => page.evaluate(() => { const app = window.netstorm, chat = app._campaignProduction.ui.chat(); return { report: chat.report(), mapBottom: app._clientGump.mapView().bottom, lines: app._clientChatWindows.log.lineCount, current: app._clientChatWindows.window.current }; });
// 논리 화면의 사각형 안에서 가장 많은 색이 아닌 픽셀 수와 빨강 계열 픽셀 수이다(글자가 그려졌는지).
const ink = (page, rect) => page.evaluate((box) => {
  const app = window.netstorm, canvas = app.canvas, context = canvas.getContext('2d'), scale = canvas.width / app._layout.logicalWidth;
  const data = context.getImageData(Math.round(box.left * scale), Math.round(box.top * scale), Math.round((box.right - box.left) * scale), Math.round((box.bottom - box.top) * scale)).data; const counts = new Map(); let red = 0;
  for (let index = 0; index < data.length; index += 4) { const key = data[index] << 16 | data[index + 1] << 8 | data[index + 2]; counts.set(key, (counts.get(key) ?? 0) + 1); if (data[index] > 170 && data[index + 1] < 70 && data[index + 2] < 70) red += 1; }
  const total = data.length / 4, background = Math.max(...counts.values()); return { total, other: total - background, red };
}, rect);

test('채팅 창: 숨긴 채 만들어지고 F3 로 보이며 로그가 색과 함께 그려진다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (['error', 'warning'].includes(message.type()) && !/willReadFrequently/u.test(message.text())) errors.push(message.text()); }); // 시험이 픽셀을 읽을 때 브라우저가 내는 Canvas2D 성능 안내는 게임 오류가 아니다.
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, autoDemo: 0, skirmishAiCount: 3, skirmishAi2Ally: 0, skirmishAi3Ally: 0, skirmishAi4Ally: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3&lang=en');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 });
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Skirmish'); await clickLabel(page, 'Start');
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
  await clickLabel(page, 'Go!'); await page.waitForTimeout(2000);

  // 시작 때는 숨김이다: 채팅 root 는 화면 아래(768 이후)에 있고 지도 영역은 전체 높이이며 도우미는 보이지 않는다.
  const hidden = await state(page); expect(hidden.report, '채팅 창').not.toBeNull(); expect(hidden.current).toBe(hidden.report.token);
  expect(hidden.report.bounds.top).toBeGreaterThanOrEqual(768); expect(hidden.mapBottom).toBe(768); expect(hidden.report.helperVisible).toBe(0);
  // 숨긴 동안에도 로그는 쌓인다(원본처럼 같은 공유 버퍼에 추가).
  await page.evaluate(() => window.netstorm._campaignProduction.ui.chat().appendLog('Our Priest has been Immobilized')); expect((await state(page)).lines).toBe(1);

  // F3: 지도 영역 아래쪽이 줄고(768 → 672) 채팅 영역(96픽셀)이 위로 올라오며 도우미 표시 비트가 켜진다. 앞서 쌓은 로그 한 줄이 보인다.
  await page.keyboard.press('F3'); await page.waitForTimeout(800);
  const shown = await state(page); expect(shown.mapBottom).toBe(672); expect(shown.report.bounds.top).toBeLessThan(700); expect(shown.report.bounds.bottom).toBe(768); expect(shown.report.helperVisible).toBe(1);
  expect(shown.report.background.bounds.right - shown.report.background.bounds.left).toBeGreaterThan(800);
  const firstLine = await ink(page, { left: 90, top: 680, right: 420, bottom: 693 }); expect(firstLine.other, '첫 로그 줄 글자').toBeGreaterThan(60);

  // 색 표기: 빨강 글자 접두(`~r`)와 되돌림(`~n`)이 줄 안에서 색을 바꾼다. 줄이 늘면 새 줄이 아래에 그려진다.
  await page.evaluate(() => window.netstorm._campaignProduction.ui.chat().appendLog('~rRed~n line two')); await page.waitForTimeout(500);
  expect((await state(page)).lines).toBe(2);
  const second = await ink(page, { left: 90, top: 694, right: 220, bottom: 706 }); expect(second.other, '둘째 로그 줄 글자').toBeGreaterThan(30); expect(second.red, '빨강 글자').toBeGreaterThan(5);

  // 다시 F3: 숨긴다(지도 영역이 전체 높이로 돌아가고 로그는 남는다).
  await page.keyboard.press('F3'); await page.waitForTimeout(600);
  const again = await state(page); expect(again.mapBottom).toBe(768); expect(again.report.bounds.top).toBeGreaterThanOrEqual(768); expect(again.lines).toBe(2); expect(again.report.helperVisible).toBe(0);
  expect(errors).toEqual([]);
});

test('채팅 창: 미션을 떠나면 창이 정리되고 다음 미션에서 다시 만들어진다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (['error', 'warning'].includes(message.type()) && !/willReadFrequently/u.test(message.text())) errors.push(message.text()); }); // 시험이 픽셀을 읽을 때 브라우저가 내는 Canvas2D 성능 안내는 게임 오류가 아니다.
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, autoDemo: 0, skirmishAiCount: 3, skirmishAi2Ally: 0, skirmishAi3Ally: 0, skirmishAi4Ally: 0 } })));
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3&lang=en');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart', null, { timeout: 60000 }); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu', null, { timeout: 60000 });
  const start = async () => { await clickLabel(page, 'Campaign'); await clickLabel(page, 'Skirmish'); await clickLabel(page, 'Start');
    await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 }); await clickLabel(page, 'Go!'); await page.waitForTimeout(1500); };
  await start(); const first = await state(page); expect(first.report).not.toBeNull();
  await page.evaluate(() => window.netstorm._leaveCampaignMission()); await page.waitForFunction(() => !window.netstorm._campaignDisplayActive(), null, { timeout: 30000 });
  expect(await page.evaluate(() => window.netstorm._clientChatWindows.window.current)).toBe(0);
  await start(); const second = await state(page); expect(second.report, '다음 미션의 채팅 창').not.toBeNull(); expect(second.report.bounds.top).toBeGreaterThanOrEqual(768);
  expect(errors).toEqual([]);
});
