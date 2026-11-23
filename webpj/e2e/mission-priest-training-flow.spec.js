// Priest Training 미션(Death Bridge)의 실패 안내와 그 뒤 흐름(Training Menu 로 목록 미션 bc1menu76 시작, Restart 의 다시 시도 묻기)을 실제 Chromium 에서 확인한다. (2026-11-21 WB04-E1161)
// 실패 조건은 E1152 와 같이 판정기가 읽는 로컬 Player 활성 플래그를 지워 만든다. 대본은 netstorm.tarc 의 bc5walkers76.english 이며 단추 이름은 원본 그대로이다.
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

// 미션 하나를 목록에서 시작하고 안내 창을 모두 닫아 시계가 흐르게 한다. 요청한 대본이 실제로 도는지(데모가 아닌지) 확인한다.
async function startMission(page, group, mission) {
  await page.addInitScript(() => {
    localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, autoDemo: 0 } }));
    const done = {}; for (const name of ['Tutorial1', 'Tutorial2', 'Tutorial3', 'Tutorial4', 'Tutorial5', 'Tutorial6', 'bc1menu76', 'bc2bridging76', 'bc3acid_bar76', 'bc4bow_rush76', 'bc5walkers76', 'bc6mows76', 'bc7sun_bars76', 'bc8fury76', 'bc9ssun_bars76']) done[`Done${name}`] = '1'; localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: done }));
  });
  await page.setViewportSize(VIEWPORTS[ASPECT]); await page.goto(`/?aspect=${ASPECT}`);
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  await clickLabel(page, 'Campaign'); await clickLabel(page, group);
  const label = await page.evaluate((name) => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === name)?.label, mission);
  await clickLabel(page, label);
  await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
  await closeDialogs(page);
}

// 지금 도는 미션의 대본 이름(소문자)과 데모 표시이다.
const current = (page) => page.evaluate(() => ({ script: window.netstorm._clientGuide.current()?.script?.name?.toLowerCase(), demo: window.netstorm._clientGuide.current()?.runtime.gate.demo }));

// 열려 있는 안내 창을 마지막(주) 단추로 닫고, 창이 없고 게임 시계가 흐를 때까지 반복한다(안내 창 사이의 짧은 빈 순간에 끝나지 않도록 시계도 확인한다).
async function closeDialogs(page) {
  for (let index = 0; index < 40; index += 1) {
    const state = await page.evaluate(() => ({ button: window.netstorm._mainMenu.dialog?.buttons?.at(-1)?.label ?? null, paused: window.netstorm._gameClock.paused() }));
    if (state.button !== null) { await clickLabel(page, state.button); await page.waitForTimeout(500); continue; }
    if (!state.paused) { await page.waitForTimeout(400); if (!(await page.evaluate(() => window.netstorm._gameClock.paused() || Boolean(window.netstorm._mainMenu.dialog)))) return; continue; }
    await page.waitForTimeout(500);
  }
  await page.waitForFunction(() => !window.netstorm._gameClock.paused(), null, { timeout: 20000 });
}

// 현재 안내 창의 단추 이름 목록이다(창이 없으면 null).
const buttons = (page) => page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons?.map((button) => button.label) ?? null);

// 지정한 단추 목록의 안내 창이 열릴 때까지 기다린다.
async function waitButtons(page, expected, message) {
  await expect.poll(() => buttons(page), { timeout: 15000, message }).toEqual(expected);
}

// 플레이어 번호의 활성 플래그를 지워 판정기가 그 플레이어를 전멸로 보게 한다(로컬 1 → 사제 사망·실패, 적 2 → 적 팀 전멸·성공).
const knockOut = (page, owner) => page.evaluate((target) => { const records = window.netstorm._campaignWorld.session().playerRecords(); records[target * 180 + 0x50] = 0; }, owner);

