// 실제 Chromium의 한국어 미션 메뉴·브리핑과 글꼴 대체 경로를 확인한다.
const { test, expect } = require('@playwright/test');

// 실제 게임 조작부의 논리 좌표를 브라우저 마우스 좌표로 바꿔 누른다.
async function clickLabel(page, label) {
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    if (!logical) throw new Error(`조작부 없음: ${text}`);
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

test('한국어 Tutorial1 미션 메뉴와 브리핑을 표시한다', async ({ page }) => {
  const errors = []; page.on('pageerror', (error) => errors.push(error.message));
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } })));
  await page.setViewportSize({ width: 1280, height: 720 }); await page.goto('/?lang=ko&mute&aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu'); await clickLabel(page, '캠페인');
  const group = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.map(({label,command}) => ({label,command})));
  const early = group.find((item) => item.label.includes('초기')); expect(early).toBeTruthy(); await clickLabel(page, early.label);
  const first = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'Tutorial1')?.label);
  expect(first).toMatch(/[가-힣]/); await clickLabel(page, first);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 50000 });
  await page.waitForFunction(() => Boolean(window.netstorm._mainMenu.dialog?.content), null, { timeout: 15000 });
  // 원본 줄 나눔은 끝 공백의 전진 폭을 포함하므로 실제 글자 바깥쪽만 비교한다.
  const brief = await page.evaluate(() => { const app = window.netstorm, d = app._mainMenu.dialog; return { language: app.options.language, unicode: Boolean(app._resources.fonts.unicode),
    style: d.content.styleText, width: d.body.width, height: d.body.height, lines: d.body.softLines.length, maxLine: Math.max(...d.body.softLines.map((line) => { const last = line.fragments.at(-1), bytes = last?.bytes; return line.width - (bytes?.at(-1) === 32 ? app._resources.fonts.measure(bytes.slice(-1), last.context).width : 0); })), screen: [app._surface.width, app._surface.height], current: app._gameData.currentTell };  });
  expect(brief.language).toBe('korean'); expect(brief.unicode).toBe(true); expect(brief.style).toContain('적 사제를 붙잡아');
  expect(brief.style).toContain('NetStorm'); expect(brief.lines).toBeGreaterThan(0); expect(brief.maxLine).toBeLessThanOrEqual(brief.width);
  await page.screenshot({ path: test.info().outputPath('korean-mission.png') });
  await clickLabel(page, '다음'); await page.waitForFunction(() => window.netstorm._gameData.currentTell === 'A1.');
  const next = await page.evaluate(() => { const app = window.netstorm, d = app._mainMenu.dialog; return { style: d.content.styleText, width: d.body.width, maxLine: Math.max(...d.body.softLines.map((line) => { const last = line.fragments.at(-1), bytes = last?.bytes; return line.width - (bytes?.at(-1) === 32 ? app._resources.fonts.measure(bytes.slice(-1), last.context).width : 0); })) }; });
  expect(next.style).toContain('화면 이동'); expect(next.style).toContain('F4'); expect(next.maxLine).toBeLessThanOrEqual(next.width);
  await page.screenshot({ path: test.info().outputPath('korean-mission-next.png') }); expect(errors).toEqual([]);
});

// 아직 번역하지 않은 미션은 한국어 메뉴에서 시작해도 원본 영어 안내를 사용한다.
test('한국어 메뉴에서 미번역 Tutorial2는 영어 안내로 대체한다', async ({ page }) => {
  const errors = []; page.on('pageerror', (error) => errors.push(error.message));
  await page.addInitScript(() => {
    localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } }));
    localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: { DoneTutorial1: '1' } }));
  });
  await page.setViewportSize({ width: 1280, height: 720 }); await page.goto('/?lang=ko&mute&aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu'); await clickLabel(page, '캠페인'); await clickLabel(page, '초기 미션');
  const second = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'Tutorial2')?.label);
  expect(second).toMatch(/[가-힣]/); await clickLabel(page, second);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 50000 });
  await page.waitForFunction(() => Boolean(window.netstorm._mainMenu.dialog?.content), null, { timeout: 15000 });
  const brief = await page.evaluate(() => window.netstorm._mainMenu.dialog.content.styleText);
  expect(brief).toMatch(/[A-Za-z]{4}/); expect(brief).not.toMatch(/[가-힣]/); expect(errors).toEqual([]);
});
