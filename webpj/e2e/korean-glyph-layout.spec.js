// 한국어 글자 출력의 격자·혼합 규칙을 실제 Chromium 의 실제 갈무리11 글꼴과 실제 미션 화면으로 확인한다. (2026-11-23 WB04-E1288)
//  - 갈무리11 은 12px 설계 격자 글꼴이라 12px 로 그려야 1px 획이 빠지지 않는다(이전 11px 에서는 `가`→`기`, `제`→`세` 처럼 읽혔다).
//  - Canvas 의 가로 번짐 때문에 1px 간격의 두 획(ㅔ)이 메워지지 않도록 래스터라이저가 4배로 그려 칸 평균을 쓴다.
//  - 숫자·영문·공백은 원본 비트맵 글리프로 그린다: 사이드바 SP 숫자가 영어 화면과 픽셀까지 같고, Early Mission 3(사제 포획) D1 안내의 그림 세 개가 한 줄에 놓인다.
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

// 같은 설정으로 앱을 열어 메인 메뉴까지 진행한다. 진행 저장에는 Tutorial1·2 완료를 넣어 Early Mission 3 을 고를 수 있게 한다.
async function openMainMenu(page, lang) {
  await page.addInitScript(() => {
    localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, autoDemo: 0 } }));
    localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: { DoneTutorial1: '1', DoneTutorial2: '1' } }));
  });
  await page.setViewportSize({ width: 1024, height: 768 });
  await page.goto(`/?lang=${lang}&mute&aspect=4:3`);
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
}

// Early Mission 3(Tutorial3, 사제 포획)을 시작해 안내 창을 모두 닫고, SP 표시가 더 바뀌지 않을 때까지 기다린 뒤 그 영역 화면을 돌려준다.
async function startTutorial3(page, lang) {
  await openMainMenu(page, lang);
  await clickLabel(page, lang === 'korean' ? '캠페인' : 'Campaign');
  const group = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.map((item) => item.label).find((label) => /초기|Early/.test(label)));
  await clickLabel(page, group);
  const label = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'Tutorial3')?.label);
  await clickLabel(page, label);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 60000 });
  await page.waitForFunction(() => Boolean(window.netstorm._mainMenu.dialog?.content), null, { timeout: 15000 });
  // 브리핑·안내 창을 마지막 단추로 닫는다. 창이 없는 상태가 이어지면 멈춘다.
  for (let quiet = 0; quiet < 4;) {
    const button = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons?.at(-1)?.label ?? null);
    if (button === null) { quiet += 1; await page.waitForTimeout(400); continue; }
    quiet = 0; await clickLabel(page, button); await page.waitForTimeout(400);
  }
  // SP 숫자는 목표 금액까지 단계적으로 움직이므로 두 번 연속 같은 화면이 될 때까지 기다린다.
  const clip = { x: 0, y: 0, width: 85, height: 24 };
  let previous = await page.screenshot({ clip });
  for (let tries = 0; tries < 20; tries += 1) {
    await page.waitForTimeout(500);
    const current = await page.screenshot({ clip });
    if (current.equals(previous)) return current;
    previous = current;
  }
  throw new Error('SP 표시가 멈추지 않음');
}

test('실제 갈무리11 래스터라이저: 12px 격자에 맞고 1px 간격의 두 획이 메워지지 않는다', async ({ page }) => {
  test.setTimeout(120000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.message));
  await openMainMenu(page, 'ko');
  const result = await page.evaluate(() => {
    const fonts = window.netstorm._resources.fonts, font = fonts.unicodeFont({ font: 0, style: 0 });
    // 글리프 그림에서 잉크(색 번호 비영)가 있는 줄 수와 각 줄의 잉크 열 문자열을 구한다.
    const shape = (code) => {
      const glyph = font.glyph(code), image = glyph.image, rows = [];
      for (let y = 0; y < image.height; y += 1) { let line = ''; for (let x = 0; x < image.width; x += 1) line += image.indices[y * image.width + x] ? '#' : '.'; if (line.includes('#')) rows.push(line); }
      return { advance: glyph.advance, rows };
    };
    return { em: font.spec.emSize, ga: shape(0xac00), e: shape(0xc5d0), space: font.glyph(0x20).advance, nativeSpace: fonts.get(0, 0).glyph(0x20).advance };
  });
  expect(result.em).toBe(12);
  // 한글은 전진 폭 12 이고 잉크가 정확히 11줄이다(12px 설계 격자).
  expect([result.ga.advance, result.ga.rows.length]).toEqual([12, 11]);
  expect([result.e.advance, result.e.rows.length]).toEqual([12, 11]);
  // 에(ㅔ)의 두 세로획은 1px 간격으로 떨어져 있어야 한다: 오른쪽 끝 세 칸이 `#.#` 인 줄이 여러 개 있다(번짐으로 메워지면 `###`).
  expect(result.e.rows.filter((line) => line.replace(/\.+$/, '').endsWith('#.#')).length, result.e.rows.join('\n')).toBeGreaterThanOrEqual(5);
  // 공백은 원본 Arial 14 굵은 글꼴의 폭(3)이다.
  expect([result.space, result.nativeSpace]).toEqual([3, 3]);
  expect(errors).toEqual([]);
});

test('사제 포획(Tutorial3): SP 숫자가 영어 화면과 같고 D1 안내의 그림 세 개가 한 줄에 놓인다', async ({ browser }) => {
  test.setTimeout(240000);
  const errors = [];
  const english = await browser.newPage(), korean = await browser.newPage();
  for (const page of [english, korean]) page.on('pageerror', (error) => errors.push(error.message));
  const englishBudget = await startTutorial3(english, 'en');
  const koreanBudget = await startTutorial3(korean, 'korean');
  // 사이드바 맨 위 SP 표시(85×24)는 한국어에서도 원본 Arial 20 숫자라 영어 화면과 픽셀까지 같다(이전에는 갈무리11 2배 숫자라 컸다).
  expect(koreanBudget.equals(englishBudget), 'SP 표시 픽셀 동일').toBe(true);
  // D1 안내(제네레이터, 두 번째)를 열고 그림이 있는 줄을 확인한다.
  const layout = async (page) => {
    await page.evaluate(() => window.netstorm._openTell('D1.'));
    await page.waitForFunction(() => window.netstorm._gameData.currentTell === 'D1.' && Boolean(window.netstorm._mainMenu.dialog?.body), null, { timeout: 15000 });
    return page.evaluate(() => { const body = window.netstorm._mainMenu.dialog.body;
      return { width: body.width, pictures: body.softLines.map((line) => ({ count: line.fragments.filter((fragment) => fragment.picture).length, width: line.width })).filter((line) => line.count > 0) }; });
  };
  const englishLayout = await layout(english), koreanLayout = await layout(korean);
  // 영어와 같이 그림 세 개가 한 줄이며, 공백이 원본 폭이라 그 줄의 폭도 영어와 같다(이전에는 갈무리11 공백 5px 로 줄이 넘쳐 세 번째 그림이 다음 줄로 내려갔다).
  expect(englishLayout.pictures.map((line) => line.count)).toEqual([3]);
  expect(koreanLayout.pictures, JSON.stringify(koreanLayout)).toEqual(englishLayout.pictures);
  expect(koreanLayout.pictures[0].width).toBeLessThanOrEqual(koreanLayout.width);
  expect(errors).toEqual([]);
  await english.close(); await korean.close();
});