test('Death Bridge: 실패 → Training Menu 로 목록 미션 bc1menu76 이 시작되고 Restart 는 다시 시도를 묻는다', async ({ page }) => {
  test.setTimeout(300000); const errors = collectErrors(page);
  await startMission(page, 'Priest Training', 'bc5walkers76');
  expect(await current(page), '요청한 미션이 돈다').toEqual({ script: 'bc5walkers76', demo: 0 });
  await knockOut(page, 1);
  // 실패 구역(대본 [Failed])의 단추 세 개이다.
  await waitButtons(page, ['Training Menu', 'Restart', 'Leave Missions'], 'Death Bridge 실패 안내');
  // Restart → 다시 시도 묻기(Replay Mission / Leave Missions).
  await clickLabel(page, 'Restart'); await waitButtons(page, ['Replay Mission', 'Leave Missions'], '다시 시도 묻기');
  await clickLabel(page, 'Replay Mission'); await waitButtons(page, ['Technology available', 'Lets do this!'], '다시 시작한 브리핑'); await closeDialogs(page);
  expect(await current(page), '다시 시작한 미션').toEqual({ script: 'bc5walkers76', demo: 0 });
  // 다시 실패시켜 Training Menu 를 누르면 대본의 MissionBegin 이 BC1Menu76 을 시작한다(이름은 대소문자 무시).
  await knockOut(page, 1); await waitButtons(page, ['Training Menu', 'Restart', 'Leave Missions'], '두 번째 실패 안내');
  await clickLabel(page, 'Training Menu'); await waitButtons(page, ['Technology available', 'Ok'], 'Training Menu 브리핑');
  expect(await current(page), 'Training Menu 가 시작한 미션').toEqual({ script: 'bc1menu76', demo: 0 });
  await clickLabel(page, 'Ok'); await closeDialogs(page);
  expect(await page.evaluate(() => window.netstorm._gameClock.paused()), '시작 뒤 시계가 흐른다').toBe(false);
  expect(errors).toEqual([]);
});

// Priest Training 7(bc7sun_bars76) 완료 안내의 Training Menu 단추는 원본 대본이 접미 없는 `MissionBegin,BC1Menu` 라고 적혀 있다.
// 웹 자산에는 BC1Menu 대본이 없어 이전에는 아무 일도 없었다. 별칭 해석(core/mission-name-alias.js)으로 bc1menu76 이 시작되는지 확인한다. (2026-11-23 WB04-E1245)
test('Priest Training 7: 완료 안내의 Training Menu(원본 오타 BC1Menu)로 bc1menu76 이 시작된다', async ({ page }) => {
  test.setTimeout(300000); const errors = collectErrors(page);
  await startMission(page, 'Priest Training', 'bc7sun_bars76');
  expect(await current(page), '요청한 미션이 돈다').toEqual({ script: 'bc7sun_bars76', demo: 0 });
  // 대본 `[@300]` 구역은 "시작 후 300초 경과 + 열린 창 없음" 이면 열린다(raw-mission-frame-gate.js). 300초를 기다리는 대신 시작 시각을 301초 앞당긴다.
  await page.evaluate(() => { window.netstorm._clientGuide.current().runtime.gate.startSeconds -= 301; });
  // 완료 구역의 단추 네 개이다(첫 단추가 원본 오타 대본이다).
  await waitButtons(page, ['Training Menu', 'Restart', 'FINISH THEM!', 'Leave Missions'], 'Priest Training 7 완료 안내');
  await clickLabel(page, 'Training Menu'); await waitButtons(page, ['Technology available', 'Ok'], 'Training Menu 브리핑');
  expect(await current(page), 'Training Menu 가 시작한 미션').toEqual({ script: 'bc1menu76', demo: 0 });
  await clickLabel(page, 'Ok'); await closeDialogs(page);
  expect(await page.evaluate(() => window.netstorm._gameClock.paused()), '시작 뒤 시계가 흐른다').toBe(false);
  expect(errors).toEqual([]);
});
