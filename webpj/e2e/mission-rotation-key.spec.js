// 조각 회전 방향(C 키와 옵션 "Counterclockwise Piece Rotation", 설정 rotmode)이 실제로 우클릭 회전 방향을 바꾸는지 실제 Chromium 에서 확인한다. (2026-11-21 WB04-E1151)
// 연결 전에는 rotmode 설정이 회전 방향 전역에 반영되지 않아 옵션을 켜도 C 키를 눌러도 언제나 시계 방향(0→2→4→6)으로만 돌았고, 설정도 저장되지 않았다.
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

// 현재 조각 방향(0~7 짝수), 회전 방향 전역, 게임 데이터·설정의 rotmode 와 로컬 저장소에 저장된 rotmode 를 읽는다.
const state = (page) => page.evaluate(() => {
  const app = window.netstorm; let stored = null; try { stored = JSON.parse(localStorage.getItem('netstorm.settings') ?? '{}').values?.rotmode ?? null; } catch { stored = 'error'; }
  return { direction: app._campaignWorld.session().cursorDropSettings().direction >>> 0, mode: app._clientGameplay.cursorGlobals().rotationMode().value >>> 0, data: app._gameData.readInt('rotmode', 0).value | 0, settings: app._settings.get('rotmode') ?? null, stored };
});

test('The War Begins!: C 키와 설정이 조각 회전 방향(우클릭)을 바꾸고 설정에 저장된다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await startWarBegins(page);
  const viewport = VIEWPORTS[ASPECT], center = { x: Math.round(viewport.width * 0.55), y: Math.round(viewport.height * 0.5) };
  const held = () => page.evaluate(() => window.netstorm._campaignWorld.session().cursorDropSettings().preview !== 0);
  await page.mouse.move(center.x, center.y); await page.keyboard.press('Escape'); await page.waitForTimeout(300);
  await page.keyboard.press('q'); await page.waitForTimeout(500); expect(await held(), 'Q 로 다리 조각 쥐기').toBe(true);
  // 우클릭으로 한 번 돌리고 달라진 방향을 돌려준다(조각을 쥔 채 맵 위의 우클릭은 회전이다).
  const rotate = async () => { const before = (await state(page)).direction; await page.mouse.click(center.x, center.y, { button: 'right', delay: 60 }); await page.waitForTimeout(250); const after = (await state(page)).direction; return (after - before + 8) % 8; };
  const initial = await state(page); expect(initial.mode, '처음 회전 방향 전역').toBe(0); expect(initial.stored, '처음에는 저장값이 없다').toBeNull();
  expect(await rotate(), '기본: 시계 방향 +2').toBe(2); expect(await rotate(), '기본: 시계 방향 +2 (두 번째)').toBe(2);
  // C 키: 회전 방향이 뒤집히고 같은 값이 rotmode 로 게임 데이터·설정·로컬 저장소에 기록된다. 이후 우클릭은 반시계 방향(−2 → 6)이다.
  await page.keyboard.press('c'); await page.waitForTimeout(300);
  const flipped = await state(page); expect(flipped, 'C 한 번 뒤의 상태').toMatchObject({ mode: 1, data: 1, settings: 1, stored: 1 });
  expect(await held(), 'C 키는 쥔 조각을 놓지 않는다').toBe(true);
  expect(await rotate(), 'C 뒤: 반시계 방향 −2').toBe(6); expect(await rotate(), 'C 뒤: 반시계 방향 −2 (두 번째)').toBe(6);
  // 한 번 더 누르면 원래대로 시계 방향이다.
  await page.keyboard.press('c'); await page.waitForTimeout(300);
  expect(await state(page), 'C 두 번 뒤의 상태').toMatchObject({ mode: 0, data: 0, settings: 0, stored: 0 }); expect(await rotate(), 'C 두 번 뒤: 시계 방향 +2').toBe(2);
  // 옵션 메뉴의 "Counterclockwise Piece Rotation" 토글이 쓰는 설정 변경도 회전 방향 전역에 반영된다(설정 층에 쓰고 통지).
  await page.evaluate(() => window.netstorm._applyCampaignConfig('rotmode', 1)); await page.waitForTimeout(300);
  expect(await state(page), '옵션 토글 뒤의 상태').toMatchObject({ mode: 1, data: 1, settings: 1, stored: 1 }); expect(await rotate(), '옵션 토글 뒤: 반시계 방향 −2').toBe(6);
  expect(errors).toEqual([]);
});
