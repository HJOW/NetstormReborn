// 원본 도움말 키 목록 중 미션에서 동작해야 하는 단축키(F1 일반 도움말·F2 건물 숨김·Shift-F3 섬 테마·F8 목표 보기·Ctrl-F5 사제/운반체 선택)를 실제 Chromium 에서 확인한다. (2026-11-21 WB04-E1155)
// F3(채팅 창 숨김/표시)은 채팅 창이 아직 없어 제외한다(LEFT_JOBS 의 채팅 창 공장).
const { test, expect } = require('@playwright/test');

// 화면비를 환경 변수 NETSTORM_E2E_ASPECT('4:3' 기본, '16:9', '16:10')로 고른다.
const ASPECT = process.env.NETSTORM_E2E_ASPECT ?? '4:3';
const VIEWPORTS = { '4:3': { width: 1024, height: 768 }, '16:9': { width: 1280, height: 720 }, '16:10': { width: 1280, height: 800 } };
if (!VIEWPORTS[ASPECT]) throw new Error(`지원하지 않는 화면비 ${ASPECT}`);

// 실제 돌 버튼의 논리 좌표를 브라우저 마우스 좌표로 바꿔 누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 30000 });
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

// The War Begins! 를 시작하고 안내 창을 닫아 시계가 흐르게 한다.
async function startWarBegins(page) {
  await page.addInitScript(() => {
    localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } }));
    const done = {}; for (const name of ['Tutorial1', 'Tutorial2', 'Tutorial3', 'Tutorial4', 'Tutorial5', 'Tutorial6']) done[`Done${name}`] = '1'; localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: done }));
  });
  await page.setViewportSize(VIEWPORTS[ASPECT]); await page.goto(`/?aspect=${ASPECT}`);
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Struggle For Freedom');
  const label = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === 'TheWarBegins')?.label);
  await clickLabel(page, label);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
  for (let index = 0; index < 10; index += 1) {
    const button = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons?.at(-1)?.label ?? null);
    if (button === null) break; await clickLabel(page, button); await page.waitForTimeout(500);
  }
  await page.waitForFunction(() => !window.netstorm._gameClock.paused(), null, { timeout: 20000 });
}

// 새 페이지의 오류를 모은다.
function collectErrors(page) { const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); }); return errors; }

test('The War Begins!: F1 도움말·F2 건물 숨김·Shift-F3 섬 테마·F8 목표 보기·Ctrl-F5 선택이 동작한다', async ({ page }) => {
  test.setTimeout(300000); const errors = collectErrors(page); await startWarBegins(page);
  const state = () => page.evaluate(() => { const app = window.netstorm, game = app._clientGameplay; return { dialog: app._mainMenu.dialog?.constructor.name ?? null, buttons: app._mainMenu.dialog?.buttons?.map((button) => button.label) ?? null, paused: app._gameClock.paused(), hide: game.hideBuildings().value, themes: app._gameData.readInt('theammode', 1).value, selected: game.selection.selected }; });
  await page.mouse.move(540, 380);
  // F1: 일반 도움말 창이 열린다(게임은 멈추지 않는다).
  await page.keyboard.press('F1'); await expect.poll(async () => (await state()).dialog, { timeout: 5000, message: 'F1 도움말' }).toBe('HelpWindow'); expect((await state()).paused, 'F1 도움말은 시계를 멈추지 않는다').toBe(false);
  await page.evaluate(() => window.netstorm._mainMenu.dialog.closeWindow()); await expect.poll(async () => (await state()).dialog).toBeNull();
  // F2: 건물 숨김이 켜지고 한 번 더 누르면 꺼진다.
  await page.keyboard.press('F2'); await expect.poll(async () => (await state()).hide, { message: 'F2 켬' }).toBe(1);
  await page.keyboard.press('F2'); await expect.poll(async () => (await state()).hide, { message: 'F2 끔' }).toBe(0);
  // Shift-F3: 섬 테마 표시(theammode)가 뒤집힌다.
  const themes = (await state()).themes; await page.keyboard.press('Shift+F3'); await expect.poll(async () => (await state()).themes, { message: 'Shift-F3 켬/끔' }).toBe(themes ? 0 : 1);
  await page.keyboard.press('Shift+F3'); await expect.poll(async () => (await state()).themes, { message: 'Shift-F3 되돌림' }).toBe(themes);
  // Ctrl-F5: 운반체(사제)를 차례로 선택한다.
  expect((await state()).selected, '처음에는 선택이 없다').toBe(0);
  await page.keyboard.press('Control+F5'); await expect.poll(async () => (await state()).selected, { timeout: 5000, message: 'Ctrl-F5 선택' }).toBeGreaterThan(0);
  // F8: 미션 목표(브리핑)를 다시 보여 준다. 시계가 멈추고 두 단추(Review Knowledge · Play Mission)가 나온다.
  await page.keyboard.press('F8'); await expect.poll(async () => (await state()).buttons, { timeout: 5000, message: 'F8 목표 보기' }).toEqual(['Review Knowledge', 'Play Mission']); expect((await state()).paused, 'F8 중 시계 정지').toBe(true);
  await clickLabel(page, 'Play Mission'); await expect.poll(async () => (await state()).paused, { timeout: 10000, message: 'Play Mission 뒤 시계 재개' }).toBe(false);
  expect(errors).toEqual([]);
});
