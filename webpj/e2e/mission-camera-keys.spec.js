// 카메라 키(Shift+숫자 저장 / 숫자 호출, TAB 최근 건설 위치 순환, U 최근 손실 위치 이동)를 실제 Chromium 에서 확인한다. (2026-11-21 WB04-E1154)
// 원본 도움말: "Shift 를 누른 채 0~9 를 누르면 화면 위치를 기억하고 숫자 키로 돌아간다", "TAB 은 최근 놓은 유닛 다섯 곳을 차례로 보여 준다", "U 는 가장 최근에 잃은 유닛의 위치로 간다".
// 연결 전에는 세 분배기가 앱 입력 루프에 없어 이 키들이 아무 일도 하지 않았다.
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

// 지도 영역 가운데가 가리키는 칸 좌표이다(카메라 픽셀 원점과 칸 픽셀 16×11 로 환산). 원본 Shift+숫자 는 이 지도 영역 중심을 저장한다.
const mapCenter = (page) => page.evaluate(() => { const app = window.netstorm, view = app._clientGameplay.camera().rendererView(), map = app._campaignWorld.mapView();
  return { x: ((map.left + map.right) / 2 + view.pixels.x) / 16, y: ((map.top + map.bottom) / 2 + view.pixels.y) / 11 }; });
// 전체 화면 가운데가 가리키는 칸 좌표이다. 원본의 즉시 중심 이동(CenterViewPoint)은 HUD 를 뺀 지도 영역이 아니라 화면 전체 크기를 기준으로 하므로 이동 뒤의 중심은 이 값이 요청 좌표와 같다.
const center = (page) => page.evaluate(() => { const app = window.netstorm, view = app._clientGameplay.camera().rendererView();
  return { x: (app._layout.logicalWidth / 2 + view.pixels.x) / 16, y: (app._layout.logicalHeight / 2 + view.pixels.y) / 11 }; });

// 두 칸 좌표의 거리(칸 단위)이다.
const distance = (first, second) => Math.hypot(first.x - second.x, first.y - second.y);

// Alt 를 누른 채 포인터를 멀리 옮겨 화면을 크게 이동시킨다. 끝나면 포인터를 가운데로 되돌린다.
async function scrollAway(page) {
  const viewport = VIEWPORTS[ASPECT], middle = { x: Math.round(viewport.width * 0.55), y: Math.round(viewport.height * 0.5) };
  await page.mouse.move(middle.x, middle.y); await page.keyboard.down('Alt'); await page.mouse.move(middle.x + viewport.width * 0.4, middle.y + viewport.height * 0.35); await page.waitForTimeout(1200);
  await page.keyboard.up('Alt'); await page.mouse.move(middle.x, middle.y); await page.waitForTimeout(300);
}

// 새 페이지의 오류를 모은다.
function collectErrors(page) { const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); }); return errors; }

test('The War Begins!: Shift+숫자로 화면 위치를 저장하고 숫자 키로 돌아오며, 저장하지 않은 칸은 아무 일도 하지 않는다', async ({ page }) => {
  test.setTimeout(300000); const errors = collectErrors(page); await startWarBegins(page);
  const slot = (index) => page.evaluate((target) => window.netstorm._clientGameplay.camera().savedPositions().get(target), index);
  expect((await slot(0)).x, '처음에는 무효 칸(-10000)').toBe(-10000);
  const home = await mapCenter(page);
  // Shift+1: 선택음 뒤 현재 지도 중심을 1번 칸에 저장한다.
  await page.keyboard.press('Shift+Digit1'); await page.waitForTimeout(300);
  const saved = await slot(0); expect(distance(saved, home), `저장한 칸 ${JSON.stringify(saved)} 은 현재 중심 ${JSON.stringify(home)} 이다`).toBeLessThan(1);
  // 멀리 옮긴 뒤 1 을 누르면 저장한 곳으로 돌아온다.
  await scrollAway(page); const away = await mapCenter(page); expect(distance(away, home), '화면이 멀리 옮겨졌다').toBeGreaterThan(8);
  await page.keyboard.press('Digit1'); await page.waitForTimeout(800);
  expect(distance(await center(page), saved), '1 키로 저장한 위치가 화면 중심이 된다').toBeLessThan(0.7);
  // 저장하지 않은 2번 칸은 효과가 없다(원본은 무효 칸이면 이동하지 않는다).
  await scrollAway(page); const second = await mapCenter(page); await page.keyboard.press('Digit2'); await page.waitForTimeout(500);
  expect(distance(await mapCenter(page), second), '저장하지 않은 칸은 이동하지 않는다').toBeLessThan(0.5);
  // 0 키는 10번째 칸이다: Shift+0 으로 저장하고 0 으로 부른다.
  await page.keyboard.press('F4'); await page.waitForTimeout(800); await page.keyboard.press('Shift+Digit0'); await page.waitForTimeout(300); expect((await slot(9)).x, '10번째 칸 저장').toBeGreaterThan(0);
  await scrollAway(page); await page.keyboard.press('Digit0'); await page.waitForTimeout(800);
  expect(distance(await center(page), await slot(9)), '0 키로 10번째 칸을 부른다').toBeLessThan(0.7);
  expect(errors).toEqual([]);
});

