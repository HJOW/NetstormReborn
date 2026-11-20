// 캠페인 목록의 모든 미션(Early Missions 6 + 세 캠페인 묶음 16 + Priest Training 9)을 차례로 시작해 몇 초씩 진행하고 오류 없이 도는지 실제 Chromium 에서 확인한다.
// 한 페이지 안에서 미션을 떠나고 다시 시작하는 흐름이라 미션 사이에 남는 전역 상태(예: 초기 캡처 기한)가 다음 미션을 깨는 문제도 잡는다. (2026-11-20 WB04-E1121)
const { test, expect } = require('@playwright/test');

// 실제 돌 버튼의 논리 좌표를 브라우저 마우스로 누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 15000 });
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

// 캠페인 목록의 묶음 이름과 각 묶음에서 기대하는 미션 수이다. 순서대로 잠금이 풀리므로 아래 진행 저장으로 모두 연다.
const GROUPS = [['Early Missions', 6], ['Struggle For Freedom', 6], ['A Nation Rises', 5], ['Complete Victory', 5], ['Priest Training', 9]];
const MISSIONS = ['Tutorial1', 'Tutorial2', 'Tutorial3', 'Tutorial4', 'Tutorial5', 'Tutorial6', 'TheWarBegins', 'MasterOfWhirligigs', 'SaveTheIsland', 'FragileFortune', 'ThunderingPower', 'DissolvedAlliance',
  'TheNoose', 'RainVsRain', 'RunForIt', 'GuardMyBack', 'Surrounded', 'BreakingThrough', 'ToTheRescue', 'Vicious', 'EnemyTerritory', 'FinalConfrontation',
  'bc1menu76', 'bc2bridging76', 'bc3acid_bar76', 'bc4bow_rush76', 'bc5walkers76', 'bc6mows76', 'bc7sun_bars76', 'bc8fury76', 'bc9ssun_bars76'];

test('모든 캠페인 미션을 연달아 시작해 오류 없이 진행한다', async ({ page }) => {
  test.setTimeout(900000);
  const errors = []; page.on('pageerror', (error) => errors.push(error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript((names) => {
    localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } }));
    const done = {}; for (const name of names) done[`Done${name}`] = '1'; localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: done }));
  }, MISSIONS);
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  const started = [];
  for (const [group, count] of GROUPS) {
    // 앞 묶음의 마지막 미션을 떠나 메인 메뉴로 돌아온 뒤 목록을 연다.
    await page.evaluate(() => window.netstorm._leaveCampaignMission()); await page.waitForTimeout(300);
    await clickLabel(page, 'Campaign'); await clickLabel(page, group);
    const missions = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.filter((item) => item.command?.name === 'MissionBegin').map((item) => ({ label: item.label, name: item.command.argument })));
    expect(missions.length, group).toBe(count);
    await page.evaluate(() => { window.netstorm._mainMenu.setDialog(null); window.netstorm._mainMenu.dirty = true; }); await page.waitForTimeout(300);
    for (const mission of missions) {
      // 이전 미션을 떠나 메인 메뉴로 돌아간 뒤 같은 길로 다시 들어가 시작한다.
      await page.evaluate(() => window.netstorm._leaveCampaignMission()); await page.waitForTimeout(300);
      await clickLabel(page, 'Campaign'); await clickLabel(page, group); await clickLabel(page, mission.label);
      await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 60000 });
      // 브리핑·안내 창은 마지막 단추(시작·확인)로 닫는다. 시작 몇 초 뒤에 뜨는 안내(데모형 미션)도 있어 6초 동안 계속 살피다가 창이 없는 상태가 이어지면 멈춘다.
      for (let quiet = 0; quiet < 6;) {
        const label = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons?.at(-1)?.label ?? null);
        if (label === null) { quiet += 1; await page.waitForTimeout(500); continue; }
        quiet = 0; await clickLabel(page, label); await page.waitForTimeout(400);
      }
      // 창을 모두 닫은 뒤 게임 시계를 기준으로 몇 초 진행해 시계가 흐르고 정지가 풀렸는지 확인한다.
      const before = await page.evaluate(() => window.netstorm._gameClock.gameSeconds()); await page.waitForTimeout(4000);
      const after = await page.evaluate(() => ({ seconds: window.netstorm._gameClock.gameSeconds(), paused: window.netstorm._gameClock.paused() }));
      expect(after.paused, mission.name).toBe(false); expect(after.seconds - before, mission.name).toBeGreaterThan(2);
      started.push(mission.name);
    }
  }
  expect(started).toEqual(MISSIONS); expect(errors).toEqual([]);
});
