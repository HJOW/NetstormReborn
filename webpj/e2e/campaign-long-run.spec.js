// 캠페인 목록의 31개 미션(Early Missions 6·세 캠페인 묶음 16·Priest Training 9)을 미션마다 NETSTORM_LONG_SECONDS 초 동안 진행하며 AI·전투·안내가 오류 없이 도는지 실제 Chromium 에서 확인한다. 환경 변수가 없으면 건너뛴다(100초 기준 31개에 약 1시간 10분 걸림).
// 시작 몇 초만 보는 all-missions-start 와 달리 AI 가 건설·공격을 시작한 뒤 나오는 미연결 서비스·좌표 오류를 잡는다(보호막 안내음·섬 피격 경고·AI 장애물 후보·Rain 비행 피격·포획 이름 등). 쪽지·시간 안내 창은 계속 닫는다. (2026-11-20 WB04-E1122, 초급·훈련 묶음 추가 E1123)
// 사용: `$env:NETSTORM_LONG_SECONDS=120; npx playwright test e2e/campaign-long-run.spec.js`
const { test, expect } = require('@playwright/test');

// 진행 시간(초)이다. 설정하지 않으면 시험을 건너뛴다.
const SECONDS = Number(process.env.NETSTORM_LONG_SECONDS ?? 0);
// 진행할 미션을 이름으로 제한하는 선택 목록이다(쉼표로 구분, 비우면 31개 전부). 사용: `$env:NETSTORM_LONG_ONLY='FinalConfrontation,Vicious'`. (2026-11-21 WB04-E1132)
const ONLY = (process.env.NETSTORM_LONG_ONLY ?? '').split(',').map((name) => name.trim()).filter(Boolean);
// 실제 돌 버튼의 논리 좌표를 브라우저 마우스로 누른다.
async function clickLabel(page, label) {
  await page.waitForFunction((text) => Boolean(window.netstorm?._mainMenu?.controlPoint(text)), label, { timeout: 15000 });
  const point = await page.evaluate((text) => {
    const app = window.netstorm, logical = app._mainMenu.controlPoint(text), rect = app.canvas.getBoundingClientRect();
    return { x: rect.left + logical.x * rect.width / app._mainMenu.width, y: rect.top + logical.y * rect.height / app._mainMenu.height };
  }, label);
  await page.mouse.click(point.x, point.y, { delay: 60 });
}

// 캠페인 묶음과 기대하는 미션 수, 진행 저장으로 잠금을 풀 미션 이름이다.
const GROUPS = [['Early Missions', 6], ['Struggle For Freedom', 6], ['A Nation Rises', 5], ['Complete Victory', 5], ['Priest Training', 9]];
const MISSIONS = ['Tutorial1', 'Tutorial2', 'Tutorial3', 'Tutorial4', 'Tutorial5', 'Tutorial6', 'TheWarBegins', 'MasterOfWhirligigs', 'SaveTheIsland', 'FragileFortune', 'ThunderingPower', 'DissolvedAlliance', 'TheNoose', 'RainVsRain', 'RunForIt', 'GuardMyBack', 'Surrounded',
  'BreakingThrough', 'ToTheRescue', 'Vicious', 'EnemyTerritory', 'FinalConfrontation',
  'bc1menu76', 'bc2bridging76', 'bc3acid_bar76', 'bc4bow_rush76', 'bc5walkers76', 'bc6mows76', 'bc7sun_bars76', 'bc8fury76', 'bc9ssun_bars76'];

