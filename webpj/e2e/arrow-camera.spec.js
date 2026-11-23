// 방향키 카메라 이동(사용자 요구 2026-11-23, cpppj 1328 계약)을 실제 Chromium 에서 확인한다. (2026-11-23 WB04-E1291)
// 계약: 게임 화면에서 ↑↓←→ 누름·자동 반복마다 논리 화면 32픽셀을 옮기고, 입력창(채팅)·열린 메뉴·정지·키 뗌에서는 움직이지 않으며, 조각을 든 채의 ←/→ 는 회전이 먼저이다(↑/↓ 는 항상 카메라).
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

// 현재 적용된 카메라 위치와 목표 위치이다(Present 뒤 적용되므로 키 뒤 잠시 기다린 다음 읽는다).
const camera = (page) => page.evaluate(() => { const gameplay = window.netstorm._clientGameplay.camera(), view = gameplay.snapshot().pixels, target = gameplay.target().pixels; return { view: [view.x, view.y], target: [target[0], target[1]] }; });
// 조각 방향(0~7 짝수)이다.
const direction = (page) => page.evaluate(() => window.netstorm._campaignWorld.session().cursorDropSettings().direction >>> 0);

test('The War Begins!: 방향키가 32픽셀씩 카메라를 옮기고 입력창·메뉴·정지·키 뗌·조각 회전을 존중한다', async ({ page }) => {
  test.setTimeout(300000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await startWarBegins(page);
  const viewport = VIEWPORTS[ASPECT], center = { x: Math.round(viewport.width * 0.55), y: Math.round(viewport.height * 0.5) };
  await page.mouse.move(center.x, center.y); await page.waitForTimeout(600);
  // 네 방향: 누를 때마다 한 변이 32씩 바뀌고 원래 자리로 돌아온다.
  const start = (await camera(page)).view;
  const press = async (key) => { await page.keyboard.press(key); await page.waitForTimeout(500); return (await camera(page)).view; };
  expect(await press('ArrowRight'), '오른쪽').toEqual([start[0] + 32, start[1]]);
  expect(await press('ArrowDown'), '아래').toEqual([start[0] + 32, start[1] + 32]);
  expect(await press('ArrowLeft'), '왼쪽').toEqual([start[0], start[1] + 32]);
  expect(await press('ArrowUp'), '위').toEqual(start);
  // 누르고 있으면 브라우저 자동 반복마다 한 번씩 더 옮긴다(키 보관·타이머 없음). 뗄 때(keyup)는 움직이지 않는다.
  await page.keyboard.down('ArrowRight'); for (let repeat = 0; repeat < 4; repeat += 1) { await page.keyboard.down('ArrowRight'); await page.waitForTimeout(60); }
  await page.waitForTimeout(400); const held = (await camera(page)).view; await page.keyboard.up('ArrowRight'); await page.waitForTimeout(500);
  const released = (await camera(page)).view;
  expect((held[0] - start[0]) % 32, '반복 이동은 32의 배수').toBe(0); expect(held[0] - start[0], '반복 이동 횟수').toBeGreaterThanOrEqual(64); expect(held[1], '반복은 세로를 움직이지 않는다').toBe(start[1]);
  expect(released, '키 뗌은 움직이지 않는다').toEqual(held);
  // 정지 중에는 움직이지 않는다(Shift-F9 로 정지).
  await page.keyboard.press('Shift+F9'); await page.waitForFunction(() => window.netstorm._gameClock.paused(), null, { timeout: 10000 }); await page.waitForTimeout(300);
  const paused = (await camera(page)).view; await page.keyboard.press('ArrowLeft'); await page.waitForTimeout(400); expect((await camera(page)).view, '정지 중 방향키').toEqual(paused);
  await page.keyboard.press('Shift+F9'); await page.waitForFunction(() => !window.netstorm._gameClock.paused(), null, { timeout: 10000 });
  // ESC 게임 메뉴막대가 열려 있으면 방향키로 카메라가 움직이지 않는다.
  await page.keyboard.press('Escape'); await page.waitForFunction(() => window.netstorm._campaignProduction.ui.gameMenu().isOpen(), null, { timeout: 10000 });
  const menuBefore = (await camera(page)).view; // 네 방향이 서로 상쇄되어 가려지지 않도록 키마다 따로 확인한다.
  for (const key of ['ArrowLeft', 'ArrowRight', 'ArrowUp', 'ArrowDown']) { await page.keyboard.press(key); await page.waitForTimeout(400); expect((await camera(page)).view, `열린 메뉴 중 ${key}`).toEqual(menuBefore); }
  await page.keyboard.press('Escape'); await page.waitForFunction(() => !window.netstorm._campaignProduction.ui.gameMenu().isOpen(), null, { timeout: 10000 });
  // 채팅 입력줄이 보이는 동안 방향키는 입력줄이 쓰며 카메라는 움직이지 않는다. (Enter 로 입력줄을 연다)
  await page.keyboard.press('Enter'); await page.waitForFunction(() => window.netstorm._campaignProduction.ui.chat()?.inputVisible(), null, { timeout: 10000 });
  const chatBefore = (await camera(page)).view;
  for (const key of ['ArrowLeft', 'ArrowRight', 'ArrowUp', 'ArrowDown']) { await page.keyboard.press(key); await page.waitForTimeout(400); expect((await camera(page)).view, `채팅 입력 중 ${key}`).toEqual(chatBefore); }
  // 빈 입력줄에서 Enter 로 입력줄을 닫는다(ESC 는 입력줄을 닫지 않고 게임 메뉴막대를 연다). 메뉴막대가 열렸으면 닫는다.
  await page.keyboard.press('Enter'); await page.waitForFunction(() => !window.netstorm._campaignProduction.ui.chat()?.inputVisible(), null, { timeout: 10000 });
  if (await page.evaluate(() => window.netstorm._campaignProduction.ui.gameMenu().isOpen())) { await page.keyboard.press('Escape'); await page.waitForFunction(() => !window.netstorm._campaignProduction.ui.gameMenu().isOpen(), null, { timeout: 10000 }); }
  // 조각을 든 채의 ←/→ 는 회전이 먼저이고 카메라는 움직이지 않는다. ↑/↓ 는 조각을 들어도 카메라를 옮긴다.
  await page.mouse.move(center.x, center.y); await page.keyboard.press('q'); await page.waitForFunction(() => window.netstorm._campaignWorld.session().cursorDropSettings().preview !== 0, null, { timeout: 10000 });
  const pieceView = (await camera(page)).view, d0 = await direction(page);
  await page.keyboard.press('ArrowRight'); await page.waitForTimeout(400); expect((await direction(page) - d0 + 8) % 8, '→ 는 +2 회전').toBe(2); expect((await camera(page)).view, '→ 회전은 카메라를 옮기지 않는다').toEqual(pieceView);
  await page.keyboard.press('ArrowLeft'); await page.waitForTimeout(400); expect(await direction(page), '← 는 −2 회전').toBe(d0); expect((await camera(page)).view, '← 회전은 카메라를 옮기지 않는다').toEqual(pieceView);
  await page.keyboard.press('ArrowDown'); await page.waitForTimeout(500); const down = (await camera(page)).view; expect(down, '조각을 들어도 ↓ 는 카메라').toEqual([pieceView[0], pieceView[1] + 32]); expect(await direction(page), '↓ 는 회전하지 않는다').toBe(d0);
  expect(errors).toEqual([]);
});
