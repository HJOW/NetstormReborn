// ESC 가 열린 Tell 창을 닫는지 실제 Chromium 에서 확인한다. 원본 규칙(cpppj UberGump.cpp:821·1810~1823): 브리핑·미션 로딩 단계의 창은 ESC 를 무시하고,
// 그 밖의 Tell 창(Dialog 단계: 메뉴의 Credits·미션 중 ABORT 확인·결과 안내)은 `Tell Blank` 로 닫힌다. (2026-11-23 WB04-E1264)
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

// 새 페이지의 오류·경고를 모은다.
function collectErrors(page) { const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); }); return errors; }

// 현재 안내 창의 단추 이름 목록이다(창이 없으면 null).
const buttons = (page) => page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons?.map((button) => button.label) ?? null);

// 지정한 단추 목록의 안내 창이 열릴 때까지 기다린다.
async function waitButtons(page, expected, message) { await expect.poll(() => buttons(page), { timeout: 15000, message }).toEqual(expected); }

// 안내 창이 모두 닫히고 게임 시계가 흐를 때까지 마지막(주) 단추로 닫는다.
async function closeDialogs(page) {
  for (let index = 0; index < 40; index += 1) {
    const state = await page.evaluate(() => ({ button: window.netstorm._mainMenu.dialog?.buttons?.at(-1)?.label ?? null, paused: window.netstorm._gameClock.paused() }));
    if (state.button !== null) { await clickLabel(page, state.button); await page.waitForTimeout(500); continue; }
    if (!state.paused) { await page.waitForTimeout(400); if (!(await page.evaluate(() => window.netstorm._gameClock.paused() || Boolean(window.netstorm._mainMenu.dialog)))) return; continue; }
    await page.waitForTimeout(500);
  }
  await page.waitForFunction(() => !window.netstorm._gameClock.paused(), null, { timeout: 20000 });
}

// 플레이어 번호의 활성 플래그를 지워 판정기가 그 플레이어를 전멸로 보게 한다(로컬 1 → 사제 사망·실패).
const knockOut = (page, owner) => page.evaluate((target) => { const records = window.netstorm._campaignWorld.session().playerRecords(); records[target * 180 + 0x50] = 0; }, owner);

// 메인 메뉴까지 가서(환영·팁 창 없이) Priest Training 의 미션 하나를 시작한다. 시작 브리핑이 열린 상태에서 돌려준다(닫지 않는다).
async function startMissionKeepBriefing(page, mission) {
  await page.addInitScript(() => {
    localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, autoDemo: 0 } }));
    const done = {}; for (const name of ['Tutorial1', 'Tutorial2', 'Tutorial3', 'Tutorial4', 'Tutorial5', 'Tutorial6', 'bc1menu76', 'bc2bridging76', 'bc3acid_bar76', 'bc4bow_rush76', 'bc5walkers76', 'bc6mows76', 'bc7sun_bars76', 'bc8fury76', 'bc9ssun_bars76']) done[`Done${name}`] = '1'; localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: done }));
  });
  await page.setViewportSize(VIEWPORTS[ASPECT]); await page.goto(`/?aspect=${ASPECT}`);
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  await clickLabel(page, 'Campaign'); await clickLabel(page, 'Priest Training');
  const label = await page.evaluate((name) => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === name)?.label, mission);
  await clickLabel(page, label);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
}

// 메인 메뉴의 Tell 창(Credits 목록)은 Dialog 단계라 ESC 로 닫힌다.
test('메인 메뉴: Credits 창은 ESC 로 닫힌다', async ({ page }) => {
  const errors = collectErrors(page);
  await page.addInitScript(() => localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, autoDemo: 0 } })));
  await page.setViewportSize(VIEWPORTS[ASPECT]); await page.goto(`/?aspect=${ASPECT}`);
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  await clickLabel(page, 'Credits');
  await expect.poll(() => page.evaluate(() => Boolean(window.netstorm._mainMenu.dialog)), { timeout: 10000, message: 'Credits 목록 창' }).toBe(true);
  await page.keyboard.press('Escape');
  await expect.poll(() => page.evaluate(() => Boolean(window.netstorm._mainMenu.dialog)), { timeout: 5000, message: 'ESC 뒤' }).toBe(false);
  expect(await page.evaluate(() => window.netstorm.state)).toBe('mainMenu');
  expect(errors).toEqual([]);
});