test('캠페인 31개 미션을 미션마다 길게 진행해도 오류가 없다', async ({ page }) => {
  test.skip(!(SECONDS > 0), 'NETSTORM_LONG_SECONDS 를 설정한 실행에서만 돈다');
  test.setTimeout((SECONDS + 40) * 1000 * MISSIONS.length + 120000);
  // 오류는 스택까지 모아 어느 경로에서 났는지 알 수 있게 한다.
  const errors = []; page.on('pageerror', (error) => errors.push(error.stack ?? error.message)); page.on('console', (message) => { if (message.type() === 'error') errors.push(message.text()); });
  await page.addInitScript((names) => {
    localStorage.setItem('netstorm.settings', JSON.stringify({ version: 1, values: { welcomeGiven: 1, tellTips: 0, sound: 0, music: 0 } }));
    const done = {}; for (const name of names) done[`Done${name}`] = '1'; localStorage.setItem('netstorm.progress', JSON.stringify({ version: 1, values: done }));
  }, MISSIONS);
  await page.setViewportSize({ width: 1024, height: 768 }); await page.goto('/?aspect=4:3');
  await page.waitForFunction(() => window.netstorm?.state === 'clickToStart'); await page.locator('canvas').click();
  await page.waitForFunction(() => window.netstorm?.state === 'mainMenu');
  // 게임 시계 정지·재개 호출을 호출 위치와 함께 기록한다. 시계가 멈춘 채 다음 미션이 시작되는 누수를 찾는 용도이다. (2026-11-21 WB04-E1132)
  await page.evaluate(() => {
    const clock = window.netstorm._gameClock; window.__pauseLog = []; window.__currentMission = '';
    for (const name of ['pushGamePause', 'popGamePause']) {
      const original = clock[name].bind(clock);
      clock[name] = () => { window.__pauseLog.push({ name, mission: window.__currentMission, game: clock.gameSeconds(), stack: new Error().stack.split('\n').slice(2, 6).map((line) => line.trim().replace(/\(.*\//, '(')).join(' < ') }); return original(); };
    }
  });
  const ran = [];
  for (const [group, count] of GROUPS) {
    await page.evaluate(() => window.netstorm._leaveCampaignMission()); await page.waitForTimeout(300);
    await clickLabel(page, 'Campaign'); await clickLabel(page, group);
    const missions = await page.evaluate(() => window.netstorm._mainMenu.dialog.menu.items.filter((item) => item.command?.name === 'MissionBegin').map((item) => ({ label: item.label, name: item.command.argument })));
    expect(missions.length, group).toBe(count);
    await page.evaluate(() => { window.netstorm._mainMenu.setDialog(null); window.netstorm._mainMenu.dirty = true; }); await page.waitForTimeout(300);
    for (const mission of missions) {
      // NETSTORM_LONG_ONLY(쉼표로 나눈 미션 이름)가 있으면 그 미션만 진행한다. 후반 미션만 오래(수 분) 돌려 볼 때 쓴다. (2026-11-21 WB04-E1132)
      if (ONLY.length > 0 && !ONLY.includes(mission.name)) continue;
      await page.evaluate((name) => { window.__currentMission = `${name}:leave`; window.netstorm._leaveCampaignMission(); }, mission.name); await page.waitForTimeout(300);
      await page.evaluate((name) => { window.__currentMission = name; }, mission.name);
      await clickLabel(page, 'Campaign'); await clickLabel(page, group); await clickLabel(page, mission.label);
      await page.waitForFunction(() => window.netstorm?._campaignDisplayActive() && window.netstorm._clientGuide.current()?.runtime.gate.started === 1, null, { timeout: 60000 });
      // 진행하는 동안 1초마다 안내·쪽지 창이 떠 있으면 마지막 단추로 닫는다(시계를 멈추는 창이 뜨는 미션이 있다).
      const before = await page.evaluate(() => window.netstorm._gameClock.gameSeconds());
      for (let tick = 0; tick < SECONDS; tick += 1) {
        await page.waitForTimeout(1000);
        const label = await page.evaluate(() => window.netstorm._mainMenu.dialog?.buttons?.at(-1)?.label ?? null);
        if (label !== null) await clickLabel(page, label);
      }
      const after = await page.evaluate(() => window.netstorm._gameClock.gameSeconds());
      // 실패 때 원인을 바로 볼 수 있게 시계·창·입력 큐 상태를 메시지에 담는다. (2026-11-21 WB04-E1132)
      const debug = await page.evaluate(() => { const app = window.netstorm; return JSON.stringify({ paused: app._gameClock.paused(), dialog: app._mainMenu.dialog?.constructor?.name ?? null, head: app._clientInputQueue?.peek?.() ?? null, display: app._campaignDisplayActive(), state: app.state, menuPause: app._menuPauseDepth, hiddenPause: app._hiddenGamePause, log: window.__pauseLog.slice(-14) }); }).catch((error) => `평가 실패 ${error.message}`);
      expect(errors, mission.name).toEqual([]); expect(after - before, `${mission.name} before=${before} after=${after} ${debug}`).toBeGreaterThan(SECONDS * 0.5);
      ran.push(mission.name);
    }
  }
  expect(ran).toEqual(ONLY.length > 0 ? MISSIONS.filter((name) => ONLY.includes(name)) : MISSIONS);
});
