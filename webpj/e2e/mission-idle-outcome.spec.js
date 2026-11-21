// 플레이어가 아무것도 하지 않을 때 적 AI 가 스스로 승패를 만드는지(자연 패배) 실제 시간으로 지켜본다. 옵트인 시험이다. (2026-11-21 WB04-E1161)
// 사용: `$env:NETSTORM_IDLE_SECONDS=600; $env:NETSTORM_IDLE_MISSIONS='Complete Victory:Vicious,A Nation Rises:Surrounded'; npx playwright test e2e/mission-idle-outcome.spec.js`
// AI 는 프레임 수에 묶여 있어 가짜 시계 빨리 감기로는 대신할 수 없다(docs/webpj-fast-forward-clock-20261121.md). 승패 안내 창(Failed/Succeeded 구역의 단추)이 뜨면 시각과 단추를 기록하고 그 미션 지켜보기를 마친다.
const { test, expect } = require('@playwright/test');

// 미션마다 지켜볼 게임 시간(초)이다. 설정하지 않으면 시험을 건너뛴다.
const SECONDS = Number(process.env.NETSTORM_IDLE_SECONDS ?? 0);
// 지켜볼 미션 목록 `묶음:미션,…` 이다.
const TARGETS = (process.env.NETSTORM_IDLE_MISSIONS ?? 'Complete Victory:Vicious').split(',').map((entry) => entry.trim()).filter(Boolean).map((entry) => entry.split(':'));
const MISSIONS = ['Tutorial1', 'Tutorial2', 'Tutorial3', 'Tutorial4', 'Tutorial5', 'Tutorial6', 'TheWarBegins', 'MasterOfWhirligigs', 'SaveTheIsland', 'FragileFortune', 'ThunderingPower', 'DissolvedAlliance', 'TheNoose', 'RainVsRain', 'RunForIt', 'GuardMyBack', 'Surrounded',
  'BreakingThrough', 'ToTheRescue', 'Vicious', 'EnemyTerritory', 'FinalConfrontation', 'bc1menu76', 'bc2bridging76', 'bc3acid_bar76', 'bc4bow_rush76', 'bc5walkers76', 'bc6mows76', 'bc7sun_bars76', 'bc8fury76', 'bc9ssun_bars76'];

// 실제 돌 버튼의 논리 좌표를 브라우저 마우스 좌표로 바꿔 누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 30000 });
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

// 승패 안내로 보이는 단추 이름 조합이다(미션 시작 브리핑의 Play Mission·OK 등과 구별한다).
const OUTCOME_BUTTONS = /Try again|Again!|Restart|Continue|Leave Missions|Next Mission|Training Menu/u;

test('플레이어가 가만히 있을 때 적 AI 가 만드는 승패를 지켜본다(옵트인)', async ({ page }) => {
  test.skip(!(SECONDS > 0), 'NETSTORM_IDLE_SECONDS 를 설정한 실행에서만 돈다');
  test.setTimeout((SECONDS + 120) * 1000 * TARGETS.length + 120000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript((names) => {
    localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0, autoDemo: 0 } }));
    const done = {}; for (const name of names) done[`Done${name}`] = '1'; localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: done }));
  }, MISSIONS);
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  const results = [];
  for (const [group, mission] of TARGETS) {
    await page.evaluate(() => window.netstorm._leaveCampaignMission()); await page.waitForTimeout(300);
    await clickLabel(page, 'Campaign'); await clickLabel(page, group);
    const label = await page.evaluate((name) => window.netstorm._mainMenu.dialog.menu.items.find((item) => item.command?.name === 'MissionBegin' && item.command.argument === name)?.label, mission);
    await clickLabel(page, label);
    await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 120000 });
    // 시작 안내 창을 닫는다.
    for (let quiet = 0; quiet < 6;) {
      const button = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons?.at(-1)?.label ?? null);
      if (button === null) { quiet += 1; await page.waitForTimeout(500); continue; }
      quiet = 0; await clickLabel(page, button); await page.waitForTimeout(400);
    }
    const startedAt = await page.evaluate(() => window.netstorm._gameClock.gameSeconds()); let outcome = null;
    for (let tick = 0; tick < SECONDS && !outcome; tick += 5) {
      await page.waitForTimeout(5000);
      const state = await page.evaluate(() => ({ buttons: window.netstorm._mainMenu.dialog?.buttons?.map((button) => button.label) ?? null, game: Math.round(window.netstorm._gameClock.gameSeconds()), dead: window.netstorm._campaignWorld?.session().missionPriestDead(1) ?? null }));
      if (state.buttons && OUTCOME_BUTTONS.test(state.buttons.join('/'))) outcome = { ...state, after: Math.round(state.game - startedAt) };
      else if (state.buttons) await clickLabel(page, state.buttons.at(-1));
    }
    results.push({ mission, outcome: outcome ?? '없음(시간 끝)' });
    console.log('IDLE', mission, JSON.stringify(outcome ?? '없음(시간 끝)'));
  }
  test.info().annotations.push({ type: 'idle-outcomes', description: JSON.stringify(results) });
  expect(errors).toEqual([]);
});