// 미션의 시작 브리핑은 ESC 를 무시하고(Briefing 단계), 미션 중 연 ABORT 확인 창은 ESC 로 닫히며 미션은 계속된다(Dialog 단계).
test('Death Bridge: 시작 브리핑은 ESC 를 무시하고 ABORT 확인 창은 ESC 로 닫힌다', async ({ page }) => {
  test.setTimeout(300000); const errors = collectErrors(page);
  await startMissionKeepBriefing(page, 'bc5walkers76');
  // 시작 브리핑(기술 목록·시작 단추)이 열려 있고 브리핑 단계로 표시돼 있다.
  await waitButtons(page, ['Technology available', 'Lets do this!'], '시작 브리핑');
  expect(await page.evaluate(() => window.netstorm._tellBriefing), '브리핑 단계').toBe(true);
  await page.keyboard.press('Escape'); await page.waitForTimeout(800);
  expect(await buttons(page), 'ESC 뒤에도 브리핑').toEqual(['Technology available', 'Lets do this!']);
  await closeDialogs(page);
  // 미션 중 ABORT 확인 창은 Dialog 단계이며 ESC 가 닫는다. 시계는 계속 흐르고 오류가 없다.
  await page.evaluate(() => window.netstorm._openTell('ABORT'));
  await waitButtons(page, ['Main Menu', 'Replay Mission', 'Continue Mission'], 'ABORT 확인 창');
  expect(await page.evaluate(() => window.netstorm._tellBriefing), 'Dialog 단계').toBe(false);
  await page.keyboard.press('Escape');
  await expect.poll(() => buttons(page), { timeout: 5000, message: 'ESC 뒤' }).toBeNull();
  const before = await page.evaluate(() => window.netstorm._gameClock.gameSeconds()); await page.waitForTimeout(1500);
  expect(await page.evaluate(() => window.netstorm._gameClock.gameSeconds()) - before, 'ESC 로 닫은 뒤 시계').toBeGreaterThan(1);
  expect(await page.evaluate(() => window.netstorm._campaignDisplayActive()), '미션 계속').toBe(true);
  expect(errors).toEqual([]);
});

// 결과 안내(실패)도 Dialog 단계라 ESC 로 닫힌다(원본 missionOutcome.section → TryTell(section, false)). 닫은 뒤에도 게임 메뉴(ESC)로 미션을 떠날 수 있다.
test('Death Bridge: 실패 안내는 ESC 로 닫히고 이후 ESC 는 게임 메뉴막대를 연다', async ({ page }) => {
  test.setTimeout(300000); const errors = collectErrors(page);
  await startMissionKeepBriefing(page, 'bc5walkers76'); await closeDialogs(page);
  await knockOut(page, 1);
  await waitButtons(page, ['Training Menu', 'Restart', 'Leave Missions'], '실패 안내');
  expect(await page.evaluate(() => window.netstorm._tellBriefing), '결과 안내는 Dialog 단계').toBe(false);
  await page.keyboard.press('Escape');
  await expect.poll(() => buttons(page), { timeout: 5000, message: 'ESC 뒤' }).toBeNull();
  // 같은 키를 한 번 더 누르면 창이 없으므로 게임 메뉴막대가 열린다(원본 ESC 의 다음 분기).
  await page.keyboard.press('Escape'); await page.waitForTimeout(500);
  expect(await page.evaluate(() => window.netstorm._campaignProduction.ui.gameMenu().isOpen()), '두 번째 ESC 는 게임 메뉴').toBe(true);
  expect(errors).toEqual([]);
});