// TAB 은 원본 도움말대로 "최근에 놓은 유닛 다섯 곳"을 차례로 보여 준다. 이력은 전투 유닛(궁수·대포·방벽·비행장·포대·울타리·보행자·기구)을 놓을 때 쌓이며 제단·워크샾·신전·다리는 쌓지 않는다(원본 그룹 10 제외).
// 유닛 배치에는 생산이 필요해 오래 걸리므로 이력 칸(Client 장기 전역)을 직접 채우고 키의 순환·정지 규칙을 확인한다. 이력을 쌓는 통지 본문(RawConstructionNotice)은 Node 시험이 따로 다룬다.
test('The War Begins!: TAB 은 최근 건설 위치 이력을 차례로 돌고, U 는 가장 최근에 잃은 위치로 화면을 옮긴다', async ({ page }) => {
  test.setTimeout(300000); const errors = collectErrors(page); await startWarBegins(page);
  const home = await center(page), first = { x: home.x + 14, y: home.y + 9 }, second = { x: home.x + 22, y: home.y + 15 };
  const history = () => page.evaluate(() => window.netstorm._clientGameplay.camera().constructionHistory().index);
  expect(await history(), '처음 순번').toBe(0);
  // 이력: [첫째, 둘째, 무효…]. TAB 순서는 첫째 → 둘째 → (다음 칸이 무효라) 순번이 0 으로 돌아와 첫째.
  await page.evaluate(({ first, second }) => { const value = window.netstorm._clientGameplay.camera().constructionHistory(); value.points[0] = { x: first.x, y: first.y }; value.points[1] = { x: second.x, y: second.y }; value.index = 0; }, { first, second });
  await page.keyboard.press('Tab'); await page.waitForTimeout(700); expect(distance(await center(page), first), 'TAB 1: 첫째 위치').toBeLessThan(0.7); expect(await history(), 'TAB 1 뒤 순번').toBe(1);
  await page.keyboard.press('Tab'); await page.waitForTimeout(700); expect(distance(await center(page), second), 'TAB 2: 둘째 위치').toBeLessThan(0.7); expect(await history(), 'TAB 2 뒤 순번').toBe(2);
  await page.keyboard.press('Tab'); await page.waitForTimeout(700); expect(distance(await center(page), first), 'TAB 3: 다음 칸이 무효이면 순번 0 으로 돌아와 첫째').toBeLessThan(0.7); expect(await history(), 'TAB 3 뒤 순번').toBe(1);
  // U: 가장 최근에 잃은 위치(Client 전역 lostX/lostY, 삭제 경로가 쓰는 값)로 이동한다. 이 값은 유닛이 파괴될 때 쓰이므로 시험은 값을 직접 쓴다.
  const lost = { x: home.x - 6, y: home.y + 4 };
  await page.evaluate((point) => { const position = window.netstorm._clientGameplay.gameAux().lostPosition(); position[0] = point.x; position[1] = point.y; }, lost);
  await page.keyboard.press('u'); await page.waitForTimeout(700); expect(distance(await center(page), lost), 'U 는 최근 손실 위치로 이동한다').toBeLessThan(0.7);
  // 좌표가 무효이면(원점 0) 이동하지 않는다.
  await page.evaluate(() => { const position = window.netstorm._clientGameplay.gameAux().lostPosition(); position[0] = 0; position[1] = 0; });
  const stay = await center(page); await page.keyboard.press('u'); await page.waitForTimeout(500); expect(distance(await center(page), stay), '무효 손실 위치는 이동하지 않는다').toBeLessThan(0.3);
  expect(errors).toEqual([]);
});
